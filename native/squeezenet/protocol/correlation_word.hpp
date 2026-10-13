#pragma once
#include <cstdint>
#include <cstddef>
#include <immintrin.h>
#include <stdexcept>
namespace aqcf_word208 {
inline bool eligible(unsigned bits,unsigned width){return bits>=1&&bits<=8&&width>=1&&width<=60&&(1u<<(bits-1))*width<=128;}
template<class Block>inline uint64_t edge(Block key,unsigned prefix,unsigned width){
 const unsigned shift=prefix*width;if(shift+width>128)throw std::invalid_argument("208 key coordinate range");
 const __uint128_t word=__uint128_t(uint64_t(_mm_extract_epi64(key,0)))|(__uint128_t(uint64_t(_mm_extract_epi64(key,1)))<<64);
 return uint64_t(word>>shift)&((uint64_t(1)<<width)-1);
}
template<class Block>inline uint64_t sender_pad(const Block* a,const Block* b,unsigned bits,unsigned width,unsigned v){
 uint64_t pad=0;for(unsigned l=0;l<bits;++l)pad^=edge(((v>>l)&1)?b[l]:a[l],v&((1u<<l)-1),width);return pad;
}
template<class Block>inline uint64_t receiver_pad(const Block* keys,unsigned bits,unsigned width,unsigned r){
 uint64_t pad=0;for(unsigned l=0;l<bits;++l)pad^=edge(keys[l],r&((1u<<l)-1),width);return pad;
}
}
