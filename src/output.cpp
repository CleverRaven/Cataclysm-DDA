#include "output.h"

#include <cerrno>
#include <cctype>
#include <cstdio>
#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <type_traits>
#include <cmath>

#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "input.h"
#include "item.h"
#include "line.h"
#include "name.h"
#include "options.h"
#include "popup.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui_manager.h"
#include "units.h"
#include "point.h"
#include "wcwidth.h"

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#endif

// Display data
int TERMX;
int TERMY;
int POSX;
int POSY;
int TERRAIN_WINDOW_WIDTH;
int TERRAIN_WINDOW_HEIGHT;
int TERRAIN_WINDOW_TERM_WIDTH;
int TERRAIN_WINDOW_TERM_HEIGHT;
int FULL_SCREEN_WIDTH;
int FULL_SCREEN_HEIGHT;

int OVERMAP_WINDOW_HEIGHT;
int OVERMAP_WINDOW_WIDTH;

int OVERMAP_LEGEND_WIDTH;

static std::string rm_prefix( std::string str, char c1 = '<', char c2 = '>' );

scrollingcombattext SCT;
extern bool tile_iso;
extern bool use_tiles;

extern bool test_mode;

// utf8 version
std::vector<std::string> foldstring( const std::string &str, int width, const char split )
{
    std::vector<std::string> lines;
    if( width < 1 ) {
        lines.push_back( str );
        return lines;
    }
    std::stringstream sstr( str );
    std::string strline;
    std::vector<std::string> tags;
    while( std::getline( sstr, strline, '\n' ) ) {
        if( strline.empty() ) {
            // Special case empty lines as std::getline() sets failbit immediately
            // if the line is empty.
            lines.emplace_back();
        } else {
            std::string wrapped = word_rewrap( strline, width, split );
            std::stringstream swrapped( wrapped );
            std::string wline;
            while( std::getline( swrapped, wline, '\n' ) ) {
                // Ensure that each line is independently color-tagged
                // Re-add tags closed in the previous line
                const std::string rawwline = wline;
                if( !tags.empty() ) {
                    std::string swline;
                    for( const std::string &tag : tags ) {
                        swline += tag;
                    }
                    swline += wline;
                    wline = swline;
                }
                // Process the additional tags in the current line
                const std::vector<size_t> tags_pos = get_tag_positions( rawwline );
                for( const size_t tag_pos : tags_pos ) {
                    if( tag_pos + 1 < rawwline.size() && rawwline[tag_pos + 1] == '/' ) {
                        if( !tags.empty() ) {
                            tags.pop_back();
                        }
                    } else {
                        auto tag_end = rawwline.find( '>', tag_pos );
                        if( tag_end != std::string::npos ) {
                            tags.emplace_back( rawwline.substr( tag_pos, tag_end + 1 - tag_pos ) );
                        }
                    }
                }
                // Close any unclosed tags
                if( !tags.empty() ) {
                    std::string swline;
                    swline += wline;
                    for( auto it = tags.rbegin(); it != tags.rend(); ++it ) {
                        // currently the only closing tag is </color>
                        swline += "</color>";
                    }
                    wline = swline;
                }
                // The resulting line can be printed independently and have the correct color
                lines.emplace_back( wline );
            }
        }
    }
    return lines;
}

std::vector<std::string> split_by_color( const std::string &s )
{
    std::vector<std::string> ret;
    std::vector<size_t> tag_positions = get_tag_positions( s );
    size_t last_pos = 0;
    for( size_t tag_position : tag_positions ) {
        ret.push_back( s.substr( last_pos, tag_position - last_pos ) );
        last_pos = tag_position;
    }
    // and the last (or only) one
    ret.push_back( s.substr( last_pos, std::string::npos ) );
    return ret;
}

std::string remove_color_tags( const std::string &s )
{
    std::string ret;
    std::vector<size_t> tag_positions = get_tag_positions( s );
    size_t next_pos = 0;

    if( tag_positions.size() > 1 ) {
        for( size_t tag_position : tag_positions ) {
            ret += s.substr( next_pos, tag_position - next_pos );
            next_pos = s.find( ">", tag_position, 1 ) + 1;
        }

        ret += s.substr( next_pos, std::string::npos );
    } else {
        return s;
    }
    return ret;
}

static void update_color_stack( std::stack<nc_color> &color_stack, const std::string &seg )
{
    color_tag_parse_result tag = get_color_from_tag( seg );
    switch( tag.type ) {
        case color_tag_parse_result::open_color_tag:
            color_stack.push( tag.color );
            break;
        case color_tag_parse_result::close_color_tag:
            if( !color_stack.empty() ) {
                color_stack.pop();
            }
            break;
        case color_tag_parse_result::non_color_tag:
            // Do nothing
            break;
    }
}

void print_colored_text( const catacurses::window &w, const point &p, nc_color &color,
                         const nc_color &base_color, const std::string &text )
{
    if( p.y > -1 && p.x > -1 ) {
        wmove( w, p );
    }
    const auto color_segments = split_by_color( text );
    std::stack<nc_color> color_stack;
    color_stack.push( color );

    for( auto seg : color_segments ) {
        if( seg.empty() ) {
            continue;
        }

        if( seg[0] == '<' ) {
            update_color_stack( color_stack, seg );
            seg = rm_prefix( seg );
        }

        color = color_stack.empty() ? base_color : color_stack.top();
        wprintz( w, color, seg );
    }
}

void trim_and_print( const catacurses::window &w, const point &begin, int width,
                     nc_color base_color, const std::string &text )
{
    std::string sText = trim_by_length( text, width );
    print_colored_text( w, begin, base_color, base_color, sText );
}

std::string trim_by_length( const std::string  &text, int width )
{
    std::string sText;
    if( utf8_width( remove_color_tags( text ) ) > width ) {

        int iLength = 0;
        std::string sTempText;
        std::string sColor;

        const auto color_segments = split_by_color( text );
        for( const std::string &seg : color_segments ) {
            sColor.clear();

            if( !seg.empty() && ( seg.substr( 0, 7 ) == "<color_" || seg.substr( 0, 7 ) == "</color" ) ) {
                sTempText = rm_prefix( seg );

                if( seg.substr( 0, 7 ) == "<color_" ) {
                    sColor = seg.substr( 0, seg.find( '>' ) + 1 );
                }
            } else {
                sTempText = seg;
            }

            const int iTempLen = utf8_width( sTempText );
            iLength += iTempLen;

            if( iLength > width ) {
                sTempText = sTempText.substr( 0, cursorx_to_position( sTempText.c_str(),
                                              iTempLen - ( iLength - width ) - 1, nullptr, -1 ) ) + "\u2026";
            }

            sText += sColor + sTempText;
            if( !sColor.empty() ) {
                sText += "</color>";
            }

            if( iLength > width ) {
                break;
            }
        }
    } else {
        sText = text;
    }
    return sText;
}

int print_scrollable( const catacurses::window &w, int begin_line, const std::string &text,
                      const nc_color &base_color, const std::string &scroll_msg )
{
    const size_t wwidth = getmaxx( w );
    const auto text_lines = foldstring( text, wwidth );
    size_t wheight = getmaxy( w );
    const auto print_scroll_msg = text_lines.size() > wheight;
    if( print_scroll_msg && !scroll_msg.empty() ) {
        // keep the last line free for a message to the player
        wheight--;
    }
    if( begin_line < 0 || text_lines.size() <= wheight ) {
        begin_line = 0;
    } else if( begin_line + wheight >= text_lines.size() ) {
        begin_line = text_lines.size() - wheight;
    }
    nc_color color = base_color;
    for( size_t i = 0; i + begin_line < text_lines.size() && i < wheight; ++i ) {
        print_colored_text( w, point( 0, i ), color, base_color, text_lines[i + begin_line] );
    }
    if( print_scroll_msg && !scroll_msg.empty() ) {
        color = c_white;
        print_colored_text( w, point( 0, wheight ), color, color, scroll_msg );
    }
    return std::max<int>( 0, text_lines.size() - wheight );
}

// returns number of printed lines
int fold_and_print( const catacurses::window &w, const point &begin, int width,
                    const nc_color &base_color, const std::string &text, const char split )
{
    nc_color color = base_color;
    std::vector<std::string> textformatted = foldstring( text, width, split );
    for( int line_num = 0; static_cast<size_t>( line_num ) < textformatted.size(); line_num++ ) {
        print_colored_text( w, begin + point( 0, line_num ), color, base_color,
                            textformatted[line_num] );
    }
    return textformatted.size();
}

int fold_and_print_from( const catacurses::window &w, const point &begin, int width,
                         int begin_line, const nc_color &base_color, const std::string &text )
{
    const int iWinHeight = getmaxy( w );
    std::stack<nc_color> color_stack;
    std::vector<std::string> textformatted = foldstring( text, width );
    for( int line_num = 0; static_cast<size_t>( line_num ) < textformatted.size(); line_num++ ) {
        if( line_num + begin.y - begin_line == iWinHeight ) {
            break;
        }
        if( line_num >= begin_line ) {
            wmove( w, begin + point( 0, -begin_line + line_num ) );
        }
        // split into colorable sections
        std::vector<std::string> color_segments = split_by_color( textformatted[line_num] );
        // for each section, get the color, and print it
        std::vector<std::string>::iterator it;
        for( it = color_segments.begin(); it != color_segments.end(); ++it ) {
            if( !it->empty() && it->at( 0 ) == '<' ) {
                update_color_stack( color_stack, *it );
            }
            if( line_num >= begin_line ) {
                std::string l = rm_prefix( *it );
                if( l != "--" ) { // -- is a separation line!
                    nc_color color = color_stack.empty() ? base_color : color_stack.top();
                    wprintz( w, color, rm_prefix( *it ) );
                } else {
                    for( int i = 0; i < width; i++ ) {
                        wputch( w, c_dark_gray, LINE_OXOX );
                    }
                }
            }
        }
    }
    return textformatted.size();
}

