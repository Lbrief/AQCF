#pragma once
#include "resnet50/engine/input_mask_codec.hpp"
namespace aqcf_input370 {
namespace b=aqcf_bn19;using b::U;using b::W;
inline bool selected(const b::Geometry&g,U group,U j){
 b::require(g.C>=2&&g.C<=128&&!(g.C&(g.C-1))&&U(g.L)*g.C<=b::N,"370 fixed public packing");
 return j/g.C<g.L&&j%g.C<b::count(g,group);
}
inline U wire_size(const b::Geometry&g,U group){return 8+b::Codec::prefix+(U(g.L)*b::count(g,group)*aqcf_input369::bits+7)/8+b::Codec::suffix;}
inline aqcf_input186::Packet project(const b::Lane&lane,aqcf_input186::Packet packet,const b::Geometry&g,U group){
 aqcf_input369::bound(lane);const auto&old=packet.wire;
 b::require(old.size()==aqcf_input369::wire_size&&!std::memcmp(old.data(),"AQPI369A",8),"370 bound parent packet");
 b::Bytes body(old.begin()+8+b::Codec::prefix,old.end()-b::Codec::suffix);b::Bits::Reader reader{body};b::Bits::Writer writer;
 U used=0;for(U j=0;j<b::N;++j){U value=reader.get(aqcf_input369::bits);if(selected(g,group,j)){writer.put(value,aqcf_input369::bits);++used;}}
 reader.end();b::require(used==U(g.L)*b::count(g,group),"370 exact disclosed support");auto payload=writer.finish();
 b::Bytes wire(wire_size(g,group));std::memcpy(wire.data(),"AQPI370A",8);
 std::memcpy(wire.data()+8,old.data()+8,b::Codec::prefix);std::copy(payload.begin(),payload.end(),wire.begin()+8+b::Codec::prefix);
 std::memcpy(wire.data()+wire.size()-b::Codec::suffix,old.data()+old.size()-b::Codec::suffix,b::Codec::suffix);
 OPENSSL_cleanse(body.data(),body.size());OPENSSL_cleanse(packet.wire.data(),packet.wire.size());packet.wire=std::move(wire);return packet;
}
inline aqcf_input186::Packet generate(b::Lane&lane,const b::Geometry&g,U group){return project(lane,aqcf_input369::generate(lane),g,group);}
inline std::string restore(const b::Lane&lane,const b::Bytes&wire,const b::Geometry&g,U group){
 aqcf_input369::bound(lane);b::require(wire.size()==wire_size(g,group)&&!std::memcmp(wire.data(),"AQPI370A",8),"370 exact public support frame");
 std::string raw(b::Codec::raw_size,'\0');std::memcpy(raw.data(),wire.data()+8,b::Codec::prefix);
 std::memcpy(raw.data()+raw.size()-b::Codec::suffix,wire.data()+wire.size()-b::Codec::suffix,b::Codec::suffix);
 b::Bytes body(wire.begin()+8+b::Codec::prefix,wire.end()-b::Codec::suffix);b::Bits::Reader reader{body};
 const W maxword=(lane.codec->Q/b::T-1+(W(1)<<(aqcf_input369::shift-1)))>>aqcf_input369::shift;
 for(U j=0;j<b::N;++j){U word=selected(g,group,j)?reader.get(aqcf_input369::bits):0;
  b::require(W(word)<=maxword,"370 canonical public support residual");W u=W(word)<<aqcf_input369::shift;
  for(U k=0;k<3;++k){U v=U(u%lane.codec->q[k]);if(v)v=lane.codec->q[k]-v;aqcf_seeded_wire::put(raw.data()+b::Codec::prefix+8*(k*b::N+j),v);}}
 reader.end();lane.codec->validate_seeded(raw);return raw;
}
}

