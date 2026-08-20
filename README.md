# InsulatorMonitor

面向 RK3588 Linux 板卡的绝缘子智能检测 Qt 5 应用。Qt 负责界面、检测进程管理和 TCP-JPEG 视频显示，RKNN 程序负责摄像头采集、模型推理、检测框绘制和结果输出。

## 功能

- Qt 5.15 Widgets 白色主题界面
- 摄像头实时画面显示
- 显示检测框、类别、置信度和 FPS
- 通过 `QProcess` 启动和停止 RKNN 推理程序
- 通过本机 TCP-JPEG 接收最终检测画面
- 显示设备状态、电量、存储空间和检测记录等信息

## 可执行文件

工程可以生成两套 Qt 程序：

```text
bin/InsulatorMonitor
bin/InsulatorMonitorSingle
bin/single_model.ini
```

`InsulatorMonitor` 用于启动双模型推理程序，`InsulatorMonitorSingle` 用于启动单模型推理程序。两套程序共用摄像头 `/dev/video41` 和 TCP 端口 `127.0.0.1:5000`，不能同时运行检测。

## 单模型配置

单模型程序每次点击“开始检测”时，都会重新读取可执行文件旁边的 `single_model.ini`：

```ini
[Inference]
rknn_dir=/root/rknn-single
program=rknn_yolov8_demo
model=model/RK3588/best_yolov8_fp16.rknn
threads=3
```

路径说明：

- `rknn_dir`：RKNN 推理程序所在目录。
- `program`：推理程序文件名。
- `model`：相对于 `rknn_dir` 的模型路径，也可以使用绝对路径。
- `threads`：传给推理程序的线程数，必须是 1 到 12 之间的整数；每次点击“开始检测”都会重新读取。

修改 `model` 后，停止当前检测并再次点击“开始检测”即可加载新模型，不需要重新编译 Qt 程序。新模型需要保持与推理程序兼容的输入和输出结构。

## 编译