void scrollable_text( const std::function<catacurses::window()> &init_window,
                      const std::string &title, const std::string &text )
{
    constexpr int text_x = 1;
    constexpr int text_y = 1;

    input_context ctxt( "SCROLLABLE_TEXT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    catacurses::window w;
    int width = 0;
    int height = 0;
    int text_w = 0;
    int text_h = 0;
    std::vector<std::string> lines;
    int beg_line = 0;
    int max_beg_line = 0;

    ui_adaptor ui;
    const auto screen_resize_cb = [&]( ui_adaptor & ui ) {
        w = init_window();

        width = getmaxx( w );
        height = getmaxy( w );
        text_w = std::max( 0, width - 2 );
        text_h = std::max( 0, height - 2 );

        lines = foldstring( text, text_w );
        max_beg_line = std::max( 0, static_cast<int>( lines.size() ) - text_h );

        ui.position_from_window( w );
    };
    screen_resize_cb( ui );
    ui.on_screen_resize( screen_resize_cb );
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        draw_border( w, BORDER_COLOR, title, c_black_white );
        for( int line = beg_line, pos_y = text_y; line < std::min<int>( beg_line + text_h, lines.size() );
             ++line, ++pos_y ) {
            nc_color dummy = c_white;
            print_colored_text( w, point( text_x, pos_y ), dummy, dummy, lines[line] );
        }
        scrollbar().offset_x( width - 1 ).offset_y( text_y ).content_size( lines.size() )
        .viewport_pos( std::min( beg_line, max_beg_line ) ).viewport_size( text_h ).apply( w );
        wnoutrefresh( w );
    } );

    std::string action;
    do {
        ui_manager::redraw();

        action = ctxt.handle_input();
        if( action == "UP" ) {
            if( beg_line > 0 ) {
                --beg_line;
            }
        } else if( action == "DOWN" ) {
            if( beg_line < max_beg_line ) {
                ++beg_line;
            }
        } else if( action == "PAGE_UP" ) {
            if( beg_line > text_h ) {
                beg_line -= text_h;
            } else {
                beg_line = 0;
            }
        } else if( action == "PAGE_DOWN" ) {
            // always scroll an entire page's length
            if( beg_line < max_beg_line ) {
                beg_line += text_h;
            }
        }
    } while( action != "CONFIRM" && action != "QUIT" );
}

// returns single string with left aligned name and right aligned value
std::string name_and_value( const std::string &name, const std::string &value, int field_width )
{
    const int text_width = utf8_width( name ) + utf8_width( value );
    const int spacing = std::max( field_width, text_width ) - text_width;
    return name + std::string( spacing, ' ' ) + value;
}

std::string name_and_value( const std::string &name, int value, int field_width )
{
    return name_and_value( name, string_format( "%d", value ), field_width );
}

void center_print( const catacurses::window &w, const int y, const nc_color &FG,
                   const std::string &text )
{
    int window_width = getmaxx( w );
    int string_width = utf8_width( text, true );
    int x;
    if( string_width >= window_width ) {
        x = 0;
    } else {
        x = ( window_width - string_width ) / 2;
    }
    const int available_width = std::max( 1, window_width - x );
    trim_and_print( w, point( x, y ), available_width, FG, text );
}

int right_print( const catacurses::window &w, const int line, const int right_indent,
                 const nc_color &FG, const std::string &text )
{
    const int available_width = std::max( 1, getmaxx( w ) - right_indent );
    const int x = std::max( 0, available_width - utf8_width( text, true ) );
    trim_and_print( w, point( x, line ), available_width, FG, text );
    return x;
}

void wputch( const catacurses::window &w, nc_color FG, int ch )
{
    wattron( w, FG );
    waddch( w, ch );
    wattroff( w, FG );
}

void mvwputch( const catacurses::window &w, const point &p, nc_color FG, int ch )
{
    wattron( w, FG );
    mvwaddch( w, p, ch );
    wattroff( w, FG );
}

void mvwputch( const catacurses::window &w, const point &p, nc_color FG, const std::string &ch )
{
    wattron( w, FG );
    mvwprintw( w, p, ch );
    wattroff( w, FG );
}

void mvwputch_inv( const catacurses::window &w, const point &p, nc_color FG, int ch )
{
    nc_color HC = invert_color( FG );
    wattron( w, HC );
    mvwaddch( w, p, ch );
    wattroff( w, HC );
}

void mvwputch_inv( const catacurses::window &w, const point &p, nc_color FG,
                   const std::string &ch )
{
    nc_color HC = invert_color( FG );
    wattron( w, HC );
    mvwprintw( w, p, ch );
    wattroff( w, HC );
}

void mvwputch_hi( const catacurses::window &w, const point &p, nc_color FG, int ch )
{
    nc_color HC = hilite( FG );
    wattron( w, HC );
    mvwaddch( w, p, ch );
    wattroff( w, HC );
}

void mvwputch_hi( const catacurses::window &w, const point &p, nc_color FG, const std::string &ch )
{
    nc_color HC = hilite( FG );
    wattron( w, HC );
    mvwprintw( w, p, ch );
    wattroff( w, HC );
}

void draw_custom_border(
    const catacurses::window &w, const catacurses::chtype ls, const catacurses::chtype rs,
    const catacurses::chtype ts, const catacurses::chtype bs, const catacurses::chtype tl,
    const catacurses::chtype tr, const catacurses::chtype bl, const catacurses::chtype br,
    const nc_color &FG, const point &pos, int height, int width )
{
    wattron( w, FG );

    height = ( height == 0 ) ? getmaxy( w ) - pos.y : height;
    width = ( width == 0 ) ? getmaxx( w ) - pos.x : width;

    for( int j = pos.y; j < height + pos.y - 1; j++ ) {
        if( ls > 0 ) {
            mvwputch( w, point( pos.x, j ), c_light_gray, ( ls > 1 ) ? ls : LINE_XOXO ); // |
        }

        if( rs > 0 ) {
            mvwputch( w, point( pos.x + width - 1, j ), c_light_gray, ( rs > 1 ) ? rs : LINE_XOXO ); // |
        }
    }

    for( int j = pos.x; j < width + pos.x - 1; j++ ) {
        if( ts > 0 ) {
            mvwputch( w, point( j, pos.y ), c_light_gray, ( ts > 1 ) ? ts : LINE_OXOX ); // --
        }

        if( bs > 0 ) {
            mvwputch( w, point( j, pos.y + height - 1 ), c_light_gray, ( bs > 1 ) ? bs : LINE_OXOX ); // --
        }
    }

    if( tl > 0 ) {
        mvwputch( w, pos, c_light_gray, ( tl > 1 ) ? tl : LINE_OXXO ); // |^
    }

    if( tr > 0 ) {
        mvwputch( w, pos + point( -1 + width, 0 ), c_light_gray, ( tr > 1 ) ? tr : LINE_OOXX ); // ^|
    }

    if( bl > 0 ) {
        mvwputch( w, pos + point( 0, -1 + height ), c_light_gray, ( bl > 1 ) ? bl : LINE_XXOO ); // |_
    }

    if( br > 0 ) {
        mvwputch( w, pos + point( -1 + width, -1 + height ), c_light_gray,
                  ( br > 1 ) ? br : LINE_XOOX ); // _|
    }

    wattroff( w, FG );
}

