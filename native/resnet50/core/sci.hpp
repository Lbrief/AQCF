#pragma once
#include "resnet50/core/types.hpp"
#ifndef AQCF_SILENTOT_CONTROL
#define AQCF_SILENTOT_CONTROL 0
#endif
#if AQCF_SILENTOT_CONTROL
#include "resnet50/protocol/backend.hpp"
#endif
#include "resnet50/core/small_state.hpp"
#include "resnet50/core/prepared_bits.hpp"
#include "resnet50/core/tuple_selector_bank.hpp"
#include "resnet50/core/native_late_ot.hpp"
#if (defined(AQCF_R17_PUBLIC_BUILD) || defined(AQCF_EXACT_QUARTER_BUILD)) || defined(AQCF_R17_REAL_N1)
#include "resnet50/core/quotient_extension.hpp"
#endif
#ifdef AQCF_R12_PUBLIC_RESPONSE_BUILD
#include "native_response.hpp"
#endif
#include "resnet50/vendor/bolt_sci/BuildingBlocks/aux-protocols.h"
#include <array>
#include <chrono>
#include <climits>
#include <memory>
#include <map>
#include <string>
#include <openssl/rand.h>
namespace aqcf2 {
// Concrete SCI adapter. All objects are borrowed from ONE role's live session.
class Sci {
public:

 // Independent existing-technique provider; lazy setup remains in online cost.
 #if AQCF_SILENTOT_CONTROL
 std::unique_ptr<aqcf_silent_control::Provider> silent_control;
 aqcf_silent_control::Provider& control(){
  if(!silent_control)silent_control=std::make_unique<aqcf_silent_control::Provider>(party,io.io,io.io_rev);
  return *silent_control;
 }
 #endif
 void control_send_cot(int sender,U* out,U* corr,int n,int w){
 #if AQCF_SILENTOT_CONTROL
  require(party==sender,"control COT sender");control().cot(sender,out,corr,nullptr,n,w);
 #else
  auto* channel=sender==1?ot.iknp_straight:ot.iknp_reversed;channel->send_cot(out,corr,n,w);
 #endif
 }
 void control_recv_cot(int sender,U* out,bool* choices,int n,int w){
 #if AQCF_SILENTOT_CONTROL
  require(party!=sender,"control COT receiver");control().cot(sender,out,nullptr,choices,n,w);
 #else
  auto* channel=sender==1?ot.iknp_straight:ot.iknp_reversed;channel->recv_cot(out,choices,n,w);
 #endif
 }

#if (defined(AQCF_R17_PUBLIC_BUILD) || defined(AQCF_EXACT_QUARTER_BUILD)) || defined(AQCF_R17_REAL_N1)
 std::unique_ptr<aqcf17::Lookup> quotient_lookup;
 U quotient_seed_bytes=0,quotient_elided_bytes=0,quotient_rows=0;
 double quotient_extension_s=0,quotient_response_s=0;
 void configure_quotient(const std::string& mode){require(!quotient_lookup,"one extension configuration");quotient_lookup=std::make_unique<aqcf17::Lookup>(party,ot,mode);quotient_lookup->fast_response=true;}
#endif
 #ifdef AQCF_R12_PUBLIC_RESPONSE_BUILD
 std::unique_ptr<aqcf12::NativeResponse> r12_response;
 void configure_public_response(const std::string& name){require(!r12_response,"response session configured once");r12_response=std::make_unique<aqcf12::NativeResponse>(party,io,ot,name);}
 #endif

