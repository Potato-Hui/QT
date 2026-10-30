# RK3588 Qt C++14 集成接口

## 1. 模块职责

本模块接收 YOLO/RKNN 后处理得到的绝缘子与缺陷实例 mask，完成：

- mask gating、几何有效性和 `reference_eligible`
- ideal_area V1、Pitch LINEAR 系数和 Theil–Sen 尺度预测
- 缺陷 union、归属、面积与比例计算
- damage V1.1 基础损伤指数、保守风险指数、等级和建议

正式部署入口是 `InsulatorQuantifier::processMasks()`。默认参数
`reuse_metadata_geometry=false` 表示完整 mask-first 计算。

## 2. 不负责的内容

模块不负责 YOLO/RKNN 推理、RKNN tensor 解码、mask 阈值化、摄像头、RGA、
GStreamer、TCP、GUI 或 Qt 界面。上述内容应留在 Qt 主程序或 RKNN 后处理适配层。

## 3. 文件移植分类

### A. 可直接加入 Qt 主项目

- `include/InsulatorQuantifier.hpp`
- `src/Gating.cpp`
- `src/IdealArea.cpp`
- `src/DamageEvaluator.cpp`

这些文件使用 C++14、STL、OpenCV core/imgproc，无 Qt、Windows 或 MSVC API。

### B. 需要轻量接口或平台适配

- `src/InsulatorQuantifier.cpp`
  - `processMasks()` 的输入检查、二值化和核心调度可用于 RK3588。
  - 同一文件还包含 `processFromFiles()`、`cv::imread`、
    `<opencv2/imgcodecs.hpp>`、JSON/PNG 文件适配和结果文件写入。
  - 当前文件作为一个翻译单元编译时需要 OpenCV imgcodecs。若 RK3588 只有
    core/imgproc，应把文件接口拆到单独的 PC 适配源文件，或用构建宏排除
    imgcodecs/file-I/O 部分；不要改动 `processMasks()`、gating、ideal-area 或
    damage 算法。
- `config/pitch_area_model.json`
  - 可作为部署配置，由 Qt 程序启动时读取一次并构造 `CalibrationModel`。
  - 也可将其中已冻结的必要字段直接初始化为常量，但不得改变数值。

### C. RK3588 Qt 正式部署不需要

- `app/main.cpp`：命令行文件测试程序，不是 Qt/RKNN 正式入口。
- `test/`：CTest、fixture、回归输出和 Python fixture 生成脚本，仅用于开发验证。
- `CMakeLists.txt`：独立模块的 CMake/CTest 配置；已有 qmake 主项目无需加入。
- `build*`、生成的可执行文件和测试结果文件。

## 4. OpenCV 依赖

核心算法需要：

- OpenCV core：`cv::Mat`、`cv::PCA`、矩、位运算、计数等
- OpenCV imgproc：轮廓、`minAreaRect`、椭圆拟合、填充等

`processFromFiles()` 额外需要 OpenCV imgcodecs，以读取原图和 PNG mask。
OpenCV `cv::FileStorage` 属于 core，但当前文件接口与 imgcodecs 位于同一个
`InsulatorQuantifier.cpp` 中。

模块不使用 highgui、videoio、dnn、CUDA 或其他深度学习框架。

## 5. `processMasks()` 正式接口

实际声明：

```cpp
insulator::QuantificationResult processMasks(
    const std::vector<cv::Mat>& disc_masks,
    const std::vector<insulator::DiscResult>& disc_metadata,
    const std::vector<insulator::DefectMask>& defect_masks,
    const insulator::CalibrationModel& calibration,
    bool reuse_metadata_geometry = false) const;
```

RK3588 正式调用必须保持 `reuse_metadata_geometry=false`。此时 C++ 会自行计算
gating、`reference_eligible`、几何量和 ideal area，不读取历史复用字段。

### Disc 输入

`disc_masks` 与 `disc_metadata` 数量和顺序必须一一对应。每个
`DiscResult` 输入只需设置：

- `instance_id`：上游实例 ID；同一批 disc 内必须唯一
- `class_id = 0`
- `class_name = "insulator"`
- `confidence`：有限浮点数

为兼容旧数据，class 0 也接受 `class_name = "insulator_disc"`。

正式 mask-first 输入不需要设置：

