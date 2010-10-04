#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "color.h"
#include <cstdarg>
#include <string>
#include <vector>

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

/*
struct window
{
 WINDOW* w;
 window(int x0 = 0, int y0 = 0, int Width, int Height);
 outline(nc_color FG = c_white);
 mvputch(win_attribute attr = WA_NULL,
         int x, int y, nc_color FG, char ch);
 mvprint(win_attribute attr = WA_NULL,
         int x, int y, nc_color FG = c_ltgray, std::string text);
 print(win_attribute attr = WA_NULL,
       int x, int y, nc_color FG = c_ltgray, std::string text);

 int width, height;
};
*/

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

void debugmsg(const char *mes, ...);
bool query_yn(const char *mes, ...);
std::string string_input_popup(const char *mes, ...);
int  menu_vec(const char *mes, std::vector<std::string> options);
int  menu(const char *mes, ...);
void popup_top(const char *mes, ...); // Displayed at the top of the screen
void popup(const char *mes, ...);
void full_screen_popup(const char *mes, ...);

nc_color hilite(nc_color c);
nc_color rand_color();
char rand_char();

#endif
