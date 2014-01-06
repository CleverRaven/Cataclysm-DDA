#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "color.h"
#include <cstdarg>
#include <string>
#include <vector>

#include "item.h"
#include "ui.h"

//      LINE_NESW  - X for on, O for off
#define LINE_XOXO 4194424
#define LINE_OXOX 4194417
#define LINE_XXOO 4194413
#define LINE_OXXO 4194412
#define LINE_OOXX 4194411
#define LINE_XOOX 4194410
#define LINE_XXXO 4194420
#define LINE_XXOX 4194422
#define LINE_XOXX 4194421
#define LINE_OXXX 4194423
#define LINE_XXXX 4194414

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
extern int FULL_SCREEN_WIDTH; // width of "full screen" popups
extern int FULL_SCREEN_HEIGHT; // height of "full screen" popups

std::vector<std::string> foldstring ( std::string str, int width );
int fold_and_print(WINDOW* w, int begin_y, int begin_x, int width, nc_color color, const char *mes, ...);
int fold_and_print_from(WINDOW* w, int begin_y, int begin_x, int width, int begin_line, nc_color color, const char *mes, ...);
void center_print(WINDOW *w, int y, nc_color FG, const char *mes, ...);

void mvputch(int y, int x, nc_color FG, long ch);
void wputch(WINDOW* w, nc_color FG, long ch);
void mvwputch(WINDOW* w, int y, int x, nc_color FG, long ch);
void mvputch_inv(int y, int x, nc_color FG, long ch);
void mvwputch_inv(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvputch_hi(int y, int x, nc_color FG, long ch);
void mvwputch_hi(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvprintz(int y, int x, nc_color FG, const char *mes, ...);
void mvwprintz(WINDOW *w, int y, int x, nc_color FG, const char *mes, ...);
void printz(nc_color FG, const char *mes, ...);
void wprintz(WINDOW *w, nc_color FG, const char *mes, ...);

void draw_border(WINDOW *w, nc_color FG = BORDER_COLOR);
void draw_tabs(WINDOW *w, int active_tab, ...);

std::string word_rewrap (const std::string &ins, int width);
std::vector<size_t> get_tag_positions(const std::string &s);
std::vector<std::string> split_by_color(const std::string &s);

#define STRING2(x) #x
#define STRING(x) STRING2(x)

// classy
#define debugmsg(...) realDebugmsg(__FILE__, STRING(__LINE__), __VA_ARGS__)

void realDebugmsg(const char* name, const char* line, const char *mes, ...);
bool query_yn(const char *mes, ...);
int  query_int(const char *mes, ...);
std::string string_input_popup(std::string title, int width = 0, std::string input = "", std::string desc = "", std::string identifier = "", int max_length = -1, bool only_digits = false);
std::string string_input_win (WINDOW * w, std::string input, int max_length, int startx, int starty, int endx, bool loop, long & key, int & pos, std::string identifier="", int w_x=-1, int w_y=-1, bool dorefresh=true, bool only_digits = false );
char popup_getkey(const char *mes, ...);
// for the next two functions, if cancelable is true, esc returns the last option
int  menu_vec(bool cancelable, const char *mes, std::vector<std::string> options);
int  menu(bool cancelable, const char *mes, ...);
void popup_top(const char *mes, ...); // Displayed at the top of the screen
void popup(const char *mes, ...);
void popup_nowait(const char *mes, ...); // Doesn't wait for spacebar
void full_screen_popup(const char *mes, ...);
int compare_split_screen_popup(int iLeft, int iWidth, int iHeight, std::string sItemName, std::vector<iteminfo> vItemDisplay, std::vector<iteminfo> vItemCompare, int selected = -1, bool without_getch = false);

char rand_char();
long special_symbol (long sym);

// TODO: move these elsewhere
// string manipulations.
std::string from_sentence_case (const std::string &kingston);
std::string string_format(std::string pattern, ...);
std::string& capitalize_letter(std::string &pattern, size_t n=0);
std::string rm_prefix(std::string str, char c1='<', char c2='>');
#define rmp_format(...) rm_prefix(string_format(__VA_ARGS__))
size_t shortcut_print(WINDOW* w, int y, int x, nc_color color, nc_color colork, const char* fmt, ...);
size_t shortcut_print(WINDOW* w, nc_color color, nc_color colork, const char* fmt, ...);

// short visual animation (player, monster, ...) (hit, dodge, ...)
void hit_animation(int iX, int iY, nc_color cColor, char cTile, int iTimeout = 70);

void get_HP_Bar(const int current_hp, const int max_hp, nc_color &color, std::string &health_bar, const bool bMonster = false);
void draw_tab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected);
void draw_subtab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected);
void draw_scrollbar(WINDOW *window, const int iCurrentLine, const int iContentHeight, const int iNumEntries, const int iOffsetY = 0, const int iOffsetX = 0, nc_color bar_color = c_white);
void calcStartPos(int &iStartPos, const int iCurrentLine, const int iContentHeight, const int iNumEntries);
void clear_window(WINDOW* w);

#endif
