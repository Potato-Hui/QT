# Industrial White UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved three-page industrial-white Qt Widgets interface, including touch-friendly sizing and truthful local photo history/detail operations.

**Architecture:** Keep `MainWindow` as the UI coordinator and the existing `QStackedWidget` as the page router. Move filesystem enumeration/deletion/export into a small `PhotoArchive` unit so destructive behavior is independently testable, while leaving `InferenceController`, GStreamer and QProcess boundaries unchanged.

**Tech Stack:** C++14, Qt 5 Core/Gui/Widgets, Qt Designer `.ui`, QSS, CMake/CTest, qmake compatibility.

## Global Constraints

- Target runtime is Qt 5 on RK3588 Linux at 1280×720 fullscreen.
- Do not modify RKNN inference, TCP-JPEG, model arguments or process lifecycle behavior.
- Header contains only the two-line text title and right-aligned clock/date.
- Footer contains only left-aligned plain text storage and session snapshot count.
- Do not display fabricated class, confidence or historical detection metadata.
- Destructive photo actions require confirmation and report failures.

---

### Task 1: Testable local photo archive

**Files:**
- Create: `photoarchive.h`
- Create: `photoarchive.cpp`
- Create: `tests/photoarchive_test.cpp`
- Modify: `CMakeLists.txt`
- Modify: `InsulatorMonitor.pro`
- Modify: `InsulatorMonitorSingle.pro`

**Interfaces:**
- Produces: `PhotoRecord { QString path; QString fileName; QDateTime modified; qint64 bytes; QSize imageSize; }`.
- Produces: `QVector<PhotoRecord> PhotoArchive::records() const` sorted newest first.
- Produces: `bool PhotoArchive::remove(const QString&, QString*) const`, `PhotoClearResult PhotoArchive::clear() const`, and `bool PhotoArchive::exportPhoto(const QString&, const QString&, QString*) const`.

- [ ] **Step 1: Write the failing archive test**

Create two image files and one unrelated text file in a `QTemporaryDir`; assert newest-first listing, image dimensions, single-file removal, export byte equality, and clear behavior that leaves the text file untouched.

- [ ] **Step 2: Add a native-test-only CMake path and verify the archive test fails**

Add `option(BUILD_MONITOR_APPS "Build deployable monitor applications" ON)` and guard only the two GStreamer-dependent application targets and `configure_file()` with it. Configure native Windows tests with:

```powershell
C:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build-tests -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw810_64/bin/g++.exe `
  -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/mingw81_64 `
  -DBUILD_MONITOR_APPS=OFF -DBUILD_TESTING=ON
C:\Qt\Tools\CMake_64\bin\cmake.exe --build build-tests --target photoarchive_test
```

Expected: FAIL because `photoarchive.h/.cpp` and the target do not exist.

- [ ] **Step 3: Implement the minimal archive unit**

Use `QDir::entryInfoList({"*.jpg", "*.jpeg", "*.png"}, QDir::Files | QDir::Readable, QDir::Time)`, `QImageReader::size()`, `QFile::remove()` and `QFile::copy()`. Reject an export destination equal to the source and remove an existing destination only after the UI has confirmed overwrite.

- [ ] **Step 4: Register sources for both build systems**

Add `photoarchive.cpp/.h` to `MONITOR_COMMON_SOURCES`, both `.pro` files, and add `photoarchive_test` linked against `Qt5::Core` and `Qt5::Gui` under `BUILD_TESTING`.

- [ ] **Step 5: Run the focused test**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests -R photoarchive_test --output-on-failure`

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add photoarchive.h photoarchive.cpp tests/photoarchive_test.cpp CMakeLists.txt InsulatorMonitor.pro InsulatorMonitorSingle.pro
git commit -m "feat: add testable photo archive operations"
```

### Task 2: Three-page UI structure

