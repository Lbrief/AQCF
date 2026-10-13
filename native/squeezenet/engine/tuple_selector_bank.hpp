#pragma once
#include "squeezenet/engine/prepared_bits.hpp"
#include <openssl/crypto.h>
namespace aqcf2 {
// Independent one-use binary ROT bank realized by the existing KKOT product
// alphabet. This helper has no randomness/key cache and never modifies code,
// security parameter, primitive setup or the live carry-selection consumer.
template<class B> RandomBitOT tuple_selector_bank_chunk(B& s,std::size_t n,unsigned pack){
 require(pack==3||pack==4,"fixed public selector product alphabet");
 RandomBitOT out(n);if(!n)return out;
 const std::size_t rows=(n+pack-1)/pack;const U count=U(1)<<pack;
 require(rows<=65536,"bounded tuple selector bank");
 std::vector<U> choices(rows,0),table;
 auto coins=s.random_small(rows*pack);
 if(s.party==1){
  table.resize(rows*count);
  for(std::size_t row=0;row<rows;++row){
   U base=0,delta=0;
   for(unsigned bit=0;bit<pack;++bit){std::size_t j=row*pack+bit;
    U k0=coins[j]&1,k1=(coins[j]>>1)&1;base|=k0<<bit;delta|=(k0^k1)<<bit;
    if(j<n){out.k0[j]=k0;out.k1[j]=k1;}}
   for(U choice=0;choice<count;++choice)table[row*count+choice]=base^(choice&delta);
  }
 }else{
  for(std::size_t row=0;row<rows;++row)for(unsigned bit=0;bit<pack;++bit){std::size_t j=row*pack+bit;
   U choice=coins[j]&1;choices[row]|=choice<<bit;if(j<n)out.t[j]=choice;}
 }
 auto answer=s.lookup_words(table,choices,rows,pack,pack);
 if(s.party==2)for(std::size_t j=0;j<n;++j)out.kt[j]=(answer[j/pack]>>(j%pack))&1;
 OPENSSL_cleanse(coins.data(),coins.size());
 if(!table.empty())OPENSSL_cleanse(table.data(),table.size()*sizeof(U));
 if(!answer.empty())OPENSSL_cleanse(answer.data(),answer.size()*sizeof(U));
 s.flush();s.random_bit_OT_rows+=n;return out;
}
}

namespace aqcf2 {
template<class B> RandomBitOT tuple_selector_bank(B& s,std::size_t n,unsigned pack){
#if AQCF_LAYER_FRONTIER
 require((pack==3||pack==4)&&n<=9*262144,"bounded layer-frontier selector bank");
 if(n>pack*65536){
  RandomBitOT out(n);
  for(std::size_t start=0;start<n;start+=pack*65536){
   const auto len=std::min<std::size_t>(pack*65536,n-start);
   auto part=tuple_selector_bank_chunk(s,len,pack);
   for(std::size_t j=0;j<len;++j){out.k0[start+j]=part.k0[j];out.k1[start+j]=part.k1[j];out.t[start+j]=part.t[j];out.kt[start+j]=part.kt[j];}
   for(auto* v:{&part.k0,&part.k1,&part.t,&part.kt})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());
  }
  return out;
 }
#endif
 return tuple_selector_bank_chunk(s,n,pack);
}
}
