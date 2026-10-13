#pragma once
// Portability adapter, NOT a new cryptographic primitive or a speedup claim.
// Reuses the frozen AQCF linked-cofactor/direct-emission implementation and
// its equally optimized prepared/materialized control. No input is opened.
#include "squeezenet/engine/boundary_action.hpp"

namespace aqcf2 {
enum class BackboneCarry { Linked, PreparedControl };

template<class B>
std::vector<Bits> backbone_carries(B& s, const std::vector<CarryTask>& tasks,
                                 BackboneCarry method, U low_n, U high_n) {
    if (method == BackboneCarry::Linked) {
        auto c = linked_cofactor_execute(s, tasks);
        s.note_linked_execution(low_n, high_n, c.packet_rows,
                               c.packet_table_bits, c.selector_rows);
        return std::move(c.value);
    }
    auto c = carry_materialized_prepared(s, tasks);
    s.prepared_control_elements += low_n + high_n;
    s.note_carry_execution(false, low_n, high_n, c.logical_transitions,
                          c.ot_rows, c.table_bits, c.ot_calls,
                          c.nonlinear_and_elements);
    return std::move(c.value);
}

// floor(center_M(x) / 2^f), optionally followed by ReLU.
// Caller must establish |center_M(x)| < 2^53 with an admitted model/range
// certificate; observed sample ranges do not establish this precondition.
// f<=30 leaves sufficient margin for the coarse quotient before low carry.
// Unlike Q16 RTE, FLOOR has zero carry-in and no half/parity bias.
template<class B>
Ring backbone_floor(B& s, const Ring& x, unsigned f, bool relu,
                    BackboneCarry method, bool direct) {
    x.check();
    require(x.width == 55 && f >= 1 && f <= 30 &&
            !x.v.empty() && x.v.size() <= 4096, "backbone floor domain");
    const std::size_t n = x.v.size();
    Ring coarse(55 - f, n);
    std::vector<U> low(n);
    for (std::size_t j = 0; j < n; ++j) {
        low[j] = x.v[j] & mask(f);
        coarse.v[j] = x.v[j] >> f;
    }
    auto decisions = s.boundary_profile("backbone_floor_decisions", [&]() {
        std::vector<CarryTask> tasks{{&low, f, s.constant(false, n), true}};
        if (relu)
            tasks.push_back({&coarse.v, coarse.width - 1,
                             s.constant(false, n), true});
        return backbone_carries(s, tasks, method, n, relu ? n : 0);
    });
    return s.boundary_profile("backbone_floor_emission", [&]() {
        if (direct)
            return relu ? boundary_relu_emit(s, coarse, decisions[0], decisions[1])
                        : boundary_signed_emit(s, coarse, decisions[0]);
        Ring q = coarse;
        auto delta = s.b2a(decisions[0], q.width);
        for (std::size_t j = 0; j < n; ++j)
            q.v[j] = (q.v[j] + delta.v[j]) & mask(q.width);
        if (!relu) return signed_lift(s, q);
        auto sign = s.xor_(decisions[1], s.local_bit(coarse.v, coarse.width - 1));
        return positive_lift(s, s.mux(s.not_(sign), q));
    });
}

// Full-width shared ReLU: no old model-specific 39-bit narrowing assumption.
template<class B>
Ring backbone_relu(B& s, const Ring& x, BackboneCarry method) {
    x.check();
    require(x.width == 55 && !x.v.empty() && x.v.size() <= 4096,
            "backbone ReLU domain");
    std::vector<CarryTask> tasks{{&x.v, 54, s.constant(false, x.v.size()), true}};
    auto c = backbone_carries(s, tasks, method, 0, x.v.size());
    auto sign = s.xor_(c[0], s.local_bit(x.v, 54));
    return s.mux(s.not_(sign), x);
}

// Standard ReLU6 composition at output scale Q12. Both secure ReLUs are
// included; the clipping activation is not dropped or rebranded as original.
template<class B>
Ring backbone_floor_relu6(B& s, const Ring& x, unsigned f,
                          BackboneCarry method, bool direct) {
    Ring y = backbone_floor(s, x, f, true, method, direct);
    Ring shifted = y;
    if (s.party == 1)
        for (auto& v : shifted.v) v = (v - U(6 * 4096)) & mask(55);
    auto excess = backbone_relu(s, shifted, method);
    for (std::size_t j = 0; j < y.v.size(); ++j)
        y.v[j] = (y.v[j] - excess.v[j]) & mask(55);
    return y;
}
} // namespace aqcf2
