#pragma once
#ifndef CATA_SRC_OUTPUT_H
#define CATA_SRC_OUTPUT_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <iterator>
#include <locale>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "catacharset.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "item.h"
#include "line.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"

struct input_event;

namespace catacurses
{
class window;

using chtype = int;
} // namespace catacurses

//      LINE_NESW  - X for on, O for off
#define LINE_XOXO 4194424 // '|'   Vertical line. ncurses: ACS_VLINE; Unicode: U+2502
#define LINE_OXOX 4194417 // '-'   Horizontal line. ncurses: ACS_HLINE; Unicode: U+2500
#define LINE_XXOO 4194413 // '|_'  Lower left corner. ncurses: ACS_LLCORNER; Unicode: U+2514
#define LINE_OXXO 4194412 // '|^'  Upper left corner. ncurses: ACS_ULCORNER; Unicode: U+250C
#define LINE_OOXX 4194411 // '^|'  Upper right corner. ncurses: ACS_URCORNER; Unicode: U+2510
#define LINE_XOOX 4194410 // '_|'  Lower right corner. ncurses: ACS_LRCORNER; Unicode: U+2518
#define LINE_XXXO 4194420 // '|-'  Tee pointing right. ncurses: ACS_LTEE; Unicode: U+251C
#define LINE_XXOX 4194422 // '_|_' Tee pointing up. ncurses: ACS_BTEE; Unicode: U+2534
#define LINE_XOXX 4194421 // '-|'  Tee pointing left. ncurses: ACS_RTEE; Unicode: U+2524
#define LINE_OXXX 4194423 // '^|^' Tee pointing down. ncurses: ACS_TTEE; Unicode: U+252C
#define LINE_XXXX 4194414 // '-|-' Large Plus or cross over. ncurses: ACS_PLUS; Unicode: U+253C

#define LINE_XOXO_C 0xa0
#define LINE_OXOX_C 0xa1
#define LINE_XXOO_C 0xa2
#define LINE_OXXO_C 0xa3
#define LINE_OOXX_C 0xa4
#define LINE_XOOX_C 0xa5
#define LINE_XXXO_C 0xa6
#define LINE_XXOX_C 0xa7
#define LINE_XOXX_C 0xa8
#define LINE_OXXX_C 0xa9
#define LINE_XXXX_C 0xaa

#define LINE_XOXO_S "│" // '|'   Vertical line. ncurses: ACS_VLINE; Unicode: U+2502
#define LINE_OXOX_S "─" // '-'   Horizontal line. ncurses: ACS_HLINE; Unicode: U+2500
#define LINE_XXOO_S "└" // '|_'  Lower left corner. ncurses: ACS_LLCORNER; Unicode: U+2514
#define LINE_OXXO_S "┌" // '|^'  Upper left corner. ncurses: ACS_ULCORNER; Unicode: U+250C
#define LINE_OOXX_S "┐" // '^|'  Upper right corner. ncurses: ACS_URCORNER; Unicode: U+2510
#define LINE_XOOX_S "┘" // '_|'  Lower right corner. ncurses: ACS_LRCORNER; Unicode: U+2518
#define LINE_XXXO_S "├" // '|-'  Tee pointing right. ncurses: ACS_LTEE; Unicode: U+251C
#define LINE_XXOX_S "┴" // '_|_' Tee pointing up. ncurses: ACS_BTEE; Unicode: U+2534
#define LINE_XOXX_S "┤" // '-|'  Tee pointing left. ncurses: ACS_RTEE; Unicode: U+2524
#define LINE_OXXX_S "┬" // '^|^' Tee pointing down. ncurses: ACS_TTEE; Unicode: U+252C
#define LINE_XXXX_S "┼" // '-|-' Large Plus or cross over. ncurses: ACS_PLUS; Unicode: U+253C

#define LINE_XOXO_UNICODE 0x2502
#define LINE_OXOX_UNICODE 0x2500
#define LINE_XXOO_UNICODE 0x2514
#define LINE_OXXO_UNICODE 0x250C
#define LINE_OOXX_UNICODE 0x2510
#define LINE_XOOX_UNICODE 0x2518
#define LINE_XXXO_UNICODE 0x251C
#define LINE_XXOX_UNICODE 0x2534
#define LINE_XOXX_UNICODE 0x2524
#define LINE_OXXX_UNICODE 0x252C
#define LINE_XXXX_UNICODE 0x253C

// Supports line drawing
inline std::string string_from_int( const catacurses::chtype ch )
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
    char buffer[2] = { static_cast<char>( charcode ), '\0' };
    return buffer;
}

// a consistent border color
#define BORDER_COLOR c_light_gray

