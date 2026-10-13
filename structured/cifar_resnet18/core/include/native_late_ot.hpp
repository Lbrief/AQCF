#pragma once
// Calls the pinned SCI SplitKKOT RANDOM preprocessing unchanged. There is no
// AES replacement, security-parameter change, offline cache, seed reuse or
// dependency patch here. The short returned pads are exactly low bits of the
// stock CCRF outputs (same truncation as stock one-bit GOT).
#include "late_ot_bank.hpp"
#include "BuildingBlocks/aux-protocols.h"
#include <climits>
namespace aqcf2 {
inline RandomNBitOT native_random_n_ot(int party,sci::OTPack& ot,std::size_t n,unsigned in){
#if !defined(AQCF_R14_PUBLIC_BUILD) && !defined(AQCF_EXACT_QUARTER_BUILD)
    throw std::runtime_error("R14 native use remains PUBLIC ONLY; no private admission");
#endif
    require((party==1||party==2)&&in>=2&&in<=5&&n>0&&n<=65536,"native NCO public bounds");
    auto& k=*ot.kkot[in-1];
    require(sci::AES_BATCH_SIZE==2048,"frozen public cost model AES_BATCH_SIZE");
    require(k.lambda==256&&k.N==int(1U<<in)&&k.ro_batch_size==2048&&
            k.block_size==16384&&k.precomp_batch_size==0,"unchanged pinned KKOT contract");
    RandomNBitOT out(in,n,party);
    k.set_precomp_batch_size(int(n)); k.l=1;
    // Synchronous, included in the full decision timer and application bytes.
    k.preprocess();
    require(k.counter==0,"stock preprocessor must yield an unused bank");
    if(party==1)for(std::size_t j=0;j<n;++j)for(unsigned c=0;c<(1U<<in);++c)
        out.keys[j]|=std::uint32_t(k.h[c][j]&1U)<<c;
    if(party==2)for(std::size_t j=0;j<n;++j){
        require(k.r_off[j]<(1U<<in),"stock random choice range");
        out.rho[j]=k.r_off[j];out.selected[j]=k.h[0][j]&1U;
    }
    // Remove BOTH legacy byte/word copies, not only our compressed copy.
    const unsigned banks=party==1?(1U<<in):1;
    for(unsigned c=0;c<banks;++c){OPENSSL_cleanse(k.h[c],n);OPENSSL_cleanse(k.h64[c],n*sizeof(U));}
    if(party==2)OPENSSL_cleanse(k.r_off,n);
    // Stock got_*_offline already freed qT/tT. Null the non-owning stale field.
    if(party==1)k.qT=nullptr;else k.tT=nullptr;
    k.counter=int(n);k.set_precomp_batch_size(0);
    return out;
}
} // namespace aqcf2
