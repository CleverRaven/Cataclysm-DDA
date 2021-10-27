#include "dialogue_win.h"

#include <algorithm>
#include <string>
#include <vector>

#include "catacharset.h"
#include "input.h"
#include "output.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

// Height of the response section
const int RESPONSES_LINES = 15;

void dialogue_window::resize_dialogue( ui_adaptor &ui )
{
    const int win_beginy = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
    const int win_beginx = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
    const int maxy = win_beginy ? TERMY - 2 * win_beginy : FULL_SCREEN_HEIGHT;
    const int maxx = win_beginx ? TERMX - 2 * win_beginx : FULL_SCREEN_WIDTH;
    d_win = catacurses::newwin( maxy, maxx, point( win_beginx, win_beginy ) );
    ui.position_from_window( d_win );
    scroll_yoffset = 0;
}

void dialogue_window::print_header( const std::string &name )
{
    draw_border( d_win );

    mvwprintz( d_win, point( 2, 1 ), c_white, _( "Dialogue: %s" ), name );

    const int xmax = getmaxx( d_win );
    const int ymax = getmaxy( d_win );
    const int ybar = ymax - 1 - RESPONSES_LINES - 1;
    // Horizontal bar dividing history and responses
    mvwputch( d_win, point( 0, ybar ), BORDER_COLOR, LINE_XXXO );
    mvwhline( d_win, point( 1, ybar ), LINE_OXOX, xmax - 1 );
    mvwputch( d_win, point( xmax - 1, ybar ), BORDER_COLOR, LINE_XOXX );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( d_win, point( 2, ybar + 1 ), c_white, _( "Your response:" ) );
    npc_name = name;
}

void dialogue_window::clear_window_texts()
{
    werase( d_win );
    print_header( npc_name );
}

size_t dialogue_window::add_to_history( const std::string &text, const std::string &speaker_name,
                                        nc_color speaker_color )
{
    size_t lines_added = 0;
    lines_added += add_to_history( speaker_name, speaker_color );
    lines_added += add_to_history( text );
    return lines_added;
}

size_t dialogue_window::add_to_history( const std::string &text )
{
    return add_to_history( text, c_white );
}

size_t dialogue_window::add_to_history( const std::string &text, nc_color color )
{
    const int win_xmax = getmaxx( d_win );
    const int xoffset = 2;
    const int xmax = win_xmax - 2;
    std::vector<std::string> folded = foldstring( text, xmax - xoffset );
    for( const std::string &message : folded ) {
        history.emplace_back( color, message);
    }
    return folded.size();
}

// Empty line between lines of dialogue
void dialogue_window::add_history_separator()
{
    history.emplace_back( c_white, "" );
}

void dialogue_window::print_history( const size_t highlight_lines )
{
    const int xoffset = 2;
    const int ymin = 2; // Border + header
    int ycurrent = getmaxy( d_win ) - 1 - RESPONSES_LINES - 2;
    int curindex = history.size() - 1;
    // index of the first line that is highlighted
    const int newindex = history.size() - highlight_lines;
    while( curindex >= 0 && ycurrent >= ymin ) {
        // Colorized highlighted text; use light gray for all old messages
        const history_message &message = history[curindex];
        nc_color col = ( curindex >= newindex ) ? message.color : c_light_gray;
        mvwprintz( d_win, point( xoffset, ycurrent ), col, message.text );
        --ycurrent;
        --curindex;
    }
}

