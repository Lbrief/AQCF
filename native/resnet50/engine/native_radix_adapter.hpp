#pragma once
#include "resnet50/engine/native_quotient_adapter.hpp"
#include "resnet50/engine/radix_quotient_core.hpp"
namespace compose197 {using namespace compose189;
template<class B>aqcf2::Ring nonlinear(B& s,const std::vector<U>& phase,Bank& bank){
 using namespace aqcf2;const auto n=phase.size();std::vector<U> own(n),shifted(n),choice(n),table(s.party==1?64*n:0),coarse(n);
 Bits seed(n),low;std::vector<std::uint8_t> pad,pl(n),ph(n);if(s.party==1)pad=s.random_small(n);
 for(std::size_t j=0;j<n;++j){b::require(phase[j]<(U(1)<<39),"189 native Ring39 phase");U lo=phase[j]&3,hi=phase[j]>>2;
  own[j]=(s.party==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);U digit=own[j]&15;choice[j]=(digit<<2)|lo;shifted[j]=own[j]>>4;coarse[j]=own[j]>>12;
  if(s.party==1){pl[j]=bank.rows[j].terminal.low;ph[j]=bank.rows[j].terminal.high;
   for(U c=0;c<64;++c)table[64*j+c]=((digit+(c>>2)+aqcf_r50_phase_relu169::correction(c&3,unsigned(lo)))>>4)^(pad[j]&1);}}
 auto got=s.lookup_words(table,choice,n,6,1,false);s.flush();for(std::size_t j=0;j<n;++j)seed.v[j]=s.party==1?(pad[j]&1):got[j];seed.check();
 std::vector<CarryTask> tasks{{&shifted,32,seed,false}};auto rest=radix197_enabled()?quotient197_execute(s,tasks,8,low,&pl,&ph):prescribed189_execute(s,tasks,8,low,pl,ph);
 auto small=bank.consume(s,coarse,low,rest.value[0]);std::vector<U> lift_choice(n),lift_table,high_pad;
 if(s.party==1){high_pad=s.random_ring_words(n,12);lift_table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){U top=small[j]>>24;lift_choice[j]=top;if(s.party==1)for(U c=0;c<2;++c)lift_table[2*j+c]=(U(0)-(top|c)-high_pad[j])&mask(12);}
 auto high=s.lookup_words(lift_table,lift_choice,n,1,12);Ring result(37,n);
 for(std::size_t j=0;j<n;++j)result.v[j]=(small[j]+((s.party==1?high_pad[j]:high[j])<<25))&mask(37);
 result.check();return result;
}
}
