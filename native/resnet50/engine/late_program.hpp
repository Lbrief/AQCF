#pragma once
#include "resnet50/core/batch_norm_affine_core.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <openssl/rand.h>
namespace late188 {
using namespace aqcf_bn19;
using Big=boost::multiprecision::int256_t;
constexpr U word_mask=(U(1)<<25)-1, phase_mask=(U(1)<<39)-1;
inline U random64(){U x;require(RAND_bytes(reinterpret_cast<unsigned char*>(&x),8)==1,"188 RNG");return x;}
inline W random_below(W bound){require(bound>0,"188 positive bound");W x,cut=(W(0)-bound)%bound;do{require(RAND_bytes(reinterpret_cast<unsigned char*>(&x),16)==1,"188 wide RNG");}while(x<cut);return x%bound;}
inline U invodd(U x){require(x&1,"188 odd inverse");U v=1;for(int j=0;j<6;++j)v*=2-x*v;return v;}
inline Big floor_div(Big a,const Big& b){Big q=a/b,r=a%b;if(r<0)--q;return q;}
struct Phase {
 std::vector<U> qs,factor;std::vector<W> part;W Q=1,L;U t,gamma,neg_t,neg_g,inv_g;
 Phase(const std::vector<seal::Modulus>& primes,U plain,U gam):t(plain),gamma(gam){
  require(t==(U(1)<<39)||t==(U(1)<<46),"188 original quarter scale");
  require(primes.size()==2||primes.size()==3,"188 original data limbs");
  for(auto p:primes){qs.push_back(p.value());Q*=p.value();}
  require(Q<(W(1)<<115),"188 bounded original Q");
  for(U q:qs){W qi=Q/q;part.push_back(qi);U tg=U(W(t%q)*(gamma%q)%q);factor.push_back(U(W(tg)*power(U(qi%q),q-2,q)%q));}
  neg_t=(U(0)-invodd(U(Q)))&(t-1);
  neg_g=gamma-power(U(Q%gamma),gamma-2,gamma);inv_g=invodd(gamma)&(t-1);
  Big num=Big(Q)*(gamma+qs.size()),den=Big(t)*gamma;
  L=static_cast<W>((num+den-1)/den+2);require(L<Q,"188 envelope size");
 }
 U scalar(const U* residues,U at,U stride)const{
  W total=0;for(U z=0;z<qs.size();++z){require(residues[z*stride+at]<qs[z],"188 canonical residue");total+=W(U(W(residues[z*stride+at])*factor[z]%qs[z]))*part[z];}
  const U a=U(W(U(total)&(t-1))*neg_t)&(t-1),b=U(W(U(total%gamma))*neg_g%gamma);
  const U centered_difference=b>gamma/2?(a+gamma-b):(a-b);
  return U(W(centered_difference&(t-1))*inv_g)&(t-1);
 }
 U scalar(W value)const{std::array<U,3> r{};for(U z=0;z<qs.size();++z)r[z]=U(value%qs[z]);return scalar(r.data(),0,1);}
 W point(U y,W offset)const{require(y<t&&offset<L,"188 envelope point");Big lo=floor_div((2*Big(y)-1)*Big(Q),2*Big(t));Big x=(lo+Big(offset))%Big(Q);if(x<0)x+=Big(Q);return static_cast<W>(x);}
};
struct Sample {W R=0,full_k0=0,full_k1=0;U k0=0,k1=0,y=0,coarse=0;unsigned low=0,gate=0,high=0,attempts=0;};
inline Sample sample(const Phase& phase,U correction,unsigned channel_shift){
 require(correction<M&&channel_shift<=7,"188 canonical late offset");
 require((phase.t==(U(1)<<39)&&channel_shift==0)||phase.t==(U(1)<<46),"188 profile mapping");
 for(unsigned attempt=1;attempt<=128;++attempt){
  Sample v;v.attempts=attempt;
  require(RAND_bytes(reinterpret_cast<unsigned char*>(&v.full_k0),16)==1&&RAND_bytes(reinterpret_cast<unsigned char*>(&v.full_k1),16)==1,"188 ideal public RM tuples");
  v.k0=U(v.full_k0)&((U(1)<<26)-1);v.k1=U(v.full_k1)&((U(1)<<26)-1);
  U diff=(v.k1-v.k0)&word_mask;v.low=unsigned((v.k0^v.k1)>>25);v.gate=unsigned(random64()&1);
  v.coarse=((v.gate?U(0)-diff:diff)-v.low)&word_mask;v.high=1U^unsigned(v.coarse>>24)^v.gate;
  U final_own=(v.coarse<<12)|(random64()&4095),raw_own=(final_own-correction)&(M-1);
  U u=(U(0)-4*raw_own-(random64()&3))&phase_mask;
  v.y=u;
  if(phase.t==(U(1)<<46)){U extra=random64()&127;v.y=(u<<channel_shift)|(extra&((U(1)<<channel_shift)-1))|((extra>>channel_shift)<<(39+channel_shift));}
  v.R=phase.point(v.y,random_below(phase.L));
  if(phase.scalar(v.R)==v.y)return v;
 }
 throw std::runtime_error("188 declared sampler exhaustion; no retry");
}
inline U winding(U c,U s){return c==3&&s==0?1:c==0&&s==3?U(0)-1:0;}
inline U recover(U c,U s){return ((c>>2)-(s>>2)+winding(c&3,s&3))&(M-1);}
inline U normalized_server(U s){return (U(0)-(s>>2)-U((s&3)!=0))&(M-1);}
}
