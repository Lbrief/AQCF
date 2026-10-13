#pragma once
#include "resnet50/engine/shared_prefix.hpp"
#include <openssl/crypto.h>
namespace aqcf2 {
// Same logical sender throughout; all incoming sender carry shares are known
// before any preparation. The first one is the original quarter lookup pad.
template<class B> LinkedCarry prescribed189_execute(B& s,const std::vector<CarryTask>& tasks,
                                                  unsigned tap_width,Bits& tapped,const std::vector<uint8_t>& prescribed_low,const std::vector<uint8_t>& prescribed_high) {
  const auto n=validate_carry_tasks(tasks,65536);
  require(tasks.size()==1&&tasks[0].width==32&&tap_width==8&&!tasks[0].public_zero_seed,
          "quotient169 full32 secret seed and tap8 only");
  const auto& task=tasks[0];
  struct PrivateVectors {
    std::vector<std::vector<uint8_t>> masks;
    std::vector<uint8_t> digit,carry,value;
    explicit PrivateVectors(size_t count):masks(16,std::vector<uint8_t>(count)),
      digit(count),carry(count),value(count){}
    ~PrivateVectors(){for(auto& v:masks)if(!v.empty())OPENSSL_cleanse(v.data(),v.size());
      for(auto* v:{&digit,&carry,&value})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
  } private_values(n);
  auto& masks=private_values.masks;auto& digit=private_values.digit;
  auto& carry=private_values.carry;auto& value=private_values.value;value=task.seed.v;
  for(unsigned j=0;j<16;++j)if(s.party==1) {
    auto v=s.random_small(n);
    for(size_t k=0;k<n;++k)masks[j][k]=v[k]&1;
    OPENSSL_cleanse(v.data(),v.size());
  }
  if(s.party==1){require(prescribed_low.size()==n&&prescribed_high.size()==n,"189 complete prescribed carry masks");masks[3]=prescribed_low;masks[15]=prescribed_high;}
  std::vector<uint64_t> ids(16);
  s.boundary_profile("quotient169_paid_preparation",[&]{
    for(unsigned j=0;j<16;++j){
      for(size_t k=0;k<n;++k)carry[k]=j?masks[j-1][k]:task.seed.v[k];
      ids[j]=s.control().quotient169_prepare(carry);
    }return 0;
  });
  s.boundary_profile("quotient169_sixteen_choice_response_exchanges",[&]{
    for(unsigned j=0;j<16;++j) {
      for(size_t k=0;k<n;++k)digit[k]=((*task.owned)[k]>>(2*j))&3;
      auto next=s.control().quotient169_consume(ids[j],digit,value,masks[j]);
      OPENSSL_cleanse(value.data(),value.size());value.swap(next);
      if(j==3)tapped.v=value;
    }return 0;
  });
  LinkedCarry out;out.value.emplace_back(n);out.value[0].v=value;
  out.packet_rows=16*n;out.packet_table_bits=64*n;out.packet_calls=16;out.selector_rows=0;
  // These inherited counters count table payloads only. Complete accounting
  // MUST include quotient169_stats (D/corrections) and all native I/O receipts.
  tapped.check();out.value[0].check();return out;
}
}
