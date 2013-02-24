#include <string>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "color.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "options.h"
#include "cursesdef.h"

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

nc_color invert_color(nc_color c)
{
 if (OPTIONS[OPT_NO_CBLINK]) {
  switch (c) {
   case c_white:
   case c_ltgray:
   case c_dkgray:  return i_ltgray;
   case c_red:
   case c_ltred:   return i_red;
   case c_green:
   case c_ltgreen: return i_green;
   case c_blue:
   case c_ltblue:  return i_blue;
   case c_cyan:
   case c_ltcyan:  return i_cyan;
   case c_magenta:
   case c_pink:    return i_magenta;
   case c_brown:
   case c_yellow:  return i_brown;
  }
 }
 switch (c) {
  case c_white:   return i_white;
  case c_ltgray:  return i_ltgray;
  case c_dkgray:  return i_dkgray;
  case c_red:     return i_red;
  case c_green:   return i_green;
  case c_blue:    return i_blue;
  case c_cyan:    return i_cyan;
  case c_magenta: return i_magenta;
  case c_brown:   return i_brown;
  case c_yellow:  return i_yellow;
  case c_ltred:   return i_ltred;
  case c_ltgreen: return i_ltgreen;
  case c_ltblue:  return i_ltblue;
  case c_ltcyan:  return i_ltcyan;
  case c_pink:    return i_pink;
 }

 return c_pink;
}

nc_color red_background(nc_color c)
{
 switch (c) {
  case c_white:		return c_white_red;
  case c_ltgray:	return c_ltgray_red;
  case c_dkgray:	return c_dkgray_red;
  case c_red:		return c_red_red;
  case c_green:		return c_green_red;
  case c_blue:		return c_blue_red;
  case c_cyan:		return c_cyan_red;
  case c_magenta:	return c_magenta_red;
  case c_brown:		return c_brown_red;
  case c_ltred:		return c_ltred_red;
  case c_ltgreen:	return c_ltgreen_red;
  case c_ltblue:	return c_ltblue_red;
  case c_ltcyan:	return c_ltcyan_red;
  case c_pink:		return c_pink_red;
  case c_yellow:	return c_yellow_red;
 }
 return c_white_red;
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
 nc_color HC = invert_color(FG);
 attron(HC);
 mvaddch(y, x, ch);
 attroff(HC);
}

void mvwputch_inv(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC = invert_color(FG);
 wattron(w, HC);
 mvwaddch(w, y, x, ch);
 wattroff(w, HC);
}

void mvputch_hi(int y, int x, nc_color FG, long ch)
{
 nc_color HC = hilite(FG);
 attron(HC);
 mvaddch(y, x, ch);
 attroff(HC);
}

void mvwputch_hi(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC = hilite(FG);
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

void draw_tabs(WINDOW *w, int active_tab, ...)
{
 int win_width;
 win_width = getmaxx(w);
 std::vector<std::string> labels;
 va_list ap;
 va_start(ap, active_tab);
 char *tmp;
 while ((tmp = va_arg(ap, char *)))
  labels.push_back((std::string)(tmp));
 va_end(ap);

// Draw the line under the tabs
 for (int x = 0; x < win_width; x++)
  mvwputch(w, 2, x, c_white, LINE_OXOX);

 int total_width = 0;
 for (int i = 0; i < labels.size(); i++)
  total_width += labels[i].length() + 6; // "< |four| >"

 if (total_width > win_width) {
  //debugmsg("draw_tabs not given enough space! %s", labels[0]);
  return;
 }

// Extra "buffer" space per each side of each tab
 double buffer_extra = (win_width - total_width) / (labels.size() * 2);
 int buffer = int(buffer_extra);
// Set buffer_extra to (0, 1); the "extra" whitespace that builds up
 buffer_extra = buffer_extra - buffer;
 int xpos = 0;
 double savings = 0;

 for (int i = 0; i < labels.size(); i++) {
  int length = labels[i].length();
  xpos += buffer + 2;
  savings += buffer_extra;
  if (savings > 1) {
   savings--;
   xpos++;
  }
  mvwputch(w, 0, xpos, c_white, LINE_OXXO);
  mvwputch(w, 1, xpos, c_white, LINE_XOXO);
  mvwputch(w, 0, xpos + length + 1, c_white, LINE_OOXX);
  mvwputch(w, 1, xpos + length + 1, c_white, LINE_XOXO);
  if (i == active_tab) {
   mvwputch(w, 1, xpos - 2, h_white, '<');
   mvwputch(w, 1, xpos + length + 3, h_white, '>');
   mvwputch(w, 2, xpos, c_white, LINE_XOOX);
   mvwputch(w, 2, xpos + length + 1, c_white, LINE_XXOO);
   mvwprintz(w, 1, xpos + 1, h_white, labels[i].c_str());
   for (int x = xpos + 1; x <= xpos + length; x++) {
    mvwputch(w, 0, x, c_white, LINE_OXOX);
    mvwputch(w, 2, x, c_black, 'x');
   }
  } else {
   mvwputch(w, 2, xpos, c_white, LINE_XXOX);
   mvwputch(w, 2, xpos + length + 1, c_white, LINE_XXOX);
   mvwprintz(w, 1, xpos + 1, c_white, labels[i].c_str());
   for (int x = xpos + 1; x <= xpos + length; x++)
    mvwputch(w, 0, x, c_white, LINE_OXOX);
  }
  xpos += length + 1 + buffer;
 }
}

void realDebugmsg(const char* filename, const char* line, const char *mes, ...)
{
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 attron(c_red);
 mvprintw(0, 0, "DEBUG: %s                \n  Press spacebar...", buff);
 std::ofstream fout;
 fout.open("debug.log", std::ios_base::app | std::ios_base::out);
 fout << filename << "[" << line << "]: " << buff << "\n";
 fout.close();
 while(getch() != ' ') ;
;
 attroff(c_red);
}

bool query_yn(const char *mes, ...)
{
 bool force_uc = OPTIONS[OPT_FORCE_YN];
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
 vsprintf(buff, mes, ap);
 va_end(ap);
 int win_width = strlen(buff) + 26;
 WINDOW* w = newwin(3, win_width, 11, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s (%s)", buff,
           (force_uc ? "Y/N - Case Sensitive" : "y/n"));
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while (ch != 'Y' && ch != 'N' && (force_uc || (ch != 'y' && ch != 'n')));
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
 if (ch == 'Y' || ch == 'y')
  return true;
 return false;
}

int query_int(const char *mes, ...)
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
 mvwprintz(w, 1, 1, c_ltred, "%s (0-9)", buff);
 wrefresh(w);

 int temp;
 do
  temp = getch();
 while ((temp-48)<0 || (temp-48)>9);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();

 return temp-48;
}

