#pragma once
#include <seal/seal.h>
#include <seal/util/rns.h>
#include <seal/util/iterator.h>
#include <openssl/crypto.h>
#include <stdexcept>
namespace retain298 {
using U=uint64_t;
inline void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
inline std::vector<U> first(seal::Ciphertext& ct,const seal::SEALContext& quarter){
 const auto dd=quarter.first_context_data();const auto& qs=dd->parms().coeff_modulus();const U N=8192;
 need(ct.size()==2&&ct.poly_modulus_degree()==N&&ct.coeff_modulus_size()==qs.size()&&!ct.is_ntt_form(),"298 exact retained RNS shape");
 need(qs.size()==2||qs.size()==3,"298 two admitted R50 return contexts");
 std::vector<U> r(N*qs.size()),out(N);
 for(U z=0;z<qs.size();++z)for(U j=0;j<N;++j){const U v=ct.data(0)[z*N+j],q=qs[z].value();need(v<q,"298 canonical original c0");r[z*N+j]=v?q-v:0;ct.data(0)[z*N+j]=0;}
 dd->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(r.data(),N),out.data(),seal::MemoryManager::GetPool());
 OPENSSL_cleanse(r.data(),r.size()*sizeof(U));return out;
}
}
