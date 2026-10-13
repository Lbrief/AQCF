#include "squeezenet/core/squeezenet_request_scope.hpp"
#pragma once
#include "squeezenet/engine/worker_lanes.hpp"
#include "squeezenet/core/wave_policy.hpp"
namespace aqcf_wan_wave {
// Every lane keeps its existing I/O, OT state, and CSPRNG. Public contiguous
// partitions preserve element order and do not couple independent samples.
template<class F> std::vector<aqcf2::U> relu_run(
    aqcf2::WorkerLanes& lanes, const std::vector<aqcf2::U>& input, F fn) {
#if !AQCF_WAN_WAVE
    return lanes.run(input, fn);
#else
    using namespace aqcf2;
    require(!input.empty(), "nonempty public ReLU shape");
    require(input.size()>=4 && lanes.size()==4,"four real wave partitions");
 const unsigned count=4;
 std::array<U,4> jobs{},cpu_time{};
    std::vector<U> output(input.size());
    std::vector<std::exception_ptr> errors(count);
    std::vector<std::thread> threads;
    auto work=[&](unsigned lane){try{
 scope317::Scope owned317(lane,input.size()%4==0);
  const auto began=pcn_parallel::cpu_ns();
        auto& s=lanes.at(lane);
        require(s.binary5==lanes.primary.binary5,"same comparison per lane");
        const int cpu=aqcf_native4::cpu(lane);pin_lane(cpu);
        lanes.observed_cpu[lane]=sched_getcpu();
        require(lanes.observed_cpu[lane]==cpu,"actual ReLU lane CPU");
        const auto first=input.size()*lane/count,last=input.size()*(lane+1)/count;
        for(std::size_t begin=first;begin<last;begin+=relu_limit){
            const auto n=std::min<std::size_t>(relu_limit,last-begin);
            std::vector<U> part(input.begin()+begin,input.begin()+begin+n);
            auto value=fn(s,part,begin);
            require(value.size()==n,"complete ReLU wave");
            std::copy(value.begin(),value.end(),output.begin()+begin);
            ++lanes.blocks[lane];jobs[lane]+=n;
        }
    cpu_time[lane]=pcn_parallel::cpu_ns()-began;
 }catch(...){errors[lane]=std::current_exception();}};
    for(unsigned lane=1;lane<count;++lane)threads.emplace_back(work,lane);
    work(0);for(auto& t:threads)t.join();
    lanes.maximum_workers=std::max<U>(lanes.maximum_workers,count);
    aqcf_native4::restore();
 for(auto& e:errors)if(e)std::rethrow_exception(e);
 U done=0;std::cout<<"NL_WAVE4 RELU "<<input.size();
 for(unsigned lane=0;lane<4;++lane){
  require(jobs[lane]>0 && cpu_time[lane]>0,"four useful wave workers");done+=jobs[lane];
  std::cout<<" "<<lanes.observed_cpu[lane]<<":"<<jobs[lane]<<":"<<cpu_time[lane];
 }
 require(done==input.size(),"all wave rows exactly once");std::cout<<"\n"<<std::flush;

    for(auto& lane:lanes.extra)lanes.primary.absorb_accounting(lane->protocol);
    return output;
#endif
}
}
