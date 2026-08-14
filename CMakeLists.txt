cmake_minimum_required(VERSION 3.16)

project(InsulatorMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

option(BUILD_MONITOR_APPS "Build deployable monitor applications" ON)

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
            Qt5::Core Qt5::Gui Qt5::Widgets PkgConfig::GSTREAMER)
    endforeach()

    configure_file(single_model.ini
        "${CMAKE_BINARY_DIR}/bin/single_model.ini" COPYONLY)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(latest_value_slot_test
        tests/latest_value_slot_test.cpp
    )
    target_include_directories(latest_value_slot_test PRIVATE
        "${CMAKE_SOURCE_DIR}"
    )
    add_test(NAME latest_value_slot_test
             COMMAND latest_value_slot_test)

    add_executable(inferencelifecycle_test
        tests/inferencelifecycle_test.cpp)
    target_include_directories(inferencelifecycle_test PRIVATE
        "${CMAKE_SOURCE_DIR}")
    add_test(NAME inferencelifecycle_test COMMAND inferencelifecycle_test)

    add_executable(inferencelaunchspec_test
        tests/inferencelaunchspec_test.cpp
        inferencelaunchspec.cpp)
    target_include_directories(inferencelaunchspec_test PRIVATE
        "${CMAKE_SOURCE_DIR}")
    target_link_libraries(inferencelaunchspec_test PRIVATE Qt5::Core)
    add_test(NAME inferencelaunchspec_test COMMAND inferencelaunchspec_test)

    add_executable(photoarchive_test
        tests/photoarchive_test.cpp
        photoarchive.cpp
        photoarchive.h)
    target_include_directories(photoarchive_test PRIVATE
        "${CMAKE_SOURCE_DIR}")
    target_link_libraries(photoarchive_test PRIVATE Qt5::Core Qt5::Gui)
    add_test(NAME photoarchive_test COMMAND photoarchive_test)
endif()
