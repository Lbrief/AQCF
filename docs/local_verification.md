# Check your local setup

Run these commands from the AQCF root. Start with the quick checks, then prepare
the model and input before executing a complete inference.

## Quick checks

```sh
python3 tests/check_protocol_identities.py
python3 tools/summarize_results.py --output validation/tables
python3 -m compileall -q tools structured
```

The first command checks joint-carry identities, signed floor recovery, quarter
normalization, terminal widening, and trace-merge coefficient placement. It is
an arithmetic regression test. The second rebuilds five tables from retained
measurements and includes time ratios. Neither command measures inference speed.

## Rebuild and compare assets

Follow [data preparation](data.md), then [native model compilation](native_models.md)
or [structured model preparation](structured_models.md). Each compiler verifies
the complete program and public-layout hashes against the retained experiment
files. The inputs also have independent hash checks.

Build the C++ targets using [deps/README.md](../deps/README.md). For the native
models, run the preparation command and then the owner-isolation loader probe in
[native_models.md](native_models.md). Start full encrypted inference only with
the explicit `--execute` option. Keep all output directories fresh so failures
and earlier observations remain available.

## What to inspect

- Check that code paths and model names are understandable and belong to AQCF.
- Check the data and checkpoint source manifests, especially the separately
  derived MobileNet endpoint.
- Check that complete program/layout and fixed-input hashes match.
- Check both isolated parties load successfully before a full native run.
- For a full run, compare every final integer score and inspect timing scope;
  successful compilation alone does not establish inference correctness.

See [validation status](validation.md) for checks completed with this source.
The complete local SqueezeNet inference has passed. Full inference with the
other four models and the portable two-host WAN controller remain unverified.
Model downloads and their expected hashes are listed in [data.md](data.md).
