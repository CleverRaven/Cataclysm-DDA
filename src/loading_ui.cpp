#include "loading_ui.h"

#include <memory>

#include "color.h"
#include "output.h"
#include "ui.h"
#include "cursesdef.h"
#include "translations.h"

#if defined(TILES)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#       include <SDL2/SDL.h>
#   else
#       include <SDL.h>
#   endif
#endif // TILES

extern bool test_mode;

loading_ui::loading_ui( bool display )
{
    if( display && !test_mode ) {
        menu = std::make_unique<uilist>();
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
        if( menu->selected >= 0 && menu->selected < static_cast<int>( menu->entries.size() ) ) {
            // TODO: Color it red if it errored hard, yellow on warnings
            menu->entries[menu->selected].text_color = c_green;
        }

        if( menu->selected + 1 < static_cast<int>( menu->entries.size() ) ) {
            menu->scrollby( 1 );
        }
    }

    show();
}

void loading_ui::show()
{
    if( menu != nullptr ) {
        menu->show();
        catacurses::refresh();
        refresh_display();
#if defined(TILES)
        SDL_PumpEvents();
#endif // TILES
    }
}
