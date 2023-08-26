#include "mod_selection_window.h"

#include <QtCore/QSettings>

creator::mod_selection_window::mod_selection_window( QWidget *parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
{
    
    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );
    
    int row = 0;
    int col = 0;


    QHBoxLayout* mod_layout = new QHBoxLayout;
    mod_layout->setMargin( 0 );
    mod_layout->setContentsMargins( 0,0,0,0 );
    this->setLayout( mod_layout ) ;


    // =========================================================================================
    // first column of boxes
    row = 0;

    QLabel mods_label;
    mods_label.setParent( this );
    mods_label.setText( QString( "Select mods (restart required):" ) );
    mods_label.resize( default_text_box_size * 2 );
    mods_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    mods_label.show();
    row++;

    //We always load 'dda' so we exclude it from the mods list
    QStringList all_mods;
    for( const mod_id &e : world_generator->get_mod_manager().all_mods() ) {
        if( !e->obsolete && e->ident.str() != "dda" ) {
            all_mods.append( e->ident.c_str() );
        }
    }

    //Does nothing on it's own but once settings.setvalue() is called it will create
    //an ini file in C:\Users\User\AppData\Roaming\CleverRaven or equivalent directory
    QSettings settings( QSettings::IniFormat, QSettings::UserScope,
                        "CleverRaven", "Cataclysm - DDA" );

    dual_list_box mods_box;
    mods_box.initialize( all_mods );
    mods_box.resize( QSize( default_text_box_width * 8, default_text_box_height * 10 ) );
    mods_box.setParent( this );
    mods_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    mods_box.show();
    //The user's mod selection gets saved to a file
    QObject::connect( &mods_box, &dual_list_box::pressed, [&]() {
        settings.setValue( "mods/include", mods_box.get_included() );
    } );

    //A previous selection of mods is loaded from disk and applied to the modlist widget
    if( settings.contains( "mods/include" ) ) {
        QStringList modlist = settings.value( "mods/include" ).value<QStringList>();
        mods_box.set_included( modlist );
    }

    // Add the widgets to the layout
    mod_layout->addWidget( &mods_label );
    mod_layout->addWidget( &mods_box );
}
