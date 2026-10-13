#pragma once
// Conventional parallel preparation of PUBLIC tables only. No secret input,
// randomness, reusable session state, protocol or precision changes.
#include "squeezenet/engine/kernel_action.hpp"
#include "squeezenet/engine/compute_workers.hpp"
#include <seal/seal.h>
#include <seal/util/ntt.h>
#include <memory>
#include <unordered_map>

namespace aqcf3 {
struct PublicTransform4 {
    std::vector<ActionNTT> transforms;
    std::vector<std::vector<U>> broadcast;
};

template<class Record>
PublicTransform4 prepare_public_transform4(U b, U d,
        const seal::SEALContext& context, const std::array<int,4>& cpus,
        Record record) {
    require(b>=2 && b<DEGREE && !(b&(b-1)) && d>=2 && d*b==DEGREE,
            "public table fixed full-degree geometry");
    auto data=context.first_context_data();
    require(bool(data) && data->parms().poly_modulus_degree()==DEGREE &&
            data->parms().coeff_modulus().size()==2,
            "public table exactly two original data limbs");
    const auto& moduli=data->parms().coeff_modulus();
    require(moduli[0].value()==9007199254429697ULL &&
            moduli[1].value()==2199023190017ULL,
            "public table frozen projected-output primes");
    std::array<std::unique_ptr<ActionNTT>,2> transforms;
    std::array<std::unordered_map<U,U>,2> index;
    std::array<std::vector<U>,2> monomial;

    // Four distinct necessary jobs, each executed once: two small-transform
    // preparations and two full SEAL monomial transforms. Do not duplicate
    // either transform merely to occupy a core. No shared mutable evaluator.
    auto prep=aqcf_compute4::ordered<int>(4,cpus,[&](std::size_t job,unsigned lane){
        require(job==lane,"public table unique preparation owner");
        if(job<2) {
            const auto r=job;
            transforms[r]=std::make_unique<ActionNTT>(d,moduli[r].value());
            Edge e;e.coefficients.push_back({b,1});
            auto roots=transforms[r]->weight(e,b);
            for(U k=0;k<d;++k)
                require(index[r].emplace(roots[k],k).second,"distinct quotient roots");
        } else {
            const auto r=job-2;
            monomial[r].resize(DEGREE,0);monomial[r][b]=1;
            // Coefficients 0 and 1 have the same RNS lift for every modulus.
            // Native SEAL NTT ordering is kept, never a lower-degree ciphertext.
            seal::util::ntt_negacyclic_harvey(monomial[r].data(),data->small_ntt_tables()[r]);
        }
        return 0;
    },[](std::size_t,int){});
    record("PUBLIC_TABLE_PREPARE",prep);

    PublicTransform4 out;out.broadcast.resize(2);
    for(auto& row:out.broadcast)row.resize(DEGREE);
    std::array<std::vector<U>,4> counts,offsets;
    for(unsigned lane=0;lane<4;++lane){counts[lane].resize(d);offsets[lane].resize(d);}
    const auto& lookup=index; // concurrent const find, no mutation after join
    auto find=aqcf_compute4::ordered<int>(4,cpus,[&](std::size_t job,unsigned lane){
        require(job==lane,"public table unique lookup owner");
        const U r=lane/2,start=(lane%2)*(DEGREE/2),end=start+DEGREE/2;
        for(U j=start;j<end;++j){
            auto pos=lookup[r].find(monomial[r][j]);
            require(pos!=lookup[r].end(),"SEAL/quotient spectrum matching");
            out.broadcast[r][j]=pos->second;
            ++counts[lane][pos->second];
        }
        return 0;
    },[](std::size_t,int){});
    record("PUBLIC_TABLE_LOOKUP",find);

    // Prefixes preserve the EXACT serial j-order within each root fiber.
    // Even minimum d=2 has four independent (RNS limb,root) prefix jobs.
    auto prefix=aqcf_compute4::ordered<int>(4,cpus,[&](std::size_t job,unsigned lane){
        require(job==lane,"public table unique prefix owner");
        for(U x=(2*d)*job/4;x<(2*d)*(job+1)/4;++x){
            const U r=x/d,k=x%d;const unsigned first=2*r;
            require(counts[first][k]<=b && counts[first+1][k]==b-counts[first][k],
                    "complete grouped spectrum fibers");
            offsets[first][k]=0;offsets[first+1][k]=counts[first][k];
        }
        return 0;
    },[](std::size_t,int){});
    record("PUBLIC_TABLE_PREFIX",prefix);

    auto scatter=aqcf_compute4::ordered<int>(4,cpus,[&](std::size_t job,unsigned lane){
        require(job==lane,"public table unique scatter owner");
        const U r=lane/2,start=(lane%2)*(DEGREE/2),end=start+DEGREE/2;
        for(U j=start;j<end;++j){
            const U k=out.broadcast[r][j],ordinal=offsets[lane][k]++;
            require(ordinal<b,"public spectrum fiber multiplicity");
            out.broadcast[r][j]=k*b+ordinal;
        }
        return 0;
    },[](std::size_t,int){});
    record("PUBLIC_TABLE_SCATTER",scatter);
    out.transforms.reserve(2);
    for(auto& transform:transforms)out.transforms.emplace_back(std::move(*transform));
    return out;
}
} // namespace aqcf3
