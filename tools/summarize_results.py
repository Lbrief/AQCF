"""Regenerate comparison tables from retained observations; does not run inference."""
import argparse
import csv
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def table(path, rows):
    if not rows:
        return
    fields = list(dict.fromkeys(k for row in rows for k in row))
    with path.open("w", newline="", encoding="utf-8") as out:
        writer = csv.DictWriter(out, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "validation" / "tables")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    groups = json.loads((ROOT / "results" / "observations.json").read_text(encoding="utf-8"))
    current = groups["current_deployments"]
    reference = {r["architecture"]: r["online_seconds"] for r in current
                 if r["deployment"] == "real_two_host_WAN" and r["method"] == "AQCF"}
    rows = []
    for r in current:
        row = dict(r)
        if r["deployment"] == "real_two_host_WAN":
            row["baseline_time_over_aqcf"] = r["online_seconds"] / reference[r["architecture"]]
        rows.append(row)
    table(args.output / "native_deployments.csv", rows)
    removals = groups["structured_removals"]
    enabled = {r["group"]: r["service_raw_seconds"] for r in removals if r["arm"] == "ON"}
    rows = [dict(r, removed_time_over_all_on=r["service_raw_seconds"] / enabled[r["group"]])
            for r in removals]
    table(args.output / "structured_removals.csv", rows)
    pairs = [dict(r, control_time_over_candidate=r["control_seconds"] / r["candidate_seconds"])
             for r in groups["incremental_pairs"]]
    table(args.output / "incremental_pairs.csv", pairs)
    table(args.output / "earlier_fixed_inputs.csv", groups["earlier_fixed_inputs"])
    table(args.output / "earlier_model_comparisons.csv", groups["earlier_model_comparisons"])
    print("Wrote five tables to", args.output)
    print("A time ratio above 1 means AQCF / the enabled or candidate configuration is faster.")
    print("Read results/README.md for each experiment's timing scope and comparison limits.")


if __name__ == "__main__":
    main()
