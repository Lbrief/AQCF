#pragma once
#include "resnet50/protocol/native_ot_bridge.h"
#include <stdint.h>
typedef struct aqcf_pool263_stats {
 uint64_t prepared_banks,consumed_banks,prepared_rows,consumed_rows;
 uint64_t native_rot_calls,native_rot_rows,early_bytes,correction_bytes,table_bytes;
 uint64_t live_banks,peak_live_banks;
} aqcf_pool263_stats;
extern "C" int aqcf_pool263_prepare(aqcf_silent_handle*,int sender,int rows,int banks,uint64_t* ids,uint8_t* alpha);
extern "C" int aqcf_pool263_consume(aqcf_silent_handle*,uint64_t id,const uint8_t* digit,const uint8_t* carry,const uint8_t* mask,uint8_t* result,int rows);
extern "C" int aqcf_pool263_read_stats(const aqcf_silent_handle*,aqcf_pool263_stats*);
