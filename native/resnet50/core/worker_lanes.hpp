#ifndef AQCF_CPU_BASE
#define AQCF_CPU_BASE 0
#endif
#pragma once
#include "resnet50/core/sci.hpp"
#include "resnet50/engine/resnet_request_scope.hpp"
#include "resnet50/engine/native_worker_runtime.hpp"
#include <sched.h>
#include <thread>
#include <exception>
#include <time.h>
#include <iostream>
namespace aqcf2 {
// Conventional parallel execution, explicitly NOT a new cryptographic algorithm.
// Each lane owns all its I/O, OT state and randomness. No OT object is shared.
inline void pin_lane(int cpu){cpu_set_t set;CPU_ZERO(&set);CPU_SET(cpu,&set);
 require(sched_setaffinity(0,sizeof(set),&set)==0,"pin worker to its public CPU");}
struct ExtraLane {
 sci::IOPack io;sci::OTPack ot;Sci protocol;
 ExtraLane(int party,int port):io(party,port,"127.0.0.1"),ot(&io,party,true),protocol(party,io,ot){}
};
struct WorkerLanes {
 Sci& primary;int base_cpu;std::vector<std::unique_ptr<ExtraLane>> extra;
 std::vector<U> blocks;std::vector<int> observed_cpu;U maximum_workers=1;
 WorkerLanes(Sci& s,int port,unsigned workers):primary(s),base_cpu(s.party==1?8:0),blocks(workers,0),observed_cpu(workers,-1){
  require(workers==1||workers==4,"one or four actual protocol workers");
  for(unsigned j=1;j<workers;++j){extra.emplace_back(std::make_unique<ExtraLane>(s.party,port+1000*j));
   extra.back()->protocol.binary5=s.binary5;}
 }
 WorkerLanes(Sci& s,std::array<std::unique_ptr<ExtraLane>,4>& owned):primary(s),base_cpu(s.party==1?8:0),blocks(4,0),observed_cpu(4,-1){
  for(unsigned j=1;j<4;++j){require(bool(owned[j]),"four independently initialized OT lanes");extra.push_back(std::move(owned[j]));}}
 unsigned size()const{return 1+extra.size();}
 Sci& at(unsigned i){return i?extra.at(i-1)->protocol:primary;}
 U bytes()const{U n=primary.bytes();for(auto& p:extra)n+=p->protocol.bytes();return n;}
 void flush(){primary.flush();for(auto& p:extra)p->protocol.flush();}
 template<class F> std::vector<U> run(const std::vector<U>& input,F function){return run_impl(input,function,4096);}
 template<class F> std::vector<U> run_frontier(const std::vector<U>& input,F function){
  return run_impl(input,function,AQCF_R50_FRONTIER?65536:4096);
 }
 template<class F> std::vector<U> run_layer229(const std::vector<U>& input,F function){
  require(input.size()>=4&&input.size()<=802816,"229 source-bound full tensor");
  return run_impl(input,function,200704);
 }
 template<class F> std::vector<U> run_impl(const std::vector<U>& input,F function,std::size_t block){
  require(block==4096||block==65536||block==100352||block==200704,"fixed public frontier, no unbounded allocation");
  require(input.size()>=4,"four useful nonlinear rows required; no padding fallback");
  std::vector<U> output(input.size());unsigned count=std::min<std::size_t>(size(),input.size());
  const bool balanced_small=input.size()<block*size();
  std::vector<U> call_rows(count,0);std::vector<long long> call_cpu_ns(count,0);
  auto thread_ns=[](){timespec ts{};require(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts)==0,"lane CPU clock");return ts.tv_sec*1000000000LL+ts.tv_nsec;};
  std::vector<std::exception_ptr> errors(count);std::vector<aqcf_owner4::Task> threads;
  auto work=[&](unsigned lane){try{
   require(at(lane).binary5==primary.binary5,"all lanes use the same public comparison implementation");
   int cpu=aqcf_native4::cpu(lane);pin_lane(cpu);observed_cpu[lane]=sched_getcpu();
   require(observed_cpu[lane]==cpu,"worker CPU evidence");
   auto call_start=thread_ns();
   const std::size_t small_begin=input.size()*lane/count,small_end=input.size()*(lane+1)/count;
   for(std::size_t begin=balanced_small?small_begin:block*lane;begin<(balanced_small?small_end:input.size());begin+=balanced_small?(small_end-small_begin):block*count){
    std::size_t n=balanced_small?small_end-small_begin:std::min<std::size_t>(block,input.size()-begin);
    std::vector<U> part(input.begin()+begin,input.begin()+begin+n);
    scope315::Scope scope(lane,block==200704&&count==4&&input.size()%4==0&&input.size()<=4*block);
    auto result=function(at(lane),part,begin);require(result.size()==n,"lane output size");
    std::copy(result.begin(),result.end(),output.begin()+begin);++blocks[lane];call_rows[lane]+=n;
   }
   call_cpu_ns[lane]=thread_ns()-call_start;
  }catch(...){errors[lane]=std::current_exception();}};
  for(unsigned lane=0;lane<count;++lane)threads.emplace_back(work,lane);
  for(auto& t:threads)t.join();
  maximum_workers=std::max<U>(maximum_workers,count);
  aqcf_native4::restore();
  for(auto& e:errors)if(e)std::rethrow_exception(e);
  U sum=0;std::cout<<"NL_LANE_CALL "<<input.size()<<" "<<count;
  for(unsigned lane=0;lane<count;++lane){require(call_rows[lane]>0&&call_cpu_ns[lane]>0,"every scheduled lane has real work");sum+=call_rows[lane];
    std::cout<<" "<<observed_cpu[lane]<<":"<<call_rows[lane]<<":"<<call_cpu_ns[lane];}
  require(sum==input.size(),"all real elements exactly once");std::cout<<"\n"<<std::flush;
  for(auto& lane:extra)primary.absorb_accounting(lane->protocol);
  return output;
 }
};
}
