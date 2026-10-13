#include "squeezenet/core/squeezenet_joint_terminal.hpp"
#include "squeezenet/core/squeezenet_prefix_tree.hpp"
#pragma once
#include <array>
#include <cstdlib>
#include <cstring>
#include <openssl/crypto.h>
extern "C" int aqcf_seed221_prepare2(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_prepare3(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_seed221_read_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*);
extern "C" int aqcf_elide249_lookup(aqcf_silent_handle*,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
extern "C" int aqcf_elide249_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*,uint64_t*,uint64_t*);
extern "C" int aqcf_joint302_prepare3(aqcf_silent_handle*,int,uint8_t*,int,uint64_t*);
extern "C" int aqcf_joint302_consume(aqcf_silent_handle*,uint64_t,const uint8_t*,const uint8_t*,const uint8_t*,uint8_t*,uint64_t*,int);
extern "C" int aqcf_joint302_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*,uint64_t*);
namespace aqcf_sq266 {
using namespace aqcf2;
inline bool enabled(){const char* p=std::getenv("AQCF_SQ266");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"266 explicit fixed arm");return p[0]=='1';}
struct Prepared{std::vector<U> own37;Bits low,high;std::vector<U> tap302;~Prepared(){if(!tap302.empty())OPENSSL_cleanse(tap302.data(),tap302.size()*sizeof(U));}};
struct Private{
 std::vector<std::vector<uint8_t>> alpha;std::vector<uint8_t> leaf,final,digit,value;
 explicit Private(size_t n):alpha(11,std::vector<uint8_t>(n)),leaf(n),final(n),digit(n),value(n){}
 ~Private(){for(auto& v:alpha)if(!v.empty())OPENSSL_cleanse(v.data(),v.size());for(auto* v:{&leaf,&final,&digit,&value})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
};
template<class B> Prepared serial_prepare(B& s,const std::vector<U>& phase){
 const size_t n=phase.size();require(n&&n<=262144&&s.compact_cofactor,"266 original SQ frontier");
 const bool joint=aqcf_terminal302::enabled();Prepared out;if(joint)out.tap302.resize(n);out.own37.resize(n);std::vector<U> lo(n);
 for(size_t i=0;i<n;++i){require(phase[i]<(U(1)<<57),"266 ORIGINAL Ring57 phase");lo[i]=phase[i]&3;U hi=phase[i]>>2;
  out.own37[i]=(s.party==1?U(0)-hi-U(lo[i]!=0):hi+U(lo[i]==3))&mask(37);
 }
 Private p(n);std::array<uint64_t,11> ids{};constexpr unsigned widths[11]={3,3,2,3,3,3,3,3,3,3,3};uint64_t leaf_id=0;auto* h=s.control().owned189_handle();
 auto check=[&](int rc){require(rc==0,aqcf_silent_error(h));};
 if(s.party==1){auto r=s.random_small(n);for(size_t i=0;i<n;++i)p.final[i]=r[i]&1;OPENSSL_cleanse(r.data(),r.size());}
 s.boundary_profile("sq266_paid_leaf_and_core_preparation",[&]{
  check(aqcf_seed221_prepare3(h,1,s.party==1?p.leaf.data():nullptr,int(n),&leaf_id));
  for(unsigned j=0;j<11;++j)if(widths[j]==2)check(aqcf_seed221_prepare2(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
   else if(joint&&j==3)check(aqcf_joint302_prepare3(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));
   else check(aqcf_seed221_prepare3(h,1,s.party==1?p.alpha[j].data():nullptr,int(n),&ids[j]));return 0;
 });
 std::vector<U> choice(n),table(s.party==1?8*n:0);
 for(size_t i=0;i<n;++i){U u=s.party==1?(lo[i]==0?0:lo[i]==3?2:3):((lo[i]+1)&3);U x=((out.own37[i]&1)<<2)|u;choice[i]=x;
  if(s.party==1)for(unsigned y=0;y<8;++y)table[8*i+y]=((x+y)>>3)^p.leaf[i];
 }
 auto got=s.lookup_words(table,choice,n,3,1,false);s.flush();
 for(size_t i=0;i<n;++i){p.value[i]=s.party==1?p.leaf[i]:uint8_t(got[i]);p.digit[i]=(out.own37[i]>>1)&7;}
 auto next=s.control().quotient197_consume(leaf_id,p.digit,p.value,p.alpha[0]);OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);
 s.boundary_profile("sq266_eleven_core_exchanges",[&]{unsigned offset=4;
  for(unsigned j=0;j<11;++j){for(size_t i=0;i<n;++i)p.digit[i]=(out.own37[i]>>offset)&((1u<<widths[j])-1);
   std::vector<uint8_t> next;
   if(joint&&j==3){next.resize(n);check(aqcf_joint302_consume(h,ids[j],p.digit.data(),p.value.data(),s.party==1?p.alpha[j+1].data():nullptr,next.data(),out.tap302.data(),int(n)));}
   else next=widths[j]==2?s.control().quotient169_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final):s.control().quotient197_consume(ids[j],p.digit,p.value,j<10?p.alpha[j+1]:p.final);
   OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);offset+=widths[j];if(offset==12)out.low.v=p.value;
  }require(offset==36,"266 full signed bits retained");return 0;
 });
 out.high.v=p.value;out.low.check();out.high.check();s.note_linked_execution(n,n,13*n,100*n,0);
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));return out;
}
template<class B> Prepared prepare(B& s,const std::vector<U>& phase){
 if(aqcf_tree280::enabled()&&phase.size()<=8192){auto p=aqcf_tree280::prepare(s,phase);return {std::move(p.own37),std::move(p.low),std::move(p.high)};}
 return serial_prepare(s,phase);
}
template<class B>std::vector<U> lookup(B& s,const std::vector<U>& table,const std::vector<U>& choice,unsigned ib,unsigned width,unsigned low){
 const size_t n=choice.size();require(n&&n<=262144,"266 SQ terminal frontier");std::vector<uint8_t> select(s.party==2?n:0);
 if(s.party==2)for(size_t i=0;i<n;++i){require(choice[i]<(1u<<ib),"266 choice");select[i]=choice[i];}
 std::vector<U> out(n);auto* h=s.control().owned189_handle();require(aqcf_elide249_lookup(h,s.party==1?table.data():nullptr,s.party==2?select.data():nullptr,out.data(),int(n),ib,width,low)==0,aqcf_silent_error(h));s.flush();return out;
}
template<class B>Ring emit25(B& s,const Ring& a,const Bits& low,const Bits& high,const std::vector<U>* tap=nullptr){
 if(tap&&!tap->empty()){require(aqcf_terminal302::enabled(),"302 unexpected tap in control");return aqcf_terminal302::emit25(s,a,low,high,*tap);}
 const size_t n=a.v.size();a.check();low.check();high.check();require(a.width==25&&n&&n<=262144&&low.v.size()==n&&high.v.size()==n,"266 exact terminal");
 std::vector<U> choice(n),table(s.party==1?4*n:0);
 for(size_t i=0;i<n;++i){U gs=(a.v[i]>>24)^high.v[i]^U(s.party==1);choice[i]=gs|(U(low.v[i])<<1);
  if(s.party==1)for(U c=0;c<4;++c){U g=gs^(c&1),d=U(low.v[i])^(c>>1);table[4*i+c]=(((g*(a.v[i]+d))&mask(25))<<1)|g;}}
 auto got=lookup(s,table,choice,2,26,1);Ring out(25,n);std::vector<U> coefficient(n);std::vector<uint8_t> selector(n);
 for(size_t i=0;i<n;++i){U g=got[i]&1;if(s.party==1){out.v[i]=got[i]>>1;selector[i]=g;}else{out.v[i]=((got[i]>>1)+a.v[i]*g)&mask(25);coefficient[i]=(a.v[i]*(U(1)-2*g))&mask(25);}}
 auto product=s.owned_word_bit_product(2,coefficient,selector,25);for(size_t i=0;i<n;++i)out.v[i]=(out.v[i]+product.v[i])&mask(25);
 for(auto* v:{&table,&got,&coefficient,&product.v})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));if(!selector.empty())OPENSSL_cleanse(selector.data(),selector.size());out.check();return out;
}
template<class B>Ring emit37(B& s,const Ring& a,const Bits& low,const Bits& high,const std::vector<U>* tap=nullptr){
 auto small=emit25(s,a,low,high,tap);const size_t n=small.v.size();std::vector<U> c(n),t(s.party==1?2*n:0);
 for(size_t i=0;i<n;++i){U top=small.v[i]>>24;c[i]=top;if(s.party==1)for(U v=0;v<2;++v)t[2*i+v]=(U(0)-(top|v))&mask(12);}
 auto hi=lookup(s,t,c,1,12,0);Ring out(37,n);for(size_t i=0;i<n;++i)out.v[i]=(small.v[i]+(hi[i]<<25))&mask(37);
 for(auto* v:{&small.v,&t,&hi})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));out.check();return out;
}
}
