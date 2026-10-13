#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace aqcf2 {
using U=std::uint64_t; using I=std::int64_t; using W=__int128_t;
constexpr U P=1152921504606830593ULL, M=U{1}<<55, C=U{1}<<53;
inline void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
inline U mask(unsigned w){require(w>0&&w<=60,"bitwidth outside 1..60");return (U{1}<<w)-1;}
inline U residue(W x,U m){W r=x%W(m);return U(r<0?r+m:r);}
inline I centered(U x,U m){require(x<m,"noncanonical residue");return W(x)*2>=m?I(W(x)-m):I(x);}
struct Bits {std::vector<std::uint8_t> v;explicit Bits(std::size_t n=0):v(n){}void check()const{for(auto b:v)require(b<2,"invalid XOR bit");}};
struct Ring {unsigned width;std::vector<U> v;explicit Ring(unsigned w,std::size_t n=0):width(w),v(n){(void)mask(w);}void check()const{for(auto x:v)require(x<=mask(width),"noncanonical ring share");}};
struct Field {std::vector<U> v;explicit Field(std::size_t n=0):v(n){}void check()const{for(auto x:v)require(x<P,"noncanonical field share");}};
// Trusted, integer-only reference. This header has no protocol/network calls.
inline I nearest_even(I z,unsigned f){require(f>=1&&f<=53,"rounding shift");W a=z;bool neg=a<0;if(neg)a=-a;W d=W(1)<<f,q=a/d,r=a%d;q+=(2*r>d||(2*r==d&&(q&1)));return I(neg?-q:q);}
struct Spec {I coarse,quotient,relu;unsigned carry,equal,parity,correction;};
inline Spec deferred_spec(U x0,U x1,unsigned f,unsigned k=55,unsigned b=53){
 require(1<=f&&f<b&&b<=k-2&&k<=55,"invalid spec domain");U m=U(1)<<k,c=U(1)<<b,d=U(1)<<f,h=U(1)<<(k-f);
 require(x0<m&&x1<m,"spec canonical inputs");I z=centered((x0+x1)%m,m);require(W(z)>-W(c)&&W(z)<c,"spec bounded domain");
 U u0=(x0+c+d/2-1)%m,u1=x1,a0=((u0>>f)-(c>>f))&(h-1),a1=u1>>f;
 U l0=u0&(d-1),l1=u1&(d-1);unsigned cy=l0+l1>=d,eq=l0+l1==d-1,pa=(a0^a1)&1,delta=cy^(eq&pa);
 I t=centered((a0+a1)&(h-1),h),q=t+delta,y=std::max<I>(t,0)+(t>=0?delta:0);
 return {t,q,y,cy,eq,pa,delta};
}
}
