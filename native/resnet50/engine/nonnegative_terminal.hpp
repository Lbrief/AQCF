#pragma once
#include "resnet50/engine/ring37_direct.hpp"
#include <openssl/crypto.h>

// Exact nonnegative quotient terminal. No input range narrowing:
// gate/carry are produced by the unchanged full signed-domain prefix.
namespace aqcf_nonnegative_terminal {
using namespace aqcf2;
template<class B> Ring emit(B& s,const Ring& a,const Bits& low,const Bits& carry) {
 a.check();low.check();carry.check();
 const unsigned w=a.width,hi=37-w;const auto n=a.v.size();
 require(w>=4&&w<37&&n&&low.v.size()==n&&carry.v.size()==n,"bound terminal widths");
 const U mw=mask(w),mh=mask(hi);
 std::vector<U> choice(n),table,pads;std::vector<std::uint8_t> beta;
 if(s.party==1){pads=s.random_ring_words(n,w);beta=s.random_small(n);table.resize(4*n);}
 for(std::size_t j=0;j<n;++j){
  const U gs=((a.v[j]>>(w-1))&1)^carry.v[j]^U(s.party==1);
  choice[j]=gs|(U(low.v[j])<<1);
  if(s.party==1)for(U c=0;c<4;++c){
   const U g=gs^(c&1),d=U(low.v[j])^(c>>1);
   table[4*j+c]=(((g*(a.v[j]+d)-pads[j])&mw)<<1)|(g^(beta[j]&1));
  }
 }
 const auto received=s.lookup_words(table,choice,n,2,w+1);
 Ring small(w,n);std::vector<U> coefficient(n);std::vector<std::uint8_t> selector(n);
 for(std::size_t j=0;j<n;++j){
  if(s.party==1){small.v[j]=pads[j];selector[j]=beta[j]&1;}
  else{const U g=received[j]&1;small.v[j]=((received[j]>>1)+a.v[j]*g)&mw;coefficient[j]=(a.v[j]*(U(1)-2*g))&mw;}
 }
 auto product=s.owned_word_bit_product(2,coefficient,selector,w);
 for(std::size_t j=0;j<n;++j)small.v[j]=(small.v[j]+product.v[j])&mw;
 // The reconstructed small value is in [0,2^(w-1)); hence wrap of its
 // two canonical w-bit shares is OR(their top bits), with no new prefix.
 std::vector<U> lift_choice(n),lift_table,high_pad;
 if(s.party==1){high_pad=s.random_ring_words(n,hi);lift_table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){
  const U top=small.v[j]>>(w-1);lift_choice[j]=top;
  if(s.party==1)for(U c=0;c<2;++c)lift_table[2*j+c]=(U(0)-(top|c)-high_pad[j])&mh;
 }
 const auto high=s.lookup_words(lift_table,lift_choice,n,1,hi);
 Ring out(37,n);
 for(std::size_t j=0;j<n;++j)out.v[j]=(small.v[j]+((s.party==1?high_pad[j]:high[j])<<w))&mask(37);
 for(auto* v:{&table,&pads,&lift_table,&high_pad})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 if(!beta.empty())OPENSSL_cleanse(beta.data(),beta.size());
 out.check();return out;
}
}
