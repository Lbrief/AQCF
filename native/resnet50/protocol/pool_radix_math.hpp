#pragma once
#include <cstdint>
#include <cstddef>
#include <smmintrin.h>
#include <stdexcept>
namespace aqcf_radix16_263 {
inline void require(bool b){if(!b)throw std::invalid_argument("263 noncanonical radix16 input");}
struct Correction{uint8_t wire,quotient;};
inline unsigned alpha(unsigned k0,unsigned k1){require(k0<16&&k1<16);return (k0+k1)&1;}
inline unsigned early(unsigned a,unsigned k0,unsigned k1){require(a<2&&k0<16&&k1<16);return (a+k0+k1)&15;}
inline Correction correct(unsigned y,unsigned beta,unsigned r,unsigned kr,unsigned D,unsigned R){
 require(y<16&&beta<2&&r<2&&kr<16&&D<16&&R<16);
 const unsigned Q=beta?((D+16-kr)&15):kr;
 const unsigned W=y+((beta+32-2*Q)&31);
 return {uint8_t((beta^r)|(((W&15)^R)<<1)),uint8_t((W>>4)&1)};
}
inline uint16_t table(unsigned x,unsigned a,unsigned mu,unsigned k0,unsigned k1,unsigned correction,const uint8_t* pads){
 require(x<16&&a<2&&mu<2&&k0<16&&k1<16&&correction<32&&pads);
 const unsigned K=(correction&1)?k1:k0,U=x+((a+2*K)&31),d=correction>>1;
 unsigned answer=0;
 for(unsigned j=0;j<16;++j){require(pads[j]<2);answer|=((((U&15)+(j^d))>=16)^((U>>4)&1)^mu^pads[j])<<j;}
 return uint16_t(answer);
}
inline unsigned finish(unsigned encrypted,unsigned R,unsigned pad,unsigned quotient){
 require(encrypted<65536&&R<16&&pad<2&&quotient<2);
 return ((encrypted>>R)&1)^pad^quotient;
}
// Distinct coordinates of fully hashed native RM keys. The full selected key
// remains visible to the receiver; no raw COT or key-erasure argument is used.
template<class Block> inline void sender_pads(const Block* k0,const Block* k1,size_t rows,uint8_t* pads){
 for(size_t j=0;j<rows;++j){uint64_t a[4],b[4];
  for(unsigned l=0;l<4;++l){a[l]=uint64_t(_mm_extract_epi64(k0[j*5+l],0));b[l]=uint64_t(_mm_extract_epi64(k1[j*5+l],0));}
  for(unsigned v=0;v<16;++v){unsigned p=0;for(unsigned l=0;l<4;++l)p^=unsigned(((((v>>l)&1)?b[l]:a[l])>>(v&((1u<<l)-1)))&1);pads[j*16+v]=p;}
 }
}
template<class Block> inline void receiver_pads(const Block* keys,const uint8_t* choices,size_t rows,uint8_t* pads){
 for(size_t j=0;j<rows;++j){require(choices[j]<16);unsigned p=0;
  for(unsigned l=0;l<4;++l)p^=unsigned((uint64_t(_mm_extract_epi64(keys[j*5+l],0))>>(choices[j]&((1u<<l)-1)))&1);
  pads[j]=p;
 }
}
}
