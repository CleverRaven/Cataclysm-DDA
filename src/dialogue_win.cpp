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

void dialogue_window::resize_dialogue( ui_adaptor &ui )
{
    int win_beginy = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
    int win_beginx = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
    int maxy = win_beginy ? TERMY - 2 * win_beginy : FULL_SCREEN_HEIGHT;
    int maxx = win_beginx ? TERMX - 2 * win_beginx : FULL_SCREEN_WIDTH;
    d_win = catacurses::newwin( maxy, maxx, point( win_beginx, win_beginy ) );
    ui.position_from_window( d_win );
    curr_page = 0;
    draw_cache.clear();
    for( size_t idx = 0; idx < history.size(); idx++ ) {
        cache_msg( history[idx], idx );
    }
}

void dialogue_window::print_header( const std::string &name )
{
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
    werase( d_win );
    print_header( npc_name );
}

void dialogue_window::add_to_history( const std::string &msg )
{
    size_t idx = history.size();
    history.push_back( msg );
    cache_msg( msg, idx );
}

void dialogue_window::print_history()
{
    if( history.empty() ) {
        return;
    }
    int curline = getmaxy( d_win ) - 2;
    int curindex = draw_cache.size() - 1;
    // Highligh last message
    size_t msg_to_highlight = history.size() - 1;
    // Print at line 2 and below, line 1 contains the header, line 0 the border
    while( curindex >= 0 && curline >= 2 ) {
        const std::pair<std::string, size_t> &msg = draw_cache[curindex];
        const nc_color col = ( msg.second == msg_to_highlight ) ? c_white : c_light_gray;
        mvwprintz( d_win, point( 1, curline ), col, draw_cache[curindex].first );
        curline--;
        curindex--;
    }
}

struct page_entry {
    nc_color col;
    std::vector<std::string> lines;
};

struct page {
    std::vector<page_entry> entries;
};

static std::vector<page> split_to_pages( const std::vector<talk_data> &responses, int page_w,
        int page_h )
{
    page this_page;
    std::vector<page> ret;
    int fold_width = page_w - 3;
    int this_h = 0;
    for( const talk_data &resp : responses ) {
        // Assemble single entry for printing
        const std::vector<std::string> folded = foldstring( resp.text, fold_width );
        page_entry this_entry;
        this_entry.col = resp.col;
        this_entry.lines.push_back( string_format( "%c: %s", resp.letter, folded[0] ) );
        for( size_t i = 1; i < folded.size(); i++ ) {
            this_entry.lines.push_back( string_format( "   %s", folded[i] ) );
        }

        // Add entry to current / new page
        if( this_h + static_cast<int>( folded.size() ) > page_h ) {
            ret.push_back( this_page );
            this_page = page();
            this_h = 0;
        }
        this_h += this_entry.lines.size();
        this_page.entries.push_back( this_entry );
    }
    if( !this_page.entries.empty() ) {
        ret.push_back( this_page );
    }
    return ret;
}

static void print_responses( const catacurses::window &w, const page &responses )
{
    // Responses go on the right side of the window, add 2 for border + space
    const size_t x_start = getmaxx( w ) / 2 + 2;
    // First line we can print on, +1 for border, +2 for your name + newline
    const int y_start = 2 + 1;

    int curr_y = y_start;
    for( const page_entry &entry : responses.entries ) {
        const nc_color col = entry.col;
        for( const std::string &line : entry.lines ) {
            mvwprintz( w, point( x_start, curr_y ), col, line );
            curr_y += 1;
        }
    }
}

static void print_keybindings( const catacurses::window &w )
{
    // Responses go on the right side of the window, add 2 for border + space
    const size_t x = getmaxx( w ) / 2 + 2;
    const size_t y = getmaxy( w ) - 5;
    mvwprintz( w, point( x, y ), c_magenta, _( "Shift+L: Look at" ) );
    mvwprintz( w, point( x, y + 1 ), c_magenta, _( "Shift+S: Size up stats" ) );
    mvwprintz( w, point( x, y + 2 ), c_magenta, _( "Shift+Y: Yell" ) );
    mvwprintz( w, point( x, y + 3 ), c_magenta, _( "Shift+O: Check opinion" ) );
}

void dialogue_window::cache_msg( const std::string &msg, size_t idx )
{
    const std::vector<std::string> folded = foldstring( msg, getmaxx( d_win ) / 2 );
    draw_cache.push_back( {"", idx} );
    for( const std::string &fs : folded ) {
        draw_cache.push_back( {fs, idx} );
    }
}

void dialogue_window::refresh_response_display()
{
    curr_page = 0;
    can_scroll_down = false;
    can_scroll_up = false;
}

void dialogue_window::handle_scrolling( const int ch )
{
    switch( ch ) {
        case KEY_DOWN:
        case KEY_NPAGE:
            if( can_scroll_down ) {
                curr_page += 1;
            }
            break;
        case KEY_UP:
        case KEY_PPAGE:
            if( can_scroll_up ) {
                curr_page -= 1;
            }
            break;
        default:
            break;
    }
}

void dialogue_window::display_responses( const std::vector<talk_data> &responses )
{
    const int win_maxy = getmaxy( d_win );
    clear_window_texts();
    print_history();

    // TODO: cache paged entries
    // -2 for borders, -2 for your name + newline, -4 for keybindings
    const int page_h = getmaxy( d_win ) - 2 - 2 - 4;
    const int page_w = getmaxx( d_win ) / 2 - 4; // -4 for borders + padding
    const std::vector<page> pages = split_to_pages( responses, page_w, page_h );
    if( !pages.empty() ) {
        if( curr_page >= pages.size() ) {
            curr_page = pages.size() - 1;
        }
        print_responses( d_win, pages[curr_page] );
    }
    print_keybindings( d_win );
    can_scroll_up = curr_page > 0;
    can_scroll_down = curr_page + 1 < pages.size();

    if( can_scroll_up ) {
        mvwprintz( d_win, point( getmaxx( d_win ) - 2 - 2, 2 ), c_green, "^^" );
    }
    if( can_scroll_down ) {
        mvwprintz( d_win, point( FULL_SCREEN_WIDTH - 2 - 2, win_maxy - 2 ), c_green, "vv" );
    }

    wrefresh( d_win );
}
