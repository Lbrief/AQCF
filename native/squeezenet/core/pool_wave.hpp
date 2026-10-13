#include "squeezenet/core/squeezenet_pool.hpp"
#pragma once
#include "squeezenet/core/pool_selected_transition.hpp"
#include "squeezenet/core/direct_token_core.hpp"
#include "squeezenet/core/quotient_pool38.hpp"
#include "squeezenet/core/wave_policy.hpp"
#include <openssl/crypto.h>

// Pool-only public scheduling specialization of the frozen linked construction.
// No global boundary size change, new primitive, or security-strength claim.
namespace aqcf_pool_wave {
using namespace aqcf2;
constexpr std::size_t limit=aqcf_wan_wave::pool_limit;

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
 if(width==24&&aqcf_pool273::enabled())return aqcf_pool273::carry24(s,own);
#if AQCF_POOL_SELECTED
 if(width==24||width==25)return s.boundary_profile("pool_selected_transition",[&]{return aqcf_pool_selected::carry(s,own,width);});
#endif

 require(own.size()<=limit&&(width==24||width==36||width==37),"direct pool public shape");
 auto r=s.boundary_profile("direct_token_pool",[&]{return aqcf_direct_carry20::run(s,own,width,s.constant(false,own.size()),true,0);});
 s.note_linked_execution(width==36?own.size():0,width==37?own.size():0,r.rows,r.bits,0);
 return r.carry;

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
