# Final full-model protocol sources

Two independent targets retain the source versions used for the final
SqueezeNet and ResNet-50 observations:

| Target | Model source | Measured source lineage |
| --- | --- | --- |
| `aqcf_squeezenet` | `squeezenet/src/inference.cpp` | SqueezeNet private427 / WAN428 |
| `aqcf_resnet50` | `resnet50/src/inference.cpp` | ResNet-50 private516 / WAN521 |

The directory and file names describe their function. Historical namespace
names and feature switches inside the implementations remain unchanged so
that renaming does not silently change protocol selection. The public
provenance manifest records original and published content hashes.

The portable launcher supplies `AQCF_CPU_LIST` with four increasing CPU IDs.
It replaces the original machine-specific CPU numbering. Worker placement,
lane lookup and placement checks use this same list; the native validation
still requires four distinct physical cores. This scheduling adaptation is
recorded separately from the preserved cryptographic implementation.

Each model is organized into `engine` (graph execution and affine protocol),
`core` (integer-ring primitives and recovery), `protocol` (native correlation
and quotient interface), `parallel` (worker control), `he_backend` (SEAL
operation adapters), `vendor` (bound upstream headers), and `src` (compiled
entry points). Only headers actually used by the retained translation units
are included.

The three principal algorithm paths are:

1. **Retained-phase quotient interface.** Retained component and phase
   headers move affine corrections into local phases before quotient
   recovery. Quarter recovery and signed winding preserve the corrected
   affine integer under the stated input and decoding bounds.
2. **Mask-programmed joint carry.** Joint-carry bridge and prefix headers
   program a preceding lookup's output mask. The following lookup retains
   the Boolean carry share and derives the arithmetic share while advancing
   the prefix. Native OT state is freshly instantiated for a request.
3. **Output-directed trace merge.** ResNet-50's `output_trace_merge.hpp`
   and `trace_evaluator.cpp` contract recursive coefficient traces before
   output projection where the output layout permits fewer packed groups.
   `polyphase_convolution.hpp` and `exact_fma_kernel.cpp` implement the
   accompanying exact index and bounded-accumulation paths.

`build_recipe.json` records the compiler switches for each translation unit.
The public CMake build reconstructs SCI and AQCF libraries from source and
links SEAL through the standard installed package. Historical cloud workers,
account information, server addresses, private keys, caches and executable
binaries are deliberately absent.
