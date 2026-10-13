#pragma once
#include "squeezenet/engine/types.hpp"
#include "squeezenet/engine/streaming_ccrf.hpp"
#include "squeezenet/engine/fused_cofactor_response.hpp"
namespace aqcf_wide_cofactor {
using aqcf2::U;using aqcf2::require;
// Dedicated exact one-bit rows, N=64/128. Not a general wider message ABI.
// Row words retain ALL N independent full-AES256 protected responses.
template<class K> void got_post(K& k,int party,const U* table,
 const std::uint8_t* choices,U* answer,std::size_t n){
 if constexpr(AQCF_FUSED_COFACTOR){aqcf_fused_cofactor::got_post(k,party,table,choices,answer,n);return;}
 require((k.N==64||k.N==128)&&k.l==1&&n>0,"wide cofactor public shape");
 require(aqcf_tuple::have_parallel_aes(),"unchanged AES256 CCRF");
 struct alignas(32) Key{sci::block256 v;};struct alignas(16) Pad{sci::block128 v;};
 // Keep scratch bounded by the legacy N=32 hash-cell budget. This changes
 // response frame boundaries only; never split/reset the OT extension.
 const auto chunk=std::min<std::size_t>(n,std::max<std::size_t>(1,std::size_t(k.ro_batch_size)*32/k.N));
 const std::size_t stride=std::size_t(k.N)/64;
 std::vector<Key> keys(party==1?chunk*k.N:0);
 std::vector<Pad> pads(party==1?chunk*k.N:chunk);
 std::vector<U> wire(chunk*stride);
 auto cleanse=[&]{
  if(!keys.empty())OPENSSL_cleanse(keys.data(),keys.size()*sizeof(Key));
  OPENSSL_cleanse(pads.data(),pads.size()*sizeof(Pad));
  OPENSSL_cleanse(wire.data(),wire.size()*sizeof(U));
 };
 try{
  for(std::size_t at=0;at<n;at+=chunk){
   const auto rows=std::min(chunk,n-at),words=rows*stride;
   if(party==1){
    for(std::size_t j=0;j<rows;++j)for(int c=0;c<k.N;++c)
     keys[j*k.N+c].v=sci::xorBlocks(k.qT[at+j],k.c_AND_s[c]);
    aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),reinterpret_cast<sci::block256*>(keys.data()),rows*k.N);
    std::copy_n(table+at*stride,words,wire.data());
    for(std::size_t j=0;j<rows;++j)for(int c=0;c<k.N;++c)
     wire[j*stride+c/64]^=(U(_mm_extract_epi64(pads[j*k.N+c].v,0))&1)<<(c%64);
    k.io->send_data(wire.data(),int(words*sizeof(U)));
   }else{
    k.io->recv_data(wire.data(),int(words*sizeof(U)));
    aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),k.tT+at,rows);
    for(std::size_t j=0;j<rows;++j){
     const unsigned c=choices[at+j];require(c<unsigned(k.N),"wide private choice range");
     answer[at+j]=((wire[j*stride+c/64]>>(c%64))^U(_mm_extract_epi64(pads[j].v,0)))&1;
    }
   }
  }
 }catch(...){
  if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}
  cleanse();throw;
 }
 if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}
 cleanse();
}
}
