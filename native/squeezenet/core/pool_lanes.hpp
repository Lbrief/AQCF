#include "squeezenet/core/squeezenet_request_scope.hpp"
#pragma once
#include "squeezenet/engine/worker_lanes.hpp"
#include "squeezenet/core/pool_wave.hpp"

namespace aqcf_pool_wave {
// Partition by public length across the original four owning lanes, then
// aggregate each lane's independent values into waves of at most 16384.
// The previous wave scheduler could unnecessarily use only one lane for a
// 16384-value pool. This preserves parallelism without sharing OT state.
template<class F> std::vector<U> run(aqcf2::WorkerLanes& lanes,
 const std::vector<U>& input,F function){
 require(!input.empty(),"balanced pool nonempty");
 require(input.size()>=4 && lanes.size()==4,"four real wave partitions");
 const unsigned count=4;
 std::array<U,4> jobs{},cpu_time{};
 std::vector<U> output(input.size());
 std::vector<std::exception_ptr> errors(count);std::vector<std::thread> threads;
 auto work=[&](unsigned lane){try{
 scope317::Scope owned317(lane,input.size()%4==0);
  const auto began=pcn_parallel::cpu_ns();
  auto& s=lanes.at(lane);require(s.binary5==lanes.primary.binary5,"same public comparison");
  const int cpu=aqcf_native4::cpu(lane);aqcf2::pin_lane(cpu);
  lanes.observed_cpu[lane]=sched_getcpu();
  require(lanes.observed_cpu[lane]==cpu,"balanced pool affinity");
  const auto first=input.size()*lane/count,last=input.size()*(lane+1)/count;
  for(std::size_t begin=first;begin<last;begin+=limit){
   const auto n=std::min<std::size_t>(limit,last-begin);
   std::vector<U> part(input.begin()+begin,input.begin()+begin+n);
   auto result=function(s,part,begin);require(result.size()==n,"complete pool output");
   std::copy(result.begin(),result.end(),output.begin()+begin);++lanes.blocks[lane];jobs[lane]+=n;
  }
 cpu_time[lane]=pcn_parallel::cpu_ns()-began;
 }catch(...){errors[lane]=std::current_exception();}};
 for(unsigned i=1;i<count;++i)threads.emplace_back(work,i);work(0);
 for(auto& t:threads)t.join();lanes.maximum_workers=std::max<U>(lanes.maximum_workers,count);
 aqcf_native4::restore();
 for(auto& e:errors)if(e)std::rethrow_exception(e);
 U done=0;std::cout<<"NL_WAVE4 POOL "<<input.size();
 for(unsigned lane=0;lane<4;++lane){
  require(jobs[lane]>0 && cpu_time[lane]>0,"four useful wave workers");done+=jobs[lane];
  std::cout<<" "<<lanes.observed_cpu[lane]<<":"<<jobs[lane]<<":"<<cpu_time[lane];
 }
 require(done==input.size(),"all wave rows exactly once");std::cout<<"\n"<<std::flush;

 for(auto& lane:lanes.extra)lanes.primary.absorb_accounting(lane->protocol);
 return output;
}
}
