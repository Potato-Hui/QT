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
    cameramode.h
    gpiolightcontroller.cpp
    gpiolightcontroller.h
    photoarchive.cpp
    photoarchive.h
    snapshotprotocol.cpp
    snapshotprotocol.h
    quantificationservice.cpp
    quantificationservice.h

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

set(INSULATOR_QUANTIFIER_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../lianghua/cpp_qualification"
    CACHE PATH "Insulator quantification source directory")
set(INSULATOR_CALIBRATION_PATH "/data/config/pitch_area_model.json"
    CACHE STRING "Deployed pitch calibration JSON path")

function(add_quantification_core opencv_target)
    if(TARGET insulator_quantification)
        return()
    endif()
    add_library(insulator_quantification STATIC
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/InsulatorQuantifier.cpp"
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/Gating.cpp"
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/IdealArea.cpp"
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/DamageEvaluator.cpp"
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/BrokenAssociation.cpp")
    target_include_directories(insulator_quantification PUBLIC
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/include"
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/qt")
    target_link_libraries(insulator_quantification PUBLIC ${opencv_target})
endfunction()

function(configure_quantifier target)
    if(TARGET insulator_quantification)
        target_link_libraries(${target} PRIVATE insulator_quantification)
        target_compile_definitions(${target} PRIVATE
            INSULATOR_QUANTIFIER_AVAILABLE
            INSULATOR_CALIBRATION_PATH=\"${INSULATOR_CALIBRATION_PATH}\")
    endif()
endfunction()

if(BUILD_MONITOR_APPS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-video-1.0
    )
    if(NOT EXISTS "${INSULATOR_QUANTIFIER_SOURCE_DIR}/src/InsulatorQuantifier.cpp")
        message(FATAL_ERROR "Quantification source directory not found: ${INSULATOR_QUANTIFIER_SOURCE_DIR}")
    endif()
    pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv4)
    add_quantification_core(PkgConfig::OPENCV)

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
        configure_quantifier(${monitor_target})
    endforeach()

    configure_file(single_model.ini
        "${CMAKE_BINARY_DIR}/bin/single_model.ini" COPYONLY)
    configure_file(
        "${INSULATOR_QUANTIFIER_SOURCE_DIR}/config/pitch_area_model.json"
        "${CMAKE_BINARY_DIR}/bin/pitch_area_model.json" COPYONLY)
endif()

# Standalone test targets are intentionally not configured for this project.
if(BUILD_DESKTOP_UI_PREVIEW)
    message(STATUS "DesktopUiPreview uses generated snapshot and quantification JSON samples")
    add_executable(DesktopUiPreview
        desktop_preview_main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        cameramode.h
        gpiolightcontroller.cpp
        gpiolightcontroller.h
        photoarchive.cpp
        photoarchive.h
        snapshotprotocol.cpp
        snapshotprotocol.h
        quantificationservice.cpp
        quantificationservice.h
        resources.qrc
    )
    target_link_libraries(DesktopUiPreview PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets)
    target_compile_definitions(DesktopUiPreview PRIVATE INSULATOR_DESKTOP_PREVIEW)
    set_target_properties(DesktopUiPreview PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