- `mask_quality`、`geometry_validity`、`local_failure`
- `reference_eligible`
- `centroid_x`、`centroid_y`、`axis_position`
- `major_span`、`multi_axis_measurement_valid`、`pose_ratio_q75`
- `predicted_major_span`、`coefficient`、`ideal_area`
- 所有 `frozen_*` 字段

### Defect 输入

每个 `DefectMask` 设置：

- `mask`
- `instance_id`
- `class_id`
- `class_name`
- `confidence`

类别名称严格对应：

| class_id | class_name |
|---:|---|
| 0 | `insulator`（兼容 `insulator_disc`） |
| 1 | `crack` |
| 2 | `pollution` |
| 3 | `flashover` |
| 4 | `broken` |

缺陷输入只允许 class 1～4，且名称必须与 ID 一致。

## 6. Mask 格式要求

传给 `processMasks()` 的所有 disc 和 defect mask 必须满足：

- 类型为 `CV_8UC1`
- 是完整图像坐标系下的 full-image mask
- 所有 mask 的宽高完全一致
- 非零像素表示 foreground，零表示 background

函数会用 `cv::compare(mask, 0, ..., cv::CMP_GT)` 将 disc mask 标准化为
0/255。不要直接传入浮点概率、logit 或 RKNN tensor；应先在 RKNN 后处理层
完成阈值化与坐标恢复。

输入 `cv::Mat` 以 const 引用传入。核心为 disc 创建内部二值 mask，不要求调用方
移交原始缓冲区所有权。调用期间必须保证外部缓冲区仍然有效。

## 7. Bbox/ROI mask 恢复规则

若 RKNN 后处理输出的是 bbox 大小的 ROI mask，适配层应按半开区间
`[x1, y1, x2, y2)` 恢复到整图：

```cpp
cv::Mat full = cv::Mat::zeros(image_height, image_width, CV_8UC1);
cv::Mat roi = full(cv::Rect(x1, y1, x2 - x1, y2 - y1));
roi_mask.copyTo(roi);
cv::compare(full, 0, full, cv::CMP_GT);
```

适配层应先验证：

- `0 <= x1 < x2 <= image_width`
- `0 <= y1 < y2 <= image_height`
- `roi_mask.cols == x2 - x1`
- `roi_mask.rows == y2 - y1`
- ROI mask 已转换为 `CV_8UC1`

这一步属于 RKNN 后处理适配层，不应写入或修改量化核心。

## 8. `processFromFiles()` 测试接口

实际声明：

```cpp
insulator::QuantificationResult processFromFiles(
    const std::string& image_path,
    const std::string& metadata_path,
    const std::string& calibration_path,
    const std::string& result_path,
    bool verbose = false,
    bool reuse_metadata_geometry = false) const;
```

该接口读取原图、metadata JSON、PNG mask 和 Pitch JSON，并写出结果 JSON，适合
Windows/PC 文件回归测试。它支持完整尺寸 PNG，也支持由 metadata 中
`bbox_xyxy` 指定的 ROI PNG。

仅安装 OpenCV core/imgproc 的 RK3588 无法按当前源码直接编译此接口，因为
`cv::imread` 和 `<opencv2/imgcodecs.hpp>` 依赖 imgcodecs。正式 Qt/RKNN 部署应
直接调用 `processMasks()`。

## 9. `CalibrationModel`

结构字段：

```cpp
struct CalibrationModel {
    std::string version;
    std::string feature_name;
    std::string method;
    bool valid;
    double slope;
    double intercept;
};
```

当前冻结配置中实际使用的值是：

```cpp
insulator::CalibrationModel calibration;
calibration.version = "PITCH_IDEAL_AREA_V1";
calibration.feature_name = "pose_ratio_q75";
calibration.method = "LINEAR";
calibration.valid = true;
calibration.slope = 1.5601636488425434;
calibration.intercept = -0.10735866032344729;
```

推荐 Qt 程序在启动时用 `QFile` + `QJsonDocument` 读取
`pitch_area_model.json` 一次，填充一个只读 `CalibrationModel`，后续重复传给
`processMasks()`。这样不需要 imgcodecs，配置仍可核验且不重复解析。

若部署环境要求完全无配置文件，也可直接使用以上冻结值初始化，但应保留数值精度
并通过现有 fixture 回归确认。

