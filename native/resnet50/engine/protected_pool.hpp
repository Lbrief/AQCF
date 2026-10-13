#pragma once
#include "resnet50/engine/pool_frontier.hpp"
// Same signed37 -> Ring38 embedding and maximum primitive. Only the public
// evaluation DAG changes: horizontal maxima are shared by overlapping windows.
namespace aqcf_pool255 {
template<class B> std::vector<aqcf2::U> reduce(B& b,const std::vector<aqcf2::U>& x,int ch,int ih,int iw){
 using namespace aqcf2;using V=std::vector<U>;
 require(ch>0&&ih>0&&iw>0&&x.size()==std::size_t(ch)*ih*iw,"255 public input geometry");
 const int oh=(ih+1)/2,ow=(iw+1)/2;
 const std::size_t m=std::size_t(ch)*ih*ow,n=std::size_t(ch)*oh*ow;
 require(m>=4&&m<=401408&&n>=4&&n<=200704,"255 bounded public pooling geometry");
 V horizontal;
 for(int k=0;k<3;++k){
  V v(m);b.tensor_work("POOL255_HORIZONTAL_GATHER",m,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){
   int xx=int(j%ow),yy=int((j/ow)%ih),c=int(j/(std::size_t(ih)*ow));int sx=2*xx+k;
   v[j]=sx<iw?x[(std::size_t(c)*ih+yy)*iw+sx]:0;
  }});
  if(k==0)horizontal=std::move(v);else horizontal=b.pool_maximum(horizontal,v);
 }
 V out;
 for(int k=0;k<3;++k){
  V v(n);b.tensor_work("POOL255_VERTICAL_GATHER",n,[&](std::size_t lo,std::size_t hi){for(auto j=lo;j<hi;++j){
   int xx=int(j%ow),yy=int((j/ow)%oh),c=int(j/(std::size_t(oh)*ow));int sy=2*yy+k;
   v[j]=sy<ih?horizontal[(std::size_t(c)*ih+sy)*ow+xx]:0;
  }});
  if(k==0)out=std::move(v);else out=b.pool_maximum(out,v);
 }
 return out;
}
}
