#pragma once
#include "resnet50/protocol/owned_correlation_fifo.hpp"
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
namespace commonA315 {
using B=emp::block;
inline thread_local int slot=-1;
inline thread_local uint64_t stats[24]{};
inline void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
inline bool enabled(){const char*p=std::getenv("AQCF_COMMON315");need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"315 explicit mode");return p[0]=='1';}
inline void bind(int lane){need(lane>=0&&lane<4&&(slot==-1||slot==lane),"315 owner registration");slot=lane;}
struct alignas(64) Pack{__m256i lo,hi;};
inline Pack pack(B a,B b,B c,B d){return {_mm256_inserti128_si256(_mm256_castsi128_si256(a),b,1),_mm256_inserti128_si256(_mm256_castsi128_si256(c),d,1)};}
inline void store(const Pack&z,B*a,B*b,B*c,B*d){*a=_mm256_castsi256_si128(z.lo);*b=_mm256_extracti128_si256(z.lo,1);*c=_mm256_castsi256_si128(z.hi);*d=_mm256_extracti128_si256(z.hi,1);}
inline void add(Pack& a,const Pack&b){a.lo=_mm256_xor_si256(a.lo,b.lo);a.hi=_mm256_xor_si256(a.hi,b.hi);}
struct Entry{B*out=nullptr;const B*in=nullptr;int64_t n=0;int k=0,mask=0;};
struct Group{
 std::mutex m;std::condition_variable cv;unsigned arrived=0,epoch=0;bool failed=false;
 const unsigned required;explicit Group(unsigned size=4):required(size){}
 std::array<Entry,4> entries{};std::vector<Pack> keys;B seed;
 void poison(){std::lock_guard<std::mutex> lock(m);failed=true;cv.notify_all();}
 void wait(){std::unique_lock<std::mutex> lock(m);need(!failed,"315 poisoned");unsigned e=epoch;
  if(++arrived==required){arrived=0;++epoch;cv.notify_all();}
  else if(!cv.wait_for(lock,std::chrono::seconds(150),[&]{return epoch!=e||failed;})){failed=true;cv.notify_all();}
  need(!failed,"315 failed/timed-out cohort");
 }
};
inline Group group;
inline thread_local bool joint_now=false;
inline void note(int64_t n){if(slot<0)return;need(slot<4,"315 bound owner");++stats[0];stats[1]+=n;}

inline bool paired364(){const char*p=std::getenv("AQCF_COHORT364");need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"364 explicit fixed cohort");return p[0]=='1';}
struct alignas(32) PairPack364{__m256i z;};
inline Group pair_group0{2},pair_group1{2};
inline std::array<std::vector<PairPack364>,2> pair_keys364;
inline Group&pair_group364(){need(slot>=0&&slot<4,"364 owner bound");return slot%2?pair_group1:pair_group0;}
inline void poison_groups364(){group.poison();pair_group0.poison();pair_group1.poison();}
template<class Seed>void compute_pair364(B*out,const B*in,int64_t n,int k,int mask,Seed&&fresh_seed){
 need(slot>=0&&slot<4,"364 explicit owner");const int rank=slot/2,index=slot%2;auto&g=pair_group364();auto&keys=pair_keys364[index];
 try{
  note(n);++stats[5];stats[6]+=n;g.entries[rank]={out,in,n,k,mask};g.wait();
  if(rank==0){
   need((n==10608640&&k==589824)||(n==649728&&k==36288),"364 original native sizes");
   for(int i=0;i<2;++i){const auto&e=g.entries[i];need(e.n==n&&e.k==k&&e.mask==mask&&e.out&&e.in,"364 pair public shape");
    uintptr_t a=reinterpret_cast<uintptr_t>(e.out),b=reinterpret_cast<uintptr_t>(e.in);need(a+size_t(n)*16<=b||b+size_t(k)*16<=a,"364 distinct native arrays");}
   // Fresh seed only after both independent MPCOT/input states are fixed.
   g.seed=fresh_seed();keys.resize(k);
  }
  g.wait();const int first=int(int64_t(k)*rank/2),last=int(int64_t(k)*(rank+1)/2);
  for(int j=first;j<last;++j)keys[j].z=_mm256_inserti128_si256(_mm256_castsi128_si256(g.entries[0].in[j]),g.entries[1].in[j],1);
  stats[2]+=last-first;g.wait();emp::PRP prp(g.seed);
  const int64_t groups=(n-1)/4,begin=groups*rank/2,end=groups*(rank+1)/2;stats[3]+=end-begin;
  auto apply=[&](int64_t i,const uint32_t*r,bool tail){
   B*a=g.entries[0].out+i,*b=g.entries[1].out+i;__m256i z=_mm256_inserti128_si256(_mm256_castsi128_si256(*a),*b,1);
   for(int j=0;j<10;++j){uint32_t x=tail?r[j]%uint32_t(k):(r[j]&uint32_t(mask));if(!tail&&x>=uint32_t(k))x-=k;z=_mm256_xor_si256(z,keys[x].z);}
   *a=_mm256_castsi256_si128(z);*b=_mm256_extracti128_si256(z,1);
  };
  for(int64_t i=begin;i<end;++i){const int64_t row=4*i;B tmp[10];for(int m=0;m<10;++m)tmp[m]=emp::makeBlock(row,m);
   emp::AES_ecb_encrypt_blks(tmp,10,&prp.aes);uint32_t r[40];std::memcpy(r,tmp,sizeof r);for(int m=0;m<4;++m)apply(row+m,r+10*m,false);}
  if(rank==1)for(int64_t i=4*groups;i<n;++i){B tmp[3];for(int m=0;m<3;++m)tmp[m]=emp::makeBlock(i,m);prp.permute_block(tmp,3);uint32_t r[12];std::memcpy(r,tmp,sizeof r);apply(i,r,true);++stats[4];}
  g.wait();if(rank==0){OPENSSL_cleanse(keys.data(),keys.size()*sizeof(PairPack364));std::vector<PairPack364>().swap(keys);}
  g.entries[rank]={};g.wait();
 }catch(...){poison_groups364();throw;}
}

template<class Seed> void compute(B* out,const B* in,int64_t n,int k,int mask,Seed&& fresh_seed){
 if(paired364()){compute_pair364(out,in,n,k,mask,std::forward<Seed>(fresh_seed));return;}
 need(slot>=0&&slot<4,"315 no implicit owner");
 try{
  note(n);++stats[5];stats[6]+=n;group.entries[slot]={out,in,n,k,mask};group.wait();
  if(slot==0){
   need((n==10608640&&k==589824)||(n==649728&&k==36288),"315 pinned native sizes");
   for(auto&e:group.entries){need(e.n==n&&e.k==k&&e.mask==mask&&e.out&&e.in,"315 equal public cohorts only");
    uintptr_t a=reinterpret_cast<uintptr_t>(e.out),b=reinterpret_cast<uintptr_t>(e.in);
    need(a+size_t(n)*16<=b||b+size_t(k)*16<=a,"315 distinct native arrays");}
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