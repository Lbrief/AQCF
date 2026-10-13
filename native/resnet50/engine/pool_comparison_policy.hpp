#pragma once
#include "resnet50/engine/protected_pool.hpp"
namespace aqcf_pool258 {
inline bool enabled(){const char* p=std::getenv("AQCF_POOL258");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"258 explicit fixed public arm");return p[0]=='1';}
}
