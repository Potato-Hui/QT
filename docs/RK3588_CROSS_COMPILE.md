# InsulatorMonitor RK3588 交叉编译指南

本文只说明如何在 x86_64 Ubuntu 主机上，将本项目交叉编译为可在 RK3588 Linux 开发板上运行的 ARM64 程序。

> RKNN-Toolkit2 用于在电脑上把 ONNX 等模型转换成 `.rknn`，这一过程不是交叉编译，因此本文不包含模型转换。交叉编译使用的是 RK3588 工具链、目标板 sysroot，以及 sysroot 内的 Qt、GStreamer、OpenCV 等开发文件。

## 1. 编译环境

推荐使用 Ubuntu 20.04 或 Ubuntu 22.04 x86_64：

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config file
```

项目默认路径如下：

```text
项目源码：       ~/QtProject/gptproject/QT
RK3588工具链：  /opt/atk-dlrk3588-toolchain
sysroot：       /opt/atk-dlrk3588-toolchain/aarch64-buildroot-linux-gnu/sysroot
量化程序源码：  项目同级目录 ../lianghua/cpp_qualification
构建输出：      build-cross-rk3588-new
```

这些路径均可通过环境变量修改，不需要改构建脚本。

## 2. 获取工具链和 sysroot

交叉工具链应从开发板厂商提供的、与板端当前固件匹配的 SDK 中获取。不要随意使用另一套板卡或另一版固件的工具链，因为 Qt、GStreamer、OpenCV、glibc 和 libstdc++ 必须与目标系统保持 ABI 兼容。

本项目按以下结构使用工具链：

```text
/opt/atk-dlrk3588-toolchain/
├── bin/
│   ├── aarch64-buildroot-linux-gnu-gcc
│   ├── aarch64-buildroot-linux-gnu-g++
│   └── pkg-config
├── aarch64-buildroot-linux-gnu/
│   └── sysroot/
└── share/buildroot/toolchainfile.cmake
```

从厂商下载并解压工具链。压缩包名称仅为示例：

```bash
sudo mkdir -p /opt
sudo tar -xf atk-dlrk3588-toolchain.tar.xz -C /opt
sudo chown -R "$USER":"$USER" /opt/atk-dlrk3588-toolchain
```

如果厂商提供完整 RK3588 SDK，应优先使用其中的工具链和 sysroot。仅安装通用 `aarch64-linux-gnu-g++` 通常不够，因为本项目还依赖目标板版本的 Qt、GStreamer 和 OpenCV。

## 3. 检查工具链

```bash
export RK3588_SDK=/opt/atk-dlrk3588-toolchain
export RK3588_SYSROOT="$RK3588_SDK/aarch64-buildroot-linux-gnu/sysroot"
export PATH="$RK3588_SDK/bin:$PATH"

"$RK3588_SDK/bin/aarch64-buildroot-linux-gnu-gcc" --version
"$RK3588_SDK/bin/aarch64-buildroot-linux-gnu-g++" --version
test -f "$RK3588_SDK/share/buildroot/toolchainfile.cmake"
test -d "$RK3588_SYSROOT"
```

如果编译器前缀不同，应以厂商工具链的实际名称为准，并使用与之配套的 `toolchainfile.cmake`，不要混用两套交叉编译器。

## 4. 检查 sysroot 依赖

设置目标 `pkg-config` 环境，避免错误链接 Ubuntu 主机上的 x86_64 库：

```bash
export PKG_CONFIG_SYSROOT_DIR="$RK3588_SYSROOT"
export PKG_CONFIG_LIBDIR="$RK3588_SYSROOT/usr/lib/pkgconfig:$RK3588_SYSROOT/usr/share/pkgconfig"
unset PKG_CONFIG_PATH
```

### 4.1 Qt 5

```bash
find "$RK3588_SYSROOT" -name Qt5Config.cmake
```

必须找到目标板使用的 Qt 5 配置。如果找不到，需要从匹配的开发板 SDK 安装 Qt 开发文件，或使用相同工具链交叉编译 Qt 5.15。不能让 CMake 使用主机 `/usr/lib/x86_64-linux-gnu` 中的 Qt。

### 4.2 GStreamer 和 OpenCV

```bash
PKG_CONFIG_TOOL="$RK3588_SDK/bin/pkg-config"

