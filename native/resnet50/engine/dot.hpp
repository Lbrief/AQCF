#pragma once
#include <seal/seal.h>
#include <seal/util/uintarithsmallmod.h>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace dot28 {
using U=std::uint64_t;using W=unsigned __int128;
struct Accumulator {
 const seal::SEALContext& ctx;seal::Ciphertext base;
 std::vector<W> sums;std::size_t terms=0,N=8192,K=3;
 explicit Accumulator(const seal::SEALContext& c,const seal::Ciphertext& zero):ctx(c),base(zero){
  auto d=ctx.first_context_data();const auto& p=d->parms();
  if(p.poly_modulus_degree()!=N||p.plain_modulus().value()!=(U(1)<<44)||p.coeff_modulus().size()!=K
     ||zero.parms_id()!=d->parms_id()||!zero.is_ntt_form()||zero.size()!=2)
   throw std::runtime_error("dot28 enforced profile");
  for(auto&q:p.coeff_modulus())if(q.bit_count()!=38)throw std::runtime_error("dot28 modulus bound");
  sums.resize(2*K*N);for(std::size_t j=0;j<sums.size();++j)sums[j]=base.data()[j];
 }
 void add(const seal::Ciphertext& ct,const seal::Plaintext& pt){
  if(terms==2048||ct.size()!=2||!ct.is_ntt_form()||ct.parms_id()!=base.parms_id()
     ||pt.parms_id()!=base.parms_id()||pt.coeff_count()!=N*K)
   throw std::runtime_error("dot28 public term or metadata bound");
  for(std::size_t comp=0;comp<2;++comp){
   W* out=sums.data()+comp*K*N;const U* in=ct.data(comp);const U* p=pt.data();
   for(std::size_t j=0;j<K*N;++j)out[j]+=W(in[j])*p[j];
  }++terms;
 }
 seal::Ciphertext finish(){
  auto out=base;auto& qs=ctx.first_context_data()->parms().coeff_modulus();
  for(std::size_t comp=0;comp<2;++comp)for(std::size_t z=0;z<K;++z){
   U* dst=out.data(comp)+z*N;const W* src=sums.data()+(comp*K+z)*N;
   for(std::size_t j=0;j<N;++j){U v[2]={U(src[j]),U(src[j]>>64)};dst[j]=seal::util::barrett_reduce_128(v,qs[z]);}
  }return out;
 }
};
}

