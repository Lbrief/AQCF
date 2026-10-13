#pragma once
#include "portable_cpu.hpp"
#include "squeezenet/parallel/pcn_parallel.hpp"
#include "squeezenet/parallel/parallel_span.hpp"
#include "squeezenet/parallel/parallel_context.hpp"
#include <functional>
namespace aqcf_native4 {
inline std::array<int,4> allocation{};
inline void restore(){cpu_set_t a;CPU_ZERO(&a);for(int c:allocation)CPU_SET(c,&a);if(sched_setaffinity(0,sizeof(a),&a))throw std::runtime_error("R50 root allocation");}
inline void initialize(int party){
 const int base=std::getenv("AQCF_COMPUTE4_LOCAL_PUBLIC")?0:(std::getenv("AQCF_COMPUTE4_CLOUD_PUBLIC")?16:(party==1?16:0));
 allocation=aqcf_portable::cpu_allocation({{base,base+2,base+4,base+6}});restore();pcn_ntt085_register();
}
struct Outer {
 unsigned lane;const char* phase;std::size_t jobs=0;unsigned long long began;
 pcn_parallel::OuterGuard guard;
 Outer(unsigned k,const char* name):lane(k),phase(name),began(pcn_parallel::cpu_ns()){}
 ~Outer(){const auto elapsed=pcn_parallel::cpu_ns()-began;
  std::lock_guard<std::mutex> lock(pcn_parallel::scheduler_log);
  std::cerr<<"AQCF_OUTER4 {\"stage\":\""<<phase<<"\",\"lane\":"<<lane<<",\"cpu\":"<<sched_getcpu()<<",\"jobs\":"<<jobs<<",\"cpu_ns\":"<<elapsed<<"}\n";
 }
};
inline int cpu(unsigned lane){return allocation.at(lane);}
}
