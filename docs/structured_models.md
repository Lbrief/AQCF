# Structured-model removal study

This directory reproduces the three model profiles used in the mechanism-removal study. The Tiny ImageNet model is a conventional dense ResNet-18. The two CIFAR-100 profiles use the bound PrivCirNet-derived trained models. These are separate from the final native SqueezeNet and ResNet-50 implementation.

| Profile | Dataset and fixed input | Affine layers | Output classes | Settings |
|---|---|---:|---:|---:|
| `cifar_resnet18` | CIFAR-100 test index 7762 (zero-based) | 21 | 100 | 12 |
| `tiny_resnet18` | Tiny ImageNet `val_9197.JPEG` | 21 | 200 | 12 |
| `cifar_mobilenetv2` | CIFAR-100 test index 3901 (zero-based) | 53 | 100 | 9 |

The repository preserves all 33 applicable configurations. Removing a mechanism selects its same-function replacement; it does not remove a network layer. The retained integer output must remain identical in every configuration. A model's ordinary classification error is retained: the fixed CIFAR ResNet-18 example has label 28 and prediction 9.

## Source organization

Each profile has `inference_role.cpp`, `core/include/`, and `include/`; MobileNet also has its generated public graph. The three source variants are retained separately because their ring domains and nonlinear boundaries differ. No dependency on the original research directory is required.

| Module | Main files | Operation |
|---|---|---|
| Polynomial affine evaluation | `bfv_program.hpp`, `signed_weight.hpp`, `spectral_ingress.hpp` | Coefficient-packed HE affine layers and eligible short-weight paths |
| Shared inverse transform | `prescaled_original_zero.hpp` | Fuse the accumulator transform with fresh-zero construction |
| Linked carry and prefix | `linked_cofactor.hpp`, `late_carry.hpp`, `shared_prefix.hpp` | Protected carry/prefix composition and shared prefix work |
| Quotient extension | `quotient_extension.hpp` | Extend a bounded arithmetic value using the quotient path |
| Quarter representation | `quarter_protocol.hpp`, `quarter_recover.hpp` | Quarter normalization and protected recovery |
| Same-function controls | `aqcf_nonlinear_ablation.hpp`, `backbone_boundary.hpp` | Select the secure replacement for each removed mechanism |
| Public layout compiler | `compiler/spatial_planner.py`, `coefficient_program.py`, `program_writer.py` | Certified placement, polynomial coefficients, depthwise groups, and binary serialization |

Some files are specific to one profile. An unavailable mechanism is not represented as a removal experiment. In particular, shared-prefix, lift-specialization, and quarter-normalization removals do not apply to the MobileNet profile.

## Checkpoints and data

Use the source links and acquisition terms in [data.md](data.md). The exporters hash-check their inputs; an arbitrary ResNet or MobileNet checkpoint is not interchangeable with these profiles. Raw dataset images and model checkpoints are not embedded in the source tree.

### Python environments

The preparation checks used two existing Python 3.12 environments. These exact package versions were inspected locally:

| Preparation step | Python | PyTorch | torchvision | NumPy | Pillow |
|---|---|---|---|---|---|
| Tensor-only checkpoints, MobileNet endpoint, and all fixed-image preprocessing | 3.12.14, Windows | 2.3.0+cu121 | 0.18.0+cu121 | 1.26.4 | 12.3.0 |
| Original author ResNet checkpoint import with a restricted metadata allowlist | 3.12.3, Linux | 2.6.0+cpu | Not required | 1.26.4 | Not required |

All preparation operations ran on the CPU. The first environment happened to contain CUDA wheels; no CUDA computation or training was used. The original-checkpoint exports and tensor-only exports produced identical quantized parameter hashes. A different package combination must still pass the program, layout, and input hash checks before it is used.

Create the preparation environment with an installed Python 3.12 interpreter:

```sh
python3.12 -m venv .venv-structured
. .venv-structured/bin/activate
python -m pip install torch==2.3.0 torchvision==0.18.0 --index-url https://download.pytorch.org/whl/cu121
python -m pip install numpy==1.26.4 pillow==12.3.0
```

On Windows, use `py -3.12 -m venv .venv-structured` and activate `.venv-structured/Scripts/Activate.ps1`. The installation commands specify the inspected package versions; a fresh installation was not part of the packaging checks.

