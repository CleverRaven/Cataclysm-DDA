#ifndef OUTPUT_H
#define OUTPUT_H

#include "color.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "translations.h"
#include "units.h"
#include "printf_check.h"

#include <cstdarg>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <map>

struct input_event;
struct iteminfo;
enum direction : unsigned;
class input_context;

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

// a consistent border colour
#define BORDER_COLOR c_ltgray

// Display data
extern int TERMX; // width available for display
extern int TERMY; // height available for display
extern int POSX; // X position of '@' inside terrain window
extern int POSY; // Y position of '@' inside terrain window
extern int VIEW_OFFSET_X; // X position of terrain window
extern int VIEW_OFFSET_Y; // Y position of terrain window
extern int TERRAIN_WINDOW_WIDTH; // width of terrain window
extern int TERRAIN_WINDOW_HEIGHT; // height of terrain window
extern int TERRAIN_WINDOW_TERM_WIDTH; // width of terrain window in terminal characters
extern int TERRAIN_WINDOW_TERM_HEIGHT; // same for height
extern int FULL_SCREEN_WIDTH; // width of "full screen" popups
extern int FULL_SCREEN_HEIGHT; // height of "full screen" popups
extern int OVERMAP_WINDOW_WIDTH; // width of overmap window
extern int OVERMAP_WINDOW_HEIGHT; // height of overmap window

struct delwin_functor {
    void operator()( WINDOW *w ) const;
};
/**
 * A Wrapper around the WINDOW pointer, it automatically deletes the
 * window (see delwin_functor) when the variable gets out of scope.
 * This includes calling werase, wrefresh and delwin.
 * Usage:
 * 1. Acquire a WINDOW pointer via @ref newwin like normal, store it in a pointer variable.
 * 2. Create a variable of type WINDOW_PTR *on the stack*, initialize it with the pointer from 1.
 * 3. Do the usual stuff with window, print, update, etc. but do *not* call delwin on it.
 * 4. When the function is left, the WINDOW_PTR variable is destroyed, and its destructor is called,
 *    it calls werase, wrefresh and most importantly delwin to free the memory associated wit the pointer.
 * To trigger the delwin call earlier call some_window_ptr.reset().
 * To prevent the delwin call when the function is left (because the window is already deleted or, it should
 * not be deleted), call some_window_ptr.release().
 */
typedef std::unique_ptr<WINDOW, delwin_functor> WINDOW_PTR;

enum game_message_type : int {
    m_good,    /* something good happened to the player character, eg. health boost, increasing in skill */
    m_bad,      /* something bad happened to the player character, eg. damage, decreasing in skill */
    m_mixed,   /* something happened to the player character which is mixed (has good and bad parts),
                  eg. gaining a mutation with mixed effect*/
    m_warning, /* warns the player about a danger. eg. enemy appeared, an alarm sounds, noise heard. */
    m_info,    /* informs the player about something, eg. on examination, seeing an item,
                  about how to use a certain function, etc. */
    m_neutral,  /* neutral or indifferent events which aren’t informational or nothing really happened eg.
                  a miss, a non-critical failure. May also effect for good or bad effects which are
                  just very slight to be notable. This is the default message type. */

    m_debug, /* only shown when debug_mode is true */
    /* custom SCT colors */
    m_headshot,
    m_critical,
    m_grazing
};

nc_color msgtype_to_color( const game_message_type type, const bool bOldMsg = false );
int msgtype_to_tilecolor( const game_message_type type, const bool bOldMsg = false );

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
 * Note: use @ref string_from_color to convert a `nc_color` value to a string suitable for a
 * color tag:
 * \code
 *    nc_color color = ...;
 *    text = "<color_" + string_from_color( color ) + ">some text</color>";
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
std::string remove_color_tags( const std::string &text );
/*@}*/

/**
 * Split the input text into separate lines and wrap long lines. Each resulting line is at most
 * `width` console cells long.
 * The functions handles @ref color_tags.
 * @return A vector of lines, it may contain empty strings. Each entry is at most `width`
 * console cells width.
 */
