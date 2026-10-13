# Code and asset notices

This repository contains the AQCF research artifact.
No repository-wide license has been assigned to the authors' AQCF additions.
Do not interpret a third-party dependency's license as a license for the whole
repository or for its model weights and datasets.

The included BOLT SCI subset retains its MIT license. Microsoft SEAL and EzPC/SCI
derived files retain their Microsoft notices. EMP headers retain their notices.
The corresponding license texts are under `deps/licenses/` and
`deps/bolt_sci/LICENSE`. TensorFlow preprocessing retains its Apache-2.0 notice
under `tools/data/`.

OpenCheetah model files are fetched from their pinned upstream source; they are
not redistributed in the source tree. ImageNet, CIFAR-100, Tiny ImageNet, and
PrivCirNet checkpoints remain subject to their providers' terms. See
`docs/data.md` and the source manifests for acquisition and attribution.

Source-file provenance records describe the retained implementations and the
packaging changes. Renaming files, adapting local paths, and making CPU selection
explicit do not turn upstream code into original AQCF algorithms.
