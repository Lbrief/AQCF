#pragma once
// Same existing quotient identity, selected at PUBLIC output-tile granularity.
// No private equality/sparsity-dependent cache or change to ciphertext degree.
#include "resnet50/core/kernel_action.hpp"
#include "resnet50/core/signed_weight.hpp"
#include <map>
#include <memory>
#include <unordered_map>
namespace aqcf3::tile_weight {
struct Plan {
 U b,d;std::vector<ActionNTT> transform;std::vector<std::vector<U>> index;
 Plan(const seal::SEALContext& cx,U spacing):b(spacing),d(DEGREE/spacing){
  require(b>=2 && b<=4096 && !(b&(b-1)),"public tile quotient domain");
  const auto& data=*cx.first_context_data();const auto& qs=data.parms().coeff_modulus();
  seal::Evaluator evaluator(cx);seal::Plaintext monomial;monomial.resize(DEGREE);monomial[b]=1;
  evaluator.transform_to_ntt_inplace(monomial,cx.first_parms_id());
  for(unsigned r=0;r<qs.size();++r){
   transform.emplace_back(d,qs[r].value());Edge e;e.coefficients.push_back({b,1});
   auto roots=transform.back().weight(e,b);std::unordered_map<U,U> positions;
   for(U k=0;k<d;++k)require(positions.emplace(roots[k],k).second,"distinct quotient roots");
   std::vector<U> map(DEGREE),counts(d,0);
   for(U j=0;j<DEGREE;++j){auto pos=positions.find(monomial[r*DEGREE+j]);require(pos!=positions.end(),"full and quotient root matching");map[j]=pos->second;++counts[map[j]];}
   for(U n:counts)require(n==b,"complete quotient multiplicity");index.push_back(std::move(map));
  }
 }
 std::vector<U> weight(const Edge& edge)const{
  std::vector<U> result(DEGREE*transform.size());
  for(unsigned r=0;r<transform.size();++r){auto small=transform[r].weight(edge,b);
   for(U j=0;j<DEGREE;++j)result[r*DEGREE+j]=small[index[r][j]];
  }return result;
 }
};
// The key contains only a live context address and the PUBLIC layout spacing.
// Initialization is paid inside the online call, never moved outside timing.
inline std::vector<U> spectrum(const seal::SEALContext& cx,const Edge& edge,U spacing){
 if(spacing<=1)return aqcf_signed_weight::spectrum(cx,edge.coefficients);
 thread_local std::map<std::pair<const seal::SEALContext*,U>,std::unique_ptr<Plan>> plans;
 auto key=std::make_pair(&cx,spacing);auto it=plans.find(key);
 if(it==plans.end())it=plans.emplace(key,std::make_unique<Plan>(cx,spacing)).first;
 return it->second->weight(edge);
}
}
