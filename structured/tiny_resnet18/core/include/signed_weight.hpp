#pragma once
// Direct representation of the ALREADY admitted signed coefficients in each
// existing RNS limb. This is exact CRT arithmetic, not a new NTT/HE primitive.
// The input is private server data. No transformed weight or model digest is
// sent to the client. All preparation remains inside the original online MAC.
#include <seal/seal.h>
#include <seal/util/ntt.h>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace aqcf_signed_weight {
template<class Coefficients>
std::vector<std::uint64_t> spectrum(const seal::SEALContext& context,
                                   const Coefficients& coefficients) {
    const auto& data=*context.first_context_data();
    const auto& p=data.parms();const auto n=p.poly_modulus_degree();
    const auto& qs=p.coeff_modulus();
    if(n!=8192 || p.plain_modulus().value()!=(std::uint64_t(1)<<41) || qs.size()!=2)
        throw std::runtime_error("signed-weight fixed public parameter domain");
    std::vector<std::uint64_t> result(n*qs.size(),0);
    for(std::size_t r=0;r<qs.size();++r){
        const auto q=qs[r].value();auto* dst=result.data()+r*n;
        if(q<=(std::uint64_t(1)<<30))throw std::runtime_error("signed coefficient bound below prime");
        for(const auto& a:coefficients){
            // Unique coefficient positions are checked by the original parser.
            const auto bits=static_cast<std::uint64_t>(a.value);
            const auto magnitude=a.value<0?std::uint64_t(0)-bits:bits;
            if(a.exponent>=n || magnitude>(std::uint64_t(1)<<30))
                throw std::runtime_error("original coefficient domain exceeded");
            dst[a.exponent]=a.value<0?q-magnitude:magnitude;
        }
        seal::util::ntt_negacyclic_harvey(dst,data.small_ntt_tables()[r]);
    }
    return result;
}
}
