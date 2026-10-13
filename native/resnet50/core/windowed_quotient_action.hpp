#pragma once
// Exact internal action in the ORIGINAL RNS ring. Public quotient fibers and
// an odd-unit permutation expose a bounded-span kernel. Overlap-save, NTT and
// automorphisms are established tools; priority of this arrangement unproven.
// No short-dimensional ciphertext is serialized or decrypted.
#include "resnet50/core/stride_normalized_weight.hpp"
#include "resnet50/core/band_accumulator.hpp"
namespace aqcf3::window_action {
struct Plan {
 const seal::SEALContext& cx;
 U stride,support,b,d,unit,inverse,L,step,blocks,slots;
 std::vector<U> gather;
 std::vector<seal::util::NTTTables> tables;
 explicit Plan(const seal::SEALContext& context,U s,U span):cx(context),stride(s),support(span),b(std::gcd<U>(DEGREE,s)),d(DEGREE/b),unit(s/b),inverse(1),L(2){
  require(s&&span>1&&(span-1)*s<DEGREE&&span<=d,"fixed public compact kernel class");
  while((inverse*unit)%(2*d)!=1)inverse+=2;
  while(L<4*support)L*=2;L=std::min(L,d);step=L-support+1;blocks=(d+step-1)/step;slots=b*blocks*L;
  require(L<d&&step>0&&slots<=2*DEGREE,"windowed action public eligibility");
  auto& qs=cx.first_context_data()->parms().coeff_modulus();require(qs.size()==2,"unchanged two RNS limbs");
  std::array<std::unique_ptr<seal::util::NTTTables>,2> prepared;
  // Compile only PUBLIC layout. Compose inverse automorphism and window
  // gathering once; do not move, inspect or cache any private coefficient.
  gather.resize(slots);
  pcn_data089::each("AQCF_WINDOW_PUBLIC_PREPARE",4,[&](std::size_t job){
   if(job<2){prepared[job]=std::make_unique<seal::util::NTTTables>(__builtin_ctzll(L),qs[job],seal::MemoryPoolHandle::New());return;}
   for(U pos=slots*(job-2)/2;pos<slots*(job-1)/2;++pos){U j=pos%L,tile=(pos/L)%blocks,f=pos/(L*blocks);
    I t=I(tile*step+j)-I(support-1);bool neg=t<0||t>=I(d);if(t<0)t+=d;else if(t>=I(d))t-=d;
    U source=(U(t)*unit)&(2*d-1);neg^=source>=d;gather[pos]=f+b*(source&(d-1))+(neg?DEGREE:0);
   }
  });for(auto& q:prepared)tables.emplace_back(std::move(*q));
 }
 std::vector<U> prepare_coefficients(const std::vector<U>& x)const{
  require(x.size()==4*DEGREE,"both full ciphertext components/RNS limbs");
  std::vector<U> y(4*slots);
  for(unsigned c=0;c<2;++c)for(unsigned r=0;r<2;++r){const U q=tables[r].modulus().value();
   const U* original=x.data()+(c*2+r)*DEGREE;
   for(U f=0;f<b;++f)for(U tile=0;tile<blocks;++tile){U* out=y.data()+((c*2+r)*b*blocks+f*blocks+tile)*L;
    for(U j=0;j<L;++j){U index=gather[(f*blocks+tile)*L+j],v=original[index&(DEGREE-1)];out[j]=index>=DEGREE&&v?q-v:v;}
    seal::util::ntt_negacyclic_harvey(out,tables[r]);
   }
  }return y;
 }
 std::vector<U> weight(const Edge&e)const{
  std::vector<U>w(2*L,0);
  for(unsigned r=0;r<2;++r){U q=tables[r].modulus().value();for(auto&a:e.coefficients){require(a.exponent%stride==0&&a.exponent/stride<support,"fixed support, not secret-selected trimming");U v=magnitude(a.value);require(v<q,"original signed coefficient cap");w[r*L+a.exponent/stride]=a.value<0&&v?q-v:v;}seal::util::ntt_negacyclic_harvey(w.data()+r*L,tables[r]);}
  return w;
 }
 void add(std::vector<__uint128_t>& sums,const std::vector<U>&x,const std::vector<U>&w)const{
  require(sums.size()==4*slots&&x.size()==4*slots&&w.size()==2*L,"complete window accumulator");
  for(unsigned c=0;c<2;++c)for(unsigned r=0;r<2;++r)for(U tile=0;tile<b*blocks;++tile){U off=((c*2+r)*b*blocks+tile)*L;for(U j=0;j<L;++j)sums[off+j]+=__uint128_t(x[off+j])*w[r*L+j];}
 }
 void add(aqcf_band::Accumulator&sums,const std::vector<U>&x,const std::vector<U>&w)const{
  require(sums.size()==4*slots&&x.size()==4*slots&&w.size()==2*L,"complete two-band window action");
  for(U c=0;c<2;++c)for(U r=0;r<2;++r)for(U tile=0;tile<b*blocks;++tile){U off=((c*2+r)*b*blocks+tile)*L;sums.add(off,x.data()+off,w.data()+r*L,L);}
 }
 template<class Accumulator> std::vector<U> finish_coefficients(const Accumulator&sums)const{
  require(sums.size()==4*slots,"all windows present");std::vector<U>out(4*DEGREE),tile_value(L);
  for(unsigned c=0;c<2;++c)for(unsigned r=0;r<2;++r){const U q=tables[r].modulus().value();
   for(U f=0;f<b;++f)for(U tile=0;tile<blocks;++tile){U off=((c*2+r)*b*blocks+f*blocks+tile)*L;
    for(U j=0;j<L;++j)tile_value[j]=U(sums[off+j]%q);seal::util::inverse_ntt_negacyclic_harvey(tile_value.data(),tables[r]);
    for(U j=0;j<std::min(step,d-tile*step);++j){U k=tile*step+j,dest=(k*unit)&(2*d-1),v=tile_value[support-1+j];out[(c*2+r)*DEGREE+f+b*(dest&(d-1))]=dest>=d&&v?q-v:v;}
   }
   // All ORIGINAL coefficients restored. No ciphertext/mask truncation.
  }return out;
 }
 template<class Accumulator> std::vector<U> finish_spectrum(const Accumulator&sums)const{
  auto out=finish_coefficients(sums);
  for(U c=0;c<2;++c)for(U r=0;r<2;++r)
   seal::util::ntt_negacyclic_harvey(out.data()+(c*2+r)*DEGREE,cx.first_context_data()->small_ntt_tables()[r]);
  return out;
 }
};
}