// Display data
extern int TERMX; // width available for display
extern int TERMY; // height available for display
extern int POSX; // X position of '@' inside terrain window
extern int POSY; // Y position of '@' inside terrain window
extern int TERRAIN_WINDOW_WIDTH; // width of terrain window
extern int TERRAIN_WINDOW_HEIGHT; // height of terrain window
extern int TERRAIN_WINDOW_TERM_WIDTH; // width of terrain window in terminal characters
extern int TERRAIN_WINDOW_TERM_HEIGHT; // same for height
extern int FULL_SCREEN_WIDTH; // width of "full screen" popups
extern int FULL_SCREEN_HEIGHT; // height of "full screen" popups
extern int OVERMAP_WINDOW_WIDTH; // width of overmap window
extern int OVERMAP_WINDOW_HEIGHT; // height of overmap window
extern int OVERMAP_LEGEND_WIDTH; // width of overmap window legend

nc_color msgtype_to_color( game_message_type type, bool bOldMsg = false );

/**
 * @anchor color_tags
 * @name color tags
 *
 * Most print function have only one color parameter (or none at all), therefor they would
 * print the whole text in one color. To print some parts of a text in a different color,
 * one would have to write separate calls to the print functions.
 *
 * Color tags allow embedding coloring instructions directly into the text, which allows
 * a single call to the print function with the full text. The color tags are removed
 * (not printed), but the text inside the tags is printed with a specific color.
 *
 * Example: Print "You need a tool to do this." in white and the word "tool" in red.
 * \code
 *    wprintz( w, c_white, "You need a " );
 *    wprintz( w, c_red, "tool" );
 *    wprintz( w, c_white, " to do this." );
 * \endcode
 * Same code with color tags:
 * \code
 *    print_colored_text( w, c_white, "You need a <color_red>tool</color> to do this." );
 * \endcode
 *
 * Color tags must appear in pairs, first `<color_XXX>`, followed by text, followed by `</color>`.
 * The text in between is colored according to the color `XXX`. `XXX` must be a valid color name,
 * see @ref color_from_string. Color tags do *not* stack, the text inside the color tags should
 * not contain any other color tags.
 *
 * Functions that handle color tags should contain a note about this in their documentation. If
 * they have no such note, they probably don't handle color tags (which means they just print the
 * string as is).
 *
 * Note: use @ref colorize to add color tags to a string.
 * \code
 *    nc_color color = ...;
 *    text = colorize( "some text", color );
 * \endcode
 *
 * One can use @ref utf8_width with the second parameter set to `true` to determine the printed
 * length of a string containing color tags. The parameter instructs `utf8_width` to ignore the
 * color tags. For example `utf8_width("<color_red>text</color>")` would return 23, but
 * `utf8_width("<color_red>text</color>", true)` returns 4 (the length of "text").
 */

/*@{*/
/**
 * Removes the color tags from the input string. This might be required when the string is to
 * be used for functions that don't handle color tags.
 */
std::string remove_color_tags( const std::string &s );
/*@}*/

/**
 * Split the input text into separate lines and wrap long lines. Each resulting line is at most
 * `width` console cells long.
 * The functions handles @ref color_tags. Color tags are added to the resulting lines so each
 * line can be independently printed.
 * @return A vector of lines, it may contain empty strings. Each entry is at most `width`
 * console cells width.
 */
std::vector<std::string> foldstring( const std::string &str, int width, char split = ' ' );

/**
 * Print text with embedded @ref color_tags, x, y are in curses system.
 * The text is not word wrapped, but may automatically be wrapped on new line characters or
 * when it reaches the border of the window (both is done by the curses system).
 * If the text contains no color tags, it's equivalent to a simple mvprintz.
 *
 * @param w Window we are drawing in
 * @param p Curses-style coordinates to print text at.
 * @param text The text to print.
 * @param cur_color The current color (could have been set by a previously encountered color tag),
 * change to a color according to the color tags that are in the text.
 * @param base_color Base color that is used outside of any color tag.
 **/
void print_colored_text( const catacurses::window &w, const point &p, nc_color &cur_color,
                         const nc_color &base_color, const std::string &text );
/**
 * Print word wrapped text (with @ref color_tags) into the window.
 *
 * @param w Window we are printing in
 * @param begin_line Line in the word wrapped text that is printed first (lines before that are not printed at all).
 * @param text Text to print
 * @param base_color Color used outside of any color tags.
 * @param scroll_msg Optional, can be empty. If not empty and the text does not fit the window, the string is printed
 * on the last line (in light green), it should show how to scroll the text.
 * @return The maximal scrollable offset ([number of lines to be printed] - [lines available in the window]).
 * This allows the caller to restrict the begin_line number on future calls / when modified by the user.
 */
