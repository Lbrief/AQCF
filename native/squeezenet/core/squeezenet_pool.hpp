#include "squeezenet/core/retained_squeezenet_phase.hpp"
#pragma once
#include <array>
#include <cstdlib>
#include <cstring>
#include <openssl/crypto.h>
extern "C" int aqcf_seed221_prepare3(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
namespace aqcf_pool273 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_POOL273");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"273 explicit arm");return p[0]=='1';}
struct Census{U calls=0,rows=0,core_rows=0;};inline thread_local Census census;
struct Secret{std::array<std::vector<uint8_t>,7> alpha;std::vector<uint8_t> last,value,digit;
 explicit Secret(size_t n):last(n),value(n),digit(n){for(auto& v:alpha)v.resize(n);}
 ~Secret(){for(auto& v:alpha)OPENSSL_cleanse(v.data(),v.size());for(auto* v:{&last,&value,&digit})OPENSSL_cleanse(v->data(),v->size());}};
// Exact carry from two private additive words, low24 bits, public-zero seed.
// First3-bit leaf and seven fresh existing seed221/quotient197 transitions.
// No change to masks, native supplier, Ring25 mux, or original pool geometry.
template<class B> Bits carry24(B& s,const std::vector<U>& own){
 const size_t n=own.size();require(n&&n<=262144&&s.compact_cofactor,"273 original frontier");
 Secret p(n);std::array<uint64_t,7> ids{};auto* h=s.control().owned189_handle();
 phase317::Phase phase(h,int(n),2);
 auto checked=[&](int rc){require(rc==0,aqcf_silent_error(h));};
 if(s.party==1){auto r=s.random_small(n);for(size_t i=0;i<n;++i)p.last[i]=r[i]&1;OPENSSL_cleanse(r.data(),r.size());}
 s.boundary_profile("pool273_all_paid_preparation",[&]{
  for(unsigned j=0;j<7;++j)checked(aqcf_seed221_prepare3(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));return 0;});
 std::vector<U> choice(n),table(s.party==1?8*n:0);
 for(size_t i=0;i<n;++i){const U a=own[i]&7;choice[i]=a;
  if(s.party==1)for(U b=0;b<8;++b)table[8*i+b]=((a+b)>>3)^p.alpha[0][i];}
 auto got=s.lookup_words(table,choice,n,3,1,false);s.flush();
 for(size_t i=0;i<n;++i)p.value[i]=s.party==1?p.alpha[0][i]:uint8_t(got[i]);
 s.boundary_profile("pool273_seven_dependent_cores",[&]{
  for(unsigned j=0;j<7;++j){for(size_t i=0;i<n;++i)p.digit[i]=(own[i]>>(3+3*j))&7;
   auto next=s.control().quotient197_consume(ids[j],p.digit,p.value,j<6?p.alpha[j+1]:p.last);
   OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);}
  return 0;});
 Bits out(n);out.v=p.value;out.check();++census.calls;census.rows+=n;census.core_rows+=7*n;
 s.note_linked_execution(0,0,8*n,64*n,0);
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));return out;
}
}
