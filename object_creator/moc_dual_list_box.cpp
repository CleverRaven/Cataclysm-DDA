/****************************************************************************
** Meta object code from reading C++ file 'dual_list_box.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../object_creator/dual_list_box.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dual_list_box.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_creator__dual_list_box_t {
    QByteArrayData data[3];
    char stringdata0[37];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
            qptrdiff(offsetof(qt_meta_stringdata_creator__dual_list_box_t, stringdata0) + ofs \
                     - idx * sizeof(QByteArrayData)) \
            )
static const qt_meta_stringdata_creator__dual_list_box_t
qt_meta_stringdata_creator__dual_list_box = {
    {
        QT_MOC_LITERAL( 0, 0, 28 ), // "creator::dual_list_box"
        QT_MOC_LITERAL( 1, 29, 8 ), // "click"
        QT_MOC_LITERAL( 2, 38, 0 ) // ""

    },
    "creator::dual_list_box\0click\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_creator__dual_list_box[] = {

    // content:
    8,       // revision
    0,       // classname
    0,    0, // classinfo
    1,   14, // methods
    0,    0, // properties
    0,    0, // enums/sets
    0,    0, // constructors
    0,       // flags
    1,       // signalCount

    // signals: name, argc, parameters, tag, flags
    1,    0,   19,    2, 0x06 /* Public */,

    // signals: parameters
    QMetaType::Void,

    0        // eod
};

void creator::dual_list_box::qt_static_metacall( QObject *_o, QMetaObject::Call _c, int _id,
        void **_a )
{
    if( _c == QMetaObject::InvokeMetaMethod ) {
        auto *_t = static_cast<dual_list_box *>( _o );
        Q_UNUSED( _t )
        switch( _id ) {
            case 0:
                _t->pressed();
                break;
            default:
                ;
        }
    } else if( _c == QMetaObject::IndexOfMethod ) {
        int *result = reinterpret_cast<int *>( _a[0] );
        {
            using _t = void ( dual_list_box::* )();
            if( *reinterpret_cast<_t *>( _a[1] ) == static_cast<_t>( &dual_list_box::pressed ) ) {
                *result = 0;
                return;
            }
        }
    }
    Q_UNUSED( _a );
}

QT_INIT_METAOBJECT const QMetaObject creator::dual_list_box::staticMetaObject = { {
        QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
        qt_meta_stringdata_creator__dual_list_box.data,
        qt_meta_data_creator__dual_list_box,
        qt_static_metacall,
        nullptr,
        nullptr
    }
};


const QMetaObject *creator::dual_list_box::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *creator::dual_list_box::qt_metacast( const char *_clname )
{
    if( !_clname ) {
        return nullptr;
    }
    if( !strcmp( _clname, qt_meta_stringdata_creator__dual_list_box.stringdata0 ) ) {
        return static_cast<void *>( this );
    }
    return QWidget::qt_metacast( _clname );
}

int creator::dual_list_box::qt_metacall( QMetaObject::Call _c, int _id, void **_a )
{
    _id = QWidget::qt_metacall( _c, _id, _a );
    if( _id < 0 ) {
        return _id;
    }
    if( _c == QMetaObject::InvokeMetaMethod ) {
        if( _id < 1 ) {
            qt_static_metacall( this, _c, _id, _a );
        }
        _id -= 1;
    } else if( _c == QMetaObject::RegisterMethodArgumentMetaType ) {
        if( _id < 1 ) {
            *reinterpret_cast<int *>( _a[0] ) = -1;
        }
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void creator::dual_list_box::pressed()
{
    QMetaObject::activate( this, &staticMetaObject, 0, nullptr );
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
