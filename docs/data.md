# Data, checkpoints, and exact input preparation

The code repository contains model geometry, data indices, source locations, and
checksums. The preparation tools require Python 3.11 or 3.12. Obtain image datasets and third-party checkpoints from their providers
under the applicable terms. Keep downloaded images, checkpoints, generated
programs, and experiment output outside the tracked source tree.

## Native SqueezeNet and ResNet-50

The final native-model experiments use the trained Q12 integer checkpoints
published by [OpenCheetah](https://github.com/Alibaba-Gemini-Lab/OpenCheetah),
revision `f71d4d49ffde81701fc3cc2d887fae62381fb472`. Its pretrained-asset README
attributes the exports to EzPC Athos. Use these exact files: substituting a
torchvision checkpoint changes the function being evaluated.

| Profile | Upstream checkpoint | Logical graph | Class outputs |
|---|---|---:|---:|
| `squeezenet` | `pretrained/sqnet_model_scale12.inp` | 117 operations, 26 convolutions | 1000 |
| `resnet50` | `pretrained/resnet50_model_scale12.inp` | 180 operations, 72 affine stages | 1001, including background |

Fetch and import the weights from the repository root:

```sh
python3 -m venv .venv-data
. .venv-data/bin/activate
python -m pip install -r tools/data/requirements-squeezenet.txt
python tools/fetch_models.py --model all --output assets/weights
python tools/prepare_models.py --model squeezenet --weights assets/weights/squeezenet/weights.inp --output assets/models/squeezenet
python tools/prepare_models.py --model resnet50 --weights assets/weights/resnet50/weights.inp --output assets/models/resnet50
```

`fetch_models.py --upstream /path/to/OpenCheetah` also works without a network
download. The fetcher checks the pinned upstream file hash before converting its
LF line endings to the CRLF convention of the retained experiment checkpoint.
It then checks the experiment-file hash. Interrupted HTTP bodies resume only
when the server confirms the requested byte range. This conversion changes no integer
tokens. Both hashes are recorded in `data/model_sources.json`.

`prepare_models.py` imports every trained tensor and writes `GRAPH.json`,
`PARAMETERS.npz`, and a per-tensor checksum binding. SqueezeNet has 52 parameter
tensors; ResNet-50 has 251. Model import performs no training or inference.
The separate coefficient compiler transforms these assets into the server's
`PROGRAM.bin` and the public `CLIENT_LAYOUT.bin`.

### Image source and fixed selection

Obtain the ILSVRC 2012 validation data through the
[ImageNet download page](https://www.image-net.org/download.php). Accept the
provider's access terms before downloading or sharing data. This repository does
not redistribute ImageNet images.

The retained experiments used the first 100 validation records from the
`benjamin-paine/imagenet-1k` Hugging Face mirror at revision
`ac6a45918fb830d6d1555b01090d7b991e8f8a4a`, shard
`data/validation-00000-of-00014.parquet`. That was a third-party mirror, not an
official ImageNet download. Its exact provenance and checksum are retained in
`data/imagenet_input_index.json`. The same manifest provides official filenames,
image-byte hashes, class labels, and the prepared integer-input hashes for all
100 images. Official archive files can therefore be used directly if their bytes
match; the mirror is not required by the preparation tool.

The final native cohost/WAN observations use fixed index `0`:
`ILSVRC2012_val_00000001.JPEG`, sea snake, label 65 in the 1000-class convention.
The ResNet-50 graph adds the background offset and uses label 66. The input is
different from the demonstration dog image shipped in OpenCheetah's pretrained
directory. The older 100-image evaluations and the final native observations are
different experiment versions; the manifest does not turn the latter into a new
100-image benchmark.

### Prepare exact integer inputs

Place the validation JPEGs in a directory. Preserve their official filenames.

```sh
python tools/prepare_data.py --model squeezenet --images /data/imagenet/val --indices 0 --output assets/inputs/squeezenet
python -m pip install -r tools/data/requirements-resnet50.txt
python tools/prepare_data.py --model resnet50 --images /data/imagenet/val --indices 0 --output assets/inputs/resnet50
```

Replace `--indices 0` with `--indices all` to prepare the original fixed 100-image
set. A comma-separated subset such as `--indices 0,1,2` is also supported.
For the historical mirror shard, install `tools/data/requirements-parquet.txt`
and replace `--images ...` with `--parquet /path/to/validation-shard.parquet`.
The entire shard hash is checked before any records are used.

SqueezeNet converts the JPEG to RGB, resizes to 227 by 227 with Pillow bilinear
interpolation, changes RGB to BGR, subtracts the author's float32 channel means
`[104.006, 116.669, 122.679]`, multiplies by 4096, and truncates toward zero.
ResNet-50 uses the retained Athos/TensorFlow evaluation pipeline: aspect-preserving
resize with minimum side 256, central 224 by 224 crop, RGB mean subtraction, and
Q12 truncation. It uses TensorFlow 2.17.1 with v1-compatible image operators;
replacing those operators with modern resize defaults can change integer inputs.
The TensorFlow source and Apache-2.0 notice are in `tools/data/`.

Each selected input must match its recorded SHA256 before the output directory
is created. The outputs are `INPUT.npy` in NHWC order, whitespace-separated
`INPUT.inp` in NHWC order for native baseline readers, and `INPUT_NCHW.i64` as
little-endian int64 for AQCF. The text file's whitespace layout is immaterial;
the integer tensor hash is the comparison boundary. The tools reject existing
output directories rather than silently replacing an earlier preparation.

## Structured-model experiments and mechanism removals

These are the earlier model profiles used by the structured runtime and the
33-configuration mechanism-removal study. They do not share the final native
SqueezeNet/ResNet-50 binaries or all of their optimization switches.

| Profile | Dataset and fixed ablation input | Checkpoint provenance |
|---|---|---|
| `cifar_resnet18` | CIFAR-100 test index 7762, label 28 | PrivCirNet author block-circulant ResNet-18 checkpoint |
| `tiny_resnet18` | Tiny ImageNet `val_9197.JPEG`, label 90 | PrivCirNet author ordinary dense ResNet-18 checkpoint |
| `cifar_mobilenetv2` | CIFAR-100 test index 3901, label 86 | Frozen BN/IR-compatible derived MobileNetV2 endpoint used in the AQCF comparison |

Get CIFAR-100 from the [dataset authors' page](https://www.cs.toronto.edu/~kriz/cifar.html).
The Python archive's extracted `cifar-100-python/test` file has the experiment
SHA256 `4b67687d9933c4db8f0831104447f15b93774f4f464bd0516f0f0f2ac83b7864`.
Get Tiny ImageNet from the [Stanford archive](https://cs231n.stanford.edu/tiny-imagenet-200.zip);
the recorded archive SHA256 is
`6198c8ae015e2b3e007c7841da39ec069199b9aa3bfa943a462022fe5e43c821`.
Tiny ImageNet labels use lexically sorted WNIDs. The earlier fixed 100-image
selection ranked validation filenames by SHA256 of
`AQCF-TinyImageNet-external100-20260915-v1`, a newline, and the filename. Index 0
of that fixed ranking is `val_9197.JPEG`.

PrivCirNet's model zoo provides the original ResNet checkpoints. The precise
download locations and hashes are in `data/structured_model_sources.json`.
`structured/compiler/quantize_resnet.py` imports the pinned ResNet checkpoints;
`structured/compiler/quantize_mobilenet.py` converts the frozen derived MobileNet
endpoint to its exact 53-block integer packet. The MobileNet endpoint is not
identical to the original upstream checkpoint. Obtain the frozen endpoint from
the [AQCF models-v1 release asset](https://github.com/Lbrief/AQCF/releases/download/models-v1/cifar_mobilenetv2_state.pt),
then follow the MobileNet download and verification commands in
[the structured-model guide](structured_models.md). Its filename is
`cifar_mobilenetv2_state.pt`, its size is 9,372,732 bytes, and its SHA256 is
`0e889f03a6ee93017f4c4fa4cd1a3c5a3ea5b2a465cd9bde45afc2104ad39e8a`.
The release preserves the retained endpoint bytes. No training or fine-tuning
was performed to prepare it. Downloading the original upstream MobileNet
checkpoint does not reproduce this endpoint.

The release contains only a `state_dict` wrapper with 106 named tensors.
It excludes optimizer state, training metadata, and dataset images. The retained
preparation materials do not record a separate license or SPDX identifier for
this derived checkpoint; see [NOTICE.md](../NOTICE.md) and applicable upstream
terms. A dependency's code license does not automatically apply to model weights.

Use the structured input and coefficient export instructions in
[`structured/README.md`](../structured/README.md). Input preparation preserves
each profile's original normalization, rounding, class ordering, and fixed
sample. The hash checks intentionally reject a visually equivalent re-encoded
JPEG or a different preprocessing implementation.

## Preparation checks

The portable native tools were checked locally by rebuilding all 100 SqueezeNet
inputs and all 100 ResNet-50 inputs from retained original JPEGs, and by
importing all 52/251 checkpoint tensors. The input hashes matched the retained
experiment bindings. This verifies the preparation path; it is not a new WAN
measurement, a retraining result, or a reproduction of the reported latency.
Model and dataset downloads remain subject to upstream availability and terms.
