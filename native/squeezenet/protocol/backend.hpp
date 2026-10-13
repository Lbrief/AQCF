#pragma once
#include "squeezenet/protocol/native_ot_bridge.h"
#include "squeezenet/protocol/quotient_recovery.h"
#include "squeezenet/protocol/radix_quotient.h"
#include "squeezenet/protocol/late_quotient_api.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace aqcf_silent_control {
class Provider {
  aqcf_silent_handle* handle;
  int party;
  static void must(bool b,const char* s){if(!b)throw std::runtime_error(s);}
  template<class IO>static aqcf_silent_io callbacks(IO* io){
    return {io,
      [](void* c,const void* p,int n){static_cast<IO*>(c)->send_data(p,n);},
      [](void* c,void* p,int n){static_cast<IO*>(c)->recv_data(p,n);},
      [](void* c){static_cast<IO*>(c)->flush();}};
  }
  void checked(int rc){if(rc)throw std::runtime_error(std::string("SilentOT control abort: ")+aqcf_silent_error(handle));}
public:
  aqcf_silent_handle* owned189_handle(){return handle;}
  template<class IO>Provider(int p,IO* f,IO* r):handle(aqcf_silent_create(p,callbacks(f),callbacks(r))),party(p){must(handle,"SilentOT create");}
  ~Provider(){aqcf_silent_destroy(handle);}
  Provider(const Provider&)=delete;Provider& operator=(const Provider&)=delete;
  aqcf_silent_stats stats()const{aqcf_silent_stats s{};must(aqcf_silent_read_stats(handle,&s)==0,"SilentOT stats");return s;}
  std::vector<uint64_t> lookup(const std::vector<uint64_t>& table,const std::vector<uint64_t>& choice,
                             size_t n,unsigned in,unsigned out,bool compact,int sender=1){
    must(n>0&&n<=size_t(std::numeric_limits<int>::max())&&in>=1&&in<=8&&out>=1&&out<=60,"SilentOT lookup ABI");
    must(sender==1||sender==2,"SilentOT sender");
    const size_t N=size_t(1)<<in,stride=(N*out+63)/64;const uint64_t mask=(uint64_t(1)<<out)-1;
    must(choice.size()==n,"SilentOT choice shape");
    must(n<=(size_t(1)<<20)&&n*N<=(size_t(1)<<25),"whole-call public memory bound");
    if(party==sender)must(table.size()==n*(compact?stride:N),"SilentOT table shape");
    std::vector<uint64_t> result(n);
    // One independent callback, one native invocation. The native code streams
    // packing internally. Never merge dependent carry steps or share OT state.
    for(size_t b=0;b<n;b+=n){const size_t count=n-b;
      std::vector<uint64_t> flat;std::vector<uint8_t> selector;
      if(party==sender){flat.resize(count*N);
        for(size_t j=0;j<count;++j)for(size_t c=0;c<N;++c){
          if(!compact)flat[j*N+c]=table[(b+j)*N+c];
          else{const size_t bit=c*out,word=bit/64;const unsigned shift=bit%64;
            uint64_t v=table[(b+j)*stride+word]>>shift;
            if(shift+out>64)v|=table[(b+j)*stride+word+1]<<(64-shift);
            flat[j*N+c]=v&mask;}
        }
      }else{selector.resize(count);for(size_t j=0;j<count;++j){must(choice[b+j]<N,"SilentOT selector");selector[j]=uint8_t(choice[b+j]);}}
      checked(aqcf_silent_lookup(handle,sender,flat.empty()?nullptr:flat.data(),
              selector.empty()?nullptr:selector.data(),result.data()+b,int(count),int(in),int(out)));
    }
    return result;
  }
  void rot_bits(int sender,uint8_t* k0,uint8_t* k1,uint8_t* t,uint8_t* kt,int n){
    checked(aqcf_silent_rot_bits(handle,sender,party==sender?k0:nullptr,
       party==sender?k1:nullptr,party==sender?nullptr:t,party==sender?nullptr:kt,n));
  }

