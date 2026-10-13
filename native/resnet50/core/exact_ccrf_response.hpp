#pragma once
#include "resnet50/core/types.hpp"
#include "resnet50/core/streaming_ccrf.hpp"
namespace aqcf_tuple {
// Original message pack/unpack and full AES256 CCRF function. Only exact
// instruction scheduling, actual tail hashing, and scratch allocation differ.
// Both low-traffic and ordinary control may use this surrounding improvement.
template<class K> void exact_got_post(K& k,int party,std::uint64_t** table,
 const std::uint8_t* choices,std::uint64_t* answer,std::size_t n){
 using aqcf2::U;using aqcf2::require;
 require(have_parallel_aes(),"original AES256 parallel instruction available");
 require(k.N>=4&&k.N<=32&&k.l>=1&&k.l<=64&&n>0,"original GOT public dimensions");
 struct alignas(32) Key{sci::block256 v;};struct alignas(16) Pad{sci::block128 v;};
 const auto chunk=std::min<std::size_t>(k.ro_batch_size,n);
 std::vector<Key>keys(party==1?chunk*k.N:0);std::vector<Pad>pads(party==1?chunk*k.N:chunk);
 std::vector<U> wire((chunk*k.N*k.l+63)/64);
 try{
  for(std::size_t at=0;at<n;at+=chunk){
   const auto rows=std::min(chunk,n-at);const auto words=(rows*k.N*k.l+63)/64;
   if(party==1){
    for(std::size_t j=0;j<rows;++j)for(int c=0;c<k.N;++c)keys[j*k.N+c].v=sci::xorBlocks(k.qT[at+j],k.c_AND_s[c]);
    parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),reinterpret_cast<sci::block256*>(keys.data()),rows*k.N);
    sci::pack_ot_messages<U>(wire.data(),table+at,reinterpret_cast<sci::block128*>(pads.data()),words,rows,k.l,k.N);
    k.io->send_data(wire.data(),int(words*8));
   }else{
    k.io->recv_data(wire.data(),int(words*8));
    parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),k.tT+at,rows);
    sci::unpack_ot_messages<U>(answer+at,choices+at,wire.data(),reinterpret_cast<sci::block128*>(pads.data()),rows,k.l,k.N);
   }
  }
 }catch(...){if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}throw;}
 if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}
 if(!keys.empty())OPENSSL_cleanse(keys.data(),keys.size()*sizeof(Key));
 OPENSSL_cleanse(pads.data(),pads.size()*sizeof(Pad));OPENSSL_cleanse(wire.data(),wire.size()*sizeof(U));
}
}
