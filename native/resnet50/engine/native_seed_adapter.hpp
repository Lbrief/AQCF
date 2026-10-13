#pragma once
#include "resnet50/engine/native_quotient_adapter.hpp"
#include "resnet50/engine/radix_quotient_core.hpp"
#include "resnet50/engine/seeded_mask.hpp"
#include "resnet50/engine/mixed_quotient_policy.hpp"
#include "resnet50/engine/elided_component_policy.hpp"
namespace compose221 {using namespace compose189;
template<class B>aqcf2::Ring nonlinear(B& s,const std::vector<U>& phase,Bank& bank){
 using namespace aqcf2;const auto n=phase.size();std::vector<U> own(n),lo(n),coarse(n);Bits low;std::vector<uint8_t> pl(n),ph(n);
 for(size_t j=0;j<n;++j){b::require(phase[j]<(U(1)<<39),"221 exact original191 Ring39 phase");lo[j]=phase[j]&3;U hi=phase[j]>>2;
  own[j]=(s.party==1?U(0)-hi-U(lo[j]!=0):hi+U(lo[j]==3))&mask(37);coarse[j]=own[j]>>12;
  if(s.party==1){pl[j]=bank.rows[j].terminal.low;ph[j]=bank.rows[j].terminal.high;}}
 auto rest=aqcf_mixed238::enabled()?aqcf_mixed237::prefix(s,own,lo,low,&pl,&ph):aqcf_seed221::prefix(s,own,lo,low,&pl,&ph);
 auto small=bank.consume(s,coarse,low,rest.value[0]);std::vector<U> lift_choice(n),lift_table,high_pad;
 const bool elide250=aqcf_elide250::enabled();
 if(s.party==1){high_pad=elide250?std::vector<U>(n):s.random_ring_words(n,12);lift_table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){U top=small[j]>>24;lift_choice[j]=top;if(s.party==1)for(U c=0;c<2;++c)lift_table[2*j+c]=(U(0)-(top|c)-high_pad[j])&mask(12);}
 auto high=elide250?aqcf_terminal249::lookup(s,lift_table,lift_choice,1,12,0):s.lookup_words(lift_table,lift_choice,n,1,12);Ring result(37,n);
 for(std::size_t j=0;j<n;++j)result.v[j]=(small[j]+((elide250?high[j]:(s.party==1?high_pad[j]:high[j]))<<25))&mask(37);
 result.check();return result;
}
}
