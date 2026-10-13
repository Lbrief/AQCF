#pragma once
#include <cstdlib>
#include <cstring>
#include "resnet50/engine/retained_resnet_component.hpp"
namespace retained307 {inline bool enabled(){const char*p=std::getenv("AQCF_RETAINED307");retain298::need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"307 explicit BN retained arm");return p[0]=='1';}}