 U late_nco_rows=0,late_nco_cells=0,late_nco_calls=0;
 RandomNBitOT random_n_ot(std::size_t n,unsigned in){
   auto result=native_random_n_ot(party,ot,n,in);
   late_nco_rows+=n;late_nco_cells+=n*(U(1)<<in);++late_nco_calls;
   return result;
 }
 bool binary5=false; bool compact_cofactor=false; U compact_response_calls=0,compact_response_rows=0;
 U tuple_selector_elements=0;
 U linked_low_elements=0,linked_high_elements=0,linked_packet_rows=0,linked_packet_bits=0,linked_selector_rows=0;
 U prepared_control_elements=0,random_bit_OT_rows=0,masked_bit_messages=0,masked_bit_payload_sent=0;
 U token_low_elements=0,token_high_elements=0,token_transitions=0,token_ot_rows=0,token_table_bits=0,token_calls=0;
 U materialized_low_elements=0,materialized_high_elements=0,materialized_and_elements=0;
 void note_carry_execution(bool token,U low,U high,U transitions,U rows,U bits,U calls,U ands){
  if(token){require(ands==0,"token path cannot hide materialized ANDs");token_low_elements+=low;token_high_elements+=high;
    token_transitions+=transitions;token_ot_rows+=rows;token_table_bits+=bits;token_calls+=calls;}
  else{materialized_low_elements+=low;materialized_high_elements+=high;materialized_and_elements+=ands;}
 }
 U owned_cot_rows=0,owned_cot_payload_bits=0;
 std::map<std::string,double> boundary_seconds;
 std::map<std::string,U> boundary_bytes;
 int party;sci::IOPack& io;sci::OTPack& ot;AuxProtocols aux;
 double decision_seconds=0;U decision_bytes=0;
 U and_elements=0,b2a_elements=0,mux_elements=0,compare_elements=0,bitmul_elements=0,lookup_rows=0,lookup_calls=0,lookup_table_bits=0;
 Sci(int p,sci::IOPack&i,sci::OTPack&o):party(p),io(i),ot(o),aux(p,&i,&o){require(p==1||p==2,"invalid role");}
 // Merge completed lane accounting only; never reset OT, masks, PRNG or I/O state.
 void absorb_accounting(Sci& v){
  compact_response_calls+=v.compact_response_calls;v.compact_response_calls=0;compact_response_rows+=v.compact_response_rows;v.compact_response_rows=0;
#if (defined(AQCF_R17_PUBLIC_BUILD) || defined(AQCF_EXACT_QUARTER_BUILD)) || defined(AQCF_R17_REAL_N1)
  quotient_seed_bytes+=v.quotient_seed_bytes;v.quotient_seed_bytes=0;
  quotient_elided_bytes+=v.quotient_elided_bytes;v.quotient_elided_bytes=0;
  quotient_rows+=v.quotient_rows;v.quotient_rows=0;
  quotient_extension_s+=v.quotient_extension_s;v.quotient_extension_s=0;
  quotient_response_s+=v.quotient_response_s;v.quotient_response_s=0;
#endif
  late_nco_rows+=v.late_nco_rows;v.late_nco_rows=0;
  late_nco_cells+=v.late_nco_cells;v.late_nco_cells=0;
  late_nco_calls+=v.late_nco_calls;v.late_nco_calls=0;
  tuple_selector_elements+=v.tuple_selector_elements;v.tuple_selector_elements=0;
  linked_low_elements+=v.linked_low_elements;v.linked_low_elements=0;
  linked_high_elements+=v.linked_high_elements;v.linked_high_elements=0;
  linked_packet_rows+=v.linked_packet_rows;v.linked_packet_rows=0;
  linked_packet_bits+=v.linked_packet_bits;v.linked_packet_bits=0;
  linked_selector_rows+=v.linked_selector_rows;v.linked_selector_rows=0;
  prepared_control_elements+=v.prepared_control_elements;v.prepared_control_elements=0;
  random_bit_OT_rows+=v.random_bit_OT_rows;v.random_bit_OT_rows=0;
  masked_bit_messages+=v.masked_bit_messages;v.masked_bit_messages=0;
  masked_bit_payload_sent+=v.masked_bit_payload_sent;v.masked_bit_payload_sent=0;
  token_low_elements+=v.token_low_elements;v.token_low_elements=0;
  token_high_elements+=v.token_high_elements;v.token_high_elements=0;
  token_transitions+=v.token_transitions;v.token_transitions=0;
  token_ot_rows+=v.token_ot_rows;v.token_ot_rows=0;
  token_table_bits+=v.token_table_bits;v.token_table_bits=0;
  token_calls+=v.token_calls;v.token_calls=0;
  materialized_low_elements+=v.materialized_low_elements;v.materialized_low_elements=0;
  materialized_high_elements+=v.materialized_high_elements;v.materialized_high_elements=0;
  materialized_and_elements+=v.materialized_and_elements;v.materialized_and_elements=0;
  owned_cot_rows+=v.owned_cot_rows;v.owned_cot_rows=0;
  owned_cot_payload_bits+=v.owned_cot_payload_bits;v.owned_cot_payload_bits=0;
  decision_bytes+=v.decision_bytes;v.decision_bytes=0;
  and_elements+=v.and_elements;v.and_elements=0;
  b2a_elements+=v.b2a_elements;v.b2a_elements=0;
  mux_elements+=v.mux_elements;v.mux_elements=0;
  compare_elements+=v.compare_elements;v.compare_elements=0;
  bitmul_elements+=v.bitmul_elements;v.bitmul_elements=0;
  lookup_rows+=v.lookup_rows;v.lookup_rows=0;
  lookup_calls+=v.lookup_calls;v.lookup_calls=0;
  lookup_table_bits+=v.lookup_table_bits;v.lookup_table_bits=0;
  decision_seconds+=v.decision_seconds;v.decision_seconds=0;
  for(auto [k,x]:v.boundary_seconds)boundary_seconds[k]+=x;
  for(auto [k,x]:v.boundary_bytes)boundary_bytes[k]+=x;
  v.boundary_seconds.clear();v.boundary_bytes.clear();
 }
 static int dim(std::size_t n){require(n>0&&n<=U(INT_MAX),"invalid public size");return int(n);}
 void flush(){io.io->flush();io.io_rev->flush();io.io_GC->flush();}
 U bytes()const{return io.io->counter+io.io_rev->counter+io.io_GC->counter;}
 static U uniform(U m){U v,threshold=(U(0)-m)%m;do{require(RAND_bytes(reinterpret_cast<unsigned char*>(&v),8)==1,"CSPRNG failed");}while(v<threshold);return v%m;}
 Bits constant(bool v,std::size_t n)const{Bits b(n);if(party==1&&v)std::fill(b.v.begin(),b.v.end(),1);return b;}
 Bits local_bit(const std::vector<U>&x,unsigned bit)const{Bits b(x.size());for(std::size_t j=0;j<x.size();++j)b.v[j]=(x[j]>>bit)&1;return b;}
 Bits xor_(const Bits&a,const Bits&b)const{require(a.v.size()==b.v.size(),"XOR shape");a.check();b.check();Bits c(a.v.size());for(std::size_t j=0;j<c.v.size();++j)c.v[j]=a.v[j]^b.v[j];return c;}
 Bits not_(const Bits&a)const{return xor_(a,constant(true,a.v.size()));}
 Bits and_(const Bits&a,const Bits&b){require(a.v.size()==b.v.size(),"AND shape");a.check();b.check();auto av=a.v,bv=b.v;Bits c(av.size());aux.AND(av.data(),bv.data(),c.v.data(),dim(av.size()));and_elements+=av.size();c.check();return c;}
 std::vector<Bits> and_batch(const std::vector<std::pair<Bits,Bits>>& p){
  require(!p.empty(),"empty AND batch");std::size_t n=p[0].first.v.size();Bits a(n*p.size()),b(a.v.size());
  for(std::size_t j=0;j<p.size();++j){require(p[j].first.v.size()==n&&p[j].second.v.size()==n,"AND batch shape");std::copy(p[j].first.v.begin(),p[j].first.v.end(),a.v.begin()+j*n);std::copy(p[j].second.v.begin(),p[j].second.v.end(),b.v.begin()+j*n);}
  auto c=and_(a,b);std::vector<Bits> out;for(std::size_t j=0;j<p.size();++j){Bits x(n);std::copy_n(c.v.begin()+j*n,n,x.v.begin());out.push_back(std::move(x));}return out;
 }
 Bits or_(const Bits&a,const Bits&b){return xor_(xor_(a,b),and_(a,b));}
 Ring b2a(const Bits&b,unsigned w){b.check();auto v=b.v;Ring a(w,v.size());aux.B2A(v.data(),a.v.data(),dim(v.size()),int(w));b2a_elements+=v.size();a.check();return a;}
 std::pair<Bits,Bits> carry_eq(const std::vector<U>&x,unsigned w){
  if(binary5){compare_elements+=x.size();return binary5_carry_eq(*this,x,w);}
   auto v=x;U mm=mask(w);for(U&a:v){a&=mm;if(party==2)a=mm-a;}Bits g(v.size()),e(v.size());
  aux.mill_and_eq->compare_with_eq(g.v.data(),e.v.data(),v.data(),dim(v.size()),int(w),true,4);compare_elements+=v.size();g.check();e.check();return {g,e};
 }
 Bits msb(const Ring&a){a.check();if(binary5){auto ge=binary5_carry_eq(*this,a.v,a.width-1);compare_elements+=a.v.size();return xor_(ge.first,local_bit(a.v,a.width-1));}auto v=a.v;Bits b(v.size());aux.MSB(v.data(),b.v.data(),dim(v.size()),int(a.width));compare_elements+=v.size();b.check();return b;}
 Ring mux(const Bits&g,const Ring&a){
  a.check();g.check();require(g.v.size()==a.v.size(),"MUX shape");int n=dim(a.v.size());U mm=mask(a.width);Ring z(a.width,n);std::vector<U> corr(n),rs(n),rr(n);std::unique_ptr<bool[]>choice(new bool[n]);
  for(int j=0;j<n;++j){choice[j]=bool(g.v[j]);corr[j]=(a.v[j]*(U(1)-2*U(g.v[j])))&mm;}
  // Explicitly execute BOTH complementary COT directions, even with OMP=1.
  if(party==1)control_send_cot(1,rs.data(),corr.data(),n,int(a.width));else control_recv_cot(1,rr.data(),choice.get(),n,int(a.width));flush();
  if(party==2)control_send_cot(2,rs.data(),corr.data(),n,int(a.width));else control_recv_cot(2,rr.data(),choice.get(),n,int(a.width));flush();
  for(int j=0;j<n;++j){z.v[j]=(a.v[j]*g.v[j]+rr[j]-rs[j])&mm;}
  mux_elements+=n;return z;
 }
 // Product of ALICE's PRIVATE bit and BOB's PRIVATE bit, arithmetic output.
 // This is mature BitMul; it replaces AND followed by B2A in range corrections.
 Ring bit_product(const std::vector<std::uint8_t>&own,unsigned w){
  int n=dim(own.size());Ring z(w,n);std::vector<U>corr(n),r(n);std::unique_ptr<bool[]>ch(new bool[n]);
  for(int j=0;j<n;++j){require(own[j]<2,"private predicate bit");corr[j]=own[j];ch[j]=bool(own[j]);}
  if(party==1){control_send_cot(1,r.data(),corr.data(),n,int(w));for(int j=0;j<n;++j)z.v[j]=(U(0)-r[j])&mask(w);}
  else{control_recv_cot(1,r.data(),ch.get(),n,int(w));for(int j=0;j<n;++j)z.v[j]=r[j]&mask(w);}flush();bitmul_elements+=n;return z;
 }
 std::vector<U> got(int sender,std::vector<std::array<U,2>> rows,const std::vector<std::uint8_t>&choices,std::size_t n,unsigned w){
 #if AQCF_SILENTOT_CONTROL
  std::vector<U> flat,selected(n);
  if(party==sender){require(rows.size()==n,"GOT sender shape");flat.reserve(2*n);
   for(const auto& row:rows){flat.push_back(row[0]);flat.push_back(row[1]);}}
  else{require(choices.size()==n,"GOT receiver shape");std::copy(choices.begin(),choices.end(),selected.begin());}
  auto result=control().lookup(flat,selected,n,1,w,false,sender);flush();return result;
 #endif
  auto* t=sender==1?ot.iknp_straight:ot.iknp_reversed;std::vector<U>out(n);
  if(party==sender){require(rows.size()==n,"GOT sender shape");std::vector<U*>ptr(n);for(std::size_t j=0;j<n;++j)ptr[j]=rows[j].data();t->send(ptr.data(),dim(n),int(w));}
  else{require(choices.size()==n,"GOT receiver shape");for(auto c:choices)require(c<2,"GOT choice");auto mutable_choices=choices;t->recv(out.data(),mutable_choices.data(),dim(n),int(w));}flush();return out;
 }
 template<class Function> auto boundary_profile(const std::string& name,Function&& fn){
  flush();auto started=std::chrono::steady_clock::now();U b=bytes();auto result=fn();flush();
  boundary_seconds[name]+=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
  boundary_bytes[name]+=bytes()-b;return result;
 }
 template<class Function> auto decision_profile(Function&& f){flush();auto t=std::chrono::steady_clock::now();U b=bytes();auto x=f();flush();decision_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();decision_bytes+=bytes()-b;return x;}
 std::vector<std::uint8_t> random_small(std::size_t n){std::vector<std::uint8_t>v(n);require(RAND_bytes(v.data(),dim(n))==1,"leaf CSPRNG failed");for(auto&x:v)x&=3;return v;}
 std::vector<U> lookup_words(std::vector<U>&table,const std::vector<U>&choice,std::size_t n,unsigned in,unsigned out,bool compact=false){
 #if AQCF_SILENTOT_CONTROL
  {auto v=control().lookup(table,choice,n,in,out,compact);
  if(compact){++compact_response_calls;compact_response_rows+=n;}
  ++lookup_calls;lookup_rows+=n;lookup_table_bits+=n*(U(1)<<in)*out;return v;}
 #endif
#if (defined(AQCF_R17_PUBLIC_BUILD) || defined(AQCF_EXACT_QUARTER_BUILD)) || defined(AQCF_R17_REAL_N1)
  if(quotient_lookup&&in>=2&&in<=5){auto& q=*quotient_lookup;
   const auto sb=q.seed_bytes(),eb=q.elided_bytes();const auto es=q.extension_s,rs=q.response_s;
   auto result=q.execute(table,choice,n,in,out,compact);if(compact){++compact_response_calls;compact_response_rows+=n;}
   quotient_seed_bytes+=q.seed_bytes()-sb;quotient_elided_bytes+=q.elided_bytes()-eb;
   quotient_rows+=n;quotient_extension_s+=q.extension_s-es;quotient_response_s+=q.response_s-rs;
   ++lookup_calls;lookup_rows+=n;lookup_table_bits+=n*(U(1)<<in)*out;return result;}
#endif
 #ifdef AQCF_R12_PUBLIC_RESPONSE_BUILD
  if(r12_response&&in>=2&&in<=5&&out<=2){auto v=r12_response->lookup(table,choice,n,in,out);++lookup_calls;lookup_rows+=n;lookup_table_bits+=n*(U(1)<<in)*out;return v;}
 #endif
  require(!compact,"compact response requires pinned quotient backend");
  require(in>=1&&in<=8&&out<=64&&choice.size()==n,"lookup ABI");std::vector<U>v(n);std::vector<U*>ptr;auto c=choice;
  if(party==1){require(table.size()==n*(U(1)<<in),"lookup table size");ptr.resize(n);for(std::size_t j=0;j<n;++j)ptr[j]=table.data()+j*(U(1)<<in);aux.lookup_table<U>(ptr.data(),nullptr,nullptr,dim(n),int(in),int(out));}
  else aux.lookup_table<U>(nullptr,c.data(),v.data(),dim(n),int(in),int(out));
  ++lookup_calls;lookup_rows+=n;lookup_table_bits+=n*(U(1)<<in)*out;return v;
 }
 // R6 uses the same pinned IKNP API as the existing MUX and private scale.
 // Only 'sender' owns coefficient; only the opposite role owns selector.
 Ring owned_word_bit_product(int sender,const std::vector<U>& coefficient,
                             const std::vector<std::uint8_t>& selector,unsigned w){
  require((sender==1||sender==2)&&coefficient.size()==selector.size(),"owned COT shape");
  const int n=dim(coefficient.size());Ring z(w,n);std::vector<U> r(n),corr=coefficient;
  std::unique_ptr<bool[]> choices(new bool[n]);
  for(int j=0;j<n;++j){require(selector[j]<2 && coefficient[j]<=mask(w),"owned COT inputs");choices[j]=bool(selector[j]);}
  auto* channel=sender==1?ot.iknp_straight:ot.iknp_reversed;
  if(party==sender){control_send_cot(sender,r.data(),corr.data(),n,int(w));for(int j=0;j<n;++j)z.v[j]=(U(0)-r[j])&mask(w);}
  else{control_recv_cot(sender,r.data(),choices.get(),n,int(w));for(int j=0;j<n;++j)z.v[j]=r[j]&mask(w);}
  flush();owned_cot_rows+=n;owned_cot_payload_bits+=U(n)*w;return z;
 }
 std::vector<U> random_ring_words(std::size_t n,unsigned w){
  require(w<=60 && n<=U(INT_MAX)/8,"bounded bulk CSPRNG");std::vector<U> a(n);
  require(RAND_bytes(reinterpret_cast<unsigned char*>(a.data()),int(n*8))==1,"terminal CSPRNG failed");
  for(auto&v:a)v&=mask(w);return a;
 }

