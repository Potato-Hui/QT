QT += core gui widgets

CONFIG += c++14
CONFIG += link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 opencv4

TARGET = InsulatorMonitor
TEMPLATE = app

SOURCES += \
    main.cpp \
    monitorapplication.cpp \
    mainwindow.cpp \
    inferencecontroller.cpp \
    inferencelaunchspec.cpp \
    gstvideoreceiver.cpp \
    photoarchive.cpp \
    snapshotprotocol.cpp \
    quantificationservice.cpp \
    $$PWD/../lianghua/cpp_qualification/src/InsulatorQuantifier.cpp \
    $$PWD/../lianghua/cpp_qualification/src/Gating.cpp \
    $$PWD/../lianghua/cpp_qualification/src/IdealArea.cpp \
    $$PWD/../lianghua/cpp_qualification/src/DamageEvaluator.cpp

HEADERS += \
    cameramode.h \
    mainwindow.h \
    monitorapplication.h \
    inferencecontroller.h \
    inferencelaunchspec.h \
    inferenceprofile.h \
    inferencelifecycle.h \
    gstvideoreceiver.h \
    latest_value_slot.hpp \
    photoarchive.h \
    snapshotprotocol.h \
    quantificationservice.h \
    $$PWD/../lianghua/cpp_qualification/include/InsulatorQuantifier.hpp \
    $$PWD/../lianghua/cpp_qualification/qt/QtInsulatorQuantifier.hpp

INCLUDEPATH += $$PWD/../lianghua/cpp_qualification/include \
    $$PWD/../lianghua/cpp_qualification/qt
DEFINES += INSULATOR_QUANTIFIER_AVAILABLE \
    INSULATOR_CALIBRATION_PATH=\"/data/config/pitch_area_model.json\"

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    thermal_camera.example.ini
