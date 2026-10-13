#pragma once
#include "resnet50/engine/native_worker_runtime.hpp"
#include <condition_variable>
#include <exception>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
namespace pipeline467 {
inline void need(bool x,const char* why){if(!x)throw std::runtime_error(why);}
inline bool enabled(){const char* p=std::getenv("AQCF_PIPE467");need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"467 explicit fixed arm");return p[0]=='1';}
struct Counts{unsigned long long outgoing_calls=0,outgoing_rows=0,incoming_calls=0,incoming_rows=0,transform_calls=0,transform_rows=0;};
inline Counts counts;
// All callbacks use original arrays and original per-lane key objects. Public
// row i retains owner i mod4 and unchanged increasing order on that owner.
// mode0: local produce -> wire; mode1: wire -> local work;
// mode2: wire -> local work -> wire. Window8 is fixed, never tuned.
template<class Receive,class Work,class Send,class Abort>
void execute(unsigned long long n,const char* label,unsigned mode,Receive receive,Work work,Send send,Abort abort){
 need(n>=4&&mode<=2,"467 original four nonempty owners");
 std::mutex lock;std::condition_variable wake;
 std::vector<unsigned char> ready(n,0),done(n,0);
 unsigned long long admitted=0,consumed=0;
 bool cancelled=false;std::exception_ptr failure;
 auto fail=[&](std::exception_ptr e){bool first=false;{std::lock_guard<std::mutex> g(lock);if(!failure){failure=e;first=true;}cancelled=true;}wake.notify_all();if(first)abort();};
 auto check_failure=[&](){std::lock_guard<std::mutex> g(lock);if(failure)std::rethrow_exception(failure);};
 auto owner=[&](unsigned k){try{
  need(aqcf_owner4::current_lane==int(k)&&sched_getcpu()==aqcf_native4::cpu(k),"467 unchanged owner/key affinity");
  aqcf_native4::Outer audit(k,label);
  for(unsigned long long j=k;j<n;j+=4){
   {std::unique_lock<std::mutex> g(lock);wake.wait(g,[&]{return cancelled||(mode==0?j<consumed+8:ready[j]!=0);});if(cancelled)return;}
   work(j,k);++audit.jobs;
   {std::lock_guard<std::mutex> g(lock);done[j]=1;}wake.notify_all();
  }
 }catch(...){fail(std::current_exception());}};
 std::vector<aqcf_owner4::Task> workers;
 try{
  for(unsigned k=0;k<4;++k)workers.emplace_back(k,[&,k]{owner(k);});
  for(unsigned long long j=0;j<n;++j){
   if(mode!=0)while(admitted<n&&admitted<j+8){
    check_failure();receive(admitted);
    {std::lock_guard<std::mutex> g(lock);ready[admitted]=1;}++admitted;wake.notify_all();
   }
   {std::unique_lock<std::mutex> g(lock);wake.wait(g,[&]{return cancelled||done[j]!=0;});if(failure)std::rethrow_exception(failure);}
   if(mode!=1)send(j);
   {std::lock_guard<std::mutex> g(lock);consumed=j+1;}wake.notify_all();
  }
 }catch(...){fail(std::current_exception());}
 for(auto& t:workers)t.join();
 if(failure)std::rethrow_exception(failure);
 need(consumed==n&&(mode==0||admitted==n),"467 complete one-use stream");
 if(mode==0){++counts.outgoing_calls;counts.outgoing_rows+=n;}
 if(mode==1){++counts.incoming_calls;counts.incoming_rows+=n;}
 if(mode==2){++counts.transform_calls;counts.transform_rows+=n;}
}
inline void census(int party){
 const bool on=enabled();const auto& c=counts;
 need(on?(party==2?(c.outgoing_calls==54&&c.outgoing_rows==2132&&!c.incoming_calls&&!c.transform_calls):(c.outgoing_calls==36&&c.outgoing_rows==1424&&c.incoming_calls==36&&c.incoming_rows==1204&&c.transform_calls==18&&c.transform_rows==928)):(!c.outgoing_calls&&!c.incoming_calls&&!c.transform_calls),"467 complete original54 affine stream census");
 std::cout<<"PIPE467_CENSUS "<<party<<" "<<on<<" "<<c.outgoing_calls<<" "<<c.outgoing_rows<<" "<<c.incoming_calls<<" "<<c.incoming_rows<<" "<<c.transform_calls<<" "<<c.transform_rows<<"\n"<<std::flush;
}
}
