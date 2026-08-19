# Camera Mode Switching Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add mutually exclusive visible-light and thermal-camera modes with one controller owning every process and GStreamer pipeline.

**Architecture:** `InferenceController` owns selected/active modes, the RKNN process, asynchronous network process, timers, and one source-aware `GstVideoReceiver`. `MainWindow` only presents selection and forwards user intent. Local thermal credentials remain outside Git.

**Tech Stack:** Qt 5.15 Widgets/Core, C++14, QProcess, QSettings, GStreamer 1.x appsink, CMake and qmake.

**Spec:** `docs/superpowers/specs/2026-08-19-camera-mode-switching-design.md`

## Global Constraints

- Keep Qt 5.15, C++14 and RK3588 Linux compatibility.
- Do not change RKNN command protocol, inference profile arguments, camera device or TCP port.
- Do not add standalone test sources or separate test targets.
- Never log the credential-bearing RTSP URL.
- Any start or mode switch must clean the previous managed resources first.

---

### Task 1: Shared camera and receiver source types

**Files:** Create `cameramode.h`; modify `gstvideoreceiver.h`, `gstvideoreceiver.cpp`, all build manifests.

**Interfaces:** Produce `CameraMode`, `VideoSource`, and `bool GstVideoReceiver::start(VideoSource, const QString &, int)`.

- [ ] Add the shared mode enum and source enum.
- [ ] Make receiver start call stop before building either exact requested pipeline.
- [ ] Retain one appsink callback, Bus polling, first-frame signal, latest-frame slot and complete NULL-state cleanup.
- [ ] Run DesktopUiPreview build to catch shared-header/UI regressions.

### Task 2: Controller lifecycle and thermal configuration

**Files:** Modify `inferencecontroller.h`, `inferencecontroller.cpp`; create `thermal_camera.example.ini`; modify `.gitignore` and build manifests.

**Interfaces:** Produce `selectCameraMode(CameraMode)`, `selectedMode()`, `activeMode()`, and `selectedModeChanged(CameraMode, QString)`.

- [ ] Add selected/active mode state and a unified asynchronous cleanup outcome.
- [ ] Preserve ready-gated RKNN/TCP startup for visible light.
- [ ] Load and validate the local INI without logging its URL.
- [ ] Configure the interface through a managed QProcess and start RTSP only after exit code zero.
- [ ] Route stop, switching, errors, timeouts, restart and destruction through the same receiver/process cleanup.
- [ ] Check source contracts for no `startDetached`, `sudo`, `sh -c`, or unmanaged gst-launch.

### Task 3: Settings UI and application wiring

**Files:** Modify `mainwindow.h`, `mainwindow.cpp`, `mainwindow.ui`, `monitorapplication.cpp`, `desktop_preview_main.cpp`, `styles/white_theme.qss`.

**Interfaces:** MainWindow emits `cameraModeSelectionRequested(CameraMode)` and accepts `setSelectedCameraMode(CameraMode, QString)`.

- [ ] Configure the existing two buttons as checkable and mutually exclusive with visible light selected.
- [ ] Forward clicks without starting detection and show selected/switching messages.
- [ ] Connect controller selection state in both deployable applications and keep desktop preview independent of GStreamer.
- [ ] Add an obvious checked-state style and preserve existing page navigation.

### Task 4: Focused verification

**Files:** Review all changed source, config, build and documentation files.

- [ ] Build the available DesktopUiPreview target.
- [ ] Run an offscreen startup smoke check.
- [ ] Run `git diff --check` and targeted source/config contract searches.
- [ ] Record Windows verification limits and an RK3588 manual matrix for network permissions, RTSP/mpp plugins, unplug/replug and repeated switching.