int print_scrollable( const catacurses::window &w, int begin_line, const std::string &text,
                      const nc_color &base_color, const std::string &scroll_msg );
/**
 * Fold and print text in the given window. The function handles @ref color_tags and
 * uses them while printing.
 *
 * @param w Window we are printing in
 * @param begin The (row, column) index on which to start.
 * @param width The width used to fold the text (see @ref foldstring). `width + begin_y` should be
 * less than the window width, otherwise the lines will be wrapped by the curses system, which
 * defeats the purpose of using `foldstring`.
 * @param base_color The initially used color. This can be overridden using color tags.
 * @param text Actual message to print
 * @param split Character after string is folded
 * @return The number of lines of the formatted text (after folding). This may be larger than
 * the height of the window.
 */
int fold_and_print( const catacurses::window &w, const point &begin, int width,
                    const nc_color &base_color, const std::string &text, char split = ' ' );
/**
 * Same as other @ref fold_and_print, but does string formatting via @ref string_format.
 */
template<typename ...Args>
inline int fold_and_print( const catacurses::window &w, const point &begin,
                           const int width, const nc_color &base_color,
                           const char *const mes, Args &&... args )
{
    return fold_and_print( w, begin, width, base_color, string_format( mes,
                           std::forward<Args>( args )... ) );
}
/**
 * Like @ref fold_and_print, but starts the output with the N-th line of the folded string.
 * This can be used for scrolling large texts. Parameters have the same meaning as for
 * @ref fold_and_print, the function therefor handles @ref color_tags correctly.
 *
 * @param w Window we are printing in
 * @param begin The (row,column) index on which to start.
 * @param width The width used to fold the text (see @ref foldstring). `width + begin_y` should be
 * @param begin_line The index of the first line (of the folded string) that is to be printed.
 * The function basically removes all lines before this one and prints the remaining lines
 * with `fold_and_print`.
 * @param base_color The initially used color. This can be overridden using color tags.
 * @param text Actual message to print
 * @return Same as `fold_and_print`: the number of lines of the text (after folding). This is
 * always the same value, regardless of `begin_line`, it can be used to determine the maximal
 * value for `begin_line`.
 */
int fold_and_print_from( const catacurses::window &w, const point &begin, int width,
                         int begin_line, const nc_color &base_color, const std::string &text );
/**
 * Same as other @ref fold_and_print_from, but does formatting via @ref string_format.
 */
template<typename ...Args>
inline int fold_and_print_from( const catacurses::window &w, const point &begin,
                                const int width, const int begin_line, const nc_color &base_color,
                                const char *const mes, Args &&... args )
{
    return fold_and_print_from( w, begin, width, begin_line, base_color,
                                string_format( mes, std::forward<Args>( args )... ) );
}
/**
 * Prints a single line of text. The text is automatically trimmed to fit into the given
 * width. The function handles @ref color_tags correctly.
 *
 * @param w Window we are printing in
 * @param begin The coordinates of line start (curses coordinates)
 * @param width Maximal width of the printed line, if the text is longer, it is cut off.
 * @param base_color The initially used color. This can be overridden using color tags.
 * @param text Actual message to print
 */
void trim_and_print( const catacurses::window &w, const point &begin, int width,
                     nc_color base_color, const std::string &text );
std::string trim_by_length( const std::string &text, int width );
template<typename ...Args>
inline void trim_and_print( const catacurses::window &w, const point &begin,
                            const int width, const nc_color base_color, const char *const mes, Args &&... args )
{
    return trim_and_print( w, begin, width, base_color, string_format( mes,
                           std::forward<Args>( args )... ) );
}
void center_print( const catacurses::window &w, int y, const nc_color &FG,
                   const std::string &text );
int right_print( const catacurses::window &w, int line, int right_indent,
                 const nc_color &FG, const std::string &text );
void insert_table( const catacurses::window &w, int pad, int line, int columns,
                   const nc_color &FG, const std::string &divider, bool r_align,
                   const std::vector<std::string> &data );
void scrollable_text( const catacurses::window &w, const std::string &title,
                      const std::string &text );
void scrollable_text( const std::function<catacurses::window()> &init_window,
                      const std::string &title, const std::string &text );
std::string name_and_value( const std::string &name, int value, int field_width );
std::string name_and_value( const std::string &name, const std::string &value, int field_width );

void wputch( const catacurses::window &w, nc_color FG, int ch );
// Using int ch is deprecated, use an UTF-8 encoded string instead
void mvwputch( const catacurses::window &w, const point &p, nc_color FG, int ch );
void mvwputch( const catacurses::window &w, const point &p, nc_color FG, const std::string &ch );
// Using int ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_inv( const catacurses::window &w, const point &p, nc_color FG, int ch );
void mvwputch_inv( const catacurses::window &w, const point &p, nc_color FG,
                   const std::string &ch );
