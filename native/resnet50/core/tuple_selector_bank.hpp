#pragma once
#include "resnet50/core/prepared_bits.hpp"
#include <openssl/crypto.h>
namespace aqcf2 {
// Independent one-use binary ROT bank realized by the existing KKOT product
// alphabet. This helper has no randomness/key cache and never modifies code,
// security parameter, primitive setup or the live carry-selection consumer.
template<class B> RandomBitOT tuple_selector_bank_segment(B& s,std::size_t n,unsigned pack){
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
// The original per-call65536-row table bound stays intact. Independently fresh
// banks are prepared in segments, then each key is consumed exactly once by
// the unchanged carry consumer. No seed/key state is persisted or shared.
template<class B> RandomBitOT tuple_selector_bank(B& s,std::size_t n,unsigned pack){
 require((pack==3||pack==4)&&n<=(std::size_t(1)<<20),"bounded complete selector frontier");
 const std::size_t capacity=std::size_t(65536)*pack;
 if(n<=capacity)return tuple_selector_bank_segment(s,n,pack);
 RandomBitOT out(n);
 for(std::size_t begin=0;begin<n;begin+=capacity){
  const auto count=std::min(capacity,n-begin);
  auto part=tuple_selector_bank_segment(s,count,pack);
  std::copy(part.k0.begin(),part.k0.end(),out.k0.begin()+begin);
  std::copy(part.k1.begin(),part.k1.end(),out.k1.begin()+begin);
  std::copy(part.t.begin(),part.t.end(),out.t.begin()+begin);
  std::copy(part.kt.begin(),part.kt.end(),out.kt.begin()+begin);
  OPENSSL_cleanse(part.k0.data(),part.k0.size());OPENSSL_cleanse(part.k1.data(),part.k1.size());
  OPENSSL_cleanse(part.t.data(),part.t.size());OPENSSL_cleanse(part.kt.data(),part.kt.size());
 }
 return out;
}
}
