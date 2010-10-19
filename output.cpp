#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <curses.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>
#include "color.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"

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

void mvputch(int y, int x, nc_color FG, long ch)
{
 attron(FG);
 mvaddch(y, x, ch);
 attroff(FG);
}

void wputch(WINDOW* w, nc_color FG, long ch)
{
 wattron(w, FG);
 waddch(w, ch);
 wattroff(w, FG);
}

void mvwputch(WINDOW *w, int y, int x, nc_color FG, long ch)
{
 wattron(w, FG);
 mvwaddch(w, y, x, ch);
 wattroff(w, FG);
}

void mvputch_inv(int y, int x, nc_color FG, long ch)
{
 nc_color HC;
 switch (FG) {
  case c_white:
   HC = i_white;
   break;
  case c_ltgray:
   HC = i_ltgray;
   break;
  case c_dkgray:
   HC = i_dkgray;
   break;
  case c_red:
   HC = i_red;
   break;
  case c_green:
   HC = i_green;
   break;
  case c_blue:
   HC = i_blue;
   break;
  case c_cyan:
   HC = i_cyan;
   break;
  case c_magenta:
   HC = i_magenta;
   break;
  case c_brown:
   HC = i_brown;
   break;
  case c_yellow:
   HC = i_yellow;
   break;
  case c_ltred:
   HC = i_ltred;
   break;
  case c_ltgreen:
   HC = i_ltgreen;
   break;
  case c_ltblue:
   HC = i_ltblue;
   break;
  case c_ltcyan:
   HC = i_ltcyan;
   break;
  case c_pink:
   HC = i_pink;
   break;
 }
 attron(HC);
 mvaddch(y, x, ch);
 attroff(HC);
}

void mvwputch_inv(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC;
 switch (FG) {
  case c_white:
   HC = i_white;
   break;
  case c_ltgray:
   HC = i_ltgray;
   break;
  case c_dkgray:
   HC = i_dkgray;
   break;
  case c_red:
   HC = i_red;
   break;
  case c_green:
   HC = i_green;
   break;
  case c_blue:
   HC = i_blue;
   break;
  case c_cyan:
   HC = i_cyan;
   break;
  case c_magenta:
   HC = i_magenta;
   break;
  case c_brown:
   HC = i_brown;
   break;
  case c_yellow:
   HC = i_yellow;
   break;
  case c_ltred:
   HC = i_ltred;
   break;
  case c_ltgreen:
   HC = i_ltgreen;
   break;
  case c_ltblue:
   HC = i_ltblue;
   break;
  case c_ltcyan:
   HC = i_ltcyan;
   break;
  case c_pink:
   HC = i_pink;
   break;
 }
 wattron(w, HC);
 mvwaddch(w, y, x, ch);
 wattroff(w, HC);
}

void mvputch_hi(int y, int x, nc_color FG, long ch)
{
 nc_color HC;
 switch (FG) {
  case c_white:
   HC = h_white;
   break;
  case c_ltgray:
   HC = h_ltgray;
   break;
  case c_dkgray:
   HC = h_dkgray;
   break;
  case c_red:
   HC = h_red;
   break;
  case c_green:
   HC = h_green;
   break;
  case c_blue:
   HC = h_blue;
   break;
  case c_cyan:
   HC = h_cyan;
   break;
  case c_magenta:
   HC = h_magenta;
   break;
  case c_brown:
   HC = h_brown;
   break;
  case c_yellow:
   HC = h_yellow;
   break;
  case c_ltred:
   HC = h_ltred;
   break;
  case c_ltgreen:
   HC = h_ltgreen;
   break;
  case c_ltblue:
   HC = h_ltblue;
   break;
  case c_ltcyan:
   HC = h_ltcyan;
   break;
  case c_pink:
   HC = h_pink;
   break;
 }
 attron(HC);
 mvaddch(y, x, ch);
 attroff(HC);
}

