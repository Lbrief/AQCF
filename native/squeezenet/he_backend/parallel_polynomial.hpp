#pragma once
#include "squeezenet/he_backend/parallel_data.hpp"
namespace pcn_poly096 {
inline std::atomic<unsigned long long> calls{0},inherited{0},short_calls{0};
inline bool split(std::size_t n){if(pcn_ntt089_in_outer()){++inherited;return false;}if(n<4){++short_calls;return false;}return true;}
template<class F>void chunks(const char* label,std::size_t n,F&& f){
 pcn_data089::each(label,4,[&](std::size_t k){const auto begin=n*k/4;f(begin,n*(k+1)/4-begin);});++calls;
}
}
