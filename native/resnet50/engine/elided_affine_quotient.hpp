#pragma once
#include "resnet50/engine/quotient_recovery_core.hpp"
#include "resnet50/engine/elided_terminal.hpp"
#include "resnet50/engine/radix_quotient_core.hpp"
#include "resnet50/engine/quotient_phase_relu.hpp"
#include "resnet50/engine/protected_residual.hpp"
#include "resnet50/engine/prescribed_correlation.hpp"
extern "C" int aqcf_seed221_prepare2(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_prepare3(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_read_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*);
namespace aqcf_mixed249 {using namespace aqcf2;
struct Private {
 std::vector<std::vector<uint8_t>> alpha;std::vector<uint8_t> leaf,final,digit,value;
 explicit Private(size_t n):alpha(11,std::vector<uint8_t>(n)),leaf(n),final(n),digit(n),value(n){}
 ~Private(){for(auto& v:alpha)if(!v.empty())OPENSSL_cleanse(v.data(),v.size());for(auto* v:{&leaf,&final,&digit,&value})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
};
template<class B>LinkedCarry prefix(B& s,const std::vector<U>& own,const std::vector<U>& lo,Bits& tapped,const std::vector<uint8_t>* pl=nullptr,const std::vector<uint8_t>* ph=nullptr){
 const size_t n=own.size();require(n>0&&n<=200704&&lo.size()==n&&bool(pl)==bool(ph)&&!radix197_enabled(),"221 fixed full32 scope and mask ownership");
 if(pl)require(pl->size()==n&&ph->size()==n,"221 prescribed masks size");
 Private p(n);std::vector<uint64_t> ids(11);constexpr unsigned widths[11]={3,3,2,3,3,3,3,3,3,3,3};uint64_t leaf_id=0;auto* h=s.control().owned189_handle();
 auto check=[&](int rc){require(rc==0,aqcf_silent_error(h));};
 if(s.party==1){if(ph)p.final=*ph;else{auto r=s.random_small(n);for(size_t i=0;i<n;++i)p.final[i]=r[i]&1;OPENSSL_cleanse(r.data(),r.size());}}
 s.boundary_profile("mixed237_paid_leaf_and_core_preparation",[&]{
  check(aqcf_seed221_prepare3(h,1,s.party==1?p.leaf.data():nullptr,int(n),&leaf_id));
  for(unsigned j=0;j<11;++j){
   if(pl&&j==3){if(s.party==1)p.alpha[j]=*pl;ids[j]=s.control().quotient197_prepare(p.alpha[j],3);}
   else if(widths[j]==2)check(aqcf_seed221_prepare2(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
   else check(aqcf_seed221_prepare3(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
  }return 0;
 });
 std::vector<U> choice(n),table(s.party==1?8*n:0);
 for(size_t i=0;i<n;++i){require(own[i]<=mask(37)&&lo[i]<4,"221 normalized Ring37/quarter domain");
  U u=s.party==1?(lo[i]==0?0:lo[i]==3?2:3):((lo[i]+1)&3);
  U x=((own[i]&1)<<2)|u;choice[i]=x;
  if(s.party==1)for(unsigned y=0;y<8;++y)table[8*i+y]=((x+y)>>3)^p.leaf[i];
 }
 auto got=s.lookup_words(table,choice,n,3,1,false);s.flush();
 for(size_t i=0;i<n;++i){p.value[i]=s.party==1?p.leaf[i]:uint8_t(got[i]);p.digit[i]=(own[i]>>1)&7;}
 auto next=s.control().quotient197_consume(leaf_id,p.digit,p.value,p.alpha[0]);
 OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);
 s.boundary_profile("mixed237_eleven_core_exchanges",[&]{
  unsigned offset=4;
  for(unsigned j=0;j<11;++j){for(size_t i=0;i<n;++i)p.digit[i]=(own[i]>>offset)&((1U<<widths[j])-1);
   auto next=widths[j]==2?s.control().quotient169_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final):s.control().quotient197_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final);
   OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);offset+=widths[j];if(offset==12)tapped.v=p.value;
  }require(offset==36,"237 all full signed bits retained");return 0;
 });
 if(pl&&s.party==1)require(tapped.v==*pl&&p.value==*ph,"221 retained prescribed tap/final masks");
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 LinkedCarry out;out.value.emplace_back(n);out.value[0].v=p.value;
 out.packet_rows=12*n;out.packet_table_bits=92*n;out.packet_calls=12;out.selector_rows=0;
 tapped.check();out.value[0].check();return out;
}
template<class B>Ring consume(B& s,const std::vector<U>& phase,const std::vector<U>* add=nullptr,const std::vector<uint8_t>* pl=nullptr,const std::vector<uint8_t>* ph=nullptr){
 const size_t n=phase.size();require(n>0&&n<=200704&&s.compact_cofactor&&(!add||add->size()==n),"221 original full consumer");
 std::vector<U> own(n),lo(n);Ring coarse(25,n);
 for(size_t i=0;i<n;++i){require(phase[i]<(U(1)<<39),"221 exact Ring39 phase");lo[i]=phase[i]&3;U hi=phase[i]>>2;
  own[i]=(s.party==1?U(0)-hi-U(lo[i]!=0):hi+U(lo[i]==3))&mask(37);
  if(add){require((*add)[i]<=mask(37),"221 Ring37 residual");own[i]=(own[i]+(*add)[i])&mask(37);}coarse.v[i]=own[i]>>12;
 }
 Bits low;auto rest=prefix(s,own,lo,low,pl,ph);
 s.note_linked_execution(n,n,rest.packet_rows+n,rest.packet_table_bits+8*n,rest.selector_rows);
 return add?floor216::emit(s,coarse,low,rest.value[0]):aqcf_terminal249::emit(s,coarse,low,rest.value[0]);
}
}
