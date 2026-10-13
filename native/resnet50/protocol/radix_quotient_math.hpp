#pragma once
#include <cstdint>
#include <stdexcept>
namespace quotient197 {
inline void require(bool b){if(!b)throw std::invalid_argument("quotient197 noncanonical input");}
inline unsigned base(unsigned k){require(k==2||k==3);return 1u<<k;}
struct Correction{uint8_t wire,quotient;};
inline uint8_t early(unsigned k,unsigned alpha,unsigned k0,unsigned k1){
 const unsigned B=base(k);require(alpha<2&&k0<B&&k1<B);return (alpha+k0+k1)&(B-1);
}
inline Correction correct(unsigned k,unsigned y,unsigned beta,unsigned r,unsigned kr,unsigned D,unsigned R){
 const unsigned B=base(k);require(y<B&&beta<2&&r<2&&kr<B&&D<B&&R<B);
 const unsigned Q=beta?((D+B-kr)&(B-1)):kr;
 const unsigned V=(beta+2*B-2*Q)&(2*B-1),W=y+V;
 return {uint8_t((beta^r)|(((W&(B-1))^R)<<1)),uint8_t((W>>k)&1)};
}
inline uint8_t table(unsigned k,unsigned x,unsigned alpha,unsigned mu,unsigned k0,unsigned k1,unsigned correction,const uint8_t* pads){
 const unsigned B=base(k);require(x<B&&alpha<2&&mu<2&&k0<B&&k1<B&&correction<2*B&&pads);
 const unsigned K=(correction&1)?k1:k0,U=x+((alpha+2*K)&(2*B-1)),d=correction>>1;
 unsigned answer=0;
 for(unsigned j=0;j<B;++j){require(pads[j]<2);const unsigned bit=(((U&(B-1))+(j^d))>=B)^((U>>k)&1)^mu^pads[j];answer|=bit<<j;}
 return uint8_t(answer);
}
inline uint8_t finish(unsigned k,unsigned encrypted,unsigned R,unsigned pad,unsigned quotient){
 const unsigned B=base(k);require(encrypted<(1u<<B)&&R<B&&pad<2&&quotient<2);
 return ((encrypted>>R)&1)^pad^quotient;
}
}
