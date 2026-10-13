#pragma once
#include "resnet50/engine/horner_polynomial.hpp"
#include <cstdlib>
#include <cstring>
namespace horner293 {
inline bool enabled(){const char* p=std::getenv("AQCF_HORNER292");aqcf_bn19::require(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"293 explicit fixed arm");return p[0]=='1';}
}
