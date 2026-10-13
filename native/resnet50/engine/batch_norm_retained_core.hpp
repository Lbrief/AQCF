#pragma once
#include "resnet50/engine/input_stream_codec.hpp"
#include "resnet50/engine/horner_policy.hpp"
#include "resnet50/engine/joint_retained_policy.hpp"
#include "resnet50/core/cofactor_word_response.hpp"
#include "resnet50/engine/native_quotient_adapter.hpp"
#include <seal/util/ntt.h>
#include <seal/util/polyarithsmallmod.h>
namespace bn191 {
namespace b=aqcf_bn19;namespace q=compose189;namespace l=late188;
using U=b::U;using W=b::W;using V=std::vector<U>;using Bytes=b::Bytes;
template<class T>void discard(std::vector<T>& v){if(!v.empty())OPENSSL_cleanse(v.data(),v.size()*sizeof(T));std::vector<T>().swap(v);}
struct State {
 const b::Geometry& g;std::vector<W> c0;V product,phase306;bool early=false,late=false;
 State(const b::Geometry& geom,int party):g(geom){if(party==1){if(retained307::enabled())phase306.resize(rows());else c0.resize(rows());}else product.resize(3*rows());}
 U rows()const{return U(g.L)*g.channels;}
 ~State(){discard(c0);discard(product);discard(phase306);}
};
seal::Ciphertext unmasked(b::Lane& lane,const b::Geometry& g,U group,const Bytes& frame,const b::PrivateOwner& owner){
 auto data=aqcf_input371::restore(lane,frame,g,group);std::istringstream input(data,std::ios::binary);seal::Ciphertext ct;ct.load(*lane.context,input);
 b::require(input.peek()==std::char_traits<char>::eof()&&ct.size()==2&&!ct.is_ntt_form()&&ct.parms_id()==lane.context->first_parms_id(),"189 bound original input metadata");
 std::vector<b::I> weights(g.C);for(U r=0;r<g.C;++r)weights[r]=owner.weight[group*g.C+r];auto qs=lane.context->first_context_data()->parms().coeff_modulus();
 seal::Ciphertext out;if(horner293::enabled()){horner292::Counts paid;out=horner292::action(ct,weights,lane,paid);lane.switches+=paid.switches;lane.zeros+=paid.extra_zeros;}else out=b::trace(ct,weights,*lane.ev,lane.gk,qs,lane.switches);seal::Ciphertext zero;lane.enc->encrypt_zero(zero);lane.ev->add_inplace(out,zero);++lane.zeros;
 seal::Plaintext bias;bias.resize(b::N);std::fill_n(bias.data(),b::N,U(0));for(U r=0;r<g.C;++r)for(U j=0;j<g.L;++j)bias[j*g.C+r]=b::mod(b::S(g.C)*owner.bias[group*g.C+r],b::T);lane.ev->add_plain_inplace(out,bias);return out;
}
Bytes early_server(b::Lane& lane,State& layer,U group,const Bytes& input,const b::PrivateOwner& owner){
 const auto& g=layer.g;auto ct=unmasked(lane,g,group,input,owner);b::Bits::Writer writer;
 for(U j=0;j<b::N;++j)writer.put(U((lane.codec->crt(ct.data(1),j)+(W(1)<<51))>>52),62);
 auto query=b::query(g,group);U begin=group*g.C*g.L;
 if(retained307::enabled()){
  auto owned=retain298::first(ct,*lane.quarter);
  for(U j=0;j<query.size();++j)layer.phase306[begin+j]=b::local_phase(owned[query[j]],g);
  discard(owned);++lane.returned;return writer.finish();
 }
 std::vector<unsigned char> used(b::N,0);
 for(U j=0;j<query.size();++j){used[query[j]]=1;layer.c0[begin+j]=lane.codec->crt(ct.data(0),query[j]);}
 // Pay original full-Q randomness and c0 updates for unqueried coefficients.
 // Queried coefficients are masked later after the actual input correction.
 q::Random random;for(U j=0;j<b::N;++j)if(!used[j]){W value=random.below(lane.codec->Q);for(U z=0;z<3;++z){U& x=ct.data(0)[z*b::N+j];x+=U(value%lane.codec->q[z]);if(x>=lane.codec->q[z])x-=lane.codec->q[z];}}
 lane.masked_coefficients+=b::N;++lane.returned;return writer.finish();
}
void early_client(b::Lane& lane,State& layer,U group,const Bytes& frame){
 b::require(frame.size()==62*b::N/8,"189 original c1 size");const auto& cd=*lane.context->first_context_data();auto qs=cd.parms().coeff_modulus();
 std::vector<U> product(3*b::N);b::Bits::Reader reader{frame};
 for(U j=0;j<b::N;++j){W value=(W(reader.get(62))<<52)%lane.codec->Q;for(U z=0;z<3;++z)product[z*b::N+j]=U(value%qs[z].value());}reader.end();
 for(U z=0;z<3;++z){auto p=product.data()+z*b::N;seal::util::ntt_negacyclic_harvey(p,cd.small_ntt_tables()[z]);seal::util::dyadic_product_coeffmod(p,lane.sk->data().data()+z*b::N,b::N,qs[z],p);seal::util::inverse_ntt_negacyclic_harvey(p,cd.small_ntt_tables()[z]);}
 auto query=b::query(layer.g,group);U begin=group*layer.g.C*layer.g.L,n=layer.rows();
 for(U j=0;j<query.size();++j)for(U z=0;z<3;++z)layer.product[z*n+begin+j]=product[z*b::N+query[j]];
 ++lane.decrypted;OPENSSL_cleanse(product.data(),product.size()*8);
}

}
