# AQCF

[English](README.md) | [简体中文](README.zh-CN.md)

<p align="center">
  <a href="docs/media/aqcf-overview.gif"><img src="docs/media/aqcf-overview.gif?v=bodoni-matrix" alt="aqcf 完整算法动图：矩阵乘法、通道打包、迹合并、保留相位和联合进位" width="100%"></a>
</p>

[查看大图](docs/media/aqcf-overview.png) · [高清视频](docs/media/aqcf-overview.mp4) · [动图说明](docs/media/README.md)

**面向客户端与云服务器的精确整数隐私推理。**

AQCF 让客户端获得神经网络预测，同时保护输入图像不被云服务器获知，并保护服务器的模型权重不被客户端获知。同态加密负责仿射层，双方安全计算协议负责非线性操作。在论文给出的数值范围和安全假设下，协议保留对应明文计算的整数输出。

本仓库提供算法实现、模型编译器、数据工具和论文保留的实验观测。**第一次使用建议按下面四步运行 SqueezeNet**，在本地启动双方协议并逐项检查输出。

[快速检查](#quick-check) · [完整推理](#complete-inference) · [数据与权重](#data-and-weights) · [实验结果](#results) · [验证情况](#validation-status)

## 算法思想

AQCF 围绕隐私推理中的三类计算展开：仿射层与非线性层交界处的商恢复、非线性协议中的进位转换，以及加密输出的系数提取。

1. **保留相位的商接口。** 将密文分量 `c0` 保留为本地相位，只传递 `c1`，再通过受保护的低位校正恢复所需有符号商的份额。这减少了仿射计算与非线性计算边界上需要传递的信息，并在给定数值范围内保留校正后的整数结果。[分量保留实现](native/squeezenet/engine/retained_squeezenet_component.hpp) · [商恢复实现](native/squeezenet/core/quarter_consumer_fusion.hpp)。
2. **掩码编程的联合进位。** 对查表的随机输出掩码进行编程，使后续前缀阶段在推进下一位置的布尔进位时，同步推导当前输入进位的算术份额。进位转换与前缀递推共享这一步计算。[联合进位实现](native/squeezenet/protocol/squeezenet_joint_carry_bridge.inc)。
3. **面向输出的迹合并。** 在投影到所需输出之前，递归合并系数迹，同时保持系数索引和缩放关系。对于适用的输出布局，这减少了密钥切换操作。[迹合并](native/resnet50/engine/output_trace_merge.hpp) · [求值实现](native/resnet50/src/trace_evaluator.cpp)。

[源码说明](native/README.md)给出这些算法与完整模型实现的对应关系。算术检查验证有界恒等式和系数映射；协议安全性还依赖论文中说明的后端和组合假设。

<a id="quick-check"></a>
## 快速检查：无需下载数据和模型

准备 Git 和 Python 3.11 或 3.12，然后运行：

```sh
git clone https://github.com/Lbrief/AQCF.git
cd AQCF
python3 tests/check_protocol_identities.py
python3 tools/summarize_results.py --output validation/quick-check/tables
```

第一条 Python 命令检查算术恒等式，成功时输出 `"status": "PASS"`。第二条根据保留的实验观测生成五份 CSV 表格，其中包含加速比。这两步不执行加密推理，也不测量新的推理时间。

<a id="complete-inference"></a>
## 第一次完整运行：SqueezeNet

以下命令均在克隆后的仓库根目录执行，使用 **Ubuntu 24.04 或 WSL2 中的 Ubuntu 24.04 终端**。原生代码需要 x86-64 处理器，支持 AES-NI、AVX2 和 FMA，并有四个可用物理核心。不需要 GPU。启动器会自动选择合适的 CPU 编号；运行结果应放在 Linux 文件系统中。

### 1. 构建可执行程序

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake git curl python3 python3-venv \
  binutils util-linux libssl-dev libgmp-dev libboost-dev zlib1g-dev libzstd-dev

python3 deps/fetch_dependencies.py --jobs 2
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target aqcf_squeezenet --parallel 2
```

依赖下载脚本和 CMake 使用普通用户运行。脚本会下载固定版本的 SEAL、Eigen，并构建所需依赖。详细说明见[构建指南](deps/README.md)。

### 2. 获取指定权重并编译模型

```sh
python3 -m venv .venv-data
. .venv-data/bin/activate
python -m pip install -r tools/data/requirements-squeezenet.txt

python tools/fetch_models.py --model squeezenet --output assets/weights
python tools/prepare_models.py --model squeezenet \
  --weights assets/weights/squeezenet/weights.inp \
  --output assets/models/squeezenet
python tools/compile_models.py --model squeezenet \
  --tensors assets/models/squeezenet --output assets/compiled/squeezenet
```

这些命令导入公开的整数模型权重，生成服务器程序和客户端公开布局，并核对它们是否与最终实验模型的哈希一致。无需重新训练。

### 3. 准备指定 ImageNet 图像

按照 [ImageNet 的获取要求](https://www.image-net.org/download.php)，从 2012 验证集中取得 `ILSVRC2012_val_00000001.JPEG`。先创建目录，再将原始 JPEG 放入其中，保留原文件名：

```sh
mkdir -p datasets/imagenet/val
```

确认 `datasets/imagenet/val/ILSVRC2012_val_00000001.JPEG` 已存在后运行：

```sh
python tools/prepare_data.py --model squeezenet \
  --images datasets/imagenet/val --indices 0 --output assets/inputs/squeezenet
```

工具同时核对原图字节和预处理后的整数张量。这次示例只需要这一张固定图像。任意替换图片或重新编码的 JPEG 都无法通过对应参考结果的绑定检查。原有 100 张图像的选择方式及其他输入来源见[数据准备说明](docs/data.md)。

### 4. 启动双方并检查全部输出

```sh
sudo python3 --version
sudo python3 tools/run_native.py --model squeezenet \
  --binary build/native/aqcf_squeezenet \
  --compiled-model assets/compiled/squeezenet \
  --input assets/inputs/squeezenet/sample_000/INPUT_NCHW.i64 \
  --output /var/tmp/aqcf-squeezenet-first-run --execute
```

`sudo` 使用的系统 Python 必须是 3.11 或 3.12；Ubuntu 24.04 提供的是 3.12。激活虚拟环境不会改变 `sudo python3` 使用的解释器。

`sudo` 用于建立参与方的隔离环境；随后两个协议参与方分别以非特权用户运行。启动器先检查动态加载，再启动新的协议会话，最后逐项比较全部分数。成功时输出行以以下内容开头：

```text
PASS: all 1000 integer scores match; class 65.
```

结果和计时字段位于 `/var/tmp/aqcf-squeezenet-first-run/result.json`，同目录保留双方日志和资源采样。每次运行都应使用新的输出目录；启动器拒绝覆盖已有目录。数据准备命令也会保留已有输出。

这是一次**本地双端推理**。客户端的 `service_monotonic_seconds` 从首个加密输入帧开始计时，到收到最终受保护结果为止；初始化和本地最终重构分别记录。报告也保留 RAW 时钟数据。引用结果时请同时保留计时边界和部署类型。仅准备文件、加载检查及 ResNet-50 的运行方式见[原生模型复现指南](docs/native_models.md)。

<a id="data-and-weights"></a>
## 数据与权重

| 模型 | 图像来源 | 指定权重来源 |
|---|---|---|
| Native SqueezeNet、ResNet-50 | [ImageNet 2012 验证集](https://www.image-net.org/download.php) | 固定版本的 [OpenCheetah 整数权重](https://github.com/Alibaba-Gemini-Lab/OpenCheetah/tree/f71d4d49ffde81701fc3cc2d887fae62381fb472/pretrained)，由 `tools/fetch_models.py` 下载 |
| Structured ResNet-18 / CIFAR-100 | [CIFAR-100](https://www.cs.toronto.edu/~kriz/cifar.html) | PrivCirNet 作者权重；见[下载来源和哈希](data/structured_model_sources.json) |
| Structured ResNet-18 / Tiny ImageNet | [Tiny ImageNet](https://cs231n.stanford.edu/tiny-imagenet-200.zip) | PrivCirNet 作者权重；见[下载来源和哈希](data/structured_model_sources.json) |
| Structured MobileNetV2 / CIFAR-100 | [CIFAR-100](https://www.cs.toronto.edu/~kriz/cifar.html) | AQCF 使用的固定派生权重，作为独立发布附件提供 |

从 `models-v1` 发布附件下载 MobileNetV2 派生权重：[`cifar_mobilenetv2_state.pt`](https://github.com/Lbrief/AQCF/releases/download/models-v1/cifar_mobilenetv2_state.pt)。其 SHA-256 为：

```text
0e889f03a6ee93017f4c4fa4cd1a3c5a3ea5b2a465cd9bde45afc2104ad39e8a
```

上游原始 MobileNet 权重不能直接替代这个文件。[数据说明](docs/data.md)记录预处理、固定样本、来源版本和校验值；[Structured 模型指南](docs/structured_models.md)给出权重导入、编译、运行和消融方法。下载的数据集、权重、生成的模型文件及运行结果均不纳入 Git。

<a id="results"></a>
## 结果与消融加速比

仓库保留论文中的实验观测，并提供表格生成工具。时间比统一按以下方式理解：

- **跨框架加速比**：同一模型、同一比较范围内，基线时间 / AQCF 时间。
- **机制消融**：关闭机制后的时间 / 全部机制开启时的时间。
- **逐项改动**：对应实验中的对照时间 / 候选实现时间。

比值大于 1，分别表示 AQCF、全部机制开启的配置或候选实现更快。33 个消融设置属于较早的 Structured 模型，并不是最终 Native 版本的全因子消融。比较数值前请阅读[结果及比较范围](results/README.md)。[WAN 指南](docs/wan.md)说明保留的传输设计和跨主机运行仍需验证的部分。

<a id="validation-status"></a>
## 验证情况

代码分为两组：`native/` 是最终 SqueezeNet 和 ResNet-50 实现；`structured/` 是较早用于机制消融的三个模型配置，共有 33 个适用的消融设置。两组代码的开关和实验范围不同，请使用对应指南。

下表描述本仓库重新整理后的代码验证情况：

| 模型与数据集 | 代码组 | 从源码构建 | 程序、布局和输入哈希 | 完整加密推理 |
|---|---|---|---|---|
| SqueezeNet / ImageNet | Native | 通过 | 一致 | **本地通过：1000 个分数及分类索引全部一致** |
| ResNet-50 / ImageNet | Native | 通过 | 一致 | 尚未使用本仓库执行 |
| ResNet-18 / CIFAR-100 | Structured | 通过 | 一致 | 尚未使用本仓库执行 |
| ResNet-18 / Tiny ImageNet | Structured | 通过 | 一致 | 尚未使用本仓库执行 |
| MobileNetV2 / CIFAR-100 | Structured | 通过 | 一致 | 尚未使用本仓库执行 |

已完成的 SqueezeNet 验证在同一台主机上运行两个隔离的协议参与方。本仓库的跨主机 WAN 复现尚未完成验证；论文原有的 WAN 实验结果保留在 [results](results/README.md)。各项检查的依据见[验证记录](docs/validation.md)。

## 目录导览

| 路径 | 内容 |
|---|---|
| [`native/`](native/README.md) | 最终 SqueezeNet 和 ResNet-50 协议实现 |
| [`structured/`](docs/structured_models.md) | 较早的模型配置与机制消融流程 |
| [`tools/aqcf_compiler/`](tools/aqcf_compiler/) | 系数编译器 |
| [`data/`](docs/data.md) | 公开计算图、参考绑定和权重来源记录 |
| [`tests/`](tests/) | 算术恒等式与系数映射检查 |
| [`results/`](results/README.md) | 保留的实验观测与比较范围 |

## 作者与声明

Keliang Xing、Ru Xue、Hejuan Du、Guangxiu Pan、Jianglu Wen。
Xizang Minzu University, Xianyang, Shaanxi 712082, China.

当前许可状态及第三方声明见 [NOTICE.md](NOTICE.md)。
