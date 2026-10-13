#!/usr/bin/env python3
"""Fetch the exact upstream integer checkpoints, checking every file digest."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def download(url, target, expected_size):
    """Resume a short HTTP body only when the server confirms the exact range."""
    for attempt in range(6):
        offset = target.stat().st_size if target.exists() else 0
        if offset == expected_size:
            return
        if offset > expected_size:
            raise ValueError(f"Partial download is larger than the pinned source: {target}")
        headers = {"Accept-Encoding": "identity"}
        request_url = url
        if offset:
            headers["Range"] = f"bytes={offset}-"
            # Some caches retain a truncated 200 response. The revision still
            # pins the resource; the query separates the resumed HTTP request.
            request_url += f"?aqcf_resume={offset}"
        request = urllib.request.Request(request_url, headers=headers)
        with urllib.request.urlopen(request, timeout=90) as response:
            if offset and (response.status != 206 or not response.headers.get("Content-Range", "").startswith(f"bytes {offset}-")):
                raise ValueError("Server did not honor the exact resume range; use a local pinned checkout")
            with target.open("ab" if offset else "wb") as stream:
                shutil.copyfileobj(response, stream)
        if target.stat().st_size == offset:
            raise IOError("Download made no progress; partial source was retained")
    if target.stat().st_size != expected_size:
        raise IOError(f"Download is incomplete after six requests: {target}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", choices=["squeezenet", "resnet50", "all"], default="all")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--upstream", type=Path, help="Use a local OpenCheetah checkout instead of downloading")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "data/model_sources.json").read_text())
    names = list(manifest["models"]) if args.model == "all" else [args.model]
    args.output.mkdir(parents=True, exist_ok=True)
    for name in names:
        for item in manifest["models"][name]["files"]:
            target = args.output / name / item["filename"]
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                if sha(target) != item["sha256"]:
                    raise ValueError(f"Existing file has wrong digest: {target}")
                print(f"Verified existing {target}")
                continue
            temporary = target.with_suffix(target.suffix + ".partial")
            if temporary.exists() and args.upstream:
                raise FileExistsError(f"Inspect or remove unfinished download first: {temporary}")
            try:
                if args.upstream:
                    with temporary.open("xb") as out:
                        with (args.upstream / item["upstream_path"]).open("rb") as source:
                            shutil.copyfileobj(source, out)
                else:
                    download(item["url"], temporary, item["upstream_bytes"])
                actual = sha(temporary)
                if actual == item.get("upstream_sha256"):
                    # The retained experiment checkout used CRLF. Normalize only
                    # ASCII line endings; integer values and order do not change.
                    raw = temporary.read_bytes()
                    if b"\r" in raw:
                        raise ValueError("Unexpected carriage return in the pinned LF source")
                    temporary.write_bytes(raw.replace(b"\n", b"\r\n"))
                if sha(temporary) != item["sha256"]:
                    raise ValueError(f"Digest mismatch: {temporary}; retained for inspection")
                temporary.rename(target)
            except BaseException:
                # Preserve an interrupted or unexpected response for inspection.
                raise
            print(f"Verified {target}")


if __name__ == "__main__":
    main()
