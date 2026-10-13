# AQCF cover animation

[`aqcf-overview.gif`](aqcf-overview.gif) is the animated README cover.
[`aqcf-overview.png`](aqcf-overview.png) is a still version for presentations or
readers who prefer a static image.

The three panels depict **mechanisms**, not a literal sequence of network
messages. Coefficient counts and animation timing are illustrative.

- **Retain the phase.** Keep `c0` as a cloud-local phase and send `c1`.
  Protected low-bit correction contributes to the exact shared signed quotient.
  The picture does not expose an intermediate plaintext or discard `c0`.
  See [component retention](../../native/squeezenet/engine/retained_squeezenet_component.hpp)
  and [quotient recovery](../../native/squeezenet/core/quarter_consumer_fusion.hpp).
- **Advance the carry.** A programmed lookup mask lets the joint prefix stage
  produce arithmetic shares of its input carry while advancing the next
  Boolean carry. These are distinct values, and both remain shared.
  See [joint carry](../../native/squeezenet/protocol/squeezenet_joint_carry_bridge.inc).
- **Merge the traces.** Recursive even/odd trace merging preserves coefficient
  indices and scale before projection into the required output groups.
  Colors represent retained indices, not plaintext values. The cartoon shows
  the merge structure; the implementation reduces key switching only for
  eligible output layouts.
  See [trace merging](../../native/resnet50/engine/output_trace_merge.hpp).

The exact-output claim uses the numerical bounds and security assumptions
stated in the paper. The animation adds no experimental or performance claims.

To regenerate both images from the repository root:

```sh
python -m pip install Pillow
python tools/render_readme_banner.py
```

The renderer uses system fonts (Segoe UI on Windows, DejaVu Sans or Liberation
Sans on Linux). It requires no dataset, trained model, network connection, or
inference run.
