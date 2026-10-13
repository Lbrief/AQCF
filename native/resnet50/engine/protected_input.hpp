#pragma once
#include "resnet50/engine/input_program.hpp"
#include <cstdlib>
#include <string>
namespace aqcf_input213 {
inline bool enabled(){static const bool flag=[](){const char* p=std::getenv("AQCF_DOT_INPUT213");aqcf_bn19::require(p&&(std::string(p)=="0"||std::string(p)=="1"),"213 explicit fixed input flag");return p[0]=='1';}();return flag;}
constexpr std::uint64_t wire_size=8+aqcf_bn19::Codec::prefix+70*aqcf_bn19::N/8+aqcf_bn19::Codec::suffix;
}
