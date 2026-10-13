#pragma once
#include "sci.hpp"
#include <sched.h>
#include <thread>
#include <exception>
#include <cstdlib>
namespace aqcf2 {
// Conventional parallel execution, explicitly NOT a new cryptographic algorithm.
// Each lane owns all its I/O, OT state and randomness. No OT object is shared.
inline int configured_lane_cpu(int cpu){
 const char* name=cpu==0?"AQCF_CPU_SERVER":(cpu==2?"AQCF_CPU_CLIENT":nullptr);
 const char* value=name?std::getenv(name):nullptr;
 if(!value)return cpu;
 std::size_t used=0;int selected=std::stoi(value,&used);
 require(used==std::string(value).size()&&selected>=0&&selected<CPU_SETSIZE,"valid selected CPU");
 return selected;
}
inline void pin_lane(int cpu){cpu_set_t set;CPU_ZERO(&set);CPU_SET(configured_lane_cpu(cpu),&set);
 require(sched_setaffinity(0,sizeof(set),&set)==0,"pin worker to its public CPU");}
struct ExtraLane {
 sci::IOPack io;sci::OTPack ot;Sci protocol;
 ExtraLane(int party,int port):io(party,port,"127.0.0.1"),ot(&io,party,true),protocol(party,io,ot){}
};
struct WorkerLanes {
 Sci& primary;int base_cpu;std::vector<std::unique_ptr<ExtraLane>> extra;
 std::vector<U> blocks;std::vector<int> observed_cpu;U maximum_workers=1;
 WorkerLanes(Sci& s,int port,unsigned workers):primary(s),base_cpu(s.party==1?0:(workers==1?2:4)),blocks(workers,0),observed_cpu(workers,-1){
  require(workers==1||workers==4,"one or four actual protocol workers");
  for(unsigned j=1;j<workers;++j){extra.emplace_back(std::make_unique<ExtraLane>(s.party,port+1000*j));
   extra.back()->protocol.binary5=s.binary5;}
 }
 unsigned size()const{return 1+extra.size();}
 Sci& at(unsigned i){return i?extra.at(i-1)->protocol:primary;}
 U bytes()const{U n=primary.bytes();for(auto& p:extra)n+=p->protocol.bytes();return n;}
 void flush(){primary.flush();for(auto& p:extra)p->protocol.flush();}
 template<class F> std::vector<U> run(const std::vector<U>& input,F function){
  require(!input.empty(),"nonempty chunk input");
  std::vector<U> output(input.size());unsigned count=std::min<std::size_t>(size(),(input.size()+4095)/4096);
  std::vector<std::exception_ptr> errors(count);std::vector<std::thread> threads;
  auto work=[&](unsigned lane){try{
   require(at(lane).binary5==primary.binary5,"all lanes use the same public comparison implementation");
   int cpu=base_cpu+int(lane);pin_lane(cpu);observed_cpu[lane]=sched_getcpu();
   require(observed_cpu[lane]==configured_lane_cpu(cpu),"worker CPU evidence");
   for(std::size_t begin=4096*lane;begin<input.size();begin+=4096*count){
    std::size_t n=std::min<std::size_t>(4096,input.size()-begin);
    std::vector<U> part(input.begin()+begin,input.begin()+begin+n);
    auto result=function(at(lane),part,begin);require(result.size()==n,"lane output size");
    std::copy(result.begin(),result.end(),output.begin()+begin);++blocks[lane];
   }
  }catch(...){errors[lane]=std::current_exception();}};
  for(unsigned lane=1;lane<count;++lane)threads.emplace_back(work,lane);
  work(0);for(auto& t:threads)t.join();
  maximum_workers=std::max<U>(maximum_workers,count);
  for(auto& e:errors)if(e)std::rethrow_exception(e);
  for(auto& lane:extra)primary.absorb_accounting(lane->protocol);
  return output;
 }
};
}
