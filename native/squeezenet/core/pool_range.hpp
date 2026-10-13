#include "squeezenet/core/squeezenet_duplex_pool.hpp"
#pragma once
#include "squeezenet/core/pool_wave.hpp"

namespace aqcf_pool_range {
using namespace aqcf2;
// Public producer certificate: BOTH values lie in [0,2^value_bits-1].
// Hence b-a is a signed (value_bits+1)-bit integer, with no signed overflow.
// Compare its low bits, but select the ORIGINAL Ring37 shares. This avoids
// any truncation/zero-extension of the selected value or its private shares.
// Validity must come from fixed graph semantics, never observed secret data.
template<class B> Ring maximum(B& s,const Ring& a,const Ring& b,unsigned value_bits){
 a.check();b.check();require(a.width==37&&b.width==37&&a.v.size()==b.v.size(),"range max shares");
 require(value_bits==24||value_bits==36,"certified ReLU source range only");
 require(!a.v.empty()&&a.v.size()<=aqcf_pool_wave::limit,"range max wave bound");
 Ring diff(37,a.v.size());
 for(std::size_t j=0;j<diff.v.size();++j)diff.v[j]=(b.v[j]-a.v[j])&mask(37);
 auto carry=aqcf_pool_wave::carry(s,diff.v,value_bits);
 auto sign=s.xor_(carry,s.local_bit(diff.v,value_bits));
 auto selected=aqcf_pool426::select(s,s.not_(sign),diff);
 for(std::size_t j=0;j<selected.v.size();++j)selected.v[j]=(selected.v[j]+a.v[j])&mask(37);
 selected.check();return selected;
}
}
