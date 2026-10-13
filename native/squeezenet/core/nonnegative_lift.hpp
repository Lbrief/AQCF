#pragma once
#include "squeezenet/engine/sci.hpp"
#include "squeezenet/core/wave_policy.hpp"

namespace aqcf_nonnegative_lift {
using namespace aqcf2;
// Public producer certificate: x=(a+b) mod2^37 is in [0,2^36-1].
// For canonical a,b in [0,2^37), wrap(a+b)=a36 OR b36 under this certificate.
// Indeed, neither MSB set implies a+b<2^37. At least one MSB set implies
// a+b>=2^36>x, so a+b=x+2^37. No flag is opened or used for public scheduling.
// A single existing binary lookup gives additive Ring18 shares of the OR;
// subtracting those shares times2^37 yields uniform, exact Ring55 shares of x.
// Unknown/signed producers MUST retain the full signed lift.
template<class B> Ring lift37_to55(B& s,const Ring& input){
 input.check();require(input.width==37&&!input.v.empty()&&input.v.size()<=aqcf_wan_wave::pool_limit,"nonnegative lift public shape");
 const auto n=input.v.size();std::vector<U> table,choice(n),pad;
 if(s.party==1){pad=s.random_ring_words(n,18);table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){
  const U high=(input.v[j]>>36)&1;choice[j]=high;
  if(s.party==1)for(U other=0;other<2;++other)table[2*j+other]=((high|other)-pad[j])&mask(18);
 }
 auto selected=s.lookup_words(table,choice,n,1,18,false);Ring output(55,n);
 for(std::size_t j=0;j<n;++j){const U c=s.party==1?pad[j]:selected[j];output.v[j]=(input.v[j]-(c<<37))&mask(55);}
 output.check();return output;
}
}
