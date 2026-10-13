#pragma once
#include "resnet50/engine/pool_frontier.hpp"
#include "resnet50/protocol/pool_carry.h"
namespace aqcf_pool263 {
using namespace aqcf2;
struct Private {
 std::vector<uint8_t> alpha,final,digit,value;
 explicit Private(size_t n):alpha(8*n),final(n),digit(n),value(n){}
 ~Private(){for(auto* p:{&alpha,&final,&digit,&value})if(!p->empty())OPENSSL_cleanse(p->data(),p->size());}
};
template<class B> Bits carry37(B& s,const std::vector<U>& own){
 const size_t n=own.size();require(n>0&&n<=100352&&s.compact_cofactor,"263 exact full37 carry domain");
 Private p(n);uint64_t ids[8]{};auto* h=s.control().owned189_handle();
 auto check=[&](int rc){require(rc==0,aqcf_silent_error(h));};
 if(s.party==1){auto r=s.random_small(n);for(size_t i=0;i<n;++i)p.final[i]=r[i]&1;OPENSSL_cleanse(r.data(),r.size());}
 s.boundary_profile("pool263_eight_fresh_radix16_preparations",[&]{
  check(aqcf_pool263_prepare(h,1,int(n),8,ids,s.party==1?p.alpha.data():nullptr));return 0;
 });
 std::vector<U> choice(n),table(s.party==1?n:0);
 for(size_t i=0;i<n;++i){require(own[i]<=mask(38),"263 unchanged Ring38 difference");choice[i]=own[i]&31;
  if(s.party==1)table[i]=aqcf_cofactor_word::threshold(choice[i],0,5)^(p.alpha[i]?mask(32):0);
 }
 auto got=s.lookup_words(table,choice,n,5,1,true);s.flush();
 for(size_t i=0;i<n;++i)p.value[i]=s.party==1?p.alpha[i]:uint8_t(got[i]);
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 s.boundary_profile("pool263_eight_carry_exchanges",[&]{
  unsigned offset=5;
  for(unsigned j=0;j<8;++j){
   for(size_t i=0;i<n;++i)p.digit[i]=(own[i]>>offset)&15;
   std::vector<uint8_t> next(n);
   const uint8_t* nextmask=j<7?p.alpha.data()+(j+1)*n:p.final.data();
   check(aqcf_pool263_consume(h,ids[j],p.digit.data(),p.value.data(),s.party==1?nextmask:nullptr,next.data(),int(n)));
   OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);offset+=4;
  }
  require(offset==37,"263 covers all37 low bits");return 0;
 });
 aqcf_pool263_stats stats{};check(aqcf_pool263_read_stats(h,&stats));require(stats.live_banks==0,"263 complete one-use banks");
 Bits out(n);out.v=p.value;out.check();return out;
}
template<class B> Ring maximum(B& s,const Ring& a,const Ring& b){
 a.check();b.check();const size_t n=a.v.size();
 require(a.width==38&&b.width==38&&n==b.v.size()&&n>0&&n<=100352,"263 full-signed max shape");
 Ring diff(38,n);for(size_t j=0;j<n;++j)diff.v[j]=(b.v[j]-a.v[j])&mask(38);
 auto carry=carry37(s,diff.v);auto sign=s.xor_(carry,s.local_bit(diff.v,37));
 auto pos=s.mux(s.not_(sign),diff);
 for(size_t j=0;j<n;++j)pos.v[j]=(pos.v[j]+a.v[j])&mask(38);
 pos.check();return pos;
}
}
