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

#define FULL_SCREEN_WIDTH 80  // Width of full Screen popup
#define FULL_SCREEN_HEIGHT 25 // Height of full Screen popup

// Display data
extern int TERMX;
extern int TERMY;
extern int VIEWX;
extern int VIEWY;
extern int VIEW_OFFSET_X;
extern int VIEW_OFFSET_Y;
extern int TERRAIN_WINDOW_WIDTH;
extern int TERRAIN_WINDOW_HEIGHT;

std::vector<std::string> foldstring ( std::string str, int width );
int fold_and_print(WINDOW* w, int begin_y, int begin_x, int width, nc_color color, const char *mes, ...);
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
std::string word_rewrap (const std::string &ins, int width);
void draw_tabs(WINDOW *w, int active_tab, ...);

#define STRING2(x) #x
#define STRING(x) STRING2(x)

// classy
#define debugmsg(...) realDebugmsg(__FILE__, STRING(__LINE__), __VA_ARGS__)

void realDebugmsg(const char* name, const char* line, const char *mes, ...);
bool query_yn(const char *mes, ...);
int  query_int(const char *mes, ...);
std::string string_input_popup(std::string title, int max_length = 0, std::string input = "");
char popup_getkey(const char *mes, ...);
// for the next two functions, if cancelable is true, esc returns the last option
int  menu_vec(bool cancelable, const char *mes, std::vector<std::string> options);
int  menu(bool cancelable, const char *mes, ...);
void popup_top(const char *mes, ...); // Displayed at the top of the screen
void popup(const char *mes, ...);
void popup_nowait(const char *mes, ...); // Doesn't wait for spacebar
void full_screen_popup(const char *mes, ...);
int compare_split_screen_popup(int iLeft, int iWidth, int iHeight, std::string sItemName, std::vector<iteminfo> vItemDisplay, std::vector<iteminfo> vItemCompare, int selected=-1);

nc_color hilite(nc_color c);
nc_color invert_color(nc_color c);
nc_color red_background(nc_color c);
nc_color white_background(nc_color c);
nc_color green_background(nc_color c);
nc_color yellow_background(nc_color c);
nc_color magenta_background(nc_color c);
nc_color cyan_background(nc_color c);
nc_color rand_color();
char rand_char();
long special_symbol (long sym);

// string manipulations.
std::string from_sentence_case (const std::string &kingston);
std::string string_format(std::string pattern, ...);
std::string& capitalize_letter(std::string &pattern, size_t n=0);
size_t shortcut_print(WINDOW* w, int y, int x, nc_color color, nc_color colork, const char* fmt, ...);

// short visual animation (player, monster, ...) (hit, dodge, ...)
void hit_animation(int iX, int iY, nc_color cColor, char cTile, int iTimeout = 70);

void draw_tab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected);
void clear_window(WINDOW* w);

#endif
