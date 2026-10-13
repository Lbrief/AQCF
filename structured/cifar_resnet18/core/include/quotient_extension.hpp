#pragma once
// Root candidate: exact code-fiber synthesis and constant-column seed lifting.
// The original 256-bit KKOT keys, base OTs and CCRF remain unchanged.
#include "types.hpp"
#include "BuildingBlocks/aux-protocols.h"
#include <array>
#include <chrono>
#include <memory>
#include <mutex>
namespace aqcf17 {
using aqcf2::U;using aqcf2::require;
using K=sci::SplitKKOT<sci::NetIO>;
struct alignas(32) Block {sci::block256 v;};
inline bool code_bit(unsigned choice,unsigned col){return (sci::WH_Code[choice][col/64]>>(col%64))&1;}
inline void certify_code(unsigned N){
    require(N>=4&&N<=32&&!(N&(N-1)),"quotient alphabet");
    // Immutable public SDK code; certify once per alphabet, not per OT call.
    static std::array<std::once_flag,6> once;
    std::call_once(once[__builtin_ctz(N)],[N](){
    for(unsigned a=0;a<N;++a)for(unsigned col=0;col<256;++col)
        require(code_bit(a,col)==bool(__builtin_parity(a&col)),"actual Walsh bit convention");
    for(unsigned a=0;a<N;++a)for(unsigned b=a+1;b<N;++b){unsigned old=0,kept=0;
        for(unsigned c=0;c<256;++c){unsigned diff=code_bit(a,c)^code_bit(b,c);old+=diff;
            if(c%N)kept+=diff;else require(!diff,"constant column cannot carry choice difference");}
        require(old==128&&kept==128,"unchanged pairwise hidden support");}
    });
}
class CommonColumns {
    std::array<std::unique_ptr<sci::PRG256>,256> stream_;
    bool ready_=false;
public:
    U setup_bytes=0,elided_body_bytes=0,code_fiber_bytes=0;
    void initialize(K& k){
        require(k.setup&&k.lambda==256&&k.block_size==16384&&k.precomp_batch_size==0,"original fresh KKOT contract");
        certify_code(k.N);if(ready_)return;
        std::vector<Block> seeds(256/k.N);
        if(k.party==2){for(unsigned i=0,j=0;i<256;i+=k.N,++j)seeds[j].v=k.k0[i];
            k.io->send_data(seeds.data(),int(seeds.size()*32));k.io->flush();}
        else {k.io->recv_data(seeds.data(),int(seeds.size()*32));
            for(unsigned i=0,j=0;i<256;i+=k.N,++j)stream_[i]=std::make_unique<sci::PRG256>(&seeds[j].v);}
        setup_bytes+=seeds.size()*32;OPENSSL_cleanse(seeds.data(),seeds.size()*32);ready_=true;
    }
    void pre(K& k,const std::uint8_t* choices,int actual,bool quotient,bool materialized=false){
        require(k.setup&&k.precomp_batch_size==0&&actual>0&&actual<=65536,"bounded initialized extension");
        certify_code(k.N);if(quotient)initialize(k);
        // Retained exact public-tail correction from joint_v2.
        const int length=((actual+255)/256)*256,max_bs=std::min(k.block_size,length);
        const int cols=quotient?256-256/k.N:256;

        // The pinned got_*_post frees these arrays but does not null the public
        // pointer fields. Completed legacy B2A/compare calls therefore leave
        // nonnull dangling fields. Like original send_pre/recv_pre, overwrite
        // them without freeing them again. Liveness is tracked by Lookup,
        // not inferred from those legacy pointer values.
        if(k.party==1){
            k.qT=new(std::align_val_t(32)) sci::block256[length];if(!k.precomp_masks)k.precompute_masks();}
        else {require(choices,"receiver choices required");
            k.tT=new(std::align_val_t(32)) sci::block256[length];k.prg.random_data(k.extended_r,max_bs);}
        for(int start=0;start<length;start+=max_bs){
            const int bs=std::min(max_bs,length-start),bytes=bs/8,blocks=bs/256;
            std::vector<Block> matrix(bs),wire(std::size_t(cols)*blocks);
            if(k.party==2){
                unsigned bits=0;while((1U<<bits)<unsigned(k.N))++bits;
                std::vector<Block> full_rows(materialized?bs:0),full_columns(materialized?bs:0);
                std::vector<std::vector<std::uint8_t>> planes(bits,std::vector<std::uint8_t>(bytes));
                for(int r=0;r<bs;++r){const U c=start+r<actual?choices[start+r]:(k.extended_r[start+r-actual]&(k.N-1));
                    require(c<unsigned(k.N),"public lookup alphabet");
                    if(materialized)full_rows[r].v=_mm256_lddqu_si256(reinterpret_cast<const __m256i*>(sci::WH_Code[c]));
                    else for(unsigned b=0;b<bits;++b)planes[b][r/8]|=((c>>b)&1)<<(r%8);}
                std::vector<std::vector<std::uint8_t>> fiber(k.N,std::vector<std::uint8_t>(bytes));
                if(materialized)sci::sse_trans(reinterpret_cast<std::uint8_t*>(full_columns.data()),reinterpret_cast<std::uint8_t*>(full_rows.data()),bs,256);
                else for(unsigned c=1;c<unsigned(k.N);++c){unsigned bit=__builtin_ctz(c),prev=c&(c-1);
                    for(int j=0;j<bytes;++j)fiber[c][j]=fiber[prev][j]^planes[bit][j];}
                if(!materialized)code_fiber_bytes+=U(k.N)*bytes;
                unsigned sent=0;
                for(unsigned col=0;col<256;++col){auto* dst=&matrix[col*blocks].v;
                    k.G0[col].random_data(dst,bytes);
                    if(quotient&&col%k.N==0){k.G1[col].counter+=bytes/16;continue;}
                    auto* message=&wire[sent*blocks].v;++sent;k.G1[col].random_data(message,bytes);
                    sci::xorBlocks_arr(message,dst,message,blocks);
                    auto* mb=reinterpret_cast<std::uint8_t*>(message);
                    if(materialized)sci::xorBlocks_arr(message,&full_columns[col*blocks].v,message,blocks);
                    else for(int j=0;j<bytes;++j)mb[j]^=fiber[col%k.N][j];
                }
                require(sent==unsigned(cols),"receiver active-column count");
                k.io->send_data(wire.data(),int(wire.size()*32));
                sci::sse_trans(reinterpret_cast<std::uint8_t*>(k.tT+start),reinterpret_cast<std::uint8_t*>(matrix.data()),256,bs);
            }else{
                k.io->recv_data(wire.data(),int(wire.size()*32));unsigned received=0;
                for(unsigned col=0;col<256;++col){auto* dst=&matrix[col*blocks].v;
                    if(quotient&&col%k.N==0){stream_[col]->counter=k.G0[col].counter;
                        stream_[col]->random_data(dst,bytes);k.G0[col].counter+=bytes/16;continue;}
                    k.G0[col].random_data(dst,bytes);const auto* message=&wire[received*blocks].v;++received;
                    if(k.s[col])sci::xorBlocks_arr(dst,dst,message,blocks);
                }
                require(received==unsigned(cols),"sender active-column count");
                sci::sse_trans(reinterpret_cast<std::uint8_t*>(k.qT+start),reinterpret_cast<std::uint8_t*>(matrix.data()),256,bs);
            }
            if(quotient)elided_body_bytes+=U(256/k.N)*bytes;
        }
    }
};
class Lookup {
    int party_;sci::OTPack& ot_;std::string mode_;bool executing_=false;
    std::array<CommonColumns,5> banks_;
public:
    U rows=0,calls=0;double extension_s=0,response_s=0;
    Lookup(int p,sci::OTPack& ot,const std::string& mode):party_(p),ot_(ot),mode_(mode){
#if !(defined(AQCF_R17_PUBLIC_BUILD) || defined(AQCF_EXACT_QUARTER_BUILD)) && !defined(AQCF_R17_REAL_N1)
        throw std::runtime_error("quotient extension remains public-only");
#endif
        require(mode=="stock"||mode=="bulk"||mode=="fiber"||mode=="quotient","extension mode");
        require(ot_.do_setup,"independent base OT setup required");
        // Reject the SDK copy() path: its reused per-alphabet seeds are unsafe
        // for revealing a coordinate constant only in one alphabet.
        for(unsigned a=0;a<8;++a)for(unsigned b=a+1;b<8;++b)for(unsigned c=0;c<256;++c)
            require(std::memcmp(&ot_.kkot[a]->k0[c],&ot_.kkot[b]->k0[c],32)!=0,"no cross-alphabet base-key reuse");
    }
    U seed_bytes()const{U v=0;for(auto&b:banks_)v+=b.setup_bytes;return v;}
    U elided_bytes()const{U v=0;for(auto&b:banks_)v+=b.elided_body_bytes;return v;}
    std::vector<U> execute(std::vector<U>& table,const std::vector<U>&choice,std::size_t n,unsigned in,unsigned out){
        require(!executing_,"single synchronous owner of this OT lane");
        struct CallScope {bool& live;explicit CallScope(bool& v):live(v){live=true;}~CallScope(){live=false;}} call_scope(executing_);
        require(in>=2&&in<=5&&out>=1&&out<=64&&n>0&&n<=65536&&choice.size()==n,"complete original lookup shape");
        auto& k=*ot_.kkot[in-1];require(k.N==int(1U<<in)&&k.precomp_batch_size==0,"original alphabet and no precomputed bank");
        std::vector<std::uint8_t> ch(n);std::vector<U*> ptr;std::vector<U> answer(n);
        if(party_==1){require(table.size()==n*(U(1)<<in),"one-owner table");ptr.resize(n);for(U j=0;j<n;++j)ptr[j]=table.data()+j*k.N;}
        else for(U j=0;j<n;++j){require(choice[j]<unsigned(k.N),"private choice range");ch[j]=choice[j];}
        k.l=out;auto begin=std::chrono::steady_clock::now();
        if(mode_=="stock"){if(party_==1)k.send_pre(n);else k.recv_pre(ch.data(),n);}
        else banks_[in-1].pre(k,party_==2?ch.data():nullptr,n,mode_=="quotient",mode_=="bulk");
        auto middle=std::chrono::steady_clock::now();
        if(party_==1){k.got_send_post<U>(ptr.data(),n);k.qT=nullptr;}
        else {k.got_recv_post<U>(answer.data(),ch.data(),n);k.tT=nullptr;}
        auto end=std::chrono::steady_clock::now();extension_s+=std::chrono::duration<double>(middle-begin).count();response_s+=std::chrono::duration<double>(end-middle).count();
        rows+=n;++calls;return answer;
    }
};
}