// Using int ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_hi( const catacurses::window &w, const point &p, nc_color FG, int ch );
void mvwputch_hi( const catacurses::window &w, const point &p, nc_color FG, const std::string &ch );

void mvwprintz( const catacurses::window &w, const point &p, const nc_color &FG,
                const std::string &text );
template<typename ...Args>
inline void mvwprintz( const catacurses::window &w, const point &p, const nc_color &FG,
                       const char *const mes, Args &&... args )
{
    mvwprintz( w, p, FG, string_format( mes, std::forward<Args>( args )... ) );
}

void wprintz( const catacurses::window &w, const nc_color &FG, const std::string &text );
template<typename ...Args>
inline void wprintz( const catacurses::window &w, const nc_color &FG, const char *const mes,
                     Args &&... args )
{
    wprintz( w, FG, string_format( mes, std::forward<Args>( args )... ) );
}

void draw_custom_border(
    const catacurses::window &w, catacurses::chtype ls = 1, catacurses::chtype rs = 1,
    catacurses::chtype ts = 1, catacurses::chtype bs = 1, catacurses::chtype tl = 1,
    catacurses::chtype tr = 1, catacurses::chtype bl = 1, catacurses::chtype br = 1,
    const nc_color &FG = BORDER_COLOR, const point &pos = point_zero, int height = 0, int width = 0 );
void draw_border( const catacurses::window &w, nc_color border_color = BORDER_COLOR,
                  const std::string &title = "", nc_color title_color = c_light_red );
void draw_border_below_tabs( const catacurses::window &w, nc_color border_color = BORDER_COLOR );

class border_helper
{
    public:
        class border_info
        {
            public:
                void set( const point &pos, const point &size );

                // Prevent accidentally copying the return value from border_helper::add_border
                border_info( const border_info & ) = delete;
                border_info( border_info && ) = default;
                border_info &operator=( const border_info & ) = delete;
                border_info &operator=( border_info && ) = default;

                friend class border_helper;
            private:
                border_info( border_helper &helper );

                point pos;
                point size;
                std::reference_wrapper<border_helper> helper;
        };

        border_info &add_border();
        void draw_border( const catacurses::window &win );

        friend class border_info;
    private:
        struct border_connection {
            bool top = false;
            bool right = false;
            bool bottom = false;
            bool left = false;

            int as_curses_line() const;
        };
        cata::optional<std::map<point, border_connection>> border_connection_map;

        std::forward_list<border_info> border_info_list;
};

std::string word_rewrap( const std::string &ins, int width, uint32_t split = ' ' );
std::vector<size_t> get_tag_positions( const std::string &s );
std::vector<std::string> split_by_color( const std::string &s );

bool query_yn( const std::string &text );
template<typename ...Args>
inline bool query_yn( const char *const msg, Args &&... args )
{
    return query_yn( string_format( msg, std::forward<Args>( args )... ) );
}

bool query_int( int &result, const std::string &text );
template<typename ...Args>
inline bool query_int( int &result, const char *const msg, Args &&... args )
{
    return query_int( result, string_format( msg, std::forward<Args>( args )... ) );
}

std::vector<std::string> get_hotkeys( const std::string &s );

/**
 * @name Popup windows
 *
 * Each function displays a popup (above all other windows) with the given (formatted)
 * text. The popup function with the flags parameters does all the work, the other functions
 * call it with specific flags. The function can be called with a bitwise combination of flags.
 *
 * The functions return the key (taken from @ref getch) that was entered by the user.
 *
 * The message is a printf-like string. It may contain @ref color_tags, which are used while printing.
 *
 * - PF_GET_KEY (ignored when combined with PF_NO_WAIT) cancels the popup on *any* user input.
 *   Without the flag the popup is only canceled when the user enters new-line, Space and Escape.
 *   This flag is passed by @ref popup_getkey.
 * - PF_ON_TOP makes the window appear on the top of the screen (at the upper most row). Without
 *   this flag, the popup is centered on the screen.
 *   The flag is passed by @ref popup_top.
 * - PF_FULLSCREEN makes the popup window as big as the whole screen.
 *   This flag is passed by @ref full_screen_popup.
 * - PF_NONE is a placeholder for none of the above flags.
 *
 */
/*@{*/
enum PopupFlags {
    PF_NONE        = 0,
    PF_GET_KEY     = 1 << 0,
    PF_ON_TOP      = 1 << 2,
    PF_FULLSCREEN  = 1 << 3,
};

