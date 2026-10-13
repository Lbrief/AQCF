# WAN deployment and retained transport topology

The paper's WAN experiment ran the client and server on different hosts. Both native AQCF executables kept loopback protocol sockets and carried them through authenticated SSH connections. Replacing the server's loopback address with a public address is therefore not the retained deployment method.

The source tree provides the native protocol, opaque relay, and a transport-plan generator. A complete portable cross-host controller has not yet been validated. The local `tools/run_native.py` runner must not be described as a WAN runner. This document records the components and synchronization needed to build that controller without changing the algorithm or its timing boundary.

## Data connections

Each of the four protocol lanes owns three native TCP endpoints:

| Endpoint | Native port in lane `j` | SSH connection |
|---|---|---|
| Main | `BASE + 1000*j` | Shared with that lane's GC endpoint |
| Reverse-direction I/O | `BASE + 1000*j + 50` | Separate connection |
| GC/spare | `BASE + 1000*j + 100` | Shared with that lane's main endpoint |

All twelve endpoints remain present. There are eight independent SSH data connections, with connection multiplexing disabled. Each local forward maps a client loopback native port `p` to server loopback port `p+5000`. The server relay then connects to the native server listener at `p`.

```text
client role -> client 127.0.0.1:p -> SSH over WAN
            -> server 127.0.0.1:(p+5000) -> opaque relay
            -> server role 127.0.0.1:p
```

The reverse-direction endpoint is still a server listener in the retained SCI transport. It does not require an SSH remote-forward (`-R`). Each TCP stream carries both byte directions.

`tools/runtime/wan_relay.py` preserves the recorded relay implementation. It uses `TCP_NODELAY`, 262144-byte socket send/receive buffer requests, and 65536-byte opaque copy chunks. It waits up to 120 seconds for a native listener to appear. It does not parse, cache, replay, or retry application protocol messages. Relay logs include per-port byte counts and termination state. The operating system may report a different effective socket buffer size.

Generate a reviewable plan without starting SSH or inference:

```sh
python tools/plan_wan_transport.py --server aqcf-server --base-port 47000 \
  --relay-cpus 8,10 --server-log /path/to/fresh/relay.log \
  --output local/wan-plan
```

Use actual allowed server CPU IDs in place of `8,10`. Keep transport helpers separate from the four physical compute cores when comparing to the paper's resource profile. The output contains eight SSH commands and `relay.json`; no command is executed by the generator. The SSH host alias belongs in the operator's SSH configuration. The repository does not contain account credentials, host keys, old server addresses, or cloud lifecycle scripts.

The relay can be started on the server after the operator installs the reviewed configuration:

```sh
python tools/runtime/wan_relay.py "$(cat /path/to/relay.json)"
```

Wait for its `TUNNEL_READY` log record, then start the eight data forwards. Keep their process handles and logs. Do not combine the streams into one SSH connection when attributing a timing difference to an algorithm change.

## Role preparation and synchronization

A cross-host controller must perform these steps:

1. Bind both roles to the same compiled binary, public layout, algorithm-switch profile, and fresh session identifier. Distribute the model program only to the server and the input only to the client. Retain SHA-256 bindings for the executable, layout, own asset, and dependencies.
2. Create fresh role work directories and OT state. Mount only the role's own asset, public layout, runtime, and output directory. Run the existing owner-isolation and loader probes before invoking the protocol.
3. Use the current `tools/runtime/isolate_squeezenet.py` or `isolate_resnet50.py` interface on each host. Supply that host's four physical compute CPU IDs. The server and client may have different CPU numbers.
4. Read both stdout streams without merging their identities. Send `C` to both roles only after both emit `ASSETS_READY`. Send `G` to both only after both emit `READY`. Keep control traffic outside the data streams. The local `pair_harness.py` demonstrates this two-barrier protocol; its host-local resource inventory is not a substitute for remote resource records.
5. Stop on any failed role, SSH connection, or relay. Preserve all outputs. Shut down only processes created for that session, retaining exit codes and final relay counters. Do not reconnect a partially consumed inference and call it the same run.
6. Compare every client output integer and the final class with the supplied clear reference. Retain server and client resource receipts, model/output hashes, transport logs, and all relevant timing boundaries.

The ResNet-50 transport profile is `WAN_NATIVE`. Its `LOCAL50` profile enables artificial pacing and must not be used for an actual WAN measurement. `CLOUD_PUBLIC` and `WAN_NATIVE` both disable that pacing in the recorded transport code, but retain the deployment label that was actually used. The historical SqueezeNet and ResNet-50 WAN measurements each used the same executable as their corresponding final cohost observation.

## What to measure

Report client-side native `CLOCK_MONOTONIC_RAW` time from the first input ciphertext frame through receipt of the final protected scores/class payload. Local reconstruction and result comparison follow that interval. Setup and complete-process time remain separate. Capture an independent monotonic boundary clock as a cross-check; the original Windows/WSL deployment also used Windows QPC markers.

Protocol bytes are application bytes and exclude IP and SSH framing. Relay bytes, SSH wire bytes, and native protocol counters have different boundaries; label them separately. Record the actual route, host specifications, CPU allocation, memory and swap constraints, RTT, throughput, and transport-helper CPU use. The original Windows route explicitly bound its data SSH sockets to the selected WLAN interface. A Linux SSH deployment or another network route is a new environment and must be identified as such.

Preparing the transport plan does not reproduce latency, verify a WAN path, or launch an experiment. No cross-host measurement was executed while preparing this repository.
