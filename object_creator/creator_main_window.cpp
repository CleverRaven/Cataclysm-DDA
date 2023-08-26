#include "creator_main_window.h"

#include "spell_window.h"
#include "item_group_window.h"
#include "mod_selection_window.h"
#include "enum_conversions.h"
#include "translations.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qstackedwidget.h>
#include <QtWidgets/qmenubar.h>


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


    //Create a menu bar with one menu called navigation
    //The navigation menu has three actions, one for each window
    QMenuBar menu_bar( &creator_main_window );
    QMenu* navigation_menu = menu_bar.addMenu( pgettext( "menu", "Navigation" ) );
    QAction* spell_action = navigation_menu->addAction( pgettext( "menu", "Spell" ) );
    QAction* item_group_action = navigation_menu->addAction( pgettext( "menu", "Item group" ) );
    QAction* mod_selection_action = navigation_menu->addAction( pgettext( "menu", "Mod selection" ) );

    // Connect the navigation menu items to slots that switch the current widget of the stacked widget
    QObject::connect( spell_action, &QAction::triggered, [&]() {
        stackedWidget.setCurrentWidget( &spell_editor );
    } );
    QObject::connect( item_group_action, &QAction::triggered, [&]() {
        stackedWidget.setCurrentWidget( &item_group_editor );
    } );
    QObject::connect( mod_selection_action, &QAction::triggered, [&]() {
        stackedWidget.setCurrentWidget( &mod_selection );
    } );


    // Set the menu bar as the main window's menu bar
    creator_main_window.setMenuBar( &menu_bar );
    
    //Make the main window fullscreen
    creator_main_window.showFullScreen();



    return app.exec();
}
