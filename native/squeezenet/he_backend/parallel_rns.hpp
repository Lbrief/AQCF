#pragma once
// Native SEAL RNS algorithms, split only by independent coefficient index.
// All lane scratch uses fresh clearing pools. No modulus/rounding/seed changes.
#include "squeezenet/he_backend/parallel_data.hpp"
#include "seal/util/uintcore.h"
namespace pcn_rns094 {
inline std::atomic<unsigned long long> calls{0},native_inherited{0},short_calls{0};
inline bool split(std::size_t n) {
 if(pcn_ntt089_in_outer()){++native_inherited;return false;}
 if(n<4){++short_calls;return false;}
 return true;
}
template<class F> void array(const char* label,const std::uint64_t* input,
 std::uint64_t* output,std::size_t n,std::size_t in_width,std::size_t out_width,
 bool input_rns,bool output_rns,F&& native) {
 using namespace seal;using namespace seal::util;
 if(!input||!output||!in_width||!out_width||!product_fits_in(n,in_width)||!product_fits_in(n,out_width))
  throw std::invalid_argument("RNS094 bounded array");
 std::array<MemoryPoolHandle,4> pools;
 std::array<Pointer<std::uint64_t>,4> in,out;
 std::array<std::size_t,4> begin{},length{};
 for(unsigned k=0;k<4;++k){
  pools[k]=MemoryPoolHandle::New(true);begin[k]=n*k/4;length[k]=n*(k+1)/4-begin[k];
  in[k]=allocate_uint(length[k]*in_width,pools[k]);out[k]=allocate_uint(length[k]*out_width,pools[k]);
 }
 pcn_data089::team(label,[&](pcn_data089::Stage& s){
  s.each(4,[&](std::size_t k){
   for(std::size_t i=0;i<length[k];++i)for(std::size_t j=0;j<in_width;++j)
    in[k][input_rns?j*length[k]+i:i*in_width+j]=input[input_rns?j*n+begin[k]+i:(begin[k]+i)*in_width+j];
  });
  // OuterGuard is already active; the recursive original method cannot fan out.
  s.each(4,[&](std::size_t k){native(in[k].get(),out[k].get(),length[k],pools[k]);});
  s.each(4,[&](std::size_t k){
   for(std::size_t i=0;i<length[k];++i)for(std::size_t j=0;j<out_width;++j)
    output[output_rns?j*n+begin[k]+i:(begin[k]+i)*out_width+j]=out[k][output_rns?j*length[k]+i:i*out_width+j];
  });
 });
 ++calls;
}
}
