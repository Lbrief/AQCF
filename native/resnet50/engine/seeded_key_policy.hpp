#pragma once
#include "resnet50/engine/seeded_key_codec.hpp"
#include <cstdlib>
#include <string>
namespace policy343{
inline bool enabled(){static const bool flag=[](){const char*p=std::getenv("AQCF_SEEDED343");key343::need(p&&(std::string(p)=="0"||std::string(p)=="1"),"343 explicit fixed key-wire mode");return p[0]=='1';}();return flag;}
}
