#include "resnet50/engine/directed_phase.hpp"
#pragma once
#include "resnet50/core/types.hpp"
#include <cstdlib>
#include <cstring>
#include <openssl/crypto.h>
extern "C" int aqcf_elide249_lookup(aqcf_silent_handle*,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
namespace aqcf_terminal304 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_JOINT304");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"304 prospective arm");return p[0]=='1';}
struct Census{U calls=0,rows=0,cot_rows=0;};inline thread_local Census census;
template<class B> Ring emit25(B& s,const Ring& a,const Bits& low,const Bits& high,const std::vector<U>& tap){
 a.check();low.check();high.check();const size_t n=a.v.size();
 require(a.width==25&&n&&n<=262144&&low.v.size()==n&&high.v.size()==n&&tap.size()==n,"304 complete terminal domain");
 std::vector<U> q(n),table(s.party==1?2*n:0),got(n),coefficient(n);
 std::vector<uint8_t> choice(s.party==2?n:0),selector(n);
 for(size_t j=0;j<n;++j){
  require(tap[j]<=mask(25),"304 canonical joint tap");q[j]=(a.v[j]+tap[j])&mask(25);
  U gs=(a.v[j]>>24)^high.v[j]^U(s.party==1);
  if(s.party==1)for(U c=0;c<2;++c){U g=gs^c;table[2*j+c]=(((g*q[j])&mask(25))<<1)|g;}
  else choice[j]=gs;
 }
 auto* h=s.control().owned189_handle();
 require(direction359::aqcf_elide359_lookup(h,direction359::sender(s,1),s.party==1?table.data():nullptr,s.party==2?choice.data():nullptr,got.data(),int(n),1,26,1)==0,aqcf_silent_error(h));++s.lookup_calls;s.lookup_rows+=n;s.lookup_table_bits+=n*26;s.flush();
 Ring out(25,n);
 for(size_t j=0;j<n;++j){U g=got[j]&1;
  if(s.party==1){out.v[j]=got[j]>>1;selector[j]=g;}
  else{out.v[j]=((got[j]>>1)+q[j]*g)&mask(25);coefficient[j]=(q[j]*(U(1)-2*g))&mask(25);}
 }
 auto product=s.owned_word_bit_product(2,coefficient,selector,25);
 for(size_t j=0;j<n;++j)out.v[j]=(out.v[j]+product.v[j])&mask(25);
 for(auto* v:{&q,&table,&got,&coefficient,&product.v})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 for(auto* v:{&choice,&selector})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());
 ++census.calls;census.rows+=n;census.cot_rows+=n;out.check();return out;
}
}
