`resnet_preprocessing.py` is derived from TensorFlow Authors' image preprocessing
code distributed in [EzPC Athos](https://github.com/mpc-msri/EzPC/tree/f24bf3e022dde3493cf0ffe3ebd92dd73936b734/Athos/Networks/ResNet/PreProcessingImages)
at commit `f24bf3e022dde3493cf0ffe3ebd92dd73936b734`.

Copyright 2016 The TensorFlow Authors. It is licensed under Apache-2.0; see
`LICENSE-Apache-2.0.txt` and the original header retained in the source. AQCF's
portability change replaces `import tensorflow as tf` with
`import tensorflow.compat.v1 as tf`. The preprocessing arithmetic is unchanged.

The native graph descriptions were extracted from OpenCheetah's published
`networks/main_sqnet.cpp` and `networks/main_resnet50.cpp` at the revision recorded
in `data/model_sources.json`. Checkpoints are fetched from that upstream project;
they are not relicensed or redistributed in this repository. OpenCheetah's
`pretrained/README.md` attributes those assets to the EzPC Athos network exports.
