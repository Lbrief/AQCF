#pragma once
// Exact public radix scheduling and local private-table representation only.
// Larger radix is an established idea, not a new OT/security primitive.
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace aqcf_stride6_math {
using U=std::uint64_t;
inline void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
struct Digit { unsigned pos,width; bool free_seed; };
inline std::vector<Digit> schedule(unsigned width,unsigned tap,bool public_zero){
 check(width>=1&&width<=54&&(!tap||tap<width),"stride6 public width/tap");
 std::vector<Digit> out;
 for(unsigned pos=0;pos<width;){
  const unsigned boundary=tap&&pos<tap?tap:width;
  const unsigned d=boundary-pos<6?boundary-pos:6;
  out.push_back({pos,d,pos==0&&public_zero});pos+=d;
 }
 return out;
}
inline U spread32(U x){
 x&=0xffffffffULL;
 x=(x|(x<<16))&0x0000ffff0000ffffULL;
 x=(x|(x<<8)) &0x00ff00ff00ff00ffULL;
 x=(x|(x<<4)) &0x0f0f0f0f0f0f0f0fULL;
 x=(x|(x<<2)) &0x3333333333333333ULL;
 return (x|(x<<1))&0x5555555555555555ULL;
}
inline U threshold(unsigned base,unsigned a,unsigned carry){
 check(base>=2&&base<=64&&a<base&&carry<2,"stride6 threshold range");
 const unsigned cut=base-a-carry;
 const U domain=base==64?~U(0):(U(1)<<base)-1;
 return cut==64?0:domain&(~U(0)<<cut);
}
inline std::array<U,2> row(unsigned d,U digit,U own_carry,U pad,bool free_seed){
 check(d>=1&&d<=6&&digit<(U(1)<<d)&&own_carry<2&&pad<2,"stride6 private row range");
 const unsigned base=1U<<d,entries=base*(free_seed?1:2);
 std::array<U,2> result{};
 if(free_seed)result[0]=threshold(base,unsigned(digit),0);
 else {
  const U even=threshold(base,unsigned(digit),unsigned(own_carry));
  const U odd =threshold(base,unsigned(digit),unsigned(own_carry^1));
  result[0]=spread32(even)|(spread32(odd)<<1);
  result[1]=spread32(even>>32)|(spread32(odd>>32)<<1);
 }
 const U lo_mask=entries>=64?~U(0):(U(1)<<entries)-1;
 if(pad){result[0]^=lo_mask;if(entries>64)result[1]^=~U(0);}
 result[0]&=lo_mask;if(entries<=64)result[1]=0;
 return result;
}
inline U cell(const std::array<U,2>& r,unsigned choice){
 check(choice<128,"stride6 table choice");return (r[choice/64]>>(choice%64))&1;
}
}
