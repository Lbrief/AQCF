// Existing native backend CONTROL, not an original AQCF primitive.
// This TU must use only its separately pinned native headers. Namespace aliases
// prevent old SCI/EMP inline symbols or OT classes being interposed at link time.
#include "squeezenet/protocol/native_ot_bridge.h"
#ifndef AQCF_NATIVE_STREAM_BATCH
#define AQCF_NATIVE_STREAM_BATCH 0
#endif
#if AQCF_NATIVE_STREAM_BATCH
#include "squeezenet/protocol/stream_batch_io.hpp"
#endif
#ifndef AQCF_NATIVE_BOOL_BATCH
#define AQCF_NATIVE_BOOL_BATCH 0
#endif
#if AQCF_NATIVE_BOOL_BATCH
#include "squeezenet/protocol/bool_batch_io.hpp"
#endif
#include <algorithm>
#include <openssl/crypto.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#define emp aqcf_control_emp
#define sci aqcf_control_sci
#define cheetah aqcf_control_cheetah
#include "squeezenet/protocol/OT/ferret/silent_ot.h"
#undef cheetah
#undef sci
#undef emp
#include <cstdlib>
#include "squeezenet/protocol/correlation_key_slice.hpp"
#include "squeezenet/protocol/correlation_word.hpp"
#ifndef AQCF_FERRET_PERSISTENCE_DISABLED
#error "Refuse native default cache reuse; use the pinned no-persistence materializer"
#endif

namespace {
struct CallbackIO : aqcf_control_emp::IOChannel<CallbackIO> {
  aqcf_silent_io callback;
  uint64_t received=0;
#if AQCF_NATIVE_STREAM_BATCH
  aqcf_native_stream::Writer writer;
#endif
  explicit CallbackIO(aqcf_silent_io c):callback(c) {
    if(!c.context||!c.send||!c.recv||!c.flush)throw std::invalid_argument("I/O callbacks");
  }
  void send_data_internal(const void* p,int n){
    if(n<0)throw std::invalid_argument("negative send");
#if AQCF_NATIVE_STREAM_BATCH
    writer.send(callback,p,n);
#else
    callback.send(callback.context,p,n);
#endif
  }
  void recv_data_internal(void* p,int n){
    if(n<0)throw std::invalid_argument("negative receive");
#if AQCF_NATIVE_STREAM_BATCH
    flush(); // Native receive boundary: drain pending sends, never read ahead.
#endif
    callback.recv(callback.context,p,n);received+=uint64_t(n);
  }
  void flush(){
#if AQCF_NATIVE_STREAM_BATCH
    writer.drain(callback);
#endif
    callback.flush(callback.context);
  }
#if AQCF_NATIVE_BOOL_BATCH
  void send_bool(bool* p,int n){aqcf_bool_batch::send(*this,p,n);}
  void recv_bool(bool* p,int n){aqcf_bool_batch::recv(*this,p,n);}
#endif
};
using NativeOT=aqcf_control_cheetah::SilentOT<CallbackIO>;
using Clock=std::chrono::steady_clock;
void check(bool b,const char* why){if(!b)throw std::invalid_argument(why);}
}

