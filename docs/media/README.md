# AQCF algorithm animation

[`aqcf-overview.gif`](aqcf-overview.gif) is the animated README cover.
[`aqcf-overview.png`](aqcf-overview.png) is its static companion.

The animation redraws mechanisms from **Figures 1–2 and Algorithm 1 of the AQCF
paper**. Three full-width scenes follow the actual algebra and dependencies:
preserve the low bits, reuse an incoming carry in two representations, and merge
the required output coefficients.

## From the paper to the animation

| Scene | Paper source | What the animation shows | Implementation |
|---|---|---|---|
| **Retain the phase** | Figure 1(b), retained-phase interface | `A = 4ã + r` becomes `A′ = A − 4corr = 4a + r`. The same two low bits `r` survive the correction. Together with `B = 4b + t`, the protected interface reconstructs the shared integer `z ≡ b − a + ω(t,r) (mod 2³⁷)`. | [Retained phase](../../native/resnet50/engine/retained_resnet_phase.hpp), [quarter recovery](../../native/resnet50/core/quarter_recover.hpp) |
| **Join the carry** | Algorithm 1, “One-use joint carry in the prefix” | Fresh random-OT keys determine mask `α` **before** the preceding lookup produces `β = d ⊕ α`. The next lookup returns arithmetic shares `u_A + u_B ≡ d (mod 2ʷ)` of that incoming carry and advances the next Boolean carry. | [Joint carry bridge](../../native/resnet50/protocol/resnet_joint_carry_bridge.inc), [joint mask core](../../native/resnet50/protocol/joint_mask_core.hpp) |
| **Merge the traces** | Figure 2(a–c), output-directed linear evaluation | One input pack feeds two private short weight packs. Both branches apply `X⁻¹` before channel contraction. The resulting sixteen-coefficient view then exposes the cancellation: `V = A + U`, `T = A − U`, `U = XZ₁`, and `σ₁(X) = −X`. Thus `M = V + σ₁(T) = 2[a₀, u₁, a₂, u₃, …]`. Even coefficients come from `A`; odd coefficients come from `U`; the original trace scale of two is preserved. | [Output trace merge](../../native/resnet50/engine/output_trace_merge.hpp) |

## How to read it

- **Protected symbolic views.** Coefficient labels explain the algebra; they do
  not represent an intermediate plaintext opening. Each party retains only its
  own local state and shares.
- **Distinct corrections.** The quarter correction `ω(t,r)` in the first scene
  is not the floor carry `c₁₂`. Its reconstructed `z` is the shared integer
  before the later floor operation, not that operation’s quotient.
- **Dependencies, not message routes.** Arrows in the carry scene show preparation
  and use order. They do not send secret keys between parties. The arithmetic
  shares concern the incoming carry; the outgoing Boolean carry is a separate
  sharing.
- **A layer-specific cost reduction.** The `128 → 32` key-switch count belongs
  to the native ResNet-50 layer from `256 × 56 × 56` to `64 × 56 × 56`, with
  `C = 2`. It is an operation count, not a fourfold inference speedup. The
  layer’s `8,192` ciphertext–plaintext products remain unchanged.

Animation timing is chosen for readability and does not measure latency. The
paper’s numerical bounds and security assumptions still apply.

## Regenerate the cover

From the repository root:

```sh
python -m pip install Pillow
python tools/render_readme_banner.py
```

The renderer writes both images in this directory and uses system fonts. It
needs no dataset, trained model, network connection, or inference run.
