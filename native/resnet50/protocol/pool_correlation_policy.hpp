#pragma once
#include "resnet50/protocol/owned_correlation_fifo.hpp"
#include <cstdlib>
#include <cstring>
namespace fifo444 { inline bool enabled(){const char*p=std::getenv("AQCF_POOLFLOOR444");fifo443::need(p&&(!std::strcmp(p,"0")||!std::strcmp(p,"1")),"444 explicit composed candidate arm");return p[0]=='1';}}