#include "squeezenet/protocol/late_quotient_fields.inc"
#include "squeezenet/protocol/quotient_recovery_fields.inc"
#include "squeezenet/protocol/radix_quotient_fields.inc"
namespace {
bool read_slice207(){const char* p=std::getenv("AQCF_KEY_SLICE207");
 check(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"fixed public slice207 flag");return p[0]=='1';}
}
struct aqcf_silent_handle {
 uint64_t pool426[7]{};
 uint64_t elide249_calls=0,elide249_rows=0,elide249_rm_rows=0,elide249_payload_bytes=0,elide249_omitted_bits=0;
 const bool slice207=read_slice207();
 const bool slice208=[](){const char* p=std::getenv("AQCF_WORD_SLICE208");check(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"fixed public word208 flag");return p[0]=='1';}();
 uint64_t word208_calls=0,word208_rows=0,word208_native_rows=0;
 uint64_t sliced207_radix4_rows=0,sliced207_radix8_rows=0;
 std::map<uint64_t,std::unique_ptr<Quotient197Bank>> quotient197;
 uint64_t quotient197_next=0;
 uint64_t seed221_core_rows=0,seed221_leaf_rows=0;
 uint64_t joint302_prepared_calls=0,joint302_prepared_rows=0,joint302_consumed_calls=0,joint302_consumed_rows=0;
 aqcf_quotient197_stats quotient197_stats{};
 std::map<uint64_t,std::unique_ptr<Quotient169Bank>> quotient169;
 uint64_t quotient169_next=0;
 aqcf_quotient169_stats quotient169_stats{};
 std::map<uint64_t,std::unique_ptr<Late37Bank>> late37;
 uint64_t late37_next=0,late37_calls=0,late37_rows=0,late37_consumed=0;
  int party;
  CallbackIO io[2];
  CallbackIO* ios[2][1];
  std::unique_ptr<NativeOT> ot[2];
  aqcf_silent_stats stats{};
  uint64_t rot_calls=0,rot_rows=0;
  uint64_t joint26_calls=0,joint26_rows=0;
  std::atomic_flag busy=ATOMIC_FLAG_INIT;
  bool poisoned=false;
  std::string error;
  aqcf_silent_handle(int p,aqcf_silent_io f,aqcf_silent_io r):party(p),io{CallbackIO(f),CallbackIO(r)}{
    check(p==1||p==2,"party");ios[0][0]=&io[0];ios[1][0]=&io[1];
  }
  NativeOT& direction(int sender){
    check(sender==1||sender==2,"sender");const int k=sender-1;
    if(!ot[k]){
      const auto t=Clock::now();const auto s=io[k].counter,r=io[k].received;
      // Lazy fresh setup AND native warm-up are inside the measured call.
      // One native worker per owning AQCF lane; no hidden oversubscription.
      ot[k]=std::make_unique<NativeOT>(party==sender?1:2,1,ios[k],false,true,
                                      "AQCF_NO_PERSISTENCE",true);
      io[k].flush();++stats.directions_initialized;
      stats.setup_send_bytes+=io[k].counter-s;stats.setup_recv_bytes+=io[k].received-r;
      stats.setup_seconds+=std::chrono::duration<double>(Clock::now()-t).count();
    }
    return *ot[k];
  }
};
namespace {
template<class F>int invoke(aqcf_silent_handle* h,F f){
  if(!h)return -1;
  if(h->busy.test_and_set())return -2; // forbidden shared owning state
  int rc=0;
  try {check(!h->poisoned,"provider previously failed; retry forbidden");f();
#if AQCF_NATIVE_STREAM_BATCH
    h->io[0].flush();h->io[1].flush(); // No buffered payload crosses into legacy Aux.
#endif
}
  catch(const std::exception& e){h->poisoned=true;h->error=e.what();rc=-1;}
  catch(...){h->poisoned=true;h->error="unknown native error";rc=-1;}
  h->busy.clear();return rc;
}
}
extern "C" aqcf_silent_handle* aqcf_silent_create(int p,aqcf_silent_io f,aqcf_silent_io r){
  try{return new aqcf_silent_handle(p,f,r);}catch(...){return nullptr;}
}
extern "C" void aqcf_silent_destroy(aqcf_silent_handle* h){delete h;}
extern "C" const char* aqcf_silent_error(const aqcf_silent_handle* h){
  return h?h->error.c_str():"provider creation failed";
}
extern "C" int aqcf_silent_read_stats(const aqcf_silent_handle* h,aqcf_silent_stats* s){
  if(!h||!s)return -1;*s=h->stats;
  s->send_bytes=h->io[0].counter+h->io[1].counter;
  s->recv_bytes=h->io[0].received+h->io[1].received;return 0;
}
#include "squeezenet/protocol/correlation_lookup.inc"
extern "C" int aqcf_silent_lookup(aqcf_silent_handle* h,int sender,const uint64_t* table,
 const uint8_t* choice,uint64_t* result,int rows,int ib,int ob){
  return invoke(h,[&]{
    check((sender==1||sender==2)&&rows>0&&rows<=(1<<20)&&ib>=1&&ib<=8&&ob>=1&&ob<=60&&result,"lookup shape");
    const int N=1<<ib;const bool sending=h->party==sender;const uint64_t mask=(uint64_t(1)<<ob)-1;
    check(sending?table!=nullptr:choice!=nullptr,"owned lookup input");
    if(sending){for(size_t j=0;j<size_t(rows)*N;++j)check(table[j]<=mask,"table range");}
    else{for(int j=0;j<rows;++j)check(choice[j]<N,"choice range");}
    auto& ot=h->direction(sender);
    for(int begin=0;begin<rows;begin+=rows){
      const int n=rows-begin;
      if(sending){
        std::vector<uint64_t*> pointers(n);
        for(int j=0;j<n;++j)pointers[j]=const_cast<uint64_t*>(table+size_t(begin+j)*N);
        // Use native prefix-hash chosen 1-of-N, not naive XOR pad composition.
        if(h->slice208&&aqcf_word208::eligible(ib,ob))lookup208_send(ot,table+size_t(begin)*N,n,ib,ob);
        else ot.send_impl(pointers.data(),n,N,ob);
        std::fill_n(result+begin,n,uint64_t(0));
      }else if(h->slice208&&aqcf_word208::eligible(ib,ob))lookup208_recv(ot,result+begin,choice+begin,n,ib,ob);
      else ot.recv_impl(result+begin,choice+begin,n,N,ob);
      h->io[sender-1].flush();
    }
    if(h->slice208&&aqcf_word208::eligible(ib,ob)){++h->word208_calls;h->word208_rows+=rows;h->word208_native_rows+=uint64_t(rows)*ib;}
    ++h->stats.lookup_calls;h->stats.lookup_rows+=rows;
  });
}
extern "C" int aqcf_silent_cot(aqcf_silent_handle* h,int sender,const uint64_t* corr,
 const bool* choice,uint64_t* result,int rows,int width){
  return invoke(h,[&]{
    check((sender==1||sender==2)&&rows>0&&rows<=(1<<20)&&width>=1&&width<=60&&result,"COT shape");
    const bool sending=h->party==sender;const uint64_t mask=(uint64_t(1)<<width)-1;
    check(sending?corr!=nullptr:choice!=nullptr,"owned COT input");
    if(sending)for(int j=0;j<rows;++j)check(corr[j]<=mask,"COT range");
    auto& ot=h->direction(sender);
    for(int b=0;b<rows;b+=rows){const int n=rows-b;
      if(sending)ot.send_cot(result+b,corr+b,n,width);else ot.recv_cot(result+b,choice+b,n,width);
      h->io[sender-1].flush();
    }
    ++h->stats.cot_calls;h->stats.cot_rows+=rows;
  });
}