template<typename ...Args>
inline int popup_getkey( const char *const mes, Args &&... args )
{
    return popup( string_format( mes, std::forward<Args>( args )... ), PF_GET_KEY );
}
template<typename ...Args>
inline void popup_top( const char *const mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_ON_TOP );
}
template<typename ...Args>
inline void popup( const char *mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_NONE );
}
int popup( const std::string &text, PopupFlags flags = PF_NONE );
template<typename ...Args>
inline void full_screen_popup( const char *mes, Args &&... args )
{
    popup( string_format( mes, std::forward<Args>( args )... ), PF_FULLSCREEN );
}

/*@}*/
std::string format_item_info( const std::vector<iteminfo> &vItemDisplay,
                              const std::vector<iteminfo> &vItemCompare );

// the extra data that item_info needs to draw
struct item_info_data {
    private:
        std::string sItemName;
        std::string sTypeName;
        std::vector<iteminfo> vItemDisplay;
        std::vector<iteminfo> vItemCompare;
        int selected = 0;

    public:

        item_info_data() = default;

        item_info_data( const std::string &sItemName, const std::string &sTypeName,
                        const std::vector<iteminfo> &vItemDisplay, const std::vector<iteminfo> &vItemCompare )
            : sItemName( sItemName ), sTypeName( sTypeName ),
              vItemDisplay( vItemDisplay ), vItemCompare( vItemCompare ),
              selected( 0 ), ptr_selected( &selected ) {}

        item_info_data( const std::string &sItemName, const std::string &sTypeName,
                        const std::vector<iteminfo> &vItemDisplay, const std::vector<iteminfo> &vItemCompare,
                        int &ptr_selected )
            : sItemName( sItemName ), sTypeName( sTypeName ),
              vItemDisplay( vItemDisplay ), vItemCompare( vItemCompare ),
              ptr_selected( &ptr_selected ) {}

        const std::string &get_item_name() const {
            return sItemName;
        }
        const std::string &get_type_name() const {
            return sTypeName;
        }
        const std::vector<iteminfo> &get_item_display() const {
            return vItemDisplay;
        }
        const std::vector<iteminfo> &get_item_compare() const {
            return vItemCompare;
        }

        int *ptr_selected = &selected;
        bool without_getch = false;
        bool without_border = false;
        bool handle_scrolling = false;
        bool any_input = true;
        bool scrollbar_left = true;
        bool use_full_win = false;
        unsigned int padding = 1;
};

input_event draw_item_info( const catacurses::window &win, item_info_data &data );

input_event draw_item_info( int iLeft, int iWidth, int iTop, int iHeight, item_info_data &data );

input_event draw_item_info( const std::function<catacurses::window()> &init_window,
                            item_info_data &data );

enum class item_filter_type : int {
    FIRST = 1, // used for indexing into tables
    FILTER = 1,
    LOW_PRIORITY = 2,
    HIGH_PRIORITY = 3
};
/**
 * Write some tips (such as precede items with - to exclude them) onto the window.
 *
 * @param win Window we are drawing in
 * @param starty Where to start relative to the top of the window.
 * @param height Every row from starty to starty + height - 1 will be cleared before printing the rules.
 * @param type Filter to use when drawing
*/
void draw_item_filter_rules( const catacurses::window &win, int starty, int height,
                             item_filter_type type );

char rand_char();
int special_symbol( int sym );

// Remove spaces from the start and the end of a string.
std::string trim( const std::string &s );
// Removes punctuation marks from the start and the end of a string.
std::string trim_punctuation_marks( const std::string &s );
// Converts the string to upper case.
std::string to_upper_case( const std::string &s );

std::string rewrite_vsnprintf( const char *msg );

// TODO: move these elsewhere
// string manipulations.
void replace_name_tags( std::string &input );
void replace_city_tag( std::string &input, const std::string &name );

void replace_substring( std::string &input, const std::string &substring,
                        const std::string &replacement, bool all );

std::string string_replace( std::string text, const std::string &before, const std::string &after );
std::string replace_colors( std::string text );
std::string &capitalize_letter( std::string &str, size_t n = 0 );
size_t shortcut_print( const catacurses::window &w, const point &p, nc_color text_color,
                       nc_color shortcut_color, const std::string &fmt );
size_t shortcut_print( const catacurses::window &w, nc_color text_color, nc_color shortcut_color,
                       const std::string &fmt );
std::string shortcut_text( nc_color shortcut_color, const std::string &fmt );

// short visual animation (player, monster, ...) (hit, dodge, ...)
// cTile is a UTF-8 strings, and must be a single cell wide!
void hit_animation( const point &p, nc_color cColor, const std::string &cTile );

/**
 * @return Pair of a string containing the bar, and its color
 * @param cur Current value being formatted
 * @param max Maximum possible value, e.g. a full bar
 * @param extra_resolution Double the resolution of displayed values with \ symbols.
 * @param colors A vector containing N colors with which to color the bar at different values
 */
