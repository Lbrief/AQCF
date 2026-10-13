#pragma once
#include "resnet50/he_backend/parallel_data.hpp"
namespace pcn_span091 {
inline std::atomic<unsigned long long> short_root_calls{0};
template<class F>void each(const char* label,std::size_t n,F&& f){
 if(n<4 && !pcn_ntt089_in_outer()){++short_root_calls;for(std::size_t i=0;i<n;++i)f(i);return;}
 pcn_data089::each(label,n,std::forward<F>(f));
}
}
