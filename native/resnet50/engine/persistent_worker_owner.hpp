#pragma once
// Execution substrate only. One process owns one persistent four-worker team.
// No cryptographic object, random state, model, or request survives in this team.
// This serial-dispatch integration is not yet the cooperative wavefront ON arm.
#include <array>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <sched.h>

namespace aqcf_owner4 {
inline thread_local int current_lane=-1;
inline void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
class Team {
 struct Lane {
  std::mutex lock;std::condition_variable wake;
  std::deque<std::function<void()>> pending;
  bool stop=false;std::thread thread;
 };
 std::array<Lane,4> lanes;
 std::array<int,4> allocation;
public:
 explicit Team(std::array<int,4> cpus):allocation(cpus){
  for(unsigned i=0;i<4;++i){check(cpus[i]>=0&&cpus[i]<CPU_SETSIZE,"owner4 CPU bounds");
   for(unsigned j=0;j<i;++j)check(cpus[i]!=cpus[j],"owner4 four distinct CPUs");}
  std::array<std::future<void>,4> started;
  try {
   for(unsigned k=0;k<4;++k){auto ready=std::make_shared<std::promise<void>>();started[k]=ready->get_future();
    lanes[k].thread=std::thread([this,k,ready]{
     try {cpu_set_t set;CPU_ZERO(&set);CPU_SET(allocation[k],&set);
      check(!sched_setaffinity(0,sizeof(set),&set),"owner4 permanent pin");
      check(sched_getcpu()==allocation[k],"owner4 actual CPU");current_lane=int(k);ready->set_value();
     }catch(...){ready->set_exception(std::current_exception());return;}
     auto& lane=lanes[k];
     for(;;){std::function<void()> work;
      {std::unique_lock<std::mutex> hold(lane.lock);lane.wake.wait(hold,[&]{return lane.stop||!lane.pending.empty();});
       if(lane.pending.empty()&&lane.stop)break;work=std::move(lane.pending.front());lane.pending.pop_front();}
      // packaged_task captures exceptions; never silently drops a failed job.
      work();
     }
     current_lane=-1;
    });
   }
   for(auto& ready:started)ready.get();
  }catch(...){shutdown();throw;}
 }
 Team(const Team&)=delete;Team& operator=(const Team&)=delete;
 ~Team(){shutdown();}
 void shutdown() noexcept {
  for(auto& lane:lanes){{std::lock_guard<std::mutex> hold(lane.lock);lane.stop=true;}lane.wake.notify_all();}
  for(auto& lane:lanes)if(lane.thread.joinable())lane.thread.join();
 }
 template<class F>std::future<void> submit(unsigned k,F&& work){
  check(current_lane<0,"owner4 nested dispatch needs cooperative integration, no fallback pool");
  check(k<4,"owner4 lane range");auto& lane=lanes[k];
  auto task=std::make_shared<std::packaged_task<void()>>(std::forward<F>(work));auto done=task->get_future();
  {std::lock_guard<std::mutex> hold(lane.lock);check(!lane.stop,"owner4 stopped team");
   lane.pending.emplace_back([task]{(*task)();});}
  lane.wake.notify_one();return done;
 }
};
inline std::unique_ptr<Team> instance;
inline void initialize(const std::array<int,4>& cpus){check(!instance,"owner4 initialize once per process");instance=std::make_unique<Team>(cpus);}
class Task {
 std::future<void> done;
public:
 template<class F>Task(unsigned k,F&& work){check(bool(instance),"owner4 explicit team required");done=instance->submit(k,std::forward<F>(work));}
 template<class F>Task(F&& work,unsigned k):Task(k,[fn=std::forward<F>(work),k]()mutable{fn(k);}){}
 Task(Task&&)=default;Task& operator=(Task&&)=default;Task(const Task&)=delete;
 ~Task(){if(done.valid())done.wait();}
 bool joinable()const{return done.valid();}
 void join(){check(done.valid(),"owner4 task consumed once");done.get();}
};
}
