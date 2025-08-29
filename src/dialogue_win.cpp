#include "dialogue_win.h"

#include <array>
#include <string>
#include <vector>

#include "catacharset.h"
#include "debug.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "ui_manager.h"

// Height of the response section
static const int RESPONSES_LINES = 15;

multiline_list_entry talk_data::get_entry() const
{
    multiline_list_entry entry;
    entry.entry_text = colorize( text, color );
    entry.prefix = formatted_hotkey( hotkey_desc, color );
    return entry;
}

dialogue_window::dialogue_window()
{
    responses_list = std::make_unique<multiline_list>( resp_win );
    history_view = std::make_unique<scrolling_text_view>( history_win );
}

void dialogue_window::resize( ui_adaptor &ui )
{
    const int win_beginy = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
    const int win_beginx = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
    const int maxy = win_beginy ? TERMY - 2 * win_beginy : FULL_SCREEN_HEIGHT;
    const int maxx = win_beginx ? TERMX - 2 * win_beginx : FULL_SCREEN_WIDTH;
    d_win = catacurses::newwin( maxy, maxx, point( win_beginx, win_beginy ) );
    ui.position_from_window( d_win );
    history_win = catacurses::newwin( maxy - 1 - RESPONSES_LINES - 2 - 1, maxx - 1, point( win_beginx,
                                      win_beginy + 2 ) );
    resp_win = catacurses::newwin( RESPONSES_LINES - 1, maxx / 2, point( win_beginx,
                                   win_beginy + maxy - RESPONSES_LINES ) );

    // Reset size-dependant state
    update_history_view = true;
    responses_list->fold_entries();
}

void dialogue_window::draw( const std::string &npc_name )
{
    werase( d_win );

    print_header( npc_name );
    int ycurrent = getmaxy( d_win ) - 1 - RESPONSES_LINES + 1;
    const int xmid = getmaxx( d_win ) / 2;
    // Actions go on the right column; they're unaffected by scrolling.
    const int actions_xoffset = xmid + 2;
    nc_color cur_color = c_magenta;
    input_context ctxt( "DIALOGUE_CHOOSE_RESPONSE" );
    if( debug_mode ) {
        for( const std::string &line : responses_debug ) {
            ycurrent += fold_and_print( d_win, point( actions_xoffset, ycurrent - 1 ), xmid - 4, c_yellow,
                                        line );
        }
    } else {
        if( !is_computer && !is_not_conversation ) {
            std::string formatted_text = formatted_hotkey( ctxt.get_desc( "LOOK_AT", 1 ),
                                         cur_color ).append( _( "Look at" ) );
            print_colored_text( d_win, point( actions_xoffset, ycurrent ), cur_color, c_magenta,
                                formatted_text );
            ++ycurrent;
            formatted_text = formatted_hotkey( ctxt.get_desc( "SIZE_UP_STATS", 1 ),
                                               cur_color ).append( _( "Size up stats" ) );
            print_colored_text( d_win, point( actions_xoffset, ycurrent ), cur_color, c_magenta,
                                formatted_text );
            ++ycurrent;
            formatted_text = formatted_hotkey( ctxt.get_desc( "ASSESS_PERSONALITY", 1 ),
                                               cur_color ).append( _( "Assess personality" ) );
            print_colored_text( d_win, point( actions_xoffset, ycurrent ), cur_color, c_magenta,
                                formatted_text );
            ++ycurrent;
            formatted_text = formatted_hotkey( ctxt.get_desc( "YELL", 1 ), cur_color ).append( _( "Yell" ) );
            print_colored_text( d_win, point( actions_xoffset, ycurrent ), cur_color, c_magenta,
                                formatted_text );
            ++ycurrent;
            formatted_text = formatted_hotkey( ctxt.get_desc( "CHECK_OPINION", 1 ),
                                               cur_color ).append( _( "Check opinion" ) );
            print_colored_text( d_win, point( actions_xoffset, ycurrent ), cur_color, c_magenta,
                                formatted_text );
        }
    }

    wnoutrefresh( d_win );

    responses_list->print_entries();

    if( update_history_view ) {
        update_history_view = false;
        const int newindex = history.size() - num_lines_highlighted;
        std::string assembled;
        for( int i = 0; i < static_cast<int>( history.size() ); ++i ) {
            nc_color col = ( i >= newindex ) ? history[i].color : c_light_gray;
            assembled += colorize( history[i].text, col ).append( "\n" );
        }

        history_view->set_text( assembled, false );
    }
    history_view->draw( c_light_gray );
}

