#pragma once
// Public-screen implementation of established spectral/seeded BFV techniques.
// Not an admitted private protocol and not a new cryptographic primitive.
#include <seal/seal.h>
#include <seal/util/rlwe.h>
#include <seal/util/ntt.h>
#include <seal/util/scalingvariant.h>
#include <seal/util/polyarithsmallmod.h>
#include <algorithm>
#include <cstring>
#include <vector>
namespace ingress {
using U=std::uint64_t; using Bytes=std::vector<unsigned char>;
constexpr U N=8192, LIMBS=4, MAGIC=0x3150475346514341ULL;
constexpr std::size_t HEADER=112;
inline void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
inline void profile(const seal::SEALContext&cx){
 auto c=cx.first_context_data();need(c&&cx.parameters_set(),"valid context");
 need(c->parms().scheme()==seal::scheme_type::bfv&&c->parms().poly_modulus_degree()==N&&c->parms().coeff_modulus().size()==LIMBS&&c->parms().plain_modulus().value()==(U(1)<<55),"fixed ingress domain");
 for(auto&q:c->parms().coeff_modulus())need(q.bit_count()==45,"fixed limb width");
 need(cx.key_context_data()->qualifiers().sec_level==seal::sec_level_type::tc128,"actual SEAL tc128");
}
inline std::vector<U> lift(const seal::SEALContext&cx,const seal::Plaintext&m){
 std::vector<U> v(N*LIMBS,0);
 seal::util::multiply_add_plain_with_scaling_variant(m,*cx.first_context_data(),seal::util::RNSIter(v.data(),N));return v;
}
inline void ntt(const seal::SEALContext&cx,U*v,bool inverse=false){
 auto tables=cx.first_context_data()->small_ntt_tables();
 for(U r=0;r<LIMBS;++r){if(inverse)seal::util::inverse_ntt_negacyclic_harvey(v+r*N,tables[r]);else seal::util::ntt_negacyclic_harvey(v+r*N,tables[r]);}
}
struct Randomness {
 seal::prng_seed_type seed{};std::vector<U> a,e;
 Randomness(const seal::SEALContext&cx):a(N*LIMBS),e(N*LIMBS){
  auto bootstrap=cx.first_context_data()->parms().random_generator()->create();
  bootstrap->generate(seal::prng_seed_byte_count,reinterpret_cast<seal::seal_byte*>(seed.data()));
  auto prng=seal::UniformRandomGeneratorFactory::DefaultFactory()->create(seed);
  seal::util::sample_poly_uniform(prng,cx.first_context_data()->parms(),a.data());
  seal::util::SEAL_NOISE_SAMPLER(bootstrap,cx.first_context_data()->parms(),e.data());
 }
 ~Randomness(){seal::util::seal_memzero(seed.data(),sizeof(seed));seal::util::seal_memzero(e.data(),e.size()*sizeof(U));}
};
inline seal::Ciphertext produce(const seal::SEALContext&cx,const seal::SecretKey&sk,const seal::Plaintext&m,const Randomness&r){
 seal::Ciphertext ct;ct.resize(cx,cx.first_parms_id(),2);ct.is_ntt_form()=true;
 auto v=lift(cx,m);auto&qs=cx.first_context_data()->parms().coeff_modulus();
 for(U j=0;j<LIMBS;++j)seal::util::sub_poly_coeffmod(v.data()+j*N,r.e.data()+j*N,N,qs[j],v.data()+j*N);
 ntt(cx,v.data());
 for(U j=0;j<LIMBS;++j){
  seal::util::dyadic_product_coeffmod(r.a.data()+j*N,sk.data().data()+j*N,N,qs[j],ct.data(0)+j*N);
  seal::util::sub_poly_coeffmod(v.data()+j*N,ct.data(0)+j*N,N,qs[j],ct.data(0)+j*N);
 }
 std::copy(r.a.begin(),r.a.end(),ct.data(1));return ct;
}
inline seal::Ciphertext coupled_coefficient_reference(const seal::SEALContext&cx,const seal::SecretKey&sk,const seal::Plaintext&m,const Randomness&r){
 // Independent coefficient-form algebra with the very same A,e; not timed as
 // the native control. Native performance uses Encryptor::encrypt_symmetric.
 seal::Ciphertext ct;ct.resize(cx,cx.first_parms_id(),2);auto&qs=cx.first_context_data()->parms().coeff_modulus();
 for(U j=0;j<LIMBS;++j)seal::util::dyadic_product_coeffmod(r.a.data()+j*N,sk.data().data()+j*N,N,qs[j],ct.data(0)+j*N);
 ntt(cx,ct.data(0),true);std::copy(r.a.begin(),r.a.end(),ct.data(1));ntt(cx,ct.data(1),true);
 for(U j=0;j<LIMBS;++j){
  seal::util::add_poly_coeffmod(ct.data(0)+j*N,r.e.data()+j*N,N,qs[j],ct.data(0)+j*N);
  seal::util::negate_poly_coeffmod(ct.data(0)+j*N,N,qs[j],ct.data(0)+j*N);
 }
 seal::Evaluator ev(cx);ev.add_plain_inplace(ct,m);return ct;
}
inline Bytes serialize(const seal::SEALContext&cx,const seal::Ciphertext&ct,const seal::prng_seed_type&seed,unsigned width){
 need(width==45||width==64,"wire width");need(ct.is_ntt_form()&&ct.size()==2&&ct.parms_id()==cx.first_parms_id(),"spectral input only");
 const U payload=N*LIMBS*width/8;Bytes out(HEADER+payload,0);U w=width;
 std::memcpy(out.data(),&MAGIC,8);std::memcpy(out.data()+8,&w,8);std::memcpy(out.data()+16,ct.parms_id().data(),32);std::memcpy(out.data()+48,seed.data(),64);
 auto&qs=cx.first_context_data()->parms().coeff_modulus();std::size_t at=HEADER;__uint128_t buf=0;unsigned bits=0;
 for(U r=0;r<LIMBS;++r)for(U i=0;i<N;++i){U v=ct.data(0)[r*N+i];need(v<qs[r].value(),"canonical sender coefficient");buf|=__uint128_t(v)<<bits;bits+=width;
  if(bits>=64){U word=U(buf);std::memcpy(out.data()+at,&word,8);at+=8;buf>>=64;bits-=64;}}
 need(bits==0&&at==out.size(),"exact fixed frame");return out;
}
inline seal::Ciphertext consume(const seal::SEALContext&cx,const Bytes&frame,const seal::Plaintext&server_share){
 need(frame.size()>=HEADER,"short header");U tag=0,w=0;std::memcpy(&tag,frame.data(),8);std::memcpy(&w,frame.data()+8,8);
 need(tag==MAGIC&&(w==45||w==64),"version or spectral domain mismatch");
 need(frame.size()==HEADER+N*LIMBS*w/8,"exact length required");need(!std::memcmp(frame.data()+16,cx.first_parms_id().data(),32),"parameter mismatch");
 seal::prng_seed_type seed;std::memcpy(seed.data(),frame.data()+48,64);
 seal::Ciphertext ct;ct.resize(cx,cx.first_parms_id(),2);ct.is_ntt_form()=true;
 auto prng=seal::UniformRandomGeneratorFactory::DefaultFactory()->create(seed);seal::util::sample_poly_uniform(prng,cx.first_context_data()->parms(),ct.data(1));
 auto&qs=cx.first_context_data()->parms().coeff_modulus();__uint128_t buf=0;unsigned bits=0;std::size_t at=HEADER;
 for(U r=0;r<LIMBS;++r)for(U j=0;j<N;++j){if(bits<w){U word;std::memcpy(&word,frame.data()+at,8);at+=8;buf|=__uint128_t(word)<<bits;bits+=64;}
  U v=U(buf&((__uint128_t(1)<<w)-1));buf>>=w;bits-=w;need(v<qs[r].value(),"noncanonical wire coefficient");ct.data(0)[r*N+j]=v;}
 need(bits==0&&buf==0&&at==frame.size(),"frame tail");
 auto add=lift(cx,server_share);ntt(cx,add.data());for(U r=0;r<LIMBS;++r)seal::util::add_poly_coeffmod(ct.data(0)+r*N,add.data()+r*N,N,qs[r],ct.data(0)+r*N);
 return ct;
}
} // namespace ingress
