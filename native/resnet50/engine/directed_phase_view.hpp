#pragma once
#include "resnet50/core/sci.hpp"
#include "resnet50/engine/directed_phase.hpp"
namespace direction359 {
class Control {
 aqcf_silent_control::Provider& p;const int logical;
 void check(int rc){aqcf2::require(rc==0,aqcf_silent_error(p.owned189_handle()));}
public:
 Control(aqcf_silent_control::Provider&v,int l):p(v),logical(l){}
 auto owned189_handle(){return p.owned189_handle();}
 std::vector<uint8_t> quotient169_consume(uint64_t id,const std::vector<uint8_t>&d,const std::vector<uint8_t>&c,const std::vector<uint8_t>&m){
  aqcf2::require(d.size()==c.size()&&m.size()==d.size(),"359 bank vector sizes");std::vector<uint8_t>o(d.size());
  check(aqcf_quotient169_consume(owned189_handle(),id,d.data(),c.data(),logical==1?m.data():nullptr,o.data(),int(o.size())));return o;
 }
 std::vector<uint8_t> quotient197_consume(uint64_t id,const std::vector<uint8_t>&d,const std::vector<uint8_t>&c,const std::vector<uint8_t>&m){
  aqcf2::require(d.size()==c.size()&&m.size()==d.size(),"359 bank vector sizes");std::vector<uint8_t>o(d.size());
  check(aqcf_quotient197_consume(owned189_handle(),id,d.data(),c.data(),logical==1?m.data():nullptr,o.data(),int(o.size())));return o;
 }
};
class View {
 aqcf2::Sci& original;Control ctrl;
public:
 const int physical_party,party;const bool reversed,compact_cofactor;
 decltype(aqcf2::Sci::lookup_calls)&lookup_calls;
 decltype(aqcf2::Sci::lookup_rows)&lookup_rows;
 decltype(aqcf2::Sci::lookup_table_bits)&lookup_table_bits;
 View(aqcf2::Sci&s,bool rev):original(s),ctrl(s.control(),rev?3-s.party:s.party),physical_party(s.party),party(rev?3-s.party:s.party),reversed(rev),compact_cofactor(s.compact_cofactor),lookup_calls(s.lookup_calls),lookup_rows(s.lookup_rows),lookup_table_bits(s.lookup_table_bits){}
 int map_sender(int d)const{aqcf2::require(d==1||d==2,"359 sender");return reversed?3-d:d;}
 auto&control(){return ctrl;}
 void flush(){original.flush();}
 auto random_small(size_t n){return original.random_small(n);}
 template<class F>auto boundary_profile(const std::string&n,F&&f){return original.boundary_profile(n,std::forward<F>(f));}
 void note_linked_execution(aqcf2::U a,aqcf2::U b,aqcf2::U c,aqcf2::U d,aqcf2::U e){original.note_linked_execution(a,b,c,d,e);}
 std::vector<aqcf2::U>lookup_words(const std::vector<aqcf2::U>&t,const std::vector<aqcf2::U>&c,size_t n,unsigned ib,unsigned ob,bool compact){
  auto z=original.control().lookup(t,c,n,ib,ob,compact,map_sender(1));
  ++lookup_calls;lookup_rows+=n;lookup_table_bits+=n*(aqcf2::U(1)<<ib)*ob;
  if(compact){++original.compact_response_calls;original.compact_response_rows+=n;}return z;
 }
 auto owned_word_bit_product(int sender,const std::vector<aqcf2::U>&c,const std::vector<uint8_t>&b,unsigned w){return original.owned_word_bit_product(map_sender(sender),c,b,w);}
};
struct Census {uint64_t calls=0,rows=0,reversed_calls=0,reversed_rows=0;};
inline thread_local Census census;
}
