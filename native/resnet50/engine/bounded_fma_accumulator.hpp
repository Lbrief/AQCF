#pragma once
#include <seal/seal.h>
#include <seal/util/uintarithsmallmod.h>
#include <immintrin.h>
#include <cfenv>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
namespace dot499 {
using U=std::uint64_t;using W=__uint128_t;using S=__int128_t;
inline __m256d convert(__m256i x){
 return _mm256_sub_pd(_mm256_castsi256_pd(_mm256_or_si256(x,_mm256_set1_epi64x(0x4330000000000000LL))),_mm256_set1_pd(0x1p52));
}
struct alignas(32) State{double high[4],low[4];};
struct Accumulator {
 const seal::SEALContext& ctx;seal::Ciphertext base;
 std::vector<State> sums;std::size_t terms=0,N=8192,K=3;
 explicit Accumulator(const seal::SEALContext& c,const seal::Ciphertext& zero):ctx(c),base(zero){
  const auto d=ctx.first_context_data();const auto& p=d->parms();
  if(p.poly_modulus_degree()!=N||p.plain_modulus().value()!=(U(1)<<44)||p.coeff_modulus().size()!=K
   ||zero.parms_id()!=d->parms_id()||!zero.is_ntt_form()||zero.size()!=2)
   throw std::runtime_error("499 unchanged dot28 profile");
  for(auto&q:p.coeff_modulus())if(q.bit_count()!=38)throw std::runtime_error("499 unchanged modulus bound");
  if(std::fegetround()!=FE_TONEAREST||_MM_GET_ROUNDING_MODE()!=_MM_ROUND_NEAREST)
   throw std::runtime_error("499 rounding environment");
  sums.resize(2*K*N/4);
  for(std::size_t j=0;j<sums.size();++j){
   _mm256_store_pd(sums[j].high,_mm256_set1_pd(0x1p76));
   _mm256_store_pd(sums[j].low,convert(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(base.data()+4*j))));
  }
 }
 void add(const seal::Ciphertext& ct,const seal::Plaintext& pt){
  if(terms==2048||ct.size()!=2||!ct.is_ntt_form()||ct.parms_id()!=base.parms_id()
   ||pt.parms_id()!=base.parms_id()||pt.coeff_count()!=N*K)
   throw std::runtime_error("499 unchanged public term/metadata bound");
  for(std::size_t comp=0;comp<2;++comp){
   State* out=sums.data()+comp*K*N/4;const U* in=ct.data(comp);const U* p=pt.data();
   for(std::size_t j=0;j<K*N;j+=4){
    const auto x=convert(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(in+j)));
    const auto w=convert(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p+j)));
    const auto product=_mm256_mul_pd(x,w),tail=_mm256_fmsub_pd(x,w,product);
    const auto high=_mm256_load_pd(out[j/4].high),next=_mm256_add_pd(high,product);
    const auto error=_mm256_sub_pd(product,_mm256_sub_pd(next,high));
    _mm256_store_pd(out[j/4].high,next);
    _mm256_store_pd(out[j/4].low,_mm256_add_pd(_mm256_load_pd(out[j/4].low),_mm256_add_pd(tail,error)));
   }
  }++terms;
 }
 W exact_word(std::size_t j)const{
  const auto& v=sums[j/4];U bits;std::memcpy(&bits,v.high+j%4,8);
  const U mantissa=(bits&((U(1)<<52)-1))|(U(1)<<52);
  const unsigned shift=unsigned((bits>>52)&2047)-1023-52;
  const W high=W(mantissa)<<shift;
  const auto low=static_cast<std::int64_t>(v.low[j%4]);
  return W(S(high)-S(W(1)<<76)+S(low));
 }
 seal::Ciphertext finish(){
  auto out=base;const auto& qs=ctx.first_context_data()->parms().coeff_modulus();
  for(std::size_t comp=0;comp<2;++comp)for(std::size_t z=0;z<K;++z){
   U* dst=out.data(comp)+z*N;
   for(std::size_t j=0;j<N;++j){const W src=exact_word((comp*K+z)*N+j);U v[2]{U(src),U(src>>64)};dst[j]=seal::util::barrett_reduce_128(v,qs[z]);}
  }return out;
 }
};
}
