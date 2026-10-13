# Native dependencies

AQCF targets Linux x86-64 with AES-NI and AVX2. The bounded exact dot-product
kernel also uses FMA instructions. Install a C++17 compiler, CMake, Git, Python 3.11 or 3.12,
OpenSSL development headers, GMP development headers, Boost headers, zlib and
zstd development libraries. GCC 12 and GCC 13 were used in the retained builds.

On Ubuntu 24.04:

```sh
sudo apt-get install build-essential cmake git curl python3 python3-venv libssl-dev libgmp-dev \
  libboost-dev zlib1g-dev libzstd-dev
python3 deps/fetch_dependencies.py --jobs 2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
```

The native launcher uses Python's standard library only. Its `sudo python3`
commands use the system interpreter, which must also be Python 3.11 or newer;
activating a virtual environment does not change that interpreter automatically.

`fetch_dependencies.py` obtains Microsoft SEAL 4.1.2 at commit
`119dc32e135cb89c1062076a69310d4413ebc824` and Eigen at commit
`603e213d13311af286c8c1abd4ea14a8bd3d204e`. SEAL uses its pinned dependency
configuration for Microsoft GSL, zlib and zstd. Intel HEXL is disabled.

`bolt_sci/src` is the retained SCI source subset from
[BOLT](https://github.com/cmu-cryptosystems/BOLT), commit
`6911b1c26cb0405793e7b5a3e9effe67f659bc49`. Its MIT license is included.
`serial_sci` contains the retained transport-completion changes to three native
operators and the comparison header. These sources build `aqcf_serial_sci`;
the original subset builds `aqcf_sci`. No precompiled archive is shipped.

The model-specific `native/*/vendor` directories retain the precise SCI and
EMP header versions that entered their translation units. The native OT
headers originated in OpenCheetah's SCI and EMP dependencies. They include
AQCF's fresh-provider and buffer-tail changes. Their upstream notices are
retained in the files and in `licenses/`. Do not replace one model's header
snapshot with another merely because a file has the same name.

The SEAL adapter sources under `native/*/he_backend` are modified parallel
implementations of SEAL operations. They retain Microsoft's copyright and
license. Source hashes in `native/source_provenance.json` identify the
original inputs and the mechanically renamed public files. A new compiler
or build directory can change executable hashes and performance; the
archived executable hashes in the experiment records identify the measured
artifacts, rather than a promise of bit-identical binaries from every build.
