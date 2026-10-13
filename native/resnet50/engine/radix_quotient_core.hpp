#pragma once
#include "resnet50/engine/quotient_recovery_core.hpp"
#include <cstdlib>
#include <cstring>
namespace aqcf2 {
inline bool radix197_enabled(){const char* p=std::getenv("AQCF_RADIX197");require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"trusted fixed radix197 flag");return p[0]=='1';}
template<class B>LinkedCarry quotient197_execute(B& s,const std::vector<CarryTask>& tasks,unsigned tap_width,Bits& tapped,const std::vector<uint8_t>* pl=nullptr,const std::vector<uint8_t>* ph=nullptr){
 const auto n=validate_carry_tasks(tasks,65536);require(tasks.size()==1&&tasks[0].width==32&&tap_width==8&&!tasks[0].public_zero_seed,"197 fixed full32/tap8 secret seed");
 require(bool(pl)==bool(ph),"197 both prescribed masks");const auto& task=tasks[0];
 constexpr unsigned width[11]={3,3,2,3,3,3,3,3,3,3,3};
 struct PrivateVectors{
  std::vector<std::vector<uint8_t>> masks;std::vector<uint8_t> digit,carry,value;
  explicit PrivateVectors(size_t n):masks(11,std::vector<uint8_t>(n)),digit(n),carry(n),value(n){}
  ~PrivateVectors(){for(auto& v:masks)if(!v.empty())OPENSSL_cleanse(v.data(),v.size());for(auto* v:{&digit,&carry,&value})if(!v->empty())OPENSSL_cleanse(v->data(),v->size());}
 } p(n);p.value=task.seed.v;
 for(unsigned j=0;j<11;++j)if(s.party==1){auto v=s.random_small(n);for(size_t k=0;k<n;++k)p.masks[j][k]=v[k]&1;OPENSSL_cleanse(v.data(),v.size());}
 if(pl&&s.party==1){require(pl->size()==n&&ph->size()==n,"197 prescribed dimensions");p.masks[2]=*pl;p.masks[10]=*ph;}
 std::vector<uint64_t> ids(11);
 s.boundary_profile("quotient197_paid_preparation",[&]{for(unsigned j=0;j<11;++j){for(size_t k=0;k<n;++k)p.carry[k]=j?p.masks[j-1][k]:task.seed.v[k];ids[j]=s.control().quotient197_prepare(p.carry,width[j]);}return 0;});
 s.boundary_profile("quotient197_eleven_choice_response_exchanges",[&]{unsigned offset=0;for(unsigned j=0;j<11;++j){for(size_t k=0;k<n;++k)p.digit[k]=((*task.owned)[k]>>offset)&((1u<<width[j])-1);auto next=s.control().quotient197_consume(ids[j],p.digit,p.value,p.masks[j]);OPENSSL_cleanse(p.value.data(),p.value.size());p.value.swap(next);offset+=width[j];if(offset==8)tapped.v=p.value;}require(offset==32,"197 complete bit range");return 0;});
 LinkedCarry out;out.value.emplace_back(n);out.value[0].v=p.value;out.packet_rows=11*n;out.packet_table_bits=84*n;out.packet_calls=11;out.selector_rows=0;tapped.check();out.value[0].check();return out;
}
}
