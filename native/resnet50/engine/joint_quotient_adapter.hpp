#pragma once
#include "resnet50/core/types.hpp"
#include "resnet50/engine/retained_resnet_phase.hpp"
#include "resnet50/engine/elided_terminal.hpp"
#include "resnet50/engine/resnet_joint_terminal.hpp"
#include "resnet50/engine/joint_floor.hpp"
#include "resnet50/engine/floor_terminal.hpp"
#include <openssl/crypto.h>
extern "C" int aqcf_seed221_prepare2(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_prepare3(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_read_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*);
extern "C" int aqcf_joint304_prepare3(aqcf_silent_handle*,int,uint8_t*,int,unsigned,uint64_t*);
extern "C" int aqcf_joint304_consume(aqcf_silent_handle*,uint64_t,const uint8_t*,const uint8_t*,const uint8_t*,uint8_t*,uint64_t*,int);
extern "C" int aqcf_joint304_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*,uint64_t*);
namespace aqcf_joint304 {using namespace aqcf2;
struct Carries{std::vector<Bits> value;U packet_rows=0,packet_table_bits=0,packet_calls=0,selector_rows=0;};
struct Tap {std::vector<U> value;~Tap(){if(!value.empty())OPENSSL_cleanse(value.data(),value.size()*sizeof(U));}};

struct Private {
 std::vector<std::vector<uint8_t>> alpha;std::vector<uint8_t> leaf,final,digit,value;
 explicit Private(size_t n):alpha(11,std::vector<uint8_t>(n)),leaf(n),final(n),digit(n),value(n){}
 ~Private(){for(auto& v:alpha)if(!v.empty())OPENSSL_cleanse(v.data(),v.size());for(auto* v:{&leaf,&final,&digit,&value})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
};
template<class B>Carries prefix(B& s,const std::vector<U>& own,const std::vector<U>& lo,Bits& tapped,std::vector<U>& arithmetic,unsigned w){
 const bool joint=aqcf_terminal304::enabled();if(joint)arithmetic.resize(own.size());
 const size_t n=own.size();require(n>0&&n<=200704&&lo.size()==n&&(w==25||w==26||w==37),"221 fixed full32 scope and mask ownership");
 Private p(n);std::vector<uint64_t> ids(11);constexpr unsigned widths[11]={3,3,2,3,3,3,3,3,3,3,3};uint64_t leaf_id=0;auto* h=s.control().owned189_handle();
 auto check=[&](int rc){require(rc==0,aqcf_silent_error(h));};
 if(s.party==1){{auto r=s.random_small(n);for(size_t i=0;i<n;++i)p.final[i]=r[i]&1;OPENSSL_cleanse(r.data(),r.size());}}
 s.boundary_profile("mixed237_paid_leaf_and_core_preparation",[&]{
  check(aqcf_seed221_prepare3(h,direction359::sender(s,1),s.party==1?p.leaf.data():nullptr,int(n),&leaf_id));
  for(unsigned j=0;j<11;++j){
   if(joint&&j==3)check(aqcf_joint304_prepare3(h,direction359::sender(s,1),s.party==1?p.alpha[j].data():nullptr,int(n),w,&ids[j]));
   else if(widths[j]==2)check(aqcf_seed221_prepare2(h,direction359::sender(s,1),s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
   else check(aqcf_seed221_prepare3(h,direction359::sender(s,1),s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
  }return 0;
 });
 std::vector<U> choice(n),table(s.party==1?8*n:0);
 for(size_t i=0;i<n;++i){require(own[i]<=mask(37)&&lo[i]<4,"221 normalized Ring37/quarter domain");
  U u=direction359::physical(s)==1?(lo[i]==0?0:lo[i]==3?2:3):((lo[i]+1)&3);
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
   std::vector<uint8_t> next;
   if(joint&&j==3){next.resize(n);check(aqcf_joint304_consume(h,ids[j],p.digit.data(),p.value.data(),s.party==1?p.alpha[j+1].data():nullptr,next.data(),arithmetic.data(),int(n)));}
   else next=widths[j]==2?s.control().quotient169_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final):s.control().quotient197_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final);
   OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);offset+=widths[j];if(offset==12)tapped.v=p.value;
  }require(offset==36,"237 all full signed bits retained");return 0;
 });
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 Carries out;out.value.emplace_back(n);out.value[0].v=p.value;
 out.packet_rows=12*n;out.packet_table_bits=92*n;out.packet_calls=12;out.selector_rows=0;
 tapped.check();out.value[0].check();return out;
}
template<class B>Ring consume(B& s,const std::vector<U>& phase,const std::vector<U>* add=nullptr){
 const size_t n=phase.size();require(n>0&&n<=200704&&s.compact_cofactor&&(!add||add->size()==n),"221 original full consumer");
 require(aqcf_terminal304::enabled(),"315 exact joint304 budget requires retained307 route");
 phase315::Phase paid_phase(s.control().owned189_handle(),int(n),!add,s.party);
 std::vector<U> own(n),lo(n);Ring coarse(25,n);
 for(size_t i=0;i<n;++i){require(phase[i]<(U(1)<<39),"221 exact Ring39 phase");lo[i]=phase[i]&3;U hi=phase[i]>>2;
  own[i]=(direction359::physical(s)==1?U(0)-hi-U(lo[i]!=0):hi+U(lo[i]==3))&mask(37);
  if(add){require((*add)[i]<=mask(37),"221 Ring37 residual");own[i]=(own[i]+(*add)[i])&mask(37);}coarse.v[i]=own[i]>>12;
 }
 Bits low;Tap tap;auto rest=prefix(s,own,lo,low,tap.value,add?37:25);
 s.note_linked_execution(n,n,rest.packet_rows+n,rest.packet_table_bits+8*n,rest.selector_rows);
 if(!aqcf_terminal304::enabled())return add?floor216::emit(s,coarse,low,rest.value[0]):aqcf_terminal249::emit(s,coarse,low,rest.value[0]);
 if(add)return floor304::emit(s,coarse,low,rest.value[0],tap.value);
 auto small=aqcf_terminal304::emit25(s,coarse,low,rest.value[0],tap.value);std::vector<U> lc(n),lt(s.party==1?2*n:0);
 for(size_t j=0;j<n;++j){U top=small.v[j]>>24;lc[j]=top;if(s.party==1)for(U c=0;c<2;++c)lt[2*j+c]=(U(0)-(top|c))&mask(12);}
 auto high=aqcf_terminal249::lookup(s,lt,lc,1,12,0);Ring out(37,n);for(size_t j=0;j<n;++j)out.v[j]=(small.v[j]+(high[j]<<25))&mask(37);
 for(auto* v:{&small.v,&lt,&high})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));out.check();return out;
}
}
