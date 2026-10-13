#pragma once
#include "resnet50/core/batch_norm_affine_core.hpp"
#include <limits>

namespace aqcf_input186 {
namespace b=aqcf_bn19;
using b::U; using b::W;
struct Packet { b::Bytes wire; std::vector<U> plaintext; U attempts=0; };
inline W embed(U m,W Q){
    const W d=Q/b::T; const U r=U(Q%b::T);
    return d*m+(W(r)*m+b::T/2)/b::T;
}
inline U locate(W y,W Q){
    const W d=Q/b::T;
    b::require(d>b::T,"186 bounded quotient adjustment premise");
    U m=U(y/d);
    if(m==b::T || embed(m,Q)>y)--m;
    b::require(m<b::T&&embed(m,Q)<=y&&y<embed(m+1,Q),"186 exact scale bin");
    return m;
}
inline Packet generate(b::Lane& lane){
    b::require(bool(lane.sk),"186 client-only mask programming");
    const W Q=lane.codec->Q,d=Q/b::T;
    b::require(d>=(W(1)<<69)&&d<(W(1)<<70),"186 fixed original 70-bit residual");
    for(U attempt=1;attempt<=4;++attempt){
        std::ostringstream out(std::ios::binary);
        lane.enc->encrypt_symmetric(seal::Plaintext("0")).save(out,seal::compr_mode_type::none);
        auto raw=out.str(); lane.codec->validate_seeded(raw);
        std::vector<U> residues(3*b::N),m(b::N);std::vector<W> u(b::N);
        for(U j=0;j<3*b::N;++j)residues[j]=aqcf_seeded_wire::word(raw.data()+b::Codec::prefix+8*j);
        bool accept=true;
        for(U j=0;j<b::N;++j){
            W z=lane.codec->crt(residues.data(),j),y=z?Q-z:0;
            m[j]=locate(y,Q);u[j]=y-embed(m[j],Q);accept=accept&&(u[j]<d);
        }
        if(!accept){OPENSSL_cleanse(residues.data(),residues.size()*sizeof(U));OPENSSL_cleanse(m.data(),m.size()*sizeof(U));OPENSSL_cleanse(u.data(),u.size()*sizeof(W));continue;}
        b::Bits::Writer writer;
        for(W x:u){writer.put(U(x)&((U(1)<<35)-1),35);writer.put(U(x>>35),35);}
        auto packed=writer.finish();
        b::Bytes result(8+b::Codec::prefix+packed.size()+b::Codec::suffix);
        std::memcpy(result.data(),"AQPI186A",8);
        std::memcpy(result.data()+8,raw.data(),b::Codec::prefix);
        std::copy(packed.begin(),packed.end(),result.begin()+8+b::Codec::prefix);
        std::memcpy(result.data()+result.size()-b::Codec::suffix,raw.data()+raw.size()-b::Codec::suffix,b::Codec::suffix);
        OPENSSL_cleanse(residues.data(),residues.size()*sizeof(U));OPENSSL_cleanse(u.data(),u.size()*sizeof(W));
        return {std::move(result),std::move(m),attempt};
    }
    throw std::runtime_error("186 bounded sampler exhausted; no fallback or request retry");
}
inline std::string restore(const b::Lane& lane,const b::Bytes& wire){
    constexpr U length=8+b::Codec::prefix+70*b::N/8+b::Codec::suffix;
    b::require(wire.size()==length&&!std::memcmp(wire.data(),"AQPI186A",8),"186 exact input frame");
    std::string raw(b::Codec::raw_size,'\0');
    std::memcpy(raw.data(),wire.data()+8,b::Codec::prefix);
    std::memcpy(raw.data()+raw.size()-b::Codec::suffix,wire.data()+wire.size()-b::Codec::suffix,b::Codec::suffix);
    b::Bytes packed(wire.begin()+8+b::Codec::prefix,wire.end()-b::Codec::suffix);b::Bits::Reader r{packed};
    for(U j=0;j<b::N;++j){W u=r.get(35);u|=W(r.get(35))<<35;b::require(u<lane.codec->Q/b::T,"186 canonical residual");
        for(U k=0;k<3;++k){U v=U(u%lane.codec->q[k]);if(v)v=lane.codec->q[k]-v;aqcf_seeded_wire::put(raw.data()+b::Codec::prefix+8*(k*b::N+j),v);}}
    r.end();lane.codec->validate_seeded(raw);return raw;
}
}
