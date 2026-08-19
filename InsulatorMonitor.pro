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
    photoarchive.cpp \
    snapshotprotocol.cpp \
    quantificationservice.cpp

HEADERS += \
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
    quantificationservice.h

!isEmpty(INSULATOR_QUANTIFIER_INCLUDE_DIR):!isEmpty(INSULATOR_QUANTIFIER_LIBRARY) {
    DEFINES += INSULATOR_QUANTIFIER_AVAILABLE
    INCLUDEPATH += $$INSULATOR_QUANTIFIER_INCLUDE_DIR
    LIBS += $$INSULATOR_QUANTIFIER_LIBRARY
    isEmpty(INSULATOR_QUANTIFIER_HEADER) {
        DEFINES += INSULATOR_QUANTIFIER_HEADER=\"QtInsulatorQuantifier.h\"
    } else {
        DEFINES += INSULATOR_QUANTIFIER_HEADER=\"$$INSULATOR_QUANTIFIER_HEADER\"
    }
}

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
