#include "loading_ui.h"
#include "output.h"
#include "ui.h"
#include "debug.h"

loading_ui::loading_ui( bool display )
{
    if( display ) {
        menu.reset( new uimenu );
    }
}

loading_ui::~loading_ui()
{
}

void loading_ui::queue_callback( const std::string &description, std::function<void()> callback )
{
    entries.emplace_back( named_callback{ description, callback } );
}

void loading_ui::set_menu_description( const std::string &desc )
{
    if( menu == nullptr ) {
        return;
    }

    // @todo Prepend some cool MOTD tip here
    menu->title = desc;
}

void loading_ui::process()
{
    // The first pass creates menu
    if( menu != nullptr ) {
        for( const named_callback &cur : entries ) {
            menu->addentry( cur.name );
        }
    }

    for( const named_callback &cur : entries ) {
        if( menu != nullptr ) {
            menu->show();
            refresh();
            refresh_display();
        }

        cur.fun();
        if( menu != nullptr ) {
            menu->scrollby( 1 );
        }
    }

    entries.clear();
    if( menu != nullptr ) {
        menu->reset();
    }
}