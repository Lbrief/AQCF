#!/usr/bin/env python3
"""Recreate the trained BN/IR-compatible MobileNet integer model packet.

The checkpoint is the frozen AQCF experiment endpoint derived from PrivCirNet,
not the unmodified upstream author checkpoint. No training or inference runs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

CHECKPOINT_SHA256 = "0e889f03a6ee93017f4c4fa4cd1a3c5a3ea5b2a465cd9bde45afc2104ad39e8a"
MODEL_SHA256 = "93f954f0bba8559380a8c8159c4a8b5f7211164015fabe2624fb735169067032"
AFFINE = {"CONV", "CIRCONV", "DEPTHWISE", "FC"}


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sha(args.checkpoint) != CHECKPOINT_SHA256:
        raise ValueError("Checkpoint is not the frozen experiment endpoint")
    if args.output.exists():
        parser.error("output must be a new directory")
    import torch
    torch.set_num_threads(2)
    torch.set_num_interop_threads(1)
    payload = torch.load(args.checkpoint, map_location="cpu", weights_only=True)
    state = payload["state_dict"]
    graph_path = Path(__file__).parent / "graphs/mobilenetv2_folded.json"
    graph = json.loads(graph_path.read_text())
    parameters, used = {}, set()
    for node in graph["nodes"]:
        if node["op"] not in AFFINE:
            continue
        name = node["name"]
        weight_key, bias_key = name + ".weight", name + ".bias"
        weight, bias = state[weight_key].detach().cpu(), state[bias_key].detach().cpu()
        used.update((weight_key, bias_key))
        if list(weight.shape) != node["weight_shape"] or bias.numel() != weight.shape[0]:
            raise ValueError(f"Invalid tensor geometry: {name}")
        if not torch.isfinite(weight).all() or not torch.isfinite(bias).all():
            raise ValueError(f"Non-finite checkpoint tensor: {name}")
        wi = torch.round(weight.double() * 4096).to(torch.int64)
        bi = torch.round(bias.double() * 4096).to(torch.int64)
        if node["op"] == "CIRCONV":
            block = int(node["block"])
            co, ci, kh, kw = wi.shape
            blocks = wi.reshape(co // block, block, ci // block, block, kh, kw)
            for i in range(block):
                if not torch.equal(blocks[:, i], torch.roll(blocks[:, 0], shifts=i, dims=2)):
                    raise ValueError(f"Circulant structure changed: {name}")
            if block > 1 and not torch.equal(bi.reshape(-1, block), bi.reshape(-1, block)[:, :1].expand(-1, block)):
                raise ValueError(f"Group bias changed: {name}")
        parameters[node["id"]] = (wi, bi)
    if len(parameters) != 53 or set(state) != used:
        raise ValueError("Incomplete checkpoint/graph binding")
    args.output.mkdir(parents=True)
    target = args.output / "MODEL.bin"
    with target.open("xb") as out:
        out.write(b"PCNMBV21" + struct.pack("<Q", len(parameters)))
        for ident, (weight, bias) in parameters.items():
            out.write(struct.pack("<QQQ", ident, weight.numel(), bias.numel()))
            out.write(weight.numpy().astype("<i8").tobytes())
            out.write(bias.numpy().astype("<i8").tobytes())
    actual = sha(target)
    if actual != MODEL_SHA256:
        raise ValueError(f"Rebuilt model packet differs: {actual}; retained for inspection")
    (args.output / "binding.json").write_text(json.dumps({
        "checkpoint_sha256": CHECKPOINT_SHA256, "model_sha256": actual,
        "graph_sha256": sha(graph_path), "affine_tensors": 53,
        "pytorch": torch.__version__, "quantization": "nearest-even Q12",
        "checkpoint_scope": "frozen derived BN/IR-compatible endpoint",
    }, indent=2) + "\n")
    print(f"Verified all 53 parameter blocks: {target} ({actual})")


if __name__ == "__main__":
    main()
