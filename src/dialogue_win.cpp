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
static const int RESPONSES_LINES = 15;

void dialogue_window::resize( ui_adaptor &ui )
{
    const int win_beginy = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
    const int win_beginx = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
    const int maxy = win_beginy ? TERMY - 2 * win_beginy : FULL_SCREEN_HEIGHT;
    const int maxx = win_beginx ? TERMX - 2 * win_beginx : FULL_SCREEN_WIDTH;
    d_win = catacurses::newwin( maxy, maxx, point( win_beginx, win_beginy ) );
    ui.position_from_window( d_win );

    // Reset size-dependant state
    scroll_yoffset = 0;
    rebuild_folded_history();
}

void dialogue_window::draw( const std::string &npc_name, const std::vector<talk_data> &responses )
{
    werase( d_win );

    print_header( npc_name );
    print_history();
    can_scroll_down = print_responses( responses );
    can_scroll_up = scroll_yoffset > 0;

    wnoutrefresh( d_win );
}

void dialogue_window::handle_scrolling( const std::string &action, int num_responses )
{
    // Scroll the responses section
    const int displayable_lines = RESPONSES_LINES - 2;
    const bool offscreen_lines = displayable_lines < num_responses;
    int next_offset = 0;
    for( int i = scroll_yoffset; i < num_responses; i++ ) {
        next_offset += folded_heights[i];
        if( next_offset >= displayable_lines || i == num_responses - 1 ) {
            next_offset = i - scroll_yoffset;
            break;
        }
    }
    int prev_offset = 0;
    for( int i = scroll_yoffset; i >= 0; i-- ) {
        prev_offset += folded_heights[i];
        if( prev_offset >= displayable_lines || i == 0 ) {
            prev_offset = i;
            break;
        }
    }
    if( action == "PAGE_DOWN" ) {
        if( can_scroll_down ) {
            scroll_yoffset += next_offset;
            sel_response = scroll_yoffset;
        }
    } else if( action == "PAGE_UP" ) {
        if( can_scroll_up ) {
            scroll_yoffset = prev_offset;
            sel_response = scroll_yoffset;
        }
    } else if( action == "UP" ) {
        sel_response--;
        if( sel_response < 0 ) {
            sel_response = num_responses - 1;
        }
    } else if( action == "DOWN" ) {
        sel_response++;
        if( sel_response >= num_responses ) {
            sel_response = 0;
        }
    }
    if( offscreen_lines && ( action == "UP" || action == "DOWN" ) ) {
        if( sel_response < scroll_yoffset ) {
            scroll_yoffset = sel_response;
        } else if( sel_response >= scroll_yoffset + next_offset ) {
            scroll_yoffset = sel_response - next_offset;
        }
    }
    if( scroll_yoffset < 0 ) {
        scroll_yoffset = 0;
    } else if( scroll_yoffset >= num_responses ) {
        scroll_yoffset = sel_response;
    }
}

void dialogue_window::refresh_response_display()
{
    scroll_yoffset = 0;
    can_scroll_down = false;
    can_scroll_up = false;
    sel_response = 0;
}

void dialogue_window::add_to_history( const std::string &text, const std::string &speaker_name,
                                      nc_color speaker_color )
{
    add_to_history( speaker_name, speaker_color );
    add_to_history( text );
}

void dialogue_window::add_to_history( const std::string &text )
{
    add_to_history( text, default_color() );
}

void dialogue_window::add_to_history( const std::string &text, nc_color color )
{
    history.emplace_back( color, text );
    ++num_lines_highlighted;

    add_to_folded_history( history.back(), true );
}

void dialogue_window::add_history_separator()
{
    add_to_history( "", default_color() );
}

void dialogue_window::clear_history_highlights()
{
    num_lines_highlighted = 0;
    num_folded_lines_highlighted = 0;
}

void dialogue_window::add_to_folded_history( const history_message &message, bool is_highlighted )
{
    const int win_xmax = getmaxx( d_win );
    const int xoffset = 2;
    const int xmax = win_xmax - 2;
    std::vector<std::string> folded = foldstring( message.text, xmax - xoffset );
    if( folded.empty() ) {
        // foldstring doesn't preserve empty lines. Add it back in.
        folded.push_back( message.text );
    }
    for( const std::string &line : folded ) {
        history_folded.emplace_back( message.color, line );
    }

    if( is_highlighted ) {
        num_folded_lines_highlighted += folded.size();
    }
}

void dialogue_window::rebuild_folded_history()
{
    history_folded.clear();
    num_folded_lines_highlighted = 0;

    history_folded.reserve( history.size() * 2 );
    for( auto message_iter = history.begin(); message_iter < history.end(); ++message_iter ) {
        add_to_folded_history( *message_iter, ( history.end() - message_iter ) <= num_lines_highlighted );
    }
}