"$PKG_CONFIG_TOOL" --modversion gstreamer-1.0
"$PKG_CONFIG_TOOL" --modversion gstreamer-app-1.0
"$PKG_CONFIG_TOOL" --modversion gstreamer-video-1.0
"$PKG_CONFIG_TOOL" --modversion opencv4
```

确认 `.pc` 文件来自目标 sysroot：

```bash
"$PKG_CONFIG_TOOL" --variable=pcfiledir gstreamer-app-1.0
```

输出中不能出现 `/usr/lib/x86_64-linux-gnu` 或 `/usr/lib/i386-linux-gnu`。

抽查动态库架构：

```bash
find "$RK3588_SYSROOT/usr/lib" -name 'libQt5Core.so*' -print -quit | xargs file
find "$RK3588_SYSROOT/usr/lib" -name 'libgstreamer-1.0.so*' -print -quit | xargs file
```

结果应包含 `ARM aarch64`，不能是 `x86-64`。

## 5. 准备项目源码

首次获取项目：

```bash
cd ~/QtProject/gptproject
git clone <本项目的Git地址> QT
cd QT
```

更新已有项目：

```bash
cd ~/QtProject/gptproject/QT
git pull
```

RK3588 构建还会编译量化源码，默认要求存在：

```text
../lianghua/cpp_qualification/src/InsulatorQuantifier.cpp
```

若量化源码位于其他位置，可指定：

```bash
INSULATOR_QUANTIFIER_SOURCE_DIR=/实际路径/cpp_qualification \
./build_rk3588.sh
```

## 6. 推荐方式：使用项目脚本

```bash
cd ~/QtProject/gptproject/QT
chmod +x build_rk3588.sh
./build_rk3588.sh
```

脚本会自动：

1. 检查工具链、sysroot、量化源码和项目关键文件；
2. 设置目标 `pkg-config` 环境；
3. 检查 Qt 5、GStreamer 和 OpenCV；
4. 使用 Release 模式配置 CMake；
5. 编译双模型和单模型应用；
6. 检查最终可执行文件是否生成。

可按需覆盖路径和并行数：

```bash
PROJECT_DIR=/源码目录 \
RK3588_SDK=/工具链目录 \
BUILD_DIR=/构建目录 \
INSULATOR_QUANTIFIER_SOURCE_DIR=/量化源码目录 \
BUILD_JOBS=8 \
./build_rk3588.sh
```

## 7. 手动使用 CMake

需要排查脚本问题时，可执行等效的手动配置：

```bash
cd ~/QtProject/gptproject/QT

export RK3588_SDK=/opt/atk-dlrk3588-toolchain
export RK3588_SYSROOT="$RK3588_SDK/aarch64-buildroot-linux-gnu/sysroot"
export PATH="$RK3588_SDK/bin:$PATH"
export PKG_CONFIG_SYSROOT_DIR="$RK3588_SYSROOT"
export PKG_CONFIG_LIBDIR="$RK3588_SYSROOT/usr/lib/pkgconfig:$RK3588_SYSROOT/usr/share/pkgconfig"
unset PKG_CONFIG_PATH

export QT5_CONFIG="$(find "$RK3588_SYSROOT" -type f -name Qt5Config.cmake -print -quit)"
export QT5_DIR="$(dirname "$QT5_CONFIG")"

cmake -S . \
    -B build-cross-rk3588-new \
    -DCMAKE_TOOLCHAIN_FILE="$RK3588_SDK/share/buildroot/toolchainfile.cmake" \
    -DCMAKE_PREFIX_PATH="$RK3588_SYSROOT/usr" \
    -DQt5_DIR="$QT5_DIR" \
    -DPKG_CONFIG_EXECUTABLE="$RK3588_SDK/bin/pkg-config" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_MONITOR_APPS=ON \
    -DBUILD_DESKTOP_UI_PREVIEW=OFF \
    -DINSULATOR_QUANTIFIER_SOURCE_DIR="$PWD/../lianghua/cpp_qualification"

cmake --build build-cross-rk3588-new \
    --target InsulatorMonitor InsulatorMonitorSingle \
    --parallel 4
```

不要在同一构建目录中混合桌面 x86_64 构建和 RK3588 交叉构建。切换工具链后应使用新的构建目录。

## 8. 编译产物

默认产物位于：

```text
build-cross-rk3588-new/bin/InsulatorMonitor
build-cross-rk3588-new/bin/InsulatorMonitorSingle
build-cross-rk3588-new/bin/single_model.ini
build-cross-rk3588-new/bin/pitch_area_model.json
```

- `InsulatorMonitor`：双模型 Qt 应用；
- `InsulatorMonitorSingle`：单模型 Qt 应用；
- `single_model.ini`：单模型配置；
- `pitch_area_model.json`：量化标定配置，部署到 `/data/config/pitch_area_model.json`。

检查架构：

```bash
file build-cross-rk3588-new/bin/InsulatorMonitor
file build-cross-rk3588-new/bin/InsulatorMonitorSingle
```

正确结果应包含 `ELF 64-bit` 和 `ARM aarch64`。如果显示 `x86-64`，说明 CMake 使用了主机编译器，不能部署到 RK3588。

## 9. 部署到开发板

```bash
ssh root@<RK3588_IP> 'mkdir -p /opt/InsulatorMonitor /data/config'