std::vector<std::string> foldstring( std::string str, int width );

/**
 * Print text with embedded @ref color_tags, x, y are in curses system.
 * The text is not word wrapped, but may automatically be wrapped on new line characters or
 * when it reaches the border of the window (both is done by the curses system).
 * If the text contains no color tags, it's equivalent to a simple mvprintz.
 * @param text The text to print.
 * @param cur_color The current color (could have been set by a previously encountered color tag),
 * change to a color according to the color tags that are in the text.
 * @param base_color Base color that is used outside of any color tag.
 **/
void print_colored_text( WINDOW *w, int y, int x, nc_color &cur_color, nc_color base_color,
                         const std::string &text );
/**
 * Print word wrapped text (with @ref color_tags) into the window.
 * @param begin_line Line in the word wrapped text that is printed first (lines before that are not printed at all).
 * @param base_color Color used outside of any color tags.
 * @param scroll_msg Optional, can be empty. If not empty and the text does not fit the window, the string is printed
 * on the last line (in light green), it should show how to scroll the text.
 * @return The maximal scrollable offset ([number of lines to be printed] - [lines available in the window]).
 * This allows the caller to restrict the begin_line number on future calls / when modified by the user.
 */
int print_scrollable( WINDOW *w, int begin_line, const std::string &text, nc_color base_color,
                      const std::string &scroll_msg );
/**
 * Format, fold and print text in the given window. The function handles @ref color_tags and
 * uses them while printing. It expects a printf-like format string and matching
 * arguments to that format (see @ref string_format).
 * @param begin_x The row index on which to print the first line.
 * @param begin_y The column index on which to start each line.
 * @param width The width used to fold the text (see @ref foldstring). `width + begin_y` should be
 * less than the window width, otherwise the lines will be wrapped by the curses system, which
 * defeats the purpose of using `foldstring`.
 * @param color The initially used color. This can be overridden using color tags.
 * @return The number of lines of the formatted text (after folding). This may be larger than
 * the height of the window.
 */
int fold_and_print( WINDOW *w, int begin_y, int begin_x, int width, nc_color color, const char *mes,
                    ... ) PRINTF_LIKE(6,7);
/**
 * Same as other @ref fold_and_print, but does not do any string formatting, the string is uses as is.
 */
int fold_and_print( WINDOW *w, int begin_y, int begin_x, int width, nc_color color,
                    const std::string &text );
/**
 * Like @ref fold_and_print, but starts the output with the N-th line of the folded string.
 * This can be used for scrolling large texts. Parameters have the same meaning as for
 * @ref fold_and_print, the function therefor handles @ref color_tags correctly.
 * @param begin_line The index of the first line (of the folded string) that is to be printed.
 * The function basically removes all lines before this one and prints the remaining lines
 * with `fold_and_print`.
 * @return Same as `fold_and_print`: the number of lines of the text (after folding). This is
 * always the same value, regardless of `begin_line`, it can be used to determine the maximal
 * value for `begin_line`.
 */
int fold_and_print_from( WINDOW *w, int begin_y, int begin_x, int width, int begin_line,
                         nc_color color, const char *mes, ... ) PRINTF_LIKE(7,8);
/**
 * Same as other @ref fold_and_print_from, but does not do any string formatting, the string is uses as is.
 */
int fold_and_print_from( WINDOW *w, int begin_y, int begin_x, int width, int begin_line,
                         nc_color color, const std::string &text );
/**
 * Prints a single line of formatted text. The text is automatically trimmed to fit into the given
 * width. The function handles @ref color_tags correctly.
 * @param begin_x,begin_y The row and column index on which to start the line.
 * @param width Maximal width of the printed line, if the text is longer, it is cut off.
 * @param base_color The initially used color. This can be overridden using color tags.
 */
