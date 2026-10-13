#pragma once
// Engineering control, not a new primitive: same full Blake2Xb rejection stream.
// Every invocation creates a fresh generator; buffer surplus is never reused.
#include "resnet50/core/bulk_full_mask.hpp"
#include "resnet50/core/quarter_protocol.hpp"
namespace aqcf3 {
inline void quarter_mask_bulk(seal::Ciphertext&ct,const seal::SEALContext&quarter,
 const std::vector<Query>&queries,std::vector<U>&own){
 const auto dd=quarter.first_context_data();const auto&qs=dd->parms().coeff_modulus();
 require(qs.size()==2&&qs[0].value()==274877153281ULL&&qs[1].value()==274877202433ULL,"fixed complete mask moduli");
 require(ct.coeff_modulus_size()==qs.size()&&ct.size()==2&&!ct.is_ntt_form(),"full coefficient ciphertext");
 std::vector<U>random(2*DEGREE),B(DEGREE);
 bulk_full_mask(seal::Blake2xbPRNGFactory().create(),random);
 for(U r=0;r<2;++r)for(U j=0;j<DEGREE;++j){U&v=ct.data(0)[r*DEGREE+j];v+=random[r*DEGREE+j];if(v>=qs[r].value())v-=qs[r].value();}
 dd->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(random.data(),DEGREE),B.data(),seal::MemoryManager::GetPool());
 for(const auto&q:queries)own[q.destination]=B[q.exponent];
 OPENSSL_cleanse(random.data(),random.size()*sizeof(U));OPENSSL_cleanse(B.data(),B.size()*sizeof(U));
}
}
