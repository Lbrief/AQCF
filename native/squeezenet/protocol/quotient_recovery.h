#pragma once
#include "squeezenet/protocol/native_ot_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  uint64_t prepared_calls, prepared_rows, consumed_calls, consumed_rows;
  uint64_t native_rot_rows, live_banks, peak_live_banks;
  uint64_t explicit_early_bytes, explicit_correction_bytes, explicit_table_bytes;
} aqcf_quotient169_stats;
// Sender owns alpha. Receiver passes null. All masks for a chain are freshly
// sampled by its actual logical sender before preparation, never by a dealer.
int aqcf_quotient169_prepare(aqcf_silent_handle*,int sender,
                           const uint8_t* alpha,int rows,uint64_t* id);
// Sender owns x,alpha,mu; receiver owns y,beta and passes null mask.
// On ANY nonzero return the caller must close both transport directions and
// destroy the handle. No retry/resume/cancel or consumed-bank reuse is legal.
int aqcf_quotient169_consume(aqcf_silent_handle*,uint64_t id,
                           const uint8_t* digit,const uint8_t* carry,
                           const uint8_t* mask,uint8_t* result,int rows);
// Call only with the owning lane quiescent. -2 means outstanding live banks;
// -1 means invalid/poisoned handle; counters are still returned when poisoned.
int aqcf_quotient169_read_stats(const aqcf_silent_handle*,aqcf_quotient169_stats*);
#ifdef __cplusplus
}
#endif
