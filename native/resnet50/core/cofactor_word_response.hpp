#pragma once
// Exact local representation of the existing private cofactor responses.
// These words are NEVER sent in clear; the original full AES256 OT pads apply.
// No new cryptographic primitive or novelty-priority claim.
#include "resnet50/core/types.hpp"
#include "resnet50/core/streaming_ccrf.hpp"
namespace aqcf_cofactor_word {
using aqcf2::U; using aqcf2::require;
inline U threshold(unsigned a,unsigned carry,unsigned d){
    const unsigned B=1U<<d, cut=B-a-carry;
    const U all=(U(1)<<B)-1;
    return all^((U(1)<<cut)-1);
}
inline U spread16(U x){
    x=(x|(x<<8))&0x00ff00ffULL;
    x=(x|(x<<4))&0x0f0f0f0fULL;
    x=(x|(x<<2))&0x33333333ULL;
    return (x|(x<<1))&0x55555555ULL;
}
inline U pair(unsigned a,unsigned r,unsigned next,unsigned p0,unsigned p1,unsigned d){
    const U all=(U(1)<<(1U<<d))-1;
    const U v0=threshold(a,r,d)^((next^p0)?all:0);
    const U v1=threshold(a,r^1,d)^((next^p1)?all:0);
    return spread16(v0)|(spread16(v1)<<1);
}
inline void pack(U* wire,const U* table,const sci::block128* pads,
                 std::size_t rows,unsigned N,unsigned width){
    const unsigned rowbits=N*width;
    require(rowbits>0&&rowbits<=64&&!(rowbits&(rowbits-1)),"whole compact OT rows");
    const U mm=aqcf2::mask(width),rm=rowbits==64?~U(0):((U(1)<<rowbits)-1);
    const std::size_t words=(rows*rowbits+63)/64;
    std::fill(wire,wire+words,0);
    for(std::size_t j=0;j<rows;++j){
        U masked=table[j]&rm;
        for(unsigned c=0;c<N;++c)
            masked^=(U(_mm_extract_epi64(pads[j*N+c],0))&mm)<<(c*width);
        const auto pos=j*rowbits;
        wire[pos/64]|=masked<<(pos%64);
    }
}
template<class K> void got_post(K& k,int party,const U* table,
                               const std::uint8_t* choices,U* answer,std::size_t n){
    require(k.N>=4&&k.N<=32&&k.l>=1&&k.N*k.l<=64&&n>0,"compact public dimensions");
    require(aqcf_tuple::have_parallel_aes(),"unchanged AES256 function");
    struct alignas(32) Key{sci::block256 v;};struct alignas(16) Pad{sci::block128 v;};
    const auto chunk=std::min<std::size_t>(k.ro_batch_size,n);
    std::vector<Key> keys(party==1?chunk*k.N:0);
    std::vector<Pad> pads(party==1?chunk*k.N:chunk);
    std::vector<U> wire((chunk*k.N*k.l+63)/64);
    try{
        for(std::size_t at=0;at<n;at+=chunk){
            const auto rows=std::min(chunk,n-at),words=(rows*k.N*k.l+63)/64;
            if(party==1){
                for(std::size_t j=0;j<rows;++j)for(int c=0;c<k.N;++c)
                    keys[j*k.N+c].v=sci::xorBlocks(k.qT[at+j],k.c_AND_s[c]);
                aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),reinterpret_cast<sci::block256*>(keys.data()),rows*k.N);
                pack(wire.data(),table+at,reinterpret_cast<sci::block128*>(pads.data()),rows,k.N,k.l);
                k.io->send_data(wire.data(),int(words*8));
            }else{
                k.io->recv_data(wire.data(),int(words*8));
                aqcf_tuple::parallel_ccrf(reinterpret_cast<sci::block128*>(pads.data()),k.tT+at,rows);
                sci::unpack_ot_messages<U>(answer+at,choices+at,wire.data(),reinterpret_cast<sci::block128*>(pads.data()),rows,k.l,k.N);
            }
        }
    }catch(...){if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}throw;}
    if(party==1){delete[]k.qT;k.qT=nullptr;}else{delete[]k.tT;k.tT=nullptr;}
    if(!keys.empty())OPENSSL_cleanse(keys.data(),keys.size()*sizeof(Key));
    OPENSSL_cleanse(pads.data(),pads.size()*sizeof(Pad));OPENSSL_cleanse(wire.data(),wire.size()*sizeof(U));
}
}
