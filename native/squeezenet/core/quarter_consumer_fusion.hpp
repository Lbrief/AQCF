#pragma once
#include "squeezenet/core/ring37_direct.hpp"
#include "squeezenet/engine/quarter_recover.hpp"
#include "squeezenet/core/squeezenet_affine_adapter.hpp"
#ifndef AQCF_FUSED_QUARTER_ENTRY
#define AQCF_FUSED_QUARTER_ENTRY 0
#endif
static_assert(AQCF_FUSED_QUARTER_ENTRY==0||AQCF_FUSED_QUARTER_ENTRY==1,"explicit quarter consumer candidate");
namespace aqcf_quarter_entry {
using namespace aqcf2;
inline unsigned correction_bit(unsigned client_low,unsigned server_low){
 const int h=int(aqcf_quarter::winding(client_low,server_low))
           -int(client_low==3)+int(server_low!=0);
 require(client_low<4&&server_low<4&&(h==0||h==1),"normalized winding is Boolean");
 return unsigned(h);
}
struct Prepared {std::vector<U> own37;Bits low,high;std::vector<U> tap302;~Prepared(){if(!tap302.empty())OPENSSL_cleanse(tap302.data(),tap302.size()*sizeof(U));}};
template<class B> Prepared prepare(B& s,const std::vector<U>& phase){
 if(aqcf_sq266::enabled()){auto p=aqcf_sq266::prepare(s,phase);return {std::move(p.own37),std::move(p.low),std::move(p.high),std::move(p.tap302)};}
 const auto n=phase.size();require(n>0&&n<=aqcf_wan_wave::relu_limit,"quarter consumer public wave");
 require(s.compact_cofactor,"same exact chosen lookup");
 Prepared p;p.own37.resize(n);std::vector<U> choice(n),table(s.party==1?64*n:0),shifted(n);
 std::vector<std::uint8_t> pad;Bits seed(n);if(s.party==1)pad=s.random_small(n);
 for(std::size_t j=0;j<n;++j){
  require(phase[j]<(U(1)<<57),"raw quarter phase canonical");
  const U lo=phase[j]&3,hi=phase[j]>>2;
  p.own37[j]=(s.party==1?U(0)-hi-U(lo!=0):hi+U(lo==3))&mask(37);
  const U d=p.own37[j]&15;choice[j]=(d<<2)|lo;shifted[j]=p.own37[j]>>4;
  if(s.party==1)for(unsigned k=0;k<64;++k){
   const unsigned h=correction_bit(k&3,unsigned(lo));
   const U carry=(d+(k>>2)+h)>>4;
   require(carry<2,"first normalized carry remains Boolean");
   table[j*64+k]=carry^(pad[j]&1);
  }
 }
 // The receiver selects its local normalized digit AND original two phase bits.
 // Exactly one bit is returned, independently masked by party1; no winding opens.
 auto got=s.lookup_words(table,choice,n,6,1,false);s.flush();
 for(std::size_t j=0;j<n;++j)seed.v[j]=s.party==1?(pad[j]&1):got[j];
 seed.check();
 // After the first4bits, the rest is the existing exact secret-seed recurrence.
 // Tap8 here corresponds to original bit12. No additional high-bit truncation.
 auto rest=aqcf_direct_carry20::run(s,shifted,32,seed,false,8);
 p.low=rest.tap;p.high=rest.carry;
 s.note_linked_execution(n,n,rest.rows+n,rest.bits+64*n,0);
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 if(!pad.empty())OPENSSL_cleanse(pad.data(),pad.size());
 return p;
}
template<class B> Ring floor_relu37(B& s,const std::vector<U>& phase){
 auto p=prepare(s,phase);Ring coarse(25,phase.size());
 for(std::size_t j=0;j<phase.size();++j)coarse.v[j]=p.own37[j]>>12;
 return aqcf_sq266::enabled()?aqcf_sq266::emit37(s,coarse,p.low,p.high,&p.tap302):aqcf_cifar_comm::emit37(s,coarse,p.low,p.high);
}
template<class B> Ring quotient26(B& s,const std::vector<U>& phase){
 auto p=prepare(s,phase);const auto n=phase.size();
 std::vector<U> choice(n),table,pad;if(s.party==1){pad=s.random_ring_words(n,26);table.resize(8*n);}
 for(std::size_t j=0;j<n;++j){
  const U top=(p.own37[j]>>36)&1;
  choice[j]=U(p.high.v[j])|(top<<1)|(U(p.low.v[j])<<2);
  if(s.party==1)for(U c=0;c<8;++c){
   const U high=U(p.high.v[j])^(c&1),other_top=(c>>1)&1,low=U(p.low.v[j])^((c>>2)&1);
   const U correction=(top+other_top+high+1)/2;
   table[j*8+c]=((p.own37[j]>>12)+low-(correction<<25)-pad[j])&mask(26);
  }
 }
 auto got=s.lookup_words(table,choice,n,3,26);Ring out(26,n);
 for(std::size_t j=0;j<n;++j)out.v[j]=(s.party==1?pad[j]:got[j]+(p.own37[j]>>12))&mask(26);
 out.check();return out;
}
}
