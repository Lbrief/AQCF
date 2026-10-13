#pragma once
#include "resnet50/protocol/carry_correlation_common.hpp"
namespace commonA315 {
inline thread_local bool in_scope=false,scope_allowed=false,in_phase=false,in_raw=false;
inline thread_local int logical_party=0;
inline thread_local std::array<int64_t,2> demand{};
struct PhaseEntry {int64_t left=0,limit=0,D=0;int direction=0;uint64_t ordinal=0;};
inline Group raw_barrier;
inline Group pair_barrier0{2},pair_barrier1{2};
inline Group&phase_barrier364(){return paired364()?(slot%2?pair_barrier1:pair_barrier0):raw_barrier;}
inline std::array<PhaseEntry,4> phase_entries{};
inline void poison(){raw_barrier.poison();pair_barrier0.poison();pair_barrier1.poison();poison_groups364();}
inline void scope_begin(int lane,bool eligible){
 bind(lane);need(!in_scope&&!in_phase&&!in_raw&&!joint_now,"315 no nested caller");
 in_scope=true;scope_allowed=eligible;++stats[eligible?12:13];
}
inline void scope_end(bool abort){
 if(abort){poison();in_phase=in_raw=joint_now=false;demand={};}
 need(in_scope&&!in_phase&&!in_raw&&!joint_now,"315 scope closes outside phase/refill");
 in_scope=scope_allowed=false;
}
template<class Ferret> void prefill(Ferret* f,int64_t D,int direction){
 need(D>0&&D<=10436608&&f->n==10608640&&f->k==589824&&f->queued315()==0,"315 original warmed context, bounded exact phase");
 const int64_t limit=f->ot_limit,left=limit-f->ot_used;
 need(limit==10001853&&left>=0&&left<=limit,"315 original native cursor");
 phase_entries[slot]={left,limit,D,direction,stats[7]};phase_barrier364().wait();
 int64_t count=2;
 for(int owner=0;owner<4;++owner){
  if(paired364()&&owner%2!=slot%2)continue;const auto&e=phase_entries[owner];
  need(e.limit==limit&&e.D==D&&e.direction==direction&&e.ordinal==stats[7],"315 matching public consumer ABI");
  need(e.left>=0&&e.left<=limit,"315 canonical published cursor");
  int64_t need_count=e.D<=e.left?0:(e.D-e.left+limit-1)/limit;
  count=std::min(count,need_count);
 }
 need(count>=0&&count<=2,"315 at most two required native refills");
 stats[20]=std::max(stats[20],uint64_t(count));
 for(int64_t i=0;i<count;++i){
  const auto before=stats[5];joint_now=true;f->prefill315();joint_now=false;
  need(stats[5]==before+1,"315 exactly one original regular generation per prefill");++stats[10];
 }
 phase_barrier364().wait();
}
template<class Get> void phase_begin(size_t n,bool normal,int party,Get&& get){
 need(!in_phase&&!in_raw&&!joint_now,"315 phase no nesting");
 if(!in_scope||!scope_allowed)return;
 try {
  need(slot>=0&&slot<4&&n>0&&n<=200704&&(party==1||party==2),"315 fixed full consumer");
  in_phase=true;logical_party=party;demand={int64_t(52*n),normal?int64_t(n):0};
  ++stats[7];stats[16]+=demand[0];stats[17]+=demand[1];
  if(enabled()){
   prefill(get(1),demand[0],1);
   if(demand[1])prefill(get(2),demand[1],2);
  }
 }catch(...){joint_now=false;poison();throw;}
}
template<class Get> void phase_begin_tree(size_t n,bool normal,int party,Get&& get){
 need(!in_phase&&!in_raw&&!joint_now,"315 phase no nesting");
 if(!in_scope||!scope_allowed)return;
 try {
  need(slot>=0&&slot<4&&n>0&&n<=25088&&(party==1||party==2),"315 fixed full consumer");
  in_phase=true;logical_party=party;demand={int64_t(54*n),normal?int64_t(13*n):int64_t(12*n)};
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
 need(!in_raw&&!joint_now&&demand[0]==0&&demand[1]==0,"315 exact derived demand fully consumed");
 for(int d=1;d<=2;++d){auto* f=get(d);if(f)need(f->queued315()==0,"315 no prefetched material beyond this consumer");}
 ++stats[21];in_phase=false;logical_party=0;
}
template<class Ferret,class Work> void raw(Ferret*,int64_t length,int,int local_party,Work&& work){
 if(slot>=0){++stats[14];stats[15]+=length;}
 if(!in_phase){work();return;}
 try {
  need(in_scope&&scope_allowed&&!in_raw&&!joint_now&&length>0&&(local_party==1||local_party==2),"315 measured original native request");
  const int direction=local_party==1?logical_party:3-logical_party;
  need(direction>=1&&direction<=2&&length<=demand[direction-1],"315 no unbudgeted or extra OT");
  demand[direction-1]-=length;++stats[8];stats[9]+=length;in_raw=true;work();in_raw=false;
 }catch(...){in_raw=false;poison();throw;}
}
inline void reset(int lane){bind(lane);need(!in_scope&&!in_phase&&!in_raw&&!joint_now,"315 no live reset");std::fill(stats,stats+24,0);std::fill(fifo443::stats,fifo443::stats+5,0);}
}