bool dialogue_window::print_responses( const std::vector<talk_data> &responses )
{
    // Responses go on the left column
    const int responses_xoffset = 2; // 1 pad + 1 indentation under "Your response"
    const int xmid = getmaxx( d_win ) / 2;
    const int yoffset = getmaxy( d_win ) - 1 - RESPONSES_LINES + 1;
    const int ymax = getmaxy( d_win ) - 2;

    // Even if we're scrolled, we have to iterate through the full list of responses, since scroll
    // amount is based on the number of lines *after* folding.
    int ycurrent = yoffset - scroll_yoffset;
    for( size_t i = 0; i < responses.size() && ycurrent <= ymax; i++ ) {
        //~ %s: hotkey description
        const std::string hotkey_text = string_format( pgettext( "talk option", "%s: " ),
                                        responses[i].hotkey_desc );
        const int hotkey_width = utf8_width( hotkey_text );
        const int fold_width = xmid - responses_xoffset - hotkey_width - 1;
        const std::vector<std::string> folded = foldstring( responses[i].text, fold_width );
        const nc_color &color = responses[i].color;
        for( size_t j = 0; j < folded.size(); j++, ycurrent++ ) {
            if( ycurrent < yoffset ) {
                // Off screen (above)
                continue;
            } else if( ycurrent > ymax ) {
                // Off screen (below)
                break;
            }
            if( j == 0 ) {
                // First line; display hotkey
                mvwprintz( d_win, point( responses_xoffset, ycurrent ), color, hotkey_text );
            }
            mvwprintz( d_win, point( responses_xoffset + hotkey_width, ycurrent ), color, folded[j] );
        }
    }
    bool more_responses_to_display = ycurrent > ymax;

    // Actions go on the right column; they're unaffected by scrolling.
    input_context ctxt( "DIALOGUE_CHOOSE_RESPONSE" );
    ycurrent = yoffset;
    const int actions_xoffset = xmid + 2;
    mvwprintz( d_win, point( actions_xoffset, ycurrent ), c_magenta, _( "%s: Look at" ),
               ctxt.get_desc( "LOOK_AT", 1 ) );
    ++ycurrent;
    mvwprintz( d_win, point( actions_xoffset, ycurrent ), c_magenta, _( "%s: Size up stats" ),
               ctxt.get_desc( "SIZE_UP_STATS", 1 ) );
    ++ycurrent;
    mvwprintz( d_win, point( actions_xoffset, ycurrent ), c_magenta, _( "%s: Yell" ),
               ctxt.get_desc( "YELL", 1 ) );
    ++ycurrent;
    mvwprintz( d_win, point( actions_xoffset, ycurrent ), c_magenta, _( "%s: Check opinion" ),
               ctxt.get_desc( "CHECK_OPINION", 1 ) );
    return more_responses_to_display;
}

void dialogue_window::refresh_response_display()
{
    scroll_yoffset = 0;
    can_scroll_down = false;
    can_scroll_up = false;
}

void dialogue_window::handle_scrolling( const std::string &action )
{
    // Scroll the responses section
    const int displayable_lines = RESPONSES_LINES - 2;
    if( action == "DOWN" || action == "PAGE_DOWN" ) {
        if( can_scroll_down ) {
            scroll_yoffset += displayable_lines;
        }
    } else if( action == "UP" || action == "PAGE_UP" ) {
        if( can_scroll_up ) {
            scroll_yoffset = std::max( 0, scroll_yoffset - displayable_lines );
        }
    }
}

void dialogue_window::display_responses( const int highlight_lines,
        const std::vector<talk_data> &responses )
{
    clear_window_texts();
    print_history( highlight_lines );
    can_scroll_down = print_responses( responses );
    can_scroll_up = scroll_yoffset > 0;

    // Mid - 1 pad - 2 text width
    const int xoffset = getmaxx( d_win ) / 2 - 1 - 2;
    const int yoffset = getmaxy( d_win ) - 1 - RESPONSES_LINES + 1;
    if( can_scroll_up ) {
        mvwprintz( d_win, point( xoffset, yoffset ), c_light_green, "^^" );
    }
    if( can_scroll_down ) {
        mvwprintz( d_win, point( xoffset, yoffset + RESPONSES_LINES - 2 ), c_light_green, "vv" );
    }
    wnoutrefresh( d_win );
}
