#include "creator_main_window.h"

#include "spell_window.h"
#include "enum_conversions.h"
#include "translations.h"
#include "worldfactory.h"
#include "mod_manager.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtCore/QSettings>

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<creator::jsobj_type>( creator::jsobj_type data )
    {
        switch( data ) {
        case creator::jsobj_type::SPELL: return "Spell";
        case creator::jsobj_type::LAST: break;
        }
        debugmsg( "Invalid valid_target" );
        abort();
    }
    // *INDENT-ON*

} // namespace io

int creator::main_window::execute( QApplication &app )
{
    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size(default_text_box_width, default_text_box_height);

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    QMainWindow title_menu;
    spell_window spell_editor( &title_menu );

    //Does nothing on it's own but once settings.setvalue() is called it will create
    //an ini file in C:\Users\User\AppData\Roaming\CleverRaven or equivalent directory
    QSettings settings( QSettings::IniFormat, QSettings::UserScope,
        "CleverRaven", "Cataclysm - DDA" );


    // =========================================================================================
    // first column of boxes

    QLabel mods_label;
    mods_label.setParent( &title_menu );
    mods_label.setText( QString( "Select mods (restart required):" ) );
    mods_label.resize( default_text_box_size*2 );
    mods_label.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    mods_label.show();
    row++;

    //We always load 'dda' so we exclude it from the mods list
    QStringList all_mods;
    for( const mod_id& e : world_generator->get_mod_manager().all_mods() ) {
        if( !e->obsolete && e->ident.str() != "dda" ) {
            all_mods.append( e->ident.c_str() );
        }
    }

    dual_list_box mods_box;
    mods_box.initialize( all_mods );
    mods_box.resize( QSize( default_text_box_width * 8, default_text_box_height * 10 ) );
    mods_box.setParent( &title_menu );
    mods_box.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    mods_box.show();
    //The user's mod selection gets saved to a file
    QObject::connect( &mods_box, &dual_list_box::pressed, [&]() {
            settings.setValue( "mods/include", mods_box.get_included() );
        });

    //A previous selection of mods is loaded from disk and applied to the modlist widget
    if( settings.contains( "mods/include" ) ) {
        QStringList modlist = settings.value( "mods/include" ).value<QStringList>();
        mods_box.set_included( modlist );
    }

    row += 11;

    QPushButton spell_button( _( "Spell Creator" ), &title_menu );
    spell_button.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    QObject::connect( &spell_button, &QPushButton::released,
        [&]() {
            title_menu.hide();
            spell_editor.show();
        });



    title_menu.show();
    spell_button.show();

    row += 3;
    col += 6;
    max_row = std::max( max_row, row );
    max_col = std::max( max_col, col );
    title_menu.resize( QSize( ( max_col + 1) * default_text_box_width,
        ( max_row )*default_text_box_height ) );


    return app.exec();
}
