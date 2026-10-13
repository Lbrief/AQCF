#pragma once
#include <seal/seal.h>
#include <seal/util/iterator.h>
#include <seal/util/rns.h>
#include <openssl/crypto.h>
#include <vector>
#include <cstdlib>
#include <string>
#include <stdexcept>
namespace retain296 {
using U=uint64_t;
inline void need(bool p,const char* s){if(!p)throw std::runtime_error(s);}
inline bool enabled(){const char* p=std::getenv("AQCF_RETAINED303");need(p&&(std::string(p)=="0"||std::string(p)=="1"),"303 explicit fixed arm");return p[0]=='1';}
// Pure ownership conversion after the original fresh zero and modulus switch.
// The caller owns the separate evidence counters; this is not an ordinary mask.
template<class Query> void first(seal::Ciphertext& ct,const seal::SEALContext& quarter,
 const std::vector<Query>& query,std::vector<U>& own){
 const auto dd=quarter.last_context_data();const auto& qs=dd->parms().coeff_modulus();
 need(ct.size()==2&&!ct.is_ntt_form()&&ct.poly_modulus_degree()==8192&&ct.coeff_modulus_size()==1&&qs.size()==1&&qs[0].value()==9007199254429697ULL,"296 original SQ returned context");
 need(dd->parms().plain_modulus().value()==(U(1)<<39),"296 quarter Ring39 only");
 const U q=qs[0].value();std::vector<U> r(8192),b(8192);
 for(U j=0;j<8192;++j){const U x=ct.data(0)[j];need(x<q,"296 canonical c0");r[j]=x?q-x:0;ct.data(0)[j]=0;}
 dd->rns_tool()->decrypt_scale_and_round(seal::util::ConstRNSIter(r.data(),8192),b.data(),seal::MemoryManager::GetPool());
 for(const auto& x:query){need(x.exponent<8192&&x.destination<own.size(),"296 public query bound");own[x.destination]=b[x.exponent];}
 OPENSSL_cleanse(r.data(),r.size()*sizeof(U));OPENSSL_cleanse(b.data(),b.size()*sizeof(U));
}
}
