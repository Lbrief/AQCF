#pragma once
// Experimental extension of the AQCF quotient action to non-power-of-two
// public channel spacing. No lower-degree ciphertext scheme, pruning, model
// change or approximate arithmetic. NTT/pruned FFT are established tools;
// research priority for this arrangement is NOT claimed here.
#include "resnet50/core/public_tile_weight.hpp"
#include <numeric>
#include <tuple>

namespace aqcf3::stride_weight {
struct TruncatedAction {
 U d,q,span,block;
 std::vector<U> reverse;
 std::vector<ActionMultiplier> twist;
 std::vector<std::vector<ActionMultiplier>> stages;
 TruncatedAction(U degree,U prime,U support):d(degree),q(prime),span(1){
  require(d>=2&&d<=DEGREE&&!(d&(d-1))&&support&&support<=d,"public normalized action shape");
  while(span<support)span*=2;
  block=d/span;reverse.resize(span);twist.resize(span);
  U generator=2;while(generator<4096&&action_pow(generator,(q-1)/2,q)!=q-1)++generator;
  require(generator<4096,"public nonresidue bound");
  U psi=action_pow(generator,(q-1)/(2*d),q),omega=U(__uint128_t(psi)*psi%q),v=1;
  require(action_pow(psi,d,q)==q-1,"primitive normalized action root");
  for(U k=0;k<span;++k){twist[k]=ActionMultiplier(v,q);v=U(__uint128_t(v)*psi%q);
   if(k)reverse[k]=(reverse[k>>1]>>1)|((k&1)?span>>1:0);}
  // Before this stage all butterfly right inputs are publicly zero. Replicate
  // the corresponding twisted coefficient exactly, without multiplying zeros.
  for(U len=2*block;len<=d;len*=2){std::vector<ActionMultiplier> roots(len/2);
   U root=action_pow(omega,d/len,q),w=1;for(U j=0;j<len/2;++j){roots[j]=ActionMultiplier(w,q);w=U(__uint128_t(w)*root%q);}stages.push_back(std::move(roots));}
 }
 std::vector<U> weight(const Edge& e,U stride)const{
  std::vector<U> dense(span,0),values(d);
  for(const auto& a:e.coefficients){require(a.exponent%stride==0&&a.exponent/stride<span,"frozen public stride support");
   U m=magnitude(a.value);require(m<q,"original signed coefficient cap");dense[a.exponent/stride]=a.value<0?q-m:m;}
  for(U k=0;k<span;++k){U i=reverse[k];U v=twist[i].apply(dense[i],q);std::fill_n(values.data()+k*block,block,v);}
  U stage=0;for(U len=2*block;len<=d;len*=2,++stage){const auto& roots=stages[stage];
   for(U start=0;start<d;start+=len)for(U j=0;j<len/2;++j){
    // Established lazy modular butterfly, maintaining public [0,4q) bounds.
    // Shoup's omitted final correction gives a remainder in [0,2q), because
    // the input is <4q<2^64. No approximate arithmetic or support-dependent
    // omission is used; this is not a separate constant-time certification.
    U u=values[start+j];if(u>=2*q)u-=2*q;
    U y=values[start+j+len/2],quotient=U((__uint128_t(y)*roots[j].scaled)>>64);
    U v=y*roots[j].w-quotient*q;
    values[start+j]=u+v;values[start+j+len/2]=u+2*q-v;
   }}
  for(auto& v:values){if(v>=2*q)v-=2*q;if(v>=q)v-=q;}
  return values;
 }
 U butterflies()const{return (d/2)*stages.size();}
};

struct Plan {
 U stride,support,b,d;
 std::vector<TruncatedAction> transforms;
 std::vector<std::vector<U>> index;
 Plan(const seal::SEALContext& cx,U stride_,U support_):stride(stride_),support(support_),b(std::gcd<U>(DEGREE,stride)),d(DEGREE/b){
  require(stride>0&&stride<DEGREE&&support>0&&(support-1)*stride<DEGREE,"public no-wrap filter support");
  require(d>=2&&support<=d,"quotient action dimension");
  const auto& data=*cx.first_context_data();const auto& qs=data.parms().coeff_modulus();
  seal::Evaluator evaluator(cx);seal::Plaintext monomial;monomial.resize(DEGREE);monomial[stride]=1;
  evaluator.transform_to_ntt_inplace(monomial,cx.first_parms_id());
  for(unsigned r=0;r<qs.size();++r){U q=qs[r].value();transforms.emplace_back(d,q,support);
   ActionNTT calibration(d,q);Edge y;y.coefficients.push_back({1,1});auto roots=calibration.weight(y,1);
   std::unordered_map<U,U> positions;for(U k=0;k<d;++k)require(positions.emplace(roots[k],k).second,"unique action root");
   std::vector<U> map(DEGREE),counts(d,0);
   for(U j=0;j<DEGREE;++j){auto it=positions.find(monomial[r*DEGREE+j]);require(it!=positions.end(),"odd-stride quotient automorphism");map[j]=it->second;++counts[map[j]];}
   for(U n:counts)require(n==b,"complete full-degree spectrum fiber");index.push_back(std::move(map));
  }
 }
 std::vector<U> weight(const Edge& e)const{
  std::vector<U> out(transforms.size()*DEGREE);
  for(unsigned r=0;r<transforms.size();++r){auto v=transforms[r].weight(e,stride);
   for(U j=0;j<DEGREE;++j)out[r*DEGREE+j]=v[index[r][j]];}return out;
 }
};

inline std::vector<U> spectrum(const seal::SEALContext& cx,const Edge& e,U stride,U support,U old_spacing){
 if(!stride)return tile_weight::spectrum(cx,e,old_spacing);
 // Only the public context, stride and maximum support select a schedule.
 // No private coefficient equality/zero-dependent trimming or cached spectrum.
 thread_local std::map<std::tuple<const seal::SEALContext*,U,U>,std::unique_ptr<Plan>> plans;
 auto key=std::make_tuple(&cx,stride,support);auto it=plans.find(key);
 if(it==plans.end())it=plans.emplace(key,std::make_unique<Plan>(cx,stride,support)).first;
 return it->second->weight(e);
}
}
