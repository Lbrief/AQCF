#pragma once
#include "resnet50/core/quarter_recover.hpp"
namespace aqcf_quarter {
// Native Ring37 phase recovery; no precision change to graph.
// Only the CIFAR backend that immediately reduces every affine to Ring37 may
// call this function. BFV degree, all-Q masking and the secret winding
// protocol remain unchanged. Final score lifting remains the old Ring55 path.
inline aqcf2::Ring recover_consumer37(aqcf2::Sci& sci,
                                     const std::vector<aqcf2::U>& owned){
 using namespace aqcf2;
 require(!owned.empty()&&owned.size()<=4096,"original quarter correction chunk");
 const auto n=owned.size();std::vector<U> choices(n),table,pad;Ring out(37,n);
 if(sci.party==1){pad=sci.random_ring_words(n,37);table.resize(4*n);}
 for(U j=0;j<n;++j){
  require(owned[j]<(U(1)<<39),"new Ring37 full quarter-word domain");choices[j]=owned[j]&3;
  if(sci.party==1){
   for(U c=0;c<4;++c)table[4*j+c]=U(winding(c,owned[j]&3)-I(pad[j]))&mask(37);
   out.v[j]=(pad[j]-(owned[j]>>2))&mask(37);
  }
 }
 auto answer=sci.lookup_words(table,choices,n,2,37);
 if(sci.party==2)for(U j=0;j<n;++j)out.v[j]=((owned[j]>>2)+answer[j])&mask(37);
 out.check();return out;
}
}
