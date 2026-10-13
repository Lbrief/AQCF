#!/usr/bin/env python3
"""Import the pinned Q12 native checkpoints into named NumPy tensor assets.

This imports the complete original trained tensors. It does not replace their
quantization, graph, activation functions, or classifier with torchvision.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", choices=["squeezenet", "resnet50"], required=True)
    parser.add_argument("--weights", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    entry = json.loads((ROOT / "data/model_sources.json").read_text())["models"][args.model]
    expected = entry["files"][0]["sha256"]
    if sha(args.weights) != expected:
        raise ValueError("Weight digest differs from the published experiment checkpoint")
    if args.output.exists():
        parser.error("output must be a new directory")
    import numpy as np
    graph_file = ROOT / entry["graph"]
    graph = json.loads(graph_file.read_text())
    count = 52 if args.model == "squeezenet" else 251
    raw = np.fromfile(args.weights, dtype=np.int64, sep=" ")
    expected_values = sum(math.prod(graph["arrays"][f"tmp{i}"]) for i in range(1, count + 1))
    if raw.size != expected_values:
        raise ValueError(f"Checkpoint has {raw.size} values, expected {expected_values}")
    tensors, offset = {}, 0
    for i in range(1, count + 1):
        name = f"tmp{i}"
        shape = graph["arrays"][name]
        size = math.prod(shape)
        values = raw[offset:offset + size].reshape(shape)
        if args.model == "resnet50":
            values = (values + (1 << 36)) % (1 << 37) - (1 << 36)
        tensors[name] = values
        offset += size
    args.output.mkdir(parents=True)
    shutil.copyfile(graph_file, args.output / "GRAPH.json")
    np.savez_compressed(args.output / "PARAMETERS.npz", **tensors)
    tensor_manifest = {name: {"shape": list(value.shape),
                                "sha256_int64_le": hashlib.sha256(value.astype("<i8").tobytes()).hexdigest()}
                       for name, value in tensors.items()}
    binding = {"model": args.model, "checkpoint_sha256": expected,
               "fractional_bits": 12, "classes": entry["classes"],
               "graph_sha256": sha(graph_file), "values": int(raw.size),
               "tensor_count": count, "tensors": tensor_manifest}
    (args.output / "binding.json").write_text(json.dumps(binding, indent=2) + "\n")
    print(f"Imported all {count} {args.model} tensors ({raw.size} integers) into {args.output}")


if __name__ == "__main__":
    main()