void mvwputch_hi(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC;
 switch (FG) {
  case c_white:
   HC = h_white;
   break;
  case c_ltgray:
   HC = h_ltgray;
   break;
  case c_dkgray:
   HC = h_dkgray;
   break;
  case c_red:
   HC = h_red;
   break;
  case c_green:
   HC = h_green;
   break;
  case c_blue:
   HC = h_blue;
   break;
  case c_cyan:
   HC = h_cyan;
   break;
  case c_magenta:
   HC = h_magenta;
   break;
  case c_brown:
   HC = h_brown;
   break;
  case c_yellow:
   HC = h_yellow;
   break;
  case c_ltred:
   HC = h_ltred;
   break;
  case c_ltgreen:
   HC = h_ltgreen;
   break;
  case c_ltblue:
   HC = h_ltblue;
   break;
  case c_ltcyan:
   HC = h_ltcyan;
   break;
  case c_pink:
   HC = h_pink;
   break;
 }
 wattron(w, HC);
 mvwaddch(w, y, x, ch);
 wattroff(w, HC);
}

void mvprintz(int y, int x, nc_color FG, const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
 attron(FG);
 mvprintw(y, x, buff);
 attroff(FG);
}

void mvwprintz(WINDOW* w, int y, int x, nc_color FG, const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
// char buff[4096];
// vsprintf(buff, mes, ap);
 char buff[6000];
 vsprintf(buff, mes, ap);
 wattron(w, FG);
// wmove(w, y, x);
 mvwprintw(w, y, x, buff);
 wattroff(w, FG);
 va_end(ap);
}

