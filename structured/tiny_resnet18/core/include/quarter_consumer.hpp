#pragma once
#include "quarter_recover.hpp"
#ifndef AQCF_ABLATE_QUARTER
#define AQCF_ABLATE_QUARTER 0
#endif
static_assert(AQCF_ABLATE_QUARTER==0 || AQCF_ABLATE_QUARTER==1,"binary quarter flag");
namespace aqcf_quarter {
// Native Ring41 phase recovery; no precision change to graph.
// Only the Tiny ImageNet backend that immediately reduces every affine to Ring41 may
// call this function. BFV degree, all-Q masking and the secret winding
// protocol remain unchanged. Final score lifting remains the old Ring55 path.
inline aqcf2::Ring recover_consumer41(aqcf2::Sci& sci,
                                     const std::vector<aqcf2::U>& owned){
 using namespace aqcf2;
#if AQCF_ABLATE_QUARTER
 // Ordinary two private-bit products; no winding or low two bits are opened.
 // winding(c,s)=[c==3][s==0]-[c==0][s==3]. Each COT output is Ring41.
 require(!owned.empty()&&owned.size()<=4096,"ordinary quarter chunk");
 const auto n=owned.size();std::vector<std::uint8_t> pos(n),neg(n);
 for(size_t j=0;j<n;++j){
  require(owned[j]<(U(1)<<43),"ordinary quarter word canonical");
  const U low=owned[j]&3;
  pos[j]=sci.party==1?low==0:low==3;
  neg[j]=sci.party==1?low==3:low==0;
 }
 auto positive=sci.bit_product(pos,41),negative=sci.bit_product(neg,41);
 Ring ordinary(41,n);
 for(size_t j=0;j<n;++j){const U high=owned[j]>>2;
  ordinary.v[j]=((sci.party==1?U(0)-high:high)+positive.v[j]-negative.v[j])&mask(41);
 }
 ordinary.check();return ordinary;
#else
 require(!owned.empty()&&owned.size()<=4096,"original quarter correction chunk");
 const auto n=owned.size();std::vector<U> choices(n),table,pad;Ring out(41,n);
 if(sci.party==1){pad=sci.random_ring_words(n,41);table.resize(4*n);}
 for(U j=0;j<n;++j){
  require(owned[j]<(U(1)<<43),"new Ring41 full quarter-word domain");choices[j]=owned[j]&3;
  if(sci.party==1){
   for(U c=0;c<4;++c)table[4*j+c]=U(winding(c,owned[j]&3)-I(pad[j]))&mask(41);
   out.v[j]=(pad[j]-(owned[j]>>2))&mask(41);
  }
 }
 auto answer=sci.lookup_words(table,choices,n,2,41);
 if(sci.party==2)for(U j=0;j<n;++j)out.v[j]=((owned[j]>>2)+answer[j])&mask(41);
 out.check();return out;
#endif
}
}
