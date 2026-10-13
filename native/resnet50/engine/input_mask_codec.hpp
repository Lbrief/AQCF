#pragma once
#include "resnet50/engine/input_program.hpp"
namespace aqcf_input369 {
namespace b=aqcf_bn19;using b::U;using b::W;
constexpr unsigned shift=30,bits=70-shift;
constexpr U wire_size=8+b::Codec::prefix+bits*b::N/8+b::Codec::suffix;
inline void bound(const b::Lane& lane){
 const W Q=lane.codec->Q;const W N=b::N,T=b::T,C=128,Wcap=W(1)<<30;
 W digits=0;for(U q:lane.codec->q)digits+=q-1;
 // Original special prime is >=2^37. This larger K is public and conservative.
 const W special_lower=W(1)<<37,K=(N*21*digits+special_lower-1)/special_lower+(N+1);
 const W E=C*C*(22+K)*Wcap+8194;
 const W original_wire=(W(1)<<64)+N*(W(1)<<51);
 const W added=C*Wcap*(W(1)<<(shift-1));
 b::require(8*T*(E+original_wire+added)<Q,"369 original quarter correctness margin plus exact induced rounding error");
 b::require(((Q/T-1+(W(1)<<(shift-1)))>>shift)<(W(1)<<bits),"369 public rounded residual fits fixed word");
}
inline aqcf_input186::Packet compress(const b::Lane& lane,aqcf_input186::Packet packet){
 bound(lane);
 const auto& old=packet.wire;b::Bytes body(old.begin()+8+b::Codec::prefix,old.end()-b::Codec::suffix);
 b::Bits::Reader read{body};b::Bits::Writer writer;
 for(U j=0;j<b::N;++j){W u=read.get(35);u|=W(read.get(35))<<35;
  b::require(u<lane.codec->Q/b::T,"369 canonical original residual");
  const W h=(u+(W(1)<<(shift-1)))>>shift,rounded=h<<shift;
  const W error=rounded>=u?rounded-u:u-rounded;
  b::require(error<=(W(1)<<(shift-1))&&h<(W(1)<<bits),"369 deterministic nearest rounding");
  writer.put(U(h),bits);
 }
 read.end();auto packed=writer.finish();b::Bytes wire(wire_size);
 std::memcpy(wire.data(),"AQPI369A",8);
 std::memcpy(wire.data()+8,old.data()+8,b::Codec::prefix);
 std::copy(packed.begin(),packed.end(),wire.begin()+8+b::Codec::prefix);
 std::memcpy(wire.data()+wire.size()-b::Codec::suffix,old.data()+old.size()-b::Codec::suffix,b::Codec::suffix);
 OPENSSL_cleanse(body.data(),body.size());OPENSSL_cleanse(packet.wire.data(),packet.wire.size());
 packet.wire=std::move(wire);return packet;
}
inline aqcf_input186::Packet generate(b::Lane& lane){return compress(lane,aqcf_input186::generate(lane));}
inline std::string restore(const b::Lane& lane,const b::Bytes& wire){
 bound(lane);b::require(wire.size()==wire_size&&!std::memcmp(wire.data(),"AQPI369A",8),"369 fixed bound input format");
 std::string raw(b::Codec::raw_size,'\0');
 std::memcpy(raw.data(),wire.data()+8,b::Codec::prefix);
 std::memcpy(raw.data()+raw.size()-b::Codec::suffix,wire.data()+wire.size()-b::Codec::suffix,b::Codec::suffix);
 b::Bytes body(wire.begin()+8+b::Codec::prefix,wire.end()-b::Codec::suffix);b::Bits::Reader reader{body};
 const W maxword=(lane.codec->Q/b::T-1+(W(1)<<(shift-1)))>>shift;
 for(U j=0;j<b::N;++j){U word=reader.get(bits);b::require(W(word)<=maxword,"369 canonical bounded quantization word");W u=W(word)<<shift;
  for(U k=0;k<3;++k){U v=U(u%lane.codec->q[k]);if(v)v=lane.codec->q[k]-v;aqcf_seeded_wire::put(raw.data()+b::Codec::prefix+8*(k*b::N+j),v);}}
 reader.end();lane.codec->validate_seeded(raw);return raw;
}
}

