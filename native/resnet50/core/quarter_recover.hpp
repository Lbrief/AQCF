#pragma once
#include "resnet50/core/sci.hpp"
namespace aqcf_quarter {
inline aqcf2::I winding(aqcf2::U c,aqcf2::U s){return c==3&&s==0?1:c==0&&s==3?-1:0;}
// Client owns quarter-decoded masked phase b; server owns quarter-decoded
// uniform ring mask B. Neither the winding nor the other party's two low bits
// is opened. Output shares sum to the exact original Ring55 coefficient under
// the proven quarter-grid error-margin premise.
inline aqcf2::Ring recover(aqcf2::Sci& sci,const std::vector<aqcf2::U>& owned){
 using namespace aqcf2;require(!owned.empty()&&owned.size()<=4096,"fixed public correction chunk");
 std::vector<U> choices(owned.size()),table,maskwords;Ring out(55,owned.size());
 if(sci.party==1){maskwords=sci.random_ring_words(owned.size(),55);table.resize(4*owned.size());}
 for(U j=0;j<owned.size();++j){require(owned[j]<(U(1)<<57),"quarter word canonical");choices[j]=owned[j]&3;
  if(sci.party==1){for(U c=0;c<4;++c)table[4*j+c]=U(winding(c,owned[j]&3)-I(maskwords[j]))&mask(55);out.v[j]=(maskwords[j]-(owned[j]>>2))&mask(55);}}
 auto answer=sci.lookup_words(table,choices,owned.size(),2,55);
 if(sci.party==2)for(U j=0;j<owned.size();++j)out.v[j]=((owned[j]>>2)+answer[j])&mask(55);
 out.check();return out;
}
} // namespace aqcf_quarter