std::string string_input_popup(std::string title, int max_length, std::string input)
{
 std::string ret = input;

 int startx = title.size() + 2;
 WINDOW* w = newwin(3, 80, 11, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 for (int i = startx + 1; i < 79; i++)
  mvwputch(w, 1, i, c_ltgray, '_');

 mvwprintz(w, 1, 1, c_ltred, "%s", title.c_str());

 if (input != "")
  mvwprintz(w, 1, startx, c_magenta, "%s", input.c_str());

 int posx = startx + input.size();
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
  } else if ((ch == KEY_BACKSPACE || ch == 127) && posx > startx) {
// Move the cursor back and re-draw it
   ret = ret.substr(0, ret.size() - 1);
   mvwputch(w, 1, posx, c_ltgray, '_');
   posx--;
   mvwputch(w, 1, posx, h_ltgray, '_');
  } else if(ret.size() < max_length || max_length == 0) {
   ret += ch;
   mvwputch(w, 1, posx, c_magenta, ch);
   posx++;
   mvwputch(w, 1, posx, h_ltgray, '_');
  }
 } while (true);
}

char popup_getkey(const char *mes, ...)
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
 char ch = getch();;
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
 return ch;
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
 WINDOW* w = newwin(height, width, 1, 10);
 wattron(w, c_white);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintw(w, 1, 1, title.c_str());
 for (int i = 0; i < options.size(); i++)
  mvwprintw(w, i + 2, 1, "%c: %s", (i < 9? i + '1' :
                                   (i == 9? '0' : 'a' + i - 10)),
            options[i].c_str());
 long ch;
 wrefresh(w);
 int res;
 do
 {
  ch = getch();
  if (ch >= '1' && ch <= '9')
   res = ch - '1' + 1;
  else
  if (ch == '0')
   res = 10;
  else
  if (ch >= 'a' && ch <= 'z')
   res = ch - 'a' + 11;
  else
   res = -1;
  if (res > options.size())
   res = -1;
 }
 while (res == -1);
 werase(w);
 wrefresh(w);
 delwin(w);
 return (res);
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

void popup_nowait(const char *mes, ...)
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

