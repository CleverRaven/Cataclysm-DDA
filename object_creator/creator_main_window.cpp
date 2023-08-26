#include "creator_main_window.h"

#include "spell_window.h"
#include "item_group_window.h"
#include "mod_selection_window.h"
#include "enum_conversions.h"
#include "translations.h"
#include "worldfactory.h"
#include "mod_manager.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtCore/QSettings>
#include <QtWidgets/qstackedwidget.h>

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
    //Create the main window
    QMainWindow creator_main_window;

    //Create a stacked widget and add it to the main window
    QStackedWidget stackedWidget( &creator_main_window );
    creator_main_window.setCentralWidget( &stackedWidget );

    //Add the spell_window and item_group_window to the stacked widget
    spell_window spell_editor( &stackedWidget );
    item_group_window item_group_editor( &stackedWidget );

    //Add the mod selection window to the stacked widget
    mod_selection_window mod_selection( &stackedWidget );

    //Set the current widget to the mod selection window
    stackedWidget.setCurrentWidget( &mod_selection );
    
    int row = 0;
    int col = 0;
    int max_row = 0;
    int max_col = 0;



    return app.exec();
}
