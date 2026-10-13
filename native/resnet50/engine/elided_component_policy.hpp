#pragma once
#include "resnet50/engine/elided_affine_quotient.hpp"
namespace aqcf_elide250 {
inline bool enabled(){const char* p=std::getenv("AQCF_ELIDE250");aqcf2::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"250 explicit fixed public arm");return p[0]=='1';}
}
