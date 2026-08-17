# 历史照片无边框窗口实现记录

## 记录信息

- 日期：2026-08-17
- 功能：历史照片查看窗口的无边框外观和自定义关闭键
- 适用端：RK3588 板载程序、Windows UI 预览
- 相关代码：[mainwindow.cpp](../mainwindow.cpp)、[white_theme.qss](../styles/white_theme.qss)

## 背景

板载主程序使用全屏窗口，历史照片点击后需要打开一个单独的图片查看窗口。默认的 Qt 对话框标题栏和关闭按钮尺寸较小，与工业白色触控界面不一致，因此历史照片窗口改为无边框对话框，并在内容区域内提供触控友好的关闭键。

这项实现只改变历史照片查看对话框，不改变板载主窗口的全屏行为，也不改变检测、拍照和历史记录接口。

## 实现方式

### 1. 创建无边框对话框

实现位于 `MainWindow::openHistoryPhoto()`：

```cpp
QDialog dialog(this);
dialog.setObjectName(QStringLiteral("historyPhotoDialog"));
dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
dialog.resize(960, 680);
```

- `Qt::FramelessWindowHint` 移除系统标题栏和系统关闭按钮。
- `historyPhotoDialog` 作为 QSS 选择器，提供白色背景和边框。
- 对话框默认大小为 `960 x 680`，适合显示历史图片和触控操作区域。

### 2. 增加自定义顶部栏

对话框内容顶部增加一个 `QHBoxLayout`，内部包括：

- 当前照片文件名：`historyPhotoTitleLabel`
- 关闭按钮：`historyPhotoCloseButton`
- 标题与关闭按钮之间的横向弹簧，用于将关闭按钮固定在右侧

顶部栏复用对话框的垂直布局，外边距为 `16, 12, 16, 16`，控件间距为 `10px`。

### 3. 增加触控关闭键

关闭键使用 `QToolButton`，不依赖系统标题栏：

```cpp
closeButton->setText(QStringLiteral("×"));
closeButton->setToolTip(QStringLiteral("关闭"));
closeButton->setFixedSize(52, 52);
connect(closeButton, &QToolButton::clicked,
        &dialog, &QDialog::accept);
```

点击关闭键会调用 `QDialog::accept()`，结束当前 `dialog.exec()`，返回历史记录页面。关闭键不会删除照片，也不会改变历史记录数据。

## QSS 样式

样式位于 `styles/white_theme.qss`，通过对象名限定到历史照片窗口：

| 选择器 | 作用 |
| --- | --- |
| `QDialog#historyPhotoDialog` | 白色背景、深色边框 |
| `QLabel#historyPhotoTitleLabel` | 深蓝色、加粗文件名 |
| `QToolButton#historyPhotoCloseButton` | 白色触控关闭键、边框和圆角 |
| `:hover` | 悬停时切换为红色警示色 |
| `:pressed` | 按下时使用更深的浅红背景 |

关闭键的实际外部尺寸为 `52 x 52px`。QSS 的 `50px` 内容尺寸与 `1px` 边框共同形成这个可触控的外部尺寸；代码也使用 `setFixedSize(52, 52)` 固定其布局尺寸。

另外，`QScrollBar:vertical` 的 `width` 已调整为 `30px`，使历史记录和其他页面右侧的竖向滚动条更适合触控操作。

## 板载端与 Windows 预览的关系

两端都使用同一个 `mainwindow.cpp` 和 `styles/white_theme.qss`：

- 板载端通过 `monitorapplication.cpp` 加载 `:/styles/white_theme.qss`。
- Windows 预览通过 `desktop_preview_main.cpp` 加载同一个资源，并以普通窗口方式运行。
- `CMakeLists.txt` 中的板载目标和 `DesktopUiPreview` 都使用 `mainwindow.cpp`、`mainwindow.ui` 和 `resources.qrc`。

因此，修改历史照片窗口的对象名、布局或 QSS 时，会同时影响板载端和 Windows 预览。Windows 预览的模拟摄像头画面和推理状态文字不属于这个窗口功能。

## 交互流程

```text
历史记录页点击照片
        |
        v
MainWindow::openHistoryPhoto()
        |
        +-- 检查图片是否可读
        |       |
        |       +-- 失败：显示错误提示
        |
        +-- 创建无边框 QDialog
        +-- 创建标题栏、文件名和关闭键
        +-- 显示图片
        +-- dialog.exec()
                |
                +-- 点击关闭键 -> QDialog::accept() -> 返回历史记录页
```

## 修改注意事项

1. 不要把 `Qt::FramelessWindowHint` 直接加到板载主窗口，板载主窗口仍由 `showFullScreen()` 管理。
2. 修改 QSS 时必须保留 `historyPhotoDialog`、`historyPhotoTitleLabel` 和 `historyPhotoCloseButton` 对象名，否则对应样式不会生效。
3. 关闭键只能负责关闭查看窗口，不应复用删除照片或返回历史页的业务逻辑。
4. 如果以后把照片详情改成 `QStackedWidget` 页面，需要重新评估这套 `QDialog` 实现，不要同时保留两套详情入口。

## 验证

Windows 预览已使用 Qt 6 工具链重新编译成功。`mainwindow_structure_test` 会创建临时照片、打开历史照片对话框，并验证：

- 对话框具有 `Qt::FramelessWindowHint`。
- 自定义关闭键存在且实际尺寸为 `52 x 52px`。
- 点击关闭键能结束对话框。

同时运行了 `latest_value_slot_test`、`inferencelifecycle_test`、`inferencelaunchspec_test`、`photoarchive_test` 和 `mainwindow_structure_test`。

板载端仍需在 RK3588 目标环境中使用实际 Qt 5 和 GStreamer 构建验证；本功能不依赖 GStreamer，只依赖共享的 Qt Widgets 界面代码和资源文件。