  RandomBitOT random_ot_bits(std::size_t n){
   // Existing native random-message/random-choice OT. Same one-use interface.
   // Unchanged tuple implementation below remains provenance, not executed.
   {
    require(n<=(1U<<20),"native ROT bound");
    RandomBitOT out(n);if(!n)return out;
    control().rot_bits(1,out.k0.data(),out.k1.data(),out.t.data(),out.kt.data(),int(n));
    flush();random_bit_OT_rows+=n;return out;
   }

#if AQCF_TUPLE_SELECTORS
   tuple_selector_elements+=n;
   return tuple_selector_bank(*this,n,AQCF_TUPLE_SELECTORS);
#endif
   RandomBitOT out(n);if(!n)return out;
   const int count=dim(n);std::vector<U> corr(n),value(n);auto random=random_small(n);
   std::unique_ptr<bool[]> choice(new bool[n]);
   for(std::size_t j=0;j<n;++j){corr[j]=random[j]&1;choice[j]=bool(random[j]&1);if(party==2)out.t[j]=choice[j];}
   // COT modulo TWO with independently uniform correlation yields an ROT pair.
   if(party==1){control_send_cot(1,value.data(),corr.data(),count,1);
    for(std::size_t j=0;j<n;++j){out.k0[j]=value[j]&1;out.k1[j]=(value[j]+corr[j])&1;}}
   else{control_recv_cot(1,value.data(),choice.get(),count,1);for(std::size_t j=0;j<n;++j)out.kt[j]=value[j]&1;}
   flush();random_bit_OT_rows+=n;return out;
  }
  std::vector<std::uint8_t> transfer_masked_bits(int sender,const std::vector<std::uint8_t>& own){
   require((sender==1||sender==2)&&!own.empty(),"masked-bit transport public shape");
   const std::size_t n=own.size(),bytes_count=(n+7)/8;std::vector<unsigned char> wire(bytes_count,0);
   if(party==sender){for(std::size_t j=0;j<n;++j){require(own[j]<2,"masked bit canonical");wire[j/8]|=own[j]<<(j%8);}
    io.io->send_data(wire.data(),dim(bytes_count));flush();masked_bit_payload_sent+=n;}
   else{io.io->recv_data(wire.data(),dim(bytes_count));flush();}
   if(n%8)require((wire.back()>>(n%8))==0,"masked-bit zero padding");
   std::vector<std::uint8_t> result(n);for(std::size_t j=0;j<n;++j)result[j]=(wire[j/8]>>(j%8))&1;
   ++masked_bit_messages;return result;
  }
  void note_linked_execution(U low,U high,U packets,U bits,U selectors){
   linked_low_elements+=low;linked_high_elements+=high;linked_packet_rows+=packets;
   linked_packet_bits+=bits;linked_selector_rows+=selectors;
  }
 Field b2p(const Bits&b){
  b.check();std::size_t n=b.v.size();Field a(n);std::vector<std::array<U,2>>rows;
  if(party==1){rows.resize(n);for(std::size_t j=0;j<n;++j){a.v[j]=uniform(P);rows[j]={residue(W(b.v[j])-a.v[j],P),residue(W(b.v[j]^1)-a.v[j],P)};}}
  auto v=got(1,std::move(rows),b.v,n,60);if(party==2)a.v=std::move(v);b2a_elements+=n;a.check();return a;
 }
 Field mux(const Bits&g,const Field&a){
  a.check();g.check();require(g.v.size()==a.v.size(),"prime MUX shape");std::size_t n=a.v.size();Field z(n);
  for(int sender=1;sender<=2;++sender){std::vector<std::array<U,2>>rows;
   if(party==sender){rows.resize(n);for(std::size_t j=0;j<n;++j){U r=uniform(P);z.v[j]=residue(W(z.v[j])+r,P);rows[j]={residue(W(g.v[j])*a.v[j]-r,P),residue(W(g.v[j]^1)*a.v[j]-r,P)};}}
   auto v=got(sender,std::move(rows),g.v,n,60);if(party!=sender)for(std::size_t j=0;j<n;++j)z.v[j]=residue(W(z.v[j])+v[j],P);
  }mux_elements+=n;z.check();return z;
 }
};
}
