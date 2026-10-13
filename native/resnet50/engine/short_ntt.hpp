#pragma once
#include <seal/seal.h>
#include <seal/util/ntt.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace short_ntt27 {
using U=std::uint64_t;using I=std::int64_t;
inline void transform(const std::vector<I>& owner,unsigned C,
                      const seal::SEALContext& context,seal::Plaintext& out){
 const auto d=context.first_context_data();const auto& par=d->parms();
 const U N=par.poly_modulus_degree(),T=par.plain_modulus().value();
 if(N!=8192||T!=(U(1)<<44)||C<1||C>128||(C&(C-1))||owner.size()!=C)
  throw std::runtime_error("short NTT public support/profile contract");
 U norm=0;for(I v:owner){if(v<-(I(1)<<30)||v>(I(1)<<30))throw std::runtime_error("short NTT owner cap");norm+=U(v<0?-v:v);}
 if(norm>U(C)*(U(1)<<30))throw std::runtime_error("short NTT derived original column L1 cap");
 out.parms_id()=seal::parms_id_zero;out.resize(N*par.coeff_modulus().size());
 for(unsigned z=0;z<par.coeff_modulus().size();++z){
  const auto& table=d->small_ntt_tables()[z];const auto& mod=table.modulus();
  U q=mod.value(),q2=q*2;U* v=out.data()+z*N;
  for(U i=0;i<C;++i)v[i]=owner[i]>=0?U(owner[i]):q-U(-owner[i]);
  for(U size=C;size<N;size*=2)std::memcpy(v+size,v,size*sizeof(U));
  const auto* roots=table.get_from_root_powers();
  for(U m=N/C,gap=C/2;m<N;m*=2,gap/=2){
   for(U i=0;i<m;++i){const auto& r=roots[m+i];const U at=i*2*gap;
    for(U j=0;j<gap;++j){U x=v[at+j];if(x>=q2)x-=q2;
     U y=seal::util::multiply_uint_mod_lazy(v[at+j+gap],r,mod);
     v[at+j]=x+y;v[at+j+gap]=x+q2-y;
    }
   }
  }
  for(U j=0;j<N;++j){U x=v[j];if(x>=q2)x-=q2;if(x>=q)x-=q;v[j]=x;}
 }
 out.parms_id()=context.first_parms_id();
}
}
