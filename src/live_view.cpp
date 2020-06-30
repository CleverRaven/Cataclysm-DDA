#include "live_view.h"

#include <algorithm> // min & max
#include <memory>

#include "color.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "translations.h"
#include "ui_manager.h"

namespace
{

constexpr int START_LINE = 1;
constexpr int MIN_BOX_HEIGHT = 3;

} //namespace

live_view::live_view() = default;
live_view::~live_view() = default;

void live_view::init()
{
    hide();
}

void live_view::hide()
{
    ui = nullptr;
}

void live_view::show( const tripoint &p )
{
    mouse_position = p;
    if( !ui ) {
        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            auto &mgr = panel_manager::get_manager();
            const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
            const int width = sidebar_right ? mgr.get_width_right() : mgr.get_width_left();

            const int max_height = TERMY / 2;
            const int line_limit = max_height - 2;
            const visibility_variables &cache = get_map().get_visibility_variables_cache();
            int line_out = START_LINE;
            // HACK: using dummy window to get the window height without refreshing.
            win = catacurses::newwin( 1, width, point_zero );
            g->pre_print_all_tile_info( mouse_position, win, line_out, line_limit, cache );
            const int live_view_box_height = std::min( max_height, std::max( line_out + 2, MIN_BOX_HEIGHT ) );

            win = catacurses::newwin( live_view_box_height, width,
                                      point( sidebar_right ? TERMX - width : 0, 0 ) );
            ui.position_from_window( win );
        } );
        ui->on_redraw( [this]( const ui_adaptor & ) {
            werase( win );
            const visibility_variables &cache = get_map().get_visibility_variables_cache();
            int line_out = START_LINE;
            g->pre_print_all_tile_info( mouse_position, win, line_out, getmaxy( win ) - 2, cache );
            draw_border( win );
            center_print( win, 0, c_white, _( "< <color_green>Mouse View</color> >" ) );
            wnoutrefresh( win );
        } );
    }
    // Always mark ui for resize as the required box height may have changed.
    ui->mark_resize();
}

bool live_view::is_enabled()
{
    return ui != nullptr;
}
