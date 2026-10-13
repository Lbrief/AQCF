#pragma once
#include "resnet50/engine/shared_prefix.hpp"
#include "resnet50/engine/quotient_pool38.hpp"
// Existing exact masked carry + full38-bit compare, using fixed public frontier.
// No secret predicates are opened; no precision/domain narrowing or new OT.
namespace aqcf_pool_frontier {
using namespace aqcf2;
template<class B> Ring maximum(B& s,const Ring& a,const Ring& b){
 a.check();b.check();const auto n=a.v.size();
 require(a.width==38&&b.width==38&&n==b.v.size()&&n>0&&n<=100352,"bounded full38 pool frontier");
 Ring diff(38,n);for(std::size_t j=0;j<n;++j)diff.v[j]=(b.v[j]-a.v[j])&mask(38);
 std::vector<CarryTask> tasks{{&diff.v,37,s.constant(false,n),true}};
 Bits unused_tap;
 auto carry=linked_cofactor_prefix_execute(s,tasks,4,unused_tap).value[0];
 auto sign=s.xor_(carry,s.local_bit(diff.v,37));auto pos=s.mux(s.not_(sign),diff);
 for(std::size_t j=0;j<n;++j)pos.v[j]=(pos.v[j]+a.v[j])&mask(38);
 pos.check();return pos;
}
// Same eight comparisons for nine values, depth4 rather than depth8.
// All locations at a public tree level use separate fresh protocol rows.
template<class B> std::vector<U> reduce(B& b,std::vector<std::vector<U>> values){
 require(!values.empty()&&values.size()<=49,"bounded public pooling arity");
 const auto n=values[0].size();require(n>=4,"four useful pool rows");
 for(auto& v:values)require(v.size()==n,"same candidate shapes");
 while(values.size()>1){
  const auto pairs=values.size()/2;std::vector<U> left(pairs*n),right(pairs*n);
  b.tensor_work("POOL_TREE_GATHER",pairs*n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){left[j]=values[2*(j/n)][j%n];right[j]=values[2*(j/n)+1][j%n];}});
  auto got=b.pool_maximum(left,right);require(got.size()==pairs*n,"complete public level");
  std::vector<std::vector<U>> next(pairs+(values.size()%2));
  for(std::size_t k=0;k<pairs;++k)next[k].resize(n);
  b.tensor_work("POOL_TREE_SCATTER",pairs*n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j)next[j/n][j%n]=got[j];});
  if(values.size()%2)next.back()=std::move(values.back());
  values=std::move(next);
 }
 return std::move(values[0]);
}
}
