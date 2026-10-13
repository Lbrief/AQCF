#pragma once
#include <cstdint>
#include <stdexcept>

// ONLY after the unchanged native random-message ROT hash, NEVER raw COT.
// A native row is one-use; bit coordinates are distinct within each key.
namespace aqcf_slice207 {
inline unsigned lowbit(uint64_t key,unsigned prefix){
 if(prefix>=4)throw std::invalid_argument("207 supported coordinate");
 return unsigned((key>>prefix)&1);
}
template<class Block>
inline void sender(unsigned bits,const Block* k0,const Block* k1,size_t rows,uint8_t* pads){
 if(bits!=2&&bits!=3)throw std::invalid_argument("207 bits");
 const unsigned B=1u<<bits,S=bits+1;
 for(size_t j=0;j<rows;++j){uint64_t a[3],b[3];
  for(unsigned l=0;l<bits;++l){a[l]=uint64_t(_mm_extract_epi64(k0[j*S+l],0));b[l]=uint64_t(_mm_extract_epi64(k1[j*S+l],0));}
  for(unsigned v=0;v<B;++v){unsigned p=0;for(unsigned l=0;l<bits;++l)p^=lowbit(((v>>l)&1)?b[l]:a[l],v&((1u<<l)-1));pads[j*B+v]=p;}
 }
}
template<class Block>
inline void receiver(unsigned bits,const Block* keys,const uint8_t* choices,size_t rows,uint8_t* pads){
 if(bits!=2&&bits!=3)throw std::invalid_argument("207 bits");
 const unsigned B=1u<<bits,S=bits+1;
 for(size_t j=0;j<rows;++j){if(choices[j]>=B)throw std::invalid_argument("207 choice");unsigned p=0;
  for(unsigned l=0;l<bits;++l)p^=lowbit(uint64_t(_mm_extract_epi64(keys[j*S+l],0)),choices[j]&((1u<<l)-1));pads[j]=p;
 }
}
}