nc_color dialogue_window::default_color() const
{
    return is_computer ? c_green : c_white;
}

void dialogue_window::print_header( const std::string &name )
{
    draw_border( d_win );
    if( is_computer ) {
        mvwprintz( d_win, point( 2, 1 ), default_color(), _( "Interaction: %s" ), name );
    } else if( !is_not_conversation ) {
        mvwprintz( d_win, point( 2, 1 ), default_color(), _( "Dialogue: %s" ), name );
    }
    const int xmax = getmaxx( d_win );
    const int ymax = getmaxy( d_win );
    const int ybar = ymax - 1 - RESPONSES_LINES - 1;
    // Horizontal bar dividing history and responses
    mvwputch( d_win, point( 0, ybar ), BORDER_COLOR, LINE_XXXO );
    mvwhline( d_win, point( 1, ybar ), LINE_OXOX, xmax - 1 );
    mvwputch( d_win, point( xmax - 1, ybar ), BORDER_COLOR, LINE_XOXX );
    if( is_computer ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "Your input:" ) );
    } else if( is_not_conversation ) {
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "What do you do?" ) );
    } else {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "Your response:" ) );
    }
}

void dialogue_window::print_history()
{
    const int xoffset = 2;
    const int ymin = 2; // Border + header
    int ycurrent = getmaxy( d_win ) - 1 - RESPONSES_LINES - 2;
    int curindex = history_folded.size() - 1;
    // index of the first line that is highlighted
    const int newindex = history_folded.size() - num_folded_lines_highlighted;
    while( curindex >= 0 && ycurrent >= ymin ) {
        // Colorized highlighted text; use light gray for all old messages
        const history_message &message = history_folded[curindex];
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
    int total_length = 0;

    // Even if we're scrolled, we have to iterate through the full list of responses, since scroll
    // amount is based on the number of lines *after* folding.
    folded_txt.clear();
    folded_heights.clear();
    for( const talk_data &response : responses ) {
        //~ %s: hotkey description
        const std::string hotkey_text = string_format( pgettext( "talk option", "%s: " ),
                                        response.hotkey_desc );
        const int hotkey_width = utf8_width( hotkey_text );
        const int fold_width = xmid - responses_xoffset - hotkey_width - 1;
        const std::vector<std::string> folded = foldstring( response.text, fold_width );
        folded_heights.emplace_back( static_cast<int>( folded.size() ) );
        folded_txt.emplace_back( std::make_tuple( hotkey_text, folded ) );
        total_length += static_cast<int>( folded.size() );
    }

    best_fit responses_to_print = find_best_fit_in_size( folded_heights, sel_response,
                                  RESPONSES_LINES );

    int ycurrent = yoffset;
    int viewport_start_pos = 0;
    for( size_t i = 0; i < folded_txt.size(); ++i ) {
        if( static_cast<int>( i ) < responses_to_print.start ) {
            viewport_start_pos += folded_heights[i];
        } else if( static_cast<int>( i ) < responses_to_print.start + responses_to_print.length ) {
            nc_color color = is_computer ? default_color() : responses[i].color;
            if( i == static_cast<size_t>( sel_response ) ) {
                color = hilite( color );
            }
            const std::string &hotkey_text = std::get<0>( folded_txt[i] );
            const std::vector<std::string> &folded = std::get<1>( folded_txt[i] );
            const int hotkey_width = utf8_width( hotkey_text );
            const int fold_width = xmid - responses_xoffset - hotkey_width - 1;
            for( size_t j = 0; j < folded.size(); ++j, ++ycurrent ) {
                if( j == 0 ) {
                    // First line; display hotkey
                    mvwprintz( d_win, point( responses_xoffset, ycurrent ), color, hotkey_text );
                }
                mvwprintz( d_win, point( responses_xoffset + hotkey_width, ycurrent ), color,
                           left_justify( folded[j], fold_width, true ) );
            }
        }
    }
    // Actions go on the right column; they're unaffected by scrolling.
    input_context ctxt( "DIALOGUE_CHOOSE_RESPONSE" );
    ycurrent = yoffset;
    if( !is_computer && !is_not_conversation ) {
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
    }

    if( responses_to_print.start > 0 ||
        responses_to_print.length < static_cast<int>( responses.size() ) ) {
        scrollbar()
        .offset_x( getmaxx( d_win ) / 2 - 1 )
        .offset_y( getmaxy( d_win ) - RESPONSES_LINES )
        .content_size( total_length )
        .viewport_pos( viewport_start_pos )
        .viewport_size( RESPONSES_LINES - 1 )
        .apply( d_win );
    }

    return responses_to_print.start + responses_to_print.length < static_cast<int>( folded_txt.size() );
}
