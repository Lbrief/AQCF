#pragma once
// ROOT FOLLOW-UP: execute the unchanged R15 auxiliary polynomial action using
// the same pinned SEAL transform kernel already granted to the native control.
// This is implementation integration, NOT a new NTT or an originality claim.
#include "resnet50/core/kernel_action.hpp"
#include <seal/util/ntt.h>
#include <unordered_map>
namespace aqcf3 {
class NativeAuxWide {
    U n_,q_;
    seal::util::NTTTables tables_;
    std::vector<U> physical_;
    mutable std::vector<U> scratch_;
    static int power(U n) {
        require(n>=2&&n<=DEGREE&&!(n&(n-1)),"native auxiliary degree");
        int p=0;while((U(1)<<p)!=n)++p;return p;
    }
public:
    NativeAuxWide(U n,U q):n_(n),q_(q),tables_(power(n),seal::Modulus(q)),physical_(n),scratch_(n) {
        // Derive a PUBLIC permutation from the transform of X. This binds
        // ordering AND primitive-root choice, rather than guessing bit order.
        std::vector<U> x(n);x[1]=1;
        seal::util::ntt_negacyclic_harvey(x.data(),tables_);
        std::unordered_map<U,U> where;where.reserve(n);
        for(U j=0;j<n;++j)require(where.emplace(x[j],j).second,"distinct native evaluation points");
        U g=2;while(g<4096&&action_pow(g,(q-1)/2,q)!=q-1)++g;
        require(g<4096,"public auxiliary generator");
        const U psi=action_pow(g,(q-1)/(2*n),q),step=U(__uint128_t(psi)*psi%q);
        U point=psi;
        for(U j=0;j<n;++j){auto it=where.find(point);require(it!=where.end(),"same complete evaluation domain");physical_[j]=it->second;point=U(__uint128_t(point)*step%q);}
    }
    U degree()const{return n_;}
    void transform(U* a,U lanes,bool inverse)const {
        require(a&&lanes==1,"auxiliary transform is a single local polynomial");
        // scratch_ is object-local. The HE path is sequential in BOTH worker
        // configurations; no unprotected shared global scratch or new threads.
        if(!inverse){
            std::copy_n(a,n_,scratch_.data());
            seal::util::ntt_negacyclic_harvey(scratch_.data(),tables_);
            for(U j=0;j<n_;++j)a[j]=scratch_[physical_[j]];
        }else{
            for(U j=0;j<n_;++j)scratch_[physical_[j]]=a[j];
            seal::util::inverse_ntt_negacyclic_harvey(scratch_.data(),tables_);
            std::copy_n(scratch_.data(),n_,a);
        }
    }
};
}
