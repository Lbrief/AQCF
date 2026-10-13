#pragma once
#include "squeezenet/engine/types.hpp"

// Standard exact max separability, retaining original NCHW and zero padding.
// Only pooled values are returned: no hidden argmax/tie index is observable.
namespace aqcf_pool_geometry {
using aqcf2::U;using V=std::vector<U>;
struct Shape {int ih,iw,ch,oh,ow,kh,kw,pt,pl,sh,sw;};
inline void check(const V& x,const Shape& q){
 aqcf2::require(q.ih>0&&q.iw>0&&q.ch>0&&q.oh>0&&q.ow>0&&q.kh>0&&q.kw>0&&q.sh>0&&q.sw>0&&q.pt>=0&&q.pl>=0,"pool public geometry");
 aqcf2::require(x.size()==std::size_t(q.ch)*q.ih*q.iw,"pool full input");
}
template<class Max> V flat(const V& x,const Shape& q,Max max){
 check(x,q);const std::size_t n=std::size_t(q.ch)*q.oh*q.ow;V y;bool first=true;
 for(int ky=0;ky<q.kh;++ky)for(int kx=0;kx<q.kw;++kx){V v(n);
  for(int c=0;c<q.ch;++c)for(int h=0;h<q.oh;++h)for(int w=0;w<q.ow;++w){
   int sy=h*q.sh-q.pt+ky,sx=w*q.sw-q.pl+kx;
   if(sy>=0&&sy<q.ih&&sx>=0&&sx<q.iw)v[(c*q.oh+h)*q.ow+w]=x[(c*q.ih+sy)*q.iw+sx];}
  y=first?std::move(v):max(y,v);first=false;
 }
 return y;
}
template<class Max> V separable(const V& x,const Shape& q,Max max){
 check(x,q);V horizontal;bool first=true;
 for(int kx=0;kx<q.kw;++kx){V v(std::size_t(q.ch)*q.ih*q.ow);
  for(int c=0;c<q.ch;++c)for(int h=0;h<q.ih;++h)for(int w=0;w<q.ow;++w){int sx=w*q.sw-q.pl+kx;
   if(sx>=0&&sx<q.iw)v[(c*q.ih+h)*q.ow+w]=x[(c*q.ih+h)*q.iw+sx];}
  horizontal=first?std::move(v):max(horizontal,v);first=false;
 }
 V y;first=true;
 for(int ky=0;ky<q.kh;++ky){V v(std::size_t(q.ch)*q.oh*q.ow);
  for(int c=0;c<q.ch;++c)for(int h=0;h<q.oh;++h)for(int w=0;w<q.ow;++w){int sy=h*q.sh-q.pt+ky;
   if(sy>=0&&sy<q.ih)v[(c*q.oh+h)*q.ow+w]=horizontal[(c*q.ih+sy)*q.ow+w];}
  y=first?std::move(v):max(y,v);first=false;
 }
 return y;
}
template<class Max,class Work> V separable(const V& x,const Shape& q,Max max,Work work){
 check(x,q);V horizontal;bool first=true;
 for(int kx=0;kx<q.kw;++kx){V v(std::size_t(q.ch)*q.ih*q.ow);
  work("POOL_HORIZONTAL_GATHER",v.size(),[&](std::size_t lo,std::size_t hi){
   for(auto j=lo;j<hi;++j){const int w=j%q.ow,h=(j/q.ow)%q.ih,c=j/(q.ow*q.ih),sx=w*q.sw-q.pl+kx;
    v[j]=sx>=0&&sx<q.iw?x[(c*q.ih+h)*q.iw+sx]:0;}});
  horizontal=first?std::move(v):max(horizontal,v);first=false;
 }
 V y;first=true;
 for(int ky=0;ky<q.kh;++ky){V v(std::size_t(q.ch)*q.oh*q.ow);
  work("POOL_VERTICAL_GATHER",v.size(),[&](std::size_t lo,std::size_t hi){
   for(auto j=lo;j<hi;++j){const int w=j%q.ow,h=(j/q.ow)%q.oh,c=j/(q.ow*q.oh),sy=h*q.sh-q.pt+ky;
    v[j]=sy>=0&&sy<q.ih?horizontal[(c*q.ih+sy)*q.ow+w]:0;}});
  y=first?std::move(v):max(y,v);first=false;
 }
 return y;
}
}
