#pragma once
#include "resnet50/engine/quotient_recovery_core.hpp"
#include <openssl/crypto.h>
namespace floor216 {
using namespace aqcf2;
// c = alpha XOR beta is the incoming carry into the top coarse bit.
// ceil((p+t+c)/2) = (p OR alpha)+(t OR beta)
//                   -alpha*beta-(p XOR alpha)*(t XOR beta).
// The two cross products live in Ring12 because their coefficient is 2^25.
template<class B> Ring emit(B& s,const Ring& a,const Bits& low,const Bits& carry){
 const auto n=a.v.size();a.check();low.check();carry.check();
 require(a.width==25&&n>0&&n<=200704&&low.v.size()==n&&carry.v.size()==n,"216 full residual terminal domain");
 std::vector<U> lc(n),hc(2*n);std::vector<uint8_t> ls(n),hs(2*n);
 for(size_t j=0;j<n;++j){
  const U top=a.v[j]>>24,bit=carry.v[j];
  if(s.party==1){lc[j]=(U(1)-2*U(low.v[j]))&mask(37);hc[j]=bit;hc[n+j]=top^bit;}
  else{ls[j]=low.v[j];hs[j]=bit;hs[n+j]=top^bit;}
 }
 auto l=s.owned_word_bit_product(1,lc,ls,37);
 auto h=s.owned_word_bit_product(1,hc,hs,12);
 Ring out(37,n);
 for(size_t j=0;j<n;++j){
  const U top=a.v[j]>>24,bit=carry.v[j];
  U high=(h.v[j]+h.v[n+j]-(top|bit))&mask(12);
  out.v[j]=(a.v[j]+l.v[j]+(s.party==1?U(low.v[j]):0)+(high<<25))&mask(37);
 }
 for(auto* v:{&lc,&hc,&l.v,&h.v})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 for(auto* v:{&ls,&hs})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());
 out.check();return out;
}
}
