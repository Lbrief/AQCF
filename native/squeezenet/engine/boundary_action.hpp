#pragma once
// R6: direct full-ring arithmetic emission from coarse quotient descriptors.
// No plaintext opening, no locally cast XOR bit, no shortened output mask.
#include "squeezenet/engine/kernels.hpp"
#include "squeezenet/engine/small_state.hpp"
#include "squeezenet/engine/carry_token.hpp"
#include "squeezenet/engine/linked_cofactor.hpp"
#include "squeezenet/engine/late_carry.hpp"
#include <string>
namespace aqcf2 {

// Existing comparison prefixes have exactly the same boolean functionality.
// 'binary5' reuses the native author's strong R4/R5 control; 'ternary5' retains
// the old AQCF option. No new comparator or radix is introduced.
template<class B> Bits boundary_carry(B& s,const std::vector<U>& own,
                                      unsigned width,const Bits* parity,
                                      const std::string& prefix) {
    if(prefix=="ternary5" || prefix=="xor3ary5")return ternary_carry(s,own,width,parity,prefix=="xor3ary5");
    require(prefix=="binary5","boundary prefix");
    require(s.binary5,"binary5 prefix configuration must match the strong control");
    auto ge=s.carry_eq(own,width);
    return parity?s.xor_(ge.first,s.and_(ge.second,*parity)):ge.first;
}

// Three public labels for a PRIVATE local coarse quotient. No label is opened.
inline U boundary_zone(U a,unsigned k){
    require(k>=4 && k<55 && a<=mask(k),"zone domain");
    const U h=(U(1)<<k)/3;
    return a<h?0:a>=(U(1)<<k)-h?2:1;
}
inline U boundary_winding(U za,U zb){
    require(za<3 && zb<3,"zone alphabet");
    return 1-U(za==0 && zb==0)+U(za==2 && zb==2);
}

// Exact for |center_H(aS+aC)| < floor(H/3). The output is already in M.
// OT input = client zone (3 states) and XOR correction share (2 states).
// Six valid alternatives, padded to eight WITHOUT private-dependent lengths.
template<class B> Ring boundary_signed_emit(B& s,const Ring& a,const Bits& delta){
    a.check();delta.check();
    require(a.width>=4 && a.width<55 && !a.v.empty() && a.v.size()==delta.v.size(),"signed emission shape");
    const U H=U(1)<<a.width, mm=mask(55);
    const std::size_t n=a.v.size();std::vector<U> table,choice(n),r;
    if(s.party==1){r=s.random_ring_words(n,55);table.resize(n*8);}
    for(std::size_t j=0;j<n;++j){
        const U own_zone=boundary_zone(a.v[j],a.width);
        choice[j]=2*own_zone+delta.v[j];
        if(s.party==1)for(U c=0;c<8;++c){
            U correction=0;
            if(c<6){const U winding=boundary_winding(own_zone,c>>1);
                correction=((U(delta.v[j])^(c&1))-H*winding)&mm;}
            table[j*8+c]=(correction-r[j])&mm;
        }
    }
    auto received=s.lookup_words(table,choice,n,3,55);Ring y(55,n);
    for(std::size_t j=0;j<n;++j)y.v[j]=(a.v[j]+(s.party==1?r[j]:received[j]))&mm;
    y.check();return y;
}

// Direct ReLU(RTE) action. carry is the XOR sharing of the carry of the lower
// k-1 bits of the ORIGINAL coarse local quotients, NOT of rerandomized B2A/MUX.
// A 3-input-bit OT emits (full M word, masked gate bit), then ONE reverse COT
// transports the client's private coarse word contribution. This does NOT
// omit a direction of the old two-direction MUX; it is a different sharing.
template<class B> Ring boundary_relu_emit(B& s,const Ring& a,
                                         const Bits& delta,const Bits& carry){
    a.check();delta.check();carry.check();
    require(a.width>=4 && a.width<55 && !a.v.empty() && a.v.size()==delta.v.size() && a.v.size()==carry.v.size(),"ReLU emission shape");
    const U H=U(1)<<a.width,mm=mask(55);
    const std::size_t n=a.v.size();std::vector<U> choice(n),table,r;std::vector<std::uint8_t> beta;
    if(s.party==1){r=s.random_ring_words(n,55);beta=s.random_small(n);table.resize(n*8);}
    for(std::size_t j=0;j<n;++j){
        const U top=(a.v[j]>>(a.width-1))&1;
        choice[j]=U(carry.v[j]) | (top<<1) | (U(delta.v[j])<<2);
        if(s.party==1)for(U c=0;c<8;++c){
            const U other_top=(c>>1)&1;
            const U gate=1 ^ top ^ other_top ^ U(carry.v[j]) ^ (c&1);
            const U correction=U(delta.v[j])^((c>>2)&1);
            // On the positive branch, ordinary carry_H = topS OR topC.
            const U term=(gate*(a.v[j]+correction-H*(top|other_top))-r[j])&mm;
            table[j*8+c]=(term<<1)|(gate^(beta[j]&1));
        }
    }
    auto received=s.lookup_words(table,choice,n,3,56);
    std::vector<U> coefficient(n,0);std::vector<std::uint8_t> selector(n,0);Ring y(55,n);
    for(std::size_t j=0;j<n;++j){
        if(s.party==1){selector[j]=beta[j]&1;y.v[j]=r[j];}
        else{const U g=received[j]&1,v=received[j]>>1;
            coefficient[j]=(a.v[j]*(U(1)-2*g))&mm;
            y.v[j]=(v+a.v[j]*g)&mm;}
    }
    // Sender=client, receiver=server. Return shares of coefficient_C * beta_S.
    auto transfer=s.owned_word_bit_product(2,coefficient,selector,55);
    for(std::size_t j=0;j<n;++j)y.v[j]=(y.v[j]+transfer.v[j])&mm;
    y.check();return y;
}

template<class B> Ring boundary_rte(B& s,const Ring& x,unsigned f,bool relu,
                                   const std::string& prefix,bool direct){
    auto a=half_bias(s,x,f);
    // Identical profiling boundary for ALL controls and candidate: one combined
    // decision phase, then one emission phase. No candidate-only flush removal.
    auto decisions=s.boundary_profile("low_high_decisions",[&](){
        Bits delta,carry;
        if(prefix=="late4" || prefix=="late-add4" || prefix=="linked4" || prefix=="prepared4" || prefix=="token4" || prefix=="materialized4"){
            std::vector<CarryTask> tasks{{&a.low,f,a.parity,false}};
            if(relu)tasks.push_back({&a.base.v,a.base.width-1,s.constant(false,x.v.size()),true});
            if(prefix=="late4" || prefix=="late-add4"){
                auto computed=late_carry_execute(s,tasks,prefix=="late-add4");
                s.note_carry_execution(true,x.v.size(),relu?x.v.size():0,
                  computed.rows,computed.rows,computed.table_bits,computed.online_calls,0);
                delta=std::move(computed.value[0]);if(relu)carry=std::move(computed.value[1]);
            }else if(prefix=="linked4"){
                auto computed=linked_cofactor_execute(s,tasks);
                s.note_linked_execution(x.v.size(),relu?x.v.size():0,computed.packet_rows,computed.packet_table_bits,computed.selector_rows);
                delta=std::move(computed.value[0]);if(relu)carry=std::move(computed.value[1]);
            }else{
                auto computed=prefix=="prepared4"?carry_materialized_prepared(s,tasks):
                  prefix=="token4"?carry_token_execute(s,tasks):carry_materialized_execute(s,tasks);
                if(prefix=="prepared4")s.prepared_control_elements+=x.v.size();
                s.note_carry_execution(prefix=="token4",x.v.size(),relu?x.v.size():0,
                  computed.logical_transitions,computed.ot_rows,computed.table_bits,
                  computed.ot_calls,computed.nonlinear_and_elements);
                delta=std::move(computed.value[0]);if(relu)carry=std::move(computed.value[1]);
            }
        }else{
            delta=boundary_carry(s,a.low,f,&a.parity,prefix);
            if(relu)carry=boundary_carry(s,a.base.v,a.base.width-1,nullptr,prefix);
        }
        return std::make_pair(std::move(delta),std::move(carry));
    });
    const auto& delta=decisions.first;const auto& carry=decisions.second;
    if(direct)return s.boundary_profile("direct_terminal",[&](){return relu?boundary_relu_emit(s,a.base,delta,carry):boundary_signed_emit(s,a.base,delta);});
    // Equally optimized, same-prefix ordinary composition. The scalar branches
    // are the existing coarse-sign/B2A/MUX/lift primitives, not a weak strawman.
    return s.boundary_profile("split_terminal",[&](){
    auto da=s.b2a(delta,a.base.width);Ring q=a.base;
    for(std::size_t j=0;j<q.v.size();++j)q.v[j]=(q.v[j]+da.v[j])&mask(q.width);
    if(!relu)return signed_lift(s,q);
    auto gate=s.not_(s.xor_(carry,s.local_bit(a.base.v,a.base.width-1)));
    return positive_lift(s,s.mux(gate,q));
    });
}
} // namespace aqcf2
