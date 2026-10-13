#pragma once
#include "portable_cpu.hpp"
#include "resnet50/parallel/pcn_parallel.hpp"
#include "resnet50/engine/persistent_worker_owner.hpp"
#include "resnet50/parallel/parallel_span.hpp"
#include "resnet50/parallel/parallel_context.hpp"
#include <functional>
namespace aqcf_native4 {
inline std::array<int,4> allocation{};
inline void restore(){cpu_set_t a;CPU_ZERO(&a);for(int c:allocation)CPU_SET(c,&a);if(sched_setaffinity(0,sizeof(a),&a))throw std::runtime_error("R50 root allocation");}
inline void initialize(int party){
 const char* profile=std::getenv("AQCF_EXECUTION_PROFILE170");
 if(!profile || (std::string(profile)!="LOCAL50" && std::string(profile)!="CLOUD_PUBLIC" && std::string(profile)!="WAN_NATIVE"))throw std::runtime_error("trusted170 execution profile required");
 const char* local=std::getenv("AQCF_COMPUTE4_LOCAL_PUBLIC");const bool local_public=local&&std::string(local)=="1";
 if(local_public&&std::string(profile)!="CLOUD_PUBLIC")throw std::runtime_error("293 local public profile only");
 const int base=(local_public||std::string(profile)=="LOCAL50")?0:((std::string(profile)=="CLOUD_PUBLIC" || party==1)?16:0);
 allocation=aqcf_portable::cpu_allocation({{base,base+2,base+4,base+6}});restore();pcn_ntt085_register();aqcf_owner4::initialize(allocation);
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
