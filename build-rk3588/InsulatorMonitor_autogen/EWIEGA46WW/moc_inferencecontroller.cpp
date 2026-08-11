/****************************************************************************
** Meta object code from reading C++ file 'inferencecontroller.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../inferencecontroller.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'inferencecontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_InferenceController_t {
    QByteArrayData data[35];
    char stringdata0[463];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_InferenceController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_InferenceController_t qt_meta_stringdata_InferenceController = {
    {
QT_MOC_LITERAL(0, 0, 19), // "InferenceController"
QT_MOC_LITERAL(1, 20, 10), // "frameReady"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 5), // "frame"
QT_MOC_LITERAL(4, 38, 12), // "stateChanged"
QT_MOC_LITERAL(5, 51, 26), // "InferenceController::State"
QT_MOC_LITERAL(6, 78, 5), // "state"
QT_MOC_LITERAL(7, 84, 7), // "message"
QT_MOC_LITERAL(8, 92, 14), // "metricsUpdated"
QT_MOC_LITERAL(9, 107, 11), // "pipelineFps"
QT_MOC_LITERAL(10, 119, 9), // "latencyMs"
QT_MOC_LITERAL(11, 129, 11), // "logReceived"
QT_MOC_LITERAL(12, 141, 14), // "startDetection"
QT_MOC_LITERAL(13, 156, 13), // "stopDetection"
QT_MOC_LITERAL(14, 170, 17), // "readProcessOutput"
QT_MOC_LITERAL(15, 188, 16), // "readProcessError"
QT_MOC_LITERAL(16, 205, 21), // "handleProcessFinished"
QT_MOC_LITERAL(17, 227, 8), // "exitCode"
QT_MOC_LITERAL(18, 236, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(19, 257, 10), // "exitStatus"
QT_MOC_LITERAL(20, 268, 18), // "handleProcessError"
QT_MOC_LITERAL(21, 287, 22), // "QProcess::ProcessError"
QT_MOC_LITERAL(22, 310, 5), // "error"
QT_MOC_LITERAL(23, 316, 16), // "handleFirstFrame"
QT_MOC_LITERAL(24, 333, 11), // "handleFrame"
QT_MOC_LITERAL(25, 345, 17), // "handleStreamError"
QT_MOC_LITERAL(26, 363, 20), // "handleStartupTimeout"
QT_MOC_LITERAL(27, 384, 17), // "handleStopTimeout"
QT_MOC_LITERAL(28, 402, 17), // "checkVideoTimeout"
QT_MOC_LITERAL(29, 420, 5), // "State"
QT_MOC_LITERAL(30, 426, 4), // "Idle"
QT_MOC_LITERAL(31, 431, 8), // "Starting"
QT_MOC_LITERAL(32, 440, 7), // "Running"
QT_MOC_LITERAL(33, 448, 8), // "Stopping"
QT_MOC_LITERAL(34, 457, 5) // "Error"

    },
    "InferenceController\0frameReady\0\0frame\0"
    "stateChanged\0InferenceController::State\0"
    "state\0message\0metricsUpdated\0pipelineFps\0"
    "latencyMs\0logReceived\0startDetection\0"
    "stopDetection\0readProcessOutput\0"
    "readProcessError\0handleProcessFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "handleProcessError\0QProcess::ProcessError\0"
    "error\0handleFirstFrame\0handleFrame\0"
    "handleStreamError\0handleStartupTimeout\0"
    "handleStopTimeout\0checkVideoTimeout\0"
    "State\0Idle\0Starting\0Running\0Stopping\0"
    "Error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_InferenceController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       1,  132, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   94,    2, 0x06 /* Public */,
       4,    2,   97,    2, 0x06 /* Public */,
       8,    2,  102,    2, 0x06 /* Public */,
      11,    1,  107,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,  110,    2, 0x0a /* Public */,
      13,    0,  111,    2, 0x0a /* Public */,
      14,    0,  112,    2, 0x08 /* Private */,
      15,    0,  113,    2, 0x08 /* Private */,
      16,    2,  114,    2, 0x08 /* Private */,
      20,    1,  119,    2, 0x08 /* Private */,
      23,    0,  122,    2, 0x08 /* Private */,
      24,    1,  123,    2, 0x08 /* Private */,
      25,    1,  126,    2, 0x08 /* Private */,
      26,    0,  129,    2, 0x08 /* Private */,
      27,    0,  130,    2, 0x08 /* Private */,
      28,    0,  131,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,
    QMetaType::Void, 0x80000000 | 5, QMetaType::QString,    6,    7,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    9,   10,
    QMetaType::Void, QMetaType::QString,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 18,   17,   19,
    QMetaType::Void, 0x80000000 | 21,   22,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QImage,    3,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // enums: name, alias, flags, count, data
      29,   29, 0x2,    5,  137,

 // enum data: key, value
      30, uint(InferenceController::State::Idle),
      31, uint(InferenceController::State::Starting),
      32, uint(InferenceController::State::Running),
      33, uint(InferenceController::State::Stopping),
      34, uint(InferenceController::State::Error),

       0        // eod
};

void InferenceController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<InferenceController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->frameReady((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 1: _t->stateChanged((*reinterpret_cast< InferenceController::State(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->metricsUpdated((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 3: _t->logReceived((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->startDetection(); break;
        case 5: _t->stopDetection(); break;
        case 6: _t->readProcessOutput(); break;
        case 7: _t->readProcessError(); break;
        case 8: _t->handleProcessFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 9: _t->handleProcessError((*reinterpret_cast< QProcess::ProcessError(*)>(_a[1]))); break;
        case 10: _t->handleFirstFrame(); break;
        case 11: _t->handleFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 12: _t->handleStreamError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->handleStartupTimeout(); break;
        case 14: _t->handleStopTimeout(); break;
        case 15: _t->checkVideoTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (InferenceController::*)(const QImage & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceController::frameReady)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (InferenceController::*)(InferenceController::State , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceController::stateChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (InferenceController::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceController::metricsUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (InferenceController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceController::logReceived)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject InferenceController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_InferenceController.data,
    qt_meta_data_InferenceController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *InferenceController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *InferenceController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_InferenceController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int InferenceController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void InferenceController::frameReady(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void InferenceController::stateChanged(InferenceController::State _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void InferenceController::metricsUpdated(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void InferenceController::logReceived(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
