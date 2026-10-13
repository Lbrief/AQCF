#pragma once
// Exact accumulator/PK-zero modulus-switch fusion. Uses unchanged key/data levels.
// Not a new encryption scheme; no direct-data EncZero/noise-budget substitution.
#include "resnet50/core/program.hpp"
#include <seal/seal.h>
#include <seal/util/rlwe.h>
#include <seal/util/ntt.h>
#include <seal/util/polyarithsmallmod.h>
#include <seal/util/rns.h>
#include <seal/util/iterator.h>
#include <openssl/crypto.h>
namespace aqcf3 {
struct LiftedZeroStats { U fresh_zeros=0,u_forward_ntts=0,inverse_ntts=0,
    avoided_inverse_ntts=0,pk_dyadic_products=0,accumulator_scale_products=0,
    modulus_switch_components=0; };
class ZeroCoins {
public:
    std::vector<U> u,e0,e1;
    explicit ZeroCoins(const seal::SEALContext& context) {
        const auto& parms=context.key_context_data()->parms();
        const auto n=parms.poly_modulus_degree()*parms.coeff_modulus().size();
        u.resize(n);e0.resize(n);e1.resize(n);
        auto rng=parms.random_generator()->create();
        seal::util::sample_poly_ternary(rng,parms,u.data());
        // SEAL_NOISE_SAMPLER is exactly the pinned build's original sampler.
        seal::util::SEAL_NOISE_SAMPLER(rng,parms,e0.data());
        seal::util::SEAL_NOISE_SAMPLER(rng,parms,e1.data());
    }
    ZeroCoins(const ZeroCoins&)=delete;ZeroCoins& operator=(const ZeroCoins&)=delete;
    ~ZeroCoins(){for(auto* p:{&u,&e0,&e1})if(!p->empty())OPENSSL_cleanse(p->data(),p->size()*sizeof(U));}
};
// Consume an ordinary SEAL-ordered data-level NTT accumulator (2*4*N entries).
// Coins must be fresh for every live return. Reusing a coin bank is TEST ONLY,
// for public coupled-coins algebra; the production wrapper creates its own bank.
inline seal::Ciphertext lifted_zero_with_coins(
    const seal::SEALContext& context,const seal::PublicKey& pk,
    const std::vector<U>& data_ntt,const ZeroCoins& coins,LiftedZeroStats& stats) {
    const auto kd=context.key_context_data(),dd=context.first_context_data();
    aqcf2::require(kd&&dd&&kd->parms().coeff_modulus().size()==5&&
      dd->parms().coeff_modulus().size()==4&&kd->parms().poly_modulus_degree()==DEGREE,
      "unchanged five-key/four-data degree8192 context");
    aqcf2::require(data_ntt.size()==8*DEGREE&&coins.u.size()==5*DEGREE&&
      coins.e0.size()==5*DEGREE&&coins.e1.size()==5*DEGREE,"lifted finalizer shape");
    aqcf2::require(seal::is_valid_for(pk,context)&&pk.data().size()==2&&pk.data().is_ntt_form(),
      "genuine original NTT public key");
    const auto& mod=kd->parms().coeff_modulus();const U p=mod[4].value();
    auto u=coins.u;std::vector<U> temp(10*DEGREE);
    for(unsigned r=0;r<5;++r)seal::util::ntt_negacyclic_harvey(u.data()+r*DEGREE,kd->small_ntt_tables()[r]);
    for(unsigned c=0;c<2;++c)for(unsigned r=0;r<5;++r){
        U* dst=temp.data()+(c*5+r)*DEGREE;
        seal::util::dyadic_product_coeffmod(u.data()+r*DEGREE,
            pk.data().data(c)+r*DEGREE,DEGREE,mod[r],dst);
        if(r<4){const U pm=p%mod[r].value(),q=mod[r].value();
            for(U j=0;j<DEGREE;++j){const U a=data_ntt[(c*4+r)*DEGREE+j];
                aqcf2::require(a<q,"canonical ordinary SEAL spectrum");
                const U v=seal::util::multiply_uint_mod(a,pm,mod[r]);
                dst[j]+=v;if(dst[j]>=q)dst[j]-=q;}
        }
        seal::util::inverse_ntt_negacyclic_harvey(dst,kd->small_ntt_tables()[r]);
        const U* error=(c?coins.e1:coins.e0).data()+r*DEGREE;
        for(U j=0;j<DEGREE;++j){dst[j]+=error[j];if(dst[j]>=mod[r].value())dst[j]-=mod[r].value();}
    }
    seal::Ciphertext out;out.resize(context,context.first_parms_id(),2);out.is_ntt_form()=false;
    auto pool=seal::MemoryManager::GetPool();
    for(unsigned c=0;c<2;++c){U* src=temp.data()+c*5*DEGREE;
        kd->rns_tool()->divide_and_round_q_last_inplace(seal::util::RNSIter(src,DEGREE),pool);
        std::copy_n(src,4*DEGREE,out.data(c));}
    OPENSSL_cleanse(u.data(),u.size()*sizeof(U));OPENSSL_cleanse(temp.data(),temp.size()*sizeof(U));
    aqcf2::require(seal::is_valid_for(out,context)&&!out.is_transparent(),"valid fresh coefficient ciphertext");
    ++stats.fresh_zeros;stats.u_forward_ntts+=5;stats.inverse_ntts+=10;
    stats.avoided_inverse_ntts+=8;stats.pk_dyadic_products+=10*DEGREE;
    stats.accumulator_scale_products+=8*DEGREE;stats.modulus_switch_components+=2;
    return out;
}
inline seal::Ciphertext lifted_pk_zero(const seal::SEALContext& context,
    const seal::PublicKey& pk,const std::vector<U>& data_ntt,LiftedZeroStats& stats){
    ZeroCoins fresh(context);return lifted_zero_with_coins(context,pk,data_ntt,fresh,stats);
}
} // namespace aqcf3
