#pragma once
#include "resnet50/engine/persistent_worker_owner.hpp"
// Scheduling only. Each stage owns disjoint outputs; barriers preserve dependencies.
#include "resnet50/parallel/pcn_parallel.hpp"
#include "resnet50/parallel/parallel_ntt.hpp"
extern "C" bool pcn_ntt089_in_outer();
namespace pcn_data089 {
using U=std::uint64_t;
inline std::atomic<U> teams{0},stages{0},inherited{0};
inline std::mutex log_lock;
struct Stage {
 unsigned lane;pcn_ntt4::Barrier& barrier;pcn_ntt4::Lane total{};U minimum=~U(0),count=0;
 template<class F> void each(std::size_t n,F&& f){
  auto begin=n*lane/4,end=n*(lane+1)/4;
  pcn_ntt4::require(begin<end,"data089 empty work lane");
  auto start=pcn_ntt4::thread_ns();
  for(std::size_t i=begin;i<end;++i)f(i);
  auto elapsed=pcn_ntt4::thread_ns()-start;
  pcn_ntt4::require(elapsed>0,"data089 zero thread CPU");
  total.jobs+=end-begin;total.cpu_ns+=elapsed;minimum=std::min(minimum,elapsed);++count;
  barrier.wait();
 }
};
template<class F> void team(const char* label,F&& body){
 pcn_ntt4::require(!pcn_ntt089_in_outer(),"nested data089 team");
 const auto allowed=pcn_parallel::cpus();pcn_ntt4::require(allowed.size()==4,"data089 exactly four CPUs");
 std::array<int,4> cpus{allowed[0],allowed[1],allowed[2],allowed[3]};
 pcn_ntt4::admission(cpus);pcn_ntt085_register();
 pcn_ntt4::Barrier barrier;std::exception_ptr error;std::mutex error_lock;
 std::array<pcn_ntt4::Lane,4> totals{};std::array<U,4> minimum{},counts{};
 std::vector<aqcf_owner4::Task> workers;
 auto run=[&](unsigned k){try{
  cpu_set_t a;CPU_ZERO(&a);CPU_SET(cpus[k],&a);
  pcn_ntt4::require(!pthread_setaffinity_np(pthread_self(),sizeof(a),&a),"data089 pin");
  pcn_parallel::OuterGuard guard;Stage stage{k,barrier};barrier.wait();body(stage);
  stage.total.actual_cpu=sched_getcpu();
  pcn_ntt4::require(stage.count&&stage.total.actual_cpu==cpus[k],"data089 actual work/core");
  totals[k]=stage.total;minimum[k]=stage.minimum;counts[k]=stage.count;
 }catch(...){std::lock_guard<std::mutex> lock(error_lock);if(!error)error=std::current_exception();barrier.abort();}};
 try{for(unsigned k=0;k<4;++k)workers.emplace_back(run,k);}catch(...){
  {std::lock_guard<std::mutex> lock(error_lock);if(!error)error=std::current_exception();}barrier.abort();}
 for(auto& t:workers)t.join();if(error)std::rethrow_exception(error);
 for(unsigned k=0;k<4;++k)pcn_ntt4::require(counts[k]==counts[0],"data089 stage agreement");
 ++teams;stages+=counts[0];std::lock_guard<std::mutex> lock(log_lock);
 std::cerr<<"DATA089 {\"label\":\""<<label<<"\",\"stages\":"<<counts[0]<<",\"lanes\":[";
 for(unsigned k=0;k<4;++k){if(k)std::cerr<<',';std::cerr<<"{\"cpu\":"<<cpus[k]
  <<",\"jobs\":"<<totals[k].jobs<<",\"cpu_ns\":"<<totals[k].cpu_ns<<",\"minimum_stage_cpu_ns\":"<<minimum[k]<<'}';}
 std::cerr<<"]}\n";
}
template<class F>void each(const char* label,std::size_t n,F&& f){
 if(pcn_ntt089_in_outer()){++inherited;for(std::size_t i=0;i<n;++i)f(i);return;}
 // Fewer than four scalar metadata items cannot demonstrate four useful lanes.
 pcn_ntt4::require(n>=4,"root data089 span below four; explicit scalar classification required");
 team(label,[&](Stage& s){s.each(n,f);});
}
template<class Roots,class Reverse>void cyclic(U* output,const U* input,std::size_t n,
 U p,const Roots& roots,const Reverse& reverse,bool inverse,U inv_n,int inject=-1){
 pcn_ntt4::require(n>=8 && !(n&(n-1)),"data089 cyclic degree");
 // Preserve alias-safe source; the copy itself is real four-worker work.
 std::vector<U> copy(output==input?n:0);const U* src=output==input?copy.data():input;
 team(inverse?"cyclic_inverse":"cyclic_forward",[&](Stage& s){
  if(output==input)s.each(n,[&](std::size_t i){copy[i]=input[i];});
  s.each(n,[&](std::size_t i){output[reverse[i]]=src[i];});
  if(inject==int(s.lane))throw std::runtime_error("public data089 injected lane failure");
  for(std::size_t m=2;m<=n;m*=2){const auto half=m/2,step=n/m;
   s.each(n/2,[&](std::size_t q){const auto k=(q/half)*m,j=q%half;
    const U w=roots[j*step],u=output[k+j],t=U((__uint128_t(w)*output[k+j+half])%p);
    output[k+j]=(u+t)%p;output[k+j+half]=(u+p-t)%p;
   });
  }
  if(inverse)s.each(n,[&](std::size_t i){output[i]=U((__uint128_t(output[i])*inv_n)%p);});
 });
}
}
