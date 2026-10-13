#pragma once
#include "squeezenet/engine/sci.hpp"
#include "squeezenet/engine/linked_cofactor.hpp"

namespace aqcf_range_tail {
using namespace aqcf2;
// Small, public-dimension helper; existing single-use linked-cofactor protocol.
template<class B> Bits carry(B& s,const std::vector<U>& own,unsigned width){
 require((width==8||width==24||width==32)&&!own.empty()&&own.size()<=4096,"tail carry domain");
 std::vector<CarryTask> tasks{{&own,width,s.constant(false,own.size()),true}};
 auto c=linked_cofactor_execute(s,tasks);return c.value[0];
}
// Same proven nonnegative wrap identity as candidate003, with target Ring57.
template<class B> Ring lift57(B& s,const Ring& x){
 x.check();require(x.width==37&&!x.v.empty()&&x.v.size()<=4096,"tail lift domain");
 const auto n=x.v.size();std::vector<U> table,choice(n),pad;
 if(s.party==1){pad=s.random_ring_words(n,20);table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){U a=(x.v[j]>>36)&1;choice[j]=a;
  if(s.party==1)for(U b=0;b<2;++b)table[2*j+b]=((a|b)-pad[j])&mask(20);}
 auto got=s.lookup_words(table,choice,n,1,20,false);Ring y(57,n);
 for(std::size_t j=0;j<n;++j)y.v[j]=(x.v[j]-((s.party==1?pad[j]:got[j])<<37))&mask(57);
 y.check();return y;
}
// Native13x13 average, exact for x=sum169 certified floor12-ReLU values.
// x<=169*(2^24-1). Let C=floor(2^32/169). Then 0<=floor(x/169)-
// floor(x*C/2^32)<=1. A single comparison of the residual repairs the quotient.
// This is standard reciprocal division specialized to a publicly proved range,
// not a newly invented division or cryptographic primitive.
template<class B> Ring divide169(B& s,const Ring& input){
 constexpr U C=(U(1)<<32)/169,H=U(1)<<25;
 static_assert(169*((U(1)<<24)-1)<(U(1)<<32),"public sum bound");
 static_assert(169*((U(1)<<24)-1)*C<(U(1)<<56),"public product bound");
 input.check();require(input.width==37&&!input.v.empty()&&input.v.size()<=4096,"certified division shape");
 auto product=lift57(s,input);const auto n=product.v.size();
 for(auto&v:product.v)v=(v*C)&mask(57);
 auto low=carry(s,product.v,32);std::vector<U> table,choice(n),pad;
 if(s.party==1){pad=s.random_ring_words(n,37);table.resize(4*n);}
 for(std::size_t j=0;j<n;++j){U top=(product.v[j]>>56)&1;choice[j]=U(low.v[j])|(top<<1);
  if(s.party==1)for(U c=0;c<4;++c){U c32=U(low.v[j])^(c&1),wrap=top|((c>>1)&1);
   table[4*j+c]=((product.v[j]>>32)+c32-wrap*H-pad[j])&mask(37);}}
 auto got=s.lookup_words(table,choice,n,2,37,false);Ring q(37,n),diff(37,n);
 for(std::size_t j=0;j<n;++j){q.v[j]=s.party==1?pad[j]:(got[j]+(product.v[j]>>32))&mask(37);
  diff.v[j]=(input.v[j]-169*q.v[j]-(s.party==1?169:0))&mask(37);}
 auto c8=carry(s,diff.v,8);auto sign=s.xor_(c8,s.local_bit(diff.v,8));auto add=s.b2a(s.not_(sign),37);
 for(std::size_t j=0;j<n;++j)q.v[j]=(q.v[j]+add.v[j])&mask(37);
 q.check();return q;
}

// Return an XOR-shared bit for strict a<b. Both reconstructed operands must
// be certified in [0,2^24-1]. Their DIFFERENCE fits signed25, without overflow.
template<class B> Bits less24(B& s,const Ring& a,const Ring& b){
 a.check();b.check();require(a.width==37&&b.width==37&&a.v.size()==b.v.size(),"tail compare shape");
 Ring diff(37,a.v.size());for(std::size_t j=0;j<diff.v.size();++j)diff.v[j]=(a.v[j]-b.v[j])&mask(37);
 auto c=carry(s,diff.v,24);return s.xor_(c,s.local_bit(diff.v,24));
}
}