### 使用 CMake

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/path/to/target/qt5.15.8
cmake --build build -j4
```

请将工具链和 Qt 路径替换为实际路径，避免误用主机上的 Qt 库。

### 使用 qmake

```bash
/path/to/target/qt5.15.8/bin/qmake InsulatorMonitor.pro
make -j4
```

调试阶段可以使用普通窗口；部署到开发板时，可以在 `main.cpp` 中将：

```cpp
window.show();
```

改为：

```cpp
window.showFullScreen();
```

## 双模型程序启动

双模型程序默认使用以下设备和参数：

```text
Camera: /dev/video41
Format: MJPG
Resolution: 1280x720
TCP-JPEG: 127.0.0.1:5000
```

如果 RKNN 程序不在 Qt 可执行文件所在目录，可以通过环境变量指定目录：

```bash
export INSULATOR_RKNN_DIR=/root/rknn/insulator_pipeline_Linux
./InsulatorMonitor
```

该目录应至少包含：

```text
insulator_pipeline
model/RK3588/mixdet_fp16.rknn
model/RK3588/v8n-seg.rknn
```

如果不设置 `INSULATOR_RKNN_DIR`，程序会在自身所在目录查找上述文件。

## RKNN 程序通信

点击“开始检测”后，Qt 通过 `QProcess` 启动 RKNN 程序。推理程序完成模型、摄像头和 TCP 输出管线初始化后，应通过标准输出发送就绪消息，例如：

```text
@status {"state":"ready","width":1280,"height":720,"fps":30,"port":5000}
```

Qt 收到 `ready` 消息后，启动 GStreamer TCP-JPEG 接收管线：

```text
tcpclientsrc host=127.0.0.1 port=5000 !
jpegparse ! jpegdec ! videoconvert ! appsink
```

视频管线只保留最新帧，避免界面处理速度较慢时产生持续累积的延迟。RKNN 程序输出的 `@metrics` 消息用于更新 FPS 和延迟信息。

## 热像仪模式

热像仪模式不启动 RKNN。程序会先通过受 Qt 管理的 `ifconfig` 进程配置有线网卡，再用 GStreamer RTSP/appsink 将热成像帧送到主界面。

将 `thermal_camera.example.ini` 复制为可执行文件旁边的 `thermal_camera.ini`，再填写实际账号和密码：

```ini
[ThermalCamera]
interface=eth1
address=192.168.1.100
netmask=255.255.255.0
url=rtsp://username:password@192.168.1.111:554/video1/stream0
latency=0
```

`thermal_camera.ini` 已被 Git 忽略。程序需要以 root 身份运行，或拥有配置网卡所需的 `CAP_NET_ADMIN` 权限。热像仪解码依赖 `rtspsrc`、`rtph264depay`、`h264parse` 和 RK3588 的 `mppvideodec` 插件。

### 同帧拍照协议

Qt 拍照时会向 RKNN 程序标准输入写入一行：

```text
@snapshot {"request_id":"<uuid>","output_dir":"/data/records/<uuid>"}
```

RKNN 必须针对同一张未绘制画面的原始帧生成 `image.jpg`、`masks/*.png` 和
`metadata.json`，在临时目录完成后原子改名，再输出：

```text
@snapshot {"request_id":"<uuid>","state":"ready","record_dir":"/data/records/<uuid>"}
```

每个 mask 为与 `bbox_xyxy` 对应的单通道 8 位 PNG，像素只能是 `0` 或 `255`。
Qt 校验数据包后立即纳入拍照历史；用户在记录详情页点击“量化”时，才调用
`QtInsulatorQuantifier` 并在相同目录写入 `result.json`。

RK3588 构建会直接编译同级 `../lianghua/cpp_qualification` 中的量化源码，并要求
工具链 sysroot 提供 `opencv4`（`core`、`imgproc`、`imgcodecs`）。如量化目录不在
默认位置，可指定：

```bash
INSULATOR_QUANTIFIER_SOURCE_DIR=/path/to/cpp_qualification \
./build_rk3588.sh
```

将构建输出的 `pitch_area_model.json` 部署到板端：

```bash
install -D -m 644 build-cross-rk3588-new/bin/pitch_area_model.json \
  /data/config/pitch_area_model.json
```

Windows `DesktopUiPreview` 不启动 RKNN 或 OpenCV；点击拍照会生成演示数据包到
`%USERPROFILE%/InsulatorMonitor/data/<UUID>/`。详情页先通过“查看 JSON”检查
`metadata.json`，点击“量化”后生成模拟 `result.json`，再可通过同一窗口查看其内容。
历史页同时兼容 `InsulatorMonitor` 根目录和 `data` 目录中原有的单张 `snapshot_*.jpg`
照片。

## 板卡部署检查

确认必要的 GStreamer 插件：

```bash
for e in appsrc appsink queue videoconvert jpegenc jpegparse jpegdec \
         tcpserversink tcpclientsrc; do
    gst-inspect-1.0 "$e" >/dev/null 2>&1 \
        && echo "$e: YES" \
        || echo "$e: MISSING"
done
```

确认双模型文件可读：

```bash
test -x "$INSULATOR_RKNN_DIR/insulator_pipeline"
test -r "$INSULATOR_RKNN_DIR/model/RK3588/mixdet_fp16.rknn"
test -r "$INSULATOR_RKNN_DIR/model/RK3588/v8n-seg.rknn"
```

确认摄像头格式：

```bash
v4l2-ctl -d /dev/video41 --list-formats-ext
```

推荐使用 `MJPG`、`1280x720`、`30 fps`。模型内部仍使用自己的输入尺寸（例如 `640x640`），摄像头显示分辨率和模型输入分辨率可以不同。

## 当前限制

- 双模型和单模型程序不能同时使用同一个摄像头和 TCP 端口。
- `127.0.0.1:5000` 只允许本机接收视频；如需其他设备接收，需要调整 TCP 监听地址和网络防火墙配置。
- 电量、存储空间和 SQLite 检测记录需要根据具体板卡系统接口进一步接入。

