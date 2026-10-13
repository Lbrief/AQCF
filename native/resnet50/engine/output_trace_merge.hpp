#pragma once
#include "resnet50/engine/trace_evaluator_internal.hpp"
#include "resnet50/engine/exact_fma_dot.hpp"
#include "resnet50/engine/pointwise_owner.hpp"
#include "resnet50/engine/short_ntt.hpp"
namespace output_merge515 {
using U=aqcf3::U;using I=aqcf3::I;using W=__uint128_t;
inline bool enabled(){static const bool v=[](){const char*p=std::getenv("AQCF_MERGE515");if(!p||(std::strcmp(p,"0")&&std::strcmp(p,"1")))throw std::runtime_error("515 explicit public arm");return p[0]=='1';}();return v;}
inline std::atomic<U> outputs{0},leaves{0},products{0},switches{0};
inline void margins(const seal::SEALContext& ctx,U C){
 const auto& qs=ctx.first_context_data()->parms().coeff_modulus();const auto& key=ctx.key_context_data()->parms().coeff_modulus();
 aqcf3::require(qs.size()==3&&key.size()==4&&ctx.first_context_data()->parms().poly_modulus_degree()==8192&&ctx.first_context_data()->parms().plain_modulus().value()==(U(1)<<44),"515 original HE context");
 W sum=0,Q=1;for(auto q:qs){aqcf3::require(q.bit_count()==38,"515 original data prime");sum+=q.value()-1;Q*=q.value();}
 W pk=key.back().value(),K=(W(8192)*21*sum+pk-1)/pk+8193,B=W(1)<<30;
 W before=W(C)*C*(22+K)*B+8194,after=W(C)*C*22*B+(W(C)*C-1)*K/3+8194;
 aqcf3::require(after<=before&&W(8)*(W(1)<<44)*(after+(W(1)<<64)+W(8192)*(W(1)<<51))<Q,"515 original codec margin without relaxation");
}
inline seal::Ciphertext merge(std::vector<seal::Ciphertext> z,U s,aqcf_bn19::Lane& lane){
 aqcf3::require(aqcf_internal514::active&&!z.empty(),"515 fixed internal merge scope");if(z.size()==1)return std::move(z[0]);
 std::vector<seal::Ciphertext> even,odd;for(U j=0;j<z.size();++j)(j%2?odd:even).push_back(std::move(z[j]));
 auto a=merge(std::move(even),2*s,lane),b=merge(std::move(odd),2*s,lane);auto qs=lane.context->first_context_data()->parms().coeff_modulus();aqcf_bn19::shift(b,I(s),qs);
 auto d=a;lane.ev->sub_inplace(d,b);lane.ev->add_inplace(a,b);lane.ev->apply_galois_inplace(d,1+8192/s,lane.gk);++lane.switches;switches.fetch_add(1,std::memory_order_relaxed);lane.ev->add_inplace(a,d);return a;
}
struct RawAccumulator {
 void* p=nullptr;
 RawAccumulator(const seal::SEALContext& ctx){seal::Ciphertext zero;zero.resize(ctx,ctx.first_parms_id(),2);std::fill_n(zero.data(),6*8192,U(0));zero.is_ntt_form()=true;p=dot500_kernel::create(ctx,zero);}
 ~RawAccumulator(){dot500_kernel::destroy(p);}
 RawAccumulator(const RawAccumulator&)=delete;RawAccumulator& operator=(const RawAccumulator&)=delete;
};
inline seal::Ciphertext produce(const pw25::Shape& shape,U og,const pw25::Owner& owner,const std::vector<seal::Ciphertext>& input,aqcf_bn19::Lane& lane,U& pm){
 aqcf3::require(enabled()&&shape.Co<shape.Ci&&dot500::enabled()&&input.size()==shape.Ci/shape.C,"515 exact chosen shape and accepted accumulator");margins(*lane.context,shape.C);
 seal::Ciphertext out;U terms=0;
 {aqcf_internal514::Scope scope;std::vector<seal::Ciphertext> z(shape.C);
  for(U r=0;r<shape.C;++r){RawAccumulator acc(*lane.context);
   for(U ig=0;ig<input.size();++ig){std::vector<I> w(shape.C);for(U t=0;t<shape.C;++t)w[shape.C-1-t]=owner.w[(og*shape.C+r)*shape.Ci+ig*shape.C+t];seal::Plaintext pt;short_ntt27::transform(w,shape.C,*lane.context,pt);dot500_kernel::add(acc.p,input[ig],pt);++pm;}
   aqcf3::require(dot500_kernel::terms(acc.p)==input.size(),"515 full original terms");terms+=dot500_kernel::terms(acc.p);z[r]=dot500_kernel::finish(acc.p);lane.ev->transform_from_ntt_inplace(z[r]);leaves.fetch_add(1,std::memory_order_relaxed);
  }out=merge(std::move(z),1,lane);
 }
 aqcf_internal514::require_closed();seal::Ciphertext zero;lane.enc->encrypt_zero(zero);++lane.zeros;lane.ev->add_inplace(out,zero);aqcf3::require(!out.is_transparent(),"515 original final sanitization required");
 aqcf3::require(terms==shape.Ci,"515 logical output exact product count");dot500::products.fetch_add(terms,std::memory_order_relaxed);dot500::outputs.fetch_add(1,std::memory_order_relaxed);outputs.fetch_add(1,std::memory_order_relaxed);products.fetch_add(terms,std::memory_order_relaxed);return out;
}
inline void census(int party){U eo=0,el=0,ep=0,es=0;if(enabled()&&party==1)for(auto s:pw25::shapes)if(s.Co<s.Ci){eo+=s.Co/s.C;el+=s.Co;ep+=s.Ci*s.Co/s.C;es+=(s.Co/s.C)*(s.C-1);}
 aqcf3::require(outputs==eo&&leaves==el&&products==ep&&switches==es,"515 actual leaf and logical output counters");aqcf_internal514::require_closed();
 std::cout<<"MERGE515_CENSUS "<<party<<" "<<enabled()<<" "<<outputs<<" "<<leaves<<" "<<products<<" "<<switches<<"\n";
}
}
