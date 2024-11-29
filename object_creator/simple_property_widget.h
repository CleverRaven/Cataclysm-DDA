#ifndef CATA_OBJECT_CREATOR_SIMPLE_PROPERTY_WIDGET_H
#define CATA_OBJECT_CREATOR_SIMPLE_PROPERTY_WIDGET_H


#include "item_group.h"

#include "QtWidgets/qlistwidget.h"
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qplaintextedit.h>
#include "QtWidgets/qpushbutton.h"
#include <QtWidgets/qlayout.h>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include "QtWidgets/qlabel.h"
#include <QtWidgets/qcheckbox.h>


namespace creator
{



    enum class property_type : int {
        LINEEDIT,       // Just a label and a one-line text box
        MINMAX,         // Label, spinbox for minimum and a spinbox for maximum number
        NUMBER,         // Label and one spinbox holding a number
        BOOLEAN,        // Label and one checkbox
        NUM_TYPES       // always keep at the end, number of property types
    };

    //A widget for the most simple properties
    class simple_property_widget : public QFrame
    {
    public:
        /* The constructor function
        * @param parent: required by parent class but unused here. just provide the parent widget
        * @param propertyText: the name of the property in json. Used as a label and for writing json
        * @param prop_type: The type of property. See property_type enum for options
        * @param to_notify: A widget that will receive the change notification if something changes
        * you need to specify the property_changed event in the to_notify widget's code
        */
        explicit simple_property_widget( QWidget* parent, QString propertyText, 
                                    property_type prop_type, QObject* to_notify );
        void get_json( JsonOut &jo );
        void allow_hiding( bool allow );
        QString get_propertyName();
        /* Set the minimum and maximum limits for the MINMAX property
        * @param min_min: The minimum possible value for the minimum box
        * @param min_max: The maximum possible value for the minimum box
        * @param max_min: The minimum possible value for the maximum box
        * @param max_max: The maximum possible value for the maximum box
        */
        void set_minmax_limits( int min_min, int min_max, int max_min, int max_max );

    private:
        void change_notify_widget();
        void min_changed();
        void max_changed();
        QLabel* prop_label;
        QSpinBox* prop_number;
        QSpinBox* prop_min;
        QSpinBox* prop_max;
        QCheckBox* prop_bool;
        QLineEdit* prop_line;
        QObject* widget_to_notify;
        QPushButton* btnHide;
        property_type p_type;
        QString propertyName;
    };


    class property_changed : public QEvent
    {
    public:
        property_changed() : QEvent( registeredType() ) { }
        static QEvent::Type eventType;

    private:
        static QEvent::Type registeredType();
    };
}
#endif
