#include "live_view.h"

#include "catacharset.h" // center_text_pos
#include "color.h"
#include "game.h"
#include "map.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "cursesport.h"
#endif

#include <algorithm> // min & max
#include <string>

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
    int curr_line = START_LINE;

    nc_color clr = c_white;
    auto text = g->get_full_look_around_text( mouse_position );
    print_colored_text(win, curr_line, 1, clr, clr, text, line_limit);
    g->draw_terrain_indicators( mouse_position, !is_draw_tiles_mode() );

    if( text.size() > line_limit ) {
        clr = c_yellow;
        std::string message = "There are more things here...";
        print_colored_text( win, curr_line, 1, clr, clr, message );
        curr_line++;
    }

    const int live_view_box_height = std::min( max_height, std::max( curr_line + 1, MIN_BOX_HEIGHT ) );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
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

    clr = c_white;
    static const std::string pre = "< ", mid = _( "Mouse View" ), post = " >";
    static const std::string raw_title = pre + mid + post;
    static auto title = colorize( pre, clr ) + colorize( mid, c_green ) + colorize( post, clr );
    int start_pos = center_text_pos( raw_title, 0, getmaxx( win ) - 1 );

    print_colored_text( win, 0, start_pos, clr, clr, title );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
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
