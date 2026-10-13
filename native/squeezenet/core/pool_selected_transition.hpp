#pragma once
#include "squeezenet/core/carry_stride_math.hpp"
#include "squeezenet/core/wave_policy.hpp"
#include <openssl/crypto.h>
#ifndef AQCF_POOL_SELECTED
#define AQCF_POOL_SELECTED 0
#endif
static_assert(AQCF_POOL_SELECTED==0||AQCF_POOL_SELECTED==1,"public pool candidate flag");
namespace aqcf_pool_selected {
using namespace aqcf2;
// Existing secret-carry recurrence, now connected to the dominant pool path.
// One fresh selected response instead of two prepared cofactor responses and
// a separate ROT selector. No secret branch, input, carry or mask is opened.
template<class B> Bits carry(B& s,const std::vector<U>& own,unsigned width){
 const auto n=own.size();
 require((width==24||width==25)&&n>0&&n<=aqcf_wan_wave::pool_limit,
         "public selected-pool dimensions");
 require(s.compact_cofactor,"pinned compact protected pool response");
 Bits state(n);U rows=0,table_bits=0;
 for(unsigned pos=0;pos<width;){
  const unsigned d=std::min(5U,width-pos),in=d+(pos?1:0),entries=1U<<in;
  std::vector<U> choice(n),table(s.party==1?n:0);
  std::vector<std::uint8_t> pad;
  if(s.party==1)pad=s.random_small(n);
  for(std::size_t j=0;j<n;++j){
   const U digit=(own[j]>>pos)&mask(d),c=state.v[j];
   choice[j]=pos?(digit<<1)|c:digit;
   if(s.party==1){
    auto packed=aqcf_stride6_math::row(d,digit,c,pad[j]&1,pos==0);
    require(packed[1]==0,"selected pool compact row fits64bits");
    table[j]=packed[0];
   }
  }
  auto got=s.lookup_words(table,choice,n,in,1,true);s.flush();
  for(std::size_t j=0;j<n;++j)state.v[j]=s.party==1?(pad[j]&1):got[j];
  state.check();rows+=n;table_bits+=U(n)*entries;pos+=d;
  if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
  if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 }
 s.note_linked_execution(0,0,rows,table_bits,0);
 return state;
}
}
