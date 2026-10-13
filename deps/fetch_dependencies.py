#!/usr/bin/env python3
"""Fetch pinned public dependency sources and build SEAL. Does not run inference."""
from pathlib import Path
import argparse, subprocess

HERE = Path(__file__).resolve().parent
PINS = {
    "seal": ("https://github.com/microsoft/SEAL.git", "119dc32e135cb89c1062076a69310d4413ebc824"),
    "eigen": ("https://gitlab.com/libeigen/eigen.git", "603e213d13311af286c8c1abd4ea14a8bd3d204e"),
}

def run(*args):
    subprocess.run(list(map(str, args)), check=True)

def fetch(name, url, commit):
    dest = HERE / "_sources" / name
    git = ["git", "-c", f"safe.directory={dest}", "-C", str(dest)]
    if not dest.exists():
        dest.mkdir(parents=True)
        run(*git, "init")
    if not (dest / ".git").is_dir():
        raise SystemExit(f"Refusing to overwrite non-git directory: {dest}")
    remote = subprocess.run([*git, "remote", "get-url", "origin"], capture_output=True, text=True)
    if remote.returncode:
        run(*git, "remote", "add", "origin", url)
    elif remote.stdout.strip() != url:
        raise SystemExit(f"Unexpected dependency remote in {dest}")
    dirty = subprocess.check_output([*git, "status", "--porcelain", "--untracked-files=no"], text=True)
    if dirty.strip():
        raise SystemExit(f"Local dependency edits found in {dest}; preserve them before fetching")
    current = subprocess.run([*git, "rev-parse", "HEAD"], capture_output=True, text=True)
    if current.returncode or current.stdout.strip() != commit:
        run(*git, "fetch", "--depth", "1", "origin", commit)
        run(*git, "checkout", "--detach", commit)
    actual = subprocess.check_output([*git, "rev-parse", "HEAD"], text=True).strip()
    if actual != commit:
        raise SystemExit(f"Commit mismatch for {name}")
    return dest

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--fetch-only", action="store_true")
    args = parser.parse_args()
    sources = {name: fetch(name, *pin) for name, pin in PINS.items()}
    if args.fetch_only:
        return
    build = HERE / "_build" / "seal"
    run("cmake", "-S", sources["seal"], "-B", build,
        "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={HERE / 'install'}",
        "-DSEAL_BUILD_DEPS=ON", "-DSEAL_USE_INTEL_HEXL=OFF",
        "-DSEAL_USE_ZLIB=ON", "-DSEAL_USE_ZSTD=ON", "-DSEAL_USE_MSGSL=ON",
        "-DSEAL_BUILD_EXAMPLES=OFF", "-DSEAL_BUILD_TESTS=OFF", "-DSEAL_BUILD_BENCH=OFF")
    run("cmake", "--build", build, "--parallel", args.jobs)
    run("cmake", "--install", build)

if __name__ == "__main__":
    main()
