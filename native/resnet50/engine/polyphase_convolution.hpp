#pragma once
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace aqcf_polyphase458 {
inline bool enabled(){const char*p=std::getenv("AQCF_POLYPHASE458");if(!p||(std::strcmp(p,"0")&&std::strcmp(p,"1")))throw std::runtime_error("458 explicit public packing arm");return p[0]=='1';}
}
