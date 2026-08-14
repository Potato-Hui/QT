QT += core gui widgets

CONFIG += c++14
CONFIG += link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0

TARGET = InsulatorMonitor
TEMPLATE = app

SOURCES += \
    main.cpp \
    monitorapplication.cpp \
    mainwindow.cpp \
    inferencecontroller.cpp \
    inferencelaunchspec.cpp \
    gstvideoreceiver.cpp \
    photoarchive.cpp

HEADERS += \
    mainwindow.h \
    monitorapplication.h \
    inferencecontroller.h \
    inferencelaunchspec.h \
    inferenceprofile.h \
    inferencelifecycle.h \
    gstvideoreceiver.h \
    latest_value_slot.hpp \
    photoarchive.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
