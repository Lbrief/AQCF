#pragma once
// Candidate only: fused full-domain Ring41 FLOOR-ReLU -> Ring55.
// Keeps AQCF linked carry, direct emission and quotient OT. No range fitting,
// plaintext opening, altered weights, or weaker cryptographic parameters.
#include "squeezenet/engine/backbone_boundary.hpp"
namespace aqcf_tiny_comm {
template<class B> aqcf2::Ring direct_ring_relu(B&s,const aqcf2::Ring&in,unsigned f){
 using namespace aqcf2;
 require(in.width==55&&f>=1&&f<=30&&!in.v.empty()&&in.v.size()<=4096,"direct Ring41 ReLU shape");
 const size_t n=in.v.size();Ring coarse(41-f,n);std::vector<U> own(n),low(n);
 for(size_t j=0;j<n;++j){own[j]=in.v[j]&mask(41);low[j]=own[j]&mask(f);coarse.v[j]=own[j]>>f;}
 // Crucial: upper carry is of ORIGINAL low40 bits, including the low-f carry.
 // The ordinary coarse-only carry would fail at the negative Ring41 endpoint.
 std::vector<CarryTask> tasks{{&low,f,s.constant(false,n),true},{&own,40,s.constant(false,n),true}};
 auto c=backbone_carries(s,tasks,BackboneCarry::Linked,n,n);
 // gate=1 XOR topS XOR topC XOR carry40. On gate=1, full wrap is topS OR topC.
 // y=gate*(aS>>f + aC>>f + carry_f - 2^(41-f)*wrap), emitted as Ring55 shares.
 return boundary_relu_emit(s,coarse,c[0],c[1]);
}
}
