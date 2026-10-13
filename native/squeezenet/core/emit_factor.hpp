#pragma once
// Proposed exact emitter factorization only. Compile into a new candidate;
// never replace the accepted source before public crypto/full graph gates.
namespace aqcf_gate_factor {
template<class B> aqcf2::Ring emit37(B& s,const aqcf2::Ring& a,
                                    const aqcf2::Bits& low,const aqcf2::Bits& carry){
 using namespace aqcf2;
 a.check();low.check();carry.check();
 const std::size_t n=a.v.size();
 require(a.width>=4&&a.width<37&&n>0&&n<=262144&&n==low.v.size()&&n==carry.v.size(),"factor emitter shape and native COT row bound");
 const U H=U(1)<<a.width,mm=mask(37);
 std::vector<U> choice(n),table,r;std::vector<std::uint8_t> beta;
 if(s.party==1){r=s.random_ring_words(n,37);beta=s.random_small(n);table.resize(4*n);}
 for(std::size_t j=0;j<n;++j){
  const U top=(a.v[j]>>(a.width-1))&1;
  choice[j]=U(carry.v[j])|(top<<1);
  if(s.party==1)for(U c=0;c<4;++c){
   const U other_top=(c>>1)&1;
   const U g=1^top^other_top^U(carry.v[j])^(c&1);
   const U v=g*U(low.v[j]);
   const U term=(g*(a.v[j]+U(low.v[j])-H*(top|other_top))-r[j])&mm;
   // beta's two independent uniform bits mask g and g*low_A separately.
   table[j*4+c]=(term<<2)|(g^(beta[j]&1))|((v^((beta[j]>>1)&1))<<1);
  }
 }
 auto received=s.lookup_words(table,choice,n,2,39);
 std::vector<U> coefficient(2*n,0);std::vector<std::uint8_t> selector(2*n,0);
 Ring y(37,n);
 for(std::size_t j=0;j<n;++j){
  if(s.party==1){selector[j]=beta[j]&1;selector[n+j]=(beta[j]>>1)&1;y.v[j]=r[j];}
  else{
   const U ug=received[j]&1,uv=(received[j]>>1)&1;
   const U c0=(a.v[j]+U(low.v[j]))&mm,c1=(U(0)-2*U(low.v[j]))&mm;
   y.v[j]=((received[j]>>2)+c0*ug+c1*uv)&mm;
   coefficient[j]=(c0*(U(1)-2*ug))&mm;
   coefficient[n+j]=(c1*(U(1)-2*uv))&mm;
  }
 }
 // One batched call, two fresh independent COT rows per element. No reuse.
 auto product=s.owned_word_bit_product(2,coefficient,selector,37);
 for(std::size_t j=0;j<n;++j)y.v[j]=(y.v[j]+product.v[j]+product.v[n+j])&mm;
 y.check();
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 if(!r.empty())OPENSSL_cleanse(r.data(),r.size()*sizeof(U));
 if(!beta.empty())OPENSSL_cleanse(beta.data(),beta.size());
 if(!coefficient.empty())OPENSSL_cleanse(coefficient.data(),coefficient.size()*sizeof(U));
 if(!selector.empty())OPENSSL_cleanse(selector.data(),selector.size());
 return y;
}
}
