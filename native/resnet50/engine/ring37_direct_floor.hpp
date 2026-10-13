#pragma once
#include "resnet50/engine/ring37_direct.hpp"
namespace aqcf_cifar_comm {
// Exact signed Ring37 floor/2^12, directly returning uniform Ring37 shares.
// Uses the existing linked-prefix carries c12,c36 and existing 1-of-8 lookup.
// It does not widen to Ring55 and truncate again. No plaintext intermediates.
// Let h=a>>12 and t=a36+b36+c36. The signed correction is ceil(t/2)*2^25.
// Thus floor(center37(a+b)/2^12)=h_a+h_b+c12-ceil(t/2)*2^25.
template<class B> aqcf2::Ring floor37_direct(B& s,const aqcf2::Ring& in,unsigned f){
 using namespace aqcf2;
 require(in.width==55&&f==12&&!in.v.empty()&&in.v.size()<=(AQCF_R50_FRONTIER?65536:4096),"direct signed Ring37 floor domain");
 const size_t n=in.v.size();const U mm=mask(37),H=U(1)<<25;
 std::vector<U> own(n),coarse(n);
 for(size_t j=0;j<n;++j){own[j]=in.v[j]&mm;coarse[j]=own[j]>>12;}
 std::vector<CarryTask> tasks{{&own,36,s.constant(false,n),true}};Bits low;
 auto carries=linked_cofactor_prefix_execute(s,tasks,f,low);
 s.note_linked_execution(n,n,carries.packet_rows,carries.packet_table_bits,carries.selector_rows);
 std::vector<U> choice(n),table,r;
 if(s.party==1){r=s.random_ring_words(n,37);table.resize(n*8);}
 for(size_t j=0;j<n;++j){
  U top=(own[j]>>36)&1;choice[j]=U(carries.value[0].v[j])|(top<<1)|(U(low.v[j])<<2);
  if(s.party==1)for(U c=0;c<8;++c){
   U c36=U(carries.value[0].v[j])^(c&1),c12=U(low.v[j])^((c>>2)&1),other_top=(c>>1)&1;
   U correction=(top+other_top+c36+1)/2;
   table[j*8+c]=(coarse[j]+c12-correction*H-r[j])&mm;
  }
 }
 auto received=s.lookup_words(table,choice,n,3,37);Ring y(37,n);
 for(size_t j=0;j<n;++j)y.v[j]=s.party==1?r[j]:(received[j]+coarse[j])&mm;
 y.check();return y;
}
}
