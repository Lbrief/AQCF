#pragma once
#include <array>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <openssl/crypto.h>
namespace fifo443 {
inline thread_local unsigned long long stats[5]{}; // moves, rows, avoided-copy bytes, allocations, retirements
inline void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class B> class Queue {
 struct Slice{std::unique_ptr<B[]> data;size_t begin=0,count=0,used=0;};
 std::array<Slice,2> q;
 std::array<std::unique_ptr<B[]>,2> spare;
 size_t head=0,tail=0,total=0,capacity=0;
 void retire(Slice& s){
  OPENSSL_cleanse(s.data.get()+s.begin,s.count*sizeof(B));
  bool placed=false;for(auto& p:spare)if(!p){p=std::move(s.data);placed=true;break;}
  need(placed,"443 bounded reusable buffer ownership");s.begin=s.count=s.used=0;++stats[4];
 }
public:
 Queue()=default;Queue(const Queue&)=delete;Queue&operator=(const Queue&)=delete;
 ~Queue(){for(auto&s:q)if(s.data)OPENSSL_cleanse(s.data.get(),capacity*sizeof(B));for(auto&p:spare)if(p)OPENSSL_cleanse(p.get(),capacity*sizeof(B));}
 size_t size()const{return total;}
 void preserve(B*&current,size_t offset,size_t count,size_t n){
  need(current&&count&&offset<=n&&count<=n-offset&&head==0&&tail<2,"443 original bounded pending suffix");
  need(!capacity||capacity==n,"443 fixed original native allocation");capacity=n;
  std::unique_ptr<B[]> next;
  for(auto&p:spare)if(p){next=std::move(p);break;}
  if(!next){next.reset(new B[n]);std::memset(next.get(),0,n*sizeof(B));++stats[3];}
  auto&s=q[tail++];need(!s.data,"443 no live allocation replaced");
  s.data.reset(current);s.begin=offset;s.count=count;s.used=0;current=next.release();total+=count;
  ++stats[0];stats[1]+=count;stats[2]+=count*sizeof(B);
 }
 size_t take(B*dst,size_t wanted){
  const size_t initial=wanted;
  while(wanted&&total){auto&s=q[head];need(s.data&&s.used<s.count,"443 live FIFO slice");const size_t n=std::min(wanted,s.count-s.used);
   std::memcpy(dst,s.data.get()+s.begin+s.used,n*sizeof(B));dst+=n;wanted-=n;s.used+=n;total-=n;
   if(s.used==s.count){retire(s);++head;}
   if(!total){need(head==tail,"443 exact closed queue");head=tail=0;}
  }return initial-wanted;
 }
};
}
