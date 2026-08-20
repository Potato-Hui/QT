# 摄像头模式可靠切换设计

## 目标

在现有 Qt 5.15/C++14 应用内，以 `InferenceController` 作为唯一生命周期所有者，可靠地互斥管理可见光 RKNN/TCP-JPEG 来源与热像仪 RTSP 来源。设置页只选择模式，主页面的统一检测按钮负责启动和停止。

## 状态与所有权

- `CameraMode::VisibleLight` 为默认选择。
- `m_selectedMode` 表示用户最新选择，`m_activeMode` 表示当前正在启动、运行或清理的来源。
- 模式切换先更新选择，再按 `m_activeMode` 停止接收器、RKNN 进程和网卡配置进程；清理完成后回到 Idle，不自动启动新来源。
- 每次点击开始都先清理残留资源，再启动当前选择模式。

## 视频接收器

`GstVideoReceiver` 保持单实例。`start(VideoSource, url, latency)` 入口先调用 `stop()`：

- 可见光使用本机 `tcpclientsrc`、JPEG 解码和现有 appsink。
- 热像仪使用 TCP RTSP、H.264 解包、`mppvideodec`、BGR 转换和同一 appsink。
- 两种来源共用首帧、Bus ERROR/EOS、最新帧投递和停止清理逻辑。

## 热像仪网络与配置

控制器从可执行文件旁的 `thermal_camera.ini` 读取接口、地址、掩码、RTSP URL 和延迟。仓库只提交无密码示例，真实配置被 `.gitignore` 排除。

热像仪启动时使用受 Qt 管理的 `QProcess` 异步执行 `ifconfig`，程序与参数分开设置。只有正常退出且退出码为零时才创建 RTSP 管线。错误消息可以包含网卡配置标准错误，但日志和 UI 不得输出 RTSP URL。

## UI

设置页两个按钮 checkable 且由 `QButtonGroup` 互斥，默认勾选可见光。按钮点击发送选择请求；控制器发送确认消息和最终选择，主窗口据此同步选中样式与状态提示。

## 验证

不增加独立测试源或目标。构建 DesktopUiPreview 验证 UI 与跨平台头文件，使用源码契约检查确认互斥按钮、配置忽略、双管线和清理入口；RK3588 上人工验证真实 `ifconfig`、RTSP、mpp 解码、拔插恢复及进程/管线唯一性。