extern "C" int aqcf_silent_read_rot_stats(const aqcf_silent_handle* h,uint64_t* calls,uint64_t* rows){
 if(!h||!calls||!rows)return -1;*calls=h->rot_calls;*rows=h->rot_rows;return 0;
}
extern "C" int aqcf_silent_rot_bits(aqcf_silent_handle* h,int sender,uint8_t* k0,uint8_t* k1,uint8_t* t,uint8_t* kt,int rows){
 return invoke(h,[&]{
  check((sender==1||sender==2)&&rows>0&&rows<=(1<<20),"ROT shape");
  const bool sending=h->party==sender;
  check(sending?(k0&&k1&&!t&&!kt):(!k0&&!k1&&t&&kt),"owned ROT output");
  auto& ot=h->direction(sender);
  std::vector<uint64_t> a(rows),b(sending?rows:0);
  std::unique_ptr<bool[]> choice(sending?nullptr:new bool[rows]);
  if(sending){
   ot.send_ot_rm_rc<uint64_t>(a.data(),b.data(),rows,1);
   for(int j=0;j<rows;++j){k0[j]=uint8_t(a[j]);k1[j]=uint8_t(b[j]);}
  }else{
   ot.recv_ot_rm_rc<uint64_t>(a.data(),choice.get(),rows,1);
   for(int j=0;j<rows;++j){t[j]=uint8_t(choice[j]);kt[j]=uint8_t(a[j]);}
  }
  OPENSSL_cleanse(a.data(),a.size()*sizeof(uint64_t));
  if(!b.empty())OPENSSL_cleanse(b.data(),b.size()*sizeof(uint64_t));
  if(choice)OPENSSL_cleanse(choice.get(),size_t(rows)*sizeof(bool));
  h->io[sender-1].flush();++h->rot_calls;h->rot_rows+=rows;
 });
}

