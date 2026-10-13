#!/usr/bin/env python3
"""Prepare the fixed ImageNet inputs without inference or model access.

Images are acquired separately under the provider's terms. Their bytes and the
resulting NCHW integer tensors are checked against the retained experiment index.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def preprocess_squeezenet(raw):
    import io
    import numpy as np
    from PIL import Image

    image = np.asarray(Image.open(io.BytesIO(raw)).convert("RGB").resize(
        (227, 227), Image.Resampling.BILINEAR)).astype(np.float64)
    mean = np.array([104.006, 116.669, 122.679], dtype=np.float32)
    return ((image[:, :, ::-1] - mean) * 4096).astype("<i8")[None]


def preprocess_resnet50(raw):
    os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
    os.environ.setdefault("TF_ENABLE_ONEDNN_OPTS", "0")
    import numpy as np
    import tensorflow as tf

    if not hasattr(preprocess_resnet50, "module"):
        path = ROOT / "tools/data/resnet_preprocessing.py"
        spec = importlib.util.spec_from_file_location("resnet_preprocessing", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        tf.config.threading.set_intra_op_parallelism_threads(2)
        tf.config.threading.set_inter_op_parallelism_threads(1)
        preprocess_resnet50.module = module
    image = preprocess_resnet50.module.preprocess_image(
        image_buffer=raw, bbox=None, output_height=224, output_width=224,
        num_channels=3, is_training=False).numpy()
    if image.shape != (224, 224, 3) or not np.isfinite(image).all():
        raise ValueError("Invalid ResNet-50 preprocessing result")
    return np.trunc(image.astype(np.float64) * 4096).astype("<i8")[None]


def image_from_directory(folder, row):
    # Official archive names and the historical mirror's WNID suffix are both
    # supported; acceptance depends on the image digest, never the filename.
    choices = [folder / row["filename"], folder / row["mirror_filename"],
               folder / f"sample_{row['index']:03d}" / "IMAGE.jpeg"]
    found = [path for path in choices if path.is_file()]
    if not found:
        raise FileNotFoundError(f"Missing image {row['filename']} in {folder}")
    return found[0].read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", choices=["squeezenet", "resnet50"], required=True)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--images", type=Path, help="Directory of licensed validation JPEGs")
    source.add_argument("--parquet", type=Path, help="Recorded mirror shard, verified by SHA256")
    parser.add_argument("--indices", default="0", help="Comma-separated fixed indices, or all")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "data/imagenet_input_index.json").read_text())
    selected = list(range(100)) if args.indices == "all" else [int(x) for x in args.indices.split(",")]
    if not selected or len(set(selected)) != len(selected) or not all(0 <= i < 100 for i in selected):
        parser.error("indices must be unique integers in [0,99]")
    if args.output.exists():
        parser.error("output must be a new directory; existing assets are never overwritten")
    parquet_rows = None
    if args.parquet:
        with args.parquet.open("rb") as stream:
            actual = hashlib.file_digest(stream, "sha256").hexdigest()
        if actual != manifest["historical_mirror"]["sha256"]:
            raise ValueError("Parquet shard does not match the recorded source")
        import pyarrow.parquet as pq
        parquet_rows = next(pq.ParquetFile(args.parquet).iter_batches(batch_size=100)).to_pylist()
    import numpy as np
    prepare = preprocess_squeezenet if args.model == "squeezenet" else preprocess_resnet50
    # Calculate and verify all selected inputs before creating an output tree.
    prepared = []
    for index in selected:
        row = manifest["images"][index]
        raw = parquet_rows[index]["image"]["bytes"] if parquet_rows else image_from_directory(args.images, row)
        if digest(raw) != row["image_sha256"]:
            raise ValueError(f"Image digest mismatch at fixed index {index}")
        tensor = prepare(raw)
        nchw = np.ascontiguousarray(tensor.transpose(0, 3, 1, 2), dtype="<i8")
        actual = digest(nchw.tobytes())
        expected = row[args.model]["input_nchw_sha256"]
        if actual != expected:
            raise ValueError(f"Preprocessing mismatch at index {index}: {actual} != {expected}; check dependency versions")
        prepared.append((row, tensor, nchw))
    args.output.mkdir(parents=True)
    report = []
    for row, tensor, nchw in prepared:
        out = args.output / f"sample_{row['index']:03d}"
        out.mkdir()
        np.save(out / "INPUT.npy", tensor, allow_pickle=False)
        nchw.tofile(out / "INPUT_NCHW.i64")
        # Native baseline readers accept whitespace-delimited NHWC integers.
        np.savetxt(out / "INPUT.inp", tensor.reshape(-1), fmt="%d")
        binding = {"index": row["index"], "model": args.model,
                   "image_sha256": row["image_sha256"], **row[args.model],
                   "shape_nchw": list(nchw.shape), "dtype": "little-endian int64"}
        (out / "binding.json").write_text(json.dumps(binding, indent=2) + "\n")
        report.append(binding)
    (args.output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Verified {len(report)} {args.model} inputs in {args.output}")


if __name__ == "__main__":
    main()
