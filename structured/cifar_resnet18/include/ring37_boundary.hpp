#pragma once
// Exact ring-width portability, not a claim of a new cryptographic primitive.
// Reuse AQCF linked carry + standard padded OT. No intermediate is opened.
#include "backbone_boundary.hpp"
namespace aqcf_rhombus {
template<class B>
aqcf2::Ring centered_lift(B& s,const aqcf2::Ring& input,unsigned width=37){
    using namespace aqcf2;
    input.check();require(input.width==55&&width>=3&&width<=53&&
        !input.v.empty()&&input.v.size()<=4096,"bounded full-domain width lift");
    const auto n=input.v.size();const U modulus=U(1)<<width;
    std::vector<U> own(n);for(std::size_t j=0;j<n;++j)own[j]=input.v[j]&mask(width);
    // c is the carry of the lower width-1 bits. For each element let
    // v=top_S+top_C+c (0..3). Full wrap + sign = ceil(v/2).
    // Thus center((aS+aC) mod2^w)=aS+aC-2^w*ceil(v/2), for ALL shares.
    std::vector<CarryTask> tasks{{&own,width-1,s.constant(false,n),true}};
    auto carry=backbone_carries(s,tasks,BackboneCarry::Linked,n,0)[0];
    std::vector<U> table,pad,choice(n);
    if(s.party==1){pad=s.random_ring_words(n,55);table.resize(n*4);}
    for(std::size_t j=0;j<n;++j){
        const U top=(own[j]>>(width-1))&1;
        choice[j]=top|(U(carry.v[j])<<1);
        if(s.party==1)for(U c=0;c<4;++c){
            const U low_carry=U(carry.v[j])^(c>>1);
            const U k=(top+(c&1)+low_carry+1)/2;
            table[j*4+c]=(U(0)-modulus*k-pad[j])&mask(55);
        }
    }
    auto received=s.lookup_words(table,choice,n,2,55);Ring out(55,n);
    for(std::size_t j=0;j<n;++j)
        out.v[j]=(own[j]+(s.party==1?pad[j]:received[j]))&mask(55);
    out.check();return out;
}
template<class B>
aqcf2::Ring floor_boundary(B&s,const aqcf2::Ring& x,unsigned f,bool relu){
    // The exact lift ensures |z|<=2^36, inside the old2^53 premise on every
    // input, including ring37 endpoints; no fitted/observed model range used.
    auto signed55=centered_lift(s,x,37);
    return aqcf2::backbone_floor(s,signed55,f,relu,aqcf2::BackboneCarry::Linked,true);
}
}