For an original author ResNet checkpoint, create a separate import environment. Use it for `quantize_resnet.py`, then return to the preparation environment for the following compilation and image-preparation commands:

```sh
python3.12 -m venv .venv-checkpoint
. .venv-checkpoint/bin/activate
python -m pip install torch==2.6.0 --index-url https://download.pytorch.org/whl/cpu
python -m pip install numpy==1.26.4
```

The tensor-only checkpoint path does not need this second environment. The public layout compiler requires NumPy; the C++ build and protocol executables do not require PyTorch or torchvision.

For CIFAR ResNet-18, use the PrivCirNet `Circulant/resnet_c100_b4_1.4` checkpoint. The exporter accepts its original checkpoint or the corresponding tensor-only state dictionary. Original author checkpoints require PyTorch 2.6 or newer for the restricted `argparse.Namespace` metadata allowlist; tensor-only states also work on PyTorch 2.3. The exporter materializes the published block-circulant projection before folding BN and applying nearest-even Q12/Q24 quantization.

```sh
python structured/compiler/quantize_resnet.py \
  --profile cifar_resnet18 --checkpoint /path/to/resnet_c100_checkpoint.pth.tar \
  --output assets/cifar_resnet18/parameters
python structured/compiler/export_program.py \
  --profile cifar_resnet18 --parameters assets/cifar_resnet18/parameters/parameters.npz \
  --output assets/cifar_resnet18/compiled
python structured/prepare_inputs.py --profile cifar_resnet18 --mode REAL \
  --cifar-test /path/to/cifar-100-python/test --output assets/cifar_resnet18/input
```

For Tiny ResNet-18, use the original `resnet_tiny.pth.tar` checkpoint or its exact tensor-only dense state identified in the data manifest. The graph is the author's 200-class network with a 3x3, stride-2 stem and no max pool. The preprocessing is bicubic resize to 71, center crop to 64, RGB normalization, and nearest-even Q12 quantization.

```sh
python structured/compiler/quantize_resnet.py \
  --profile tiny_resnet18 --checkpoint /path/to/tiny_resnet18_state.pt \
  --output assets/tiny_resnet18/parameters
python structured/compiler/export_program.py \
  --profile tiny_resnet18 --parameters assets/tiny_resnet18/parameters/parameters.npz \
  --output assets/tiny_resnet18/compiled
python structured/prepare_inputs.py --profile tiny_resnet18 --mode REAL \
  --tiny-image /path/to/tiny-imagenet-200/val/images/val_9197.JPEG \
  --output assets/tiny_resnet18/input
```

For MobileNetV2, the frozen derived endpoint preserves the BN/IR-compatible function used by the comparison. Its fixed graph contains 53 affine operations and 35 ReLU6 operations. This is not the original PrivCirNet author checkpoint. The `models-v1` asset preserves the retained endpoint bytes; preparing the release performed no training or fine-tuning. See [data.md](data.md) for provenance and the recorded license status.

Download the endpoint into the ignored asset directory and verify it before export:

```sh
mkdir -p assets/checkpoints
curl --fail --location \
  https://github.com/Lbrief/AQCF/releases/download/models-v1/cifar_mobilenetv2_state.pt \
  --output assets/checkpoints/cifar_mobilenetv2_state.pt
python -c "import hashlib; from pathlib import Path; p=Path('assets/checkpoints/cifar_mobilenetv2_state.pt'); assert p.stat().st_size == 9372732; assert hashlib.sha256(p.read_bytes()).hexdigest() == '0e889f03a6ee93017f4c4fa4cd1a3c5a3ea5b2a465cd9bde45afc2104ad39e8a'; print('Checkpoint verified')"
```

The exporter also checks this hash and loads only the tensor state with PyTorch's `weights_only=True` option.

```sh
python structured/compiler/quantize_mobilenet.py \
  --checkpoint assets/checkpoints/cifar_mobilenetv2_state.pt --output assets/cifar_mobilenetv2/parameters
python structured/compiler/export_program.py --profile cifar_mobilenetv2 \
  --parameters assets/cifar_mobilenetv2/parameters/MODEL.bin \
  --output assets/cifar_mobilenetv2/compiled
python structured/prepare_inputs.py --profile cifar_mobilenetv2 --mode REAL \
  --cifar-test /path/to/cifar-100-python/test --output assets/cifar_mobilenetv2/input
```