**Files:**
- Modify: `mainwindow.ui`
- Modify: `mainwindow.h`
- Modify: `mainwindow.cpp`
- Create: `tests/mainwindow_structure_test.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces object names used by `MainWindow`: `monitorPage`, `recordsPage`, `photoDetailPage`, `detailImageLabel`, `detailFileNameLabel`, `detailFileSizeValueLabel`, `detailImageSizeValueLabel`, `detailPathValueLabel`, `backToRecordsButton`, `zoomOutButton`, `zoomResetButton`, `zoomInButton`, `fitImageButton`, `exportPhotoButton`, `deletePhotoButton`, `clearRecordsButton`, `recordsCountLabel`.
- Removes object names: `batteryProgress`, `batteryValueLabel`, `deviceStatusDot`, `deviceStatusLabel`, `recordsNavButton`.

- [ ] **Step 1: Write the failing structure test**

Instantiate `MainWindow` with `QT_QPA_PLATFORM=offscreen`, find the three pages and required buttons by object name, assert `pageStack->count() == 3`, and assert removed header/footer objects are absent.

- [ ] **Step 2: Run the structure test and verify failure**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests -R mainwindow_structure_test --output-on-failure`

Expected: FAIL because the detail page and new controls do not exist.

- [ ] **Step 3: Replace the Designer layout**

Rebuild `mainwindow.ui` around one shared 84 px header, a three-page `QStackedWidget`, and one 54 px footer. Use a 68:32 realtime split, a five-column history table, and a 72:28 detail split. Keep all existing object names that `MainWindow` and `monitorapplication.cpp` use for realtime behavior. Remove obsolete constructor connections and battery/device widget accesses from `mainwindow.cpp/.h` so this task ends in a compiling state; inference and preview behavior remain unchanged.

- [ ] **Step 4: Register and run the structure test**

Link the target with `mainwindow.cpp`, `photoarchive.cpp`, Qt Core/Gui/Widgets, enable the offscreen test environment, then run the focused CTest target.

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add mainwindow.ui mainwindow.h mainwindow.cpp tests/mainwindow_structure_test.cpp CMakeLists.txt
git commit -m "feat: add three-page touchscreen UI structure"
```

### Task 3: History and photo-detail behavior

**Files:**
- Modify: `mainwindow.h`
- Modify: `mainwindow.cpp`
- Create: `tests/mainwindow_photo_flow_test.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- `MainWindow(QWidget* parent = nullptr, const QString& storagePath = QString())` uses the production path when `storagePath` is empty and a temporary test directory otherwise.
- Produces private slots: `openPhotoDetail(int,int)`, `openRecordsPage()`, `openMonitorPage()`, `openPreviousPage()`, `zoomDetailIn()`, `zoomDetailOut()`, `resetDetailZoom()`, `fitDetailImage()`, `exportCurrentPhoto()`, `deleteCurrentPhoto()`, `clearHistoryPhotos()`.
- Produces helper methods: `showPhotoDetail(const QString&)`, `updateDetailPixmap()`, `updateHistoryCount()`.

- [ ] **Step 1: Write the failing photo-flow test**

Create a temporary JPEG, construct `MainWindow` with that directory, invoke `openRecordsPage`, verify one history row, invoke the row handler and verify `photoDetailPage` is selected with correct file name/size/dimensions. Verify zoom-in increases the displayed scale state through a public test-visible label (`zoomValueLabel`).

- [ ] **Step 2: Run the test and verify failure**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests -R mainwindow_photo_flow_test --output-on-failure`

Expected: FAIL because the detail routing and file information are not implemented.

- [ ] **Step 3: Integrate `PhotoArchive` and history table**

Populate thumbnail, file name, modified time, size and an action cell. Store the absolute path in `Qt::UserRole`, set touch-friendly row heights, update `recordsCountLabel`, and show one spanned empty-state row when no images exist.

- [ ] **Step 4: Implement non-dialog detail viewing and zoom**

Load the selected file into `m_detailPixmap`, initialize fit mode, fill truthful file fields, switch to `photoDetailPage`, and scale from the immutable source pixmap. Clamp manual zoom to 25%～400%.

- [ ] **Step 5: Implement export/delete/clear UI actions**

Use `QFileDialog::getSaveFileName()` for export. Use `QMessageBox::question()` before deleting one file or clearing all files. After successful mutations, refresh history/count/storage and move away from a deleted detail page.

- [ ] **Step 6: Preserve realtime behavior**

Keep `setPreviewFrame`, detection start/stop signals, `setDetectionUiState`, metrics updates and snapshot saving semantics. Remove obsolete battery/device setters only after confirming there are no callers, otherwise leave no-op compatibility methods without UI access.

- [ ] **Step 7: Run UI behavior and existing tests**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests --output-on-failure`

