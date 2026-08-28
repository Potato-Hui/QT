# GPIO Light Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a safe settings-page light toggle that drives RK3588 GPIO4_A4 high and low.

**Architecture:** A focused `GpioLightController` owns the Linux GPIO character-device line handle and exposes initialize, set, query, and shutdown operations. `MainWindow` only translates button intent into controller calls and updates UI from confirmed controller state; `INSULATOR_DESKTOP_PREVIEW` uses the same interface with an in-memory simulation.

**Tech Stack:** Qt 5.15 Widgets, C++14, Linux GPIO character-device UAPI, CMake, qmake.

**Spec:** User-provided GPIO light-control requirements in the 2026-08-28 task.

## Global Constraints

- Keep Qt 5.15, C++14, Windows DesktopUiPreview, and RK3588 Linux compatibility.
- GPIO4_A4 defaults to `/dev/gpiochip4`, line offset `4`, active-high.
- Centralize the chip path, line offset, and active level as macros.
- Do not invoke shell commands, `sudo`, `system()`, or background helpers.
- Do not change inference, camera, GStreamer, snapshot, history, quantification, or school-logo behavior.
- Do not add standalone test sources or test targets.
- Preserve CMake, `InsulatorMonitor.pro`, and `InsulatorMonitorSingle.pro` builds.
- Do not create a Git commit unless the user requests one.

---

### Task 1: GPIO light controller

**Files:**
- Create: `gpiolightcontroller.h`
- Create: `gpiolightcontroller.cpp`

**Interfaces:**
- Produces: `bool initialize(QString*)`, `bool setLightEnabled(bool, QString*)`, `bool isLightEnabled() const`, and `void shutdown()`.
- Configuration: `LIGHT_GPIO_CHIP_PATH`, `LIGHT_GPIO_LINE_OFFSET`, and `LIGHT_GPIO_ACTIVE_LEVEL`.

- [ ] Verify the controller files and configuration macros are absent.
- [ ] Add a non-copyable controller with preview simulation and Linux GPIO character-device branches.
- [ ] On Linux, open the configured gpiochip, request one output line with the disabled value, retain the line handle, and close the chip descriptor.
- [ ] Map `ENOENT`, `EACCES`/`EPERM`, `EBUSY`, and `EINVAL` to actionable Chinese error text.
- [ ] Make shutdown best-effort set the disabled level, close the line handle, and reset state.
- [ ] Compile through DesktopUiPreview after build integration.

### Task 2: Settings-page interaction

**Files:**
- Modify: `mainwindow.ui`
- Modify: `mainwindow.h`
- Modify: `mainwindow.cpp`
- Modify: `styles/white_theme.qss`

**Interfaces:**
- Consumes: `GpioLightController`.
- Produces: checkable `lightToggleButton`, `lightStatusLabel`, `toggleLight()`, and `updateLightUi()`.

- [ ] Verify the new widget names and MainWindow light slots are absent.
- [ ] Add the button and status label below the camera-mode controls without changing other pages.
- [ ] Initialize the controller to the safe low state during MainWindow construction.
- [ ] On click, request the new state, update text only after success, and restore actual state on failure.
- [ ] Show a concrete warning on failure while keeping camera and inference behavior unchanged.
- [ ] Call shutdown before deleting the UI.
- [ ] Add a restrained checked-state style for the active light button.

### Task 3: Build integration and verification

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `InsulatorMonitor.pro`
- Modify: `InsulatorMonitorSingle.pro`
- Modify: `README.md`

**Interfaces:**
- All application targets compile the controller.
- DesktopUiPreview defines `INSULATOR_DESKTOP_PREVIEW` and never opens GPIO devices.

- [ ] Add controller sources to the shared CMake list, desktop preview target, and both qmake projects.
- [ ] Document GPIO4_A4 mapping, permission expectations, active-level macro, and `gpioinfo` verification.
- [ ] Parse the UI and resource XML and run focused static assertions for widgets, macros, and build references.
- [ ] Build `DesktopUiPreview`.
- [ ] Run `git diff --check` and inspect the final diff for unrelated changes.
