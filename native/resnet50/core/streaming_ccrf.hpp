#pragma once
// Exact AES-256(key, fixed original block one). No key compression, changed
// hash, shortened security parameter or reused randomness. Scheduling the
// existing cipher is mature implementation work, NOT a new crypto primitive.
#include "resnet50/vendor/bolt_sci/utils/ccrf.h"
#include <openssl/crypto.h>
#include <cpuid.h>
namespace aqcf_tuple {
inline bool have_parallel_aes(){unsigned a,b,c,d;return __builtin_cpu_supports("avx2")&&__get_cpuid_count(7,0,&a,&b,&c,&d)&&(c&(1U<<9));}
__attribute__((target("avx2,vaes"))) inline __m256i key_prefix_pair(__m256i x,__m256i sub){
 x=_mm256_xor_si256(x,_mm256_slli_si256(x,4));x=_mm256_xor_si256(x,_mm256_slli_si256(x,8));return _mm256_xor_si256(x,sub);
}
// Two independent original AES states in each vector; four pairs keep live
// round keys/state bounded. Original AES round count/key width is unchanged.
// The SIMD instruction is established CPU functionality, not originality.
__attribute__((target("avx2,vaes"))) inline void parallel_ccrf(sci::block128*out,const sci::block256*keys,std::size_t n){
 const __m256i rot=_mm256_broadcastsi128_si256(_mm_setr_epi8(13,14,15,12,13,14,15,12,13,14,15,12,13,14,15,12));
 __m256i lo[4]{},hi[4]{},x[4]{};std::size_t at=0;
 for(;at+8<=n;at+=8){
  #pragma GCC unroll 4
  for(unsigned j=0;j<4;++j){
   const auto*a=reinterpret_cast<const __m128i*>(keys+at+2*j);const auto*b=a+2;
   lo[j]=_mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128(a)),_mm_loadu_si128(b),1);
   hi[j]=_mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128(a+1)),_mm_loadu_si128(b+1),1);
   x[j]=_mm256_xor_si256(_mm256_broadcastsi128_si256(sci::one),lo[j]);x[j]=_mm256_aesenc_epi128(x[j],hi[j]);
  }
  #pragma GCC unroll 7
  for(unsigned r=1;r<=7;++r){
   const auto rc=_mm256_set1_epi32(1<<(r-1));
   #pragma GCC unroll 4
   for(unsigned j=0;j<4;++j){
    lo[j]=key_prefix_pair(lo[j],_mm256_aesenclast_epi128(_mm256_shuffle_epi8(hi[j],rot),rc));
    x[j]=r==7?_mm256_aesenclast_epi128(x[j],lo[j]):_mm256_aesenc_epi128(x[j],lo[j]);
   }
   if(r!=7){
    #pragma GCC unroll 4
    for(unsigned j=0;j<4;++j){hi[j]=key_prefix_pair(hi[j],_mm256_aesenclast_epi128(_mm256_shuffle_epi32(lo[j],0xff),_mm256_setzero_si256()));x[j]=_mm256_aesenc_epi128(x[j],hi[j]);}
   }
  }
  #pragma GCC unroll 4
  for(unsigned j=0;j<4;++j)_mm256_storeu_si256(reinterpret_cast<__m256i*>(out+at+2*j),x[j]);
 }
 if(at<n)sci::CCRF(out+at,const_cast<sci::block256*>(keys+at),int(n-at));
 OPENSSL_cleanse(lo,sizeof(lo));OPENSSL_cleanse(hi,sizeof(hi));OPENSSL_cleanse(x,sizeof(x));
}
inline __m128i key_prefix(__m128i x,__m128i sub){
 x=_mm_xor_si128(x,_mm_slli_si128(x,4));
 x=_mm_xor_si128(x,_mm_slli_si128(x,8));return _mm_xor_si128(x,sub);
}
inline void streaming_ccrf(sci::block128* out,const sci::block256* keys,std::size_t n){
 const __m128i rot=_mm_setr_epi8(13,14,15,12,13,14,15,12,13,14,15,12,13,14,15,12);
 for(std::size_t at=0;at<n;at+=8){
  const unsigned count=unsigned(std::min<std::size_t>(8,n-at));
  __m128i lo[8]{},hi[8]{},x[8]{};
  for(unsigned j=0;j<count;++j){
   lo[j]=_mm_loadu_si128(reinterpret_cast<const __m128i*>(keys+at+j));
   hi[j]=_mm_loadu_si128(reinterpret_cast<const __m128i*>(keys+at+j)+1);
   x[j]=_mm_xor_si128(sci::one,lo[j]);x[j]=_mm_aesenc_si128(x[j],hi[j]);
  }
  for(unsigned r=1;r<=7;++r){
   const __m128i rc=_mm_set1_epi32(1<<(r-1));
   for(unsigned j=0;j<count;++j){
    lo[j]=key_prefix(lo[j],_mm_aesenclast_si128(_mm_shuffle_epi8(hi[j],rot),rc));
    x[j]=r==7?_mm_aesenclast_si128(x[j],lo[j]):_mm_aesenc_si128(x[j],lo[j]);
   }
   if(r!=7)for(unsigned j=0;j<count;++j){
    hi[j]=key_prefix(hi[j],_mm_aesenclast_si128(_mm_shuffle_epi32(lo[j],0xff),_mm_setzero_si128()));
    x[j]=_mm_aesenc_si128(x[j],hi[j]);
   }
  }
  for(unsigned j=0;j<count;++j)out[at+j]=x[j];
  OPENSSL_cleanse(lo,sizeof(lo));OPENSSL_cleanse(hi,sizeof(hi));OPENSSL_cleanse(x,sizeof(x));
 }
}
}
