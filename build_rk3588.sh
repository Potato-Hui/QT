#!/usr/bin/env bash

set -Eeuo pipefail

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${PROJECT_DIR:-$SCRIPT_DIR}"
RK3588_SDK="${RK3588_SDK:-/opt/atk-dlrk3588-toolchain}"
RK3588_SYSROOT="${RK3588_SYSROOT:-$RK3588_SDK/aarch64-buildroot-linux-gnu/sysroot}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build-cross-rk3588-new}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-$RK3588_SDK/share/buildroot/toolchainfile.cmake}"
PKG_CONFIG_TOOL="${PKG_CONFIG_TOOL:-$RK3588_SDK/bin/pkg-config}"

echo "=========================================="
echo "  RK3588 cross compile: InsulatorMonitor"
echo "=========================================="

[[ -d "$PROJECT_DIR" ]] || fail "项目目录不存在：$PROJECT_DIR"
[[ -d "$RK3588_SYSROOT" ]] || fail "sysroot 不存在：$RK3588_SYSROOT"
[[ -f "$TOOLCHAIN_FILE" ]] || fail "工具链文件不存在：$TOOLCHAIN_FILE"
[[ -x "$PKG_CONFIG_TOOL" ]] || fail "pkg-config 不可执行：$PKG_CONFIG_TOOL"
command -v cmake >/dev/null 2>&1 || fail "找不到 cmake"

cd "$PROJECT_DIR"

grep -q "openPhotoDetail" mainwindow.cpp \
    || fail "当前源码没有 openPhotoDetail，可能不是最新版本"

grep -q "photoDetailPage" mainwindow.ui \
    || fail "当前 mainwindow.ui 没有 photoDetailPage"

export PATH="$RK3588_SDK/bin:$PATH"
export PKG_CONFIG_SYSROOT_DIR="$RK3588_SYSROOT"
export PKG_CONFIG_LIBDIR="$RK3588_SYSROOT/usr/lib/pkgconfig:$RK3588_SYSROOT/usr/share/pkgconfig"
unset PKG_CONFIG_PATH

echo "检查 GStreamer..."

for module in gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0; do
    "$PKG_CONFIG_TOOL" --exists "$module" \
        || fail "sysroot 中找不到 $module"

    echo "$module: $("${PKG_CONFIG_TOOL}" --modversion "$module")"
done

PCFILEDIR="$("$PKG_CONFIG_TOOL" --variable=pcfiledir gstreamer-app-1.0)"

case "$PCFILEDIR" in
    *x86_64-linux-gnu*|*i386-linux-gnu*)
        fail "pkg-config 指向了主机库：$PCFILEDIR"
        ;;
esac

echo "GStreamer pcfiledir：$PCFILEDIR"

echo "检查 Qt5..."

QT5_CONFIG="$(find "$RK3588_SYSROOT" \
    -type f \
    -name Qt5Config.cmake \
    -print \
    -quit)"

[[ -n "$QT5_CONFIG" ]] || fail "sysroot 中找不到 Qt5Config.cmake"

QT5_DIR="$(dirname -- "$QT5_CONFIG")"

echo "Qt5Config.cmake：$QT5_CONFIG"

echo "配置 CMake..."

cmake -S "$PROJECT_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_PREFIX_PATH="$RK3588_SYSROOT/usr" \
    -DQt5_DIR="$QT5_DIR" \
    -DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG_TOOL" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DBUILD_MONITOR_APPS=ON \
    -DBUILD_DESKTOP_UI_PREVIEW=OFF

echo "开始编译..."

cmake --build "$BUILD_DIR" \
    --target InsulatorMonitor InsulatorMonitorSingle \
    --parallel "${BUILD_JOBS:-4}"

MONITOR_BIN="$BUILD_DIR/bin/InsulatorMonitor"
SINGLE_BIN="$BUILD_DIR/bin/InsulatorMonitorSingle"

[[ -x "$MONITOR_BIN" ]] || fail "未生成：$MONITOR_BIN"
[[ -x "$SINGLE_BIN" ]] || fail "未生成：$SINGLE_BIN"

echo "=========================================="
echo "编译完成："
ls -lh "$MONITOR_BIN" "$SINGLE_BIN" "$BUILD_DIR/bin/single_model.ini"

if command -v file >/dev/null 2>&1; then
    file "$MONITOR_BIN" "$SINGLE_BIN"
fi

echo
echo "请运行新编译的程序："
echo "$MONITOR_BIN"
echo
echo "不要继续运行旧的 ./bin/InsulatorMonitor"
echo "=========================================="
