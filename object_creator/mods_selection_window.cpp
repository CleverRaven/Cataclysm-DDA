#include "mod_selection_window.h"

#include <QtCore/QSettings>

creator::mod_selection_window::mod_selection_window( QWidget *parent, Qt::WindowFlags flags )
    : QWidget ( parent, flags )
{
    QVBoxLayout* mod_layout = new QVBoxLayout();
    this->setLayout( mod_layout ) ;
 
 
    QLabel* mods_label = new QLabel( "Select mods (restart required):", this );
    mod_layout->addWidget( mods_label );

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

    dual_list_box* mods_box = new dual_list_box();
    mods_box->initialize( all_mods );
    mod_layout->addWidget( mods_box );

    //The user's mod selection gets saved to a file
    QObject::connect( mods_box, &dual_list_box::pressed, [&]() {
        settings.setValue( "mods/include", mods_box->get_included() );
    } );

    //A previous selection of mods is loaded from disk and applied to the modlist widget
    if( settings.contains( "mods/include" ) ) {
        QStringList modlist = settings.value( "mods/include" ).value<QStringList>();
        mods_box->set_included( modlist );
    }

}