Every program compilation checks the complete output against the retained program and layout SHA-256 digests. Input preparation verifies the resulting tensor and writes the retained independent clear integer reference scores. These are exact-output checks, not retraining or additional measured inference results.

For a synthetic correctness input, replace the data-preparation command with `python structured/prepare_inputs.py --profile PROFILE --mode PUBLIC --output assets/PROFILE/public_input`. This uses the same trained coefficients and public geometry. It is a synthetic check, not a dataset accuracy result.

## Build and run

Install the shared dependencies using the root build instructions. Then build the full mechanism configuration:

```sh
cmake -S . -B build/all_on -DAQCF_BUILD_STRUCTURED=ON -DAQCF_ABLATION_SETTING=all_on
cmake --build build/all_on --target aqcf_cifar_resnet18 aqcf_tiny_resnet18 aqcf_cifar_mobilenetv2 -j2
```

The preserved timing profile uses one worker per party, with Linux CPU IDs 0 and 2 by default. On a host with different allowed CPU IDs, pass `--cpus SERVER_ID,CLIENT_ID`; the helper validates and records both IDs. This changes affinity only and does not make timings from different hardware directly comparable. The role executables bind to loopback. `run_local_pair.py` provides local process coordination and asset hashes; it does not provide OS file isolation. Run untrusted parties on separately isolated machines or containers when that isolation is required. The code remains a conditional research implementation under the security premises stated in the paper.

```sh
python structured/run_local_pair.py \
  --binary build/all_on/structured/aqcf_cifar_resnet18 \
  --layout assets/cifar_resnet18/compiled/layout.bin \
  --program assets/cifar_resnet18/compiled/program.bin \
  --input assets/cifar_resnet18/input/input.i64 \
  --reference assets/cifar_resnet18/input/reference.i64 \
  --mode REAL --classes 100 --output runs/cifar_resnet18/all_on
```

Substitute the profile name for the other models; use `--classes 200` for Tiny ImageNet. Each output directory must be new. Add `--check-only` to verify the supplied files without starting either role.

The helper retains both process logs, role results, and `comparison.json`. It compares every final integer score. The client's `service_raw_seconds` measures the online interval ending at final protected-score receipt, before local reconstruction and checking. Compilation, setup, and process time are separate. Local timing varies with hardware and load; these new observations do not replace the paper's retained measurements. The role communication counters are retained as recorded and must not be silently relabeled as independently measured network bytes.

## Build a removal configuration

Use a new build directory for each setting. For example:

```sh
cmake -S . -B build/no_linked_prefix -DAQCF_BUILD_STRUCTURED=ON \
  -DAQCF_ABLATION_SETTING=no_linked_prefix
cmake --build build/no_linked_prefix --target aqcf_cifar_resnet18 -j2
```

Run that binary with the same layout, program, input, and reference. The supported setting names are:

- `all_on`, `all_off`
- `no_short_weights`, `no_inverse_transform_fusion`
- `no_linked_prefix`, `no_shared_prefix`, `no_compact_emitter`, `no_quotient`
- `no_lift_specialization`, `no_quarter_normalization`
- `no_prefix_and_quotient`, `no_short_weights_and_fusion`

The exact profile-to-flag mapping is in [ablation_settings.json](../structured/ablation_settings.json). `0` enables a mechanism; `1` selects its removal control. CMake omits a model when a requested setting is inapplicable to that model. Report each matched timing ratio as `time_removed / time_all_on`. Values above one mean the full configuration is faster. Do not add the individual ratios or interpret them as the speedup over a different framework.

## Packaging checks performed

- All three reorganized role sources compiled against the retained dependency archives.
- CIFAR ResNet-18 and Tiny ResNet-18 were re-quantized from their retained checkpoints without training.
- MobileNetV2's integer packet was regenerated from its frozen endpoint.
- All three complete program files and all three layout files exactly matched the historical SHA-256 digests.
- All three fixed real inputs were regenerated from retained raw pixels, and their tensor hashes matched the recorded study inputs.
- No encrypted inference, cloud run, or training was executed during these checks.

The root verification record describes the separate clean build of the shared dependency closure.