## 10. `QuantificationResult`

整串字段：

| 字段 | 含义 |
|---|---|
| `basic_mean` | 有效单片基础损伤指数均值 |
| `basic_max` | 单片基础损伤指数最大值 |
| `basic_damage_index` | 整串基础损伤指数 |
| `conservative_mean` | 有效单片保守风险指数均值 |
| `conservative_max` | 单片保守风险指数最大值 |
| `conservative_risk_index` | 整串保守风险指数，等级主依据 |
| `overall_level` | `正常`、`轻度`、`中度` 或 `严重` |
| `recommendation` | 对应处理建议 |
| `highest_risk_disc_id` | 保守风险最高单片的排序后 `id` |
| `disc_count` | 参与量化的绝缘子片数量 |
| `defect_instance_count` | 输入缺陷实例数量（union 前） |
| `discs` | 排序后的每片 `DiscResult` |

`highest_risk_disc_id` 对应 `DiscResult::id`，不是上游 `instance_id`。

## 11. `DiscResult` 输出字段

### 身份与 gating

| 字段 | 含义 |
|---|---|
| `id` | 沿绝缘子串 PCA 轴排序后的连续 ID |
| `instance_id` | 上游 YOLO/RKNN 实例 ID |
| `class_id`、`class_name`、`confidence` | 输入实例属性 |
| `mask_quality` | `NORMAL`、`SUSPECT` 或 `ANOMALOUS` |
| `geometry_validity` | `VALID`、`UNCERTAIN` 或 `INVALID` |
| `local_failure` | `NONE`、`LOCAL_CONTOUR_DEVIATION`、`MAJOR_MASK_PROTRUSION`、`OCCLUSION_OR_TRUNCATION`、`FIT_UNSTABLE` 或 `UNCERTAIN` |
| `reference_eligible` | 是否进入 ideal-area reference 集合 |

### 几何与 ideal area

| 字段 | 含义 |
|---|---|
| `centroid_x`、`centroid_y` | mask 质心 |
| `axis_position` | 沿绝缘子串主轴的位置 |
| `major_span` | 当前 mask 的多轴主跨度测量 |
| `multi_axis_measurement_valid` | 多轴测量是否有效 |
| `pose_ratio_q75` | 多截面宽度的线性 Q75/major span |
| `predicted_major_span` | Theil–Sen 预测主跨度 |
| `coefficient` | Pitch LINEAR 得到的面积系数 |
| `ideal_area` | 最终理想面积 |

`frozen_geometry_present`、`frozen_axis_position`、`frozen_major_span`、
`frozen_multi_axis_measurement_valid`、`frozen_pose_ratio_q75` 是历史严格回归复用字段，
正式 mask-first 调用不应设置或消费它们。

### 缺陷与评分

| 字段 | 含义 |
|---|---|
| `crack_area`、`pollution_area`、`flashover_area`、`broken_area` | 各类别 union 后与当前 disc 相交的像素面积 |
| `crack_ratio`、`pollution_ratio`、`flashover_ratio`、`broken_ratio` | 各缺陷面积除以 `ideal_area` |
| `basic_damage_index` | 当前单片基础损伤指数 |
| `conservative_risk_index` | 当前单片保守风险指数 |

## 12. RKNN → Quantifier 推荐数据流

```text
RKNN output tensors
  -> YOLO/RKNN 后处理与实例筛选
  -> class_id/name/confidence
  -> mask 阈值化为 CV_8UC1
  -> ROI mask 按 bbox 恢复为 full-image mask（如需要）
  -> 构造 disc_masks / disc_metadata / defect_masks
  -> InsulatorQuantifier::processMasks(..., false)
  -> QuantificationResult
  -> Qt 界面、业务逻辑或序列化层
```

不要把 RKNN tensor、NPU 句柄或 YOLO 解码代码引入 `InsulatorQuantifier`。

## 13. 最小调用示例

```cpp
#include "InsulatorQuantifier.hpp"

insulator::InsulatorQuantifier quantifier;

// disc_masks、disc_metadata、defect_masks 已由 RKNN 后处理适配层构造。
// calibration 在程序启动时读取一次。
const insulator::QuantificationResult result = quantifier.processMasks(
    disc_masks,
    disc_metadata,
    defect_masks,
    calibration
);
```

