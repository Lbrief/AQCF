#pragma once
// Unqualified source candidate. No production caller includes this header.
#include <cstdint>
#include <stdexcept>
namespace aqcf_joint_mask {
using U=std::uint64_t;
using I=__int128_t;
constexpr U q=9007199254429697ULL;
constexpr U t=U(1)<<39, M=U(1)<<25, mm=M-1;
constexpr U envelope=U(1)<<14;
static_assert(q<(U(1)<<53) && q>2*t && (q&1));
inline void require(bool x,const char* why){if(!x)throw std::runtime_error(why);}
inline I ceil_div(I x,I d){return x>=0?(x+d-1)/d:-((-x)/d);}
struct Candidate { U mask=0,phase=0,coarse=0; std::uint8_t low=0,gate=0,high=0; bool accepted=false; };
// For the ORIGINAL SQ return level only: one53-bit prime, t=2^39.
// SEAL's61-bit gamma exceeds2*q. Its source rounding perturbation is less
// than1/gamma; every half-integer distance is at least1/(2*q). Therefore
// the exact native phase is nearest(t*R/q), with no ties. This identity is
// not asserted for the original multi-prime/BN profiles.
inline U scalar_phase(U r){
 require(r<q,"canonical full mask");
 return U((I(2)*t*r+q)/(I(2)*q))&(t-1);
}
inline Candidate propose(U k0,U k1,U random){
 require(k0<(U(1)<<26)&&k1<(U(1)<<26),"post-hash26-bit words");
 Candidate z;
 const U a0=k0&mm,a1=k1&mm,delta=(a1-a0)&mm;
 z.low=std::uint8_t((k0^k1)>>25); z.gate=random&1;
 z.coarse=((z.gate?U(0)-delta:delta)-z.low)&mm;
 z.high=std::uint8_t(1^(z.coarse>>24)^z.gate);
 const U d=(random>>1)&4095,j=(random>>13)&3,offset=(random>>15)&(envelope-1);
 const U own=(z.coarse<<12)|d;
 z.phase=(U(0)-4*own-j)&(t-1);
 const I lo=ceil_div((I(2)*z.phase-1)*q,I(2)*t);
 const I hi=ceil_div((I(2)*z.phase+1)*q,I(2)*t);
 const U count=U(hi-lo);
 require(count<=envelope&&count>0,"full source fiber fits envelope");
 if(offset>=count)return z;
 I r=(lo+offset)%I(q);if(r<0)r+=q;z.mask=U(r);
 require(scalar_phase(z.mask)==z.phase,"source fiber identity");
 require(((U(0)-(z.phase>>2)-U((z.phase&3)!=0))&((U(1)<<37)-1))==own,"exact normalized own");
 z.accepted=true;return z;
}
struct SenderStep { U local=0; std::uint8_t gate=0,h=0; };
inline SenderStep sender_step(U k0,U k1,const Candidate& z,std::uint8_t e){
 require(z.accepted&&e<2,"one accepted mixed row");
 const U u=e?(k1&mm):U(0)-(k0&mm);
 return {(z.gate*(z.coarse+z.low)+u)&mm,z.gate,
  std::uint8_t((z.low*z.gate)^((e?k1:k0)>>25))};
}
struct ReceiverStep { U local=0,c1=0,c2=0; std::uint8_t e=0; };
inline ReceiverStep receiver_step(U key,std::uint8_t r,U b,std::uint8_t chi,std::uint8_t theta){
 require(key<(U(1)<<26)&&r<2&&b<M&&chi<2&&theta<2,"receiver owned values");
 const std::uint8_t e=theta^r,h=key>>25;
 const U u=e?U(0)-(key&mm):key&mm;
 return {(u+theta*(b+chi)-2*U(chi)*h)&mm,
         ((b+chi)*(U(1)-2*theta))&mm,
         (U(chi)*(U(1)-2*h))&mm,e};
}
inline U sender_output(const SenderStep& z,U cot1,U cot2){return (z.local+cot1-2*cot2)&mm;}
// Native additive COT's sender is the client, whose additive product share
// is MINUS its native random message. Both correlations stay width25 so
// the two rows can share one call; no extra interaction to save one bit.
inline U receiver_output(const ReceiverStep& z,U cot1,U cot2){return (z.local-cot1+2*cot2)&mm;}
}
