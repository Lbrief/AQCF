#pragma once
#include <cstdlib>
#include <cstring>
#include "resnet50/core/types.hpp"
namespace direction359 {
inline bool enabled(){const char*p=std::getenv("AQCF_DIRECTION359");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"359 explicit prospective direction");return p[0]=='1';}
template<class B>auto physical_impl(B&s,int)->decltype(s.physical_party){return s.physical_party;}
template<class B>int physical_impl(B&s,long){return s.party;}
template<class B>int physical(B&s){return physical_impl(s,0);}
template<class B>auto sender_impl(B&s,int d,int)->decltype(s.map_sender(d)){return s.map_sender(d);}
template<class B>int sender_impl(B&,int d,long){return d;}
template<class B>int sender(B&s,int d){return sender_impl(s,d,0);}
extern "C" int aqcf_elide359_lookup(aqcf_silent_handle*,int,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
extern "C" int aqcf_leaf359_lookup(aqcf_silent_handle*,int,const uint64_t*,const uint8_t*,uint64_t*,int,unsigned,unsigned,unsigned);
extern "C" int aqcf_common359_phase_begin(aqcf_silent_handle*,int,int,int,int);
}
