#include "resnet50/engine/directed_phase.hpp"
#pragma once
#include <vector>
#include <cstdlib>
#include <cstring>
#include <openssl/crypto.h>
#include "resnet50/core/types.hpp"
namespace aqcf_tree335 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_TREE334");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"334 explicit fixed arm");return p[0]=='1';}
struct Census{uint64_t calls=0,rows=0,leaf_rows=0,composition_rows=0;};
inline thread_local Census census;
struct Node{std::vector<uint8_t> v;}; // XOR shares: bit0 generate, bit1 propagate
extern "C" int aqcf_leaf335_lookup(aqcf_silent_handle*,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
struct Result{std::vector<U> own37;Bits low,high;};
inline void wipe(std::vector<U>&v){if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(U));}
inline void wipe(std::vector<uint8_t>&v){if(!v.empty())OPENSSL_cleanse(v.data(),v.size());}

// Two independent fresh hashed-ROT cross products share one role-ordered exchange.
extern "C" int aqcf_duplex334_product(aqcf_silent_handle*,const uint8_t*,const uint8_t*,uint8_t*,int,unsigned);
template<class B>std::vector<uint8_t> product(B& s,const std::vector<uint8_t>& a,const std::vector<uint8_t>& b,unsigned width){
 require(a.size()==b.size()&&!a.empty()&&a.size()<=(1u<<20)&&(width==1||width==2),"334 product dimensions");
 std::vector<uint8_t> out(a.size());auto* h=s.control().owned189_handle();
 require(aqcf_duplex334_product(h,a.data(),b.data(),out.data(),int(a.size()),width)==0,aqcf_silent_error(h));s.flush();return out;
}

template<class B>Result prepare(B& s,const std::vector<U>& phase,const std::vector<U>* add=nullptr){
 const size_t n=phase.size();require(n&&n<=25088&&s.compact_cofactor&&(!add||add->size()==n),"334 fixed small-consumer frontier");
 Result out;out.own37.resize(n);std::vector<U> digit(13*n),table(s.party==1?8*13*n:0);

 for(size_t i=0;i<n;++i){
  require(phase[i]<(U(1)<<39),"334 original full phase domain");const U lo=phase[i]&3,hi=phase[i]>>2;
  out.own37[i]=(direction359::physical(s)==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);
  if(add){require((*add)[i]<=mask(37),"334 residual domain");out.own37[i]=(out.own37[i]+(*add)[i])&mask(37);}
  digit[i]=direction359::physical(s)==1?(lo==0?0:lo==3?2:3):((lo+1)&3);
  for(unsigned d=1;d<13;++d)digit[d*n+i]=(out.own37[i]>>(3*(d-1)))&7;
 }
 if(s.party==1)for(unsigned d=0;d<13;++d)for(size_t i=0;i<n;++i)for(unsigned c=0;c<8;++c){
  const unsigned Bv=d?8:4,sum=unsigned(digit[d*n+i])+(d?c:(c&3));
  const unsigned state=unsigned(sum>=Bv)|(unsigned(sum==Bv-1)<<1);
  table[(d*n+i)*8+c]=state;
 }
 std::vector<uint8_t> selection(s.party==2?13*n:0);if(s.party==2)for(size_t i=0;i<13*n;++i)selection[i]=digit[i];
 std::vector<U> got(13*n);auto* h=s.control().owned189_handle();
 require(direction359::aqcf_leaf359_lookup(h,direction359::sender(s,1),s.party==1?table.data():nullptr,s.party==2?selection.data():nullptr,got.data(),int(13*n),3,2,2)==0,aqcf_silent_error(h));s.flush();wipe(selection);
 std::vector<std::vector<Node>> forest(2);
 for(unsigned d=0;d<13;++d){Node x;x.v.resize(n);for(size_t i=0;i<n;++i)x.v[i]=uint8_t(got[d*n+i]);forest[d<5?0:1].push_back(std::move(x));}
 wipe(digit);wipe(table);wipe(got);census.leaf_rows+=13*n;
 unsigned levels=0;
 for(;;){
  size_t pairs=0;for(auto& part:forest)pairs+=part.size()/2;if(!pairs)break;
  std::vector<uint8_t> a(pairs*n),b(pairs*n);size_t k=0;
  for(auto& part:forest)for(size_t j=0;j+1<part.size();j+=2){for(size_t i=0;i<n;++i){a[k*n+i]=part[j].v[i];b[k*n+i]=(part[j+1].v[i]>>1)&1;}++k;}
  auto v=product(s,a,b,2);wipe(a);wipe(b);k=0;
  for(auto& part:forest){std::vector<Node> next;
   for(size_t j=0;j<part.size();j+=2){if(j+1==part.size()){next.push_back(std::move(part[j]));continue;}
    Node x;x.v.resize(n);for(size_t i=0;i<n;++i)x.v[i]=v[k*n+i]^(part[j+1].v[i]&1);++k;
    wipe(part[j].v);wipe(part[j+1].v);next.push_back(std::move(x));
   }part=std::move(next);
  }
  require(k==pairs,"334 complete public composition bucket");wipe(v);census.composition_rows+=pairs*n;++levels;
 }
 require(levels==3&&forest[0].size()==1&&forest[1].size()==1,"334 fixed13leaf reduction");
 out.low=Bits(n);out.high=Bits(n);std::vector<uint8_t> a(n),b(n);
 for(size_t i=0;i<n;++i){out.low.v[i]=a[i]=forest[0][0].v[i]&1;b[i]=(forest[1][0].v[i]>>1)&1;}
 auto v=product(s,a,b,1);for(size_t i=0;i<n;++i)out.high.v[i]=(forest[1][0].v[i]&1)^v[i];
 wipe(a);wipe(b);wipe(v);for(auto& part:forest)wipe(part[0].v);
 out.low.check();out.high.check();++census.calls;census.rows+=n;census.composition_rows+=n;return out;
}
}
