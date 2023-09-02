#include "creator_main_window.h"

#include "spell_window.h"
#include "item_group_window.h"
#include "enum_conversions.h"
#include "translations.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qsplashscreen.h>
#include <QtGui/qpainter.h>


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

    //Create a splash screen that tells the user we're loading
    //First we create a pixmap with the desired size
    QPixmap splash( QSize(640, 480) );
    splash.fill(Qt::gray);
    //Then we use a painter to draw text on it
    QPainter painter( &splash );
    painter.setPen( Qt::black );
    painter.setFont( QFont( "Arial", 20 ) );
    painter.drawText( QRect( 0, 0, 640, 480 ), Qt::AlignCenter, "Loading main window..." );
    painter.end();

    //Then we create the splash screen and show it
    QSplashScreen splashscreen( splash );
    splashscreen.show();

    app.processEvents();
    //Create the main window
    QMainWindow creator_main_window;

    app.processEvents();
    //destroy the splashscreen when creator_main_window is shown
    splashscreen.finish( &creator_main_window );


    //Create a tab widget and add it to the main window
    QTabWidget* tabWidget = new QTabWidget(&creator_main_window);
    creator_main_window.setCentralWidget(tabWidget);

    //Create the spell window and add it as a tab
    spell_window spell_editor;
    tabWidget->addTab(&spell_editor, "Spell");

    //Create the item group window and add it as a tab
    item_group_window item_group_editor;
    tabWidget->addTab(&item_group_editor, "Item group");

    //Create the mod selection window and add it as a tab
    mod_selection = new mod_selection_window();
    tabWidget->addTab(mod_selection, "Mod selection");
    
    //Make the main window maximized
    creator_main_window.showMaximized();


    return app.exec();
}
