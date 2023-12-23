#include "creator_main_window.h"
#include "enum_conversions.h"
#include "translations.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qsplashscreen.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qmessagebox.h>



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
    //Create a splash screen that tells the user we're loading
    //First we create a pixmap with the desired size
    QPixmap splash( QSize(640, 480) );
    splash.fill(Qt::gray);

    //Then we create the splash screen and show it
    QSplashScreen splashscreen( splash );
    splashscreen.show();
    splashscreen.showMessage( "Loading mainwindow...", Qt::AlignCenter );
    //let the thread sleep for a second to show the splashscreen
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    app.processEvents();

    //Add a menu bar with the file->exit option and the help->about option
    QMenuBar* menuBar = creator_main_window.menuBar();
    QMenu* fileMenu = menuBar->addMenu( "File" );
    QAction* exitAction = fileMenu->addAction( "Exit" );
    QObject::connect( exitAction, &QAction::triggered, &creator_main_window, &QMainWindow::close );
    QMenu* helpMenu = menuBar->addMenu( "Help" );
    QAction* aboutAction = helpMenu->addAction( "About" );
    //When the about option is clicked, show a message box information about the object creator
    QObject::connect( aboutAction, &QAction::triggered, &creator_main_window, [&creator_main_window]() {
        QMessageBox::about( &creator_main_window, "About", "This is the Cataclysm: DDA object creator.\n"
                            "It is used to create new spells, itemgroups, etc.\n"
                            "Find more info and contribute to https://github.com/CleverRaven/Cataclysm-DDA.\n");
    } );

    //Add an option to the help menu to show the about qt dialog
    QAction* aboutQtAction = helpMenu->addAction( "About Qt" );
    QObject::connect( aboutQtAction, &QAction::triggered, &app, &QApplication::aboutQt );



    //Create a tab widget and add it to the main window
    QTabWidget* tabWidget = new QTabWidget(&creator_main_window);
    creator_main_window.setCentralWidget(tabWidget);

    //Create the spell window and add it as a tab
    spell_editor = new spell_window();
    tabWidget->addTab(spell_editor, "Spell");

    //Create the item group window and add it as a tab
    item_group_editor = new item_group_window();
    tabWidget->addTab(item_group_editor, "Item group");

    //Create the mod selection window and add it as a tab
    mod_selection = new mod_selection_window();
    tabWidget->addTab(mod_selection, "Mod selection");

    creator_main_window.showMaximized();
    splashscreen.finish( &creator_main_window ); //Destroy the splashscreen
    
    return app.exec();
}