Expected: all existing lifecycle/launch/latest-value tests plus archive/structure/photo-flow tests PASS.

- [ ] **Step 8: Commit**

```bash
git add mainwindow.h mainwindow.cpp tests/mainwindow_photo_flow_test.cpp CMakeLists.txt
git commit -m "feat: implement history and photo detail workflow"
```

### Task 4: Approved industrial-white QSS

**Files:**
- Modify: `styles/white_theme.qss`

**Interfaces:**
- Consumes all object names and dynamic properties defined in Tasks 2–3.
- Produces a single resource-backed style loaded by `monitorapplication.cpp` from `:/styles/white_theme.qss`.

- [ ] **Step 1: Add a style-contract assertion to the structure test**

Load `:/styles/white_theme.qss` and assert the stylesheet contains selectors for `#detectButton`, `#snapshotButton`, `#recordsTable`, `#photoDetailPage`, `#exportPhotoButton`, `#deletePhotoButton`, plus global font size `18px` or larger.

- [ ] **Step 2: Run and verify failure**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests -R mainwindow_structure_test --output-on-failure`

Expected: FAIL because detail-page selectors are absent.

- [ ] **Step 3: Implement the approved QSS**

Set the canvas to `#F4F7FA`, cards to white with `#DEE5EE` borders and 10～12 px radii, title/body/metric sizes to the approved hierarchy, primary buttons to green/blue at 76～84 px, auxiliary buttons at least 56 px, and history rows/header to large touch-readable sizes. Use red only for alarm/delete and green only for success/running.

- [ ] **Step 4: Run the style contract and all tests**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add styles/white_theme.qss tests/mainwindow_structure_test.cpp
git commit -m "style: apply approved industrial white dashboard"
```

### Task 5: Native verification and RK3588 build handoff

**Files:**
- Modify only if verification exposes a scoped defect: `mainwindow.ui`, `mainwindow.cpp`, `mainwindow.h`, `styles/white_theme.qss`, build files.

**Interfaces:**
- Produces native test evidence and RK3588 build instructions; deployable ARM binaries are produced on the user's Linux cross-build host.

- [ ] **Step 1: Run clean native configure and full test build**

Run the native configure command from Task 1, then:

```powershell
C:\Qt\Tools\CMake_64\bin\cmake.exe --build build-tests
```

Expected: all Windows-hosted test targets build without warnings introduced by this change.

- [ ] **Step 2: Run all tests**

Run: `C:\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-tests --output-on-failure`

Expected: 100% PASS.

- [ ] **Step 3: Check generated UI and document the Linux ARM build**

Verify generated `ui_mainwindow.h` contains the three expected pages. Record the Linux build commands that produce `build-rk3588-new/bin/InsulatorMonitor`, `build-rk3588-new/bin/InsulatorMonitorSingle`, and the copied `single_model.ini`; do not claim ARM binaries were rebuilt on Windows.

- [ ] **Step 4: Review the final diff against the approved spec**

Run: `git diff HEAD~4 --check` and inspect that RKNN/GStreamer/controller files are unchanged.

- [ ] **Step 5: Commit verification-only corrections if needed**

If verification required a correction, stage the exact corrected UI/build/test files shown by `git status --short`, then commit with `git commit -m "fix: complete UI verification"`. If no correction was required, do not create an empty commit.
