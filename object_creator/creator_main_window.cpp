#include "creator_main_window.h"

#include "spell_window.h"
#include "item_group_window.h"
#include "enum_conversions.h"
#include "translations.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>

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
    QMainWindow title_menu;


    const int default_text_box_height = 20;
    const int default_text_box_width = 100;
    const QSize default_text_box_size(default_text_box_width, default_text_box_height);

    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;

    // =========================================================================================
    // second column of boxes
    row = 0;
    col++;

    spell_window spell_editor( &title_menu );
    item_group_window item_group_editor( &title_menu );

    QPushButton spell_button( _( "Spell Creator" ), &title_menu );
    spell_button.resize( default_text_box_size );
    QPushButton item_group_button( _( "Item group Creator" ), &title_menu );
    item_group_button.move( QPoint( col * default_text_box_width, row++ * default_text_box_height ) );
    item_group_button.resize( default_text_box_size );

    QObject::connect( &spell_button, &QPushButton::released,
    [&]() {
        title_menu.hide();
        spell_editor.show();
    } );

    QObject::connect( &item_group_button, &QPushButton::released,
    [&]() {
        title_menu.hide();
        item_group_editor.show();
    } );

    title_menu.show();
    spell_button.show();
    item_group_button.show();

    return app.exec();
}