// The last color is used for an empty bar
// extra_resolution
std::pair<std::string, nc_color> get_bar( float cur, float max, int width = 5,
        bool extra_resolution = true,
        const std::vector<nc_color> &colors = { c_green, c_light_green, c_yellow, c_light_red, c_red } );

std::pair<std::string, nc_color> get_hp_bar( int cur_hp, int max_hp, bool is_mon = false );

std::pair<std::string, nc_color> get_stamina_bar( int cur_stam, int max_stam );

std::pair<std::string, nc_color> get_light_level( float light );

/**
 * @return String containing the bar. Example: "Label [********    ]".
 * @param val Value to display. Can be unclipped.
 * @param width Width of the entire string.
 * @param label Label before the bar. Can be empty.
 * @param begin Iterator over pairs <double p, char c> (see below).
 * @param end Iterator over pairs <double p, char c> (see below).
 * Where:
 *    p - percentage of the entire bar which can be filled with c.
 *    c - character to fill the segment of the bar with
 */

// MSVC has problem dealing with template functions.
// Implementing this function in cpp file results link error.
template<typename RatingIterator>
inline std::string get_labeled_bar( const double val, const int width, const std::string &label,
                                    RatingIterator begin, RatingIterator end )
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
        for( RatingIterator it( begin ); it != end; ++it ) {
            const double factor = std::min( 1.0, std::max( 0.0, it->first * val ) );
            const int seg_width = static_cast<int>( factor * bar_width ) - used_width;

            if( seg_width <= 0 ) {
                continue;
            }
            used_width += seg_width;
            result.insert( result.end(), seg_width, it->second );
        }
        result.insert( result.end(), bar_width - used_width, ' ' );
    }
    result += ']';

    return result;
}

enum class enumeration_conjunction {
    none,
    and_,
    or_
};

/**
 * @return String containing enumerated elements in format: "a, b, c, ..., and z". Uses the Oxford comma.
 * @param values A vector of strings
 * @param conj Choose how to separate the last elements.
 */
template<typename _Container>
std::string enumerate_as_string( const _Container &values,
                                 enumeration_conjunction conj = enumeration_conjunction::and_ )
{
    const std::string final_separator = [&]() {
        switch( conj ) {
            case enumeration_conjunction::none:
                return _( ", " );
            case enumeration_conjunction::and_:
                return ( values.size() > 2 ? _( ", and " ) : _( " and " ) );
            case enumeration_conjunction::or_:
                return ( values.size() > 2 ? _( ", or " ) : _( " or " ) );
        }
        debugmsg( "Unexpected conjunction" );
        return _( ", " );
    }
    ();
    std::string res;
    for( auto iter = values.begin(); iter != values.end(); ++iter ) {
        if( iter != values.begin() ) {
            if( std::next( iter ) == values.end() ) {
                res += final_separator;
            } else {
                res += _( ", " );
            }
        }
        res += *iter;
    }
    return res;
}

/**
 * @return String containing enumerated elements in format: "a, b, c, ..., and z". Uses the Oxford comma.
 * @param first Iterator pointing to the first element.
 * @param last Iterator pointing to the last element.
 * @param string_for Function that accepts an element and returns a representing string.
 * May return an empty string to omit the element.
 * @param conj Choose how to separate the last elements.
 */
template<typename _FIter, typename F>
std::string enumerate_as_string( _FIter first, _FIter last, F string_for,
                                 enumeration_conjunction conj = enumeration_conjunction::and_ )
{
    std::vector<std::string> values;
    values.reserve( static_cast<size_t>( std::distance( first, last ) ) );
    for( _FIter iter = first; iter != last; ++iter ) {
        const std::string str( string_for( *iter ) );
        if( !str.empty() ) {
            values.push_back( str );
        }
    }
    return enumerate_as_string( values, conj );
}

/**
 * @return String containing the bar. Example: "Label [********    ]".
 * @param val Value to display. Can be unclipped.
 * @param width Width of the entire string.
 * @param label Label before the bar. Can be empty.
 * @param c Character to fill the bar with.
 */
std::string get_labeled_bar( double val, int width, const std::string &label, char c );

void draw_tab( const catacurses::window &w, int iOffsetX, const std::string &sText,
               bool bSelected );
void draw_subtab( const catacurses::window &w, int iOffsetX, const std::string &sText,
                  bool bSelected,
                  bool bDecorate = true, bool bDisabled = false );