void draw_border( const catacurses::window &w, nc_color border_color, const std::string &title,
                  nc_color title_color )
{
    wattron( w, border_color );
    wborder( w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff( w, border_color );
    if( !title.empty() ) {
        center_print( w, 0, title_color, title );
    }
}

void draw_border_below_tabs( const catacurses::window &w, nc_color border_color )
{
    int width = getmaxx( w );
    int height = getmaxy( w );
    for( int i = 1; i < width - 1; i++ ) {
        mvwputch( w, point( i, height - 1 ), border_color, LINE_OXOX );
    }
    for( int i = 3; i < height - 1; i++ ) {
        mvwputch( w, point( 0, i ), border_color, LINE_XOXO );
        mvwputch( w, point( width - 1, i ), border_color, LINE_XOXO );
    }
    mvwputch( w, point( 0, height - 1 ), border_color, LINE_XXOO ); // |_
    mvwputch( w, point( width - 1, height - 1 ), border_color, LINE_XOOX ); // _|
}

border_helper::border_info::border_info( border_helper &helper )
    : helper( helper )
{
}

void border_helper::border_info::set( const point &pos, const point &size )
{
    this->pos = pos;
    this->size = size;
    helper.get().border_connection_map.reset();
}

border_helper::border_info &border_helper::add_border()
{
    border_info_list.emplace_front( border_info( *this ) );
    return border_info_list.front();
}

void border_helper::draw_border( const catacurses::window &win )
{
    if( !border_connection_map.has_value() ) {
        border_connection_map.emplace();
        for( const border_info &info : border_info_list ) {
            const point beg = info.pos;
            const point end = info.pos + info.size;
            for( int x = beg.x; x < end.x; ++x ) {
                const point ptop( x, beg.y );
                const point pbottom( x, end.y - 1 );
                if( x > beg.x ) {
                    border_connection_map.value()[ptop].left =
                        border_connection_map.value()[pbottom].left = true;
                }
                if( x < end.x - 1 ) {
                    border_connection_map.value()[ptop].right =
                        border_connection_map.value()[pbottom].right = true;
                }
            }
            for( int y = beg.y; y < end.y; ++y ) {
                const point pleft( beg.x, y );
                const point pright( end.x - 1, y );
                if( y > beg.y ) {
                    border_connection_map.value()[pleft].top =
                        border_connection_map.value()[pright].top = true;
                }
                if( y < end.y - 1 ) {
                    border_connection_map.value()[pleft].bottom =
                        border_connection_map.value()[pright].bottom = true;
                }
            }
        }
    }
    const point win_beg( getbegx( win ), getbegy( win ) );
    const point win_end = win_beg + point( getmaxx( win ), getmaxy( win ) );
    for( const std::pair<const point, border_connection> &conn : border_connection_map.value() ) {
        if( conn.first.x >= win_beg.x && conn.first.x < win_end.x &&
            conn.first.y >= win_beg.y && conn.first.y < win_end.y ) {
            mvwputch( win, conn.first - win_beg, BORDER_COLOR, conn.second.as_curses_line() );
        }
    }
}

int border_helper::border_connection::as_curses_line() const
{
    constexpr int t = 0x8;
    constexpr int r = 0x4;
    constexpr int b = 0x2;
    constexpr int l = 0x1;
    const int conn = ( top ? t : 0 ) | ( right ? r : 0 ) | ( bottom ? b : 0 ) | ( left ? l : 0 );
    switch( conn ) {
        default:
            return '?';
        case 0x3:
            return LINE_OOXX;
        case 0x5:
            return LINE_OXOX;
        case 0x6:
            return LINE_OXXO;
        case 0x7:
            return LINE_OXXX;
        case 0x9:
            return LINE_XOOX;
        case 0xA:
            return LINE_XOXO;
        case 0xB:
            return LINE_XOXX;
        case 0xC:
            return LINE_XXOO;
        case 0xD:
            return LINE_XXOX;
        case 0xE:
            return LINE_XXXO;
        case 0xF:
            return LINE_XXXX;
    }
}

bool query_yn( const std::string &text )
{
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case
                            : input_context::allow_all_keys;

    return query_popup()
           .context( "YESNO" )
           .message( force_uc ?
                     pgettext( "query_yn", "%s (Case Sensitive)" ) :
                     pgettext( "query_yn", "%s" ), text )
           .option( "YES", allow_key )
           .option( "NO", allow_key )
           .cursor( 1 )
           .default_color( c_light_red )
           .query()
           .action == "YES";
}

bool query_int( int &result, const std::string &text )
{
    string_input_popup popup;
    popup.title( text );
    popup.text( "" ).only_digits( true );
    popup.query();
    if( popup.canceled() ) {
        return false;
    }
    result = atoi( popup.text().c_str() );
    return true;
}

std::vector<std::string> get_hotkeys( const std::string &s )
{
    std::vector<std::string> hotkeys;
    size_t start = s.find_first_of( '<' );
    size_t end = s.find_first_of( '>' );
    if( start != std::string::npos && end != std::string::npos ) {
        // hotkeys separated by '|' inside '<' and '>', for example "<e|E|?>"
        size_t lastsep = start;
        size_t sep = s.find_first_of( '|', start );
        while( sep < end ) {
            hotkeys.push_back( s.substr( lastsep + 1, sep - lastsep - 1 ) );
            lastsep = sep;
            sep = s.find_first_of( '|', sep + 1 );
        }
        hotkeys.push_back( s.substr( lastsep + 1, end - lastsep - 1 ) );
    }
    return hotkeys;
}

int popup( const std::string &text, PopupFlags flags )
{
    query_popup pop;
    pop.message( "%s", text );
    if( flags & PF_GET_KEY ) {
        pop.allow_anykey( true );
    } else {
        pop.allow_cancel( true );
    }

    if( flags & PF_FULLSCREEN ) {
        pop.full_screen( true );
    } else if( flags & PF_ON_TOP ) {
        pop.on_top( true );
    }

    pop.context( "POPUP_WAIT" );
    const auto &res = pop.query();
    if( res.evt.type == input_event_t::keyboard ) {
        return res.evt.get_first_input();
    } else {
        return UNKNOWN_UNICODE;
    }
}

//note that passing in iteminfo instances with sType == "DESCRIPTION" does special things
//all this should probably be cleaned up at some point, rather than using a function for things it wasn't meant for
// well frack, half the game uses it so: optional (int)selected argument causes entry highlight, and enter to return entry's key. Also it now returns int
//@param without_getch don't wait getch, return = (int)' ';
input_event draw_item_info( const int iLeft, const int iWidth, const int iTop, const int iHeight,
                            item_info_data &data )
{
    catacurses::window win =
        catacurses::newwin( iHeight, iWidth,
                            point( iLeft, iTop ) );

#if defined(TILES)
    clear_window_area( win );
#endif // TILES
    wclear( win );
    wnoutrefresh( win );

    const input_event result = draw_item_info( win, data );
    return result;
}

std::string string_replace( std::string text, const std::string &before, const std::string &after )
{
    // Check if there's something to replace (mandatory) and it's necessary (optional)
    // Second condition assumes that text is much longer than both &before and &after.
    if( before.empty() || before == after ) {
        return text;
    }

    const size_t before_len = before.length();
    const size_t after_len = after.length();
    size_t pos = 0;

    while( ( pos = text.find( before, pos ) ) != std::string::npos ) {
        text.replace( pos, before_len, after );
        pos += after_len;
    }

    return text;
}

std::string replace_colors( std::string text )
{
    static const std::vector<std::pair<std::string, std::string>> info_colors = {
        {"info", get_all_colors().get_name( c_cyan )},
        {"stat", get_all_colors().get_name( c_light_blue )},
        {"header", get_all_colors().get_name( c_magenta )},
        {"bold", get_all_colors().get_name( c_white )},
        {"dark", get_all_colors().get_name( c_dark_gray )},
        {"good", get_all_colors().get_name( c_green )},
        {"bad", get_all_colors().get_name( c_red )},
        {"neutral", get_all_colors().get_name( c_yellow )}
    };

    for( auto &elem : info_colors ) {
        text = string_replace( text, "<" + elem.first + ">", "<color_" + elem.second + ">" );
        text = string_replace( text, "</" + elem.first + ">", "</color>" );
    }

    return text;
}

void draw_item_filter_rules( const catacurses::window &win, int starty, int height,
                             item_filter_type type )
{
    // Clear every row, but the leftmost/rightmost pixels intact.
    const int len = getmaxx( win ) - 2;
    for( int i = 0; i < height; i++ ) {
        mvwprintz( win, point( 1, starty + i ), c_black, std::string( len, ' ' ) );
    }

    // Not static so that language changes are correctly handled
    const std::array<std::string, 3> intros = {{
            _( "Type part of an item's name to filter it." ),
            _( "Type part of an item's name to move nearby items to the bottom." ),
            _( "Type part of an item's name to move nearby items to the top." )
        }
    };
    const int tab_idx = static_cast<int>( type ) - static_cast<int>( item_filter_type::FIRST );
    starty += 1 + fold_and_print( win, point( 1, starty ), len, c_white, intros[tab_idx] );

    starty += fold_and_print( win, point( 1, starty ), len, c_white,
                              // NOLINTNEXTLINE(cata-text-style): literal comma
                              _( "Separate multiple items with [<color_yellow>,</color>]." ) );
    starty += 1 + fold_and_print( win, point( 1, starty ), len, c_white,
                                  //~ An example of how to separate multiple items with a comma when filtering items.
                                  _( "Example: back,flash,aid, ,band" ) ); // NOLINT(cata-text-style): literal comma

    if( type == item_filter_type::FILTER ) {
        starty += fold_and_print( win, point( 1, starty ), len, c_white,
                                  _( "To exclude items, place [<color_yellow>-</color>] in front." ) );
        starty += 1 + fold_and_print( win, point( 1, starty ), len, c_white,
                                      //~ An example of how to exclude items with - when filtering items.
                                      _( "Example: -pipe,-chunk,-steel" ) );
    }

    starty += fold_and_print( win, point( 1, starty ), len, c_white,
                              _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
                                 "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes or "
                                 "[<color_yellow>d</color>]isassembled components." ) );
    fold_and_print( win, point( 1, starty ), len, c_white,
                    //~ An example of how to filter items based on category or material.
                    _( "Examples: c:food,m:iron,q:hammering,n:toolshelf,d:pipe" ) );
    wnoutrefresh( win );
}

