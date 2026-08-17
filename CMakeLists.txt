cmake_minimum_required(VERSION 3.16)

project(InsulatorMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

option(BUILD_MONITOR_APPS "Build deployable monitor applications" ON)
option(BUILD_DESKTOP_UI_PREVIEW "Build the Windows-friendly UI preview" OFF)

set(MONITOR_COMMON_SOURCES
    monitorapplication.cpp
    monitorapplication.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    photoarchive.cpp
    photoarchive.h

    inferencecontroller.cpp
    inferencecontroller.h
    inferencelaunchspec.cpp
    inferencelaunchspec.h
    inferenceprofile.h
    inferencelifecycle.h

    gstvideoreceiver.cpp
    gstvideoreceiver.h

    resources.qrc
)

if(BUILD_MONITOR_APPS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-video-1.0
    )

    add_executable(InsulatorMonitor
        main.cpp
        ${MONITOR_COMMON_SOURCES}
    )
    add_executable(InsulatorMonitorSingle
        main_single.cpp
        ${MONITOR_COMMON_SOURCES}
    )

    set_target_properties(InsulatorMonitor InsulatorMonitorSingle PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    foreach(monitor_target InsulatorMonitor InsulatorMonitorSingle)
        target_link_libraries(${monitor_target} PRIVATE
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Widgets
            PkgConfig::GSTREAMER)
    endforeach()

    configure_file(single_model.ini
        "${CMAKE_BINARY_DIR}/bin/single_model.ini" COPYONLY)
endif()

# Standalone test targets are intentionally not configured for this project.
if(BUILD_DESKTOP_UI_PREVIEW)
    add_executable(DesktopUiPreview
        desktop_preview_main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        resources.qrc
    )
    target_link_libraries(DesktopUiPreview PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets)
    set_target_properties(DesktopUiPreview PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
