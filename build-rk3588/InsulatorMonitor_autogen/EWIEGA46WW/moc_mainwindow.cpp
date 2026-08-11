/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[24];
    char stringdata0[327];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 23), // "detectionStartRequested"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 22), // "detectionStopRequested"
QT_MOC_LITERAL(4, 59, 17), // "snapshotRequested"
QT_MOC_LITERAL(5, 77, 17), // "settingsRequested"
QT_MOC_LITERAL(6, 95, 15), // "setPreviewFrame"
QT_MOC_LITERAL(7, 111, 5), // "frame"
QT_MOC_LITERAL(8, 117, 15), // "setBatteryLevel"
QT_MOC_LITERAL(9, 133, 7), // "percent"
QT_MOC_LITERAL(10, 141, 8), // "charging"
QT_MOC_LITERAL(11, 150, 15), // "setDeviceStatus"
QT_MOC_LITERAL(12, 166, 11), // "cameraReady"
QT_MOC_LITERAL(13, 178, 10), // "modelReady"
QT_MOC_LITERAL(14, 189, 18), // "setDetectionResult"
QT_MOC_LITERAL(15, 208, 6), // "result"
QT_MOC_LITERAL(16, 215, 10), // "confidence"
QT_MOC_LITERAL(17, 226, 11), // "totalPixels"
QT_MOC_LITERAL(18, 238, 12), // "defectPixels"
QT_MOC_LITERAL(19, 251, 11), // "updateClock"
QT_MOC_LITERAL(20, 263, 15), // "toggleDetection"
QT_MOC_LITERAL(21, 279, 15), // "openRecordsPage"
QT_MOC_LITERAL(22, 295, 15), // "openMonitorPage"
QT_MOC_LITERAL(23, 311, 15) // "requestSnapshot"

    },
    "MainWindow\0detectionStartRequested\0\0"
    "detectionStopRequested\0snapshotRequested\0"
    "settingsRequested\0setPreviewFrame\0"
    "frame\0setBatteryLevel\0percent\0charging\0"
    "setDeviceStatus\0cameraReady\0modelReady\0"
    "setDetectionResult\0result\0confidence\0"
    "totalPixels\0defectPixels\0updateClock\0"
    "toggleDetection\0openRecordsPage\0"
    "openMonitorPage\0requestSnapshot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   84,    2, 0x06 /* Public */,
       3,    0,   85,    2, 0x06 /* Public */,
       4,    0,   86,    2, 0x06 /* Public */,
       5,    0,   87,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    1,   88,    2, 0x0a /* Public */,
       8,    2,   91,    2, 0x0a /* Public */,
       8,    1,   96,    2, 0x2a /* Public | MethodCloned */,
      11,    2,   99,    2, 0x0a /* Public */,
      14,    4,  104,    2, 0x0a /* Public */,
      19,    0,  113,    2, 0x08 /* Private */,
      20,    0,  114,    2, 0x08 /* Private */,
      21,    0,  115,    2, 0x08 /* Private */,
      22,    0,  116,    2, 0x08 /* Private */,
      23,    0,  117,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    9,   10,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,   12,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::Double, QMetaType::LongLong, QMetaType::LongLong,   15,   16,   17,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->detectionStartRequested(); break;
        case 1: _t->detectionStopRequested(); break;
        case 2: _t->snapshotRequested(); break;
        case 3: _t->settingsRequested(); break;
        case 4: _t->setPreviewFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 5: _t->setBatteryLevel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->setBatteryLevel((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->setDeviceStatus((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 8: _t->setDetectionResult((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< qint64(*)>(_a[3])),(*reinterpret_cast< qint64(*)>(_a[4]))); break;
        case 9: _t->updateClock(); break;
        case 10: _t->toggleDetection(); break;
        case 11: _t->openRecordsPage(); break;
        case 12: _t->openMonitorPage(); break;
        case 13: _t->requestSnapshot(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::detectionStartRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::detectionStopRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::snapshotRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::settingsRequested)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::detectionStartRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MainWindow::detectionStopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MainWindow::snapshotRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MainWindow::settingsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
