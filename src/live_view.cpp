#include "live_view.h"
#include "output.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "options.h"
#include "translations.h"
#include "vehicle.h"

#include <map>
#include <string>

const efftype_id effect_blind( "blind" );

namespace
{
constexpr int START_LINE = 1;
constexpr int START_COLUMN = 1;

void print_items( WINDOW *const w, const map_stack &items, int &line )
{
    std::map<std::string, int> item_names;
    for( auto &item : items ) {
        ++item_names[item.tname()];
    }

    int const last_line = getmaxy( w ) - START_LINE - 1;
    int const max_w = getmaxx( w ) - START_COLUMN - 1; // border

    for( auto const &it : item_names ) {
        if( line == last_line ) {
            mvwprintz( w, line++, START_COLUMN, c_yellow, _( "More items here..." ) );
            break;
        }

        if( it.second > 1 ) {
            //~ item name [quantity]
            trim_and_print( w, line++, START_COLUMN, max_w, c_white, _( "%s [%d]" ),
                            it.first.c_str(), it.second );
        } else {
            trim_and_print( w, line++, START_COLUMN, max_w, c_white, "%s", it.first.c_str() );
        }
    }
}
} //namespace

bool live_view::is_compact() const
{
    return compact_view;
}

void live_view::set_compact( bool const value )
{
    compact_view = value;
}

void live_view::init( int const start_x, int const start_y, int const w, int const h )
{
    enabled = true;
    width   = w;
    height  = h;

    w_live_view.reset( newwin( height, width, start_y, start_x ) );

    hide();
}

void live_view::show( const int x, const int y )
{
    if( !enabled || !w_live_view ) {
        return;
    }

    bool did_hide = hide( false ); // Clear window if it's visible

    if( !g->u.sees( x, y ) ) {
        if( did_hide ) {
            wrefresh( *this );
        }
        return;
    }

    map &m = g->m;
    mvwprintz( *this, 0, START_COLUMN, c_white, "< " );
    wprintz( *this, c_green, _( "Mouse View" ) );
    wprintz( *this, c_white, " >" );
    int line = START_LINE;

    // TODO: Z
    tripoint p( x, y, g->get_levz() );

    g->print_all_tile_info( p, *this, START_COLUMN, line, true );

    if( m.can_put_items( p ) && m.sees_some_items( p, g->u ) ) {
        if( g->u.has_effect( effect_blind ) || g->u.worn_with_flag( "BLIND" ) ) {
            mvwprintz( *this, line++, START_COLUMN, c_yellow,
                       _( "There's something here, but you can't see what it is." ) );
        } else {
            print_items( *this, m.i_at( p ), line );
        }
    }

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    // Because of the way the status UI is done, the live view window must
    // be tall enough to clear the entire height of the viewport below the
    // status bar. This hack allows the border around the live view box to
    // be drawn only as big as it needs to be, while still leaving the
    // window tall enough. Won't work for ncurses in Linux, but that doesn't
    // currently support the mouse. If and when it does, there'll need to
    // be a different code path here that works for ncurses.
    int full_height = w_live_view->height;
    if( line < w_live_view->height - 1 ) {
        w_live_view->height = ( line > 11 ) ? line : 11;
    }
    last_height = w_live_view->height;
#endif

    draw_border( *this );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    w_live_view->height = full_height;
#endif

    inuse = true;
    wrefresh( *this );
}

bool live_view::hide( bool refresh /*= true*/, bool force /*= false*/ )
{
    if( !enabled || ( !inuse && !force ) ) {
        return false;
    }

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    int full_height = w_live_view->height;
    if( use_narrow_sidebar() && last_height > 0 ) {
        // When using the narrow sidebar mode, the lower part of the screen
        // is used for the message queue. Best not to obscure too much of it.
        w_live_view->height = last_height;
    }
#endif

    werase( *this );

#if (defined TILES || defined _WIN32 || defined WINDOWS)
    w_live_view->height = full_height;
#endif

    inuse = false;
    last_height = -1;
    if( refresh ) {
        wrefresh( *this );
    }

    return true;
}
