#pragma once
#include "resnet50/engine/joint_quotient_adapter.hpp"
#include "resnet50/engine/floor_before_pool_carry.hpp"
namespace poolfloor431 {
using namespace aqcf2;
// Same joint304 circuit specialized to a signed26 result, with its own fresh
// exact original native demand. No existing request or mask is replayed.
template<class B> inline Ring raw_floor26(B& s,const std::vector<U>& phase){
 const size_t n=phase.size();require(n&&n<=200704&&s.compact_cofactor,"431 original raw39 input domain");
 phase315::Phase paid(s.control().owned189_handle(),int(n),false,s.party);
 std::vector<U> own(n),lo(n),coarse(n);
 for(size_t i=0;i<n;++i){require(phase[i]<(U(1)<<39),"431 full raw phase");lo[i]=phase[i]&3;U hi=phase[i]>>2;
  own[i]=(direction359::physical(s)==1?U(0)-hi-U(lo[i]!=0):hi+U(lo[i]==3))&mask(37);coarse[i]=own[i]>>12;
 }
 Bits low;aqcf_joint304::Tap tap;auto rest=aqcf_joint304::prefix(s,own,lo,low,tap.value,26);
 s.note_linked_execution(n,n,rest.packet_rows+n,rest.packet_table_bits+8*n,rest.selector_rows);
 std::vector<U> hc(2*n);std::vector<uint8_t> hs(2*n);
 for(size_t j=0;j<n;++j){U top=coarse[j]>>24,bit=rest.value[0].v[j];
  if(s.party==1){hc[j]=bit;hc[n+j]=top^bit;}else{hs[j]=bit;hs[n+j]=top^bit;}}
 auto h=s.owned_word_bit_product(1,hc,hs,1);Ring out(26,n);
 for(size_t j=0;j<n;++j){U high=(h.v[j]+h.v[n+j]-((coarse[j]>>24)|rest.value[0].v[j]))&1;
  out.v[j]=(coarse[j]+tap.value[j]+(high<<25))&mask(26);}
 for(auto* v:{&own,&lo,&coarse,&hc,&h.v})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 if(!hs.empty())OPENSSL_cleanse(hs.data(),hs.size());out.check();return out;
}
inline Ring guarded_lift37(Sci&s,const std::vector<U>& a){
 const size_t n=a.size();require(n&&n<=65536,"431 public output frontier");
 std::vector<U> choices(n),table,pad;
 if(s.party==1){pad=s.random_ring_words(n,11);table.resize(4*n);}
 for(size_t j=0;j<n;++j){require(a[j]<=mask(26),"431 local Ring26 share");U A=a[j]>>24;choices[j]=A;
  if(s.party==1)for(U B=0;B<4;++B)table[4*j+B]=(((A+B+2)>>2)-pad[j])&mask(11);}
 auto answer=s.lookup_words(table,choices,n,2,11);Ring out(37,n);
 for(size_t j=0;j<n;++j)out.v[j]=(a[j]-((s.party==1?pad[j]:answer[j])<<26))&mask(37);
 for(auto* v:{&choices,&table,&pad,&answer})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));out.check();return out;
}
}
