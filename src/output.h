#ifndef OUTPUT_H
#define OUTPUT_H

#include "color.h"
#include "line.h"
#include <cstdarg>
#include <string>
#include <vector>
#include <memory>

#include "item.h"
#include "ui.h"

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

enum game_message_type {
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

nc_color msgtype_to_color(const game_message_type type, const bool bOldMsg = false);
int msgtype_to_tilecolor(const game_message_type type, const bool bOldMsg = false);

/**
 * Print text with embedded color tags, x, y are in curses system.
 * The text is not word wrapped, but may automatically be wrapped on new line characters or
 * when it reaches the border of the window (both is done by the curses system).
 * If the text contains no color tags, it's equivalent to a simple mvprintz.
 * @param text The text to print.
 * @param cur_color The current color (could have been set by a previously encountered color tag),
 * change to a color according to the color tags that are in the text.
 * @param base_color Base color that is used outside of any color tag.
 **/
void print_colored_text( WINDOW *w, int x, int y, nc_color &cur_color, nc_color base_color, const std::string &text );
/**
 * Print word wrapped text (with color tags) into the window.
 * @param begin_line Line in the word wrapped text that is printed first (lines before that are not printed at all).
 * @param base_color Color used outside of any color tags.
 * @param scroll_msg Optional, can be empty. If not empty and the text does not fit the window, the string is printed
 * on the last line (in light green), it should show how to scroll the text.
 * @return The maximal scrollable offset ([number of lines to be printed] - [lines available in the window]).
 * This allows the caller to restrict the begin_line number on future calls / when modified by the user.
 */
int print_scrollable( WINDOW *w, int begin_line, const std::string &text, nc_color base_color, const std::string &scroll_msg );

std::vector<std::string> foldstring (std::string str, int width);
int fold_and_print(WINDOW *w, int begin_y, int begin_x, int width, nc_color color, const char *mes,
                   ...);
int fold_and_print(WINDOW *w, int begin_y, int begin_x, int width, nc_color color,
                   const std::string &text);
int fold_and_print_from(WINDOW *w, int begin_y, int begin_x, int width, int begin_line,
                        nc_color color, const char *mes, ...);
int fold_and_print_from(WINDOW *w, int begin_y, int begin_x, int width, int begin_line,
                        nc_color color, const std::string &text);
void center_print(WINDOW *w, int y, nc_color FG, const char *mes, ...);
void display_table(WINDOW *w, const std::string &title, int columns,
                   const std::vector<std::string> &data);
void multipage(WINDOW *w, std::vector<std::string> text, std::string caption = "", int begin_y = 0);
std::string name_and_value(std::string name, int value, int field_width);
std::string name_and_value(std::string name, std::string value, int field_width);

void mvputch(int y, int x, nc_color FG, const std::string &ch);
void wputch(WINDOW *w, nc_color FG, long ch);
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvwputch(WINDOW *w, int y, int x, nc_color FG, const std::string &ch);
void mvputch_inv(int y, int x, nc_color FG, const std::string &ch);
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_inv(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvwputch_inv(WINDOW *w, int y, int x, nc_color FG, const std::string &ch);
void mvputch_hi(int y, int x, nc_color FG, const std::string &ch);
// Using long ch is deprecated, use an UTF-8 encoded string instead
void mvwputch_hi(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvwputch_hi(WINDOW *w, int y, int x, nc_color FG, const std::string &ch);
void mvprintz(int y, int x, nc_color FG, const char *mes, ...);
void mvwprintz(WINDOW *w, int y, int x, nc_color FG, const char *mes, ...);
void printz(nc_color FG, const char *mes, ...);
void wprintz(WINDOW *w, nc_color FG, const char *mes, ...);

void draw_border(WINDOW *w, nc_color FG = BORDER_COLOR);
void draw_tabs(WINDOW *w, int active_tab, ...);

std::string word_rewrap (const std::string &ins, int width);
std::vector<size_t> get_tag_positions(const std::string &s);
std::vector<std::string> split_by_color(const std::string &s);
std::string remove_color_tags(const std::string &s);

bool query_yn(const char *mes, ...);
int  query_int(const char *mes, ...);

std::string string_input_popup(std::string title, int width = 0, std::string input = "",
                               std::string desc = "", std::string identifier = "",
                               int max_length = -1, bool only_digits = false);

std::string string_input_win (WINDOW *w, std::string input, int max_length, int startx,
                              int starty, int endx, bool loop, long &key, int &pos,
                              std::string identifier = "", int w_x = -1, int w_y = -1,
                              bool dorefresh = true, bool only_digits = false);

long popup_getkey(const char *mes, ...);
// for the next two functions, if cancelable is true, esc returns the last option
int  menu_vec(bool cancelable, const char *mes, std::vector<std::string> options);
int  menu(bool cancelable, const char *mes, ...);
void popup_top(const char *mes, ...); // Displayed at the top of the screen
void popup(const char *mes, ...);
typedef enum {
    PF_NONE        = 0,
    PF_GET_KEY     = 1 <<  0,
    PF_NO_WAIT     = 1 <<  1,
    PF_ON_TOP      = 1 <<  2,
    PF_FULLSCREEN  = 1 <<  3,
} PopupFlags;
long popup(const std::string &text, PopupFlags flags);
void popup_nowait(const char *mes, ...); // Doesn't wait for spacebar
void full_screen_popup(const char *mes, ...);

int draw_item_info(WINDOW *win, const std::string sItemName,
                   std::vector<iteminfo> &vItemDisplay, std::vector<iteminfo> &vItemCompare,
                   const int selected = -1, const bool without_getch = false, const bool without_border = false);

int draw_item_info(const int iLeft, int iWidth, const int iTop, const int iHeight,
                   const std::string sItemName,
                   std::vector<iteminfo> &vItemDisplay, std::vector<iteminfo> &vItemCompare,
                   const int selected = -1, const bool without_getch = false, const bool without_border = false);

char rand_char();
long special_symbol (long sym);

// TODO: move these elsewhere
// string manipulations.
std::string from_sentence_case (const std::string &kingston);

std::string string_format(const char *pattern, ...);
std::string vstring_format(const char *pattern, va_list argptr);
std::string string_format(const std::string pattern, ...);
std::string vstring_format(const std::string pattern, va_list argptr);

std::string &capitalize_letter(std::string &pattern, size_t n = 0);
std::string rm_prefix(std::string str, char c1 = '<', char c2 = '>');
#define rmp_format(...) rm_prefix(string_format(__VA_ARGS__))
size_t shortcut_print(WINDOW *w, int y, int x, nc_color color, nc_color colork,
                      const std::string &fmt);
size_t shortcut_print(WINDOW *w, nc_color color, nc_color colork, const std::string &fmt);

// short visual animation (player, monster, ...) (hit, dodge, ...)
// cTile is a UTF-8 strings, and must be a single cell wide!
void hit_animation(int iX, int iY, nc_color cColor, const std::string &cTile);
void get_HP_Bar(const int current_hp, const int max_hp, nc_color &color,
                std::string &health_bar, const bool bMonster = false);
void get_item_HP_Bar(const int iDamage, nc_color &color, std::string &text);
void draw_tab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected);
void draw_subtab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected, bool bDecorate = true);
void draw_scrollbar(WINDOW *window, const int iCurrentLine, const int iContentHeight,
                    const int iNumEntries, const int iOffsetY = 0, const int iOffsetX = 0,
                    nc_color bar_color = c_white);
void calcStartPos(int &iStartPos, const int iCurrentLine,
                  const int iContentHeight, const int iNumEntries);
void clear_window(WINDOW *w);

class scrollingcombattext
{
    private:

