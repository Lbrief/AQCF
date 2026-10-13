#pragma once
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace aqcf_packing408 {
inline bool enabled(){const char* p=std::getenv("AQCF_PACK408");if(!p||(std::strcmp(p,"0")&&std::strcmp(p,"1")))throw std::runtime_error("408 explicit public packing arm");return p[0]=='1';}
}
