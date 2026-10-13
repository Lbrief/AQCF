#pragma once
#include <vector>
#include <cstdlib>
#include <cstring>
#include <openssl/crypto.h>
#include "squeezenet/engine/types.hpp"
namespace aqcf_tree280 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_TREE280");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"280 explicit fixed arm");return p[0]=='1';}
struct Census{uint64_t calls=0,rows=0,leaf_rows=0,composition_rows=0;};
inline thread_local Census census;
struct Node{std::vector<uint8_t> v;}; // XOR shares: bit0 generate, bit1 propagate
struct Result{std::vector<U> own37;Bits low,high;};
inline void wipe(std::vector<U>&v){if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(U));}
inline void wipe(std::vector<uint8_t>&v){if(!v.empty())OPENSSL_cleanse(v.data(),v.size());}

// One fresh chosen lookup computes packed XOR action times a secret bit.
// Server holds the output pad; client receives only its one masked result.
template<class B>std::vector<uint8_t> product(B& s,const std::vector<uint8_t>& a,const std::vector<uint8_t>& b,unsigned width){
 require(a.size()==b.size()&&!a.empty()&&a.size()<=(1u<<20)&&(width==1||width==2),"280 product dimensions");
 const size_t n=a.size();const U mask=(1u<<width)-1,entries=1u<<(width+1);
 auto pad=s.party==1?s.random_small(n):std::vector<uint8_t>{};
 std::vector<U> choice(n),table(s.party==1?entries*n:0);std::vector<uint8_t> out(n);
 for(size_t i=0;i<n;++i){require(a[i]<=mask&&b[i]<2,"280 exact XOR domain");choice[i]=U(a[i])|(U(b[i])<<width);
  if(s.party==1)for(U c=0;c<entries;++c)table[entries*i+c]=((b[i]^(c>>width))?(U(a[i])^(c&mask)):U(0))^(pad[i]&mask);}
 auto got=s.lookup_words(table,choice,n,width+1,width,false);s.flush();
 for(size_t i=0;i<n;++i)out[i]=s.party==1?(pad[i]&mask):got[i];
 wipe(pad);wipe(choice);wipe(table);wipe(got);return out;
}

template<class B>Result prepare(B& s,const std::vector<U>& phase){
 const size_t n=phase.size();require(n&&n<=8192&&s.compact_cofactor,"280 fixed small-consumer frontier");
 Result out;out.own37.resize(n);std::vector<U> digit(13*n),table(s.party==1?8*13*n:0);
 std::vector<uint8_t> pad;if(s.party==1)pad=s.random_small(13*n);
 for(size_t i=0;i<n;++i){
  require(phase[i]<(U(1)<<57),"280 original full phase domain");const U lo=phase[i]&3,hi=phase[i]>>2;
  out.own37[i]=(s.party==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);
  digit[i]=s.party==1?(lo==0?0:lo==3?2:3):((lo+1)&3);
  for(unsigned d=1;d<13;++d)digit[d*n+i]=(out.own37[i]>>(3*(d-1)))&7;
 }
 if(s.party==1)for(unsigned d=0;d<13;++d)for(size_t i=0;i<n;++i)for(unsigned c=0;c<8;++c){
  const unsigned Bv=d?8:4,sum=unsigned(digit[d*n+i])+(d?c:(c&3));
  const unsigned state=unsigned(sum>=Bv)|(unsigned(sum==Bv-1)<<1);
  table[(d*n+i)*8+c]=state^pad[d*n+i];
 }
 auto got=s.control().lookup(table,digit,13*n,3,2,false);s.flush();
 std::vector<std::vector<Node>> forest(2);
 for(unsigned d=0;d<13;++d){Node x;x.v.resize(n);for(size_t i=0;i<n;++i)x.v[i]=s.party==1?pad[d*n+i]:uint8_t(got[d*n+i]);forest[d<5?0:1].push_back(std::move(x));}
 wipe(pad);wipe(digit);wipe(table);wipe(got);census.leaf_rows+=13*n;
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
  require(k==pairs,"280 complete public composition bucket");wipe(v);census.composition_rows+=pairs*n;++levels;
 }
 require(levels==3&&forest[0].size()==1&&forest[1].size()==1,"280 fixed13leaf reduction");
 out.low=Bits(n);out.high=Bits(n);std::vector<uint8_t> a(n),b(n);
 for(size_t i=0;i<n;++i){out.low.v[i]=a[i]=forest[0][0].v[i]&1;b[i]=(forest[1][0].v[i]>>1)&1;}
 auto v=product(s,a,b,1);for(size_t i=0;i<n;++i)out.high.v[i]=(forest[1][0].v[i]&1)^v[i];
 wipe(a);wipe(b);wipe(v);for(auto& part:forest)wipe(part[0].v);
 out.low.check();out.high.check();++census.calls;census.rows+=n;census.composition_rows+=n;return out;
}
}
