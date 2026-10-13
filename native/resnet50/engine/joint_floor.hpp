#pragma once
#include "resnet50/core/types.hpp"
#include <openssl/crypto.h>
namespace floor304 {using namespace aqcf2;
template<class B> Ring emit(B& s,const Ring& a,const Bits& low,const Bits& carry,const std::vector<U>& tap){
 const size_t n=a.v.size();a.check();low.check();carry.check();require(a.width==25&&n&&n<=200704&&tap.size()==n&&carry.v.size()==n&&low.v.size()==n,"304 exact signed residual terminal");
 std::vector<U> hc(2*n);std::vector<uint8_t> hs(2*n);
 for(size_t j=0;j<n;++j){require(tap[j]<=mask(37),"304 full37 tap");U top=a.v[j]>>24,bit=carry.v[j];if(s.party==1){hc[j]=bit;hc[n+j]=top^bit;}else{hs[j]=bit;hs[n+j]=top^bit;}}
 auto h=s.owned_word_bit_product(1,hc,hs,12);Ring out(37,n);
 for(size_t j=0;j<n;++j){U high=(h.v[j]+h.v[n+j]-((a.v[j]>>24)|carry.v[j]))&mask(12);out.v[j]=(a.v[j]+tap[j]+(high<<25))&mask(37);}
 for(auto* v:{&hc,&h.v})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));if(!hs.empty())OPENSSL_cleanse(hs.data(),hs.size());out.check();return out;
}
}