std::string format_item_info( const std::vector<iteminfo> &vItemDisplay,
                              const std::vector<iteminfo> &vItemCompare )
{
    std::string buffer;
    bool bIsNewLine = true;

    for( const auto &i : vItemDisplay ) {
        if( i.sType == "DESCRIPTION" ) {
            // Always start a new line for sType == "DESCRIPTION"
            if( !bIsNewLine ) {
                buffer += "\n";
            }
            if( i.bDrawName ) {
                buffer += i.sName;
            }
            // Always end with a linebreak for sType == "DESCRIPTION"
            buffer += "\n";
            bIsNewLine = true;
        } else {
            if( i.bDrawName ) {
                buffer += i.sName;
            }

            std::string sFmt = i.sFmt;
            std::string sPost;

            //A bit tricky, find %d and split the string
            size_t pos = sFmt.find( "<num>" );
            if( pos != std::string::npos ) {
                buffer += sFmt.substr( 0, pos );
                sPost = sFmt.substr( pos + 5 );
            } else {
                buffer += sFmt;
            }

            if( i.sValue != "-999" ) {
                nc_color thisColor = c_yellow;
                for( auto &k : vItemCompare ) {
                    if( k.sValue != "-999" ) {
                        if( i.sName == k.sName && i.sType == k.sType ) {
                            if( i.dValue > k.dValue - .1 &&
                                i.dValue < k.dValue + .1 ) {
                                thisColor = c_light_gray;
                            } else if( i.dValue > k.dValue ) {
                                if( i.bLowerIsBetter ) {
                                    thisColor = c_light_red;
                                } else {
                                    thisColor = c_light_green;
                                }
                            } else if( i.dValue < k.dValue ) {
                                if( i.bLowerIsBetter ) {
                                    thisColor = c_light_green;
                                } else {
                                    thisColor = c_light_red;
                                }
                            }
                            break;
                        }
                    }
                }
                buffer += colorize( i.sValue, thisColor );
            }
            buffer += sPost;

            // Set bIsNewLine in case the next line should always start in a new line
            if( ( bIsNewLine = i.bNewLine ) ) {
                buffer += "\n";
            }
        }
    }

    return buffer;
}

input_event draw_item_info( const catacurses::window &win, item_info_data &data )
{
    return draw_item_info( [&]() -> catacurses::window {
        return win;
    }, data );
}

input_event draw_item_info( const std::function<catacurses::window()> &init_window,
                            item_info_data &data )
{
    std::string buffer;
    int line_num = data.use_full_win || data.without_border ? 0 : 1;
    if( !data.get_item_name().empty() ) {
        buffer += data.get_item_name() + "\n";
    }
    // If type name is set, and not already contained in item name, output it too
    if( !data.get_type_name().empty() &&
        data.get_item_name().find( data.get_type_name() ) == std::string::npos ) {
        buffer += data.get_type_name() + "\n";
    }
    for( unsigned int i = 0; i < data.padding; i++ ) {
        buffer += "\n";
    }

    buffer += format_item_info( data.get_item_display(), data.get_item_compare() );

    const int b = data.use_full_win ? 0 : ( data.without_border ? 1 : 2 );

    catacurses::window win;
    int width = 0;
    int height = 0;
    std::vector<std::string> folded;

    const auto init = [&]() {
        win = init_window();
        width = getmaxx( win ) - ( data.use_full_win ? 1 : b * 2 );
        height = getmaxy( win ) - ( data.use_full_win ? 0 : 2 );
        folded = foldstring( buffer, width - 1 );
        if( *data.ptr_selected < 0 ) {
            *data.ptr_selected = 0;
        } else if( height < 0 || folded.size() < static_cast<size_t>( height ) ) {
            *data.ptr_selected = 0;
        } else if( static_cast<size_t>( *data.ptr_selected + height ) >= folded.size() ) {
            *data.ptr_selected = static_cast<int>( folded.size() ) - height;
        }
    };

    const auto redraw = [&]() {
        werase( win );
        for( int line = 0; line < height; ++line ) {
            const int idx = *data.ptr_selected + line;
            if( idx >= 0 && static_cast<size_t>( idx ) < folded.size() ) {
                if( folded[idx] == "--" ) {
                    for( int x = 0; x < width; x++ ) {
                        mvwputch( win, point( b + x, line_num + line ), c_dark_gray, LINE_OXOX );
                    }
                } else {
                    trim_and_print( win, point( b, line_num + line ), width - 1, c_light_gray, folded[idx] );
                }
            }
        }
        draw_scrollbar( win, *data.ptr_selected, height, folded.size(),
                        point( data.scrollbar_left ? 0 : getmaxx( win ) - 1,
                               ( data.without_border && data.use_full_win ? 0 : 1 ) ), BORDER_COLOR, true );
        if( !data.without_border ) {
            draw_custom_border( win, buffer.empty() );
        }
        wnoutrefresh( win );
    };

    if( data.without_getch ) {
        init();
        redraw();
        return input_event();
    }

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        init();
        ui.position_from_window( win );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        redraw();
    } );

    input_context ctxt;
    if( data.handle_scrolling ) {
        ctxt.register_action( "PAGE_UP" );
        ctxt.register_action( "PAGE_DOWN" );
    }
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( data.any_input ) {
        ctxt.register_action( "ANY_INPUT" );
    }

    std::string action;
    bool exit = false;
    while( !exit ) {
        ui_manager::redraw();
        action = ctxt.handle_input();
        if( data.handle_scrolling && action == "PAGE_UP" ) {
            if( *data.ptr_selected > 0 ) {
                --*data.ptr_selected;
            }
        } else if( data.handle_scrolling && action == "PAGE_DOWN" ) {
            if( *data.ptr_selected < 0 ||
                ( height > 0 && static_cast<size_t>( *data.ptr_selected + height ) < folded.size() ) ) {
                ++*data.ptr_selected;
            }
        } else if( action == "CONFIRM" || action == "QUIT" ) {
            exit = true;
        } else if( data.any_input && action == "ANY_INPUT" && !ctxt.get_raw_input().sequence.empty() ) {
            exit = true;
        }
    }

    return ctxt.get_raw_input();
}

char rand_char()
{
    switch( rng( 0, 9 ) ) {
        case 0:
            return '|';
        case 1:
            return '-';
        case 2:
            return '#';
        case 3:
            return '?';
        case 4:
            return '&';
        case 5:
            return '.';
        case 6:
            return '%';
        case 7:
            return '{';
        case 8:
            return '*';
        case 9:
            return '^';
    }
    return '?';
}

// this translates symbol y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
int special_symbol( int sym )
{
    switch( sym ) {
        case 'j':
            return LINE_XOXO;
        case 'h':
            return LINE_OXOX;
        case 'c':
            return LINE_XXXX;
        case 'y':
            return LINE_OXXO;
        case 'u':
            return LINE_OOXX;
        case 'n':
            return LINE_XOOX;
        case 'b':
            return LINE_XXOO;
        default:
            return sym;
    }
}

template<typename Prep>
std::string trim( const std::string &s, Prep prep )
{
    auto wsfront = std::find_if_not( s.begin(), s.end(), [&prep]( int c ) {
        return prep( c );
    } );
    return std::string( wsfront, std::find_if_not( s.rbegin(),
    std::string::const_reverse_iterator( wsfront ), [&prep]( int c ) {
        return prep( c );
    } ).base() );
}

std::string trim( const std::string &s )
{
    return trim( s, []( int c ) {
        return isspace( c );
    } );
}

std::string trim_punctuation_marks( const std::string &s )
{
    return trim( s, []( int c ) {
        return ispunct( c );
    } );
}

using char_t = std::string::value_type;
std::string to_upper_case( const std::string &s )
{
    std::string res;
    std::transform( s.begin(), s.end(), std::back_inserter( res ), []( char_t ch ) {
        return std::use_facet<std::ctype<char_t>>( std::locale() ).toupper( ch );
    } );
    return res;
}

// find the position of each non-printing tag in a string
std::vector<size_t> get_tag_positions( const std::string &s )
{
    std::vector<size_t> ret;
    size_t pos = s.find( "<color_", 0, 7 );
    while( pos != std::string::npos ) {
        ret.push_back( pos );
        pos = s.find( "<color_", pos + 1, 7 );
    }
    pos = s.find( "</color>", 0, 8 );
    while( pos != std::string::npos ) {
        ret.push_back( pos );
        pos = s.find( "</color>", pos + 1, 8 );
    }
    std::sort( ret.begin(), ret.end() );
    return ret;
}

// utf-8 version
std::string word_rewrap( const std::string &in, int width, const uint32_t split )
{
    std::string o;

    // find non-printing tags
    std::vector<size_t> tag_positions = get_tag_positions( in );

    int lastwb  = 0; //last word break
    int lastout = 0;
    const char *instr = in.c_str();
    bool skipping_tag = false;
    bool just_wrapped = false;

    for( int j = 0, x = 0; j < static_cast<int>( in.size() ); ) {
        const char *ins = instr + j;
        int len = ANY_LENGTH;
        uint32_t uc = UTF8_getch( &ins, &len );

        if( uc == '<' ) { // maybe skip non-printing tag
            std::vector<size_t>::iterator it;
            for( it = tag_positions.begin(); it != tag_positions.end(); ++it ) {
                if( static_cast<int>( *it ) == j ) {
                    skipping_tag = true;
                    break;
                }
            }
        }

        const int old_j = j;
        j += ANY_LENGTH - len;

        if( skipping_tag ) {
            if( uc == '>' ) {
                skipping_tag = false;
            }
            continue;
        }

        if( just_wrapped && uc == ' ' ) { // ignore spaces after wrapping
            lastwb = lastout = j;
            continue;
        }

        x += mk_wcwidth( uc );

        if( uc == split || uc >= 0x2E80 ) { // param split (default ' ') or CJK characters
            if( x <= width ) {
                lastwb = j; // break after character
            } else {
                lastwb = old_j; // break before character
            }
        }

        if( x > width ) {
            if( lastwb == lastout ) {
                lastwb = old_j;
            }
            // old_j may equal to lastout, this checks it and ensures there's at least one character in the line.
            if( lastwb == lastout ) {
                lastwb = j;
            }
            for( int k = lastout; k < lastwb; k++ ) {
                o += in[k];
            }
            o += '\n';
            x = 0;
            lastout = j = lastwb;
            just_wrapped = true;
        } else {
            just_wrapped = false;
        }
    }
    for( int k = lastout; k < static_cast<int>( in.size() ); k++ ) {
        o += in[k];
    }

    return o;
}

