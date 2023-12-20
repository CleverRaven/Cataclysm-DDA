#include "simple_property_widget.h"
#include <QtWidgets/QtWidgets>

creator::simple_property_widget::simple_property_widget( QWidget* parent, QString propertyText, 
                                property_type prop_type, QObject* to_notify ) : QFrame( parent ) {
    widget_to_notify = to_notify;
    p_type = prop_type;
    propertyName = propertyText;

    QHBoxLayout* property_layout = new QHBoxLayout;
    property_layout->setMargin( 0 );
    property_layout->setContentsMargins( 0,0,0,0 );
    this->setLayout( property_layout ) ;

    QLabel* prop_label = new QLabel;
    prop_label->setText( propertyName + ":" );
    property_layout->addWidget( prop_label ) ;
    
    switch( p_type ) {
    case property_type::LINEEDIT:
        prop_line = new QLineEdit;
        prop_line->setMinimumSize( QSize( 120, 24 ) );
        prop_line->setMaximumSize( QSize( 150, 24 ) );
        QObject::connect( prop_line, &QLineEdit::textChanged, [&]() { change_notify_widget(); } );
        property_layout->addWidget( prop_line ) ;
        break;

    case property_type::MINMAX:
        prop_min = new QSpinBox;
        prop_min->setRange( 0, INT_MAX );
        prop_min->setValue( 1 );
        prop_min->setMinimumSize( QSize( 50, 24 ) );
        prop_min->setMaximumSize( QSize( 55, 24 ) );
        connect( prop_min, QOverload<int>::of( &QSpinBox::valueChanged ),
            [=]( ) { min_changed(); } );
        property_layout->addWidget( prop_min ) ;

        prop_max = new QSpinBox;
        prop_max->setRange( 0, INT_MAX );
        prop_max->setValue( 1 );
        prop_max->setMinimumSize( QSize( 50, 24 ) );
        prop_max->setMaximumSize( QSize( 55, 24 ) );
        connect( prop_max, QOverload<int>::of( &QSpinBox::valueChanged ),
            [=]( ) { max_changed(); } );
        property_layout->addWidget( prop_max ) ;
        break;

    case property_type::NUMBER:
        prop_number = new QSpinBox;
        prop_number->setRange( 0, INT_MAX );
        prop_number->setMinimumSize( QSize( 50, 24 ) );
        prop_number->setMaximumSize( QSize( 55, 24 ) );
        connect( prop_number, QOverload<int>::of( &QSpinBox::valueChanged ),
            [=]( ) { change_notify_widget(); } );
        property_layout->addWidget( prop_number ) ;
        break;
    
    case property_type::BOOLEAN:
        prop_bool = new QCheckBox;
        prop_bool->setMinimumSize( QSize( 50, 24 ) );
        prop_bool->setMaximumSize( QSize( 55, 24 ) );
        prop_bool->setChecked( true );
        connect( prop_bool, &QCheckBox::stateChanged, [&]() { change_notify_widget(); } );
        property_layout->addWidget( prop_bool ) ;
        break;

    case property_type::NUM_TYPES: break;
    }

    //This button allows the user to hide the property. The button is hidden by default
    //If it is made visible and the user presses it, another mechanism should
    //allow the user to make it visible again
    btnHide = new QPushButton;
    btnHide->setText( "X" );
    btnHide->hide();
    btnHide->setMaximumSize( QSize( 16, 16 ) );
    btnHide->setStyleSheet( "background-color:rgb(206,99,108)" );
    btnHide->setToolTip( QString( _( "Delete this property from the entry" ) ) );
    btnHide->setContentsMargins( 2,2,2,2 );
    btnHide->setToolTip( QString( ( "Remove property " + propertyName ) ) );
    QObject::connect( btnHide, &QPushButton::clicked, this, [&]() { this->hide();
                                                change_notify_widget(); } );
    property_layout->addWidget( btnHide ) ;
}


void creator::simple_property_widget::get_json( JsonOut &jo ) {
    int min, max, pr;
    switch( p_type ) {
        case property_type::NUMBER:
            pr = prop_number->value(); //If prob is 0, we omit prob entirely
            if( pr ) {
                //Only add if it's less then 100 since 100 is the default
                if( pr < 100 ) {
                    jo.member( propertyName.toStdString(), pr );
                }
            }
            break;

        case property_type::LINEEDIT:
            if( prop_line->text().size() > 0 ) {
                jo.member( propertyName.toStdString(), prop_line->text().toStdString() );
            }
            break;

        case property_type::MINMAX:
            min = prop_min->value();
            max = prop_max->value();
            if( min == max ){
                if( min > 0 ){
                    jo.member( propertyName.toStdString(), min );
                }
                //Do not add to JSON if both are 0
            } else {
                //Max is bigger then min, so we add both to JSON
                jo.member( propertyName.toStdString() + "-min", min );
                jo.member( propertyName.toStdString() + "-max", max );
            }
            break;

        case property_type::BOOLEAN:
            if( !prop_bool->isChecked() ) {
                jo.member( propertyName.toStdString(), false );
            }
            break;

        case property_type::NUM_TYPES: break;
    }
}


void creator::simple_property_widget::change_notify_widget() {
    QEvent* myEvent = new QEvent( property_changed::eventType );
    QCoreApplication::sendEvent( widget_to_notify, myEvent );
}

void creator::simple_property_widget::allow_hiding( bool allow ) {
    if( allow ){
        btnHide->show();
    } else {
        btnHide->hide();
    }
}

void creator::simple_property_widget::set_minmax_limits( int min_min, int min_max, int max_min, int max_max ) {
    prop_min->setMinimum( min_min );
    prop_min->setMaximum( min_max );
    prop_max->setMinimum( max_min );
    prop_max->setMaximum( max_max );
}

void creator::simple_property_widget::min_changed() {
    int max = prop_max->value();
    int min = prop_min->value();
    //If max has a value but is smaller then min, set it to min
    if( max < min ) {
        prop_max->setValue( min );
    }
    change_notify_widget();
}

void creator::simple_property_widget::max_changed() {
    int max = prop_max->value();
    int min = prop_min->value();
    //Max cannot be lower then min, so we set min to the value of max
    if( max < min ) {
        prop_min->setValue( max );
    }
    change_notify_widget();
}

QString creator::simple_property_widget::get_propertyName() {
    return propertyName;
}



QEvent::Type creator::property_changed::eventType = QEvent::User;
QEvent::Type creator::property_changed::registeredType()
{
    if( eventType == QEvent::None ) {
        int generatedType = QEvent::registerEventType();
        eventType = static_cast<QEvent::Type>( generatedType );
    }
    return eventType;
}
