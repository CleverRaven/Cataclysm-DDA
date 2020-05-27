#include "dialogue_win.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "input.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "ui_manager.h"

void dialogue_window::open_dialogue( bool text_only )
{
    if( text_only ) {
        this->text_only = true;
        return;
    }
}

void dialogue_window::resize_dialogue( ui_adaptor &ui )
{
    if( text_only ) {
        ui.position( point_zero, point_zero );
    } else {
        int win_beginy = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
        int win_beginx = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
        int maxy = win_beginy ? TERMY - 2 * win_beginy : FULL_SCREEN_HEIGHT;
        int maxx = win_beginx ? TERMX - 2 * win_beginx : FULL_SCREEN_WIDTH;
        d_win = catacurses::newwin( maxy, maxx, point( win_beginx, win_beginy ) );
        ui.position_from_window( d_win );
    }
    yoffset = 0;
}

void dialogue_window::print_header( const std::string &name )
{
    if( text_only ) {
        return;
    }
    draw_border( d_win );
    int win_midx = getmaxx( d_win ) / 2;
    int winy = getmaxy( d_win );
    mvwvline( d_win, point( win_midx + 1, 1 ), LINE_XOXO, winy - 1 );
    mvwputch( d_win, point( win_midx + 1, 0 ), BORDER_COLOR, LINE_OXXX );
    mvwputch( d_win, point( win_midx + 1, winy - 1 ), BORDER_COLOR, LINE_XXOX );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( d_win, point( 1, 1 ), c_white, _( "Dialogue: %s" ), name );
    mvwprintz( d_win, point( win_midx + 3, 1 ), c_white, _( "Your response:" ) );
    npc_name = name;
}

void dialogue_window::clear_window_texts()
{
    if( text_only ) {
        return;
    }
    werase( d_win );
    print_header( npc_name );
}

size_t dialogue_window::add_to_history( const std::string &text )
{
    const auto folded = foldstring( text, getmaxx( d_win ) / 2 );
    history.insert( history.end(), folded.begin(), folded.end() );
    return folded.size();
}

// Empty line between lines of dialogue
void dialogue_window::add_history_separator()
{
    history.push_back( "" );
}

void dialogue_window::print_history( const size_t hilight_lines )
{
    if( text_only ) {
        return;
    }
    int curline = getmaxy( d_win ) - 2;
    int curindex = history.size() - 1;
    // index of the first line that is highlighted
    int newindex = history.size() - hilight_lines;
    // Print at line 2 and below, line 1 contains the header, line 0 the border
    while( curindex >= 0 && curline >= 2 ) {
        // white for new text, light gray for old messages
        nc_color const col = ( curindex >= newindex ) ? c_white : c_light_gray;
        mvwprintz( d_win, point( 1, curline ), col, history[curindex] );
        curline--;
        curindex--;
    }
}

// Number of lines that can be used for the list of responses:
// -2 for border, -2 for options that are always there, -1 for header
static int RESPONSE_AREA_HEIGHT( int win_height )
{
    return win_height - 2 - 2 - 1;
}

bool dialogue_window::print_responses( const int yoffset, const std::vector<talk_data> &responses )
{
    if( text_only ) {
        return false;
    }
    // Responses go on the right side of the window, add 2 for spacing
    const size_t xoffset = getmaxx( d_win ) / 2 + 2;
    // First line we can print to, +2 for borders, +1 for the header.
    const int min_line = 2 + 1;
    // Bottom most line we can print to
    const int max_line = min_line + RESPONSE_AREA_HEIGHT( getmaxy( d_win ) ) - 1;

    // Remaining width of the responses area, -2 for the border, -2 for indentation, -2 for spacing
    const size_t fold_width = xoffset - 2 - 2 - 2;

    int curline = min_line - static_cast<int>( yoffset );
    for( size_t i = 0; i < responses.size() && curline <= max_line; i++ ) {
        const std::vector<std::string> folded = foldstring( responses[i].second, fold_width );
        const nc_color &color = responses[i].first;
        for( size_t j = 0; j < folded.size(); j++, curline++ ) {
            if( curline < min_line ) {
                continue;
            } else if( curline > max_line ) {
                break;
            }
            const int off = ( j != 0 ) ? +3 : 0;
            mvwprintz( d_win, point( xoffset + off, curline ), color, folded[j] );
        }
    }
    // Those are always available, their key bindings are fixed as well.
    mvwprintz( d_win, point( xoffset, curline + 1 ), c_magenta, _( "Shift+L: Look at" ) );
    mvwprintz( d_win, point( xoffset, curline + 2 ), c_magenta, _( "Shift+S: Size up stats" ) );
    mvwprintz( d_win, point( xoffset, curline + 3 ), c_magenta, _( "Shift+Y: Yell" ) );
    mvwprintz( d_win, point( xoffset, curline + 4 ), c_magenta, _( "Shift+O: Check opinion" ) );
    return curline > max_line; // whether there is more to print.
}

void dialogue_window::refresh_response_display()
{
    yoffset = 0;
    can_scroll_down = false;
    can_scroll_up = false;
}

void dialogue_window::handle_scrolling( const int ch )
{
    if( text_only ) {
        return;
    }
    // adjust scrolling from the last key pressed
    const int win_maxy = getmaxy( d_win );
    switch( ch ) {
        case KEY_DOWN:
        case KEY_NPAGE:
            if( can_scroll_down ) {
                yoffset += RESPONSE_AREA_HEIGHT( win_maxy );
            }
            break;
        case KEY_UP:
        case KEY_PPAGE:
            if( can_scroll_up ) {
                yoffset = std::max( 0, yoffset - RESPONSE_AREA_HEIGHT( win_maxy ) );
            }
            break;
        default:
            break;
    }
}

void dialogue_window::display_responses( const int hilight_lines,
        const std::vector<talk_data> &responses )
{
    if( text_only ) {
        return;
    }
    const int win_maxy = getmaxy( d_win );
    clear_window_texts();
    print_history( hilight_lines );
    can_scroll_down = print_responses( yoffset, responses );
    can_scroll_up = yoffset > 0;
    if( can_scroll_up ) {
        mvwprintz( d_win, point( getmaxx( d_win ) - 2 - 2, 2 ), c_green, "^^" );
    }
    if( can_scroll_down ) {
        mvwprintz( d_win, point( FULL_SCREEN_WIDTH - 2 - 2, win_maxy - 2 ), c_green, "vv" );
    }
    wrefresh( d_win );
}
