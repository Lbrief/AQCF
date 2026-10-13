#pragma once
#include <cstdint>
#include <stdexcept>
namespace quotient169 {
inline void require(bool value) {
  if (!value) throw std::invalid_argument("quotient169 noncanonical input");
}
struct Correction { uint8_t wire, quotient; };
inline uint8_t early(unsigned alpha, unsigned k0, unsigned k1) {
  require(alpha < 2 && k0 < 4 && k1 < 4);
  return (alpha + k0 + k1) & 3;
}
inline Correction correct(unsigned y, unsigned beta, unsigned r, unsigned kr,
                          unsigned D, unsigned R) {
  require(y < 4 && beta < 2 && r < 2 && kr < 4 && D < 4 && R < 4);
  const unsigned Q = beta ? ((D + 4 - kr) & 3) : kr;
  const unsigned V = (beta + 8 - 2 * Q) & 7;
  const unsigned W = y + V;
  return {uint8_t((beta ^ r) | (((W & 3) ^ R) << 1)),
          uint8_t((W >> 2) & 1)};
}
inline uint8_t table(unsigned x, unsigned alpha, unsigned mu,
                     unsigned k0, unsigned k1, unsigned correction,
                     const uint8_t* pads) {
  require(x < 4 && alpha < 2 && mu < 2 && k0 < 4 && k1 < 4
          && correction < 8 && pads);
  const unsigned K = (correction & 1) ? k1 : k0;
  const unsigned U = x + ((alpha + 2 * K) & 7);
  const unsigned d = correction >> 1;
  unsigned answer = 0;
  for (unsigned j = 0; j < 4; ++j) {
    require(pads[j] < 2);
    const unsigned bit = (((U & 3) + (j ^ d)) >= 4)
                       ^ ((U >> 2) & 1) ^ mu ^ pads[j];
    answer |= bit << j;
  }
  return answer;
}
inline uint8_t finish(unsigned encrypted, unsigned R, unsigned pad, unsigned quotient) {
  require(encrypted < 16 && R < 4 && pad < 2 && quotient < 2);
  return ((encrypted >> R) & 1) ^ pad ^ quotient;
}
}