void trim_and_print( WINDOW *w, int begin_y, int begin_x, int width, nc_color base_color,
                     const char *mes, ... ) PRINTF_LIKE(6,7);
void center_print( WINDOW *w, int y, nc_color FG, const char *mes, ... );
int right_print( WINDOW *w, const int line, const int right_indent, const nc_color FG,
                 const char *mes, ... ) PRINTF_LIKE(5,6);
void display_table( WINDOW *w, const std::string &title, int columns,
                    const std::vector<std::string> &data );
void multipage( WINDOW *w, std::vector<std::string> text, std::string caption = "",
                int begin_y = 0 );
std::string name_and_value( std::string name, int value, int field_width );
std::string name_and_value( std::string name, std::string value, int field_width );

void mvputch( int y, int x, nc_color FG, const std::string &ch );
void wputch( WINDOW *w, nc_color FG, long ch );
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch( WINDOW *w, int y, int x, nc_color FG, long ch );
void mvwputch( WINDOW *w, int y, int x, nc_color FG, const std::string &ch );
void mvputch_inv( int y, int x, nc_color FG, const std::string &ch );
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_inv( WINDOW *w, int y, int x, nc_color FG, long ch );
void mvwputch_inv( WINDOW *w, int y, int x, nc_color FG, const std::string &ch );
void mvputch_hi( int y, int x, nc_color FG, const std::string &ch );
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_hi( WINDOW *w, int y, int x, nc_color FG, long ch );
void mvwputch_hi( WINDOW *w, int y, int x, nc_color FG, const std::string &ch );
void mvwprintz( WINDOW *w, int y, int x, nc_color FG, const char *mes, ... ) PRINTF_LIKE(5,6);
void wprintz( WINDOW *w, nc_color FG, const char *mes, ... ) PRINTF_LIKE( 3, 4 );

void draw_custom_border( WINDOW *w, chtype ls = 1, chtype rs = 1, chtype ts = 1, chtype bs = 1,
                         chtype tl = 1, chtype tr = 1,
                         chtype bl = 1, chtype br = 1, nc_color FG = BORDER_COLOR, int posy = 0, int height = 0,
                         int posx = 0, int width = 0 );
void draw_border( WINDOW *w, nc_color border_color = BORDER_COLOR,
                  std::string title = "", nc_color title_color = c_ltred );
void draw_tabs( WINDOW *w, int active_tab, ... );

std::string word_rewrap( const std::string &ins, int width );
std::vector<size_t> get_tag_positions( const std::string &s );
std::vector<std::string> split_by_color( const std::string &s );

