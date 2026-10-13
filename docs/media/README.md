# AQCF algorithm animation

[Animated cover](aqcf-overview.gif) · [Full-size image](aqcf-overview.png) ·
[HD video](aqcf-overview.mp4)

The cover presents three AQCF mechanisms in one academic diagram. Its heading
contains **aqcf** and the five authors' full names. The animation follows the
matrix calculation and then moves through the two-party interfaces.

## What the animation shows

| Region | Calculation | Implementation |
|---|---|---|
| **Encrypted linear evaluation** | A complete 4 × 4 matrix example is packed into two input groups. The first output group follows two shifted short-weight paths into a ten-coefficient trace merge. | [Output trace merge](../../native/resnet50/engine/output_trace_merge.hpp) |
| **Retained-phase interface** | The server keeps `R = −d₀` locally and sends `c₁`. Its affine correction changes the high part of its phase while preserving the low two bits. | [Retained phase](../../native/resnet50/engine/retained_resnet_phase.hpp), [quarter recovery](../../native/resnet50/core/quarter_recover.hpp) |
| **Joint carry** | A fresh bank programs `α` before the preceding lookup produces `β`. The next prefix step derives Boolean and arithmetic shares of the same incoming carry. | [Joint carry bridge](../../native/resnet50/protocol/resnet_joint_carry_bridge.inc), [joint mask core](../../native/resnet50/protocol/joint_mask_core.hpp) |

These regions follow Figures 1–2 and Algorithm 1 of the paper. The displayed
numbers are **independent illustrative examples**. They explain protected
values and algebra; they do not show intermediate plaintexts or keys being
opened by the protocol.

## Reading the coefficient merge

The input is packed in channel pairs. For the first output group, set
`A = Z₀`, `U = XZ₁`, and `σ(X) = −X`. Then
`M = A + U + σ(A − U)` retains even coefficients from `A` and odd coefficients
from `U`, with the original trace factor of two.

The ten displayed coefficients are
`[0, 8, 16, 18, 34, 6, 10, 4, 26, 0]`.
The eight interior values equal twice the first two output rows, interleaved by
column. Both boundary terms cancel algebraically. The Laurent notation `X⁻¹`
is an equivalent view of the negacyclic ring, not a negative array index.

The `128 → 32` key-switch count refers separately to the native ResNet-50 layer
with `256 → 64` channels, spatial size `56 × 56`, and `C = 2`. Its `8,192`
ciphertext–plaintext products remain unchanged. This is an operation count,
not a fourfold inference speedup or a cost measurement of the small example.

The lower panels distinguish the quarter correction from the floor carry.
The joint carry's `e` is the low bit of the existing prefix correction; the
next outgoing Boolean carry uses a separate sharing. The paper's numerical
bounds, backend assumptions, and security composition conditions still apply.

## View or regenerate

The GIF is a **30-second infinite loop**, 1440 × 1242, at 15 frames per second.
The video is 1600 × 1380 at 20 frames per second. Open the full-size image or
video to read the smaller annotations. Animation timing does not measure
protocol latency.

From the repository root:

```sh
python -m pip install Pillow imageio-ffmpeg
python tools/render_readme_banner.py
```

To generate only the still image and arithmetic check, or use another folder:

```sh
python tools/render_readme_banner.py --stills --output /tmp/aqcf-cover
```

The renderer uses an installed FFmpeg or the `imageio-ffmpeg` runtime. It uses
Windows system fonts when available and DejaVu fonts on Linux. Font differences
can change typography; set `AQCF_FONT_DIR` to choose a font directory. No
dataset, trained model, encryption run, or network access is needed after the
rendering dependencies are installed.

The source consists of [the renderer](../../tools/render_readme_banner.py),
[drawing primitives](../../tools/cover_drawing.py), and
[two-party diagrams](../../tools/cover_protocols.py).
