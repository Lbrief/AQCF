#pragma once
#include "resnet50/engine/input_phase_codec.hpp"
#include <atomic>
namespace aqcf_input371 {
namespace b=aqcf_bn19;using b::U;
constexpr U original_size=8+b::Codec::prefix+70*b::N/8+b::Codec::suffix;
inline std::atomic<U> packets{0},rows{0},old_bytes{0},new_bytes{0};
inline bool enabled(){const char*p=std::getenv("AQCF_INPUT371");b::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"371 explicit input codec");return p[0]=='1';}
inline U wire_size(const b::Geometry&g,U group){return enabled()?aqcf_input370::wire_size(g,group):original_size;}
inline void record(const b::Geometry&g,U group,U size){b::require(size==wire_size(g,group),"371 actual fixed public wire");++packets;rows.fetch_add(U(g.L)*b::count(g,group));old_bytes.fetch_add(original_size);new_bytes.fetch_add(size);}
inline aqcf_input186::Packet generate(b::Lane&lane,const b::Geometry&g,U group){
 auto packet=enabled()?aqcf_input370::generate(lane,g,group):aqcf_input186::generate(lane);
 record(g,group,packet.wire.size());return packet;
}
inline std::string restore(const b::Lane&lane,const b::Bytes&wire,const b::Geometry&g,U group){
 auto raw=enabled()?aqcf_input370::restore(lane,wire,g,group):aqcf_input186::restore(lane,wire);
 record(g,group,wire.size());return raw;
}
inline void census(int party){
 b::require(packets==2132&&rows==13371904&&old_bytes==153252424&&new_bytes==(enabled()?67290184:153252424),"371 original full model public input census");
 std::cout<<"INPUT371_CENSUS "<<party<<" "<<enabled()<<" "<<packets<<" "<<rows<<" "<<old_bytes<<" "<<new_bytes<<"\n";
}
}
