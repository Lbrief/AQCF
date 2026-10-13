#pragma once
#include "resnet50/engine/pointwise_owner.hpp"
#include <immintrin.h>
// Exact local arithmetic only. No IO, keys, coins or cross-request state.
namespace correction341 {
using U=std::uint64_t;using I=std::int64_t;
constexpr U M=U(1)<<37, MASK=M-1, LOW=(U(1)<<19)-1;
// w is an original signed coefficient with |w| <= 2^30.
// x=x0+2^19*x1, 0<=x0<2^19, 0<=x1<2^18.
// Both signed 32x32 products are exact. SIMD additions wrap mod2^64;
// projecting to mod2^37 commutes with every addition and multiplication.
inline __m256i product4(__m256i x,__m256i w){
 auto lo=_mm256_and_si256(x,_mm256_set1_epi64x(LOW));
 auto hi=_mm256_srli_epi64(x,19);
 return _mm256_add_epi64(_mm256_mul_epi32(lo,w),
                        _mm256_slli_epi64(_mm256_mul_epi32(hi,w),19));
}
template<class Work> std::vector<U> pointwise(const pw25::Shape& s,
 const pw25::Owner& owner,const std::vector<U>& v,Work work){
 aqcf3::require(v.size()==s.Ci*s.L*s.stride*s.stride&&owner.w.size()==s.Co*s.Ci,
                "341 original pointwise dimensions");
 std::vector<U> sample(s.Ci*s.L),out(s.Co*s.L);
 work(sample.size(),"CORRECTION341_SAMPLE",[&](U lo,U hi,unsigned){
  for(U j=lo;j<hi;++j){U c=j/s.L,k=j%s.L;
   sample[j]=v[pw25::source_index(s,c,k/s.W,k%s.W)];
   aqcf3::require(sample[j]<M,"341 canonical selected delta");}
 });
 work(s.Co,"CORRECTION341_POINTWISE",[&](U lo,U hi,unsigned){
  for(U o=lo;o<hi;++o)for(U start=0;start<s.L;start+=64){
   U width=std::min<U>(64,s.L-start),end=width&~U(3);
   alignas(32) U accum[64]{};
   for(U i=0;i<s.Ci;++i){
    I w=owner.w[o*s.Ci+i];aqcf3::require(w>=-I(U(1)<<30)&&w<=I(U(1)<<30),"341 bound signed coefficient");
    auto weight=_mm256_set1_epi64x(w);const U* x=sample.data()+i*s.L+start;
    for(U k=0;k<end;k+=4){
     auto a=_mm256_load_si256(reinterpret_cast<const __m256i*>(accum+k));
     auto b=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(x+k));
     _mm256_store_si256(reinterpret_cast<__m256i*>(accum+k),_mm256_add_epi64(a,product4(b,weight)));
    }
    for(U k=end;k<width;++k)accum[k]+=U(w)*x[k];
   }
   for(U k=0;k<width;++k)out[o*s.L+start+k]=accum[k]&MASK;
   OPENSSL_cleanse(accum,sizeof(accum));
  }
 });
 OPENSSL_cleanse(sample.data(),sample.size()*sizeof(U));
 return out;
}
template<class Work> std::vector<U> batchnorm(const aqcf_bn19::Geometry& g,
 const aqcf_bn19::PrivateOwner& owner,const std::vector<U>& v,Work work){
 aqcf3::require(v.size()==U(g.channels)*g.L&&owner.weight.size()==g.channels,
                "341 original BN dimensions");
 std::vector<U>out(v.size());
 work(g.channels,"CORRECTION341_BN",[&](U lo,U hi,unsigned){
  for(U c=lo;c<hi;++c){I w=owner.weight[c];
   aqcf3::require(w>=-I(U(1)<<30)&&w<=I(U(1)<<30),"341 bound BN coefficient");
   auto weight=_mm256_set1_epi64x(w),mask=_mm256_set1_epi64x(MASK);U j=0;
   for(;j+4<=g.L;j+=4){auto x=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(v.data()+c*g.L+j));
    auto y=_mm256_and_si256(product4(x,weight),mask);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(out.data()+c*g.L+j),y);}
   for(;j<g.L;++j)out[c*g.L+j]=(U(w)*v[c*g.L+j])&MASK;
  }
 });return out;
}
}

