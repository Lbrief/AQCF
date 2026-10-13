#include "squeezenet/engine/native_worker_runtime.hpp"
#pragma once
#include "squeezenet/engine/compute_workers.hpp"
namespace aqcf_compute4 {
// Caller owns ordered socket reads; workers own decoding/arithmetic, never IO.
// At most window frames are received but not yet consumed. Failure joins workers;
// the enclosing protocol runner still owns socket timeouts and peer cancellation.
template<class Frame,class Receive,class Work>
Evidence incoming(std::size_t n,const std::array<int,4>& cpus,Receive receive,Work work,std::size_t window=8){
 check(n>=4,"four incoming workers need four real jobs");
 check(window>=4&&window<=64,"bounded incoming window");
 for(unsigned a=0;a<4;++a)for(unsigned b=0;b<a;++b)check(cpus[a]!=cpus[b],"distinct incoming CPUs");
 std::vector<std::unique_ptr<Frame>> ready(n);
 std::mutex lock;std::condition_variable wake;std::size_t pending=0;
 bool cancelled=false;std::exception_ptr failure;Evidence evidence;
 auto fail=[&](std::exception_ptr e){std::lock_guard<std::mutex> hold(lock);if(!failure)failure=e;cancelled=true;wake.notify_all();};
 auto worker=[&](unsigned lane){try{
   cpu_set_t allowed;CPU_ZERO(&allowed);CPU_SET(cpus[lane],&allowed);
   check(sched_setaffinity(0,sizeof(allowed),&allowed)==0,"incoming affinity");
   evidence.cpus[lane]=sched_getcpu();check(evidence.cpus[lane]==cpus[lane],"incoming actual CPU");
   pcn_parallel::OuterGuard native_outer;
   for(std::size_t i=lane;i<n;i+=4){
     std::unique_ptr<Frame> value;
     {std::unique_lock<std::mutex> hold(lock);wake.wait(hold,[&]{return cancelled||bool(ready[i]);});
      if(cancelled)return;value=std::move(ready[i]);}
     auto start=thread_ns();work(i,lane,std::move(*value));
     evidence.cpu_ns[lane]+=thread_ns()-start;++evidence.items[lane];
     {std::lock_guard<std::mutex> hold(lock);--pending;}wake.notify_all();
   }
 }catch(...){fail(std::current_exception());}};
 std::vector<std::thread> workers;
 try{for(unsigned lane=0;lane<4;++lane)workers.emplace_back(worker,lane);}
 catch(...){fail(std::current_exception());for(auto& t:workers)t.join();std::rethrow_exception(failure);}
 try{for(std::size_t i=0;i<n;++i){
   {std::unique_lock<std::mutex> hold(lock);wake.wait(hold,[&]{return cancelled||pending<window;});if(failure)std::rethrow_exception(failure);}
   auto value=std::make_unique<Frame>(receive(i));
   {std::lock_guard<std::mutex> hold(lock);if(failure)std::rethrow_exception(failure);ready[i]=std::move(value);++pending;}
   wake.notify_all();
 }}catch(...){fail(std::current_exception());}
 for(auto& t:workers)t.join();if(failure)std::rethrow_exception(failure);
 for(unsigned lane=0;lane<4;++lane)check(evidence.items[lane]>0&&evidence.cpu_ns[lane]>0,"all four incoming workers must compute");
 check(pending==0,"no unconsumed received frame");return evidence;
}
}
