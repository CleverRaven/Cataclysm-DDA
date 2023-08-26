#include "creator_main_window.h"

#include "spell_window.h"
#include "item_group_window.h"
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
        case creator::jsobj_type::item_group: return "Item group";
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
    const QSize default_text_box_size( default_text_box_width, default_text_box_height );
    
    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    //Does nothing on it's own but once settings.setvalue() is called it will create
    //an ini file in C:\Users\User\AppData\Roaming\CleverRaven or equivalent directory
    QSettings settings( QSettings::IniFormat, QSettings::UserScope,
                        "CleverRaven", "Cataclysm - DDA" );

    // =========================================================================================
    // first column of boxes
    row = 0;

    QMainWindow title_menu;
    spell_window spell_editor( &title_menu );
    item_group_window item_group_editor( &title_menu );

    row += 11;

    QPushButton spell_button( _( "Spell Creator" ), &title_menu );
    spell_button.move( QPoint( col * default_text_box_width, row * default_text_box_height ) );
    QObject::connect( &spell_button, &QPushButton::released,
    [&]() {
        title_menu.hide();
        spell_editor.show();
    } );


    // =========================================================================================
    // second column of boxes
    col++;

    QPushButton item_group_button( _( "Item group Creator" ), &title_menu );
    item_group_button.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    item_group_button.resize( 150, 30 );

    QObject::connect( &item_group_button, &QPushButton::released,
    [&]() {
        title_menu.hide();
        item_group_editor.show();
    } );

    title_menu.show();
    spell_button.show();
    item_group_button.show();

    row += 3;
    col += 6;
    max_row = std::max( max_row, row );
    max_col = std::max( max_col, col );
    title_menu.resize( QSize( ( max_col + 1 ) * default_text_box_width,
        ( max_row )*default_text_box_height ) );


    return app.exec();
}
