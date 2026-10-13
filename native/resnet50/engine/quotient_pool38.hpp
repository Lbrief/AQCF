#pragma once
#include "resnet50/engine/ring37_boundary.hpp"
// Exact public-domain specialization of the existing quotient/carry mechanism.
// It is NOT an independent security proof or a newly invented OT primitive.
namespace aqcf_pool38 {
template<class B> aqcf2::Ring lift(B& s,const aqcf2::Ring& in,unsigned w=37,unsigned dest=38){
 using namespace aqcf2;in.check();
 require(w>=3&&dest>w&&dest<=55&&!in.v.empty()&&in.v.size()<=65536,"quotient lift public widths");
 const auto n=in.v.size();const unsigned h=dest-w;std::vector<U> own(n);
 for(size_t j=0;j<n;++j)own[j]=in.v[j]&mask(w);
 std::vector<CarryTask> tasks{{&own,w-1,s.constant(false,n),true}};
 auto c=backbone_carries(s,tasks,BackboneCarry::Linked,n,0)[0];
 std::vector<U> choices(n),table,pad;
 if(s.party==1){pad=s.random_ring_words(n,h);table.resize(n*4);}
 for(size_t j=0;j<n;++j){
  U top=own[j]>>(w-1);choices[j]=top|(U(c.v[j])<<1);
  if(s.party==1)for(U v=0;v<4;++v){
   U carry=U(c.v[j])^(v>>1),k=(top+(v&1)+carry+1)/2;
   table[j*4+v]=(k-pad[j])&mask(h);
  }
 }
 auto answer=s.lookup_words(table,choices,n,2,h);Ring out(dest,n);
 for(size_t j=0;j<n;++j)out.v[j]=(own[j]-((s.party==1?pad[j]:answer[j])<<w))&mask(dest);
 out.check();return out;
}
template<class B> aqcf2::Ring maximum(B& s,const aqcf2::Ring& a,const aqcf2::Ring& b){
 using namespace aqcf2;a.check();b.check();
 require(a.width==38&&b.width==38&&a.v.size()==b.v.size(),"signed37 operands in Ring38");
 const auto n=a.v.size();Ring diff(38,n);
 for(size_t j=0;j<n;++j)diff.v[j]=(b.v[j]-a.v[j])&mask(38);
 std::vector<CarryTask> tasks{{&diff.v,37,s.constant(false,n),true}};
 auto c=backbone_carries(s,tasks,BackboneCarry::Linked,0,n)[0];
 auto sign=s.xor_(c,s.local_bit(diff.v,37));auto pos=s.mux(s.not_(sign),diff);
 for(size_t j=0;j<n;++j)pos.v[j]=(pos.v[j]+a.v[j])&mask(38);
 pos.check();return pos;
}
}
