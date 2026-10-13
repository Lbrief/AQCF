#pragma once
#include "resnet50/engine/shared_prefix.hpp"
namespace aqcf_cifar_comm {
// Full signed Ring37 domain, not a weight-derived range. A fresh full Ring37
// additive mask is used for every result. Next existing graph consumers use
// this ring; final Ring55 score lifting remains the original secure protocol.
template<class B> aqcf2::Ring emit37_parent(B& s,const aqcf2::Ring& a,
                                    const aqcf2::Bits& low,const aqcf2::Bits& carry){
 using namespace aqcf2;
 a.check();low.check();carry.check();
 require(a.width>=4&&a.width<37&&!a.v.empty()&&a.v.size()==low.v.size()&&a.v.size()==carry.v.size(),"full Ring37 emitter domain");
 const U H=U(1)<<a.width,mm=mask(37);const size_t n=a.v.size();
 std::vector<U> choice(n),table,r;std::vector<uint8_t> beta;
 if(s.party==1){r=s.random_ring_words(n,37);beta=s.random_small(n);table.resize(n*8);}
 for(size_t j=0;j<n;++j){const U top=(a.v[j]>>(a.width-1))&1;
  choice[j]=U(carry.v[j])|(top<<1)|(U(low.v[j])<<2);
  if(s.party==1)for(U c=0;c<8;++c){const U other_top=(c>>1)&1;
   const U gate=1^top^other_top^U(carry.v[j])^(c&1),correction=U(low.v[j])^((c>>2)&1);
   const U term=(gate*(a.v[j]+correction-H*(top|other_top))-r[j])&mm;
   table[j*8+c]=(term<<1)|(gate^(beta[j]&1));
  }
 }
 auto received=s.lookup_words(table,choice,n,3,38);
 std::vector<U> coefficient(n,0);std::vector<uint8_t> selector(n,0);Ring y(37,n);
 for(size_t j=0;j<n;++j){if(s.party==1){selector[j]=beta[j]&1;y.v[j]=r[j];}
  else{const U g=received[j]&1,v=received[j]>>1;coefficient[j]=(a.v[j]*(U(1)-2*g))&mm;y.v[j]=(v+a.v[j]*g)&mm;}}
 auto product=s.owned_word_bit_product(2,coefficient,selector,37);
 for(size_t j=0;j<n;++j)y.v[j]=(y.v[j]+product.v[j])&mm;
 y.check();return y;
}
}
#include "resnet50/engine/nonnegative_terminal.hpp"
namespace aqcf_cifar_comm {
template<class B> aqcf2::Ring emit37(B& s,const aqcf2::Ring& a,const aqcf2::Bits& low,const aqcf2::Bits& carry){return aqcf_nonnegative_terminal::emit(s,a,low,carry);}
template<class B> aqcf2::Ring floor_relu37(B& s,const aqcf2::Ring& in,unsigned f){
 using namespace aqcf2;
 require(in.width==55&&f==12&&!in.v.empty()&&in.v.size()<=(AQCF_R50_FRONTIER?65536:4096),"CIFAR full-domain floor12-ReLU");
 const size_t n=in.v.size();std::vector<U> own(n);Ring coarse(37-f,n);
 for(size_t j=0;j<n;++j){own[j]=in.v[j]&mask(37);coarse.v[j]=own[j]>>f;}
 std::vector<CarryTask> tasks{{&own,36,s.constant(false,n),true}};Bits low;
 auto carries=linked_cofactor_prefix_execute(s,tasks,f,low);
 s.note_linked_execution(n,n,carries.packet_rows,carries.packet_table_bits,carries.selector_rows);
 return emit37(s,coarse,low,carries.value[0]);
}
}
