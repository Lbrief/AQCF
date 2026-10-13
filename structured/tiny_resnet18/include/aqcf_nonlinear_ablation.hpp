#pragma once
// Source-generated controls; qualification is required before private n=1.
// Same centered Ring41 FLOOR/ReLU functionality, no plaintext opening.
#include "ring41_boundary.hpp"
#include "ring41_direct.hpp"
#include "BuildingBlocks/value-extension.h"
#ifndef AQCF_ABLATE_PREFIX
#define AQCF_ABLATE_PREFIX 0
#endif
#ifndef AQCF_ABLATE_SHARED_PREFIX
#define AQCF_ABLATE_SHARED_PREFIX 0
#endif
#ifndef AQCF_ABLATE_EMITTER
#define AQCF_ABLATE_EMITTER 0
#endif
#ifndef AQCF_ABLATE_QUOTIENT
#define AQCF_ABLATE_QUOTIENT 0
#endif
#ifndef AQCF_ABLATE_LIFT
#define AQCF_ABLATE_LIFT 0
#endif
#ifndef AQCF_ABLATE_QUARTER
#define AQCF_ABLATE_QUARTER 0
#endif
static_assert((AQCF_ABLATE_PREFIX==0 || AQCF_ABLATE_PREFIX==1) &&
              (AQCF_ABLATE_SHARED_PREFIX==0 || AQCF_ABLATE_SHARED_PREFIX==1) &&
              (AQCF_ABLATE_EMITTER==0 || AQCF_ABLATE_EMITTER==1) &&
              (AQCF_ABLATE_QUOTIENT==0 || AQCF_ABLATE_QUOTIENT==1) &&
              (AQCF_ABLATE_LIFT==0 || AQCF_ABLATE_LIFT==1) &&
              (AQCF_ABLATE_QUARTER==0 || AQCF_ABLATE_QUARTER==1), "binary ablation flags");

