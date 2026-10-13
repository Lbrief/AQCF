#pragma once
// Conventional implementation control, NOT an original cryptographic primitive.
// Same SEAL Blake2Xb byte stream, unbiased rejection and ALL original RNS masks.
// Each production call must still create a fresh independent generator.
#include "resnet50/core/program.hpp"
#include <seal/seal.h>
#include <openssl/crypto.h>
namespace aqcf3 {
struct FullMaskStream {
 std::shared_ptr<seal::UniformRandomGenerator> rng;
 std::array<U,2048> buffer{};U at=2048,draws=0;
 explicit FullMaskStream(std::shared_ptr<seal::UniformRandomGenerator> r):rng(std::move(r)){}
 U next(){if(at==buffer.size()){rng->generate(buffer.size()*sizeof(U),reinterpret_cast<seal::seal_byte*>(buffer.data()));at=0;}++draws;return buffer[at++];}
 ~FullMaskStream(){OPENSSL_cleanse(buffer.data(),buffer.size()*sizeof(U));}
};
template<U Q> inline void full_mask_modulus(FullMaskStream& stream,U* out){
 constexpr U cut=(U(0)-Q)%Q;
 for(U j=0;j<DEGREE;++j){U v;do{v=stream.next();}while(v<cut);out[j]=v%Q;}
}
inline void bulk_full_mask(std::shared_ptr<seal::UniformRandomGenerator> rng,std::vector<U>& out){
 require(out.size()==2*DEGREE,"unchanged FULL two-RNS mask extent");FullMaskStream stream(std::move(rng));
 full_mask_modulus<274877153281ULL>(stream,out.data());full_mask_modulus<274877202433ULL>(stream,out.data()+DEGREE);
}
}
