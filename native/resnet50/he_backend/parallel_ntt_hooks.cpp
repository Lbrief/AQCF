// SEAL 4.1.2 ABI-bound scheduling adapter. No parameters, randomness, or packing changes.
#include "resnet50/he_backend/parallel_ntt.hpp"
#include <atomic>
#include <iostream>
namespace {
struct Span { std::uint64_t* data;std::size_t n;std::size_t size() const{return n;}
 std::uint64_t& operator[](std::size_t i){return data[i];} };
std::atomic<unsigned long long> calls{0},outer_calls{0};
thread_local unsigned outer_depth=0;
std::array<int,4> admitted{};
std::mutex admission_lock;
bool registered=false;
const char* stage="unlabelled";
#ifdef PCN_PUBLIC_DIAGNOSTIC084
bool enabled=true;
#else
constexpr bool enabled=true;
#endif
void invoke(seal::util::CoeffIter value,const seal::util::NTTTables& table,bool inverse,bool canonical){
 cpu_set_t a;CPU_ZERO(&a);if(sched_getaffinity(0,sizeof(a),&a))throw std::runtime_error("hook affinity");
 std::array<int,4> cpus{};unsigned count=0;
 for(int i=0;i<CPU_SETSIZE;++i)if(CPU_ISSET(i,&a)){if(count<4)cpus[count]=i;++count;}
 // No affinity widening or silent nested fallback: caller scheduling must be explicit.
 if(count!=4)throw std::runtime_error("NTT production caller must retain exactly four CPUs");
 Span values{value.ptr(),std::size_t(1)<<table.coeff_count_power()};
 const auto result=pcn_ntt4::transform(values,table,cpus,inverse,canonical);
 std::array<unsigned long long,4> jobs{},cpu{},minimum{};minimum.fill(~0ULL);
 for(const auto& s:result.stages)for(unsigned k=0;k<4;++k){
  if(!s[k].jobs||!s[k].cpu_ns||s[k].actual_cpu!=cpus[k])throw std::runtime_error("inactive NTT worker");
  jobs[k]+=s[k].jobs;cpu[k]+=s[k].cpu_ns;minimum[k]=std::min(minimum[k],(unsigned long long)s[k].cpu_ns);
 }
 ++calls;
 std::cerr<<"NTT4 {\"stage\":\""<<stage<<"\",\"N\":"<<values.n<<",\"inverse\":"<<inverse
 <<",\"canonical\":"<<canonical<<",\"stages\":"<<result.stages.size()<<",\"lanes\":[";
 for(unsigned k=0;k<4;++k){if(k)std::cerr<<',';std::cerr<<"{\"cpu\":"<<cpus[k]
 <<",\"socket\":"<<pcn_ntt4::topology_value(cpus[k],"physical_package_id")
 <<",\"core\":"<<pcn_ntt4::topology_value(cpus[k],"core_id")
 <<",\"jobs\":"<<jobs[k]<<",\"cpu_ns\":"<<cpu[k]<<",\"minimum_stage_cpu_ns\":"<<minimum[k]<<'}';}
 std::cerr<<"]}\n";
}
}
extern "C" void pcn_ntt085_register(){
 cpu_set_t set;CPU_ZERO(&set);if(sched_getaffinity(0,sizeof(set),&set)||CPU_COUNT(&set)!=4)throw std::runtime_error("outer parent must have four CPUs");
 std::array<int,4> cpus{};unsigned k=0;for(int i=0;i<CPU_SETSIZE;++i)if(CPU_ISSET(i,&set))cpus[k++]=i;
 pcn_ntt4::admission(cpus);std::lock_guard<std::mutex> lock(admission_lock);
 if(registered && admitted!=cpus)throw std::runtime_error("outer allocation changed");admitted=cpus;registered=true;
}
extern "C" void pcn_ntt085_outer_enter(){
 cpu_set_t set;CPU_ZERO(&set);if(sched_getaffinity(0,sizeof(set),&set)||CPU_COUNT(&set)!=1||outer_depth)throw std::runtime_error("outer nested/affinity");
 std::lock_guard<std::mutex> lock(admission_lock);bool found=false;
 for(int cpu:admitted)if(CPU_ISSET(cpu,&set))found=true;
 if(!registered||!found)throw std::runtime_error("unregistered outer lane");outer_depth=1;
}
extern "C" void pcn_ntt085_outer_leave(){outer_depth=0;}
extern "C" unsigned long long pcn_ntt085_outer_calls(){return outer_calls.load();}
extern "C" void pcn_ntt084_stage(const char* text){stage=text;}
extern "C" unsigned long long pcn_ntt084_calls(){return calls.load();}
#ifdef PCN_PUBLIC_DIAGNOSTIC084
extern "C" void pcn_ntt084_public_mode(bool value){enabled=value;}
#endif
// --wrap targets are bound to the exact original archive's four external symbols.
#define WRAP(NAME,INV,CAN) \
 extern "C" void __real_##NAME(seal::util::CoeffIter,const seal::util::NTTTables&); \
 extern "C" void __wrap_##NAME(seal::util::CoeffIter v,const seal::util::NTTTables& t){ \
 if(outer_depth){++outer_calls;__real_##NAME(v,t);}else if(enabled)invoke(v,t,INV,CAN);else __real_##NAME(v,t);}
WRAP(_ZN4seal4util21ntt_negacyclic_harveyENS0_7PtrIterIPmEERKNS0_9NTTTablesE,false,true)
WRAP(_ZN4seal4util26ntt_negacyclic_harvey_lazyENS0_7PtrIterIPmEERKNS0_9NTTTablesE,false,false)
WRAP(_ZN4seal4util29inverse_ntt_negacyclic_harveyENS0_7PtrIterIPmEERKNS0_9NTTTablesE,true,true)
WRAP(_ZN4seal4util34inverse_ntt_negacyclic_harvey_lazyENS0_7PtrIterIPmEERKNS0_9NTTTablesE,true,false)

extern "C" bool pcn_ntt089_in_outer(){return outer_depth!=0;}
