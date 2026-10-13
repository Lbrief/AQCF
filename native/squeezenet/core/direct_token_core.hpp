#pragma once
// PUBLIC qualification candidate: fuse digit cofactor and its selected carry.
// Reuses the earlier AQCF token recurrence and the exact same protected lookup.
// No new OT primitive; no priority/novelty claim. No private values are opened.
#include "squeezenet/engine/carry_token.hpp"
#include "squeezenet/core/wave_policy.hpp"
#include "squeezenet/core/carry_stride_core.hpp"
#include <mutex>
#include <openssl/crypto.h>
namespace aqcf_direct_carry20 {
using namespace aqcf2;
struct Result { Bits carry,tap; U rows=0,bits=0,calls=0; };
inline void check_transition(){
 static std::once_flag once;
 std::call_once(once,[]{
  for(unsigned d=1;d<=4;++d)for(U a=0;a<(U(1)<<d);++a)
   for(U b=0;b<(U(1)<<d);++b)for(U ca=0;ca<2;++ca)
    for(U cb=0;cb<2;++cb)for(U pad=0;pad<2;++pad){
     const U choice=(b<<1)|cb;
     const U cell=(((a+(choice>>1)+(ca^(choice&1)))>>d)^pad);
     require((cell^pad)==((a+b+(ca^cb))>>d),"direct secret carry recurrence");
    }
 });
}
template<class B> Result run(B& s,const std::vector<U>& own,unsigned width,
 const Bits& seed,bool public_zero,unsigned tap_width){
 if constexpr(AQCF_CARRY_STRIDE6){
  auto v=aqcf_stride6::run(s,own,width,seed,public_zero,tap_width);
  return {std::move(v.carry),std::move(v.tap),v.rows,v.bits,v.calls};
 }
 check_transition();seed.check();const auto n=own.size();
 require(n>0&&n<=aqcf_wan_wave::relu_limit&&seed.v.size()==n&&width>=1&&width<=54,
         "direct carry public dimensions");
 require(tap_width==0||(tap_width%4==0&&tap_width<width),"direct carry tap");
 require(s.compact_cofactor,"same compact protected lookup");
 if(public_zero)for(auto x:seed.v)require(x==0,"explicit public zero seed");
 Result r;r.carry=seed;
 for(unsigned pos=0;pos<width;pos+=4){
  const unsigned d=std::min(4U,width-pos);
  const bool free_seed=pos==0&&public_zero;
  const unsigned input_bits=d+(free_seed?0:1),entries=1U<<input_bits;
  std::vector<U> choices(n),table(s.party==1?n:0);
  std::vector<std::uint8_t> pads;
  if(s.party==1)pads=s.random_small(n);
  for(std::size_t j=0;j<n;++j){
   const U digit=(own[j]>>pos)&mask(d),own_carry=r.carry.v[j];
   choices[j]=free_seed?digit:(digit<<1)|own_carry;
   if(s.party==1)for(unsigned c=0;c<entries;++c){
    const U other=free_seed?c:c>>1;
    const U carry=free_seed?0:own_carry^(c&1);
    table[j]|=((((digit+other+carry)>>d)^(pads[j]&1))<<c);
   }
  }
  // One fresh chosen lookup replaces a packet plus a separate selection bank.
  auto got=s.lookup_words(table,choices,n,input_bits,1,true);s.flush();
  for(std::size_t j=0;j<n;++j)r.carry.v[j]=s.party==1?(pads[j]&1):got[j];
  r.carry.check();r.rows+=n;r.bits+=U(n)*entries;++r.calls;
  if(pos+d==tap_width)r.tap=r.carry;
  if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
  if(!pads.empty())OPENSSL_cleanse(pads.data(),pads.size());
 }
 if(tap_width)require(r.tap.v.size()==n,"complete direct carry tap");
 return r;
}
}
