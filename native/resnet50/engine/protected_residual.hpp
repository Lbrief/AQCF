#pragma once
#include "resnet50/engine/quotient_recovery_core.hpp"
#include "resnet50/engine/floor_terminal.hpp"
#include "resnet50/engine/radix_quotient_core.hpp"
#include "resnet50/engine/ring37_direct.hpp"
#include "resnet50/core/quarter_recover.hpp"
#include "resnet50/engine/ring37_direct_floor.hpp"
#include <openssl/crypto.h>

// Source candidate only: consume a bound raw quarter phase directly at the
// sole original floor12/ReLU consumer. No phase may escape to another operator.
namespace aqcf_r50_residual_phase216 {
using namespace aqcf2;
inline unsigned correction(unsigned c,unsigned z){
 require(c<4&&z<4,"quarter low digits");
 const int h=int(aqcf_quarter::winding(c,z))-int(c==3)+int(z!=0);
 require(h==0||h==1,"normalized correction is Boolean");return unsigned(h);
}
template<class B> Ring consume(B& s,const std::vector<U>& phase,const std::vector<U>& add){
 const auto n=phase.size();require(add.size()==n,"bound residual shape");
 require(n>0&&n<=65536&&s.compact_cofactor,"bound original R50 frontier");
 std::vector<U> own(n),shifted(n),choice(n),table(s.party==1?64*n:0);
 std::vector<std::uint8_t> pad;Bits seed(n);
 if(s.party==1)pad=s.random_small(n);
 for(std::size_t j=0;j<n;++j){
  require(phase[j]<(U(1)<<39),"original R50 quarter Ring39, not SQ Ring57");
  const U lo=phase[j]&3,hi=phase[j]>>2;
  own[j]=(s.party==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);
  require(add[j]<=mask(37),"original residual Ring37 share");own[j]=(own[j]+add[j])&mask(37);
  const U digit=own[j]&15;choice[j]=(digit<<2)|lo;shifted[j]=own[j]>>4;
  if(s.party==1)for(unsigned c=0;c<64;++c){
   const U carry=(digit+(c>>2)+correction(c&3,unsigned(lo)))>>4;
   require(carry<2,"first digit carry");
   table[64*j+c]=carry^(pad[j]&1);
  }
 }
 const auto got=s.lookup_words(table,choice,n,6,1,false);s.flush();
 for(std::size_t j=0;j<n;++j)seed.v[j]=s.party==1?(pad[j]&1):got[j];
 seed.check();
 // Seed stays secret. Bits4..35 are all retained. Tap8 is original bit12.
 std::vector<CarryTask> tasks{{&shifted,32,seed,false}};Bits low;
 auto rest=radix197_enabled()?quotient197_execute(s,tasks,8,low):quotient169_execute(s,tasks,8,low);
 s.note_linked_execution(n,n,rest.packet_rows+n,rest.packet_table_bits+64*n,rest.selector_rows);
 Ring coarse(25,n);
 for(std::size_t j=0;j<n;++j)coarse.v[j]=own[j]>>12;
 auto out=floor216::emit(s,coarse,low,rest.value[0]);
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 return out;
}
}
