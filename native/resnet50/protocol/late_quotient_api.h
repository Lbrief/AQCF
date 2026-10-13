#pragma once
#include <stdint.h>
extern "C" {
int aqcf_late37_prepare(aqcf_silent_handle*,int,const uint8_t*,const uint8_t*,int,uint64_t*);
int aqcf_late37_consume(aqcf_silent_handle*,uint64_t,const uint8_t*,const uint8_t*,const uint8_t*,uint8_t*,int);
int aqcf_late37_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*,uint64_t*);
}
