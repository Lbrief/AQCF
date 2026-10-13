#pragma once
#include "squeezenet/engine/packed_phase_wire.hpp"
#include <boost/multiprecision/cpp_int.hpp>
namespace aqcf_input376 {
using U=uint64_t;using W=__uint128_t;using aqcf_wire288::need;
constexpr U N=8192,q0=9007199254429697ULL,q1=2199023190017ULL,T=U(1)<<37;
constexpr W Q=W(q0)*q1;
constexpr unsigned shift=23,bits=71;
constexpr size_t prefix=113,suffix=81,raw_size=prefix+2*N*8+suffix;
constexpr size_t packed_size=8+prefix+bits*N/8+suffix;
inline U powmod(U a,U e,U q){U v=1;for(;e;e>>=1,a=U(W(a)*a%q))if(e&1)v=U(W(v)*a%q);return v;}
inline W crt(U a,U b){need(a<q0&&b<q1,"376 canonical source residues");static const U inv=powmod(q0%q1,q1-2,q1);const U d=(b+q1-a%q1)%q1;return W(a)+W(q0)*U(W(d)*inv%q1);}
inline void bound(){
 const W e=W(22)*(W(1)<<30)+4098,extra=W(1)<<(30+shift-1),after=(e+extra+q1-1)/q1+4098;
 need(8*W(T)*(e+extra)<Q&&8*W(T)*after<q0,"376 original strict full and projected quarter bounds");
 need(after==6147&&((Q-1+(W(1)<<(shift-1)))>>shift)<(W(1)<<bits),"376 fixed23bit removal and71bit word");
}
inline std::string pack(const std::string& raw){
 bound();aqcf_wire288::validate(raw);std::string out(packed_size,'\0');std::memcpy(out.data(),"AQSQ376A",8);
 std::memcpy(out.data()+8,raw.data(),prefix);std::memcpy(out.data()+packed_size-suffix,raw.data()+raw_size-suffix,suffix);
 W pending=0;unsigned live=0;size_t at=8+prefix;
 for(U j=0;j<N;++j){const W z=crt(aqcf_wire288::word(raw.data()+prefix+8*j),aqcf_wire288::word(raw.data()+prefix+8*(N+j)));
  const W word=(z+(W(1)<<(shift-1)))>>shift;need(word<(W(1)<<bits),"376 word overflow");pending|=word<<live;live+=bits;
  while(live>=8){out[at++]=char(U(pending)&255);pending>>=8;live-=8;}
 }
 need(!live&&!pending&&at==packed_size-suffix,"376 exact public frame");return out;
}
inline std::string unpack(const std::string& wire){
 bound();need(wire.size()==packed_size&&wire.compare(0,8,"AQSQ376A")==0,"376 public frame identity");
 std::string out(raw_size,'\0');std::memcpy(out.data(),wire.data()+8,prefix);std::memcpy(out.data()+raw_size-suffix,wire.data()+packed_size-suffix,suffix);
 const W maxword=(Q-1+(W(1)<<(shift-1)))>>shift;W pending=0;unsigned live=0;size_t at=8+prefix;
 for(U j=0;j<N;++j){while(live<bits){pending|=W(static_cast<unsigned char>(wire[at++]))<<live;live+=8;}
  const W word=pending&((W(1)<<bits)-1);pending>>=bits;live-=bits;need(word<=maxword,"376 canonical rounded word");const W z=word<<shift;
  aqcf_wire288::putword(out.data()+prefix+8*j,U(z%q0));aqcf_wire288::putword(out.data()+prefix+8*(N+j),U(z%q1));
 }
 need(!live&&!pending&&at==packed_size-suffix,"376 exact decode cursor");aqcf_wire288::validate(out);return out;
}
}
