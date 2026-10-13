#pragma once
#include "resnet50/engine/shared_prefix.hpp"
namespace aqcf2 {
template<class B> LinkedCarry alternating37_execute(B& s,const std::vector<CarryTask>& tasks,unsigned tap_width,Bits& tapped){
 const auto n=validate_carry_tasks(tasks,65536);require(tasks.size()==1&&tasks[0].width==32&&tap_width==8&&!tasks[0].public_zero_seed,"alternating37 exact admitted shape");
 const auto& task=tasks[0];std::vector<std::vector<uint8_t>> masks(8,std::vector<uint8_t>(n));
 for(unsigned j=0;j<8;++j)if(s.party==((j%2)?1:2)){auto v=s.random_small(n);for(size_t k=0;k<n;++k)masks[j][k]=v[k]&1;}
 std::vector<uint64_t> ids(8);std::vector<uint8_t> digit(n),carry(n),value=task.seed.v;
 s.boundary_profile("alternating37_all_fixed_choices",[&]{
  for(unsigned j=0;j<8;++j){int sender=(j%2)?1:2;
   for(size_t k=0;k<n;++k){digit[k]=((*task.owned)[k]>>(4*j))&15;carry[k]=j?masks[j-1][k]:task.seed.v[k];}
   ids[j]=s.control().late37_prepare(sender,digit,carry);
  }return 0;
 });
 s.boundary_profile("alternating37_eight_late_messages",[&]{
  for(unsigned j=0;j<8;++j){int sender=(j%2)?1:2;
   for(size_t k=0;k<n;++k){digit[k]=((*task.owned)[k]>>(4*j))&15;if(s.party!=sender)require(value[k]==(j?masks[j-1][k]:task.seed.v[k]),"alternating37 fixed receiver carry");}
   value=s.control().late37_consume(ids[j],digit,value,masks[j]);
   if(j==1)tapped.v=value;
  }return 0;
 });
 LinkedCarry out;out.value.emplace_back(n);out.value[0].v=value;out.packet_rows=8*n;out.packet_table_bits=256*n;out.packet_calls=8;out.selector_rows=0;
 for(auto& v:masks)std::fill(v.begin(),v.end(),0);tapped.check();out.value[0].check();return out;
}
}
