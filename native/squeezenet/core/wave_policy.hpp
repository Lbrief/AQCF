#pragma once
#include <cstddef>
#ifndef AQCF_WAN_WAVE
#define AQCF_WAN_WAVE 0
#endif
#ifndef AQCF_LAYER_FRONTIER
#define AQCF_LAYER_FRONTIER 0
#endif
namespace aqcf_wan_wave {
// Public scheduling only. Do not reduce precision, reveal carries, reuse OT
// randomness, move setup offline, or change the frozen carry recurrence.
constexpr std::size_t relu_limit = AQCF_LAYER_FRONTIER ? 262144 : (AQCF_WAN_WAVE ? 32768 : 4096);
constexpr std::size_t pool_limit = AQCF_LAYER_FRONTIER ? 262144 : (AQCF_WAN_WAVE ? 28672 : 16384);
static_assert(AQCF_LAYER_FRONTIER ? relu_limit<=262144 : relu_limit * 8 / 4 <= 65536, "ReLU tuple-4 bank bound");
static_assert(AQCF_LAYER_FRONTIER ? pool_limit<=262144 : (pool_limit * 9 + 3) / 4 <= 65536, "pool tuple-4 bank bound");
}
