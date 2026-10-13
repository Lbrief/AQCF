# Final native model reproduction

Use Linux x86-64 (Ubuntu/WSL2), four physical CPU cores, and AES-NI, AVX2, and FMA
support and Python 3.11 or 3.12. Ubuntu 24.04 supplies a suitable system Python.
Install the [build dependencies](../deps/README.md). The retained cloud
experiments used a 112 GiB memory limit; a lower memory requirement has not been
established by this packaging exercise. Full ResNet-50 execution can require
substantial memory. Compiler and input-preparation checks are separate from
encrypted inference.

## Acquire and compile model assets

Follow [data.md](data.md) to obtain the fixed checkpoints and prepare index 0.
From the repository root:

```sh
python tools/compile_models.py --model squeezenet \
  --tensors assets/models/squeezenet --output assets/compiled/squeezenet
python tools/compile_models.py --model resnet50 \
  --tensors assets/models/resnet50 --output assets/compiled/resnet50
```

Both commands verify every imported tensor and the complete final program/layout
digests. They preserve the original integer weights, biases, graph, and rounding.
The ResNet-50 compiler also checks all 802,816 symbolic rows of the original and
polyphase first affine, then preserves the remaining 71 affines byte for byte.

The output `PROGRAM.bin` contains the model and belongs to the server.
`CLIENT_LAYOUT.bin` contains public layout geometry. `compilation.json` binds
their hashes. Intermediate `base_program.bin` and `base_layout.bin` are retained
for diagnosis; they are not the final runtime assets.

## Build the role executables

```sh
python3 deps/fetch_dependencies.py --jobs 2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

The native executables are `build/native/aqcf_squeezenet` and
`build/native/aqcf_resnet50`. No old build directories, precompiled research
archives, cloud hostnames, or login credentials are needed. Build and dependency
sources are version-pinned; a different compiler can still produce a different
binary digest.

## Local preparation and isolation check

Use a fresh output directory on the Linux filesystem. Preparing a run does not
start inference:

```sh
python3 tools/run_native.py --model squeezenet \
  --binary build/native/aqcf_squeezenet \
  --compiled-model assets/compiled/squeezenet \
  --input assets/inputs/squeezenet/sample_000/INPUT_NCHW.i64 \
  --output /var/tmp/aqcf-squeezenet-prepared
```

The default CPU selection chooses four allowed CPUs from distinct physical
cores. Override it with `--cpus 0,2,4,6` when those IDs fit your machine. The
recorded cloud CPU IDs are not required. The tool verifies the model/input hashes,
extracts the allowed OpenMP callback addresses from the new binary, copies the
actual shared-library dependencies, and writes fresh role commands.

Add `--probe` and use `sudo` with a different fresh output directory to check the
two role jails and dynamic loader. The loader probe does not execute a model:

`sudo python3` must resolve to Python 3.11 or newer. On a system with an older
default interpreter, use the absolute path to your installed Python 3.11/3.12
instead. The launcher does not need packages from the data-preparation environment.

```sh
sudo python3 tools/run_native.py --model squeezenet \
  --binary build/native/aqcf_squeezenet \
  --compiled-model assets/compiled/squeezenet \
  --input assets/inputs/squeezenet/sample_000/INPUT_NCHW.i64 \
  --output /var/tmp/aqcf-squeezenet-probe --probe
```

The launcher uses Linux mount/PID namespaces, drops the two roles to separate
unprivileged UIDs, exposes only each role's own asset, enables `no_new_privs`, and
creates an empty per-session OT cache. Administrative privileges are required
for this namespace setup. Root is the trusted local controller, not a protocol
party. Never place a run directory inside `/usr`, which is read-only mounted in
both jails.

## Execute one local encrypted inference

When ready, replace `--probe` with `--execute` and choose another fresh output
directory. For ResNet-50, substitute `resnet50` in the model, binary, compiled
asset, and input paths. The launcher synchronizes `ASSETS_READY` and `READY`
before sending the connect and start signals; directly invoking the role binary
is insufficient.

Each execution generates fresh keys and protocol state. It retains role logs,
resource snapshots, each role's result, final client scores, and `result.json`.
Success requires exact agreement with all 1000 SqueezeNet scores or 1001
ResNet-50 scores for the fixed input, plus the final class index. The reference
scores are loaded only by the controller after the protocol; neither party
receives them as an input.

This entry point runs two isolated parties on one host and reports that scope.
It does not reproduce WAN latency. The preserved roles bind to loopback; the
original WAN campaign used a separate transport/deployment harness. This local
preparation does not yet provide a validated portable two-host WAN controller.
The retained relay and a transport-plan generator are provided in
[WAN deployment](wan.md). Do not report
a loopback result as WAN performance. The retained WAN observations and their
timing ratios remain available in [results](../results/README.md).

All-on final native profiles and earlier structured removal controls are distinct
experiments. Do not use the older switches to claim a final-version three-way
factorial ablation.
