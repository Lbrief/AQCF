#pragma once
// User-authorized matched n1 pair. H=3*2^115, SAME in both paths.
// This is stronger masking for cumulative admission, NOT an innovation.
#include "program.hpp"
#include <seal/seal.h>
namespace aqcf3 {
#if defined(AQCF_R17_REAL_N1) || (defined(AQCF_INGRESS_PUBLIC) || defined(AQCF_EXACT_QUARTER_BUILD))
// New one-image admission: H=15*2^113, stronger than old 12*2^113.
// Old reservations remain in the cumulative proof. This is not an algorithm.
inline constexpr W root_flood_halfwidth() { return W(pairprofile::H_MULT) << pairprofile::H_EXP; }
#else
inline constexpr W root_flood_halfwidth() { return W(3) << 115; }
#endif
inline W root_flood_sample(const std::shared_ptr<seal::UniformRandomGenerator>& rng) {
    __uint128_t u;
    // Uniform rejection from [0,2^124) into [0,2H); no modulo bias.
    do {
        std::array<seal::seal_byte,16> bytes{}; rng->generate(bytes.size(),bytes.data());
        const auto* b=reinterpret_cast<const unsigned char*>(bytes.data());u=0;
        for(unsigned k=0;k<16;++k)u|=__uint128_t(b[k])<<(8*k);
        u&=(__uint128_t(1)<<124)-1;
    } while(u>=__uint128_t(2)*__uint128_t(root_flood_halfwidth()));
    return W(u)-root_flood_halfwidth();
}
}
