#pragma once
#include "squeezenet/protocol/squeezenet_correlation_common.hpp"
namespace commonA317 {
inline thread_local bool in_scope=false,scope_allowed=false,in_phase=false,in_raw=false;
inline thread_local int logical_party=0;
inline thread_local std::array<int64_t,2> demand{};
struct PhaseEntry {int64_t left=0,limit=0,D=0;int direction=0;uint64_t ordinal=0;};
inline Group raw_barrier;
inline std::array<PhaseEntry,4> phase_entries{};
inline void poison(){raw_barrier.poison();group.poison();}
inline void scope_begin(int lane,bool eligible){
 bind(lane);need(!in_scope&&!in_phase&&!in_raw&&!joint_now,"317 no nested caller");
 in_scope=true;scope_allowed=eligible&&measured;++stats[eligible?12:13];
}
inline void scope_end(bool abort){
 if(abort){poison();in_phase=in_raw=joint_now=false;demand={};}
 need(in_scope&&!in_phase&&!in_raw&&!joint_now,"317 scope closes outside phase/refill");
 in_scope=scope_allowed=false;slot=-1;stats=nullptr;
}
template<class Ferret> void prefill(Ferret* f,int64_t D,int direction){
 need(D>0&&D<=10436608&&f->n==10608640&&f->k==589824&&f->queued317()==0,"317 original warmed context, bounded exact phase");
 const int64_t limit=f->ot_limit,left=limit-f->ot_used;
 need(limit==10001853&&left>=0&&left<=limit,"317 original native cursor");
 phase_entries[slot]={left,limit,D,direction,stats[7]};raw_barrier.wait();
 int64_t count=2;
 for(const auto& e:phase_entries){
  need(e.limit==limit&&e.D==D&&e.direction==direction&&e.ordinal==stats[7],"317 matching public consumer ABI");
  need(e.left>=0&&e.left<=limit,"317 canonical published cursor");
  int64_t need_count=e.D<=e.left?0:(e.D-e.left+limit-1)/limit;
  count=std::min(count,need_count);
 }
 need(count>=0&&count<=2,"317 at most two required native refills");
 stats[20]=std::max(stats[20],uint64_t(count));
 for(int64_t i=0;i<count;++i){
  const auto before=stats[5];joint_now=true;f->prefill317();joint_now=false;
  need(stats[5]==before+1,"317 exactly one original regular generation per prefill");++stats[10];
 }
 raw_barrier.wait();
}
template<class Get> void phase_begin(size_t n,int kind,int party,Get&& get){
 need(!in_phase&&!in_raw&&!joint_now,"317 phase no nesting");
 if(!in_scope||!scope_allowed)return;
 try {
  need(slot>=0&&slot<4&&n>0&&n<=262144&&(party==1||party==2),"317 fixed full consumer");
  need(kind>=0&&kind<=2,"317 original serial37/quotient25/pool24 phase");
  in_phase=true;logical_party=party;demand={int64_t((kind==2?31:kind==1?51:52)*n),kind==2?0:int64_t(n)};
  ++stats[kind==2?26:24];stats[kind==2?27:25]+=n;
  ++stats[7];stats[16]+=demand[0];stats[17]+=demand[1];
  if(enabled()){
   prefill(get(1),demand[0],1);
   if(demand[1])prefill(get(2),demand[1],2);
  }
 }catch(...){joint_now=false;poison();throw;}
}
template<class Get> void phase_end(bool abort,Get&& get){
 if(!in_phase)return;
 if(abort){poison();in_phase=in_raw=joint_now=false;demand={};return;}
 need(!in_raw&&!joint_now&&demand[0]==0&&demand[1]==0,"317 exact derived demand fully consumed");
 for(int d=1;d<=2;++d){auto* f=get(d);if(f)need(f->queued317()==0,"317 no prefetched material beyond this consumer");}
 ++stats[21];in_phase=false;logical_party=0;
}
template<class Ferret,class Work> void raw(Ferret*,int64_t length,int,int local_party,Work&& work){
 if(slot>=0){++stats[14];stats[15]+=length;}
 if(!in_phase){work();return;}
 try {
  need(in_scope&&scope_allowed&&!in_raw&&!joint_now&&length>0&&(local_party==1||local_party==2),"317 measured original native request");
  const int direction=local_party==1?logical_party:3-logical_party;
  need(direction>=1&&direction<=2&&length<=demand[direction-1],"317 no unbudgeted or extra OT");
  demand[direction-1]-=length;++stats[8];stats[9]+=length;in_raw=true;work();in_raw=false;
 }catch(...){in_raw=false;poison();throw;}
}
inline void reset(){need(slot==-1&&!in_scope&&!in_phase&&!in_raw&&!joint_now&&!measured,"317 single measured graph reset after qualification");
 for(auto& o:owners)std::fill(o.values,o.values+28,0);measured=true;}
inline int read_stats(int lane,uint64_t* out){
 if(!out||lane<0||lane>=4||slot!=-1||in_scope||in_phase||in_raw||!measured)return -1;
 const auto* v=owners[lane].values;
 if(v[7]!=28||v[21]!=28||v[16]!=47357432||v[17]!=608922||v[24]!=16||v[25]!=608922||v[26]!=12||v[27]!=512832||v[11]!=v[19])return -1;
 std::copy(v,v+28,out);return 0;
}
}
