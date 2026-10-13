#pragma once
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <array>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <openssl/crypto.h>
#include <immintrin.h>
namespace commonA317 {
using B=emp::block;
inline thread_local int slot=-1;
struct alignas(64) OwnerStats {uint64_t values[28]{};};
inline std::array<OwnerStats,4> owners{};
inline thread_local uint64_t* stats=nullptr;
inline bool measured=false;
inline void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
inline bool enabled(){const char*p=std::getenv("AQCF_COMMON317");need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"317 explicit mode");return p[0]=='1';}
inline void bind(int lane){need(lane>=0&&lane<4&&(slot==-1||slot==lane),"317 owner registration");slot=lane;stats=owners[lane].values;}
struct alignas(64) Pack{__m256i lo,hi;};
inline Pack pack(B a,B b,B c,B d){return {_mm256_inserti128_si256(_mm256_castsi128_si256(a),b,1),_mm256_inserti128_si256(_mm256_castsi128_si256(c),d,1)};}
inline void store(const Pack&z,B*a,B*b,B*c,B*d){*a=_mm256_castsi256_si128(z.lo);*b=_mm256_extracti128_si256(z.lo,1);*c=_mm256_castsi256_si128(z.hi);*d=_mm256_extracti128_si256(z.hi,1);}
inline void add(Pack& a,const Pack&b){a.lo=_mm256_xor_si256(a.lo,b.lo);a.hi=_mm256_xor_si256(a.hi,b.hi);}
struct Entry{B*out=nullptr;const B*in=nullptr;int64_t n=0;int k=0,mask=0;};
struct Group{
 std::mutex m;std::condition_variable cv;unsigned arrived=0,epoch=0;bool failed=false;
 std::array<Entry,4> entries{};std::vector<Pack> keys;B seed;
 void poison(){std::lock_guard<std::mutex> lock(m);failed=true;cv.notify_all();}
 void wait(){std::unique_lock<std::mutex> lock(m);need(!failed,"317 poisoned");unsigned e=epoch;
  if(++arrived==4){arrived=0;++epoch;cv.notify_all();}
  else if(!cv.wait_for(lock,std::chrono::seconds(150),[&]{return epoch!=e||failed;})){failed=true;cv.notify_all();}
  need(!failed,"317 failed/timed-out cohort");
 }
};
inline Group group;
inline thread_local bool joint_now=false;
inline void note(int64_t n){if(slot<0)return;need(slot<4,"317 bound owner");++stats[0];stats[1]+=n;}
template<class Seed> void compute(B* out,const B* in,int64_t n,int k,int mask,Seed&& fresh_seed){
 need(slot>=0&&slot<4,"317 no implicit owner");
 try{
  note(n);++stats[5];stats[6]+=n;group.entries[slot]={out,in,n,k,mask};group.wait();
  if(slot==0){
   need((n==10608640&&k==589824)||(n==649728&&k==36288),"317 pinned native sizes");
   for(auto&e:group.entries){need(e.n==n&&e.k==k&&e.mask==mask&&e.out&&e.in,"317 equal public cohorts only");
    uintptr_t a=reinterpret_cast<uintptr_t>(e.out),b=reinterpret_cast<uintptr_t>(e.in);
    need(a+size_t(n)*16<=b||b+size_t(k)*16<=a,"317 distinct native arrays");}
   // Called after all independent MPCOT/input states are fixed and flushed.
   group.seed=fresh_seed();group.keys.resize(k);
  }
  group.wait();
  const int first=int(int64_t(k)*slot/4),last=int(int64_t(k)*(slot+1)/4);
  for(int j=first;j<last;++j)group.keys[j]=pack(group.entries[0].in[j],group.entries[1].in[j],group.entries[2].in[j],group.entries[3].in[j]);
  stats[2]+=last-first;group.wait();
  emp::PRP prp(group.seed);const int64_t groups=(n-1)/4,begin=groups*slot/4,end=groups*(slot+1)/4;stats[3]+=end-begin;
  auto apply=[&](int64_t i,const uint32_t* r,bool tail){
   B* a=group.entries[0].out+i;B* b=group.entries[1].out+i;B* c=group.entries[2].out+i;B*d=group.entries[3].out+i;
   Pack z=pack(*a,*b,*c,*d);
   for(int j=0;j<10;++j){uint32_t x=tail?r[j]%uint32_t(k):(r[j]&uint32_t(mask));if(!tail&&x>=uint32_t(k))x-=k;add(z,group.keys[x]);}
   store(z,a,b,c,d);
  };
  for(int64_t g=begin;g<end;++g){int64_t i=4*g;B tmp[10];for(int m=0;m<10;++m)tmp[m]=emp::makeBlock(i,m);
   emp::AES_ecb_encrypt_blks(tmp,10,&prp.aes);uint32_t r[40];std::memcpy(r,tmp,sizeof r);
   for(int m=0;m<4;++m)apply(i+m,r+10*m,false);
  }
  if(slot==3)for(int64_t i=4*groups;i<n;++i){B tmp[3];for(int m=0;m<3;++m)tmp[m]=emp::makeBlock(i,m);prp.permute_block(tmp,3);uint32_t r[12];std::memcpy(r,tmp,sizeof r);apply(i,r,true);++stats[4];}
  group.wait();
  if(slot==0){OPENSSL_cleanse(group.keys.data(),group.keys.size()*sizeof(Pack));std::vector<Pack>().swap(group.keys);}
  group.entries[slot]={};group.wait();
 }catch(...){group.poison();throw;}
}
}