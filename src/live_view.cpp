#include "live_view.h"
#include "output.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "options.h"
#include "translations.h"
#include "catacharset.h"
#include "vehicle.h"

#include <map>
#include <string>

namespace
{

constexpr int START_LINE = 1;
constexpr int START_COLUMN = 1;

} //namespace

void live_view::init( int const start_x, int const start_y, int const w, int const h )
{
    width   = w;
    height  = h;

    w_live_view.reset( newwin( height, width, start_y, start_x ) );

    hide();
}

void live_view::draw()
{
    if( !enabled ) {
        return;
    }

    werase( *this );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    // Because of the way the status UI is done, the live view window must
    // be tall enough to clear the entire height of the viewport below the
    // status bar. This hack allows the border around the live view box to
    // be drawn only as big as it needs to be, while still leaving the
    // window tall enough. Won't work for ncurses in Linux, but that doesn't
    // currently support the mouse. If and when it does, there'll need to
    // be a different code path here that works for ncurses.
    w_live_view->height = height;
#endif

    // -1 for border. -1 because getmaxy() actually returns height, not y position.
    const int line_limit = height - 2;
    const visibility_variables &cache = g->m.get_visibility_variables_cache();
    int line_out = START_LINE;
    g->print_all_tile_info( mouse_position, *this, START_COLUMN, line_out,
                            line_limit, false, cache );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    w_live_view->height = std::min( height, std::max( line_out + 1, 11 ) );
#endif

    draw_border( *this );
    static const char *title_prefix = "< ";
    static const char *title = _( "Mouse View" );
    static const char *title_suffix = " >";
    static const std::string full_title = string_format( "%s%s%s", title_prefix, title, title_suffix );
    const int start_pos = center_text_pos( full_title.c_str(), 0, getmaxx( w_live_view.get() ) - 1 );

    mvwprintz( *this, 0, start_pos, c_white, title_prefix );
    wprintz( *this, c_green, title );
    wprintz( *this, c_white, title_suffix );

    wrefresh( *this );
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
