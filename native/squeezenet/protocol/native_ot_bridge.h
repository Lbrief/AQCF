#pragma once
#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct aqcf_silent_handle aqcf_silent_handle;
typedef struct {
  void* context;
  void (*send)(void*, const void*, int);
  void (*recv)(void*, void*, int);
  void (*flush)(void*);
} aqcf_silent_io;
typedef struct {
  uint64_t lookup_calls, lookup_rows, cot_calls, cot_rows;
  uint64_t send_bytes, recv_bytes, setup_send_bytes, setup_recv_bytes;
  uint64_t directions_initialized;
  double setup_seconds;
} aqcf_silent_stats;
aqcf_silent_handle* aqcf_silent_create(int party, aqcf_silent_io forward,
                                      aqcf_silent_io reverse);
void aqcf_silent_destroy(aqcf_silent_handle*);
const char* aqcf_silent_error(const aqcf_silent_handle*);
int aqcf_silent_read_stats(const aqcf_silent_handle*, aqcf_silent_stats*);
// Existing native random-message/random-choice OT, not chosen-message COT.
// Only the sender receives k0/k1; only the receiver receives random t and kt.
int aqcf_silent_rot_bits(aqcf_silent_handle*,int sender,uint8_t* k0,uint8_t* k1,
                        uint8_t* t,uint8_t* kt,int rows);
int aqcf_silent_read_rot_stats(const aqcf_silent_handle*,uint64_t* calls,uint64_t* rows);
// Only the sender supplies the full row-major table. Only the receiver supplies
// choices. Result is zero on sender and the selected word on receiver.
int aqcf_silent_lookup(aqcf_silent_handle*, int sender, const uint64_t* table,
                      const uint8_t* choice, uint64_t* result,
                      int rows, int input_bits, int output_bits);
// Additive correlation modulo 2^width. Sender obtains random r; receiver
// obtains r + choice*corr. No plaintext sharing conversion is performed here.
int aqcf_silent_cot(aqcf_silent_handle*, int sender, const uint64_t* correlation,
                   const bool* choice, uint64_t* result, int rows, int width);
#ifdef __cplusplus
}
#endif

extern "C" int aqcf_silent_joint26(aqcf_silent_handle*,uint64_t*,uint64_t*,bool*,uint64_t*,int);
extern "C" int aqcf_silent_joint26_stats(const aqcf_silent_handle*,uint64_t*,uint64_t*);
