#include "resnet50/engine/directed_phase.hpp"
#pragma once
#include "resnet50/engine/nonnegative_terminal.hpp"
extern "C" int aqcf_elide249_lookup(aqcf_silent_handle*,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
extern "C" int aqcf_elide249_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*,uint64_t*,uint64_t*);
namespace aqcf_terminal249 {
using namespace aqcf2;
template<class B> std::vector<U> lookup(B& s,const std::vector<U>& table,const std::vector<U>& choice,unsigned ib,unsigned width,unsigned low){
 const auto n=choice.size();require(n&&n<=200704,"249 terminal row bound");
 std::vector<uint8_t> select(s.party==2?n:0);if(s.party==2)for(size_t j=0;j<n;++j){require(choice[j]<(1u<<ib),"249 selection");select[j]=choice[j];}
 std::vector<U> out(n);auto* h=s.control().owned189_handle();
 require(direction359::aqcf_elide359_lookup(h,direction359::sender(s,1),s.party==1?table.data():nullptr,s.party==2?select.data():nullptr,out.data(),int(n),ib,width,low)==0,aqcf_silent_error(h));
 ++s.lookup_calls;s.lookup_rows+=n;s.lookup_table_bits+=n*((1u<<ib)-1)*width;s.flush();return out;
}
template<class B> Ring emit(B& s,const Ring& a,const Bits& low,const Bits& carry){
 a.check();low.check();carry.check();const auto n=a.v.size();constexpr unsigned w=25,hi=12;
 require(a.width==w&&n&&n<=200704&&low.v.size()==n&&carry.v.size()==n,"249 unchanged full signed floor12 terminal");
 std::vector<U> choice(n),table(s.party==1?4*n:0);
 for(size_t j=0;j<n;++j){const U gs=(a.v[j]>>24)^carry.v[j]^U(s.party==1);choice[j]=gs|(U(low.v[j])<<1);
  if(s.party==1)for(U c=0;c<4;++c){const U g=gs^(c&1),d=U(low.v[j])^(c>>1);table[4*j+c]=(((g*(a.v[j]+d))&mask(w))<<1)|g;}
 }
 auto got=lookup(s,table,choice,2,26,1);
 Ring small(w,n);std::vector<U> coefficient(n);std::vector<uint8_t> selector(n);
 for(size_t j=0;j<n;++j){const U g=got[j]&1;
  if(s.party==1){small.v[j]=got[j]>>1;selector[j]=g;}
  else{small.v[j]=((got[j]>>1)+a.v[j]*g)&mask(w);coefficient[j]=(a.v[j]*(U(1)-2*g))&mask(w);}
 }
 auto product=s.owned_word_bit_product(2,coefficient,selector,w);
 for(size_t j=0;j<n;++j)small.v[j]=(small.v[j]+product.v[j])&mask(w);
 std::vector<U> lc(n),lt(s.party==1?2*n:0);
 for(size_t j=0;j<n;++j){const U top=small.v[j]>>24;lc[j]=top;if(s.party==1)for(U c=0;c<2;++c)lt[2*j+c]=(U(0)-(top|c))&mask(hi);}
 auto high=lookup(s,lt,lc,1,12,0);Ring out(37,n);
 for(size_t j=0;j<n;++j)out.v[j]=(small.v[j]+(high[j]<<w))&mask(37);
 for(auto* v:{&table,&got,&small.v,&coefficient,&product.v,&lt,&high})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 if(!selector.empty())OPENSSL_cleanse(selector.data(),selector.size());out.check();return out;
}
}
