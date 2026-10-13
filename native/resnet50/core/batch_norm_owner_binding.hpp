#pragma once
// Server-only extraction of the ORIGINAL private BN coefficients. No file
// reader, transmission or logging of coefficients. Caller has already bound
// and parsed the original owner packet; no authority to replace the model.
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace aqcf_bn19 {
using U=std::uint64_t;using I=std::int64_t;
struct Geometry{unsigned id,L,channels,J,C;};
constexpr std::array<Geometry,18> geometry{{
 {1,3136,64,1,2},{6,3136,256,1,2},{10,3136,256,1,2},{14,3136,256,1,2},
 {19,784,512,3,8},{22,784,128,3,8},{24,784,512,3,8},{28,784,512,3,8},{32,784,512,3,8},
 {37,196,1024,6,32},{41,196,1024,6,32},{45,196,1024,6,32},{49,196,1024,6,32},{53,196,1024,6,32},{57,196,1024,6,32},
 {62,49,2048,12,128},{66,49,2048,12,128},{70,49,2048,12,128}
}};
inline void require(bool x,const char* m){if(!x)throw std::runtime_error(m);}
inline const Geometry* find(unsigned id){for(const auto& g:geometry)if(g.id==id)return &g;return nullptr;}
inline unsigned log_channels(const Geometry& g){unsigned k=0;while((1U<<k)<g.C)++k;require((1U<<k)==g.C&&g.C*g.L<=8192&&k<=7,"BN19 fixed power-two packing");return k;}
inline U local_phase(U owned,const Geometry& g){
 require(owned<(U(1)<<46),"BN19 T44 quarter phase");
 return (owned>>log_channels(g))&((U(1)<<39)-1);
}
// Bind every old gather term and query, including the original J tails.
// Reordering is not assumed; destination/source equations identify each item.
template<class Layout> void bind_layout(const Layout& l,const Geometry& g){
 const U size=U(g.L)*g.channels,groups=(g.channels+g.J-1)/g.J;
 require(l.id==g.id&&l.inputs==size&&l.outputs==size&&l.maps.size()==groups&&l.queries.size()==groups,"BN19 original layer identity");
 for(U o=0;o<groups;++o){const U start=o*g.J,n=std::min<U>(g.J,g.channels-start);
  require(l.maps[o].size()==n*g.L&&l.queries[o].size()==n*g.L,"BN19 complete tail maps");
  std::vector<unsigned char> mapped(n*g.L,0),queried(n*g.L,0);
  for(const auto& term:l.maps[o]){
   require(term.source>=start*g.L&&term.source<(start+n)*g.L&&term.multiplier==1,"BN19 original channel gather");
   U r=term.source/g.L-start,sp=term.source%g.L;
   require(term.exponent==(g.J-1-r)*g.L+sp&&!mapped[r*g.L+sp]++,"BN19 original gather exponent and uniqueness");
  }
  for(const auto& q:l.queries[o]){
   require(q.destination>=start*g.L&&q.destination<(start+n)*g.L,"BN19 original channel scatter");
   U r=q.destination/g.L-start,sp=q.destination%g.L;
   require(q.exponent==(r*g.J+g.J-1)*g.L+sp&&!queried[r*g.L+sp]++,"BN19 original query exponent and uniqueness");
  }
 }
}
struct PrivateOwner{std::vector<I> weight,bias;};
template<class Layer> PrivateOwner bind_owner(const Layer& layer,const Geometry& g){
 bind_layout(layer.layout,g);require(layer.output.size()==layer.layout.queries.size(),"BN19 complete private output");
 PrivateOwner owner;owner.weight.assign(g.channels,0);owner.bias.assign(g.channels,0);
 for(U o=0;o<layer.output.size();++o){const auto& out=layer.output[o];U start=o*g.J,n=std::min<U>(g.J,g.channels-start);
  require(out.edges.size()<=1&&out.queries.size()==n*g.L,"BN19 original diagonal edge domain");
  std::vector<unsigned char> seen(n,0),bias_seen(n,0);std::vector<unsigned char> queried(n*g.L,0);
  for(const auto& edge:out.edges){require(edge.input==o,"BN19 original diagonal edge identity");
   for(const auto& a:edge.coefficients){
    require(a.exponent%((g.J+1)*g.L)==0,"BN19 original signed coefficient exponent");U r=a.exponent/((g.J+1)*g.L);
    require(r<n&&!seen[r]++&&a.value!=0&&a.value>=-I(U(1)<<30)&&a.value<=I(U(1)<<30),"BN19 original coefficient cap");
    owner.weight[start+r]=a.value;
   }
  }
  for(const auto& q:out.queries){
   require(q.destination>=start*g.L&&q.destination<(start+n)*g.L,"BN19 private query target");
   U r=q.destination/g.L-start,sp=q.destination%g.L;
   require(q.exponent==(r*g.J+g.J-1)*g.L+sp&&!queried[r*g.L+sp]++,"BN19 private scatter binding");
   if(bias_seen[r])require(owner.bias[start+r]==q.bias,"BN19 original repeated bias identical");
   else{owner.bias[start+r]=q.bias;bias_seen[r]=1;}
  }
 }
 return owner; // Original stored scaled bias, no rescale/retrain/coefficient fold.
}
}