void draw_tab( const catacurses::window &w, int iOffsetX, const std::string &sText, bool bSelected )
{
    int iOffsetXRight = iOffsetX + utf8_width( sText ) + 1;

    mvwputch( w, point( iOffsetX, 0 ),      c_light_gray, LINE_OXXO ); // |^
    mvwputch( w, point( iOffsetXRight, 0 ), c_light_gray, LINE_OOXX ); // ^|
    mvwputch( w, point( iOffsetX, 1 ),      c_light_gray, LINE_XOXO ); // |
    mvwputch( w, point( iOffsetXRight, 1 ), c_light_gray, LINE_XOXO ); // |

    mvwprintz( w, point( iOffsetX + 1, 1 ), ( bSelected ) ? h_light_gray : c_light_gray, sText );

    for( int i = iOffsetX + 1; i < iOffsetXRight; i++ ) {
        mvwputch( w, point( i, 0 ), c_light_gray, LINE_OXOX );  // -
    }

    if( bSelected ) {
        mvwputch( w, point( iOffsetX - 1, 1 ),      h_light_gray, '<' );
        mvwputch( w, point( iOffsetXRight + 1, 1 ), h_light_gray, '>' );

        for( int i = iOffsetX + 1; i < iOffsetXRight; i++ ) {
            mvwputch( w, point( i, 2 ), c_black, ' ' );
        }

        mvwputch( w, point( iOffsetX, 2 ),      c_light_gray, LINE_XOOX ); // _|
        mvwputch( w, point( iOffsetXRight, 2 ), c_light_gray, LINE_XXOO ); // |_

    } else {
        mvwputch( w, point( iOffsetX, 2 ),      c_light_gray, LINE_XXOX ); // _|_
        mvwputch( w, point( iOffsetXRight, 2 ), c_light_gray, LINE_XXOX ); // _|_
    }
}

void draw_subtab( const catacurses::window &w, int iOffsetX, const std::string &sText,
                  bool bSelected,
                  bool bDecorate, bool bDisabled )
{
    int iOffsetXRight = iOffsetX + utf8_width( sText ) + 1;

    if( !bDisabled ) {
        mvwprintz( w, point( iOffsetX + 1, 0 ), ( bSelected ) ? h_light_gray : c_light_gray, sText );
    } else {
        mvwprintz( w, point( iOffsetX + 1, 0 ), ( bSelected ) ? h_dark_gray : c_dark_gray, sText );
    }

    if( bSelected ) {
        if( !bDisabled ) {
            mvwputch( w, point( iOffsetX - bDecorate, 0 ),      h_light_gray, '<' );
            mvwputch( w, point( iOffsetXRight + bDecorate, 0 ), h_light_gray, '>' );
        } else {
            mvwputch( w, point( iOffsetX - bDecorate, 0 ),      h_dark_gray, '<' );
            mvwputch( w, point( iOffsetXRight + bDecorate, 0 ), h_dark_gray, '>' );
        }

        for( int i = iOffsetX + 1; bDecorate && i < iOffsetXRight; i++ ) {
            mvwputch( w, point( i, 1 ), c_black, ' ' );
        }
    }
}

void draw_tabs( const catacurses::window &w, const std::vector<std::string> &tab_texts,
                size_t current_tab )
{
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        mvwputch( w, point( i, 2 ), BORDER_COLOR, LINE_OXOX ); // -
    }

    mvwputch( w, point( 0, 2 ), BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w, point( width - 1, 2 ), BORDER_COLOR, LINE_OOXX ); // ^|

    const int tab_step = 3;
    int x = 2;
    for( size_t i = 0; i < tab_texts.size(); ++i ) {
        const std::string &tab_text = tab_texts[i];
        draw_tab( w, x, tab_text, i == current_tab );
        x += utf8_width( tab_text ) + tab_step;
    }
}

void draw_tabs( const catacurses::window &w, const std::vector<std::string> &tab_texts,
                const std::string &current_tab )
{
    auto it = std::find( tab_texts.begin(), tab_texts.end(), current_tab );
    assert( it != tab_texts.end() );
    draw_tabs( w, tab_texts, it - tab_texts.begin() );
}

/**
 * Draw a scrollbar (Legacy function, use class scrollbar instead!)
 * @param window Pointer of window to draw on
 * @param iCurrentLine The starting line or currently selected line out of the iNumLines lines
 * @param iContentHeight Height of the scrollbar
 * @param iNumLines Total number of lines
 * @param offset Point indicating drawing offset
 * @param bar_color Default line color
 * @param bDoNotScrollToEnd True if the last (iContentHeight-1) lines cannot be a start position or be selected
 *   If false, iCurrentLine can be from 0 to iNumLines - 1.
 *   If true, iCurrentLine can be at most iNumLines - iContentHeight.
 **/
void draw_scrollbar( const catacurses::window &window, const int iCurrentLine,
                     const int iContentHeight, const int iNumLines, const point &offset,
                     nc_color bar_color, const bool bDoNotScrollToEnd )
{
    scrollbar()
    .offset_x( offset.x )
    .offset_y( offset.y )
    .content_size( iNumLines )
    .viewport_pos( iCurrentLine )
    .viewport_size( iContentHeight )
    .slot_color( bar_color )
    .scroll_to_last( !bDoNotScrollToEnd )
    .apply( window );
}

scrollbar::scrollbar()
    : offset_x_v( 0 ), offset_y_v( 0 ), content_size_v( 0 ),
      viewport_pos_v( 0 ), viewport_size_v( 0 ),
      border_color_v( BORDER_COLOR ), arrow_color_v( c_light_green ),
      slot_color_v( c_white ), bar_color_v( c_cyan_cyan ), scroll_to_last_v( false )
{
}

scrollbar &scrollbar::offset_x( int offx )
{
    offset_x_v = offx;
    return *this;
}

scrollbar &scrollbar::offset_y( int offy )
{
    offset_y_v = offy;
    return *this;
}

scrollbar &scrollbar::content_size( int csize )
{
    content_size_v = csize;
    return *this;
}

scrollbar &scrollbar::viewport_pos( int vpos )
{
    viewport_pos_v = vpos;
    return *this;
}

scrollbar &scrollbar::viewport_size( int vsize )
{
    viewport_size_v = vsize;
    return *this;
}

scrollbar &scrollbar::border_color( nc_color border_c )
{
    border_color_v = border_c;
    return *this;
}

scrollbar &scrollbar::arrow_color( nc_color arrow_c )
{
    arrow_color_v = arrow_c;
    return *this;
}

scrollbar &scrollbar::slot_color( nc_color slot_c )
{
    slot_color_v = slot_c;
    return *this;
}

scrollbar &scrollbar::bar_color( nc_color bar_c )
{
    bar_color_v = bar_c;
    return *this;
}

scrollbar &scrollbar::scroll_to_last( bool scr2last )
{
    scroll_to_last_v = scr2last;
    return *this;
}

void scrollbar::apply( const catacurses::window &window )
{
    if( viewport_size_v >= content_size_v || content_size_v <= 0 ) {
        // scrollbar not needed, fill output area with borders
        for( int i = offset_y_v; i < offset_y_v + viewport_size_v; ++i ) {
            mvwputch( window, point( offset_x_v, i ), border_color_v, LINE_XOXO );
        }
    } else {
        mvwputch( window, point( offset_x_v, offset_y_v ), arrow_color_v, '^' );
        mvwputch( window, point( offset_x_v, offset_y_v + viewport_size_v - 1 ), arrow_color_v, 'v' );

        int slot_size = viewport_size_v - 2;
        int bar_size = std::max( 2, slot_size * viewport_size_v / content_size_v );
        int scrollable_size = scroll_to_last_v ? content_size_v : content_size_v - viewport_size_v + 1;

        int bar_start;
        if( viewport_pos_v == 0 ) {
            bar_start = 0;
        } else if( scrollable_size > 2 ) {
            bar_start = ( slot_size - 1 - bar_size ) * ( viewport_pos_v - 1 ) / ( scrollable_size - 2 ) + 1;
        } else {
            bar_start = slot_size - bar_size;
        }
        int bar_end = bar_start + bar_size;

        for( int i = 0; i < slot_size; ++i ) {
            if( i >= bar_start && i < bar_end ) {
                mvwputch( window, point( offset_x_v, offset_y_v + 1 + i ), bar_color_v, LINE_XOXO );
            } else {
                mvwputch( window, point( offset_x_v, offset_y_v + 1 + i ), slot_color_v, LINE_XOXO );
            }
        }
    }
}

void scrolling_text_view::set_text( const std::string &text )
{
    text_ = foldstring( text, text_width() );
    offset_ = 0;
}

void scrolling_text_view::scroll_up()
{
    if( offset_ ) {
        --offset_;
    }
}

void scrolling_text_view::scroll_down()
{
    if( offset_ < max_offset() ) {
        ++offset_;
    }
}

void scrolling_text_view::page_up()
{
    offset_ = std::max( 0, offset_ - getmaxy( w_ ) );
}

void scrolling_text_view::page_down()
{
    offset_ = std::min( max_offset(), offset_ + getmaxy( w_ ) );
}