    public:
        const int iMaxSteps;

        scrollingcombattext() : iMaxSteps(8) {};
        ~scrollingcombattext() {};

        class cSCT
        {
            private:
                int iPosX;
                int iPosY;
                direction oDir;
                int iDirX;
                int iDirY;
                int iStep;
                int iStepOffset;
                std::string sText;
                game_message_type gmt;
                std::string sText2;
                game_message_type gmt2;
                std::string sType;

            public:
                cSCT(const int p_iPosX, const int p_iPosY, direction p_oDir,
                     const std::string p_sText, const game_message_type p_gmt,
                     const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                     const std::string p_sType = "");
                ~cSCT() {};

                int getStep()
                {
                    return iStep;
                }
                int getStepOffset()
                {
                    return iStepOffset;
                }
                int advanceStep()
                {
                    return ++iStep;
                }
                int advanceStepOffset()
                {
                    return ++iStepOffset;
                }
                int getPosX();
                int getPosY();
                direction getDirecton()
                {
                    return oDir;
                }
                int getInitPosX()
                {
                    return iPosX;
                }
                int getInitPosY()
                {
                    return iPosY;
                }
                std::string getType()
                {
                    return sType;
                }
                std::string getText(std::string sType = "full");
                game_message_type getMsgType(std::string sType = "first");
        };

        std::vector<cSCT> vSCT;

        void add(const int p_iPosX, const int p_iPosY, const direction p_oDir,
                 const std::string p_sText, const game_message_type p_gmt,
                 const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                 const std::string p_sType = "");
        void advanceAllSteps();
        void removeCreatureHP();
};

extern scrollingcombattext SCT;

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

void play_music(std::string playlist);
void play_sound(std::string identifier);

#endif
