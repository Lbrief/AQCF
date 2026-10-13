#!/usr/bin/env python3
"""Prepare or run a fresh, isolated two-party native AQCF inference on Linux.

Default: prepare files and commands only. --probe loads each binary in its role
jail without inference. --execute performs one complete local two-party run.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import subprocess
import sys

from runtime import pair_harness

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def cpu_selection(text):
    allowed = os.sched_getaffinity(0)
    def core(cpu):
        root = Path(f"/sys/devices/system/cpu/cpu{cpu}/topology")
        return tuple((root / field).read_text().strip() for field in ("physical_package_id", "core_id"))
    if text:
        result = list(map(int, text.split(",")))
    else:
        result, seen = [], set()
        for cpu in sorted(allowed):
            identity = core(cpu)
            if identity not in seen:
                result.append(cpu)
                seen.add(identity)
            if len(result) == 4:
                break
    if len(result) != 4 or result != sorted(set(result)) or not set(result) <= allowed or len({core(c) for c in result}) != 4:
        raise ValueError("Choose four ascending, allowed CPU IDs on four distinct physical cores")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", choices=["squeezenet", "resnet50"], required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--compiled-model", type=Path, required=True)
    parser.add_argument("--input", type=Path, required=True, help="preprocessed INPUT_NCHW.i64")
    parser.add_argument("--output", type=Path, required=True, help="fresh local Linux directory")
    parser.add_argument("--cpus", help="four physical-core CPU IDs; default chooses from allowed affinity")
    parser.add_argument("--port", type=int, default=47000)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--probe", action="store_true", help="check owner isolation and dynamic loading only")
    group.add_argument("--execute", action="store_true", help="run one local encrypted graph after loader probes")
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("native inference requires Linux; use Ubuntu/WSL2")
    if not 45000 <= args.port < 49000:
        parser.error("base port must be in [45000, 49000)")
    if (args.probe or args.execute) and os.geteuid() != 0:
        parser.error("role mount/PID namespaces require sudo for --probe or --execute")
    output = args.output.resolve()
    if output.exists():
        parser.error("output must be fresh; old runs are retained")
    binary, compiled, input_file = args.binary.resolve(strict=True), args.compiled_model.resolve(strict=True), args.input.resolve(strict=True)
    reference = json.loads((ROOT / "data/reference_scores.json").read_text())[args.model]
    binding = json.loads((compiled / "compilation.json").read_text())
    assert binding["model"] == args.model and binding["status"] == "EXACT_FINAL_PROGRAM_AND_LAYOUT"
    for name, digest in binding["hashes"].items():
        assert sha(compiled / name) == digest, name
    assert sha(input_file) == reference["input_sha256"], "This entry point reproduces the fixed sample_000 input"
    cpus = cpu_selection(args.cpus)
    nm = subprocess.check_output(["nm", "-n", "--defined-only", str(binary)], text=True)
    callbacks = [line.split() for line in nm.splitlines() if "_omp_fn." in line]
    if len(callbacks) != 2 or not all(any(name in row[-1] for name in ("MillionaireWithEquality", "Equality")) for row in callbacks):
        raise ValueError("Unexpected OpenMP callback set; do not admit an unverified build")
    dynamic = subprocess.check_output(["ldd", str(binary)], text=True)
    if "not found" in dynamic:
        raise ValueError(dynamic)
    libraries = sorted({Path(p).resolve(strict=True) for p in re.findall(r"(/[^\s()]+)", dynamic)})
    output.mkdir(parents=True)
    os.chmod(output, 0o755)
    program, runtime = output / "program", output / "runtime"
    program.mkdir()
    runtime.mkdir()
    program.chmod(0o755)
    runtime.chmod(0o755)
    shutil.copy2(binary, program / "aqcf_parallel_portable")
    shutil.copy2(compiled / "CLIENT_LAYOUT.bin", program / "CLIENT_LAYOUT.bin")
    (program / "ALLOW.txt").write_text("".join("0x" + row[0] + "\n" for row in callbacks))
    for lib in libraries:
        shutil.copy2(lib, runtime / lib.name)
    # ldd resolves the dynamic loader's canonical path; keep its ELF interpreter name.
    loader = Path("/lib64/ld-linux-x86-64.so.2")
    if not (runtime / loader.name).exists():
        shutil.copy2(loader.resolve(strict=True), runtime / loader.name)
    for folder in (program, runtime):
        for path in folder.iterdir():
            path.chmod(0o555 if path.name not in ("CLIENT_LAYOUT.bin", "ALLOW.txt") else 0o444)
    session = secrets.token_hex(32)
    profile = json.loads((ROOT / "data/runtime_profiles.json").read_text())[args.model]
    release = dict(profile, real=True, session=session, workers=4, cpus=cpus, binary_sha256=sha(binary))
    save(output / "RELEASE.json", release)
    (output / "RELEASE.json").chmod(0o444)
    commands, probes = [], []
    for rank, original_asset in enumerate((compiled / "PROGRAM.bin", input_file)):
        owner = output / f"owner{rank}"
        owner.mkdir(mode=0o700)
        asset = owner / "asset.bin"
        shutil.copyfile(original_asset, asset)
        asset.chmod(0o444)
        proof = output / f"owner_{rank + 1}.txt"
        text = sha(compiled / "CLIENT_LAYOUT.bin") + "\n" + sha(asset) + "\n" + session + "\n"
        if args.model == "squeezenet":
            text += "SQNET_WAN_WAVE_REAL_20260919\n0\n4288\n"
        proof.write_text(text)
        proof.chmod(0o444)
        common = ["unshare", "--mount", "--pid", "--fork", sys.executable, "-B",
                  str(ROOT / f"tools/runtime/isolate_{args.model}.py"),
                  "--program", str(program), "--runtime", str(runtime), "--asset", str(asset),
                  "--release", str(output / "RELEASE.json"), "--proof", str(proof),
                  "--role", str(rank + 1), "--port", str(args.port), "--workers", "4"]
        for prefix, target in (("probe_", probes), ("", commands)):
            root, work = output / f"{prefix}root{rank}", output / f"{prefix}work{rank}"
            root.mkdir()
            work.mkdir()
            target.append(common + ["--root", str(root), "--work", str(work)] + (["--probe-only"] if prefix else []))
    save(output / "commands.json", {"probes": probes, "inference": commands})
    save(output / "preparation.json", dict(model=args.model, binary_sha256=sha(binary), cpus=cpus,
         model_hashes=binding["hashes"], input_sha256=sha(input_file), encrypted_inference_runs=0,
         deployment="local_cohost", note="Fresh preparation, not a WAN run or historical executable."))
    if not (args.probe or args.execute):
        print("Prepared fresh role commands in", output)
        print("No binary execution or network connection. Use a new output directory with --probe or --execute.")
        return
    os.sched_setaffinity(0, cpus)
    for index, command in enumerate(probes):
        with (output / f"probe{index}.stdout").open("x") as out, (output / f"probe{index}.stderr").open("x") as err:
            subprocess.run(command, stdout=out, stderr=err, check=True, timeout=60)
        assert "AQCF_PARALLEL_ROLE_ISOLATION " in (output / f"probe{index}.stdout").read_text()
    save(output / "loader_probes.json", {"status": "PASS", "roles": 2, "encrypted_inference_runs": 0})
    if not args.execute:
        print("Both owner-isolation and loader probes passed. No encrypted inference performed.")
        return
    process = pair_harness.pair(commands, output)
    save(output / "process.json", process)
    count = len(reference["scores"])
    result = output / f"work1/RESULTS/FINAL{count + 1}.i64"
    actual = struct.unpack("<" + str(count + 1) + "q", result.read_bytes())
    assert list(actual[:-1]) == reference["scores"], "Final integer scores differ from the retained exact reference"
    assert actual[-1] == reference["class_id"]
    roles = [json.loads((output / f"work{i}/RESULTS/RESULT.json").read_text()) for i in range(2)]
    report = dict(status="PASS", model=args.model, exact_scores=count, class_id=actual[-1],
                  integer_mismatches=0, deployment="local_cohost", roles=roles,
                  complete_process=process, encrypted_inference_runs=1)
    save(output / "result.json", report)
    print(f"PASS: all {count} integer scores match; class {actual[-1]}. Results: {output}")


if __name__ == "__main__":
    main()