建议在 Qt 调用边界捕获 `std::exception`，将错误转换为日志、状态码或 Qt signal：

```cpp
try {
    const insulator::QuantificationResult result = quantifier.processMasks(
        disc_masks, disc_metadata, defect_masks, calibration);
    // 消费 result
} catch (const std::exception& error) {
    const QString message = QString::fromUtf8(error.what());
    // 记录或上报错误
}
```

## 14. Qt/qmake 集成提示

### qmake 3.1

目标板若具有 imgcodecs，可原样加入全部当前源文件：

```qmake
CONFIG += c++14

INCLUDEPATH += $$PWD/path/to/cpp_quantification/include

HEADERS += $$PWD/path/to/cpp_quantification/include/InsulatorQuantifier.hpp

SOURCES += \
    $$PWD/path/to/cpp_quantification/src/InsulatorQuantifier.cpp \
    $$PWD/path/to/cpp_quantification/src/Gating.cpp \
    $$PWD/path/to/cpp_quantification/src/IdealArea.cpp \
    $$PWD/path/to/cpp_quantification/src/DamageEvaluator.cpp

LIBS += -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
```

目标板只有 core/imgproc 时，先对 `InsulatorQuantifier.cpp` 做上述轻量拆分或条件
编译，仅保留 `processMasks()` 及其输入校验/调度，然后链接：

```qmake
LIBS += -lopencv_core -lopencv_imgproc
```

`Gating.cpp`、`IdealArea.cpp`、`DamageEvaluator.cpp` 不需要修改。

### Qt CMake

```cmake
target_sources(your_app PRIVATE
    path/to/cpp_quantification/src/InsulatorQuantifier.cpp
    path/to/cpp_quantification/src/Gating.cpp
    path/to/cpp_quantification/src/IdealArea.cpp
    path/to/cpp_quantification/src/DamageEvaluator.cpp)

target_include_directories(your_app PRIVATE
    path/to/cpp_quantification/include)
```

当前 `InsulatorQuantifier.cpp` 原样加入时还需链接 `opencv_imgcodecs`；完成 core/file
轻量拆分后，正式内存路径只链接 `opencv_core`、`opencv_imgproc`。

## 15. 平台兼容注意事项

实际源码检查结果：

- 核心 `include/`、`src/` 中没有 `windows.h`、`SetConsoleOutputCP`、`CP_UTF8`。
- 没有 Windows 专用路径或 Windows API；绝对 Windows 测试路径仅出现在 `test/`。
- 没有编译器专用 pragma。
- 没有 `std::filesystem`、`std::optional`、`std::variant`、
  `std::string_view`、structured binding、`if constexpr` 等 C++17 特性。
- `CMakeLists.txt` 的 `/utf-8` 和 DLL 复制仅位于 `if(MSVC)`，不影响 ARM64/GCC；
  qmake 项目不需要复制这些规则。
- `app/main.cpp` 使用普通 CLI/iostream，无 Windows API，但正式 Qt 部署不需要。
- 中文等级和建议存放在 UTF-8 `std::string` 中，转换为 Qt 字符串时使用
  `QString::fromUtf8(...)`；源文件应保持 UTF-8。
- 唯一影响 core/imgproc-only 构建的实际问题是
  `src/InsulatorQuantifier.cpp` 对 imgcodecs 文件接口的编译依赖。

## 16. 错误处理

模块通过 `std::invalid_argument` 或 `std::runtime_error` 报告输入与计算错误，包括：

- disc mask/metadata 数量不一致
- mask 类型或尺寸不一致
- class ID/name 冲突
- disc `confidence` 非有限或 `instance_id` 重复
- 标定模型无效
- reliable reference 少于两个或 axis position 不可用于 Theil–Sen
- ideal area、缺陷比例或其他数值无效

Qt 主程序应在调用边界捕获异常，不要让异常穿过 Qt event loop 或线程入口。

## 17. 当前验证状态

- 标准：C++14
- 已用 MSVC Release 构建通过
- CTest 的 2.jpg、5.jpg mask-first 与历史复用回归通过
- gating、reference、ideal area、damage 和类别名称兼容测试通过
- class 0 正式接受 `insulator`，同时兼容 `insulator_disc`
- 已使用新生成的 9 号输入目录完成文件接口测试

