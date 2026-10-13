#pragma once
#include "resnet50/engine/dot.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <iostream>
namespace dot500_kernel {
void* create(const seal::SEALContext&,const seal::Ciphertext&);
void destroy(void*) noexcept;
void add(void*,const seal::Ciphertext&,const seal::Plaintext&);
seal::Ciphertext finish(void*);
std::size_t terms(const void*);
}
namespace dot500 {
inline bool enabled(){
 static const bool flag=[](){const char*p=std::getenv("AQCF_DOT500");
  if(!p||(std::strcmp(p,"0")&&std::strcmp(p,"1")))throw std::runtime_error("500 explicit public arm");
  if(p[0]=='1'&&(!__builtin_cpu_supports("avx2")||!__builtin_cpu_supports("fma")))
   throw std::runtime_error("500 requires AVX2 and FMA");
  return p[0]=='1';}();return flag;
}
inline std::atomic<std::uint64_t> outputs{0},products{0};
struct Accumulator {
 const bool on;std::optional<dot28::Accumulator> original;void* candidate=nullptr;
 explicit Accumulator(const seal::SEALContext& c,const seal::Ciphertext& zero):on(enabled()){
  if(on)candidate=dot500_kernel::create(c,zero);else original.emplace(c,zero);
 }
 ~Accumulator(){if(candidate)dot500_kernel::destroy(candidate);}
 Accumulator(const Accumulator&)=delete;Accumulator&operator=(const Accumulator&)=delete;
 void add(const seal::Ciphertext& ct,const seal::Plaintext& pt){if(on)dot500_kernel::add(candidate,ct,pt);else original->add(ct,pt);}
 seal::Ciphertext finish(){
  auto out=on?dot500_kernel::finish(candidate):original->finish();
  products.fetch_add(on?dot500_kernel::terms(candidate):original->terms,std::memory_order_relaxed);
  outputs.fetch_add(1,std::memory_order_relaxed);return out;
 }
};
inline void census(int party){
 auto n=outputs.load(),k=products.load();
 if((party==1&&(n!=1424||k!=337920))||(party==2&&(n||k)))throw std::runtime_error("500 unchanged36 pointwise census");
 std::cout<<"DOT500_CENSUS "<<party<<" "<<enabled()<<" "<<n<<" "<<k<<"\n";
}
}
