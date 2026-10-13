#pragma once
#include "types.hpp"
#include <utility>
namespace aqcf2 {
// Generic radix GT/EQ tree versus a PRIVATE-initialized decision scan.
// All topology and table widths are public. Only owned operands enter the OT.
// Native author GT/EQ remains a separate strong control in kernels.hpp.
template<class Backend>
Bits decision_tree(Backend& s, const std::vector<U>& low,
                   const Bits& parity, unsigned f, bool folded) {
  require((f == 6 || f == 16) && parity.v.size() == low.size(),
          "decision shape/shift");
  parity.check();
  constexpr unsigned radix = 4;
  const unsigned digits = (f + radix - 1) / radix;
  const std::size_t n = low.size();
  auto owned_digit = [&](unsigned digit, std::size_t j) {
    U value = low[j] & mask(f);
    if (s.party == 2) value = mask(f) - value;
    return (value >> (radix * digit)) & U(15);
  };
  if (folded) {
    // Lowest digit: 32 choices, ONE output bit. The second receiver bit is
    // BOB's private tie parity. No unused E0 bit or dummy table is sent.
    std::vector<U> choices(n), table;
    std::vector<std::uint8_t> own_mask;
    if (s.party == 1) { table.resize(n * 32); own_mask = s.random_small(n); }
    for (std::size_t j = 0; j < n; ++j) {
      const U a = owned_digit(0, j);
      choices[j] = a | (U(parity.v[j]) << radix);
      if (s.party == 1) for (U choice = 0; choice < 32; ++choice) {
        const U b = choice & 15;
        const U tie = U(parity.v[j]) ^ (choice >> radix);
        const U decision = U(a > b) ^ (U(a == b) & tie);
        table[j * 32 + choice] = decision ^ U(own_mask[j] & 1);
      }
    }
    auto received = s.lookup_words(table, choices, n, 5, 1);
    Bits state(n);
    for (std::size_t j = 0; j < n; ++j)
      state.v[j] = (s.party == 1 ? own_mask[j] : received[j]) & 1;

    // All upper digits stay ordinary 16-choice / two-output-bit tables.
    // This is a second real OT call and is included in all costs.
    const std::size_t rows = (digits - 1) * n;
    choices.assign(rows, 0); table.clear(); own_mask.clear();
    if (s.party == 1) { table.resize(rows * 16); own_mask = s.random_small(rows); }
    for (unsigned d = 1; d < digits; ++d) for (std::size_t j = 0; j < n; ++j) {
      const std::size_t row = (d - 1) * n + j;
      const U a = owned_digit(d, j); choices[row] = a;
      if (s.party == 1) for (U b = 0; b < 16; ++b)
        table[row * 16 + b] = (U(a > b) | (U(a == b) << 1)) ^ U(own_mask[row] & 3);
    }
    received = s.lookup_words(table, choices, rows, 4, 2);
    // Fixed three-step scan for f=16; one step for f=6. It computes no
    // prefix equality state: the desired tie policy is already in state.
    for (unsigned d = 1; d < digits; ++d) {
      Bits greater(n), equal(n);
      for (std::size_t j = 0; j < n; ++j) {
        const std::size_t row = (d - 1) * n + j;
        const U value = s.party == 1 ? own_mask[row] : received[row];
        greater.v[j] = value & 1; equal.v[j] = (value >> 1) & 1;
      }
      state = s.xor_(greater, s.and_(equal, state));
    }
    return state;
  }

  // Generic explicit-tree control. Same leaf implementation family, public
  // radix and message widths; complete GT/EQ followed by the private tie bit.
  const std::size_t rows = digits * n;
  std::vector<U> choices(rows), table;
  std::vector<std::uint8_t> own_mask;
  if (s.party == 1) { table.resize(rows * 16); own_mask = s.random_small(rows); }
  for (unsigned d = 0; d < digits; ++d) for (std::size_t j = 0; j < n; ++j) {
    const std::size_t row = d * n + j;
    const U a = owned_digit(d, j); choices[row] = a;
    if (s.party == 1) for (U b = 0; b < 16; ++b)
      table[row * 16 + b] = (U(a > b) | (U(a == b) << 1)) ^ U(own_mask[row] & 3);
  }
  auto received = s.lookup_words(table, choices, rows, 4, 2);
  struct Node { Bits g, e; };
  std::vector<Node> level;
  for (unsigned d = 0; d < digits; ++d) {
    Node node{Bits(n), Bits(n)};
    for (std::size_t j = 0; j < n; ++j) {
      const U v = s.party == 1 ? own_mask[d*n+j] : received[d*n+j];
      node.g.v[j] = v & 1; node.e.v[j] = (v >> 1) & 1;
    }
    level.push_back(std::move(node));
  }
  while (level.size() > 1) {
    std::vector<std::pair<Bits,Bits>> pairs;
    for (std::size_t j = 0; j + 1 < level.size(); j += 2) {
      pairs.emplace_back(level[j+1].e, level[j].g);
      pairs.emplace_back(level[j+1].e, level[j].e);
    }
    auto products = s.and_batch(pairs);
    std::vector<Node> next;
    for (std::size_t j = 0, at = 0; j + 1 < level.size(); j += 2, at += 2)
      next.push_back({s.xor_(level[j+1].g, products[at]), products[at+1]});
    if (level.size() % 2) next.push_back(std::move(level.back()));
    level = std::move(next);
  }
  return s.xor_(level[0].g, s.and_(level[0].e, parity));
}
}
