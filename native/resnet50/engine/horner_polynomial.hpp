#pragma once
#include "resnet50/core/batch_norm_affine_core.hpp"
#include <seal/util/ntt.h>
#include <seal/util/uintarithsmallmod.h>
namespace horner292 {
namespace b=aqcf_bn19;using b::U;using b::I;using b::W;
struct Counts {U switches=0,extra_zeros=0,products=0,compiled_words=0;};
inline unsigned baby(unsigned C){b::require(C==8||C==32||C==128,"292 fixed changed C");return C==8?2:C==32?4:8;}
inline U reverse13(U x){U y=0;for(unsigned k=0;k<13;++k){y=(y<<1)|(x&1);x>>=1;}return y;}
struct Spectrum {
 unsigned C;std::array<std::vector<seal::util::MultiplyUIntModOperand>,3> f;
 Spectrum(const std::vector<I>& weights,const seal::SEALContext& context):C(weights.size()){
  b::require(C==8||C==32||C==128,"292 fixed spectrum");
  for(auto w:weights)b::require(w>=-I(U(1)<<30)&&w<=I(U(1)<<30),"292 complete owner bound");
  const auto& cd=*context.first_context_data();auto qs=cd.parms().coeff_modulus();
  for(unsigned z=0;z<3;++z){const auto& q=qs[z];U omega=b::power(cd.small_ntt_tables()[z].get_root(),2*b::N/C,q.value());
   U inv=b::power(omega,q.value()-2,q.value());b::require(b::power(omega,C,q.value())==1&&b::power(omega,C/2,q.value())!=1,"292 primitive C root");
   f[z].resize(C);
   for(U k=0;k<C;++k){U step=b::power(inv,k,q.value()),p=1,sum=0;
    for(U v=0;v<C;++v){U term=U(W(b::mod(weights[v],q.value()))*p%q.value());sum+=term;if(sum>=q.value())sum-=q.value();p=U(W(p)*step%q.value());}
    f[z][k].set(sum,q);
   }
  }
 }
 void validate_dense(const std::vector<I>& weights,const seal::SEALContext& context)const{
  const auto& cd=*context.first_context_data();auto qs=cd.parms().coeff_modulus();
  for(unsigned z=0;z<3;++z)for(U t=0;t<C;++t){std::vector<U> a(b::N,0);const U q=qs[z].value();
   for(U r=0;r<C;++r){U e=b::mod(-b::S(2*b::N/C)*r*t,2*b::N);U v=b::mod(e<b::N?b::S(weights[r]):-b::S(weights[r]),q);auto& x=a[e%b::N];x+=v;if(x>=q)x-=q;}
   seal::util::ntt_negacyclic_harvey(a.data(),cd.small_ntt_tables()[z]);
   for(U j=0;j<b::N;++j)b::require(a[j]==f[z][(t*(2*reverse13(j)+1))%C].operand,"292 independent full dense NTT spectrum");
  }
 }
};
inline W error_bound(const b::Lane& lane,unsigned C){
 const auto qs=lane.context->first_context_data()->parms().coeff_modulus();W sum=0;for(auto q:qs)sum+=q.value()-1;
 U p=lane.context->key_context_data()->parms().coeff_modulus().back().value();W K=(W(b::N)*21*sum+p-1)/p+(b::N+1),cap=U(1)<<30;
 if(C==2)return W(C)*C*(22+K)*cap+8194;
 U m=baby(C),h=C/m;return 22*W(C)*C*cap+W(C)*C*(m-1)*cap*K/2+(h-1)*K+2*8194;
}
inline void validate_noise(const b::Lane& lane){
 for(unsigned C:{2,8,32,128}){W e=error_bound(lane,C),wire=(W(1)<<64)+W(b::N)*(W(1)<<51);
  b::require(W(8)*b::T*(e+wire)<lane.codec->Q,"292 NEW complete noise and unchanged codec margin");
 }
}
inline seal::Ciphertext action(const seal::Ciphertext& input,const std::vector<I>& weights,b::Lane& lane,Counts& count){
 const unsigned C=weights.size();b::require(input.size()==2&&!input.is_ntt_form()&&input.coeff_modulus_size()==3,"292 original ciphertext");
 const auto qs=lane.context->first_context_data()->parms().coeff_modulus();
 if(C==2){return b::trace(input,weights,*lane.ev,lane.gk,qs,count.switches);}
 const unsigned m=baby(C),h=C/m;const U small=1+2*b::N/C,giant=1+2*b::N*m/C;
 b::require(lane.gk.has_key(small)&&lane.gk.has_key(giant),"292 ORIGINAL keys only");
 Spectrum spectrum(weights,*lane.context);count.compiled_words+=3*C;
 std::vector<seal::Ciphertext> v(m);auto current=input;
 for(unsigned u=0;u<m;++u){if(u){lane.ev->apply_galois_inplace(current,small,lane.gk);++count.switches;}v[u]=current;lane.ev->transform_to_ntt_inplace(v[u]);}
 // One initial zero survives through the entire Horner accumulator.
 // The caller still adds the unchanged mandatory fresh final zero.
 seal::Ciphertext out;lane.enc->encrypt_zero(out);++count.extra_zeros;
 std::array<U,b::N> index;for(U j=0;j<b::N;++j)index[j]=(2*reverse13(j)+1)%C;
 for(int step=int(h)-1;step>=0;--step){
  lane.ev->transform_to_ntt_inplace(out);
  for(unsigned u=0;u<m;++u){const U t=U(step)*m+u;
   for(unsigned z=0;z<3;++z){const auto& q=qs[z];for(unsigned c=0;c<2;++c){
    U* dst=out.data(c)+z*b::N;const U* src=v[u].data(c)+z*b::N;
    for(U j=0;j<b::N;++j){U value=seal::util::multiply_uint_mod(src[j],spectrum.f[z][t*index[j]%C],q);U sum=dst[j]+value;if(sum>=q.value())sum-=q.value();dst[j]=sum;}
   }}++count.products;
  }
  lane.ev->transform_from_ntt_inplace(out);
  if(step){lane.ev->apply_galois_inplace(out,giant,lane.gk);++count.switches;}
 }
 return out;
}
}