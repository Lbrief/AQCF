# AQCF

[English](README.md) | [简体中文](README.zh-CN.md)

<p align="center">
  <a href="docs/media/aqcf-overview.gif"><img src="docs/media/aqcf-overview.gif?v=bodoni-matrix" alt="aqcf algorithm animation: matrix products, channel packing, trace merging, retained phases, and joint carry shares" width="100%"></a>
</p>

[Full-size image](docs/media/aqcf-overview.png) · [HD video](docs/media/aqcf-overview.mp4) · [Animation guide](docs/media/README.md)

**Exact integer inference for private client–cloud computation.**

AQCF lets a client obtain a neural-network prediction while keeping its image
private from the cloud server and the server's model weights private from the
client. It combines homomorphic encryption for affine layers with two-party
protocols for nonlinear operations. Under the paper's numerical bounds and
security assumptions, the protocol preserves the integer outputs of the
corresponding clear computation.

This repository provides the algorithm implementations, model compilers, data
tools, and retained experimental observations. **Start with the four-step
SqueezeNet example** to run both protocol parties locally and check every output.

[Quick check](#quick-check) · [Complete inference](#complete-inference) ·
[Data and weights](#data-and-weights) · [Results](#results) ·
[Validation status](#validation-status)

## Algorithm overview

AQCF addresses three sources of work in private inference: quotient recovery at
the affine/nonlinear boundary, carry conversion inside the nonlinear protocol,
and coefficient extraction from encrypted outputs.

1. **Retained-phase quotient interface.** Keep the ciphertext component `c0`
   as a local phase and transmit `c1`. Protected low-bit correction then
   recovers shares of the required signed quotient. This reduces the
   information transferred across the affine/nonlinear boundary while
   preserving the corrected integer under the stated bounds.
   [Retained component](native/squeezenet/engine/retained_squeezenet_component.hpp) ·
   [Quotient recovery](native/squeezenet/core/quarter_consumer_fusion.hpp).
2. **Mask-programmed joint carry.** Program a lookup's random output mask so
   the following prefix stage derives arithmetic shares of its incoming carry
   while advancing the Boolean carry to the next prefix position. Carry
   conversion and prefix progression share this computation.
   [Joint-carry implementation](native/squeezenet/protocol/squeezenet_joint_carry_bridge.inc).
3. **Output-directed trace merge.** Merge recursive coefficient traces before
   projecting to the required outputs, preserving coefficient indices and
   scale. Eligible output layouts then need fewer key-switch operations.
   [Trace merge](native/resnet50/engine/output_trace_merge.hpp) ·
   [Evaluator](native/resnet50/src/trace_evaluator.cpp).

The [source map](native/README.md) connects these algorithms to the complete
model implementations. The arithmetic checks verify bounded identities and
coefficient mappings; security also depends on the backend and composition
assumptions in the paper.

<a id="quick-check"></a>
## Quick check: no dataset or model download

Use Git and Python 3.11 or 3.12. In a terminal:

```sh
git clone https://github.com/Lbrief/AQCF.git
cd AQCF
python3 tests/check_protocol_identities.py
python3 tools/summarize_results.py --output validation/quick-check/tables
```

The first Python command checks the arithmetic identities and prints
`"status": "PASS"`. The second generates five CSV tables, including speedup
ratios, from the retained observations. These commands do not run encrypted
inference or measure latency.

<a id="complete-inference"></a>
## First complete run: SqueezeNet

Run the following commands from the cloned repository, in an **Ubuntu 24.04
Linux shell or Ubuntu 24.04 on WSL2**. The native code requires x86-64, AES-NI,
AVX2, and FMA support, with four available physical CPU cores. A GPU is not
required. The launcher chooses suitable CPU IDs automatically. Use a directory
on the Linux filesystem for run output.

### 1. Build the executable

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl python3 python3-venv \
  binutils util-linux libssl-dev libgmp-dev libboost-dev zlib1g-dev libzstd-dev

python3 deps/fetch_dependencies.py --jobs 2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target aqcf_squeezenet --parallel 2
```

Run the dependency fetcher and CMake as your ordinary user. The fetcher obtains
fixed versions of SEAL and Eigen and builds the required dependencies. Details
are in [the build guide](deps/README.md).

### 2. Fetch the exact weights and compile the model

```sh
python3 -m venv .venv-data
. .venv-data/bin/activate
python -m pip install -r tools/data/requirements-squeezenet.txt

python tools/fetch_models.py --model squeezenet --output assets/weights
python tools/prepare_models.py --model squeezenet \
  --weights assets/weights/squeezenet/weights.inp \
  --output assets/models/squeezenet
python tools/compile_models.py --model squeezenet \
  --tensors assets/models/squeezenet --output assets/compiled/squeezenet
```

This imports the published integer checkpoint and builds the server program and
public client layout. The tools check their hashes against the retained final
model. No training is needed.

### 3. Prepare the fixed ImageNet image

Obtain `ILSVRC2012_val_00000001.JPEG` from the ImageNet 2012 validation set under
the [dataset provider's terms](https://www.image-net.org/download.php). Create
the directory below and place the original JPEG there, keeping its filename:

```sh
mkdir -p datasets/imagenet/val
```

Once `datasets/imagenet/val/ILSVRC2012_val_00000001.JPEG` is present:

```sh
python tools/prepare_data.py --model squeezenet \
  --images datasets/imagenet/val --indices 0 --output assets/inputs/squeezenet
```

The tool verifies both the image bytes and the prepared integer tensor. This
first example requires only that fixed image. An arbitrary JPEG or re-encoded
copy will not pass the bound reference check. See [data preparation](docs/data.md)
for the original 100-image selection and alternative input sources.

### 4. Run both parties and check every output

```sh
sudo python3 --version
sudo python3 tools/run_native.py --model squeezenet \
  --binary build/native/aqcf_squeezenet \
  --compiled-model assets/compiled/squeezenet \
  --input assets/inputs/squeezenet/sample_000/INPUT_NCHW.i64 \
  --output /var/tmp/aqcf-squeezenet-first-run --execute
```

The system Python used by `sudo` must be 3.11 or 3.12; Ubuntu 24.04 provides
3.12. An activated virtual environment does not change `sudo python3`.
`sudo` is used to create the isolated role environments; both protocol parties
then run as separate unprivileged users. The launcher first checks the dynamic
loader, starts a fresh protocol session, and compares every final score with the
reference. A successful run prints a line beginning with:

```text
PASS: all 1000 integer scores match; class 65.
```

Inspect `/var/tmp/aqcf-squeezenet-first-run/result.json` for the result and timing
fields. Role logs and resource samples are retained beside it. Use a new output
directory for every run; the launcher refuses an existing one. Asset-preparation
commands also preserve existing outputs.

This is a **local two-party run**. The client's `service_monotonic_seconds`
measures the interval from the first encrypted input frame to receipt of the
final protected result; setup and local final reconstruction are separate.
The report also retains the raw-clock measurement. Keep the timing boundary and
deployment label when reporting results. For preparation-only commands, loader
probes, and ResNet-50 instructions, see [native model reproduction](docs/native_models.md).

<a id="data-and-weights"></a>
## Data and weights

| Models | Images | Exact weights |
|---|---|---|
| Native SqueezeNet and ResNet-50 | [ImageNet 2012 validation set](https://www.image-net.org/download.php) | Pinned [OpenCheetah integer exports](https://github.com/Alibaba-Gemini-Lab/OpenCheetah/tree/f71d4d49ffde81701fc3cc2d887fae62381fb472/pretrained), fetched by `tools/fetch_models.py` |
| Structured ResNet-18 / CIFAR-100 | [CIFAR-100](https://www.cs.toronto.edu/~kriz/cifar.html) | PrivCirNet author checkpoint; [source and hashes](data/structured_model_sources.json) |
| Structured ResNet-18 / Tiny ImageNet | [Tiny ImageNet](https://cs231n.stanford.edu/tiny-imagenet-200.zip) | PrivCirNet author checkpoint; [source and hashes](data/structured_model_sources.json) |
| Structured MobileNetV2 / CIFAR-100 | [CIFAR-100](https://www.cs.toronto.edu/~kriz/cifar.html) | AQCF's frozen derived checkpoint; separate release asset described below |

Download the derived MobileNetV2 checkpoint from the `models-v1` release:
[`cifar_mobilenetv2_state.pt`](https://github.com/Lbrief/AQCF/releases/download/models-v1/cifar_mobilenetv2_state.pt).
Its SHA-256 is
`0e889f03a6ee93017f4c4fa4cd1a3c5a3ea5b2a465cd9bde45afc2104ad39e8a`.
The original upstream MobileNet checkpoint is not interchangeable with this file.

[Data instructions](docs/data.md) give the preprocessing, fixed sample selection,
source revisions, and checksums. [Structured-model instructions](docs/structured_models.md)
cover checkpoint import, compilation, execution, and mechanism removals. Downloaded
datasets, weights, generated assets, and run outputs are excluded from Git.

<a id="results"></a>
## Results and ablations

The repository retains the paper's observations and regenerates readable tables.
Use these time ratios:

- **Framework speedup:** baseline time / AQCF time for the same architecture and comparison.
- **Mechanism removal:** time with the mechanism removed / time with all mechanisms enabled.
- **Incremental change:** control time / candidate time for the recorded pair.

A ratio above 1 favors AQCF, the all-on configuration, or the candidate,
respectively. The 33 removal settings belong to the earlier structured profiles;
they are not a factorial ablation of the final native implementation. Read
[results and comparison boundaries](results/README.md) before comparing rows.
The [WAN guide](docs/wan.md) documents the retained transport design and what is
still needed for a portable two-host run.

<a id="validation-status"></a>
## Validation status

There are two code families. `native/` contains the final SqueezeNet and
ResNet-50 implementations. `structured/` contains the three earlier model
profiles used for the 33 applicable mechanism-removal configurations. Their
switches and experimental scopes differ; follow the guide for your profile.

The following status refers to the reorganized code in this repository:

| Model and dataset | Code family | Built from source | Program, layout, and input hashes | Complete encrypted inference |
|---|---|---|---|---|
| SqueezeNet / ImageNet | Native | Passed | Matched | **Passed locally: all 1000 scores and the class index matched** |
| ResNet-50 / ImageNet | Native | Passed | Matched | Not yet run with this repository |
| ResNet-18 / CIFAR-100 | Structured | Passed | Matched | Not yet run with this repository |
| ResNet-18 / Tiny ImageNet | Structured | Passed | Matched | Not yet run with this repository |
| MobileNetV2 / CIFAR-100 | Structured | Passed | Matched | Not yet run with this repository |

The completed SqueezeNet run used two isolated parties on one local host.
Two-host WAN reproduction with this repository has not been validated. The
paper's retained WAN measurements are available separately in
[results](results/README.md). See [the validation record](docs/validation.md)
for the checks behind this table.

## Repository guide

| Path | Contents |
|---|---|
| [`native/`](native/README.md) | Final SqueezeNet and ResNet-50 protocol implementations |
| [`structured/`](docs/structured_models.md) | Earlier model profiles and mechanism-removal workflow |
| [`tools/aqcf_compiler/`](tools/aqcf_compiler/) | Coefficient compiler |
| [`data/`](docs/data.md) | Public graphs, reference bindings, and model-source records |
| [`tests/`](tests/) | Arithmetic identity and coefficient-mapping checks |
| [`results/`](results/README.md) | Retained observations and comparison boundaries |

## Authors and notices

Keliang Xing, Ru Xue, Hejuan Du, Guangxiu Pan, and Jianglu Wen.
Xizang Minzu University, Xianyang, Shaanxi 712082, China.

See [NOTICE.md](NOTICE.md) for the current licensing status and third-party notices.