void scrolling_text_view::draw( const nc_color &base_color )
{
    werase( w_ );

    const int height = getmaxy( w_ );

    if( max_offset() > 0 ) {
        scrollbar().
        content_size( text_.size() ).
        viewport_pos( offset_ ).
        viewport_size( height ).
        scroll_to_last( false ).
        apply( w_ );
    } else {
        // No scrollbar; we need to draw the window edge instead
        for( int i = 0; i < height; i++ ) {
            mvwputch( w_, point( 0, i ), BORDER_COLOR, LINE_XOXO );
        }
    }

    nc_color color = base_color;
    int end = std::min( num_lines() - offset_, height );
    for( int line_num = 0; line_num < end; ++line_num ) {
        print_colored_text( w_, point( 1, line_num ), color, base_color,
                            text_[line_num + offset_] );
    }

    wnoutrefresh( w_ );
}

int scrolling_text_view::text_width()
{
    return getmaxx( w_ ) - 1;
}

int scrolling_text_view::num_lines()
{
    return text_.size();
}

int scrolling_text_view::max_offset()
{
    return std::max( 0, num_lines() - getmaxy( w_ ) );
}

void calcStartPos( int &iStartPos, const int iCurrentLine, const int iContentHeight,
                   const int iNumEntries )
{
    if( iNumEntries <= iContentHeight ) {
        iStartPos = 0;
    } else if( get_option<bool>( "MENU_SCROLL" ) ) {
        iStartPos = iCurrentLine - ( iContentHeight - 1 ) / 2;
        if( iStartPos < 0 ) {
            iStartPos = 0;
        } else if( iStartPos + iContentHeight > iNumEntries ) {
            iStartPos = iNumEntries - iContentHeight;
        }
    } else {
        if( iCurrentLine < iStartPos ) {
            iStartPos = iCurrentLine;
        } else if( iCurrentLine >= iStartPos + iContentHeight ) {
            iStartPos = 1 + iCurrentLine - iContentHeight;
        }
    }
}

#if defined(_MSC_VER)
std::string cata::string_formatter::raw_string_format( const char *const format, ... )
{
    va_list args;
    va_start( args, format );

    va_list args_copy;
    va_copy( args_copy, args );
    const int result = _vscprintf_p( format, args_copy );
    va_end( args_copy );
    if( result == -1 ) {
        throw std::runtime_error( "Bad format string for printf: \"" + std::string( format ) + "\"" );
    }

    std::string buffer( result, '\0' );
    _vsprintf_p( &buffer[0], result + 1, format, args ); //+1 for string's null
    va_end( args );

    return buffer;
}
#else

// Cygwin has limitations which prevents
// from using more than 9 positional arguments.
// This functions works around it in two ways:
//
// First if all positional arguments are in "natural" order
// (i.e. like %1$d %2$d %3$d),
// then their positions is stripped away and string
// formatted without positions.
//
// Otherwise only 9 arguments are passed to vsnprintf
//
std::string rewrite_vsnprintf( const char *msg )
{
    bool contains_positional = false;
    const char *orig_msg = msg;
    const char *formats = "diouxXeEfFgGaAcsCSpnm";

    std::string rewritten_msg;
    std::string rewritten_msg_optimised;
    const char *ptr = nullptr;
    int next_positional_arg = 1;
    while( true ) {

        // First find next position where argument might be used
        ptr = strchr( msg, '%' );
        if( !ptr ) {
            rewritten_msg += msg;
            rewritten_msg_optimised += msg;
            break;
        }

        // Write portion of the string that was before %
        rewritten_msg += std::string( msg, ptr );
        rewritten_msg_optimised += std::string( msg, ptr );

        const char *arg_start = ptr;

        ptr++;

        // If it simply '%%', then no processing needed
        if( *ptr == '%' ) {
            rewritten_msg += "%%";
            rewritten_msg_optimised += "%%";
            msg = ptr + 1;
            continue;
        }

        // Parse possible number of positional argument
        int positional_arg = 0;
        while( isdigit( *ptr ) ) {
            positional_arg = positional_arg * 10 + *ptr - '0';
            ptr++;
        }

        // If '$' ever follows a numeral, the string has a positional arg
        if( *ptr == '$' ) {
            contains_positional = true;
        }

        // Check if it's expected argument
        if( *ptr == '$' && positional_arg == next_positional_arg ) {
            next_positional_arg++;
        } else {
            next_positional_arg = -1;
        }

        // Now find where it ends
        const char *end = strpbrk( ptr, formats );
        if( !end ) {
            // Format string error. Just bail.
            return orig_msg;
        }

        // write entire argument to rewritten_msg
        if( positional_arg < 10 ) {
            std::string argument( arg_start, end + 1 );
            rewritten_msg += argument;
        } else {
            rewritten_msg += "<formatting error>";
        }

        // write argument without position to rewritten_msg_optimised
        if( next_positional_arg > 0 ) {
            std::string argument( ptr + 1, end + 1 );
            rewritten_msg_optimised += "%" + argument;
        }

        msg = end + 1;
    }

    if( !contains_positional ) {
        return orig_msg;
    }

    if( next_positional_arg > 0 ) {
        // If all positioned arguments were in order (%1$d %2$d) then we simply
        // strip arguments
        return rewritten_msg_optimised;
    }

    return rewritten_msg;
}

// NOLINTNEXTLINE(cert-dcl50-cpp)
std::string cata::string_formatter::raw_string_format( const char *format, ... )
{
    va_list args;
    va_start( args, format );

    errno = 0; // Clear errno before trying
    std::vector<char> buffer( 1024, '\0' );

#if defined(__CYGWIN__)
    std::string rewritten_format = rewrite_vsnprintf( format );
    format = rewritten_format.c_str();
#endif

    for( ;; ) {
        const size_t buffer_size = buffer.size();

        va_list args_copy;
        va_copy( args_copy, args );
        const int result = vsnprintf( &buffer[0], buffer_size, format, args_copy );
        va_end( args_copy );

        // No error, and the buffer is big enough; we're done.
        if( result >= 0 && static_cast<size_t>( result ) < buffer_size ) {
            break;
        }

        // Standards conformant versions return -1 on error only.
        // Some non-standard versions return -1 to indicate a bigger buffer is needed.
        // Some of the latter set errno to ERANGE at the same time.
        if( result < 0 && errno && errno != ERANGE ) {
            throw std::runtime_error( "Bad format string for printf: \"" + std::string( format ) + "\"" );
        }

        // Looks like we need to grow... bigger, definitely bigger.
        buffer.resize( buffer_size * 2 );
    }

    va_end( args );
    return std::string( &buffer[0] );
}
#endif

void replace_name_tags( std::string &input )
{
    // these need to replace each tag with a new randomly generated name
    while( input.find( "<full_name>" ) != std::string::npos ) {
        replace_substring( input, "<full_name>", Name::get( nameFlags::IsFullName ),
                           false );
    }
    while( input.find( "<family_name>" ) != std::string::npos ) {
        replace_substring( input, "<family_name>", Name::get( nameFlags::IsFamilyName ),
                           false );
    }
    while( input.find( "<given_name>" ) != std::string::npos ) {
        replace_substring( input, "<given_name>", Name::get( nameFlags::IsGivenName ),
                           false );
    }
    while( input.find( "<town_name>" ) != std::string::npos ) {
        replace_substring( input, "<town_name>", Name::get( nameFlags::IsTownName ),
                           false );
    }
}

void replace_city_tag( std::string &input, const std::string &name )
{
    replace_substring( input, "<city>", name, true );
}

void replace_substring( std::string &input, const std::string &substring,
                        const std::string &replacement, bool all )
{
    if( all ) {
        while( input.find( substring ) != std::string::npos ) {
            replace_substring( input, substring, replacement, false );
        }
    } else {
        size_t len = substring.length();
        size_t offset = input.find( substring );
        input.replace( offset, len, replacement );
    }
}

//wrap if for i18n
std::string &capitalize_letter( std::string &str, size_t n )
{
    char c = str[n];
    if( !str.empty() && c >= 'a' && c <= 'z' ) {
        c += 'A' - 'a';
        str[n] = c;
    }

    return str;
}

//remove prefix of a string, between c1 and c2, i.e., "<prefix>remove it"
std::string rm_prefix( std::string str, char c1, char c2 )
{
    if( !str.empty() && str[0] == c1 ) {
        size_t pos = str.find_first_of( c2 );
        if( pos != std::string::npos ) {
            str = str.substr( pos + 1 );
        }
    }
    return str;
}

// draw a menu-item-like string with highlighted shortcut character
// Example: <w>ield, m<o>ve
// returns: output length (in console cells)
size_t shortcut_print( const catacurses::window &w, const point &p, nc_color text_color,
                       nc_color shortcut_color, const std::string &fmt )
{
    wmove( w, p );
    return shortcut_print( w, text_color, shortcut_color, fmt );
}

//same as above, from current position
size_t shortcut_print( const catacurses::window &w, nc_color text_color, nc_color shortcut_color,
                       const std::string &fmt )
{
    std::string text = shortcut_text( shortcut_color, fmt );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    print_colored_text( w, point( -1, -1 ), text_color, text_color, text );

    return utf8_width( remove_color_tags( text ) );
}

