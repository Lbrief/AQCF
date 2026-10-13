#pragma once
// Same full AES256 CCRF: fuse key construction, evaluation and used-bit packing.
// This is implementation fusion, NOT a new OT primitive or weaker hash.
#include "squeezenet/engine/types.hpp"
#include "squeezenet/engine/streaming_ccrf.hpp"
#ifndef AQCF_FUSED_COFACTOR
#define AQCF_FUSED_COFACTOR 0
#endif
namespace aqcf_fused_cofactor {
using aqcf2::U;using aqcf2::require;
__attribute__((target("avx2,vaes"))) inline void sender_pack(
 U* wire,const U* table,const sci::block256* rows,const sci::block256* masks,
 std::size_t n,unsigned N,unsigned width){
 require(N>=8&&N<=128&&!(N&(N-1))&&(width==1||width==2),"fused response public shape");
 const std::size_t rowbits=N*width,stride=(rowbits+63)/64,words=(n*rowbits+63)/64;
 const U group_mask=(U(1)<<(8*width))-1,mask=aqcf2::mask(width);
 std::fill(wire,wire+words,0);
 const __m256i rot=_mm256_broadcastsi128_si256(_mm_setr_epi8(13,14,15,12,13,14,15,12,13,14,15,12,13,14,15,12));
 __m256i lo[4]{},hi[4]{},x[4]{};
 for(std::size_t row=0;row<n;++row){
  const __m256i base=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(rows+row));
  for(unsigned c=0;c<N;c+=8){
   #pragma GCC unroll 4
   for(unsigned j=0;j<4;++j){
    const auto ka=_mm256_xor_si256(base,_mm256_loadu_si256(reinterpret_cast<const __m256i*>(masks+c+2*j)));
    const auto kb=_mm256_xor_si256(base,_mm256_loadu_si256(reinterpret_cast<const __m256i*>(masks+c+2*j+1)));
    lo[j]=_mm256_permute2x128_si256(ka,kb,0x20);
    hi[j]=_mm256_permute2x128_si256(ka,kb,0x31);
    x[j]=_mm256_xor_si256(_mm256_broadcastsi128_si256(sci::one),lo[j]);
    x[j]=_mm256_aesenc_epi128(x[j],hi[j]);
   }
   #pragma GCC unroll 7
   for(unsigned r=1;r<=7;++r){
    const auto rc=_mm256_set1_epi32(1<<(r-1));
    #pragma GCC unroll 4
    for(unsigned j=0;j<4;++j){
     lo[j]=aqcf_tuple::key_prefix_pair(lo[j],_mm256_aesenclast_epi128(_mm256_shuffle_epi8(hi[j],rot),rc));
     x[j]=r==7?_mm256_aesenclast_epi128(x[j],lo[j]):_mm256_aesenc_epi128(x[j],lo[j]);
    }
    if(r!=7){
     #pragma GCC unroll 4
     for(unsigned j=0;j<4;++j){
      hi[j]=aqcf_tuple::key_prefix_pair(hi[j],_mm256_aesenclast_epi128(_mm256_shuffle_epi32(lo[j],0xff),_mm256_setzero_si256()));
      x[j]=_mm256_aesenc_epi128(x[j],hi[j]);
     }
    }
   }
   U bits=0;
   #pragma GCC unroll 4
   for(unsigned j=0;j<4;++j){
    bits|=(U(_mm_cvtsi128_si64(_mm256_castsi256_si128(x[j])))&mask)<<(2*j*width);
    bits|=(U(_mm_cvtsi128_si64(_mm256_extracti128_si256(x[j],1)))&mask)<<((2*j+1)*width);
   }
   const std::size_t local=c*width,pos=row*rowbits+local;
   const U plain=(table[row*stride+local/64]>>(local%64))&group_mask;
   wire[pos/64]|=(plain^bits)<<(pos%64);
  }
 }
 OPENSSL_cleanse(lo,sizeof(lo));OPENSSL_cleanse(hi,sizeof(hi));OPENSSL_cleanse(x,sizeof(x));
}
template<class K> void got_post(K& k,int party,const U* table,
 const std::uint8_t* choices,U* answer,std::size_t n){
 require(k.N>=8&&k.N<=128&&(k.l==1||k.l==2)&&n>0,"fused chosen lookup shape");
 require(aqcf_tuple::have_parallel_aes(),"retain full AES256 hardware function");
 struct alignas(16) Pad{sci::block128 v;};
 const auto chunk=std::min<std::size_t>(n,k.ro_batch_size),rowbits=std::size_t(k.N)*k.l,stride=(rowbits+63)/64;
 std::vector<U> wire((chunk*rowbits+63)/64);
 std::vector<Pad> pads(party==2?chunk:0);
 auto cleanse=[&]{OPENSSL_cleanse(wire.data(),wire.size()*sizeof(U));if(!pads.empty())OPENSSL_cleanse(pads.data(),pads.size()*sizeof(Pad));};
 try{
  for(std::size_t at=0;at<n;at+=chunk){
   const auto rows=std::min(chunk,n-at),words=(rows*rowbits+63)/64;
   if(party==1){
    sender_pack(wire.data(),table+at*stride,k.qT+at,k.c_AND_s,rows,k.N,k.l);
    k.io->send_data(wire.data(),int(words*8));
   }else{
    k.io->recv_data(wire.data(),int(words*8));
    aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),k.tT+at,rows);
    for(std::size_t j=0;j<rows;++j){
     require(choices[at+j]<k.N,"fused private choice range");
     const std::size_t pos=j*rowbits+choices[at+j]*k.l;
     answer[at+j]=((wire[pos/64]>>(pos%64))^U(_mm_extract_epi64(pads[j].v,0)))&aqcf2::mask(k.l);
    }
   }
  }
 }catch(...){if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}cleanse();throw;}
 if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}
 cleanse();
}
}
