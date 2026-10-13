#pragma once
#include "squeezenet/protocol/native_ot_bridge.h"
#include "squeezenet/engine/types.hpp"
#include <cstdlib>
#include <cstring>
extern "C" int aqcf_poolmux426(aqcf_silent_handle*,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned);
extern "C" int aqcf_poolmux426_stats(const aqcf_silent_handle*,uint64_t*);
namespace aqcf_pool426 {
inline bool enabled(){const char*p=std::getenv("AQCF_POOLMUX426");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"426 explicit paired pool mux");return p[0]=='1';}
template<class B>aqcf2::Ring select(B&s,const aqcf2::Bits&g,const aqcf2::Ring&a){
 if(!enabled())return s.mux(g,a);
 a.check();g.check();aqcf2::require(a.v.size()==g.v.size()&&(a.width==25||a.width==37),"426 complete pool selection");
 aqcf2::Ring out(a.width,a.v.size());auto*h=s.control().owned189_handle();
 aqcf2::require(aqcf_poolmux426(h,a.v.data(),g.v.data(),out.v.data(),int(a.v.size()),a.width)==0,aqcf_silent_error(h));
 s.flush();s.mux_elements+=a.v.size();out.check();return out;
}
}
