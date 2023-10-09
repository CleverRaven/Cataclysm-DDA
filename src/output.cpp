#include "output.h"

#include <cctype>
// IWYU pragma: no_include <sys/errno.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include "cached_options.h" // IWYU pragma: keep
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesport.h" // IWYU pragma: keep
#include "cursesdef.h"
#include "game_constants.h"
#include "input.h"
#include "item.h"
#include "line.h"
#include "name.h"
#include "game.h"
#include "options.h"
#include "point.h"
#include "popup.h"
#include "rng.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui_manager.h"
#include "unicode.h"
#include "units_utility.h"
#include "wcwidth.h"

#if defined(__ANDROID__)
#include <jni.h>
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
int OVERMAP_WINDOW_TERM_WIDTH;
int OVERMAP_WINDOW_TERM_HEIGHT;

int OVERMAP_LEGEND_WIDTH;

scrollingcombattext SCT;

// Mouseover selection delays to keep different types of scrolling from fighting
// Currently used by multiline_list
static const std::chrono::duration<int, std::milli> base_mouse_delay =
    std::chrono::milliseconds( 100 );
static const std::chrono::duration<int, std::milli> scrollwheel_delay =
    std::chrono::milliseconds( 500 );

std::string string_from_int( const catacurses::chtype ch )
{
    catacurses::chtype charcode = ch;
    // LINE_NESW  - X for on, O for off
    switch( ch ) {
        case LINE_XOXO:
            charcode = LINE_XOXO_C;
            break;
        case LINE_OXOX:
            charcode = LINE_OXOX_C;
            break;
        case LINE_XXOO:
            charcode = LINE_XXOO_C;
            break;
        case LINE_OXXO:
            charcode = LINE_OXXO_C;
            break;
        case LINE_OOXX:
            charcode = LINE_OOXX_C;
            break;
        case LINE_XOOX:
            charcode = LINE_XOOX_C;
            break;
        case LINE_XXOX:
            charcode = LINE_XXOX_C;
            break;
        case LINE_XXXO:
            charcode = LINE_XXXO_C;
            break;
        case LINE_XOXX:
            charcode = LINE_XOXX_C;
            break;
        case LINE_OXXX:
            charcode = LINE_OXXX_C;
            break;
        case LINE_XXXX:
            charcode = LINE_XXXX_C;
            break;
        default:
            break;
    }
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char buffer[2] = { static_cast<char>( charcode ), '\0' };
    return buffer;
}

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
                        size_t tag_end = rawwline.find( '>', tag_pos );
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

std::vector<std::string> split_by_color( const std::string_view s )
{
    std::vector<std::string> ret;
    std::vector<size_t> tag_positions = get_tag_positions( s );
    size_t last_pos = 0;
    for( size_t tag_position : tag_positions ) {
        ret.emplace_back( s.substr( last_pos, tag_position - last_pos ) );
        last_pos = tag_position;
    }
    // and the last (or only) one
    ret.emplace_back( s.substr( last_pos, std::string::npos ) );
    return ret;
}

std::string remove_color_tags( const std::string_view s )
{
    std::string ret;
    std::vector<size_t> tag_positions = get_tag_positions( s );
    if( tag_positions.empty() ) {
        return std::string( s );
    }

    size_t next_pos = 0;
    for( size_t tag_position : tag_positions ) {
        ret += s.substr( next_pos, tag_position - next_pos );
        next_pos = s.find( ">", tag_position, 1 ) + 1;
    }

    ret += s.substr( next_pos, std::string::npos );
    return ret;
}

color_tag_parse_result::tag_type update_color_stack(
    std::stack<nc_color> &color_stack, const std::string_view seg,
    const report_color_error color_error )
{
    color_tag_parse_result tag = get_color_from_tag( seg, color_error );
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
    return tag.type;
}

void print_colored_text( const catacurses::window &w, const point &p, nc_color &color,
                         const nc_color &base_color, const std::string_view text,
                         const report_color_error color_error )
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
            const color_tag_parse_result::tag_type type = update_color_stack(
                        color_stack, seg, color_error );
            if( type != color_tag_parse_result::non_color_tag ) {
                seg = rm_prefix( seg );
            }
        }

        color = color_stack.empty() ? base_color : color_stack.top();
        wprintz( w, color, seg );
    }
}

void trim_and_print( const catacurses::window &w, const point &begin,
                     const int width, const nc_color &base_color,
                     const std::string &text,
                     const report_color_error color_error )
{
    std::string sText = trim_by_length( text, width );
    nc_color dummy = base_color;
    print_colored_text( w, begin, dummy, base_color, sText, color_error );
}