void dialogue_window::handle_scrolling( std::string &action, input_context &ctxt )
{
    if( responses_list->handle_navigation( action, ctxt ) ||
        history_view->handle_navigation( action, ctxt ) ) {
        // No further action required
    }
    sel_response = responses_list->get_entry_pos();
}

void dialogue_window::set_up_scrolling( input_context &ctxt ) const
{
    if( !is_computer && !is_not_conversation ) {
        ctxt.register_action( "LOOK_AT" );
        ctxt.register_action( "SIZE_UP_STATS" );
        ctxt.register_action( "ASSESS_PERSONALITY" );
        ctxt.register_action( "YELL" );
        ctxt.register_action( "CHECK_OPINION" );
    }
    history_view->set_up_navigation( ctxt, scrolling_key_scheme::angle_bracket_scroll );
    responses_list->set_up_navigation( ctxt );
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
    update_history_view = true;
}

void dialogue_window::add_history_separator()
{
    add_to_history( "", default_color() );
}

void dialogue_window::clear_history_highlights()
{
    num_lines_highlighted = 0;
}

nc_color dialogue_window::default_color() const
{
    return is_computer ? c_green : c_white;
}

void dialogue_window::print_header( const std::string &name ) const
{
    draw_border( d_win );
    if( is_computer ) {
        mvwprintz( d_win, point( 2, 1 ), default_color(), _( "Interaction: %s" ), name );
    } else if( !is_not_conversation ) {
        mvwprintz( d_win, point( 2, 1 ), default_color(), _( "Dialogue: %s" ), name );
    }
    const int xmax = getmaxx( d_win );
    int x_debug = 15 + utf8_width( name );
    const int ymax = getmaxy( d_win );
    const int ybar = ymax - 1 - RESPONSES_LINES - 1;
    const int flag_count = 5;
    std::array<bool, flag_count> debug_flags = { show_dynamic_line_conditionals, show_response_conditionals, show_dynamic_line_effects, show_response_effects, show_all_responses };
    std::array<std::string, flag_count> debug_show_toggle = { "DL_COND", "RESP_COND", "DL_EFF", "RESP_EFF", "ALL_RESP" };
    if( debug_mode ) {
        for( int i = 0; i < flag_count; i++ ) {
            mvwprintz( d_win, point( x_debug, 1 ), debug_flags[i] ? c_yellow : c_brown, debug_show_toggle[i] );
            x_debug += utf8_width( debug_show_toggle[i] ) + 1;
        }
        x_debug += 2;
        mvwprintz( d_win, point( x_debug, 1 ), default_color(), "talk_topic: " + debug_topic_name );
    }

    // Horizontal bar dividing history and responses
    wattron( d_win, BORDER_COLOR );
    mvwaddch( d_win, point( 0, ybar ), LINE_XXXO );
    mvwhline( d_win, point( 1, ybar ), LINE_OXOX, xmax - 1 );
    mvwaddch( d_win, point( xmax - 1, ybar ), LINE_XOXX );
    wattroff( d_win, BORDER_COLOR );
    if( is_computer ) {
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "Your input:" ) );
    } else if( is_not_conversation ) {
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "What do you do?" ) );
    } else {
        mvwprintz( d_win, point( 2, ybar + 1 ), default_color(), _( "Your response:" ) );
    }
}

void dialogue_window::set_responses( const std::vector<talk_data> &responses )
{
    responses_list->create_entries( responses );
}

void dialogue_window::set_responses_debug( const std::vector<std::string> &responses )
{
    responses_debug = responses;
}