bool query_yn( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
bool query_int( int &result, const char *mes, ... ) PRINTF_LIKE( 2, 3 );

bool internal_query_yn( const char *mes, va_list ap );

/**
 * Shows a window querying the user for input.
 *
 * Returns the input that was entered. If the user cancels the input (e.g. by pressing escape),
 * an empty string is returned. An empty string may also be returned when the user does not enter
 * any text and confirms the input (by pressing ENTER). It's currently not possible these two
 * situations.
 *
 * @param title The displayed title, describing what to enter. @ref color_tags can be used.
 * @param width Width of the input area where the user input appears.
 * @param input The initially display input. The user can change this.
 * @param desc An optional text (e.h. help or formatting information) which is displayed
 * above the input. Color tags can be used.
 * @param identifier If not empty, this is used to store and retrieve previously entered
 * text. All calls with the same `identifier` share this history, the history is also stored
 * when saving the game (see @ref uistate).
 * @param max_length The maximal length of the text the user can input. More input is simply
 * ignored and the returned string is never longer than this.
 * @param only_digits Whether to only allow digits in the string.
 */
std::string string_input_popup( std::string title, int width = 0, std::string input = "",
                                std::string desc = "", std::string identifier = "",
                                int max_length = -1, bool only_digits = false );

std::string string_input_win( WINDOW *w, std::string input, int max_length, int startx,
                              int starty, int endx, bool loop, long &key, int &pos,
                              std::string identifier = "", int w_x = -1, int w_y = -1,
                              bool dorefresh = true, bool only_digits = false,
                              std::map<long, std::function<void()>> callbacks = std::map<long, std::function<void()>>(),
                              std::set<long> ch_code_blacklist = std::set<long>() );

std::string string_input_win_from_context(
    WINDOW *w, input_context &ctxt, std::string input, int max_length, int startx, int starty,
    int endx, bool loop, std::string &action, long &ch, int &pos, std::string identifier = "",
    int w_x = -1, int w_y = -1, bool dorefresh = true, bool only_digits = false, bool draw_only = false,
    std::map<long, std::function<void()>> callbacks = std::map<long, std::function<void()>>(),
    std::set<long> ch_code_blacklist = std::set<long>() );

// for the next two functions, if cancelable is true, esc returns the last option
int  menu_vec( bool cancelable, const char *mes, const std::vector<std::string> options );
int  menu_vec( bool cancelable, const char *mes, const std::vector<std::string> &options,
               const std::string &hotkeys_override );
int  menu( bool cancelable, const char *mes, ... );

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
 *   Without the flag the popup is only canceled when the user enters new-line, space and escape.
 *   This flag is passed by @ref popup_getkey.
 * - PF_NO_WAIT displays the popup, but does not wait for the user input. The popup window is
 *   immediately destroyed (but will be visible until another window is redrawn over it).
 *   The function always returns 0 upon this flag, no call to `getch` is done at all.
 *   This flag is passed by @ref popup_nowait.
 * - PF_ON_TOP makes the window appear on the top of the screen (at the upper most row). Without
 *   this flag, the popup is centered on the screen.
 *   The flag is passed by @ref popup_top.
 * - PF_FULLSCREEN makes the popup window as big as the whole screen.
 *   This flag is passed by @ref full_screen_popup.
 * - PF_NONE is a placeholder for none of the above flags.
 *
 */
/*@{*/
typedef enum {
    PF_NONE        = 0,
    PF_GET_KEY     = 1 << 0,
    PF_NO_WAIT     = 1 << 1,
    PF_ON_TOP      = 1 << 2,
    PF_FULLSCREEN  = 1 << 3,
    PF_NO_WAIT_ON_TOP = PF_NO_WAIT | PF_ON_TOP,
} PopupFlags;

long popup_getkey( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
void popup_top( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
void popup_nowait( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
void popup_status( const char *title, const char *msg, ... ) PRINTF_LIKE( 2, 3 );
void popup( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
long popup( const std::string &text, PopupFlags flags );
void full_screen_popup( const char *mes, ... ) PRINTF_LIKE( 1, 2 );
/*@}*/

input_event draw_item_info( WINDOW *win, const std::string sItemName, const std::string sTypeName,
                    std::vector<iteminfo> &vItemDisplay, std::vector<iteminfo> &vItemCompare,
                    int &selected, const bool without_getch = false, const bool without_border = false,
                    const bool handle_scrolling = false, const bool scrollbar_left = true,
                    const bool use_full_win = false );

input_event draw_item_info( const int iLeft, int iWidth, const int iTop, const int iHeight,
                    const std::string sItemName, const std::string sTypeName,
                    std::vector<iteminfo> &vItemDisplay, std::vector<iteminfo> &vItemCompare,
                    int &selected, const bool without_getch = false, const bool without_border = false,
                    const bool handle_scrolling = false, const bool scrollbar_left = true,
                    const bool use_full_win = false );

enum class item_filter_type: int {
    FIRST = 1, // used for indexing into tables
    FILTER = 1,
    LOW_PRIORITY = 2,
    HIGH_PRIORITY = 3
};
/**
 * Write some tips (such as precede items with - to exclude them) onto the window.
 *
 * @param starty: Where to start relative to the top of the window.
 * @param height: Every row from starty to starty + height - 1 will be cleared before printing the rules.
*/
void draw_item_filter_rules( WINDOW *win, int starty, int height, item_filter_type type );

char rand_char();
long special_symbol( long sym );

std::string trim( const std::string &s ); // Remove spaces from the start and the end of a string
std::string trim_punctuation_marks( const std::string &s ); // Removes punctuation marks from the start and the end of a string
std::string to_upper_case( const std::string &s ); // Converts the string to upper case

/**
 * @name printf-like string formatting.
 *
 * These functions perform string formatting, according to the rules of the `printf` function,
 * see `man 3 printf` or any other documentation.
 *
 * In short: the pattern parameter is a string with optional placeholders, which will be
 * replaced with formatted data from the further arguments. The further arguments must have
 * a type that matches the type expected by the placeholder.
 * The placeholders look like this:
 * - `%s` expects an argument of type `const char*`, which is inserted as is.
 * - `%d` expects an argument of type `int`, which is formatted as decimal number.
 * - `%f` expects an argument of type `float` or `double`, which is formatted as decimal number.
 *
 * There are more placeholders and options to them (see documentation of `printf`).
 */
/*@{*/
std::string string_format( const char *pattern, ... ) PRINTF_LIKE( 1, 2 );
std::string vstring_format( const char *pattern, va_list argptr );
std::string string_format( std::string pattern, ... );
std::string vstring_format( std::string const &pattern, va_list argptr );
/*@}*/

// TODO: move these elsewhere
// string manipulations.
void replace_name_tags( std::string &input );
void replace_city_tag( std::string &input, const std::string &name );

void replace_substring( std::string &input, const std::string &substring,
                        const std::string &replacement, bool all );

std::string string_replace( std::string text, const std::string &before, const std::string &after );
std::string replace_colors( std::string text );
std::string &capitalize_letter( std::string &pattern, size_t n = 0 );
size_t shortcut_print( WINDOW *w, int y, int x, nc_color text_color, nc_color shortcut_color,
                       const std::string &fmt );
size_t shortcut_print( WINDOW *w, nc_color text_color, nc_color shortcut_color,
                       const std::string &fmt );

// short visual animation (player, monster, ...) (hit, dodge, ...)
// cTile is a UTF-8 strings, and must be a single cell wide!
void hit_animation( int iX, int iY, nc_color cColor, const std::string &cTile );

std::pair<std::string, nc_color> const &get_hp_bar( int cur_hp, int max_hp, bool is_mon = false );

std::pair<std::string, nc_color> const &get_light_level( const float light );

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
            const int seg_width = int( factor * bar_width ) - used_width;

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

/**
 * @return String containing enumerated elements in format: "a, b, c, ..., and z". Uses the Oxford comma.
 * @param values A vector of strings
 * @param use_and If true, add "and" before the last element (comma otherwise).
 */
template<typename _Container>
std::string enumerate_as_string( const _Container &values, bool use_and = true )
{
    std::ostringstream res;
    for( auto iter = values.begin(); iter != values.end(); ++iter ) {
        if( iter != values.begin() ) {
            if( use_and && std::next( iter ) == values.end() ) {
                res << ( values.size() > 2 ? _( ", and " ) : _( " and " ) );
            } else {
                res << _( ", " );
            }
        }
        res << *iter;
    }
    return res.str();
}

/**
 * @return String containing enumerated elements in format: "a, b, c, ..., and z". Uses the Oxford comma.
 * @param first Iterator pointing to the first element.
 * @param last Iterator pointing to the last element.
 * @param pred Predicate that accepts an element and returns a representing string.
 * May return an empty string to omit the element.
 * @param use_and If true, add "and" before the last element (comma otherwise).
 */
template<typename _FIter, typename _Predicate>
std::string enumerate_as_string( _FIter first, _FIter last, _Predicate pred, bool use_and = true )
{
    std::vector<std::string> values;
    values.reserve( size_t( std::distance( first, last ) ) );
    for( _FIter iter = first; iter != last; ++iter ) {
        const std::string str( pred( *iter ) );
        if( !str.empty() ) {
            values.push_back( str );
        }
    }
    return enumerate_as_string( values, use_and );
}

/**
 * @return String containing the bar. Example: "Label [********    ]".
 * @param val Value to display. Can be unclipped.
 * @param width Width of the entire string.
 * @param label Label before the bar. Can be empty.
 * @param c Character to fill the bar with.
 */
std::string get_labeled_bar( const double val, const int width, const std::string &label, char c );

void draw_tab( WINDOW *w, int iOffsetX, std::string sText, bool bSelected );
void draw_subtab( WINDOW *w, int iOffsetX, std::string sText, bool bSelected,
                  bool bDecorate = true );
void draw_scrollbar( WINDOW *window, const int iCurrentLine, const int iContentHeight,
                     const int iNumEntries, const int iOffsetY = 0, const int iOffsetX = 0,
                     nc_color bar_color = c_white, const bool bTextScroll = false );
void calcStartPos( int &iStartPos, const int iCurrentLine,
                   const int iContentHeight, const int iNumEntries );

class scrollingcombattext
{
    public:
        enum : int { iMaxSteps = 8 };

        scrollingcombattext() = default;

        class cSCT
        {
            private:
                int iPosX;
                int iPosY;
                direction oDir;
                direction oUp, oUpRight, oRight, oDownRight, oDown, oDownLeft, oLeft, oUpLeft;
                int iDirX;
                int iDirY;
                int iStep;
                int iStepOffset;
                std::string sText;
                game_message_type gmt;
                std::string sText2;
                game_message_type gmt2;
                std::string sType;
                bool iso_mode;

            public:
                cSCT( const int p_iPosX, const int p_iPosY, direction p_oDir,
                      const std::string p_sText, const game_message_type p_gmt,
                      const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                      const std::string p_sType = "" );

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
                    return iPosX;
                }
                int getInitPosY() const {
                    return iPosY;
                }
                std::string getType() const {
                    return sType;
                }
                std::string getText( std::string const &type = "full" ) const;
                game_message_type getMsgType( std::string const &type = "first" ) const;
        };

        std::vector<cSCT> vSCT;

        void add( const int p_iPosX, const int p_iPosY, const direction p_oDir,
                  const std::string p_sText, const game_message_type p_gmt,
                  const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                  const std::string p_sType = "" );
        void advanceAllSteps();
        void removeCreatureHP();
};

extern scrollingcombattext SCT;

std::string wildcard_trim_rule( const std::string &sPatternIn );
bool wildcard_match( const std::string &sTextIn, const std::string &sPatternIn );
std::vector<std::string> &wildcard_split( const std::string &s, char delim, std::vector<std::string> &elems );
int ci_find_substr( const std::string &str1, const std::string &str2, const std::locale &loc = std::locale() );

std::string format_volume( const units::volume &volume );
std::string format_volume( const units::volume &volume, int width, bool *out_truncated, double *out_value );

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
 * drawing API and use polymorphy, but for the time being there'll
 * be a lot of switching around in the map drawing code.
 */
bool is_draw_tiles_mode();

void play_music( std::string playlist );

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
 * @return Colorized string.
 * @param func Function that accepts symbols (std::string::value_type) and returns colors.
 */
template<typename Pred>
std::string colorize_symbols( const std::string &str, Pred func )
{
    std::ostringstream res;
    nc_color prev_color = c_unset;

    const auto closing_tag = [ &res, prev_color ]() {
        if( prev_color != c_unset ) {
            res << "</color>";
        }
    };

    for( const auto &elem : str ) {
        const nc_color new_color = func( elem );

        if( prev_color != new_color ) {
            closing_tag();
            res << "<color_" << get_all_colors().get_name( new_color ) << ">";
            prev_color = new_color;
        }

        res << elem;
    }

    closing_tag();

    return res.str();
}

#endif