//generate colorcoded shortcut text
std::string shortcut_text( nc_color shortcut_color, const std::string &fmt )
{
    size_t pos = fmt.find_first_of( '<' );
    size_t pos_end = fmt.find_first_of( '>' );
    if( pos_end != std::string::npos && pos < pos_end ) {
        size_t sep = std::min( fmt.find_first_of( '|', pos ), pos_end );
        std::string prestring = fmt.substr( 0, pos );
        std::string poststring = fmt.substr( pos_end + 1, std::string::npos );
        std::string shortcut = fmt.substr( pos + 1, sep - pos - 1 );

        return prestring + colorize( shortcut, shortcut_color ) + poststring;
    }

    // no shortcut?
    return fmt;
}

std::pair<std::string, nc_color>
get_bar( float cur, float max, int width, bool extra_resolution,
         const std::vector<nc_color> &colors )
{
    std::string result;
    float status = cur / max;
    status = status > 1 ? 1 : status;
    status = status < 0 ? 0 : status;
    float sw = status * width;

    nc_color col = colors[static_cast<int>( ( 1 - status ) * colors.size() )];
    if( status == 0 ) {
        col = colors.back();
    } else if( ( sw < 0.5 ) && ( sw > 0 ) ) {
        result = ":";
    } else {
        result += std::string( sw, '|' );
        if( extra_resolution && ( sw - static_cast<int>( sw ) >= 0.5 ) ) {
            result += "\\";
        }
    }

    return std::make_pair( result, col );
}

std::pair<std::string, nc_color> get_hp_bar( const int cur_hp, const int max_hp,
        const bool is_mon )
{
    if( cur_hp == 0 ) {
        return std::make_pair( "-----", c_light_gray );
    }
    return get_bar( cur_hp, max_hp, 5, !is_mon );
}

std::pair<std::string, nc_color> get_stamina_bar( int cur_stam, int max_stam )
{
    if( cur_stam == 0 ) {
        return std::make_pair( "-----", c_light_gray );
    }
    return get_bar( cur_stam, max_stam, 5, true, { c_cyan, c_light_cyan, c_yellow, c_light_red, c_red } );
}

std::pair<std::string, nc_color> get_light_level( const float light )
{
    using pair_t = std::pair<std::string, nc_color>;
    static const std::array<pair_t, 6> strings {
        {
            pair_t {translate_marker( "unknown" ), c_pink},
            pair_t {translate_marker( "bright" ), c_yellow},
            pair_t {translate_marker( "cloudy" ), c_white},
            pair_t {translate_marker( "shady" ), c_light_gray},
            pair_t {translate_marker( "dark" ), c_dark_gray},
            pair_t {translate_marker( "very dark" ), c_black_white}
        }
    };
    // Avoid magic number
    static const int maximum_light_level = static_cast< int >( strings.size() ) - 1;
    const int light_level = clamp( static_cast< int >( std::ceil( light ) ), 0, maximum_light_level );
    const size_t array_index = static_cast< size_t >( light_level );
    return pair_t{ _( strings[array_index].first ), strings[array_index].second };
}

std::string get_labeled_bar( const double val, const int width, const std::string &label, char c )
{
    const std::array<std::pair<double, char>, 1> ratings =
    {{ std::make_pair( 1.0, c ) }};
    return get_labeled_bar( val, width, label, ratings.begin(), ratings.end() );
}

/**
 * Inserts a table into a window, with data right-aligned.
 * @param pad Reduce table width by padding left side.
 * @param line Line to insert table.
 * @param columns Number of columns. Can be 1.
 * @param nc_color &FG Default color of table text.
 * @param divider To insert a character separating table entries. Can be blank.
 * @param r_align true for right aligned, false for left aligned.
 * @param data Text data to fill.
 * Make sure each entry of the data vector fits into one cell, including divider if any.
 */
void insert_table( const catacurses::window &w, int pad, int line, int columns,
                   const nc_color &FG, const std::string &divider, bool r_align,
                   const std::vector<std::string> &data )
{
    const int width = getmaxx( w );
    const int rows = getmaxy( w );
    const int col_width = ( ( width - pad ) / columns );
    int indent = 1;  // 1 for right window border
    if( r_align ) {
        indent = ( col_width * columns ) + 1;
    }
    int div = columns - 1;
    int offset = 0;

#if defined(__ANDROID__)
    input_context ctxt( "INSERT_TABLE" );
#endif
    wattron( w, FG );
    for( int i = 0; i < rows * columns; i++ ) {
        if( i + offset * columns >= static_cast<int>( data.size() ) ) {
            break;
        }
        int y = line + ( i / columns );
        if( r_align ) {
            indent -= col_width;
        }
        if( div != 0 ) {
            if( r_align ) {
                right_print( w, y, indent - utf8_width( divider ), FG, divider );
            } else {
                fold_and_print_from( w, point( indent + col_width - utf8_width( divider ), y ),
                                     utf8_width( divider ), 0, FG, divider );
            }
            div--;
        } else {
            div = columns - 1;
        }

        if( r_align ) {
            right_print( w, y, indent, c_white, data[i + offset * columns] );
            if( indent == 1 ) {
                indent = ( col_width * columns ) + 1;
            }
        } else {
            fold_and_print_from( w, point( indent, y ), col_width, 0, c_white, data[i + offset * columns] );
            indent += col_width;
            if( indent == ( col_width * columns ) + 1 ) {
                indent = 1;
            }
        }
    }
    wattroff( w, FG );
}

scrollingcombattext::cSCT::cSCT( const point &p_pos, const direction p_oDir,
                                 const std::string &p_sText, const game_message_type p_gmt,
                                 const std::string &p_sText2, const game_message_type p_gmt2,
                                 const std::string &p_sType )
{
    pos = p_pos;
    sType = p_sType;
    oDir = p_oDir;

    // translate from player relative to screen relative direction
#if defined(TILES)
    iso_mode = tile_iso && use_tiles;
#else
    iso_mode = false;
#endif
    oUp = iso_mode ? direction::NORTHEAST : direction::NORTH;
    oUpRight = iso_mode ? direction::EAST : direction::NORTHEAST;
    oRight = iso_mode ? direction::SOUTHEAST : direction::EAST;
    oDownRight = iso_mode ? direction::SOUTH : direction::SOUTHEAST;
    oDown = iso_mode ? direction::SOUTHWEST : direction::SOUTH;
    oDownLeft = iso_mode ? direction::WEST : direction::SOUTHWEST;
    oLeft = iso_mode ? direction::NORTHWEST : direction::WEST;
    oUpLeft = iso_mode ? direction::NORTH : direction::NORTHWEST;

    point pairDirXY = direction_XY( oDir );

    dir = pairDirXY;

    if( dir == point_zero ) {
        // This would cause infinite loop otherwise
        oDir = direction::WEST;
        dir.x = -1;
    }

    iStep = 0;
    iStepOffset = 0;

    sText = p_sText;
    gmt = p_gmt;

    sText2 = p_sText2;
    gmt2 = p_gmt2;

}

