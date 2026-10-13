#pragma once
#include "squeezenet/he_backend/parallel_span.hpp"
// One job is one nonempty contiguous range. No replicated computation.
// Root ranges use four pinned workers; existing outer workers inherit their lane.
namespace pcn_context108 {
template<class F> void ranges(const char* label,std::size_t n,F&& f){
 if(pcn_ntt089_in_outer() || n<4){
  if(n<4 && !pcn_ntt089_in_outer())++pcn_span091::short_root_calls;
  f(0,n);return;
 }
 pcn_data089::team(label,[&](pcn_data089::Stage& s){
  s.each(4,[&](std::size_t k){f(n*k/4,n*(k+1)/4);});
 });
}
}
