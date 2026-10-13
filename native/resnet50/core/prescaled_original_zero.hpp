#pragma once
// Strong common control: move the public scalar p^-1 into the ORIGINAL-q PK
// spectrum once. Classical linearity; NOT credited as a new core mechanism.
#include "resnet50/core/lifted_pk_zero.hpp"
#include <array>
namespace aqcf3 {
class PrescaledOriginalZero {
 const seal::SEALContext& cx_;const seal::PublicKey& pk_;
 U h_;std::vector<U> beta_;std::vector<seal::util::MultiplyUIntModOperand> inv_;
public:
 U public_scale_products=0;
 PrescaledOriginalZero(const seal::SEALContext& cx,const seal::PublicKey& pk):cx_(cx),pk_(pk),h_(cx.first_context_data()->parms().coeff_modulus().size()),beta_(2*h_*DEGREE),inv_(h_){
  const auto kd=cx_.key_context_data();const auto&qs=kd->parms().coeff_modulus();
  require(qs.size()==h_+1&&(h_==2||h_==4),"original key/data levels");
  for(U r=0;r<h_;++r){U inverse=0;require(seal::util::try_invert_uint_mod(qs[h_].value()%qs[r].value(),qs[r],inverse),"invert last prime");inv_[r].set(inverse,qs[r]);
   for(U c=0;c<2;++c)for(U j=0;j<DEGREE;++j)beta_[(c*h_+r)*DEGREE+j]=seal::util::multiply_uint_mod(pk_.data().data(c)[r*DEGREE+j],inv_[r],qs[r]);}
  public_scale_products=2*h_*DEGREE;
 }
 seal::Ciphertext with_coins(const std::vector<U>& a,const ZeroCoins& coins,LiftedZeroStats& stats,bool coefficient_input=false){
  require(a.size()==2*h_*DEGREE,"original-q accumulator");const auto kd=cx_.key_context_data();const auto&qs=kd->parms().coeff_modulus();
  auto u=coins.u;require(u.size()==(h_+1)*DEGREE,"original fresh ternary bank");
  for(U r=0;r<h_+1;++r)seal::util::ntt_negacyclic_harvey(u.data()+r*DEGREE,kd->small_ntt_tables()[r]);
  std::vector<U> last(2*DEGREE);std::vector<I> z(2*DEGREE);const U p=qs[h_].value();
  for(U c=0;c<2;++c){U* dst=last.data()+c*DEGREE;seal::util::dyadic_product_coeffmod(u.data()+h_*DEGREE,pk_.data().data(c)+h_*DEGREE,DEGREE,qs[h_],dst);seal::util::inverse_ntt_negacyclic_harvey(dst,kd->small_ntt_tables()[h_]);
   const auto&e=c?coins.e1:coins.e0;for(U j=0;j<DEGREE;++j){U v=dst[j]+e[h_*DEGREE+j];if(v>=p)v-=p;z[c*DEGREE+j]=v>p/2?I(v)-I(p):I(v);}}
  seal::Ciphertext out;out.resize(cx_,cx_.first_parms_id(),2);out.is_ntt_form()=false;
  for(U c=0;c<2;++c)for(U r=0;r<h_;++r){U* dst=out.data(c)+r*DEGREE;U q=qs[r].value();
   seal::util::dyadic_product_coeffmod(u.data()+r*DEGREE,beta_.data()+(c*h_+r)*DEGREE,DEGREE,qs[r],dst);
   if(!coefficient_input){for(U j=0;j<DEGREE;++j){dst[j]+=a[(c*h_+r)*DEGREE+j];if(dst[j]>=q)dst[j]-=q;}}
   seal::util::inverse_ntt_negacyclic_harvey(dst,kd->small_ntt_tables()[r]);
   if(coefficient_input)for(U j=0;j<DEGREE;++j){dst[j]+=a[(c*h_+r)*DEGREE+j];if(dst[j]>=q)dst[j]-=q;}
   const auto&e=c?coins.e1:coins.e0;
   for(U j=0;j<DEGREE;++j){I v=I(e[r*DEGREE+j])-z[c*DEGREE+j];if(v<0)v+=I(q);if(v>=I(q))v-=I(q);dst[j]+=seal::util::multiply_uint_mod(U(v),inv_[r],qs[r]);if(dst[j]>=q)dst[j]-=q;}
  }
  OPENSSL_cleanse(u.data(),u.size()*sizeof(U));OPENSSL_cleanse(last.data(),last.size()*sizeof(U));OPENSSL_cleanse(z.data(),z.size()*sizeof(I));
  ++stats.fresh_zeros;stats.u_forward_ntts+=h_+1;stats.inverse_ntts+=2*(h_+1);stats.avoided_inverse_ntts+=2*h_;stats.pk_dyadic_products+=2*(h_+1)*DEGREE;stats.modulus_switch_components+=2;
  require(seal::is_valid_for(out,cx_)&&!out.is_transparent(),"original prescaled ciphertext");return out;
 }
 seal::Ciphertext finish(const std::vector<U>& a,LiftedZeroStats& stats,bool coefficient_input=false){ZeroCoins fresh(cx_);return with_coins(a,fresh,stats,coefficient_input);}
};
// Original SDK's zero generation arithmetic, with test-visible coupled coins.
// Unlike a zero-valued R9 accumulator, this pays NO useless p*0 operations.
inline seal::Ciphertext original_zero_with_coins(const seal::SEALContext& cx,const seal::PublicKey& pk,const ZeroCoins& coins){
 const auto kd=cx.key_context_data();const auto&qs=kd->parms().coeff_modulus();const U h_=qs.size()-1;auto u=coins.u;std::vector<U> temp(2*(h_+1)*DEGREE);
 for(U r=0;r<h_+1;++r)seal::util::ntt_negacyclic_harvey(u.data()+r*DEGREE,kd->small_ntt_tables()[r]);
 for(U c=0;c<2;++c)for(U r=0;r<h_+1;++r){auto*dst=temp.data()+(c*(h_+1)+r)*DEGREE;seal::util::dyadic_product_coeffmod(u.data()+r*DEGREE,pk.data().data(c)+r*DEGREE,DEGREE,qs[r],dst);seal::util::inverse_ntt_negacyclic_harvey(dst,kd->small_ntt_tables()[r]);const auto&e=c?coins.e1:coins.e0;for(U j=0;j<DEGREE;++j){dst[j]+=e[r*DEGREE+j];if(dst[j]>=qs[r].value())dst[j]-=qs[r].value();}}
 seal::Ciphertext ct;ct.resize(cx,cx.first_parms_id(),2);ct.is_ntt_form()=false;auto pool=seal::MemoryManager::GetPool();
 for(U c=0;c<2;++c){auto*src=temp.data()+c*(h_+1)*DEGREE;kd->rns_tool()->divide_and_round_q_last_inplace(seal::util::RNSIter(src,DEGREE),pool);std::copy_n(src,h_*DEGREE,ct.data(c));}
 OPENSSL_cleanse(u.data(),u.size()*sizeof(U));OPENSSL_cleanse(temp.data(),temp.size()*sizeof(U));return ct;
}
}