scp build-cross-rk3588-new/bin/InsulatorMonitor \
    root@<RK3588_IP>:/opt/InsulatorMonitor/

scp build-cross-rk3588-new/bin/InsulatorMonitorSingle \
    root@<RK3588_IP>:/opt/InsulatorMonitor/

scp build-cross-rk3588-new/bin/single_model.ini \
    root@<RK3588_IP>:/opt/InsulatorMonitor/

scp build-cross-rk3588-new/bin/pitch_area_model.json \
    root@<RK3588_IP>:/data/config/pitch_area_model.json
```

RKNN 推理程序和 `.rknn` 模型不是由上述 Qt 构建自动生成的，还要部署到项目配置所要求的位置。双模型目录至少应包含：

```text
insulator_pipeline
model/RK3588/mixdet_fp16.rknn
model/RK3588/v8n-seg.rknn
```

单模型使用的程序和模型路径由 `single_model.ini` 决定。

## 10. 板端运行前检查

```bash
ssh root@<RK3588_IP>
cd /opt/InsulatorMonitor
chmod +x InsulatorMonitor InsulatorMonitorSingle

file ./InsulatorMonitor
ldd ./InsulatorMonitor
file ./InsulatorMonitorSingle
ldd ./InsulatorMonitorSingle
```

`ldd` 输出中不能出现 `not found`。如果缺少 Qt、GStreamer、OpenCV 或 RKNN Runtime，应安装匹配板端系统的目标库，或者随程序部署并设置：

```bash
export LD_LIBRARY_PATH=/opt/InsulatorMonitor/lib:$LD_LIBRARY_PATH
```

运行双模型应用：

```bash
./InsulatorMonitor
```

运行单模型应用：

```bash
./InsulatorMonitorSingle
```

双模型和单模型程序共用摄像头及 `127.0.0.1:5000`，不能同时运行检测。

## 11. 常见问题

### 找不到 Qt5Config.cmake

sysroot 中没有 Qt 5 开发文件，或者使用了错误的 sysroot。应补充与板端固件匹配的 Qt 开发文件，不能改成主机 Qt 路径绕过。

### pkg-config 找到了 x86_64 库

重新设置：

```bash
export PKG_CONFIG_SYSROOT_DIR="$RK3588_SYSROOT"
export PKG_CONFIG_LIBDIR="$RK3588_SYSROOT/usr/lib/pkgconfig:$RK3588_SYSROOT/usr/share/pkgconfig"
unset PKG_CONFIG_PATH
```

并确认 CMake 使用 `/opt/atk-dlrk3588-toolchain/bin/pkg-config`。

### GLIBC 或 GLIBCXX 版本不匹配

出现 `GLIBC_x.xx not found` 或 `GLIBCXX_x.xx not found`，说明工具链/sysroot与板端系统版本不匹配。应换成板端固件配套 SDK 重新编译，不要通过复制主机 glibc 解决。

### Exec format error

使用 `file` 检查程序。如果显示 `x86-64`，说明生成的是电脑程序，应使用新的交叉构建目录和正确工具链重新配置。

### 找不到 librknnrt.so

Qt程序或推理程序依赖 RKNN Runtime。应使用与板端 NPU 驱动及 `.rknn` 模型版本兼容的 `librknnrt.so`，不要混用不同版本。

### file: could not find any valid magic files

通常是编译主机的 `file` 命令或 magic 数据库不完整，不表示前面的 CMake 编译失败。可修复：

```bash
sudo apt install --reinstall file libmagic1
```

然后重新运行 `file <可执行文件>` 检查架构。

## 12. 检查清单

- [ ] 工具链来自当前 RK3588 固件对应的 SDK；
- [ ] 编译器目标为 AArch64；
- [ ] sysroot 中存在 Qt 5、GStreamer 和 OpenCV；
- [ ] `pkg-config` 没有引用主机 x86_64 库；
- [ ] 使用独立的 `build-cross-rk3588-new` 构建目录；
- [ ] 两个应用目标均成功生成；
- [ ] `file` 显示程序为 `ARM aarch64`；
- [ ] 板端 `ldd` 没有 `not found`；
- [ ] RKNN推理程序、`.rknn`模型和配置文件已部署；
- [ ] 双模型和单模型程序没有同时运行。
