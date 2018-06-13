#include "loading_ui.h"
#include "output.h"
#include "ui.h"
#include "color.h"

#ifdef TILES
#include "SDL.h"
#endif // TILES

extern bool test_mode;

loading_ui::loading_ui( bool display )
{
    if( display && !test_mode ) {
        menu.reset( new uimenu );
        menu->settext( _( "Loading" ) );
    }
}

loading_ui::~loading_ui() = default;

void loading_ui::add_entry( const std::string &description )
{
    if( menu != nullptr ) {
        menu->addentry( menu->entries.size(), true, 0, description );
    }
}

void loading_ui::new_context( const std::string &desc )
{
    if( menu != nullptr ) {
        menu->reset();
        menu->settext( desc );
    }
}

void loading_ui::proceed()
{
    if( menu != nullptr && !menu->entries.empty() ) {
        if( menu->selected >= 0 && menu->selected < ( int )menu->entries.size() ) {
            // @todo: Color it red if it errored hard, yellow on warnings
            menu->entries[menu->selected].text_color = c_green;
        }

        menu->scrollby( 1 );
    }

    show();
}

void loading_ui::show()
{
    if( menu != nullptr ) {
        menu->show();
        catacurses::refresh();
        refresh_display();
#ifdef TILES
        SDL_PumpEvents();
#endif // TILES
    }
}
