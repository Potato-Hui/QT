QT += core gui widgets
CONFIG += c++14 link_pkgconfig
CONFIG -= app_bundle
PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0
TARGET = InsulatorMonitorSingle
TEMPLATE = app
SOURCES += main_single.cpp monitorapplication.cpp mainwindow.cpp \
    inferencecontroller.cpp inferencelaunchspec.cpp gstvideoreceiver.cpp
HEADERS += monitorapplication.h mainwindow.h inferencecontroller.h \
    inferencelaunchspec.h inferenceprofile.h inferencelifecycle.h \
    gstvideoreceiver.h latest_value_slot.hpp
FORMS += mainwindow.ui
RESOURCES += resources.qrc
