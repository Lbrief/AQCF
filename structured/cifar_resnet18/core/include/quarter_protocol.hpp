#pragma once
#include "quarter_consumer.hpp"
#include "program.hpp"
#include <seal/seal.h>
#include <seal/util/rns.h>
#include <seal/util/iterator.h>
#include <openssl/crypto.h>
#ifndef AQCF_EXACT_QUARTER_BUILD
#error "Explicit qualified exact-quarter core required"
#endif
static_assert(AQCF_QUARTER_PROFILE==2,"fixed admitted/qualified producer");
namespace aqcf3 {
// Mask every c0 coordinate, although only public query coordinates are sent.
// Full c1 has already received an independent original fresh PK zero.
inline void quarter_mask(seal::Ciphertext& ct,const seal::SEALContext& quarter,
 const std::vector<Query>& queries,std::vector<U>& own){
 const auto dd=quarter.first_context_data();const auto& qs=dd->parms().coeff_modulus();
 require(ct.coeff_modulus_size()==qs.size()&&ct.size()==2&&!ct.is_ntt_form(),"quarter mask shape");
 std::vector<U> random(qs.size()*DEGREE),B(DEGREE);
 auto rng=seal::Blake2xbPRNGFactory().create();
 for(U r=0;r<qs.size();++r){const U q=qs[r].value(),cut=(U(0)-q)%q;
  for(U j=0;j<DEGREE;++j){U v;do{rng->generate(sizeof(v),reinterpret_cast<seal::seal_byte*>(&v));}while(v<cut);
   v%=q;random[r*DEGREE+j]=v;U& z=ct.data(0)[r*DEGREE+j];z+=v;if(z>=q)z-=q;}}
 dd->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(random.data(),DEGREE),B.data(),seal::MemoryManager::GetPool());
 for(auto q:queries)own[q.destination]=B[q.exponent];
 OPENSSL_cleanse(random.data(),random.size()*sizeof(U));OPENSSL_cleanse(B.data(),B.size()*sizeof(U));
}
inline void quarter_recover_all(aqcf2::Sci& s,std::vector<U>& own){
 for(U off=0;off<own.size();off+=4096){U n=std::min<U>(4096,own.size()-off);
  std::vector<U> chunk(own.begin()+off,own.begin()+off+n);auto shares=aqcf_quarter::recover_consumer37(s,chunk);
  std::copy(shares.v.begin(),shares.v.end(),own.begin()+off);OPENSSL_cleanse(chunk.data(),chunk.size()*sizeof(U));}
}
}