#include "squeezenet/protocol/late_quotient_bridge.inc"

#include "squeezenet/protocol/quotient_recovery_bridge.inc"
#include "squeezenet/protocol/radix_quotient_bridge.inc"
#include "squeezenet/protocol/seeded_mask_bridge.inc"
#include "squeezenet/protocol/squeezenet_joint_carry_bridge.inc"

extern "C" int aqcf_silent_joint26(aqcf_silent_handle* h,
 uint64_t* k0,uint64_t* k1,bool* choices,uint64_t* selected,int rows){
 return invoke(h,[&]{
  check(rows>0&&rows<=(1<<19),"joint26 public row cap");
  const bool sending=h->party==1;
  check(sending?(k0&&k1&&!choices&&!selected):(!k0&&!k1&&choices&&selected),"joint26 owned fields");
  auto& ot=h->direction(1);
  if(sending)ot.send_ot_rm_rc<uint64_t>(k0,k1,rows,26);
  else ot.recv_ot_rm_rc<uint64_t>(selected,choices,rows,26);
  h->io[0].flush();++h->joint26_calls;h->joint26_rows+=rows;
 });
}
extern "C" int aqcf_silent_joint26_stats(const aqcf_silent_handle* h,uint64_t* calls,uint64_t* rows){
 if(!h||!calls||!rows)return -1;*calls=h->joint26_calls;*rows=h->joint26_rows;return 0;
}

extern "C" int aqcf_slice207_read_stats(const aqcf_silent_handle* h,uint64_t* a,uint64_t* b,int* flag){
 if(!h||!a||!b||!flag||h->poisoned)return -1;
 *a=h->sliced207_radix4_rows;*b=h->sliced207_radix8_rows;*flag=h->slice207;return 0;
}

extern "C" int aqcf_word208_read_stats(const aqcf_silent_handle* h,uint64_t* calls,uint64_t* rows,uint64_t* native,int* flag){
 if(!h||!calls||!rows||!native||!flag||h->poisoned)return -1;
 *calls=h->word208_calls;*rows=h->word208_rows;*native=h->word208_native_rows;*flag=h->slice208;return 0;
}

#include "squeezenet/protocol/elided_component_bridge.inc"

extern "C" int aqcf_common317_scope(int lane,int eligible){try{commonA317::scope_begin(lane,eligible!=0);return 0;}catch(...){return -1;}}
extern "C" int aqcf_common317_end(int abort){try{commonA317::scope_end(abort!=0);return 0;}catch(...){commonA317::poison();return -1;}}
extern "C" int aqcf_common317_reset(){try{commonA317::reset();return 0;}catch(...){return -1;}}
extern "C" int aqcf_common317_read(int lane,uint64_t* p){return commonA317::read_stats(lane,p);}
extern "C" int aqcf_common317_phase_begin(aqcf_silent_handle* h,int n,int kind){return invoke(h,[&]{commonA317::phase_begin(n,kind,h->party,[&](int sender){return h->direction(sender).ferret;});});}
extern "C" int aqcf_common317_phase_end(aqcf_silent_handle* h,int abort){
 if(abort){commonA317::poison();commonA317::in_phase=commonA317::in_raw=commonA317::joint_now=false;commonA317::demand={};return 0;}
 return invoke(h,[&]{commonA317::phase_end(false,[&](int d){return h->ot[d-1]?h->ot[d-1]->ferret:nullptr;});});
}

#include "squeezenet/protocol/squeezenet_duplex_pool_bridge.inc"
