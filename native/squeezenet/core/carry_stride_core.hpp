#pragma once
// Experimental wide-digit consumer of the already pinned fresh KKOT banks.
// Opt-in only; legacy cohost entry point remains the default.
#include "squeezenet/engine/carry_token.hpp"
#include "squeezenet/core/wave_policy.hpp"
#include "squeezenet/core/carry_stride_math.hpp"
#include <openssl/crypto.h>
#ifndef AQCF_CARRY_STRIDE6
#define AQCF_CARRY_STRIDE6 0
#endif
static_assert(AQCF_CARRY_STRIDE6==0||AQCF_CARRY_STRIDE6==1,"explicit public candidate flag");
namespace aqcf_stride6 {
using namespace aqcf2;
struct Result {Bits carry,tap;U rows=0,bits=0,calls=0;};
template<class B> Result run(B& s,const std::vector<U>& own,unsigned width,
 const Bits& seed,bool public_zero,unsigned tap_width){
 seed.check();const auto n=own.size();
 require(n>0&&n<=aqcf_wan_wave::relu_limit&&seed.v.size()==n,"stride6 public dimensions");
 require(s.compact_cofactor,"stride6 pinned compact backend");
 if(public_zero)for(auto v:seed.v)require(v==0,"stride6 explicit public zero seed");
 const auto plan=aqcf_stride6_math::schedule(width,tap_width,public_zero);
 Result result;result.carry=seed;
 for(const auto& digit:plan){
  const unsigned in=digit.width+(digit.free_seed?0:1),entries=1U<<in;
  const std::size_t words=(entries+63)/64;
  std::vector<U> choices(n),table(s.party==1?n*words:0);
  std::vector<std::uint8_t> pads;
  if(s.party==1)pads=s.random_small(n);
  for(std::size_t j=0;j<n;++j){
   const U a=(own[j]>>digit.pos)&mask(digit.width),c=result.carry.v[j];
   choices[j]=digit.free_seed?a:(a<<1)|c;
   if(s.party==1){
    const auto packed=aqcf_stride6_math::row(digit.width,a,c,pads[j]&1,digit.free_seed);
    for(std::size_t k=0;k<words;++k)table[j*words+k]=packed[k];
   }
  }
  // Never reuse a previous digit's OT/PRG state, response pad or output mask.
  auto got=s.lookup_words(table,choices,n,in,1,true);s.flush();
  for(std::size_t j=0;j<n;++j)result.carry.v[j]=s.party==1?(pads[j]&1):got[j];
  result.carry.check();result.rows+=n;result.bits+=U(n)*entries;++result.calls;
  if(digit.pos+digit.width==tap_width)result.tap=result.carry;
  if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
  if(!pads.empty())OPENSSL_cleanse(pads.data(),pads.size());
 }
 if(tap_width)require(result.tap.v.size()==n,"stride6 complete exact low tap");
 return result;
}
}