  uint64_t late37_prepare(int sender,const std::vector<uint8_t>& digit,const std::vector<uint8_t>& carry){
    must(digit.size()==carry.size()&&!digit.empty()&&digit.size()<=262144,"late37 shape");uint64_t id=0;
    checked(aqcf_late37_prepare(handle,sender,party==sender?nullptr:digit.data(),party==sender?nullptr:carry.data(),int(digit.size()),&id));return id;
  }
  std::vector<uint8_t> late37_consume(uint64_t id,const std::vector<uint8_t>& digit,const std::vector<uint8_t>& carry,const std::vector<uint8_t>& mask){
    must(digit.size()==carry.size()&&mask.size()==digit.size(),"late37 consume vectors");std::vector<uint8_t> out(digit.size());
    checked(aqcf_late37_consume(handle,id,digit.data(),carry.data(),mask.data(),out.data(),int(out.size())));return out;
  }
  std::pair<uint64_t,uint64_t> late37_stats()const{uint64_t c=0,r=0,live=0;must(aqcf_late37_stats(handle,&c,&r,&live)==0&&live==0,"late37 unconsumed banks");return {c,r};}

  uint64_t quotient169_prepare(const std::vector<uint8_t>& alpha) {
    must(!alpha.empty()&&alpha.size()<=262144,"quotient169 prepare vectors");
    uint64_t id=0;
    checked(aqcf_quotient169_prepare(handle,1,party==1?alpha.data():nullptr,int(alpha.size()),&id));
    return id;
  }
  std::vector<uint8_t> quotient169_consume(uint64_t id,const std::vector<uint8_t>& digit,
      const std::vector<uint8_t>& carry,const std::vector<uint8_t>& mask) {
    must(!digit.empty()&&digit.size()<=262144&&carry.size()==digit.size()
         &&mask.size()==digit.size(),"quotient169 consume vectors");
    std::vector<uint8_t> out(digit.size());
    checked(aqcf_quotient169_consume(handle,id,digit.data(),carry.data(),
      party==1?mask.data():nullptr,out.data(),int(out.size())));
    return out;
  }
  aqcf_quotient169_stats quotient169_stats()const {
    aqcf_quotient169_stats value{};
    must(aqcf_quotient169_read_stats(handle,&value)==0,"quotient169 incomplete/failed bank lifecycle");
    return value;
  }

  uint64_t quotient197_prepare(const std::vector<uint8_t>& alpha,unsigned bits) {
    must(!alpha.empty()&&alpha.size()<=262144,"quotient197 prepare vectors");
    uint64_t id=0;
    checked(aqcf_quotient197_prepare(handle,1,bits,party==1?alpha.data():nullptr,int(alpha.size()),&id));
    return id;
  }
  std::vector<uint8_t> quotient197_consume(uint64_t id,const std::vector<uint8_t>& digit,
      const std::vector<uint8_t>& carry,const std::vector<uint8_t>& mask) {
    must(!digit.empty()&&digit.size()<=262144&&carry.size()==digit.size()
         &&mask.size()==digit.size(),"quotient197 consume vectors");
    std::vector<uint8_t> out(digit.size());
    checked(aqcf_quotient197_consume(handle,id,digit.data(),carry.data(),
      party==1?mask.data():nullptr,out.data(),int(out.size())));
    return out;
  }
  aqcf_quotient197_stats quotient197_stats()const {
    aqcf_quotient197_stats value{};
    must(aqcf_quotient197_read_stats(handle,&value)==0,"quotient197 incomplete/failed bank lifecycle");
    return value;
  }

  std::pair<uint64_t,uint64_t> rot_stats()const{
    uint64_t c=0,n=0;must(aqcf_silent_read_rot_stats(handle,&c,&n)==0,"ROT stats");return {c,n};
  }
  void cot(int sender,uint64_t* result,const uint64_t* corr,const bool* choices,int n,int w){
    checked(aqcf_silent_cot(handle,sender,party==sender?corr:nullptr,
                          party==sender?nullptr:choices,result,n,w));
  }
};
inline void report(std::ostream& o,const char* epoch,unsigned lane,const Provider& p){
  const auto s=p.stats();const auto rot=p.rot_stats();
  o<<"SILENTOT_RECEIPT {\"epoch\":\""<<epoch<<"\",\"lane\":"<<lane
   <<",\"lookup_calls\":"<<s.lookup_calls<<",\"lookup_rows\":"<<s.lookup_rows
   <<",\"cot_calls\":"<<s.cot_calls<<",\"cot_rows\":"<<s.cot_rows
   <<",\"rot_calls\":"<<rot.first<<",\"rot_rows\":"<<rot.second
   <<",\"sent\":"<<s.send_bytes<<",\"received\":"<<s.recv_bytes
   <<",\"setup_sent\":"<<s.setup_send_bytes<<",\"setup_received\":"<<s.setup_recv_bytes
   <<",\"setup_seconds\":"<<s.setup_seconds<<",\"directions\":"<<s.directions_initialized<<"}\n";
}
}