std::string trim_by_length( const std::string &text, int width )
{
    if( width <= 0 ) {
        debugmsg( "Unable to trim string '%s' to width %d.  Returning empty string.", text, width );
        return "";
    }
    std::string sText;
    sText.reserve( width );
    if( utf8_width( remove_color_tags( text ) ) > width ) {

        int iLength = 0;
        std::string sTempText;
        sTempText.reserve( width );
        std::string sColor;
        std::string sColorClose;
        std::string sEllipsis = "\u2026";

        const std::vector<std::string> color_segments = split_by_color( text );
        for( size_t i = 0; i < color_segments.size() ; ++i ) {
            const std::string &seg = color_segments[i];
            if( seg.empty() ) {
                // TODO: Check is required right now because, for a fully-color-tagged string, split_by_color
                // returns an empty string first
                continue;
            }
            sColor.clear();
            sColorClose.clear();

            if( !seg.empty() && ( seg.substr( 0, 7 ) == "<color_" || seg.substr( 0, 7 ) == "</color" ) ) {
                sTempText = rm_prefix( seg );

                if( seg.substr( 0, 7 ) == "<color_" ) {
                    sColor = seg.substr( 0, seg.find( '>' ) + 1 );
                    sColorClose = "</color>";
                }
            } else {
                sTempText = seg;
            }

            const int iTempLen = utf8_width( sTempText );
            iLength += iTempLen;
            int next_char_width = 0;
            if( i + 1 < color_segments.size() ) {
                next_char_width = mk_wcwidth( UTF8_getch( remove_color_tags( color_segments[i + 1] ) ) );
            }

            int pos = 0;
            bool trimmed = false;
            if( iLength > width || ( iLength == width && i + 1 < color_segments.size() ) ) {
                // This segment won't fit OR
                // This segment just fits and there's another segment coming
                cursorx_to_position( sTempText.c_str(), iTempLen - ( iLength - width ) - 1, &pos, -1 );
                sTempText = sColor.append( sTempText.substr( 0, pos ) ).append( sEllipsis ).append( sColorClose );
                trimmed = true;
            } else if( iLength + next_char_width >= width ) {
                // This segments fits, but the next segment starts with a wide character
                sTempText = sColor.append( sTempText ).append( sColorClose ).append( sEllipsis );
                trimmed = true;
            }

            if( trimmed ) {
                sText += sTempText; // Color tags were handled when the segment was trimmed
                break;
            } else { // This segment fits, and the next one has room to start
                sText += sColor.append( sTempText ).append( sColorClose );
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
    const bool print_scroll_msg = text_lines.size() > wheight;
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
            color_tag_parse_result::tag_type type = color_tag_parse_result::non_color_tag;
            if( !it->empty() && it->at( 0 ) == '<' ) {
                type = update_color_stack( color_stack, *it );
            }
            if( line_num >= begin_line ) {
                std::string l = *it;
                if( type != color_tag_parse_result::non_color_tag ) {
                    l = rm_prefix( l );
                }
                if( l != "--" ) { // -- is a separation line!
                    nc_color color = color_stack.empty() ? base_color : color_stack.top();
                    wprintz( w, color, l );
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
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    constexpr point text2( 1, 1 );

    input_context ctxt( "SCROLLABLE_TEXT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // mouse
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );

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
        draw_border( w, BORDER_COLOR, title );
        for( int line = beg_line, pos_y = text2.y; line < std::min<int>( beg_line + text_h, lines.size() );
             ++line, ++pos_y ) {
            nc_color dummy = c_white;
            print_colored_text( w, point( text2.x, pos_y ), dummy, dummy, lines[line] );
        }
        scrollbar().offset_x( width - 1 ).offset_y( text2.y ).content_size( lines.size() )
        .viewport_pos( std::min( beg_line, max_beg_line ) ).viewport_size( text_h ).apply( w );
        wnoutrefresh( w );
    } );

    std::string action;
    do {
        ui_manager::redraw();

        action = ctxt.handle_input();
        if( action == "UP" || action == "SCROLL_UP" || action == "DOWN" || action == "SCROLL_DOWN" ) {
            beg_line = inc_clamp( beg_line, action == "DOWN" || action == "SCROLL_DOWN", max_beg_line );
        } else if( action == "PAGE_UP" ) {
            beg_line = std::max( beg_line - text_h, 0 );
        } else if( action == "PAGE_DOWN" ) {
            // always scroll an entire page's length
            if( beg_line < max_beg_line ) {
                beg_line += text_h;
            }
        }
    } while( action != "CONFIRM" && action != "QUIT" );
}

/*If name and value fit in field_width, should add a single string equivalent to trimmed_name_and_value
 *If the name would not fit, it is folded, preserving an empty column for values, and
 *The resulting vector of strings is returned
 */
void add_folded_name_and_value( std::vector<std::string> &current_vector, const std::string &name,
                                const std::string &value, const int field_width )
{
    const int name_width = utf8_width( name );
    const int value_width = utf8_width( value );
    if( name_width + value_width + 1 >= field_width ) {
        if( value_width >= field_width ) {
            debugmsg( "Unable to fit name (%s) and value (%s) in available space (%i)", name, value,
                      field_width );
        } else {
            std::vector<std::string> new_lines;
            new_lines = foldstring( name, field_width - value_width );
            const int spacing = field_width - utf8_width( new_lines.back() ) - value_width;
            new_lines.back() = new_lines.back() + std::string( spacing, ' ' ) + value;
            current_vector.insert( current_vector.end(), new_lines.begin(), new_lines.end() );
        }
    } else {
        const int spacing = field_width - name_width - value_width;
        current_vector.emplace_back( name + std::string( spacing, ' ' ) + value );
    }
}

void add_folded_name_and_value( std::vector<std::string> &current_vector, const std::string &name,
                                const int value, const int field_width )
{
    add_folded_name_and_value( current_vector, name, string_format( "%d", value ), field_width );
}

// returns single string with left aligned name and right aligned value
std::string trimmed_name_and_value( const std::string &name, const std::string &value,
                                    const int field_width )
{
    const int text_width = utf8_width( name ) + utf8_width( value );
    if( text_width >= field_width ) {
        //Since it's easier to abbreviate a string than a number, try to preserve the value
        std::string trimmed_name = trim_by_length( name, field_width - utf8_width( value ) - 1 );
        return trimmed_name + " " + value;
    } else {
        const int spacing = field_width - text_width;
        return name + std::string( spacing, ' ' ) + value;
    }
}

std::string trimmed_name_and_value( const std::string &name, int const value,
                                    const int field_width )
{
    return trimmed_name_and_value( name, string_format( "%d", value ), field_width );
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

void border_helper::draw_border( const catacurses::window &win, nc_color border_color )
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
            mvwputch( win, conn.first - win_beg, border_color, conn.second.as_curses_line() );
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
#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_NATIVE_UI" ) ) {
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID get_nativeui_method_id = env->GetMethodID( clazz, "getNativeUI",
                                           "()Lcom/cleverraven/cataclysmdda/NativeUI;" );
        jobject native_ui_obj = env->CallObjectMethod( activity, get_nativeui_method_id );
        jclass native_ui_cls( env->GetObjectClass( native_ui_obj ) );
        jmethodID queryYN_method_id = env->GetMethodID( native_ui_cls, "queryYN", "(Ljava/lang/String;)Z" );
        jstring jstr = env->NewStringUTF( text.c_str() );
        bool result = env->CallBooleanMethod( native_ui_obj, queryYN_method_id, jstr );
        env->DeleteLocalRef( jstr );
        env->DeleteLocalRef( native_ui_cls );
        env->DeleteLocalRef( native_ui_obj );
        env->DeleteLocalRef( clazz );
        env->DeleteLocalRef( activity );
        return result;
    }
#endif // defined(__ANDROID__)
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case_or_non_modified_letters
                            : input_context::allow_all_keys;

    return query_popup()
           .preferred_keyboard_mode( keyboard_mode::keycode )
           .context( "YESNO" )
           .message( force_uc && !is_keycode_mode_supported() ?
                     pgettext( "query_yn", "%s (Case Sensitive)" ) :
                     pgettext( "query_yn", "%s" ), text )
           .option( "YES", allow_key )
           .option( "NO", allow_key )
           .cursor( 1 )
           .default_color( c_light_red )
           .query()
           .action == "YES";
}

query_ynq_result query_ynq( const std::string &text )
{
    // TODO: android native UI
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case_or_non_modified_letters
                            : input_context::allow_all_keys;

    const std::string action =
        query_popup()
        .context( "YESNOQUIT" )
        .message( force_uc && !is_keycode_mode_supported()
                  ? pgettext( "query_yn", "%s (Case Sensitive)" )
                  : pgettext( "query_yn", "%s" ), text )
        .option( "YES", allow_key )
        .option( "NO", allow_key )
        .option( "QUIT", allow_key )
        .allow_cancel( true )
        .cursor( 2 )
        .default_color( c_light_red )
        .query()
        .action;
    if( action == "NO" ) {
        return query_ynq_result::no;
    } else if( action == "YES" ) {
        return query_ynq_result::yes;
    }
    return query_ynq_result::quit;
}

bool query_int( int &result, const std::string &text )
{
    string_input_popup popup;
    popup.title( text );
    popup.text( "" ).only_digits( true );
    int temp = popup.query_int();
    if( popup.canceled() ) {
        return false;
    }
    result = temp;
    return true;
}

std::vector<std::string> get_hotkeys( const std::string_view s )
{
    std::vector<std::string> hotkeys;
    size_t start = s.find_first_of( '<' );
    size_t end = s.find_first_of( '>' );
    if( start != std::string::npos && end != std::string::npos ) {
        // hotkeys separated by '|' inside '<' and '>', for example "<e|E|?>"
        size_t lastsep = start;
        size_t sep = s.find_first_of( '|', start );
        while( sep < end ) {
            hotkeys.emplace_back( s.substr( lastsep + 1, sep - lastsep - 1 ) );
            lastsep = sep;
            sep = s.find_first_of( '|', sep + 1 );
        }
        hotkeys.emplace_back( s.substr( lastsep + 1, end - lastsep - 1 ) );
    }
    return hotkeys;
}

int popup( const std::string &text, PopupFlags flags )
{
#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_NATIVE_UI" ) && flags == PF_NONE ) {
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID get_nativeui_method_id = env->GetMethodID( clazz, "getNativeUI",
                                           "()Lcom/cleverraven/cataclysmdda/NativeUI;" );
        jobject native_ui_obj = env->CallObjectMethod( activity, get_nativeui_method_id );
        jclass native_ui_cls( env->GetObjectClass( native_ui_obj ) );
        jmethodID queryYN_method_id = env->GetMethodID( native_ui_cls, "popup", "(Ljava/lang/String;)V" );
        jstring jstr = env->NewStringUTF( remove_color_tags( text ).c_str() );
        env->CallVoidMethod( native_ui_obj, queryYN_method_id, jstr );
        env->DeleteLocalRef( jstr );
        env->DeleteLocalRef( native_ui_cls );
        env->DeleteLocalRef( native_ui_obj );
        env->DeleteLocalRef( clazz );
        env->DeleteLocalRef( activity );
        return UNKNOWN_UNICODE;
    }
#endif
    query_popup pop;
    pop.preferred_keyboard_mode( keyboard_mode::keychar );
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
    const query_popup::result &res = pop.query();
    if( res.evt.type == input_event_t::keyboard_char ) {
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

    return draw_item_info( win, data );
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

    for( const auto &elem : info_colors ) {
        text = string_replace( text, "<" + elem.first + ">", "<color_" + elem.second + ">" );
        text = string_replace( text, "</" + elem.first + ">", "</color>" );
    }

    return text;
}

static const std::array<translation, 3> item_filter_rule_intros {
    to_translation( "Type part of an item's name to filter it." ),
    to_translation( "Type part of an item's name to move nearby items to the bottom." ),
    to_translation( "Type part of an item's name to move nearby items to the top." )
};

std::string item_filter_rule_string( const item_filter_type type )
{
    std::ostringstream str;
    const int tab_idx = static_cast<int>( type ) - static_cast<int>( item_filter_type::FIRST );
    str << item_filter_rule_intros[tab_idx];
    // NOLINTNEXTLINE(cata-text-style): literal comma
    str << "\n\n" << _( "Separate multiple items with [<color_yellow>,</color>]." );
    //~ An example of how to separate multiple items with a comma when filtering items.
    str << "\n" << _( "Example: back,flash,aid, ,band" ); // NOLINT(cata-text-style): literal comma
    str << "\n\n" << _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
                        "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes, "
                        "[<color_yellow>s</color>]skill taught by books, "
                        "[<color_yellow>d</color>]isassembled components, or "
                        "items satisfying [<color_yellow>b</color>]oth conditions." );
    //~ An example of how to filter items based on category or material.
    str << "\n" << _( "Example: c:food,m:iron,q:hammering,n:toolshelf,d:pipe,s:devices,b:mre;sealed" );
    str << "\n\n" << _( "To exclude items, place [<color_yellow>-</color>] in front.  "
                        "Place [<color_yellow>--</color>] in front to include only matching items." );
    //~ An example of how to exclude items with - when filtering items.
    str << "\n" << _( "Example: steel,-chunk,--c:spare parts" );
    return str.str();
}

void draw_item_filter_rules( const catacurses::window &win, const int starty, const int height,
                             const item_filter_type type )
{
    // Clear every row, but the leftmost/rightmost pixels intact.
    const int len = getmaxx( win ) - 2;
    for( int i = 0; i < height; i++ ) {
        mvwprintz( win, point( 1, starty + i ), c_black, std::string( len, ' ' ) );
    }

    fold_and_print( win, point( 1, starty ), len, c_white, "%s", item_filter_rule_string( type ) );

    wnoutrefresh( win );
}

std::string format_item_info( const std::vector<iteminfo> &vItemDisplay,
                              const std::vector<iteminfo> &vItemCompare )
{
    std::string buffer;
    bool bIsNewLine = true;

    for( const iteminfo &i : vItemDisplay ) {
        if( i.sType == "DESCRIPTION" ) {
            // Always start a new line for sType == "DESCRIPTION"
            if( !bIsNewLine ) {
                buffer += "\n";
            }
            if( i.bDrawName ) {
                buffer += i.sName;
            }
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
                for( const iteminfo &k : vItemCompare ) {
                    if( k.sValue != "-999" ) {
                        if( i.sName == k.sName && i.sType == k.sType ) {
                            double iVal = i.dValue;
                            double kVal = k.dValue;
                            if( i.sFmt != k.sFmt ) {
                                // Different units, compare unit adjusted vals
                                iVal = i.dUnitAdjustedVal;
                                kVal = k.dUnitAdjustedVal;
                            }
                            if( iVal > kVal - .01 &&
                                iVal < kVal + .01 ) {
                                thisColor = c_light_gray;
                            } else if( iVal > kVal ) {
                                if( i.bLowerIsBetter ) {
                                    thisColor = c_light_red;
                                } else {
                                    thisColor = c_light_green;
                                }
                            } else if( iVal < kVal ) {
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
        }

        // Set bIsNewLine in case the next line should always start in a new line
        if( ( bIsNewLine = i.bNewLine ) ) {
            buffer += "\n";
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
    scrollbar item_info_bar;

    const auto clamp_ptr_selected_scroll = [&]() {
        if( *data.ptr_selected < 0 || height < 0 ||
            folded.size() < static_cast<size_t>( height ) ) {
            *data.ptr_selected = 0;
        } else if( static_cast<size_t>( *data.ptr_selected ) + height >= folded.size() ) {
            *data.ptr_selected = static_cast<int>( folded.size() ) - height;
        }
    };

    const auto init = [&]() {
        win = init_window();
        width = getmaxx( win ) - ( data.use_full_win ? 1 : b * 2 );
        height = getmaxy( win ) - ( data.use_full_win ? 0 : 2 );
        folded = foldstring( buffer, width - 1 );
        clamp_ptr_selected_scroll();
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
        item_info_bar.offset_x( data.scrollbar_left ? 0 : getmaxx( win ) - 1 )
        .offset_y( data.without_border && data.use_full_win ? 0 : 1 )
        .content_size( folded.size() ).viewport_size( height )
        .viewport_pos( *data.ptr_selected )
        .scroll_to_last( false )
        .apply( win );
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

    input_context ctxt( "default", keyboard_mode::keychar );
    if( data.handle_scrolling ) {
        ctxt.register_action( "PAGE_UP" );
        ctxt.register_action( "PAGE_DOWN" );
        item_info_bar.set_draggable( ctxt );
    }
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( data.any_input ) {
        ctxt.register_action( "ANY_INPUT" );
    }

    std::string action;
    while( true ) {
        ui_manager::redraw();
        action = ctxt.handle_input();
        std::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );

        if( data.handle_scrolling && item_info_bar.handle_dragging( action, coord, *data.ptr_selected ) ) {
            // No action required, the scrollbar has handled it
        } else if( data.handle_scrolling && action == "PAGE_UP" ) {
            *data.ptr_selected -= height;
            clamp_ptr_selected_scroll();
        } else if( data.handle_scrolling && action == "PAGE_DOWN" ) {
            *data.ptr_selected += height;
            clamp_ptr_selected_scroll();
        } else if( action == "CONFIRM" || action == "QUIT" ||
                   ( data.any_input && action == "ANY_INPUT" &&
                     !ctxt.get_raw_input().sequence.empty() ) ) {
            break;
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

template<typename Predicate>
static std::string trim( std::string_view s, Predicate pred )
{
    auto wsfront = std::find_if_not( s.begin(), s.end(), pred );
    std::string_view::const_reverse_iterator wsfront_reverse( wsfront );
    auto wsend = std::find_if_not( s.crbegin(), wsfront_reverse, pred );
    return std::string( wsfront, wsend.base() );
}

template<typename Prep>
std::string trim_trailing( const std::string_view s, Prep prep )
{
    return std::string( s.begin(), std::find_if_not(
    s.rbegin(), s.rend(), [&prep]( int c ) {
        return prep( c );
    } ).base() );
}

std::string trim( const std::string_view s )
{
    return trim( s, []( int c ) {
        return isspace( c );
    } );
}

std::string trim_trailing_punctuations( const std::string_view s )
{
    return trim_trailing( s, []( int c ) {
        // '<' and '>' are used for tags and should not be removed
        return c == '.' || c == '!';
    } );
}

std::string remove_punctuations( const std::string_view s )
{
    std::string result;
    std::remove_copy_if( s.begin(), s.end(), std::back_inserter( result ),
    []( unsigned char ch ) {
        return std::ispunct( ch ) && ch != '_';
    } );
    return result;
}

using char_t = std::string::value_type;
std::string to_upper_case( const std::string &s )
{
    const auto &f = std::use_facet<std::ctype<wchar_t>>( std::locale() );
    std::wstring wstr = utf8_to_wstr( s );
    f.toupper( wstr.data(), wstr.data() + wstr.size() );
    return wstr_to_utf8( wstr );
}

// find the position of each non-printing tag in a string
std::vector<size_t> get_tag_positions( const std::string_view s )
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

        if( uc == split || is_cjk_or_emoji( uc ) ) {
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

void draw_tab( const catacurses::window &w, int iOffsetX, const std::string_view sText,
               bool bSelected )
{
    int iOffsetXRight = iOffsetX + utf8_width( sText, true ) + 1;

    mvwputch( w, point( iOffsetX, 0 ),      c_light_gray, LINE_OXXO ); // |^
    mvwputch( w, point( iOffsetXRight, 0 ), c_light_gray, LINE_OOXX ); // ^|
    mvwputch( w, point( iOffsetX, 1 ),      c_light_gray, LINE_XOXO ); // |
    mvwputch( w, point( iOffsetXRight, 1 ), c_light_gray, LINE_XOXO ); // |

    nc_color selected = h_white;
    nc_color not_selected = c_light_gray;
    if( bSelected ) {
        print_colored_text( w, point( iOffsetX + 1, 1 ), selected, selected, sText );
    } else {
        print_colored_text( w, point( iOffsetX + 1, 1 ), not_selected, not_selected, sText );
    }

    for( int i = iOffsetX + 1; i < iOffsetXRight; i++ ) {
        mvwputch( w, point( i, 0 ), c_light_gray, LINE_OXOX );  // -
    }

    if( bSelected ) {
        mvwputch( w, point( iOffsetX - 1, 1 ),      h_white, '<' );
        mvwputch( w, point( iOffsetXRight + 1, 1 ), h_white, '>' );

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

inclusive_rectangle<point> draw_subtab( const catacurses::window &w, int iOffsetX,
                                        const std::string &sText,  bool bSelected,
                                        bool bDecorate, bool bDisabled )
{
    int iOffsetXRight = iOffsetX + utf8_width( sText ) + 1;

    if( !bDisabled ) {
        mvwprintz( w, point( iOffsetX + 1, 0 ), bSelected ? h_white : c_light_gray, sText );
    } else {
        mvwprintz( w, point( iOffsetX + 1, 0 ), bSelected ? h_dark_gray : c_dark_gray, sText );
    }

    if( bSelected ) {
        if( !bDisabled ) {
            mvwputch( w, point( iOffsetX - bDecorate, 0 ),      h_white, '<' );
            mvwputch( w, point( iOffsetXRight + bDecorate, 0 ), h_white, '>' );
        } else {
            mvwputch( w, point( iOffsetX - bDecorate, 0 ),      h_dark_gray, '<' );
            mvwputch( w, point( iOffsetXRight + bDecorate, 0 ), h_dark_gray, '>' );
        }

        for( int i = iOffsetX + 1; bDecorate && i < iOffsetXRight; i++ ) {
            mvwputch( w, point( i, 1 ), c_black, ' ' );
        }
    }
    return inclusive_rectangle<point>( point( iOffsetX, 0 ), point( iOffsetXRight, 0 ) );
}

std::map<size_t, inclusive_rectangle<point>> draw_tabs( const catacurses::window &w,
        const std::vector<std::string> &tab_texts,
        const size_t current_tab, const size_t offset )
{
    std::map<size_t, inclusive_rectangle<point>> tab_map;

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
        const int txt_width = utf8_width( tab_text, true );
        tab_map.emplace( i + offset, inclusive_rectangle<point>( point( x, 1 ), point( x + txt_width,
                         1 ) ) );
        x += txt_width + tab_step;
    }

    return tab_map;
}

std::map<std::string, inclusive_rectangle<point>> draw_tabs( const catacurses::window &w,
        const std::vector<std::string> &tab_texts, const std::string &current_tab )
{
    auto it = std::find( tab_texts.begin(), tab_texts.end(), current_tab );
    cata_assert( it != tab_texts.end() );
    std::map<size_t, inclusive_rectangle<point>> tab_map =
                draw_tabs( w, tab_texts, it - tab_texts.begin() );
    std::map<std::string, inclusive_rectangle<point>> ret_map;
    for( size_t i = 0; i < tab_texts.size(); i++ ) {
        if( tab_map.count( i ) > 0 ) {
            ret_map.emplace( tab_texts[i], tab_map[i] );
        }
    }
    return ret_map;
}

best_fit find_best_fit_in_size( const std::vector<int> &size_of_items_to_fit, const int &selected,
                                const int &allowed_size, const int &spacer_size, const int &continuation_marker_size )
{
    int best_fit_length = 0;
    best_fit returnVal;
    returnVal.start = 0;
    returnVal.length = 0;
    int adjusted_allowed_size = allowed_size;
    for( int num_fitting = static_cast<int>( size_of_items_to_fit.size() ); num_fitting > 0;
         --num_fitting ) {
        //Iterate through all possible places the list can start
        for( int start_index = 0;
             start_index <= static_cast<int>( size_of_items_to_fit.size() ) - num_fitting; ++start_index ) {
            int space_required = 0;
            int cont_marker_allowance = 0;
            bool selected_response_included = false;
            for( int i = start_index; i < ( num_fitting + start_index ); ++i ) {
                space_required += size_of_items_to_fit[i] + spacer_size;
                if( i == selected ) {
                    selected_response_included = true;
                }
            }
            //Check if this selection fits, accounting for an indicator that the list continues at the front if necessary
            if( start_index != 0 &&
                start_index + num_fitting != static_cast<int>( size_of_items_to_fit.size() ) ) {
                cont_marker_allowance = spacer_size + continuation_marker_size;
            }
            if( space_required < ( adjusted_allowed_size - cont_marker_allowance ) &&
                selected_response_included ) {
                if( space_required > best_fit_length ) {
                    returnVal.start = start_index;
                    returnVal.length = num_fitting;
                    best_fit_length = space_required;
                }
            }
        }
        //If a fitting option has been found, don't move to fewer entries in the list
        if( best_fit_length > 0 ) {
            break;
            //If not everything fits, need at least one continuation indicator
        } else if( num_fitting == static_cast<int>( size_of_items_to_fit.size() ) ) {
            adjusted_allowed_size -= spacer_size + continuation_marker_size;
        }
    }
    return returnVal;
}

std::pair<std::vector<std::string>, size_t> fit_tabs_to_width( const size_t max_width,
        const int current_tab,
        const std::vector<std::string> &original_tab_list )
{
    const int tab_step = 3; // Step between tabs, two for tab border
    size_t available_width = max_width - 1;
    std::pair<std::vector<std::string>, size_t> tab_list_and_offset;
    std::vector<int> tab_width;
    tab_width.reserve( original_tab_list.size() );

    for( const std::string &tab : original_tab_list ) {
        tab_width.emplace_back( utf8_width( tab, true ) );
    }

    best_fit tabs_to_print = find_best_fit_in_size( tab_width, current_tab,
                             available_width, tab_step, 1 );

    //Assemble list to return
    if( tabs_to_print.start != 0 ) { //Signify that the list continues left
        tab_list_and_offset.first.emplace_back( "<" );
    }
    for( int i = tabs_to_print.start; i < static_cast<int>( original_tab_list.size() ); ++i ) {
        if( i < tabs_to_print.start + tabs_to_print.length ) {
            tab_list_and_offset.first.emplace_back( original_tab_list[i] );
        }
    }
    if( tabs_to_print.start + tabs_to_print.length != static_cast<int>( original_tab_list.size() ) ) {
        //Signify that the list continues right
        tab_list_and_offset.first.emplace_back( ">" );
    }
    //Mark down offset
    if( tabs_to_print.start > 0 ) {
        tab_list_and_offset.second = tabs_to_print.start - 1;
    } else {
        tab_list_and_offset.second = tabs_to_print.start;
    }
    return tab_list_and_offset;
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
    scrollbar_area = inclusive_rectangle<point>( point( getbegx( window ) + offset_x_v,
                     getbegy( window ) + offset_y_v ), point( getbegx( window ) + offset_x_v,
                             getbegy( window ) + offset_y_v + viewport_size_v ) );
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
        nc_color temp_bar_color = dragging ? c_magenta_magenta : bar_color_v;

        for( int i = 0; i < slot_size; ++i ) {
            if( i >= bar_start && i < bar_end ) {
                mvwputch( window, point( offset_x_v, offset_y_v + 1 + i ), temp_bar_color, LINE_XOXO );
            } else {
                mvwputch( window, point( offset_x_v, offset_y_v + 1 + i ), slot_color_v, LINE_XOXO );
            }
        }
    }
}

scrollbar &scrollbar::set_draggable( input_context &ctxt )
{
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "CLICK_AND_DRAG" );
    ctxt.register_action( "SELECT" ); // Not directly used yet, but required for mouse-up reaction
    return *this;
}

bool scrollbar::handle_dragging( const std::string &action, const std::optional<point> &coord,
                                 int &position )
{
    if( ( action != "MOUSE_MOVE" && action != "CLICK_AND_DRAG" ) && dragging ) {
        // Stopped dragging the scrollbar
        dragging = false;

        // We don't want to accidentally select something on mouse-up after dragging the scrollbar, so if
        // there's a mouse-up event, tell the UI that we've handled it
        return action == "SELECT";
    } else  if( action == "CLICK_AND_DRAG" && coord.has_value() &&
                scrollbar_area.contains( coord.value() ) ) {
        // Started dragging the scrollbar
        dragging = true;
        return true;
    } else if( action == "MOUSE_MOVE" && coord.has_value() && dragging ) {
        // Currently dragging the scrollbar.  Clamp cursor position to scrollbar area, then interpolate
        int clamped_cursor_pos = clamp( coord->y - scrollbar_area.p_min.y, 0,
                                        scrollbar_area.p_max.y - scrollbar_area.p_min.y - 1 );
        viewport_pos_v = clamped_cursor_pos * ( content_size_v - viewport_size_v ) /
                         ( scrollbar_area.p_max.y - scrollbar_area.p_min.y - 1 );
        position = viewport_pos_v;
#if !defined(TILES)
        // Tiles builds seem to trigger "SELECT" on mouse button-up (clearing "dragging") but curses does not
        dragging = false;
#endif //TILES
        return true;
    } else {
        // Not doing anything related to the scrollbar
        return false;
    }
}

void multiline_list::activate_entry( const size_t entry_pos, const bool exclusive )
{
    if( entry_pos >= entries.size() ) {
        debugmsg( "Unable to activate entry %d of %d", entry_pos, entries.size() );
        return;
    }

    const bool cur_value = entries[entry_pos].active;

    if( exclusive ) {
        for( multiline_list_entry &entry : entries ) {
            entry.active = false;
        }
    }

    entries[entry_pos].active = exclusive ? true : !cur_value;
}

void multiline_list::add_entry( const multiline_list_entry &entry )
{
    entries.emplace_back( entry );
    if( !has_prefix || entry.prefix.empty() ) {
        entries.back().prefix.clear();
        has_prefix = false;
    }
}

void multiline_list::create_entry_prep()
{
    entries.clear();
    has_prefix = true;
}

void multiline_list::fold_entries()
{
    int available_width = getmaxx( w ) - 2; // Border/scrollbar allowance
    entry_sizes.clear();
    total_length = 0;

    std::vector<std::string> folded;
    for( multiline_list_entry &entry : entries ) {
        entry.folded_text.clear();
        if( has_prefix ) {
            // Do a prefixed list (e.g. starting with a hotkey )
            const int prefix_width = utf8_width( entry.prefix, true );
            const int fold_width = available_width - prefix_width;
            folded = foldstring( entry.entry_text, fold_width );
            for( size_t j = 0; j < folded.size(); ++j ) {
                if( j == 0 ) {
                    entry.folded_text.emplace_back( entry.prefix + folded[j] );
                } else {
                    entry.folded_text.emplace_back( std::string( prefix_width, ' ' ).append( folded[j] ) );
                }
            }
        } else {
            folded = foldstring( entry.entry_text, available_width );
            for( const std::string &line : folded ) {
                entry.folded_text.emplace_back( line );
            }
        }
        entry_sizes.emplace_back( static_cast<int>( folded.size() ) );
        total_length += folded.size();
    }
    if( !entries.empty() ) {
        // Reset entry position at end, because the resulting offset depends on entry sizes
        set_entry_pos( 0, false );
    }
}

int multiline_list::get_entry_from_offset()
{
    return get_entry_from_offset( offset_position );
}

int multiline_list::get_entry_from_offset( const int offset )
{
    int offset_for_entry = 0;
    for( int i = 0; i < static_cast<int>( entry_sizes.size() ); ++i ) {
        /* If the last entry we scroll past before the end of the list is multiple lines,
         * we need to be able to jump past it.  So, if it's a single-line entry, we can
         * return it.  Otherwise, skip past it and return the next one
         */
        if( offset_for_entry + 1 > offset ) {
            return i;
        }
        offset_for_entry += entry_sizes[i];
        if( offset_for_entry > offset ) {
            return i + 1;
        }
    }
    return static_cast<int>( entry_sizes.size() ) - 1;
}

int multiline_list::get_offset_from_entry()
{
    return get_offset_from_entry( entry_position );
}

int multiline_list::get_offset_from_entry( const int entry )
{
    int target_entry = clamp( entry, 0, static_cast<int>( entry_sizes.size() ) );
    int offset = 0;
    for( int i = 0; i < target_entry; ++i ) {
        offset += entry_sizes[i];
    }
    return offset;
}

bool multiline_list::handle_navigation( std::string &action, input_context &ctxt )
{
    std::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );
    inclusive_rectangle<point> mouseover_area( point( catacurses::getbegx( w ),
            catacurses::getbegy( w ) ), point( getmaxx( w ) + catacurses::getbegx( w ),
                    getmaxy( w ) + catacurses::getbegy( w ) ) );
    bool mouse_in_window = coord.has_value() && mouseover_area.contains( coord.value() );

    mouseover_position = -1;
    std::optional<point> local_coord = ctxt.get_coordinates_text( w );
    if( local_coord.has_value() ) {
        for( const auto &entry : entry_map ) {
            if( entry.second.contains( local_coord.value() ) ) {
                mouseover_position = entry.first;
                break;
            }
        }
    }

    if( list_sb->handle_dragging( action, coord, offset_position ) ) {
        // No action required
    } else if( action == "HOME" ) {
        set_entry_pos( 0, false );
    } else if( action == "END" ) {
        set_entry_pos( entries.size() - 1, false );
    } else if( action == "PAGE_DOWN" ) {
        set_offset_pos( offset_position + getmaxy( w ), true );
    } else if( action == "PAGE_UP" ) {
        set_offset_pos( offset_position - getmaxy( w ), true );
    } else if( action == "UP" ) {
        set_entry_pos( entry_position - 1, true );
    } else if( action == "DOWN" ) {
        set_entry_pos( entry_position + 1, true );
    } else if( action == "SCROLL_UP" && mouse_in_window ) {
        // Scroll selection, but only adjust view as if we're scrolling offset
        set_entry_pos( entry_position - 1, false );
        set_offset_pos( get_offset_from_entry( entry_position ), false );
        mouseover_delay_end = std::chrono::steady_clock::now() + scrollwheel_delay;
    } else if( action == "SCROLL_DOWN" && mouse_in_window ) {
        set_entry_pos( entry_position + 1, false );
        set_offset_pos( get_offset_from_entry( entry_position ), false );
        mouseover_delay_end = std::chrono::steady_clock::now() + scrollwheel_delay;
    } else if( action == "SELECT" && mouse_in_window ) {
        if( mouseover_position >= 0 ) {
            set_entry_pos( mouseover_position, false );
            action = "CONFIRM";
            mouseover_delay_end = std::chrono::steady_clock::now() + base_mouse_delay;
        }
    } else if( action == "MOUSE_MOVE" && mouse_in_window && local_coord.has_value() ) {
        if( std::chrono::steady_clock::now() > mouseover_delay_end ) {
            if( mouseover_position >= 0 ) {
                entry_position = mouseover_position;
            }
            const int mouse_scroll_up_pos = 0;
            const int mouse_scroll_down_pos = getmaxy( w ) - 1;
            if( local_coord.value().y <= mouse_scroll_up_pos ) {
                set_offset_pos( offset_position - 1, false );
                ++mouseover_accel_counter;
            } else if( local_coord.value().y >= mouse_scroll_down_pos ) {
                set_offset_pos( offset_position + 1, false );
                ++mouseover_accel_counter;
            } else {
                mouseover_accel_counter = 1;
            }
        }
    } else {
        return false;
    }
    return true;
}

void multiline_list::print_entries()
{
    werase( w );
    entry_map.clear();

    int ycurrent = 0;
    int current_offset = 0;
    int available_height = getmaxy( w );
    for( size_t i = 0; i < entries.size(); ++i ) {
        for( size_t j = 0; j < entries[i].folded_text.size(); ++j ) {
            if( current_offset >= offset_position && current_offset < offset_position + available_height ) {
                print_line( i, point( 2, ycurrent ), entries[i].folded_text[j] );
                ++ycurrent;
            }
            ++current_offset;
        }
    }

    list_sb->offset_x( 0 )
    .offset_y( 0 )
    .content_size( total_length )
    .viewport_pos( offset_position )
    .viewport_size( getmaxy( w ) )
    .apply( w );

    wnoutrefresh( w );
}

void multiline_list::print_line( int entry, const point &start, const std::string &text )
{
    nc_color cur_color = c_light_gray;
    std::string output_text = text;
    if( entries[entry].active ) {
        cur_color = c_light_green;
        output_text = colorize( remove_color_tags( output_text ), cur_color );
    }
    if( entry == entry_position ) {
        output_text = hilite_string( output_text );
    }
    print_colored_text( w, start, cur_color, c_light_gray, output_text );
    entry_map.emplace( entry, inclusive_rectangle<point>( start, point( start.x +
                       utf8_width( text, true ), start.y + entry_sizes[entry] - 1 ) ) );
}

void multiline_list::set_entry_pos( const int entry_pos, const bool looping = false )
{
    if( looping && !entries.empty() ) {
        int new_position = entry_pos;
        const int list_size = static_cast<int>( entries.size() );
        if( new_position < 0 ) {
            // Ensure we have a positive position index by adding a multiple of the list_size to it
            new_position += list_size * ( ( std::abs( new_position ) / list_size ) + 1 );
        }
        entry_position = new_position % static_cast<int>( entries.size() );
    } else {
        entry_position = clamp( entry_pos, 0, static_cast<int>( entries.size() ) - 1 );
    }
    int available_space = getmaxy( w ) - entry_sizes[entry_position];
    set_offset_pos( get_offset_from_entry() - available_space / 2, false );
    mouseover_delay_end = std::chrono::steady_clock::now() + base_mouse_delay / mouseover_accel_counter;
}

void multiline_list::set_offset_pos( const int offset_pos, const bool update_selection )
{
    // Ensure offset is above 0 and below the maximum
    const int max_offset = total_length - getmaxy( w );
    if( max_offset <= 0 ) {
        // Can't scroll offset, so just scroll entry
        offset_position = 0;
        if( update_selection ) {
            if( offset_pos > offset_position ) {
                set_entry_pos( entry_position + 5, false );
            } else {
                set_entry_pos( entry_position - 5, false );
            }
        }
    } else {
        if( update_selection ) {
            entry_position = get_entry_from_offset( clamp( offset_pos, 0, total_length ) );
            // The offset position might be slightly offset from the actual offset from that entry, so adjust
            set_offset_pos( get_offset_from_entry(), false );
        } else {
            offset_position = clamp( offset_pos, 0, max_offset );
        }
    }
    mouseover_delay_end = std::chrono::steady_clock::now() + base_mouse_delay / mouseover_accel_counter;
}

void multiline_list::set_up_navigation( input_context &ctxt )
{
    list_sb->set_draggable( ctxt );
    ctxt.register_updown();
    ctxt.register_action( "END" );
    ctxt.register_action( "HOME" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SELECT" );
}

void scrolling_text_view::set_text( const std::string &text, const bool scroll_to_top )
{
    text_ = foldstring( text, text_width() );
    if( scroll_to_top ) {
        offset_ = 0;
    } else {
        offset_ = max_offset();
    }
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
        text_view_scrollbar.content_size( text_.size() ).
        viewport_pos( offset_ ).
        viewport_size( height ).
        scroll_to_last( false ).
        apply( w_ );
    } else {
        text_view_scrollbar = scrollbar();
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

bool scrolling_text_view::handle_navigation( const std::string &action, input_context &ctxt )
{
    std::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );
    inclusive_rectangle<point> mouseover_area( point( getbegx( w_ ), getbegy( w_ ) ),
            point( getmaxx( w_ ) + getbegx( w_ ), getmaxy( w_ ) + getbegy( w_ ) ) );
    bool mouse_in_window = coord.has_value() && mouseover_area.contains( coord.value() );

    if( text_view_scrollbar.handle_dragging( action, coord, offset_ ) ) {
        // No action required, the scrollbar has handled it
    } else if( action == scroll_up_action || ( action == "SCROLL_UP" && mouse_in_window ) ) {
        scroll_up();
    } else if( action == scroll_down_action || ( action == "SCROLL_DOWN" && mouse_in_window ) ) {
        scroll_down();
    } else if( paging_enabled && action == "PAGE_UP" ) {
        page_up();
    } else if( paging_enabled && action == "PAGE_DOWN" ) {
        page_down();
    } else {
        return false;
    }
    return true;
}

void scrolling_text_view::set_up_navigation( input_context &ctxt,
        const scrolling_key_scheme scheme,
        const bool enable_paging )
{
    ctxt.register_action( "CLICK_AND_DRAG" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "SELECT" );
    if( scheme != scrolling_key_scheme::no_scheme ) {
        if( scheme == scrolling_key_scheme::angle_bracket_scroll ) {
            scroll_up_action = "SCROLL_INFOBOX_UP";
            scroll_down_action = "SCROLL_INFOBOX_DOWN";
        } else if( scheme == scrolling_key_scheme::arrow_scroll ) {
            scroll_up_action = "UP";
            scroll_down_action = "DOWN";
        }
        ctxt.register_action( scroll_up_action );
        ctxt.register_action( scroll_down_action );
    }
    if( enable_paging ) {
        paging_enabled = true;
        ctxt.register_action( "PAGE_UP" );
        ctxt.register_action( "PAGE_DOWN" );
    }
    text_view_scrollbar.set_draggable( ctxt );
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
        } else if( iStartPos + iContentHeight > iNumEntries ) {
            iStartPos = iNumEntries - iContentHeight;
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
#if defined(_WIN32)
    // For unknown reason, vsnprintf on Windows does not seem to support positional arguments (e.g. "%1$s")
    va_list args;
    va_start( args, format );

    va_list args_copy_1;
    va_copy( args_copy_1, args );
    // Return value of _vscprintf_p does not include the '\0' terminator
    const int characters = _vscprintf_p( format, args_copy_1 ) + 1;
    va_end( args_copy_1 );

    std::vector<char> buffer( characters, '\0' );
    va_list args_copy_2;
    va_copy( args_copy_2, args );
    _vsprintf_p( &buffer[0], characters, format, args_copy_2 );
    va_end( args_copy_2 );

    va_end( args );
    return std::string( &buffer[0] );
#else
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
        const int result = vsnprintf( buffer.data(), buffer_size, format, args_copy );
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
    return std::string( buffer.data() );
#endif
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

void replace_keybind_tag( std::string &input )
{
    std::string keybind_tag_start = "<keybind:";
    size_t keybind_length = keybind_tag_start.length();
    std::string keybind_tag_end = ">";

    size_t pos = input.find( keybind_tag_start );
    while( pos != std::string::npos ) {
        size_t pos_end = input.find( keybind_tag_end, pos );
        if( pos_end == std::string::npos ) {
            debugmsg( "Mismatched keybind tag in string: '%s'", input );
            break;
        }
        size_t pos_keybind = pos + keybind_length;
        std::string keybind_full = input.substr( pos_keybind, pos_end - pos_keybind );
        std::string keybind = keybind_full;

        size_t pos_category_split = keybind_full.find( ':' );

        std::string category = "DEFAULTMODE";
        if( pos_category_split != std::string::npos ) {
            category = keybind_full.substr( 0, pos_category_split );
            keybind = keybind_full.substr( pos_category_split + 1 );
        }
        input_context ctxt( category );

        std::string keybind_desc;
        std::vector<input_event> keys = ctxt.keys_bound_to( keybind, -1, false, false );
        if( keys.empty() ) { // Display description for unbound keys
            keybind_desc = colorize( '<' + ctxt.get_desc( keybind ) + '>', c_red );

            if( !ctxt.is_registered_action( keybind ) ) {
                debugmsg( "Invalid/Missing <keybind>: '%s'", keybind_full );
            }
        } else {
            keybind_desc = enumerate_as_string( keys.begin(), keys.end(), []( const input_event & k ) {
                return colorize( '\'' + k.long_description() + '\'', c_yellow );
            }, enumeration_conjunction::or_ );
        }
        std::string to_replace = string_format( "%s%s%s", keybind_tag_start, keybind_full,
                                                keybind_tag_end );
        replace_substring( input, to_replace, keybind_desc, true );

        pos = input.find( keybind_tag_start );
    }
}

void replace_substring( std::string &input, const std::string &substring,
                        const std::string &replacement, bool all )
{
    std::size_t find_after = 0;
    std::size_t pos = 0;
    const std::size_t pattern_length = substring.length();
    const std::size_t replacement_length = replacement.length();
    while( ( pos = input.find( substring, find_after ) ) != std::string::npos ) {
        input.replace( pos, pattern_length, replacement );
        find_after = pos + replacement_length;
        if( !all ) {
            break;
        }
    }
}

std::string uppercase_first_letter( const std::string &str )
{
    std::wstring wstr = utf8_to_wstr( str );
    wstr[0] = towupper( wstr[0] );
    return wstr_to_utf8( wstr );
}

std::string lowercase_first_letter( const std::string &str )
{
    std::wstring wstr = utf8_to_wstr( str );
    wstr[0] = towlower( wstr[0] );
    return wstr_to_utf8( wstr );
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
get_bar( const float cur, const float max,
         const int width, const bool extra_resolution,
         const std::vector<nc_color> &colors )
{
    std::string result;
    const float status = clamp( cur / max, 0.0f, 1.0f );
    const float sw = status * width;

    nc_color col;
    if( !std::isfinite( status ) || colors.empty() ) {
        col = c_red_red;
    } else {
        int ind = std::floor( ( 1.0 - status ) * ( colors.size() - 1 ) + 0.5 );
        ind = clamp<int>( ind, 0, colors.size() - 1 );
        col = colors[ind];
    }
    if( !std::isfinite( sw ) || sw <= 0 ) {
        result.clear();
    } else {
        const int pipes = static_cast<int>( sw );
        if( pipes > 0 ) {
            result += std::string( pipes, '|' );
        }
        if( extra_resolution && sw - pipes >= 0.5 ) {
            result += "\\";
        } else if( pipes == 0 ) {
            result = ":";
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

std::pair<std::string, nc_color> rad_badge_color( const int rad )
{
    using pair_t = std::pair<const int, std::pair<std::string, nc_color>>;

    static const std::array<pair_t, 6> values = {{
            pair_t { 0,   { translate_marker( "green" ),  c_white_green } },
            pair_t { 30,  { translate_marker( "blue" ),   h_white       } },
            pair_t { 60,  { translate_marker( "yellow" ), i_yellow      } },
            pair_t { 120, { translate_marker( "orange" ), c_red_yellow  } },
            pair_t { 240, { translate_marker( "red" ),    c_red_red     } },
            pair_t { 500, { translate_marker( "black" ),  c_pink        } }
        }
    };

    unsigned i = 0;
    for( ; i < values.size(); i++ ) {
        if( rad <= values[i].first ) {
            break;
        }
    }
    i = i == values.size() ? i - 1 : i;

    return std::pair<std::string, nc_color>( _( values[i].second.first ), values[i].second.second );
}

std::string formatted_hotkey( const std::string &hotkey, const nc_color text_color )
{
    return colorize( hotkey, ACTIVE_HOTKEY_COLOR ).append( colorize( ": ", text_color ) );
}

std::string get_labeled_bar( const double val, const int width, const std::string &label,
                             char c )
{
    const std::array<std::pair<double, char>, 1> ratings =
    {{ std::make_pair( 1.0, c ) }};
    return get_labeled_bar( val, width, label, ratings.begin(), ratings.end() );
}

template<typename BarIterator>
inline std::string get_labeled_bar( const double val, const int width, const std::string &label,
                                    BarIterator begin, BarIterator end )
{
    return get_labeled_bar<BarIterator>( val, width, label, begin, end, []( BarIterator it,
    int seg_width ) -> std::string {
        return std::string( seg_width, std::get<1>( *it ) );} );
}

using RatingVector = std::vector<std::tuple<double, char, std::string>>;

template std::string get_labeled_bar<RatingVector::iterator>( const double val, const int width,
        const std::string &label,
        RatingVector::iterator begin, RatingVector::iterator end,
        std::function<std::string( RatingVector::iterator, int )> printer );

template<typename BarIterator>
std::string get_labeled_bar( const double val, const int width, const std::string &label,
                             BarIterator begin, BarIterator end, std::function<std::string( BarIterator, int )> printer )
{
    std::string result;

    result.reserve( width );
    if( !label.empty() ) {
        result += label;
        result += ' ';
    }
    const int bar_width = width - utf8_width( result ) - 2; // - 2 for the brackets

    result += '[';
    if( bar_width > 0 ) {
        int used_width = 0;
        for( BarIterator it( begin ); it != end; ++it ) {
            const double factor = std::min( 1.0, std::max( 0.0, std::get<0>( *it ) * val ) );
            const int seg_width = static_cast<int>( factor * bar_width ) - used_width;

            if( seg_width <= 0 ) {
                continue;
            }
            used_width += seg_width;

            result += printer( it, seg_width );
        }
        result.insert( result.end(), bar_width - used_width, ' ' );
    }
    result += ']';

    return result;
}

/**
 * Inserts a table into a window, with data right-aligned.
 * @param pad Reduce table width by padding left side.
 * @param line Line to insert table.
 * @param columns Number of columns. Can be 1.
 * @param FG Default color of table text.
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
    const int col_width = ( width - pad ) / columns;
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

std::string satiety_bar( const int calpereffv )
{
    // Arbitrary max value we will cap our vague display to. Will be lower than the actual max value, but scaling fixes that.
    constexpr float max_cal_per_effective_vol = 1500.0f;
    // Scaling the values.
    const float scaled_max = std::sqrt( max_cal_per_effective_vol );
    const float scaled_cal = std::sqrt( calpereffv );
    const std::pair<std::string, nc_color> nourishment_bar = get_bar(
                scaled_cal, scaled_max, 5, true );
    // Colorize the bar.
    std::string result = colorize( nourishment_bar.first, nourishment_bar.second );
    // Pad to 5 characters with dots.
    const int width = utf8_width( nourishment_bar.first );
    if( width < 5 ) {
        result += std::string( 5 - width, '.' );
    }
    return result;
}

std::string healthy_bar( const int healthy )
{
    if( healthy > 3 ) {
        return "<good>+++</good>";
    } else if( healthy > 0 ) {
        return "<good>+</good>";
    } else if( healthy < -3 ) {
        return "<bad>!!!</bad>";
    } else if( healthy < 0 ) {
        return "<bad>-</bad>";
    } else {
        return "";
    }
}

scrollingcombattext::cSCT::cSCT( const point &p_pos, const direction p_oDir,
                                 const std::string &p_sText, const game_message_type p_gmt,
                                 const std::string &p_sText2, const game_message_type p_gmt2,
                                 const std::string &p_sType )
{
    pos = p_pos;
    sType = p_sType;
    oDir = p_oDir;

    iso_mode = g->is_tileset_isometric();

    // translate from player relative to screen relative direction
    oUp = iso_mode ? direction::NORTHEAST : direction::NORTH;
    oUpRight = iso_mode ? direction::EAST : direction::NORTHEAST;
    oRight = iso_mode ? direction::SOUTHEAST : direction::EAST;
    oDownRight = iso_mode ? direction::SOUTH : direction::SOUTHEAST;
    oDown = iso_mode ? direction::SOUTHWEST : direction::SOUTH;
    oDownLeft = iso_mode ? direction::WEST : direction::SOUTHWEST;
    oLeft = iso_mode ? direction::NORTHWEST : direction::WEST;
    oUpLeft = iso_mode ? direction::NORTH : direction::NORTHWEST;

    point pairDirXY = displace_XY( oDir );

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
    // TODO: A non-hack
    if( test_mode ) {
        return;
    }
    if( get_option<bool>( "ANIMATION_SCT" ) ) {

        int iCurStep = 0;

        bool tiled = false;
        bool iso_mode = g->is_tileset_isometric();

#if defined(TILES)
        tiled = use_tiles;
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
                p_oDir = one_in( 2 ) ? ( iso_mode ? direction::EAST : direction::NORTHEAST ) :
                         ( iso_mode ? direction::SOUTH : direction::SOUTHEAST );

            } else if( p_oDir == ( iso_mode ? direction::NORTHWEST : direction::WEST ) ) {
                p_oDir = one_in( 2 ) ? ( iso_mode ? direction::NORTH : direction::NORTHWEST ) :
                         ( iso_mode ? direction::WEST : direction::SOUTHWEST );
            }
        }

        // in tiles, SCT that scroll downwards are inserted at the beginning of the vector to prevent
        // oversize ASCII tiles overdrawing messages below them.
        if( tiled && ( p_oDir == direction::SOUTHWEST || p_oDir == direction::SOUTH ||
                       p_oDir == ( iso_mode ? direction::WEST : direction::SOUTHEAST ) ) ) {

            //Message offset: multiple impacts in the same direction in short order overriding prior messages (mostly turrets)
            for( scrollingcombattext::cSCT &iter : vSCT ) {
                if( iter.getDirection() == p_oDir && ( iter.getStep() + iter.getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter.advanceStepOffset();
                }
            }
            vSCT.insert( vSCT.begin(), cSCT( pos, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2,
                                             p_sType ) );

        } else {
            //Message offset: this time in reverse.
            for( std::vector<cSCT>::reverse_iterator iter = vSCT.rbegin(); iter != vSCT.rend(); ++iter ) {
                if( iter->getDirection() == p_oDir && ( iter->getStep() + iter->getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter->advanceStepOffset();
                }
            }
            vSCT.emplace_back( pos, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2, p_sType );
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

// find substring (case insensitive)
int ci_find_substr( const std::string_view str1, const std::string_view str2,
                    const std::locale &loc )
{
    std::string_view::const_iterator it =
        std::search( str1.begin(), str1.end(), str2.begin(), str2.end(),
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
* Convert and format volume.
*/
std::string format_volume( const units::volume &volume )
{
    return format_volume( volume, 0, nullptr, nullptr );
}

/**
* Convert, clamp and format volume,
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
    return width < EVEN_MINIMUM_TERM_WIDTH ? EVEN_MINIMUM_TERM_WIDTH : width;
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
