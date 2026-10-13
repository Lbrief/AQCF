#pragma once
#include "resnet50/core/auxiliary_lift.hpp"
#include "resnet50/core/lifted_pk_zero.hpp"
#include "resnet50/core/prescaled_original_zero.hpp"
#include <seal/seal.h>
#include <seal/util/ntt.h>
namespace aqcf3 {
// The only production coins source is the original SEAL ZeroCoins sampler.
// No auxiliary ciphertext, key, seed, or spectrum is transmitted to the peer.
class NativeAuxiliaryLift {
    const seal::SEALContext& cx_;
    const seal::PublicKey& pk_;
public:
    aux15::Engine engine;
    U original_pk_inverse_ntts=0;
    NativeAuxiliaryLift(const seal::SEALContext& cx,const seal::PublicKey& pk):cx_(cx),pk_(pk){
        const auto kd=cx_.key_context_data(),dd=cx_.first_context_data();
        require(kd&&dd&&kd->parms().poly_modulus_degree()==DEGREE&&kd->parms().coeff_modulus().size()==5&&dd->parms().coeff_modulus().size()==4,"original BFV profile");
        require(kd->qualifiers().sec_level==seal::sec_level_type::tc128,"original tc128");
        for(U r=0;r<5;++r)require(kd->parms().coeff_modulus()[r].value()==aux15::Q[r],"original modulus roster, not an auxiliary BFV context");
        require(seal::is_valid_for(pk_,cx_)&&pk_.data().size()==2&&pk_.data().is_ntt_form(),"original valid PK");
        }
    void prepare_fused_key(){
        if(original_pk_inverse_ntts)return;
        const auto kd=cx_.key_context_data();
        std::vector<U> coefficient_pk(10*DEGREE);
        for(U c=0;c<2;++c)for(U r=0;r<5;++r){U* p=coefficient_pk.data()+(c*5+r)*DEGREE;std::copy_n(pk_.data().data(c)+r*DEGREE,DEGREE,p);seal::util::inverse_ntt_negacyclic_harvey(p,kd->small_ntt_tables()[r]);++original_pk_inverse_ntts;}
        engine.prepare_public_key(coefficient_pk);
    }
    aux15::Spectrum input(const seal::Ciphertext& ct){
        require(seal::is_valid_for(ct,cx_)&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==cx_.first_parms_id(),"original coefficient ciphertext before auxiliary lift");
        std::vector<U> v(8*DEGREE);for(U c=0;c<2;++c)std::copy_n(ct.data(c),4*DEGREE,v.data()+c*4*DEGREE);
        return engine.input(v);
    }
    seal::Ciphertext with_coins(aux15::Spectrum acc,const ZeroCoins& fresh,bool fused){
        require(fresh.u.size()==5*DEGREE&&fresh.e0.size()==5*DEGREE&&fresh.e1.size()==5*DEGREE,"original full coin vectors");
        std::vector<U> out;
        if(fused){
            prepare_fused_key();
            std::vector<I> ternary(DEGREE);
            for(U j=0;j<DEGREE;++j){ternary[j]=aux15::centered(fresh.u[j],aux15::Q[0]);require(ternary[j]>=-1&&ternary[j]<=1,"original ternary bound");
                for(U r=1;r<5;++r)require(fresh.u[r*DEGREE+j]==aux15::mod(ternary[j],aux15::Q[r]),"original shared RNS ternary sampling");}
            out=engine.finish(std::move(acc),ternary,fresh.e0,fresh.e1);
            OPENSSL_cleanse(ternary.data(),ternary.size()*sizeof(I));
        }else{
            out=engine.finish_action_only(std::move(acc));
            // Same auxiliary action, but no proposed terminal fusion. A complete
            // original modulus-down zero is paid, rather than a fake cheap zero.
            auto zero=original_zero_with_coins(cx_,pk_,fresh);
            for(U c=0;c<2;++c)for(U r=0;r<4;++r)for(U j=0;j<DEGREE;++j){U& v=out[(c*4+r)*DEGREE+j];v+=zero.data(c)[r*DEGREE+j];if(v>=aux15::Q[r])v-=aux15::Q[r];}
        }
        seal::Ciphertext ct;ct.resize(cx_,cx_.first_parms_id(),2);ct.is_ntt_form()=false;
        for(U c=0;c<2;++c)std::copy_n(out.data()+c*4*DEGREE,4*DEGREE,ct.data(c));
        require(seal::is_valid_for(ct,cx_)&&!ct.is_transparent(),"unchanged canonical original BFV ciphertext");return ct;
    }
    seal::Ciphertext finish(aux15::Spectrum acc,bool fused){
        ZeroCoins fresh(cx_);return with_coins(std::move(acc),fresh,fused);
    }
};
}
