#pragma once
#include "resnet50/engine/mixed_affine_quotient.hpp"
namespace aqcf_mixed238 {
inline bool enabled(){const char* p=std::getenv("AQCF_MIXED238");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"238 explicit fixed public arm");return p[0]=='1';}
}