void scrollingcombattext::add( const point &pos, direction p_oDir,
                               const std::string &p_sText, const game_message_type p_gmt,
                               const std::string &p_sText2, const game_message_type p_gmt2,
                               const std::string &p_sType )
{
    if( get_option<bool>( "ANIMATION_SCT" ) ) {

        int iCurStep = 0;

        bool tiled = false;
        bool iso_mode = false;
#if defined(TILES)
        tiled = use_tiles;
        iso_mode = tile_iso && use_tiles;
#endif

        if( p_sType == "hp" ) {
            //Remove old HP bar
            removeCreatureHP();

            if( p_oDir == direction::WEST || p_oDir == direction::NORTHWEST ||
                p_oDir == ( iso_mode ? direction::NORTH : direction::SOUTHWEST ) ) {
                p_oDir = ( iso_mode ? direction::NORTHWEST : direction::WEST );
            } else {
                p_oDir = ( iso_mode ? direction::SOUTHEAST : direction::EAST );
            }

        } else {
            //reserve Left/Right for creature hp display
            if( p_oDir == ( iso_mode ? direction::SOUTHEAST : direction::EAST ) ) {
                p_oDir = ( one_in( 2 ) ) ? ( iso_mode ? direction::EAST : direction::NORTHEAST ) :
                         ( iso_mode ? direction::SOUTH : direction::SOUTHEAST );

            } else if( p_oDir == ( iso_mode ? direction::NORTHWEST : direction::WEST ) ) {
                p_oDir = ( one_in( 2 ) ) ? ( iso_mode ? direction::NORTH : direction::NORTHWEST ) :
                         ( iso_mode ? direction::WEST : direction::SOUTHWEST );
            }
        }

        // in tiles, SCT that scroll downwards are inserted at the beginning of the vector to prevent
        // oversize ASCII tiles overdrawing messages below them.
        if( tiled && ( p_oDir == direction::SOUTHWEST || p_oDir == direction::SOUTH ||
                       p_oDir == ( iso_mode ? direction::WEST : direction::SOUTHEAST ) ) ) {

            //Message offset: multiple impacts in the same direction in short order overriding prior messages (mostly turrets)
            for( auto &iter : vSCT ) {
                if( iter.getDirecton() == p_oDir && ( iter.getStep() + iter.getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter.advanceStepOffset();
                }
            }
            vSCT.insert( vSCT.begin(), cSCT( pos, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2,
                                             p_sType ) );

        } else {
            //Message offset: this time in reverse.
            for( std::vector<cSCT>::reverse_iterator iter = vSCT.rbegin(); iter != vSCT.rend(); ++iter ) {
                if( iter->getDirecton() == p_oDir && ( iter->getStep() + iter->getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter->advanceStepOffset();
                }
            }
            vSCT.push_back( cSCT( pos, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2, p_sType ) );
        }

    }
}

std::string scrollingcombattext::cSCT::getText( const std::string &type ) const
{
    if( !sText2.empty() ) {
        if( oDir == oUpLeft || oDir == oDownLeft || oDir == oLeft ) {
            if( type == "first" ) {
                return sText2 + " ";

            } else if( type == "full" ) {
                return sText2 + " " + sText;
            }
        } else {
            if( type == "second" ) {
                return " " + sText2;
            } else if( type == "full" ) {
                return sText + " " + sText2;
            }
        }
    } else if( type == "second" ) {
        return {};
    }

    return sText;
}

game_message_type scrollingcombattext::cSCT::getMsgType( const std::string &type ) const
{
    if( !sText2.empty() ) {
        if( oDir == oUpLeft || oDir == oDownLeft || oDir == oLeft ) {
            if( type == "first" ) {
                return gmt2;
            }
        } else {
            if( type == "second" ) {
                return gmt2;
            }
        }
    }

    return gmt;
}

int scrollingcombattext::cSCT::getPosX() const
{
    if( getStep() > 0 ) {
        int iDirOffset = ( oDir == oRight ) ? 1 : ( ( oDir == oLeft ) ? -1 : 0 );

        if( oDir == oUp || oDir == oDown ) {

            if( iso_mode ) {
                iDirOffset = ( oDir == oUp ) ? 1 : -1;
            }

            //Center text
            iDirOffset -= utf8_width( getText() ) / 2;

        } else if( oDir == oLeft || oDir == oDownLeft || oDir == oUpLeft ) {
            //Right align text
            iDirOffset -= utf8_width( getText() ) - 1;
        }

        return pos.x + iDirOffset + ( dir.x * ( ( sType == "hp" ) ? ( getStepOffset() + 1 ) :
                                                ( getStepOffset() * ( iso_mode ? 2 : 1 ) + getStep() ) ) );
    }

    return 0;
}

int scrollingcombattext::cSCT::getPosY() const
{
    if( getStep() > 0 ) {
        int iDirOffset = ( oDir == oDown ) ? 1 : ( ( oDir == oUp ) ? -1 : 0 );

        if( iso_mode ) {
            if( oDir == oLeft || oDir == oRight ) {
                iDirOffset = ( oDir == oRight ) ? 1 : -1;
            }

            if( oDir == oUp || oDir == oDown ) {
                //Center text
                iDirOffset -= utf8_width( getText() ) / 2;

            } else if( oDir == oLeft || oDir == oDownLeft || oDir == oUpLeft ) {
                //Right align text
                iDirOffset -= utf8_width( getText() ) - 1;
            }

        }

        return pos.y + iDirOffset + ( dir.y * ( ( iso_mode && sType == "hp" ) ? ( getStepOffset() + 1 ) :
                                                ( getStepOffset() * ( iso_mode ? 2 : 1 ) + getStep() ) ) );
    }

    return 0;
}

void scrollingcombattext::advanceAllSteps()
{
    std::vector<cSCT>::iterator iter = vSCT.begin();

    while( iter != vSCT.end() ) {
        if( iter->advanceStep() > this->iMaxSteps ) {
            iter = vSCT.erase( iter );
        } else {
            ++iter;
        }
    }
}

void scrollingcombattext::removeCreatureHP()
{
    //check for previous hp display and delete it
    for( std::vector<cSCT>::iterator iter = vSCT.begin(); iter != vSCT.end(); ++iter ) {
        if( iter->getType() == "hp" ) {
            vSCT.erase( iter );
            break;
        }
    }
}

nc_color msgtype_to_color( const game_message_type type, const bool bOldMsg )
{
    static const std::map<game_message_type, std::pair<nc_color, nc_color>> colors {
        {m_good,     {c_light_green, c_green}},
        {m_bad,      {c_light_red,   c_red}},
        {m_mixed,    {c_pink,    c_magenta}},
        {m_warning,  {c_yellow,  c_brown}},
        {m_info,     {c_light_blue,  c_blue}},
        {m_neutral,  {c_white,   c_light_gray}},
        {m_debug,    {c_white,   c_light_gray}},
        {m_headshot, {c_pink,    c_magenta}},
        {m_critical, {c_yellow,  c_brown}},
        {m_grazing,  {c_light_blue,  c_blue}}
    };

    const auto it = colors.find( type );
    if( it == std::end( colors ) ) {
        return bOldMsg ? c_light_gray : c_white;
    }

    return bOldMsg ? it->second.second : it->second.first;
}

/**
 * Match text containing wildcards (*)
 * @param text_in Text to check
 * @param pattern_in Pattern to check text_in against
 * Case insensitive search
 * Possible patterns:
 * *
 * wooD
 * wood*
 * *wood
 * Wood*aRrOW
 * wood*arrow*
 * *wood*arrow
 * *wood*hard* *x*y*z*arrow*
 **/
bool wildcard_match( const std::string &text_in, const std::string &pattern_in )
{
    std::string text = text_in;

    if( text.empty() ) {
        return false;
    } else if( text == "*" ) {
        return true;
    }

    std::vector<std::string> pattern = string_split( wildcard_trim_rule( pattern_in ), '*' );

    if( pattern.size() == 1 ) { // no * found
        return ( text.length() == pattern[0].length() && ci_find_substr( text, pattern[0] ) != -1 );
    }

    for( auto it = pattern.begin(); it != pattern.end(); ++it ) {
        if( it == pattern.begin() && !it->empty() ) {
            if( text.length() < it->length() ||
                ci_find_substr( text.substr( 0, it->length() ), *it ) == -1 ) {
                return false;
            }

            text = text.substr( it->length(), text.length() - it->length() );
        } else if( it == pattern.end() - 1 && !it->empty() ) {
            if( text.length() < it->length() ||
                ci_find_substr( text.substr( text.length() - it->length(),
                                             it->length() ), *it ) == -1 ) {
                return false;
            }
        } else {
            if( !( *it ).empty() ) {
                int pos = ci_find_substr( text, *it );
                if( pos == -1 ) {
                    return false;
                }

                text = text.substr( pos + static_cast<int>( it->length() ),
                                    static_cast<int>( text.length() ) - pos );
            }
        }
    }

    return true;
}

std::string wildcard_trim_rule( const std::string &pattern_in )
{
    std::string pattern = pattern_in;
    size_t pos = pattern.find( "**" );

    //Remove all double ** in pattern
    while( pos != std::string::npos ) {
        pattern = pattern.substr( 0, pos ) + pattern.substr( pos + 1, pattern.length() - pos - 1 );
        pos = pattern.find( "**" );
    }

    return pattern;
}

std::vector<std::string> string_split( const std::string &text_in, char delim_in )
{
    std::vector<std::string> elems;

    if( text_in.empty() ) {
        return elems; // Well, that was easy.
    }

    std::stringstream ss( text_in );
    std::string item;
    while( std::getline( ss, item, delim_in ) ) {
        elems.push_back( item );
    }

    if( text_in.back() == delim_in ) {
        elems.push_back( "" );
    }

    return elems;
}

// find substring (case insensitive)
int ci_find_substr( const std::string &str1, const std::string &str2, const std::locale &loc )
{
    std::string::const_iterator it = std::search( str1.begin(), str1.end(), str2.begin(), str2.end(),
    [&]( const char str1_in, const char str2_in ) {
        return std::toupper( str1_in, loc ) == std::toupper( str2_in, loc );
    } );
    if( it != str1.end() ) {
        return it - str1.begin();
    } else {
        return -1;    // not found
    }
}

/**
* Convert, round up and format a volume.
*/
std::string format_volume( const units::volume &volume )
{
    return format_volume( volume, 0, nullptr, nullptr );
}

/**
* Convert, clamp, round up and format a volume,
* taking into account the specified width (0 for unlimited space),
* optionally returning a flag that indicate if the value was truncated to fit the width,
* optionally returning the formatted value as double.
*/
std::string format_volume( const units::volume &volume, int width, bool *out_truncated,
                           double *out_value )
{
    // convert and get the units preferred scale
    int scale = 0;
    double value = convert_volume( volume.value(), &scale );
    // clamp to the specified width
    if( width != 0 ) {
        value = clamp_to_width( value, std::abs( width ), scale, out_truncated );
    }
    // round up
    value = round_up( value, scale );
    if( out_value != nullptr ) {
        *out_value = value;
    }
    // format
    if( width < 0 ) {
        // left-justify the specified width
        return string_format( "%-*.*f", std::abs( width ), scale, value );
    } else if( width > 0 ) {
        // right-justify the specified width
        return string_format( "%*.*f", width, scale, value );
    } else {
        // no width
        return string_format( "%.*f", scale, value );
    }
}

// In non-SDL mode, width/height is just what's specified in the menu
#if !defined(TILES)
// We need to override these for Windows console resizing
#   if !defined(_WIN32)
int get_terminal_width()
{
    int width = get_option<int>( "TERMINAL_X" );
    return width < FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH : width;
}

int get_terminal_height()
{
    return get_option<int>( "TERMINAL_Y" );
}
#   endif

bool is_draw_tiles_mode()
{
    return false;
}
#endif

void mvwprintz( const catacurses::window &w, const point &p, const nc_color &FG,
                const std::string &text )
{
    wattron( w, FG );
    mvwprintw( w, p, text );
    wattroff( w, FG );
}

void wprintz( const catacurses::window &w, const nc_color &FG, const std::string &text )
{
    wattron( w, FG );
    wprintw( w, text );
    wattroff( w, FG );
}
