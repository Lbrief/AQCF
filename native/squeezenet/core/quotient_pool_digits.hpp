#pragma once
#include "squeezenet/core/pool_selected_transition.hpp"
#include "squeezenet/core/quotient_pool38.hpp"
#include "squeezenet/core/wave_policy.hpp"
#include <openssl/crypto.h>

// Pool-only public scheduling specialization of the frozen linked construction.
// No global boundary size change, new primitive, or security-strength claim.
namespace aqcf_quotient_pool_digits {
using namespace aqcf2;
constexpr std::size_t limit=aqcf_wan_wave::pool_limit;
// Candidate only: keep the original protected recurrence and OT primitives,
// but choose public 5-bit digits (one 64-bit two-cofactor response row).
// The old spread16 helper truncates a 32-choice row; use a full 32-bit spread.
inline U spread32(U x){
 x&=0xffffffffULL;
 x=(x|(x<<16))&0x0000ffff0000ffffULL;
 x=(x|(x<<8))&0x00ff00ff00ff00ffULL;
 x=(x|(x<<4))&0x0f0f0f0f0f0f0f0fULL;
 x=(x|(x<<2))&0x3333333333333333ULL;
 return (x|(x<<1))&0x5555555555555555ULL;
}
inline U mixed_pair(unsigned a,unsigned r,unsigned next,unsigned p0,unsigned p1,unsigned bits){
 const U all=mask(1U<<bits);
 const U v0=aqcf_cofactor_word::threshold(a,r,bits)^((next^p0)?all:0);
 const U v1=aqcf_cofactor_word::threshold(a,r^1,bits)^((next^p1)?all:0);
 return spread32(v0)|(spread32(v1)<<1);
}

template<class B> std::vector<U> lookup(B& s,std::vector<U>& table,
 const std::vector<U>& choices,std::size_t n,unsigned in,unsigned out,bool compact){
 require(n>0&&choices.size()==n,"wave lookup dimensions");
 const std::size_t stride=compact?1:(U(1)<<in);
 require(s.party!=1||table.size()==n*stride,"wave one-owner table");
 if(n<=65536)return s.lookup_words(table,choices,n,in,out,compact);
 std::vector<U> result(n);
 for(std::size_t begin=0;begin<n;begin+=65536){
  const auto len=std::min<std::size_t>(65536,n-begin);
  std::vector<U> c(choices.begin()+begin,choices.begin()+begin+len),t;
  if(s.party==1)t.assign(table.begin()+begin*stride,table.begin()+(begin+len)*stride);
  auto v=s.lookup_words(t,c,len,in,out,compact);
  std::copy(v.begin(),v.end(),result.begin()+begin);
  if(!t.empty())OPENSSL_cleanse(t.data(),t.size()*sizeof(U));
 }
 return result;
}

// Exactly one public-zero-seed carry task. For each independent value j, the
// frozen recurrence is unchanged; this moves independent j's into one wave.
template<class B> Bits carry(B& s,const std::vector<U>& own,unsigned width){
#if AQCF_POOL_SELECTED
 if(width==24||width==25)return s.boundary_profile("pool_selected_transition",[&]{return aqcf_pool_selected::carry(s,own,width);});
#endif
 const auto n=own.size();require(n>0&&n<=limit&&(width==12||width==24||width==25||width==36||width==37),"public quotient-pool carry domain");
 require(s.compact_cofactor,"retain compact cofactor representation");
 const unsigned depth=(width+4)/5;const std::size_t selectors=(depth-1)*n;
 std::vector<unsigned> digits(depth,5),offsets(depth,0);
 digits.back()=width-5*(depth-1);
 // A one-bit final digit would fall through to the unchanged but expensive
 // binary SDK path. Repartition the same final6bits as4+2, never pad/omit bits.
 if(digits.back()==1){digits[depth-2]=4;digits.back()=2;}
 for(unsigned d=1;d<depth;++d)offsets[d]=offsets[d-1]+digits[d-1];
 require(offsets.back()+digits.back()==width,"complete public bit partition");
 require(AQCF_LAYER_FRONTIER ? selectors<=9*262144 : (selectors+3)/4<=65536,"bounded frontier; original tuple bank cap enforced per chunk");
 std::vector<Bits> masks(depth,Bits(n));std::vector<std::vector<U>> packet(depth,std::vector<U>(n));
 if(s.party==1){auto r=s.random_small(depth*n);
  for(unsigned d=0;d<depth;++d)for(std::size_t j=0;j<n;++j)masks[d].v[j]=r[d*n+j]&1;
  OPENSSL_cleanse(r.data(),r.size());}
 std::vector<std::uint8_t> pad0(selectors),pad1(selectors),used(selectors);
 if(s.party==1){auto r=s.random_small(selectors);for(std::size_t j=0;j<selectors;++j){pad0[j]=r[j]&1;pad1[j]=(r[j]>>1)&1;}OPENSSL_cleanse(r.data(),r.size());}
 Bits state(n);U packet_rows=0,packet_bits=0;
 s.boundary_profile("pool_wave_packet_preparation",[&]{
  std::vector<U> choices(n),table(s.party==1?n:0);
  for(std::size_t j=0;j<n;++j){choices[j]=own[j]&31;
   if(s.party==1)table[j]=aqcf_cofactor_word::threshold(choices[j],0,5)^(masks[0].v[j]?mask(32):0);}
  auto got=lookup(s,table,choices,n,5,1,true);
  for(std::size_t j=0;j<n;++j)state.v[j]=s.party==1?masks[0].v[j]:got[j];
  packet_rows+=n;packet_bits+=32*n;
  for(unsigned bits=1;bits<=5;++bits){std::vector<unsigned> ids;
   for(unsigned d=1;d<depth;++d)if(digits[d]==bits)ids.push_back(d);
   if(ids.empty())continue;const auto rows=ids.size()*n;const U count=U(1)<<bits;
   choices.assign(rows,0);table.assign(s.party==1?rows:0,0);
   for(std::size_t k=0;k<ids.size();++k){auto d=ids[k];
    for(std::size_t j=0;j<n;++j){auto at=k*n+j,pi=(d-1)*n+j;
     U digit=(own[j]>>offsets[d])&mask(bits);choices[at]=digit;
     if(s.party==1)table[at]=mixed_pair(digit,masks[d-1].v[j],masks[d].v[j],pad0[pi],pad1[pi],bits);}}
   got=lookup(s,table,choices,rows,bits,2,true);
   if(s.party==2)for(std::size_t k=0;k<ids.size();++k)std::copy_n(got.begin()+k*n,n,packet[ids[k]].begin());
   packet_rows+=rows;packet_bits+=2*rows*count;
  }
  return 0;
 });
 auto rot=s.boundary_profile("pool_wave_selector_preparation",[&]{return s.random_ot_bits(selectors);});
 s.boundary_profile("pool_wave_selector_consumption",[&]{
  for(unsigned d=1;d<depth;++d){
   std::vector<std::uint8_t> e(n),reply(2*n);
   for(std::size_t j=0;j<n;++j){auto pi=(d-1)*n+j;require(!used[pi],"wave ROT cannot be reused");used[pi]=1;
    if(s.party==2)e[j]=state.v[j]^rot.t[pi];else require(state.v[j]==masks[d-1].v[j],"wave carry predecessor");}
   e=s.transfer_masked_bits(2,e);
   if(s.party==1)for(std::size_t j=0;j<n;++j){auto pi=(d-1)*n+j;
    reply[2*j]=(e[j]?pad1[pi]:pad0[pi])^rot.k0[pi];
    reply[2*j+1]=(e[j]?pad0[pi]:pad1[pi])^rot.k1[pi];}
   reply=s.transfer_masked_bits(1,reply);
   for(std::size_t j=0;j<n;++j){auto pi=(d-1)*n+j;
    if(s.party==1)state.v[j]=masks[d].v[j];
    else {U incoming=state.v[j],selected=reply[2*j+rot.t[pi]]^rot.kt[pi];state.v[j]=((packet[d][j]>>incoming)&1)^selected;}
    rot.k0[pi]=rot.k1[pi]=rot.t[pi]=rot.kt[pi]=pad0[pi]=pad1[pi]=0;
   }
  }
  return 0;
 });
 require(std::all_of(used.begin(),used.end(),[](auto x){return x==1;}),"all wave selectors consumed once");
 s.note_linked_execution(width==36?n:0,width==37?n:0,packet_rows,packet_bits,selectors);
 state.check();return state;
}

template<class B> Ring lift(B& s,const Ring& in){
 in.check();require(in.v.size()>0&&in.v.size()<=limit,"pool lift wave size");
 const auto n=in.v.size();std::vector<U> own(n);for(std::size_t j=0;j<n;++j)own[j]=in.v[j]&mask(37);
 auto c=carry(s,own,36);std::vector<U> choices(n),table,pad;
 if(s.party==1){pad=s.random_ring_words(n,1);table.resize(n*4);}
 for(std::size_t j=0;j<n;++j){U top=own[j]>>36;choices[j]=top|(U(c.v[j])<<1);
  if(s.party==1)for(U v=0;v<4;++v){U cy=U(c.v[j])^(v>>1),k=(top+(v&1)+cy+1)/2;table[4*j+v]=(k-pad[j])&1;}}
 auto got=lookup(s,table,choices,n,2,1,false);Ring out(38,n);
 for(std::size_t j=0;j<n;++j)out.v[j]=(own[j]-((s.party==1?pad[j]:got[j])<<37))&mask(38);
 out.check();return out;
}

template<class B> Ring maximum(B& s,const Ring& a,const Ring& b){
 a.check();b.check();require(a.width==38&&b.width==38&&a.v.size()==b.v.size()&&a.v.size()<=limit,"pool max wave shape");
 Ring diff(38,a.v.size());for(std::size_t j=0;j<diff.v.size();++j)diff.v[j]=(b.v[j]-a.v[j])&mask(38);
 auto c=carry(s,diff.v,37);auto sign=s.xor_(c,s.local_bit(diff.v,37));auto pos=s.mux(s.not_(sign),diff);
 for(std::size_t j=0;j<pos.v.size();++j)pos.v[j]=(pos.v[j]+a.v[j])&mask(38);
 pos.check();return pos;
}
}