void compare_split_screen_popup(bool bLeft, std::string sItemName, std::vector<iteminfo> vItemDisplay, std::vector<iteminfo> vItemCompare)
{
 WINDOW* w = newwin(25, 40, 0, (bLeft) ? 0 : 40);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 mvwprintz(w, 1, 2, c_white, sItemName.c_str());
 int line_num = 3;

 std::string sPlus;
 bool bStartNewLine = true;
 for (int i = 0; i < vItemDisplay.size(); i++) {
  if (vItemDisplay[i].sType == "DESCRIPTION") {
   std::string sText = vItemDisplay[i].sName;
   std::replace(sText.begin(), sText.end(), '\n', ' ');
   int iPos;
   while (1) {
     line_num++;
     if (sText.size() > 36) {
      int iPos = sText.find_last_of(' ', 36);
      mvwprintz(w, line_num, 2, c_white, (sText.substr(0, iPos)).c_str());
      sText = sText.substr(iPos+1, sText.size());
     } else {
      mvwprintz(w, line_num, 2, c_white, (sText).c_str());
      break;
     }
   }
  } else {
   if (bStartNewLine) {
    mvwprintz(w, line_num, 2, c_white, "%s", (vItemDisplay[i].sName).c_str());
    bStartNewLine = false;
   } else {
    wprintz(w, c_white, "%s", (vItemDisplay[i].sName).c_str());
   }

   sPlus = "";
   std::string sPre = vItemDisplay[i].sPre;
   if (sPre.size() > 1 && sPre.substr(sPre.size()-1, 1) == "+") {
     wprintz(w, c_white, "%s", (sPre.substr(0, sPre.size()-1)).c_str());
     sPlus = "+";
   } else if (sPre != "+")
     wprintz(w, c_white, "%s", sPre.c_str());
   else if (sPre == "+")
    sPlus = "+";

   if (vItemDisplay[i].iValue != -999) {
    nc_color thisColor = c_white;
    for (int k = 0; k < vItemCompare.size(); k++) {
     if (vItemCompare[k].iValue != -999) {
      if (vItemDisplay[i].sName == vItemCompare[k].sName) {
       if (vItemDisplay[i].iValue == vItemCompare[k].iValue) {
         thisColor = c_white;
       } else if (vItemDisplay[i].iValue > vItemCompare[k].iValue) {
        if (vItemDisplay[i].bLowerIsBetter) {
         thisColor = c_ltred;
        } else {
         thisColor = c_ltgreen;
        }
       } else if (vItemDisplay[i].iValue < vItemCompare[k].iValue) {
        if (vItemDisplay[i].bLowerIsBetter) {
         thisColor = c_ltgreen;
        } else {
         thisColor = c_ltred;
        }
       }
       break;
      }
     }
    }

    if (sPlus == "+" )
     wprintz(w, thisColor, "%s", (sPlus).c_str());
    wprintz(w, thisColor, "%d", vItemDisplay[i].iValue);
   }
   wprintz(w, c_white, (vItemDisplay[i].sPost).c_str());

   if (vItemDisplay[i].bNewLine) {
    line_num++;
    bStartNewLine = true;
   }
  }
 }

 wrefresh(w);
 if (!bLeft)
 {
  char ch;
  do
   ch = getch();
  while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
 }
 /*
 std::string tmp = buff;
 WINDOW* w = newwin(25, 40, 0, (bLeft) ? 0 : 40);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 size_t pos = tmp.find_first_of('\n');
 mvwprintz(w, 1, 2, c_white, sItemName.c_str());
 int line_num = 2;
 while (pos != std::string::npos) {
  std::string line = tmp.substr(0, pos);
  line_num++;
  if (line.size() > 36)
  {
    std::string sTemp;
    do
    {
      sTemp += line;
      tmp = tmp.substr(pos + 1);
      pos = tmp.find_first_of('\n');
      line = " " + tmp.substr(0, pos);
    } while (pos != std::string::npos);
    sTemp += line;
    while (sTemp.size() > 0)
    {
      size_t iPos = sTemp.find_last_of(' ', 36);
      if (iPos == 0)
        iPos = sTemp.size();
      mvwprintz(w, line_num, 2, c_white, (sTemp.substr(0, iPos)).c_str());
      line_num++;
      sTemp = sTemp.substr(iPos+1, sTemp.size());
    }
  }
  else
  {
    mvwprintz(w, line_num, 2, c_white, (line).c_str());
  }
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 line_num++;
 mvwprintz(w, line_num, 2, c_white, tmp.c_str());
 wrefresh(w);
 if (!bLeft)
 {
  char ch;
  do
   ch = getch();
  while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
 }
 */
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
 return '?';
}

// this translates symbol y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
long special_symbol (char sym)
{
    switch (sym)
    {
    case 'j': return LINE_XOXO;
    case 'h': return LINE_OXOX;
    case 'c': return LINE_XXXX;
    case 'y': return LINE_OXXO;
    case 'u': return LINE_OOXX;
    case 'n': return LINE_XOOX;
    case 'b': return LINE_XXOO;
    default: return sym;
    }
}

// crawl through string, probing each word while treating spaces & newlines the same.
std::string word_rewrap (const std::string &in, int width){
    std::ostringstream o;
    int i_ok = 0; // pos in string of next char probe
    int x_ok = 0; // ditto for column pos
    while (i_ok <= in.size()){
        bool fit = false;
        int j=0; // j = word probe counter.
        while(x_ok + j <= width){
           if (i_ok + j >= in.size()){
              fit = true;
              break;
           }
           char c = in[i_ok+j];
           if (c == '\n' || c == ' '){ //whitespace detected. copy word.
              fit = true;
              break;
           }
           j++;
        }
        if(fit == false){
           o << '\n';
           x_ok = 0;
        }
        else {
           o << ' ';
           for (int k=i_ok; k < i_ok+j; k++){
              o << in[k];
           }
           i_ok += j+1;
           x_ok += j+1;
        }
    }
    return o.str();
}





