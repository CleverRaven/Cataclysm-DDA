#include "live_view.h"

#include <algorithm> // min & max
#include <string>
#include <memory>

#include "catacharset.h" // center_text_pos
#include "color.h"
#include "cursesport.h"
#include "game.h"
#include "map.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "cursesdef.h"

namespace
{

constexpr int START_LINE = 1;
constexpr int MIN_BOX_HEIGHT = 11;

} //namespace

void live_view::init()
{
    hide();
}

int live_view::draw( const catacurses::window &win, const int max_height )
{
    if( !enabled ) {
        return 0;
    }

    // -1 for border. -1 because getmaxy() actually returns height, not y position.
    const int line_limit = max_height - 2;
    const visibility_variables &cache = g->m.get_visibility_variables_cache();
    int line_out = START_LINE;
    g->pre_print_all_tile_info( mouse_position, win, line_out, line_limit, cache );

    const int live_view_box_height = std::min( max_height, std::max( line_out + 1, MIN_BOX_HEIGHT ) );

#if defined(TILES) || defined(_WIN32)
    // Because of the way the status UI is done, the live view window must
    // be tall enough to clear the entire height of the viewport below the
    // status bar. This hack allows the border around the live view box to
    // be drawn only as big as it needs to be, while still leaving the
    // window tall enough. Won't work for ncurses in Linux, but that doesn't
    // currently support the mouse. If and when it does, there will need to
    // be a different code path here that works for ncurses.
    const int original_height = win.get<cata_cursesport::WINDOW>()->height;
    win.get<cata_cursesport::WINDOW>()->height = live_view_box_height;
#endif

    draw_border( win );
    static const char *title_prefix = "< ";
    static const char *title = _( "Mouse View" );
    static const char *title_suffix = " >";
    static const std::string full_title = string_format( "%s%s%s", title_prefix, title, title_suffix );
    const int start_pos = center_text_pos( full_title, 0, getmaxx( win ) - 1 );
    mvwprintz( win, point( start_pos, 0 ), c_white, title_prefix );
    wprintz( win, c_green, title );
    wprintz( win, c_white, title_suffix );

#if defined(TILES) || defined(_WIN32)
    win.get<cata_cursesport::WINDOW>()->height = original_height;
#endif

    return live_view_box_height;
}

void live_view::hide()
{
    enabled = false;
}

void live_view::show( const tripoint &p )
{
    enabled = true;
    mouse_position = p;
}
