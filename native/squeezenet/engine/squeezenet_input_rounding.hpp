#pragma once
#include "squeezenet/engine/squeezenet_input_floor.hpp"
namespace aqcf_input379 {
inline bool enabled(){const char*p=std::getenv("AQCF_SQ_INPUT379");aqcf_wire288::need(p&&(std::string(p)=="0"||std::string(p)=="1"),"379 explicit original-or-rounded input arm");return p[0]=='1';}
inline size_t frame_size(){aqcf_wire288::need(aqcf_wire288::enabled(),"379 original packed input retained");return enabled()?aqcf_input376::packed_size:aqcf_wire288::PACKED;}
inline std::string pack(const std::string&s){return enabled()?aqcf_input376::pack(s):aqcf_wire288::pack(s);}
inline std::string unpack(const std::string&s){return enabled()?aqcf_input376::unpack(s):aqcf_wire288::unpack(s);}
}