// Draws multiple tabs, with the titles specified in tab_texts.
// Also draws the line beneath the tabs and corners at the ends.
// The selected tab (specified by current_tab) is drawn differently.
// In total, something like the following:
//   ┌──────┐ ┌──────┐
//   │ TAB1 │ │ TAB2 │
// ┌─┴──────┴─┘      └───────────┐
void draw_tabs( const catacurses::window &, const std::vector<std::string> &tab_texts,
                size_t current_tab );
// As above, but specify current tab by its label rather than position
void draw_tabs( const catacurses::window &, const std::vector<std::string> &tab_texts,
                const std::string &current_tab );

// This overload of draw_tabs is intended for use when you track the current
// tab via some other value (like an enum) linked to each tab.  Expected use
// looks something like this:
//
// tab_mode current_tab = ...;
// const std::vector<std::pair<tab_mode, std::string>> tabs = {
//      { tab_mode::first_tab, _( "FIRST_TAB" ) },
//      { tab_mode::second_tab, _( "SECOND_TAB" ) },
// };
// draw_tabs( w, tabs, current_tab );
template<typename TabList, typename CurrentTab, typename = std::enable_if_t<
             std::is_same<CurrentTab,
                          std::remove_const_t<typename TabList::value_type::first_type>>::value>>
void draw_tabs( const catacurses::window &w, const TabList &tab_list,
                const CurrentTab &current_tab )
{
    std::vector<std::string> tab_text;
    std::transform( tab_list.begin(), tab_list.end(), std::back_inserter( tab_text ),
    []( const typename TabList::value_type & pair ) {
        return pair.second;
    } );
    auto current_tab_it = std::find_if( tab_list.begin(), tab_list.end(),
    [&current_tab]( const typename TabList::value_type & pair ) {
        return pair.first == current_tab;
    } );
    assert( current_tab_it != tab_list.end() );
    draw_tabs( w, tab_text, std::distance( tab_list.begin(), current_tab_it ) );
}

// Similar to the above, but where the order of tabs is specified separately
// TabList is expected to be a map type.
template<typename TabList, typename TabKeys, typename CurrentTab, typename = std::enable_if_t<
             std::is_same<CurrentTab,
                          std::remove_const_t<typename TabList::value_type::first_type>>::value>>
void draw_tabs( const catacurses::window &w, const TabList &tab_list, const TabKeys &keys,
                const CurrentTab &current_tab )
{
    std::vector<typename TabList::value_type> ordered_tab_list;
    for( const auto &key : keys ) {
        auto it = tab_list.find( key );
        assert( it != tab_list.end() );
        ordered_tab_list.push_back( *it );
    }
    draw_tabs( w, ordered_tab_list, current_tab );
}

// Legacy function, use class scrollbar instead!
void draw_scrollbar( const catacurses::window &window, int iCurrentLine,
                     int iContentHeight, int iNumLines, const point &offset = point_zero,
                     nc_color bar_color = c_white, bool bDoNotScrollToEnd = false );
void calcStartPos( int &iStartPos, int iCurrentLine, int iContentHeight,
                   int iNumEntries );

class scrollbar
{
    public:
        scrollbar();
        // relative position of the scrollbar to the window
        scrollbar &offset_x( int offx );
        scrollbar &offset_y( int offy );
        // total number of lines
        scrollbar &content_size( int csize );
        // index of the beginning line
        scrollbar &viewport_pos( int vpos );
        // number of lines shown
        scrollbar &viewport_size( int vsize );
        // window border color
        scrollbar &border_color( nc_color border_c );
        // scrollbar arrow color
        scrollbar &arrow_color( nc_color arrow_c );
        // scrollbar slot color
        scrollbar &slot_color( nc_color slot_c );
        // scrollbar bar color
        scrollbar &bar_color( nc_color bar_c );
        // can viewport_pos go beyond (content_size - viewport_size)?
        scrollbar &scroll_to_last( bool scr2last );
        // draw the scrollbar to the window
        void apply( const catacurses::window &window );
    private:
        int offset_x_v, offset_y_v;
        int content_size_v, viewport_pos_v, viewport_size_v;
        nc_color border_color_v, arrow_color_v, slot_color_v, bar_color_v;
        bool scroll_to_last_v;
};

// A simple scrolling view onto some text.  Given a window, it will use the
// leftmost column for the scrollbar and fill the rest with text.  When the
// scrollbar is not needed it prints a vertical border in place of it, so the
// expectation is that the given window will overlap the left edge of a
// bordered window if one exists.
// (Options to e.g. not print the border would be easy to add if needed).
// Update the text with set_text (it will be wrapped for you).
// scroll_up and scroll_down are expected to be called from handlers for the
// keys used for that purpose.
// Call draw when drawing related UI stuff.  draw calls werase/wrefresh for its
// window internally.
class scrolling_text_view
{
    public:
        scrolling_text_view( catacurses::window &w ) : w_( w ) {}

