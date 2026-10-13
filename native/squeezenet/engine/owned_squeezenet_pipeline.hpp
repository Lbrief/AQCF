#pragma once
#include "squeezenet/engine/compute_workers.hpp"
namespace aqcf_compute4 {
template<class Result,class MakeOwner,class Prepare,class Consume>
Evidence ordered_owned416(std::size_t n,const std::array<int,4>& cpus,MakeOwner make_owner,Prepare prepare,Consume consume,std::size_t window=8){
 check(n>=4,"four workers require at least four real compute jobs");
 check(window>=4&&window<=64,"bounded producer window");
 for(unsigned a=0;a<4;++a)for(unsigned b=0;b<a;++b)check(cpus[a]!=cpus[b],"four distinct CPU IDs");
 std::vector<std::unique_ptr<Result>> ready(n);
 std::mutex lock;std::condition_variable wake;std::size_t consumed=0;
 bool cancelled=false;std::exception_ptr failure;Evidence evidence;
 auto fail=[&](std::exception_ptr e){std::lock_guard<std::mutex> hold(lock);if(!failure)failure=e;cancelled=true;wake.notify_all();};
 auto work=[&](unsigned lane){try{
   cpu_set_t allowed;CPU_ZERO(&allowed);CPU_SET(cpus[lane],&allowed);
   check(sched_setaffinity(0,sizeof(allowed),&allowed)==0,"compute4 affinity");
   evidence.cpus[lane]=sched_getcpu();check(evidence.cpus[lane]==cpus[lane],"actual worker CPU");
   auto owner=make_owner(lane);
   for(std::size_t index=lane;index<n;index+=4){
     {std::unique_lock<std::mutex> hold(lock);wake.wait(hold,[&]{return cancelled||index<consumed+window;});if(cancelled)return;}
     auto start=thread_ns();auto value=std::make_unique<Result>(prepare(index,lane,*owner));
     evidence.cpu_ns[lane]+=thread_ns()-start;++evidence.items[lane];
     {std::lock_guard<std::mutex> hold(lock);if(cancelled)return;ready[index]=std::move(value);}
     wake.notify_all();
   }
 }catch(...){fail(std::current_exception());}};
 std::vector<std::thread> workers;
 try{for(unsigned lane=0;lane<4;++lane)workers.emplace_back(work,lane);}
 catch(...){fail(std::current_exception());for(auto& t:workers)t.join();std::rethrow_exception(failure);}
 try{
   for(std::size_t index=0;index<n;++index){
     std::unique_ptr<Result> value;
     {std::unique_lock<std::mutex> hold(lock);wake.wait(hold,[&]{return cancelled||bool(ready[index]);});
      if(failure)std::rethrow_exception(failure);value=std::move(ready[index]);}
     consume(index,std::move(*value)); // not delayed until all jobs finish
     {std::lock_guard<std::mutex> hold(lock);consumed=index+1;}wake.notify_all();
   }
 }catch(...){fail(std::current_exception());}
 for(auto& t:workers)t.join();
 if(failure)std::rethrow_exception(failure);
 for(unsigned lane=0;lane<4;++lane)check(evidence.items[lane]>0&&evidence.cpu_ns[lane]>0,"four workers must do real work");
 return evidence;
}
}
