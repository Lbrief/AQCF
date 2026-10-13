#pragma once
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace aqcf_pipeline416 {
inline bool enabled(){const char*p=std::getenv("AQCF_PIPE416");if(!p||(std::strcmp(p,"0")&&std::strcmp(p,"1")))throw std::runtime_error("416 explicit pipeline arm");return p[0]=='1';}
}
