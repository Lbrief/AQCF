#pragma once
#include "squeezenet/core/ring37_direct.hpp"
#include "squeezenet/core/quotient_pool_digits.hpp"

// Candidate PUBLIC qualification only. No new OT primitive or relaxed domain.
// floor(max(x,y)/4096) = max(floor(x/4096),floor(y/4096)) over signed37.
// The exact quotient lies in signed25, so differences fit signed26.
// Only the adjacent MaxPool -> floor12/ReLU pair may use this representation.
namespace aqcf_pool_quotient {
using namespace aqcf2;

template<class B> Ring quotient_lift(B& s,const Ring& input){
 input.check();const auto n=input.v.size();
 require(input.width==37&&n>0&&n<=aqcf_wan_wave::pool_limit,"signed37 quotient lift domain");
 std::vector<U> own=input.v;
 std::vector<CarryTask> tasks{{&own,36,s.constant(false,n),true}};Bits low;
 auto carries=linked_cofactor_prefix_execute(s,tasks,12,low);
 auto& high=carries.value[0];
 s.note_linked_execution(n,n,carries.packet_rows,carries.packet_table_bits,carries.selector_rows);
 std::vector<U> choices(n),table,pad;
 if(s.party==1){pad=s.random_ring_words(n,26);table.resize(8*n);}
 for(std::size_t j=0;j<n;++j){
  const U top=(own[j]>>36)&1;
  choices[j]=U(high.v[j])|(top<<1)|(U(low.v[j])<<2);
  if(s.party==1)for(U v=0;v<8;++v){
   const U carry=U(high.v[j])^(v&1),other_top=(v>>1)&1;
   const U low_carry=U(low.v[j])^((v>>2)&1);
   const U signed_correction=(top+other_top+carry+1)/2;
   table[8*j+v]=((own[j]>>12)+low_carry-(signed_correction<<25)-pad[j])&mask(26);
  }
 }
 auto got=s.lookup_words(table,choices,n,3,26);Ring out(26,n);
 for(std::size_t j=0;j<n;++j)out.v[j]=(s.party==1?pad[j]:got[j]+(own[j]>>12))&mask(26);
 out.check();return out;
}

template<class B> Ring maximum(B& s,const Ring& a,const Ring& b){
 a.check();b.check();const auto n=a.v.size();
 require(a.width==26&&b.width==26&&n>0&&n==b.v.size()&&n<=aqcf_wan_wave::pool_limit,"signed25 values embedded in26");
 Ring diff(26,n);
 for(std::size_t j=0;j<n;++j)diff.v[j]=(b.v[j]-a.v[j])&mask(26);
 auto c=aqcf_quotient_pool_digits::carry(s,diff.v,25);
 auto sign=s.xor_(c,s.local_bit(diff.v,25));auto out=s.mux(s.not_(sign),diff);
 for(std::size_t j=0;j<n;++j)out.v[j]=(out.v[j]+a.v[j])&mask(26);
 out.check();return out;
}

template<class B> Ring finish_relu37(B& s,const Ring& value){
 value.check();const auto n=value.v.size();
 require(value.width==26&&n>0&&n<=aqcf_wan_wave::pool_limit,"pooled quotient domain");
 Ring coarse(25,n);for(std::size_t j=0;j<n;++j)coarse.v[j]=value.v[j]&mask(25);
 auto c=aqcf_quotient_pool_digits::carry(s,coarse.v,24);
 return aqcf_cifar_comm::emit37(s,coarse,s.constant(false,n),c);
}
}