void printz(nc_color FG, const char *mes, ...)
{
 va_list ap;
 va_start(ap,mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
 attron(FG);
 printw(buff);
 attroff(FG);
}

void wprintz(WINDOW *w, nc_color FG, const char *mes, ...)
{
 va_list ap;
 va_start(ap,mes);
 char buff[6000];
 vsprintf(buff, mes, ap);
 va_end(ap);
 wattron(w, FG);
 wprintw(w, buff);
 wattroff(w, FG);
}

void debugmsg(const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 attron(c_red);
 mvprintw(0, 0, "DEBUG: %s                \n  Press spacebar...", buff);
 while(getch() != ' ');
 attroff(c_red);
}

bool query_yn(const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 int win_width = strlen(buff) + 10;
 WINDOW* w = newwin(3, win_width, 11, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s (Y/N)", buff);
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while (ch != 'Y' && ch != 'N');
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
 if (ch == 'Y')
  return true;
 return false;
}

std::string string_input_popup(const char *mes, ...)
{
 std::string ret;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 int startx = strlen(buff) + 2;
 WINDOW* w = newwin(3, 80, 11, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s", buff);
 for (int i = startx + 1; i < 79; i++)
  mvwputch(w, 1, i, c_ltgray, '_');
 int posx = startx;
 mvwputch(w, 1, posx, h_ltgray, '_');
 do {
  wrefresh(w);
  long ch = getch();
  if (ch == 27) {	// Escape
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return "";
  } else if (ch == '\n') {
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return ret;
  } else if (ch == KEY_BACKSPACE && posx > startx) {
   ret = ret.substr(0, ret.size() - 1);
   mvwputch(w, 1, posx, c_ltgray, '_');
   posx--;
   mvwputch(w, 1, posx, h_ltgray, '_');
  } else {
   ret += ch;
   mvwputch(w, 1, posx, c_magenta, ch);
   posx++;
   mvwputch(w, 1, posx, h_ltgray, '_');
  }
 } while (true);
}

int menu_vec(const char *mes, std::vector<std::string> options)
{
 if (options.size() == 0) {
  debugmsg("0-length menu (\"%s\")", mes);
  return -1;
 }
 std::string title = mes;
 int height = 3 + options.size(), width = title.length() + 2;
 for (int i = 0; i < options.size(); i++) {
  if (options[i].length() + 6 > width)
   width = options[i].length() + 6;
 }
 WINDOW* w = newwin(height, width, 6, 10);
 wattron(w, c_white);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintw(w, 1, 1, title.c_str());
 for (int i = 0; i < options.size(); i++)
  mvwprintw(w, i + 2, 1, "%d: %s", i + 1, options[i].c_str());
 long ch;
 wrefresh(w);
 do
  ch = getch();
 while (ch < '1' || ch >= '1' + options.size());
 werase(w);
 wrefresh(w);
 delwin(w);
 return (ch - '1' + 1);
}

int menu(const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char* tmp;
 std::vector<std::string> options;
 bool done = false;
 while (!done) {
  tmp = va_arg(ap, char*);
  if (tmp != NULL) {
   std::string strtmp = tmp;
   options.push_back(strtmp);
  } else
   done = true;
 }
 return (menu_vec(mes, options));
}

void popup_top(const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 std::string tmp = buff;
 int width = 0;
 int height = 2;
 size_t pos = tmp.find_first_of('\n');
 while (pos != std::string::npos) {
  height++;
  if (pos > width)
   width = pos;
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 if (width == 0 || tmp.length() > width)
  width = tmp.length();
 width += 2;
 WINDOW* w = newwin(height + 1, width, 0, int((80 - width) / 2));
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 tmp = buff;
 pos = tmp.find_first_of('\n');
 int line_num = 0;
 while (pos != std::string::npos) {
  std::string line = tmp.substr(0, pos);
  line_num++;
  mvwprintz(w, line_num, 1, c_white, line.c_str());
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 line_num++;
 mvwprintz(w, line_num, 1, c_white, tmp.c_str());
 
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void popup(const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
 vsprintf(buff, mes, ap);
 va_end(ap);
 std::string tmp = buff;
 int width = 0;
 int height = 2;
 size_t pos = tmp.find_first_of('\n');
 while (pos != std::string::npos) {
  height++;
  if (pos > width)
   width = pos;
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 if (width == 0 || tmp.length() > width)
  width = tmp.length();
 width += 2;
 if (height > 25)
  height = 25;
 WINDOW* w = newwin(height + 1, width, int((25 - height) / 2),
                    int((80 - width) / 2));
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 tmp = buff;
 pos = tmp.find_first_of('\n');
 int line_num = 0;
 while (pos != std::string::npos) {
  std::string line = tmp.substr(0, pos);
  line_num++;
  mvwprintz(w, line_num, 1, c_white, line.c_str());
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 line_num++;
 mvwprintz(w, line_num, 1, c_white, tmp.c_str());
 
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void full_screen_popup(const char* mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
 vsprintf(buff, mes, ap);
 va_end(ap);
 std::string tmp = buff;
 WINDOW* w = newwin(25, 80, 0, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 size_t pos = tmp.find_first_of('\n');
 int line_num = 0;
 while (pos != std::string::npos) {
  std::string line = tmp.substr(0, pos);
  line_num++;
  mvwprintz(w, line_num, 1, c_white, line.c_str());
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 line_num++;
 mvwprintz(w, line_num, 1, c_white, tmp.c_str());
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

nc_color hilite(nc_color c)
{
 switch (c) {
  case c_white:		return h_white;
  case c_ltgray:	return h_ltgray;
  case c_dkgray:	return h_dkgray;
  case c_red:		return h_red;
  case c_green:		return h_green;
  case c_blue:		return h_blue;
  case c_cyan:		return h_cyan;
  case c_magenta:	return h_magenta;
  case c_brown:		return h_brown;
  case c_ltred:		return h_ltred;
  case c_ltgreen:	return h_ltgreen;
  case c_ltblue:	return h_ltblue;
  case c_ltcyan:	return h_ltcyan;
  case c_pink:		return h_pink;
  case c_yellow:	return h_yellow;
 }
 return h_white;
}

nc_color rand_color()
{
 switch (rng(0, 9)) {
  case 0:	return c_white;
  case 1:	return c_ltgray;
  case 2:	return c_green;
  case 3:	return c_red;
  case 4:	return c_yellow;
  case 5:	return c_blue;
  case 6:	return c_ltblue;
  case 7:	return c_pink;
  case 8:	return c_magenta;
  case 9:	return c_brown;
 }
 return c_dkgray;
}

char rand_char()
{
 switch (rng(0, 9)) {
  case 0:	return '|';
  case 1:	return '-';
  case 2:	return '#';
  case 3:	return '?';
  case 4:	return '&';
  case 5:	return '.';
  case 6:	return '%';
  case 7:	return '{';
  case 8:	return '*';
  case 9:	return '^';
 }
}
#endif
