#include "loading_ui.h"

#include <memory>
#include <vector>

#include "cached_options.h"
#include "input.h"
#include "color.h"
#include "output.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

#if defined(TILES)
#include "sdl_wrappers.h"
#endif // TILES

loading_ui::loading_ui( bool display )
{
    if( display && !test_mode ) {
        menu = std::make_unique<uilist>( _( "Loading" ) );
    }
}

loading_ui::~loading_ui() = default;

void loading_ui::add_entry( const std::string &description )
{
    if( menu != nullptr ) {
        menu->addentry( menu->entries.size(), true, std::nullopt, description );
    }
}

void loading_ui::new_context( const std::string &desc )
{
    if( menu != nullptr ) {
        menu->reset();
        menu->set_title( desc );
        ui_background = nullptr;
    }
}

void loading_ui::init()
{
}

void loading_ui::proceed()
{
    init();

    if( menu != nullptr && !menu->entries.empty() ) {
        if( menu->selected >= 0 && menu->selected < menu->entries.size() ) {
            // TODO: Color it red if it errored hard, yellow on warnings
            menu->entries[menu->selected].text_color = c_green;
        }

        if( menu->selected + 1 < menu->entries.size() ) {
            menu->scrollby( 1 );
        }
    }

    show();
}

void loading_ui::show()
{
    init();

    if( menu != nullptr ) {
        ui_manager::redraw();
        refresh_display();
        inp_mngr.pump_events();
    }
}
