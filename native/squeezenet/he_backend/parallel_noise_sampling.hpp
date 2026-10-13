#pragma once
// Native sampler execution adaptation, not a new cryptographic primitive.
#include "squeezenet/he_backend/parallel_data.hpp"
#include "seal/randomgen.h"
#include "seal/util/blake2.h"
#include "seal/util/rlwe.h"
#include "seal/util/uintarithsmallmod.h"
namespace pcn_sample095 {
using U=std::uint64_t;
inline std::atomic<U> noise_calls{0},uniform_calls{0},xof_calls{0},rejection_draws{0};
template<class T>struct Sensitive {T value{};~Sensitive(){seal::util::seal_memzero(&value,sizeof(value));}};
inline void le32(void* ptr,std::uint32_t v){auto p=static_cast<unsigned char*>(ptr);for(int k=0;k<4;++k)p[k]=static_cast<unsigned char>(v>>(8*k));}
inline void xof(void* output,std::size_t n,const void* input,std::size_t len,const void* key,std::size_t keylen){
 if(n!=4096||!output)throw std::invalid_argument("native PRNG095 buffer");
 Sensitive<blake2xb_state> state;Sensitive<std::array<unsigned char,64>> root;
 if(blake2xb_init_key(&state.value,n,key,keylen)||blake2xb_update(&state.value,input,len)||blake2b_final(state.value.S,root.value.data(),64))
  throw std::runtime_error("native PRNG095 root");
 pcn_data089::each("native_Blake2xb_output_blocks",n/64,[&](std::size_t i){
  Sensitive<blake2b_param> p;p.value=state.value.P[0];p.value.key_length=0;p.value.fanout=0;p.value.depth=0;
  le32(&p.value.leaf_length,64);p.value.inner_length=64;p.value.node_depth=0;p.value.digest_length=64;le32(&p.value.node_offset,i);
  Sensitive<blake2b_state> c;
  if(blake2b_init_param(&c.value,&p.value)||blake2b_update(&c.value,root.value.data(),64)||blake2b_final(&c.value,static_cast<unsigned char*>(output)+i*64,64))
   throw std::runtime_error("native PRNG095 output block");
 });++xof_calls;
}
// Same original sampler per lane, with independent native PRNG seeds freshly
// drawn from the supplied parent. Not equal to the old single-stream bit order.
template<class F>void noise(std::shared_ptr<seal::UniformRandomGenerator> parent,const seal::EncryptionParameters& parms,U* destination,const char* label,F&& native){
 using namespace seal;using namespace seal::util;
 const auto n=parms.poly_modulus_degree(),width=parms.coeff_modulus().size();
 if(!parent||!destination||n<8||n%4||!width)throw std::invalid_argument("native noise095 geometry");
 Sensitive<std::array<prng_seed_type,4>> seeds;
 // Small sequential seed handoff is counted, no cached states or shared lane PRNG.
 parent->generate(sizeof(seeds.value),reinterpret_cast<seal_byte*>(seeds.value.data()));
 std::array<MemoryPoolHandle,4> pools;std::array<Pointer<U>,4> scratch;
 for(unsigned k=0;k<4;++k){pools[k]=MemoryPoolHandle::New(true);scratch[k]=allocate_uint(n/4*width,pools[k]);}
 pcn_data089::team(label,[&](pcn_data089::Stage& stage){
  stage.each(4,[&](std::size_t k){
   auto child=parms.random_generator()->create(seeds.value[k]);auto chunk=parms;
   // This parameter copy is ONLY a sampler row count, never a smaller HE context.
   chunk.set_poly_modulus_degree(n/4);native(child,chunk,scratch[k].get());
  });
  stage.each(4,[&](std::size_t k){for(std::size_t j=0;j<width;++j)for(std::size_t i=0;i<n/4;++i)destination[j*n+k*(n/4)+i]=scratch[k][j*(n/4)+i];});
 });++noise_calls;
}
inline void uniform(std::shared_ptr<seal::UniformRandomGenerator> prng,const seal::EncryptionParameters& parms,U* dst){
 using namespace seal;using namespace seal::util;
 const auto n=parms.poly_modulus_degree(),m=parms.coeff_modulus().size();
 if(!prng||!dst||!product_fits_in(n,m,sizeof(U)))throw std::invalid_argument("native uniform095 geometry");
 const auto total=n*m;const auto& moduli=parms.coeff_modulus();std::vector<U> limit(m);
 for(std::size_t j=0;j<m;++j)limit[j]=~U(0)-barrett_reduce_64(~U(0),moduli[j])-1;
 prng->generate(total*sizeof(U),reinterpret_cast<seal_byte*>(dst));
 // Preserve original generator consumption order, including every rejection.
 // Initial fill/XOF and coefficient tests/reductions are four-worker work.
 // Ordered exceptional draws are serial and included, not falsely four-worker.
 std::vector<unsigned char> rejected(total);
 pcn_data089::each("native_uniform_rejection_flags",total,[&](std::size_t i){rejected[i]=dst[i]>=limit[i/n];});
 for(std::size_t i=0;i<total;++i)if(rejected[i])while(dst[i]>=limit[i/n]){
  prng->generate(sizeof(U),reinterpret_cast<seal_byte*>(&dst[i]));++rejection_draws;
 }
 pcn_data089::each("native_uniform_reduce",total,[&](std::size_t i){dst[i]=barrett_reduce_64(dst[i],moduli[i/n]);});
 ++uniform_calls;
}
}
