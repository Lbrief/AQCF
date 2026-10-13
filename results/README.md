# Retained experimental observations

`observations.json` contains the numerical observations used in the manuscript.
It is an export of retained measurements, not a new benchmark run. The conversion
preserves numbers and selects public fields; it excludes machine addresses,
credentials, private filesystem paths, and controller records.

Regenerate the tables, including time ratios:

```sh
python3 tools/summarize_results.py --output validation/tables
```

| Group | Records | Scope |
|---|---:|---|
| `current_deployments` | 8 | Final SqueezeNet and ResNet-50: AQCF cohost observations and three-method WAN comparisons |
| `structured_removals` | 33 | Earlier real-input mechanism removals on three model/dataset combinations |
| `incremental_pairs` | 11 | Complete-graph control/candidate measurements with distinct parent versions |
| `earlier_fixed_inputs` | 8 | Earlier fixed-100-image aggregate observations |
| `earlier_model_comparisons` | 26 | Earlier model/dataset/framework comparisons |

For WAN, the reported speedup is baseline time / AQCF time, within the same
architecture. For a removal, the ratio is removed-configuration time / all-on
time. For an incremental pair, it is control time / candidate time. A value above
1 favors AQCF, all-on, or the candidate, respectively. Ratios below 1 are retained.

The older structured-removal records retain both raw and monotonic clock times.
Their clock discrepancy is unresolved. The raw-time ratios describe those
observations and do not establish an isolated contribution to final WAN speedup.
The incremental pairs use public synthetic inputs, trained models, and different
parent versions. Their gains cannot be added or multiplied, and the IFMA trial
was not adopted. Cross-framework timing endpoints also require care; the earlier
aggregate comparisons are not measurements of the final WAN executables.

The `n` fields record the actual sampling protocol. Do not treat an exact score
check on one input as population accuracy, manufacture variance estimates, or
infer network-only overhead by subtracting separate cohost and WAN runs.

`source_hashes.json` identifies retained source records by digest. The original
records remain in the authors' archive. Re-running the code generates fresh
cryptographic keys and measurements; elapsed time will depend on the machine and
network. The historical executable digests identify the measured binaries, not
the portable rebuilds after source renaming and CPU selection changes.
