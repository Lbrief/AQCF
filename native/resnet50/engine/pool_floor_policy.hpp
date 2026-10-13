#pragma once
#include "resnet50/engine/pool_carry_consumer.hpp"
#include <cstdlib>
#include <cstring>
namespace pool295 {
inline bool enabled(){const char* p=std::getenv("AQCF_POOL295");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"295 explicit fixed pool arm");return p[0]=='1';}
}