namespace aqcf_nl_control {
using namespace aqcf2;
inline const char* extension_mode() {
    return AQCF_ABLATE_QUOTIENT ? "stock" : "quotient";
}

// Native SCI exact signed extension. No fitted range and no known-secret MSB.
inline Ring native_signed_extend(Sci& s,const Ring& input,unsigned target) {
    input.check();require(target>input.width && target<=55,"control extension width");
    Ring out(target,input.v.size());auto own=input.v;
    XTProtocol ext(s.party,&s.io,&s.ot);
    ext.s_extend(s.dim(own.size()),own.data(),out.v.data(),input.width,target);
    s.flush();out.check();return out;
}

inline std::vector<Bits> selected_carries(Sci& s,const std::vector<CarryTask>& tasks,
                                         U low_n,U high_n) {
#if AQCF_ABLATE_PREFIX
    validate_carry_tasks(tasks);require(!s.binary5,"ordinary native comparator required");
    std::vector<Bits> result;
    for(const auto& t:tasks) {
        require(t.public_zero_seed,"these FLOOR controls have zero carry-in");
        result.push_back(s.carry_eq(*t.owned,t.width).first);
    }
    return result;
#else
    return backbone_carries(s,tasks,BackboneCarry::Linked,low_n,high_n);
#endif
}

inline Ring centered_lift(Sci& s,const Ring& input) {
    input.check();require(input.width==55 && !input.v.empty() && input.v.size()<=4096,
                          "control centered Ring41 domain");
#if AQCF_ABLATE_LIFT
    Ring own41(41,input.v.size());
    for(size_t j=0;j<input.v.size();++j)own41.v[j]=input.v[j]&mask(41);
    return native_signed_extend(s,own41,55);
#elif AQCF_ABLATE_PREFIX
    // Same original four-choice terminal, only linked carry is replaced.
    const auto n=input.v.size();const U modulus=U(1)<<41;
    std::vector<U> own(n);for(size_t j=0;j<n;++j)own[j]=input.v[j]&mask(41);
    std::vector<CarryTask> tasks{{&own,40,s.constant(false,n),true}};
    auto carry=selected_carries(s,tasks,n,0)[0];
    std::vector<U> table,pad,choice(n);
    if(s.party==1){pad=s.random_ring_words(n,55);table.resize(n*4);}
    for(size_t j=0;j<n;++j){const U top=(own[j]>>40)&1;
        choice[j]=top|(U(carry.v[j])<<1);
        if(s.party==1)for(U c=0;c<4;++c){
            const U low_carry=U(carry.v[j])^(c>>1);
            const U k=(top+(c&1)+low_carry+1)/2;
            table[j*4+c]=(U(0)-modulus*k-pad[j])&mask(55);
        }
    }
    auto received=s.lookup_words(table,choice,n,2,55);Ring out(55,n);
    for(size_t j=0;j<n;++j)out.v[j]=(own[j]+(s.party==1?pad[j]:received[j]))&mask(55);
    out.check();return out;
#else
    return aqcf_rhombus::centered_lift(s,input);
#endif
}

// Ordinary B2A + BOTH MUX directions + native extension, preserving the
// original shared low/high decisions. No redundant new MSB protocol is added.
inline Ring split_relu41(Sci& s,const Ring& coarse,const Bits& low,const Bits& carry) {
    Ring q=coarse;auto da=s.b2a(low,q.width);
    for(size_t j=0;j<q.v.size();++j)q.v[j]=(q.v[j]+da.v[j])&mask(q.width);
    auto sign=s.xor_(carry,s.local_bit(coarse.v,coarse.width-1));
    auto y=s.mux(s.not_(sign),q);
    // ReLU establishes the public nonnegative-output invariant; individual
    // shares remain uniform full-width and their secret wrap is still paid.
    Ring out(41,q.v.size());auto zero=s.constant(false,q.v.size());
    XTProtocol ext(s.party,&s.io,&s.ot);
    ext.z_extend(s.dim(y.v.size()),y.v.data(),out.v.data(),y.width,41,zero.v.data());
    s.flush();out.check();return out;
}

inline Ring floor_relu41(Sci& s,const Ring& input,unsigned f) {
#if !AQCF_ABLATE_PREFIX && !AQCF_ABLATE_SHARED_PREFIX && !AQCF_ABLATE_EMITTER
    return aqcf_cifar_comm::floor_relu41(s,input,f);
#else
    input.check();require(input.width==55 && f==12 && !input.v.empty() &&
                          input.v.size()<=4096,"control Ring41 FLOOR12 ReLU");
    const auto n=input.v.size();std::vector<U> own(n),low_owned(n);Ring coarse(29,n);
    for(size_t j=0;j<n;++j){own[j]=input.v[j]&mask(41);low_owned[j]=own[j]&mask(f);coarse.v[j]=own[j]>>f;}
    Bits low,high;
#if AQCF_ABLATE_PREFIX || AQCF_ABLATE_SHARED_PREFIX
    // Shared-prefix OFF alone keeps linked-cofactor machinery but computes
    // two independent carry paths. PREFIX OFF uses ordinary SCI comparisons.
    std::vector<CarryTask> tasks{{&low_owned,f,s.constant(false,n),true},
                                 {&own,40,s.constant(false,n),true}};
    auto c=selected_carries(s,tasks,n,n);low=std::move(c[0]);high=std::move(c[1]);
#else
    std::vector<CarryTask> tasks{{&own,40,s.constant(false,n),true}};
    auto c=linked_cofactor_prefix_execute(s,tasks,f,low);
    s.note_linked_execution(n,n,c.packet_rows,c.packet_table_bits,c.selector_rows);
    high=std::move(c.value[0]);
#endif
#if AQCF_ABLATE_EMITTER
    return split_relu41(s,coarse,low,high);
#else
    return aqcf_cifar_comm::emit41(s,coarse,low,high);
#endif
#endif
}

// Called only after the exact Ring41->55 centered lift. Thus |x|<=2^40;
// the old signed-emission premise is valid on the entire Ring41 domain.
inline Ring floor_after_lift(Sci& s,const Ring& input,unsigned f,bool relu) {
#if !AQCF_ABLATE_PREFIX && !AQCF_ABLATE_EMITTER
    return backbone_floor(s,input,f,relu,BackboneCarry::Linked,true);
#else
    input.check();require(input.width==55 && f>=1 && f<=30 && !input.v.empty() &&
                          input.v.size()<=4096 && !relu,"pool FLOOR only");
    const auto n=input.v.size();Ring coarse(55-f,n);std::vector<U> low(n);
    for(size_t j=0;j<n;++j){low[j]=input.v[j]&mask(f);coarse.v[j]=input.v[j]>>f;}
    auto delta=s.boundary_profile("backbone_floor_decisions",[&](){
        std::vector<CarryTask> tasks{{&low,f,s.constant(false,n),true}};
        return selected_carries(s,tasks,n,0)[0];
    });
    return s.boundary_profile("backbone_floor_emission",[&](){
#if AQCF_ABLATE_EMITTER
        auto da=s.b2a(delta,coarse.width);
        for(size_t j=0;j<n;++j)coarse.v[j]=(coarse.v[j]+da.v[j])&mask(coarse.width);
        return native_signed_extend(s,coarse,55);
#else
        return boundary_signed_emit(s,coarse,delta);
#endif
    });
#endif
}
} // namespace aqcf_nl_control
