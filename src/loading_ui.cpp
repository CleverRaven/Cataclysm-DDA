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
        ui = nullptr;
        ui_background = nullptr;
    }
}

void loading_ui::init()
{
    if( menu != nullptr && ui == nullptr ) {
        ui_background = std::make_unique<background_pane>();

        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            menu->reposition( ui );
        } );
        menu->reposition( *ui );
        ui->on_redraw( [this]( ui_adaptor & ui ) {
            menu->show( ui );
        } );
    }
}

void loading_ui::proceed()
{
    init();

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
    init();

    if( menu != nullptr ) {
        ui_manager::redraw();
        refresh_display();
        inp_mngr.pump_events();
    }
}
