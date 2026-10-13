#pragma once
// Exact two-band accumulator for canonical 38-bit RNS operands. No floating
// point, reduced precision, secret-dependent trimming, or cryptographic change.
// Digit splitting/Karatsuba/SIMD are established. No priority claim is made.
#include <immintrin.h>
#include <cstdint>
#include <vector>
#include <stdexcept>
namespace aqcf_band {
using U=std::uint64_t;using W=__uint128_t;
struct Accumulator {
 std::vector<U>lo,hi;
 explicit Accumulator(std::size_t n):lo(n,0),hi(n,0){}
 // Up to65536 products per lane; both bands stay below2^55. Callers enforce
 // the PUBLIC fan-in limit. This is an integer identity before reduction.
 void add(std::size_t at,const U*x,const U*w,std::size_t n,bool broadcast=false){
  if(at+n>lo.size()||n%4)throw std::runtime_error("public band shape");
  const auto mask=_mm256_set1_epi64x((U(1)<<19)-1);
  for(std::size_t j=0;j<n;j+=4){
   auto a=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(x+j));
   auto b=broadcast?_mm256_set1_epi64x(*w):_mm256_loadu_si256(reinterpret_cast<const __m256i*>(w+j));
   auto al=_mm256_and_si256(a,mask),ah=_mm256_srli_epi64(a,19),bl=_mm256_and_si256(b,mask),bh=_mm256_srli_epi64(b,19);
   auto p0=_mm256_mul_epu32(al,bl),p2=_mm256_mul_epu32(ah,bh);
   auto cross=_mm256_sub_epi64(_mm256_sub_epi64(_mm256_mul_epu32(_mm256_add_epi64(al,ah),_mm256_add_epi64(bl,bh)),p0),p2);
   auto l=_mm256_add_epi64(p0,_mm256_slli_epi64(_mm256_and_si256(cross,mask),19));
   auto h=_mm256_add_epi64(p2,_mm256_srli_epi64(cross,19));
   _mm256_storeu_si256(reinterpret_cast<__m256i*>(lo.data()+at+j),_mm256_add_epi64(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(lo.data()+at+j)),l));
   _mm256_storeu_si256(reinterpret_cast<__m256i*>(hi.data()+at+j),_mm256_add_epi64(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(hi.data()+at+j)),h));
  }
 }
 W word(std::size_t j)const{return W(lo[j])+(W(hi[j])<<38);}
 std::size_t size()const{return lo.size();}
 W operator[](std::size_t j)const{return word(j);}
};
}