        void set_text( const std::string & );
        void scroll_up();
        void scroll_down();
        void page_up();
        void page_down();
        void draw( const nc_color &base_color );
    private:
        int text_width();
        int num_lines();
        int max_offset();

        catacurses::window &w_;
        std::vector<std::string> text_;
        int offset_ = 0;
};

class scrollingcombattext
{
    public:
        enum : int { iMaxSteps = 8 };

        scrollingcombattext() = default;

        class cSCT
        {
            private:
                point pos;
                direction oDir;
                direction oUp;
                direction oUpRight;
                direction oRight;
                direction oDownRight;
                direction oDown;
                direction oDownLeft;
                direction oLeft;
                direction oUpLeft;
                point dir;
                int iStep;
                int iStepOffset;
                std::string sText;
                game_message_type gmt;
                std::string sText2;
                game_message_type gmt2;
                std::string sType;
                bool iso_mode;

            public:
                cSCT( const point &pos, direction p_oDir,
                      const std::string &p_sText, game_message_type p_gmt,
                      const std::string &p_sText2 = "", game_message_type p_gmt2 = m_neutral,
                      const std::string &p_sType = "" );

                int getStep() const {
                    return iStep;
                }
                int getStepOffset() const {
                    return iStepOffset;
                }
                int advanceStep() {
                    return ++iStep;
                }
                int advanceStepOffset() {
                    return ++iStepOffset;
                }
                int getPosX() const;
                int getPosY() const;
                direction getDirecton() const {
                    return oDir;
                }
                int getInitPosX() const {
                    return pos.x;
                }
                int getInitPosY() const {
                    return pos.y;
                }
                std::string getType() const {
                    return sType;
                }
                std::string getText( const std::string &type = "full" ) const;
                game_message_type getMsgType( const std::string &type = "first" ) const;
        };

        std::vector<cSCT> vSCT;

        void add( const point &pos, direction p_oDir,
                  const std::string &p_sText, game_message_type p_gmt,
                  const std::string &p_sText2 = "", game_message_type p_gmt2 = m_neutral,
                  const std::string &p_sType = "" );
        void advanceAllSteps();
        void removeCreatureHP();
};

extern scrollingcombattext SCT;

std::string wildcard_trim_rule( const std::string &pattern_in );
bool wildcard_match( const std::string &text_in, const std::string &pattern_in );
std::vector<std::string> string_split( const std::string &text_in, char delim );
int ci_find_substr( const std::string &str1, const std::string &str2,
                    const std::locale &loc = std::locale() );

std::string format_volume( const units::volume &volume );
std::string format_volume( const units::volume &volume, int width, bool *out_truncated,
                           double *out_value );

inline std::string format_money( int cents )
{
    return string_format( _( "$%.2f" ), cents / 100.0 );
}

/** Get the width in font glyphs of the drawing screen.
 *
 *  May differ from OPTIONS["TERMINAL_X"], for instance in
 *  SDL FULLSCREEN mode, the user setting is overridden.
 */
int get_terminal_width();

/** Get the height in font glyphs of the drawing screen.
 *
 *  May differ from OPTIONS["TERMINAL_Y"], for instance in
 *  SDL FULLSCREEN mode, the user setting is overridden.
 */
int get_terminal_height();

/**
 * Check whether we're in tile drawing mode. The most important
 * effect of this is that we don't need to draw to the ASCII
 * window.
 *
 * Ideally, of course, we'd have a unified tile drawing and ASCII
 * drawing API and use polymorphy, but for the time being there will
 * be a lot of switching around in the map drawing code.
 */
bool is_draw_tiles_mode();

/**
 * Make changes made to the display visible to the user immediately.
 *
 * In curses mode, this is a no-op. In SDL mode, this refreshes
 * the real display from the backing buffer immediately, rather than
 * delaying the update until the next time we are waiting for user input.
 */
void refresh_display();

/**
 * Assigns a custom color to each symbol.
 *
 * @param str String to colorize symbols in
 * @param color_of Function that accepts symbols (std::string::value_type) and returns colors.
 * @return Colorized string.
 */
template<typename F>
std::string colorize_symbols( const std::string &str, F color_of )
{
    std::string res;
    nc_color prev_color = c_unset;

    const auto closing_tag = [ &res, prev_color ]() {
        if( prev_color != c_unset ) {
            res += "</color>";
        }
    };

    for( const auto &elem : str ) {
        const nc_color new_color = color_of( elem );

        if( prev_color != new_color ) {
            closing_tag();
            res += "<color_" + get_all_colors().get_name( new_color ) + ">";
            prev_color = new_color;
        }

        res += elem;
    }

    closing_tag();

    return res;
}

#endif // CATA_SRC_OUTPUT_H
