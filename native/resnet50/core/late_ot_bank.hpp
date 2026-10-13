#pragma once
// R14: standard random-OT -> chosen-OT conversion, NOT a new OT primitive.
// All preparation is called synchronously inside the paid decision boundary.
#include "resnet50/core/types.hpp"
#include <openssl/crypto.h>
#include <array>
#include <string>
namespace aqcf2 {
struct RandomNBitOT {
    unsigned in=0; std::size_t rows=0,cursor=0;
    std::vector<std::uint32_t> keys; // sender: N independent ONE-BIT pads per row
    std::vector<std::uint8_t> rho,selected;
    RandomNBitOT()=default;
    RandomNBitOT(unsigned i,std::size_t n,int p):in(i),rows(n){
        require((p==1||p==2)&&i>=2&&i<=5&&n>0&&n<=65536,"bounded public NCO bank");
        if(p==1)keys.resize(n);else {rho.resize(n);selected.resize(n);}
    }
    RandomNBitOT(const RandomNBitOT&)=delete;
    RandomNBitOT& operator=(const RandomNBitOT&)=delete;
    RandomNBitOT(RandomNBitOT&& x)noexcept { *this=std::move(x); }
    RandomNBitOT& operator=(RandomNBitOT&& x)noexcept {
        if(this!=&x){erase();in=x.in;rows=x.rows;cursor=x.cursor;
            keys=std::move(x.keys);rho=std::move(x.rho);selected=std::move(x.selected);
            x.in=0;x.rows=0;x.cursor=0;}return *this;
    }
    ~RandomNBitOT(){erase();}
    void erase(){
        if(!keys.empty())OPENSSL_cleanse(keys.data(),keys.size()*sizeof(keys[0]));
        if(!rho.empty())OPENSSL_cleanse(rho.data(),rho.size());
        if(!selected.empty())OPENSSL_cleanse(selected.data(),selected.size());
    }
    void finish()const {require(cursor==rows,"entire NCO bank consumed exactly once");}
};
// In the XOR reduction, e=rho XOR choice; packet[k]=T[k XOR e] XOR K[k].
// In the equally optimized mature cyclic reduction, e=rho-choice (mod N);
// packet[k]=T[k-e (mod N)] XOR K[k]. They have identical OT-row and wire counts; local index arithmetic differs.
// Revealing both pads or reusing a row would invalidate receiver privacy.
template<class B> std::vector<U> late_take(B& s,RandomNBitOT& b,
        const std::vector<U>& table,const std::vector<U>& choice,bool cyclic){
    require(b.in>=2&&b.in<=5,"initialized NCO alphabet");
    const std::size_t n=choice.size(); const U N=U(1)<<b.in;
    require(n>0&&b.cursor<=b.rows&&n<=b.rows-b.cursor,"NCO exhaustion/reuse");
    require((s.party==1&&table.size()==n*N)||(s.party==2&&table.empty()),"one-owner NCO table");
    std::vector<std::uint8_t> correction(n*b.in),packet(n*N);
    if(s.party==2)for(std::size_t j=0;j<n;++j){
        require(choice[j]<N&&b.rho[b.cursor+j]<N,"NCO choice range");
        const U e=cyclic?(b.rho[b.cursor+j]-choice[j])&(N-1):b.rho[b.cursor+j]^choice[j];
        for(unsigned k=0;k<b.in;++k)correction[j*b.in+k]=(e>>k)&1;
    }
    correction=s.transfer_masked_bits(2,correction);
    if(s.party==1)for(std::size_t j=0;j<n;++j){
        U e=0;for(unsigned k=0;k<b.in;++k)e|=U(correction[j*b.in+k])<<k;
        for(U k=0;k<N;++k){U c=cyclic?(k-e)&(N-1):(k^e);
            require(table[j*N+c]<2,"NCO one-bit response");
            packet[j*N+k]=table[j*N+c]^((b.keys[b.cursor+j]>>k)&1U);
        }
    }
    packet=s.transfer_masked_bits(1,packet); std::vector<U> got(n);
    if(s.party==2)for(std::size_t j=0;j<n;++j)
        got[j]=packet[j*N+b.rho[b.cursor+j]]^b.selected[b.cursor+j];
    if(s.party==1)OPENSSL_cleanse(b.keys.data()+b.cursor,n*sizeof(b.keys[0]));
    else {OPENSSL_cleanse(b.rho.data()+b.cursor,n);OPENSSL_cleanse(b.selected.data()+b.cursor,n);}
    b.cursor+=n;return got;
}
} // namespace aqcf2
