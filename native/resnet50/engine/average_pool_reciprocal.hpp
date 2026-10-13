#pragma once
#include "resnet50/engine/ring37_boundary.hpp"
namespace aqcf_avg29 {
using namespace aqcf2;
template<class B> Ring positive_lift_to(B& s,const Ring& x,unsigned dest){
 x.check();require(x.width>=4&&x.width<dest&&dest<=55,"positive lift public widths");
 const unsigned f=dest-x.width;std::vector<std::uint8_t> top(x.v.size());
 for(std::size_t j=0;j<top.size();++j)top[j]=(x.v[j]>>(x.width-1))&1;
 auto prod=s.bit_product(top,f);Ring out(dest,top.size());
 for(std::size_t j=0;j<top.size();++j){const U wrap=(U(top[j])-prod.v[j])&mask(f);out.v[j]=(x.v[j]-(wrap<<x.width))&mask(dest);}
 out.check();return out;
}
// Only graph-certified sums of49 original nonnegative floor12/ReLU values.
// No secret-dependent check, opening, table index, dispatch or early exit.
template<class B> Ring divide49_bounded(B& s,const Ring& input){
 input.check();require(input.width==37&&!input.v.empty(),"certified AvgPool input ABI");
 const auto n=input.v.size();constexpr U multiplier=21913098;
 auto wide=positive_lift_to(s,input,55);
 for(auto& v:wide.v)v=(v*multiplier)&mask(55);
 auto low=s.carry_eq(wide.v,30).first;
 auto q=s.b2a(low,25);
 for(std::size_t j=0;j<n;++j)q.v[j]=((wide.v[j]>>30)+q.v[j])&mask(25);
 auto out=positive_lift_to(s,q,37);Ring remainder(7,n);
 for(std::size_t j=0;j<n;++j)remainder.v[j]=(input.v[j]-49*out.v[j]-(s.party==1?49:0))&mask(7);
 auto correction=s.b2a(s.not_(s.msb(remainder)),37);
 for(std::size_t j=0;j<n;++j)out.v[j]=(out.v[j]+correction.v[j])&mask(37);
 out.check();return out;
}
// Exact existing generic control, copied from route28 R50Backend::divide49.
template<class B> Ring divide49_control(B& s,const Ring& input){
 Ring a(55);a.v=input.v;auto rem=aqcf_rhombus::centered_lift(s,a,37);Ring quotient(55,a.v.size());
 if(s.party==1)for(auto&v:rem.v)v=(v+(U(1)<<36))&mask(55);
 for(int k=31;k>=0;--k){U d=U(49)<<k;Ring diff=rem;
  if(s.party==1)for(auto&v:diff.v)v=(v-d)&mask(55);
  auto bit=s.b2a(s.not_(s.msb(diff)),55);
  for(std::size_t j=0;j<a.v.size();++j){rem.v[j]=(rem.v[j]-bit.v[j]*d)&mask(55);quotient.v[j]=(quotient.v[j]+(bit.v[j]<<k))&mask(55);}
 }
 constexpr U offset=U(1)<<36;auto diff=rem;
 if(s.party==1)for(auto&v:diff.v)v=(v-offset%49)&mask(55);
 auto correction=s.b2a(s.msb(diff),55);Ring out(37,a.v.size());
 for(std::size_t j=0;j<a.v.size();++j)out.v[j]=(quotient.v[j]-correction.v[j]-(s.party==1?offset/49:0))&mask(37);
 return out;
}
}
