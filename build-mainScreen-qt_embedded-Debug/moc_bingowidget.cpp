/****************************************************************************
** Meta object code from reading C++ file 'bingowidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.11)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../mainScreen/bingowidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'bingowidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.11. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BingoWidget_t {
    QByteArrayData data[17];
    char stringdata0[277];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BingoWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BingoWidget_t qt_meta_stringdata_BingoWidget = {
    {
QT_MOC_LITERAL(0, 0, 11), // "BingoWidget"
QT_MOC_LITERAL(1, 12, 19), // "backToMainRequested"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 17), // "updateCameraFrame"
QT_MOC_LITERAL(4, 51, 22), // "handleCameraDisconnect"
QT_MOC_LITERAL(5, 74, 26), // "onCircleSliderValueChanged"
QT_MOC_LITERAL(6, 101, 5), // "value"
QT_MOC_LITERAL(7, 107, 23), // "onCircleCheckBoxToggled"
QT_MOC_LITERAL(8, 131, 7), // "checked"
QT_MOC_LITERAL(9, 139, 20), // "onRgbCheckBoxToggled"
QT_MOC_LITERAL(10, 160, 10), // "clearXMark"
QT_MOC_LITERAL(11, 171, 18), // "showSuccessMessage"
QT_MOC_LITERAL(12, 190, 19), // "hideSuccessAndReset"
QT_MOC_LITERAL(13, 210, 9), // "resetGame"
QT_MOC_LITERAL(14, 220, 13), // "restartCamera"
QT_MOC_LITERAL(15, 234, 19), // "onBackButtonClicked"
QT_MOC_LITERAL(16, 254, 22) // "onRestartButtonClicked"

    },
    "BingoWidget\0backToMainRequested\0\0"
    "updateCameraFrame\0handleCameraDisconnect\0"
    "onCircleSliderValueChanged\0value\0"
    "onCircleCheckBoxToggled\0checked\0"
    "onRgbCheckBoxToggled\0clearXMark\0"
    "showSuccessMessage\0hideSuccessAndReset\0"
    "resetGame\0restartCamera\0onBackButtonClicked\0"
    "onRestartButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BingoWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   80,    2, 0x08 /* Private */,
       4,    0,   81,    2, 0x08 /* Private */,
       5,    1,   82,    2, 0x08 /* Private */,
       7,    1,   85,    2, 0x08 /* Private */,
       9,    1,   88,    2, 0x08 /* Private */,
      10,    0,   91,    2, 0x08 /* Private */,
      11,    0,   92,    2, 0x08 /* Private */,
      12,    0,   93,    2, 0x08 /* Private */,
      13,    0,   94,    2, 0x08 /* Private */,
      14,    0,   95,    2, 0x08 /* Private */,
      15,    0,   96,    2, 0x08 /* Private */,
      16,    0,   97,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void BingoWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BingoWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->backToMainRequested(); break;
        case 1: _t->updateCameraFrame(); break;
        case 2: _t->handleCameraDisconnect(); break;
        case 3: _t->onCircleSliderValueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->onCircleCheckBoxToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->onRgbCheckBoxToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->clearXMark(); break;
        case 7: _t->showSuccessMessage(); break;
        case 8: _t->hideSuccessAndReset(); break;
        case 9: _t->resetGame(); break;
        case 10: _t->restartCamera(); break;
        case 11: _t->onBackButtonClicked(); break;
        case 12: _t->onRestartButtonClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BingoWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BingoWidget::backToMainRequested)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BingoWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_BingoWidget.data,
    qt_meta_data_BingoWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BingoWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BingoWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BingoWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BingoWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void BingoWidget::backToMainRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
