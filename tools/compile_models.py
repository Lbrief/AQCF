#!/usr/bin/env python3
"""Compile pinned native model tensors to AQCF owner program and client layout.

The emitted program contains model weights and stays with the server. The layout
contains public geometry only. This command does not perform encrypted inference.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import time

import numpy as np

from aqcf_compiler import coefficient_program as compiler
from aqcf_compiler import spatial_planner as planner
from aqcf_compiler import layout_cost
from aqcf_compiler import paired_layout
from aqcf_compiler import pointwise_packing as packing
from aqcf_compiler import polynomial_io
from aqcf_compiler import program_writer as writer
from aqcf_compiler import resnet_stem

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "squeezenet": {
        "PROGRAM.bin": "dda98aca0511be0c3370f910873b2660c8a2b11f02bfccba26bb1eb14cad4290",
        "CLIENT_LAYOUT.bin": "88ceeb9c5e32b1eee4b04042b712ba50c31b7b50e59c47a29084f4091ffa9fd5",
    },
    "resnet50": {
        "PROGRAM.bin": "927d6cea0f8bbcab147d8e8482a7d07e97da8749a4f0fb8adf1ed3a8bf055a21",
        "CLIENT_LAYOUT.bin": "737670bca1d5770814aaab1feb2d8d20d108011a9774afd496cada86954c6c6d",
    },
}


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def header(pf, lf, count):
    pf.write(b"AQCFGPS1")
    lf.write(b"AQCFCL01")
    for stream in (pf, lf):
        writer.words(stream, 1, 8192, 1152921504606830593, count)


def load_tensors(assets, model):
    binding = read(assets / "binding.json")
    assert binding["model"] == model
    assert sha(assets / "GRAPH.json") == binding["graph_sha256"]
    with np.load(assets / "PARAMETERS.npz", allow_pickle=False) as archive:
        tensors = {name: archive[name].copy() for name in archive.files}
    assert set(tensors) == set(binding["tensors"])
    for name, value in tensors.items():
        entry = binding["tensors"][name]
        assert list(value.shape) == entry["shape"]
        assert hashlib.sha256(value.astype("<i8").tobytes()).hexdigest() == entry["sha256_int64_le"]
    return read(assets / "GRAPH.json"), tensors


def compile_squeezenet(assets, output):
    graph, params = load_tensors(assets, "squeezenet")
    assert len(params) == 52 and len(graph["nodes"]) == 117
    aid, cache, by_affine = 0, {}, {}
    base_program = output / "base_program.bin"
    base_layout = output / "base_layout.bin"
    with base_program.open("xb") as pf, base_layout.open("xb") as lf:
        header(pf, lf, 26)
        for index, node in enumerate(graph["nodes"]):
            if node["kind"] != "Conv2DWrapper":
                continue
            args = node["args"]
            n, h, w, ci, kh, kw, co, pt, pb, pl, pr, sh, sw = args[:13]
            assert n == 1 and kh == kw and pt == pb == pl == pr and sh == sw
            scale, add = graph["nodes"][index + 1:index + 3]
            assert scale["kind"] == "ScaleUp1" and scale["args"][2] == 12
            assert add["kind"] == "MatAddBroadCast4"
            assert add["args"][4:6] == [node["output"], scale["args"][1]]
            wk, bk = args[14], scale["args"][1]
            weights = params[wk].transpose(3, 2, 0, 1).copy()
            stub = dict(shape=[node["shape"][1], node["shape"][2], co], k=kh, stride=sh, pad=pt)
            program, _ = compiler.dense_program(np, planner, aid, stub, [1, ci, h, w], weights, params[bk], cache)
            writer.validate_write(program, pf, lf)
            by_affine[aid] = dict(wk=wk, bk=bk, h=h, w=w, ci=ci, co=co, input_tensor=args[13])
            aid += 1
            print(f"SqueezeNet affine {aid}/26", flush=True)
    assert aid == 26
    head, layers = packing.read_program(base_program)
    geometry = read(ROOT / "data/plans/squeezenet_pointwise.json")
    for row in geometry["layers"]:
        if row["proposed_public_geometry"]:
            i = row["affine"]
            layers[i] = packing.repack(layers[i], row["proposed_public_geometry"], row["input_channels"], row["output_channels"], row["output_spatial"])
    for pair in read(ROOT / "data/plans/squeezenet_paired_branches.json")["pairs"]:
        first, second = pair["pair"]
        assert by_affine[first]["input_tensor"] == by_affine[second]["input_tensor"] == pair["input_tensor"]
        for i, spatial in ((first, False), (second, True)):
            item = by_affine[i]
            weights = params[item["wk"]].transpose(3, 2, 0, 1).tolist()
            bias = [int(x) * 4096 for x in params[item["bk"]]]
            layers[i] = paired_layout.build(i, item["h"], item["w"], item["ci"], item["co"], pair["chosen"], weights, bias, spatial)
        assert layers[first]["maps"] == layers[second]["maps"]
    for row in read(ROOT / "data/plans/squeezenet_final_pointwise.json")["rows"]:
        i = row["affine"]
        layers[i] = packing.repack(layers[i], row["proposed"], row["ci"], row["co"], row["spatial"])
    report = packing.write_program(head, layers, output / "PROGRAM.bin", output / "CLIENT_LAYOUT.bin")
    assert report["totals"] == dict(input_CT=630, output_CT=1277, PM=30480)
    return report


def compile_resnet50(assets, output):
    graph, params = load_tensors(assets, "resnet50")
    assert len(graph["nodes"]) == 180 and len(params) == 251
    layout_cost.install_cost(planner)
    compiler.select_plan = layout_cost.choose
    cache, aid = {}, 0
    up = next(n for n in graph["nodes"] if n["kind"] == "ScaleUp1")
    bias = params[up["args"][1]] * (1 << up["args"][2])
    assert bias.shape == (1001,)
    base_program, base_layout = output / "base_program.bin", output / "base_layout.bin"
    totals = {k: 0 for k in ("input_CT", "output_CT", "PM", "CT_Add", "Rot")}
    with base_program.open("xb") as pf, base_layout.open("xb") as lf:
        header(pf, lf, 72)
        for node in graph["nodes"]:
            kind, args = node["kind"], node["args"]
            if kind not in ("Conv2DWrapper", "FusedBN", "FusedBatchNorm4411", "MatMul2D"):
                continue
            if kind == "FusedBatchNorm4411":
                n, h, w, c = args[:4]
                assert n == 1 and args[7:9] == [0, 12]
                program, _ = compiler.bn_program(np, planner, aid, dict(shape=[h, w, c]), params[args[5]], params[args[6]])
            else:
                if kind == "MatMul2D":
                    assert args[:3] == [1, 2048, 1001] and args[6] == 0
                    h = w = kh = sh = ho = wo = 1
                    ci, co, pt, pb, pl, pr = 2048, 1001, 0, 0, 0, 0
                    weight, b = params[args[4]].reshape(1, 1, ci, co), bias
                else:
                    n, h, w, ci, kh, kw, co, pt, pb, pl, pr, sh, sw = args[:13]
                    assert n == 1 and kh == kw and sh == sw and pt == pl and pb >= pt and pr >= pl
                    weight, b = params[args[14]].copy(), np.zeros(co, dtype=np.int64)
                    ho, wo = node["shape"][1:3]
                    if kind == "FusedBN":
                        v = weight * params[args[15]].reshape(1, 1, 1, co)
                        weight = np.sign(v) * ((np.abs(v) + 2048) // 4096)
                        b = params[args[16]] * 4096
                eh, ew = h + pb - pt, w + pr - pl
                program, _ = compiler.dense_program(np, planner, aid, dict(shape=[ho, wo, co], k=kh, stride=sh, pad=pt), [1, ci, eh, ew], weight.transpose(3, 2, 0, 1).copy(), np.zeros(co, dtype=np.int64), cache)
                if (eh, ew) != (h, w):
                    for j, mapping in enumerate(program["inputs"]):
                        channel, spatial = mapping[:, 0] // (eh * ew), mapping[:, 0] % (eh * ew)
                        yy, xx = spatial // ew, spatial % ew
                        keep = (yy < h) & (xx < w)
                        mapping = mapping[keep].copy()
                        mapping[:, 0] = channel[keep] * h * w + yy[keep] * w + xx[keep]
                        assert mapping.size
                        program["inputs"][j] = mapping
                    program["input_integer_count"] = ci * h * w
                for _, query in program["outputs"]:
                    query[:, 2] = b[query[:, 0] // (ho * wo)]
            counts = writer.validate_write(program, pf, lf)
            for key in totals:
                totals[key] += counts[key]
            aid += 1
            print(f"ResNet-50 affine {aid}/72", flush=True)
    assert aid == 72
    assert sha(base_program) == "861f21642ccea4d09a44bda9d407e2141b33524b58e6136bd65c06b332f9de6e"
    assert sha(base_layout) == "258cebc6b5bd2a411a70736e9cab477c599f5740bac0ad23bb817774cbcc843a"
    head, old, program_at = resnet_stem.read_first(base_program, True)
    layout_head, old_layout, layout_at = resnet_stem.read_first(base_layout, False)
    assert head == layout_head and old["maps"] == old_layout["maps"]
    weights, biases, old_certificate = resnet_stem.certify(old)
    new = resnet_stem.make_first(weights, biases)
    _, _, new_certificate = resnet_stem.certify(new, weights, biases)
    for src, dest, offset, model in ((base_program, output / "PROGRAM.bin", program_at, True), (base_layout, output / "CLIENT_LAYOUT.bin", layout_at, False)):
        with dest.open("xb") as stream, src.open("rb") as original:
            polynomial_io.write_first(stream, head, new, model)
            new_offset = stream.tell()
            original.seek(offset)
            shutil.copyfileobj(original, stream, 1 << 20)
        assert polynomial_io.tail_sha(src, offset) == polynomial_io.tail_sha(dest, new_offset)
    return dict(base_totals=totals, original_first_layer=old_certificate, final_first_layer=new_certificate, unchanged_affines_1_through_71=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", choices=list(EXPECTED), required=True)
    parser.add_argument("--tensors", type=Path, required=True, help="output of prepare_models.py")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output must be a new directory; retained files are never overwritten")
    args.output.mkdir(parents=True)
    started = time.monotonic()
    report = (compile_squeezenet if args.model == "squeezenet" else compile_resnet50)(args.tensors, args.output)
    hashes = {name: sha(args.output / name) for name in EXPECTED[args.model]}
    assert hashes == EXPECTED[args.model], "Compiled assets differ from the measured final model"
    report.update(model=args.model, status="EXACT_FINAL_PROGRAM_AND_LAYOUT", hashes=hashes,
                  seconds=time.monotonic() - started, encrypted_inference_runs=0)
    dump(args.output / "compilation.json", report)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
