# Repository preparation checks

The following checks were performed on the reorganized source. They establish
build and artifact consistency. A subsequent complete local SqueezeNet execution
also verified all final integer outputs. No new WAN measurement was performed.

| Check | Observed result |
|---|---|
| Fresh dependency build | Pinned SEAL and Eigen acquired from their public repositories; SEAL built and installed locally |
| Complete C++ build | All five model executables linked successfully; 41 translation units; no original private-source dependency in the compilation database |
| Native program compilation | SqueezeNet and ResNet-50 programs and public layouts exactly matched retained SHA-256 digests |
| Structured program compilation | All three programs and all three layouts exactly matched retained SHA-256 digests |
| Native model import | All 52 SqueezeNet tensors and 251 ResNet-50 tensors imported and hash checked |
| Native input preprocessing | 100 fixed images per native model, 200 resulting tensors, matched the retained input hashes |
| Structured input preparation | Three fixed real inputs and three public synthetic inputs matched the retained hashes |
| Native role loading | Both roles loaded inside their separate isolation environments for each freshly built native executable |
| CPU portability | Four allowed physical compute cores selected; invalid selections rejected; actual thread binding checked |
| Arithmetic identities | Joint carry, signed floor, quarter normalization, widening, division and trace placement checks passed |
| WAN transport plan | Twelve endpoints and eight SSH data connections generated; invalid base ports rejected; retained relay bytes unchanged |
| Complete local SqueezeNet encrypted inference | Both parties completed all 117 nodes; all 1000 integer scores and the final class matched the bound reference; both exit codes were zero |
| Encrypted inference for the other four reorganized binaries | Pending local execution |
| Two-host WAN execution using this repository | Not performed; portable cross-host controller not yet validated |

The native loader check runs the dynamic loader inside the prepared role
environment. It does not generate protocol keys or evaluate the graph. The
structured runner's `--check-only` option validates its supplied files without
starting either party. These checks must not be interpreted as output or
performance validation of a full inference.

The arithmetic checks exercise bounded identities and coefficient mappings;
they do not prove security of the cryptographic composition. The result-table
tool recomputes ratios from retained observations without performing experiments.

For the next validation step, use [local verification](local_verification.md)
and the native or structured execution instructions. A full inference must
compare every final integer score with its bound reference. Keep the resulting
deployment label and timing boundary with any new measurements.

See [data acquisition](data.md) for checkpoint locations and dataset terms,
and [NOTICE.md](../NOTICE.md) for the source and asset license status.
