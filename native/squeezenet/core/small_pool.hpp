#include "squeezenet/core/squeezenet_duplex_pool.hpp"
#include "squeezenet/core/squeezenet_pool.hpp"
#pragma once
#include "squeezenet/core/quarter_consumer_fusion.hpp"
#include "squeezenet/core/pool_quotient_domain.hpp"
#include <openssl/crypto.h>
namespace aqcf_small_pool20 {
using namespace aqcf2;
template<class B> Ring input_relu25(B& s,const std::vector<U>& phase){
 auto p=aqcf_quarter_entry::prepare(s,phase);const auto n=phase.size();
 if(aqcf_sq266::enabled()){Ring a(25,n);for(size_t i=0;i<n;++i)a.v[i]=p.own37[i]>>12;return aqcf_sq266::emit25(s,a,p.low,p.high,&p.tap302);}
 constexpr unsigned w=25;const U mw=mask(w);
 std::vector<U> a(n),choice(n),table,pads;std::vector<std::uint8_t> beta;
 if(s.party==1){pads=s.random_ring_words(n,w);beta=s.random_small(n);table.resize(4*n);}
 for(std::size_t j=0;j<n;++j){
  a[j]=p.own37[j]>>12;const U gs=((a[j]>>24)&1)^p.high.v[j]^U(s.party==1);
  choice[j]=gs|(U(p.low.v[j])<<1);
  if(s.party==1)for(U c=0;c<4;++c){const U g=gs^(c&1),d=U(p.low.v[j])^(c>>1);
   table[4*j+c]=(((g*(a[j]+d)-pads[j])&mw)<<1)|(g^(beta[j]&1));}
 }
 auto got=s.lookup_words(table,choice,n,2,26);Ring out(w,n);
 std::vector<U> coeff(n);std::vector<std::uint8_t> select(n);
 for(std::size_t j=0;j<n;++j){
  if(s.party==1){out.v[j]=pads[j];select[j]=beta[j]&1;}
  else{const U g=got[j]&1;out.v[j]=((got[j]>>1)+a[j]*g)&mw;coeff[j]=(a[j]*(U(1)-2*g))&mw;}
 }
 auto product=s.owned_word_bit_product(2,coeff,select,w);
 for(std::size_t j=0;j<n;++j)out.v[j]=(out.v[j]+product.v[j])&mw;
 for(auto* v:{&table,&pads,&a,&coeff})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));
 if(!beta.empty())OPENSSL_cleanse(beta.data(),beta.size());out.check();return out;
}
template<class B> Ring maximum25(B& s,const Ring& a,const Ring& b){
 a.check();b.check();require(a.width==25&&b.width==25&&a.v.size()==b.v.size(),"certified nonnegative24 pool shares25");
 Ring d(25,a.v.size());for(std::size_t j=0;j<d.v.size();++j)d.v[j]=(b.v[j]-a.v[j])&mask(25);
 Bits carry;
 if(aqcf_pool273::enabled())carry=aqcf_pool273::carry24(s,d.v);
 else {auto c=aqcf_direct_carry20::run(s,d.v,24,s.constant(false,d.v.size()),true,0);s.note_linked_execution(0,0,c.rows,c.bits,0);carry=std::move(c.carry);}
 auto sign=s.xor_(carry,s.local_bit(d.v,24));auto z=aqcf_pool426::select(s,s.not_(sign),d);
 for(std::size_t j=0;j<z.v.size();++j)z.v[j]=(z.v[j]+a.v[j])&mask(25);
 z.check();return z;
}
template<class B> Ring finish37(B& s,const Ring& small){
 small.check();require(small.width==25,"certified nonnegative24 final lift");const auto n=small.v.size();
 std::vector<U> choice(n),table,pad;if(s.party==1){pad=s.random_ring_words(n,12);table.resize(2*n);}
 for(std::size_t j=0;j<n;++j){U top=small.v[j]>>24;choice[j]=top;
  if(s.party==1)for(U c=0;c<2;++c)table[2*j+c]=(U(0)-(top|c)-pad[j])&mask(12);}
 auto got=s.lookup_words(table,choice,n,1,12);Ring out(37,n);
 for(std::size_t j=0;j<n;++j)out.v[j]=(small.v[j]+((s.party==1?pad[j]:got[j])<<25))&mask(37);
 for(auto* v:{&table,&pad})if(!v->empty())OPENSSL_cleanse(v->data(),v->size()*sizeof(U));out.check();return out;
}
}
