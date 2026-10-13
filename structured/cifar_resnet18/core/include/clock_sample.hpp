#pragma once
#include "types.hpp"
#include <time.h>
namespace aqcf2 {
inline double clock_value(clockid_t which){timespec t{};require(clock_gettime(which,&t)==0,"required clock read");return double(t.tv_sec)+double(t.tv_nsec)*1e-9;}
struct ClockSample {double monotonic,raw,realtime;
 static ClockSample now(){return {clock_value(CLOCK_MONOTONIC),clock_value(CLOCK_MONOTONIC_RAW),clock_value(CLOCK_REALTIME)};}
 ClockSample since(const ClockSample& b)const{return {monotonic-b.monotonic,raw-b.raw,realtime-b.realtime};}
};
}
