#pragma once
#include <array>
#include <cstdint>
namespace aqcf3::pairprofile {
inline constexpr std::array<std::uint64_t,5> Q{{35184371613697ULL,35184371417089ULL,35184371138561ULL,35184371089409ULL,274877562881ULL}};
inline constexpr unsigned H_EXP=119,H_MULT=15,H113_DEN=960;
inline constexpr std::uint64_t PAIR_RETURNS=1834;
}
