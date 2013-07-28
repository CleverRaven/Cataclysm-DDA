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
#include "catacharset.h"
#include "debug.h"

// Display data
int TERMX;
int TERMY;
int VIEWX;
int VIEWY;
int VIEW_OFFSET_X;
int VIEW_OFFSET_Y;
int TERRAIN_WINDOW_WIDTH;
int TERRAIN_WINDOW_HEIGHT;

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
/// colors need to be totally redone, really..
nc_color white_background(nc_color c) {
  switch(c) {
     case c_black: return c_black_white;
     case c_dkgray: return c_dkgray_white;
     case c_ltgray: return c_ltgray_white;
     case c_white: return c_white_white;
     case c_red: return c_red_white;
     case c_ltred: return c_ltred_white;
     case c_green: return c_green_white;
     case c_ltgreen: return c_ltgreen_white;
     case c_brown: return c_brown_white;
     case c_yellow: return c_yellow_white;
     case c_blue: return c_blue_white;
     case c_ltblue: return c_ltblue_white;
     case c_magenta: return c_magenta_white;
     case c_pink: return c_pink_white;
     case c_cyan: return c_cyan_white;
     case c_ltcyan: return c_ltcyan_white;
     default: return c_black_white;
   }
}
 nc_color green_background(nc_color c) {
  switch(c) {
     case c_black: return c_black_green;
     case c_dkgray: return c_dkgray_green;
     case c_ltgray: return c_ltgray_green;
     case c_white: return c_white_green;
     case c_red: return c_red_green;
     case c_ltred: return c_ltred_green;
     case c_green: return c_green_green;
     case c_ltgreen: return c_ltgreen_green;
     case c_brown: return c_brown_green;
     case c_yellow: return c_yellow_green;
     case c_blue: return c_blue_green;
     case c_ltblue: return c_ltblue_green;
     case c_magenta: return c_magenta_green;
     case c_pink: return c_pink_green;
     case c_cyan: return c_cyan_green;
     case c_ltcyan: return c_ltcyan_green;
     default: return c_black_green;
   }
}
 nc_color yellow_background(nc_color c) {
  switch(c) {
     case c_black: return c_black_yellow;
     case c_dkgray: return c_dkgray_yellow;
     case c_ltgray: return c_ltgray_yellow;
     case c_white: return c_white_yellow;
     case c_red: return c_red_yellow;
     case c_ltred: return c_ltred_yellow;
     case c_green: return c_green_yellow;
     case c_ltgreen: return c_ltgreen_yellow;
     case c_brown: return c_brown_yellow;
     case c_yellow: return c_yellow_yellow;
     case c_blue: return c_blue_yellow;
     case c_ltblue: return c_ltblue_yellow;
     case c_magenta: return c_magenta_yellow;
     case c_pink: return c_pink_yellow;
     case c_cyan: return c_cyan_yellow;
     case c_ltcyan: return c_ltcyan_yellow;
     default: return c_black_yellow;
   }
}
 nc_color magenta_background(nc_color c) {
  switch(c) {
     case c_black: return c_black_magenta;
     case c_dkgray: return c_dkgray_magenta;
     case c_ltgray: return c_ltgray_magenta;
     case c_white: return c_white_magenta;
     case c_red: return c_red_magenta;
     case c_ltred: return c_ltred_magenta;
     case c_green: return c_green_magenta;
     case c_ltgreen: return c_ltgreen_magenta;
     case c_brown: return c_brown_magenta;
     case c_yellow: return c_yellow_magenta;
     case c_blue: return c_blue_magenta;
     case c_ltblue: return c_ltblue_magenta;
     case c_magenta: return c_magenta_magenta;
     case c_pink: return c_pink_magenta;
     case c_cyan: return c_cyan_magenta;
     case c_ltcyan: return c_ltcyan_magenta;
     default: return c_black_magenta;
   }
}
 nc_color cyan_background(nc_color c) {
  switch(c) {
     case c_black: return c_black_cyan;
     case c_dkgray: return c_dkgray_cyan;
     case c_ltgray: return c_ltgray_cyan;
     case c_white: return c_white_cyan;
     case c_red: return c_red_cyan;
     case c_ltred: return c_ltred_cyan;
     case c_green: return c_green_cyan;
     case c_ltgreen: return c_ltgreen_cyan;
     case c_brown: return c_brown_cyan;
     case c_yellow: return c_yellow_cyan;
     case c_blue: return c_blue_cyan;
     case c_ltblue: return c_ltblue_cyan;
     case c_magenta: return c_magenta_cyan;
     case c_pink: return c_pink_cyan;
     case c_cyan: return c_cyan_cyan;
     case c_ltcyan: return c_ltcyan_cyan;
     default: return c_black_cyan;
   }
}


///
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

// utf8 version
std::vector<std::string> foldstring ( std::string str, int width ) {
    std::vector<std::string> lines;
    if ( width < 1 ) {
        lines.push_back( str );
        return lines;
    }
    std::stringstream sstr(str);
    std::string strline;
    while (std::getline(sstr, strline, '\n')) {
        std::string wrapped = word_rewrap(strline, width);
        std::stringstream swrapped(wrapped);
        std::string wline;
        while (std::getline(swrapped, wline, '\n')) {
            lines.push_back(wline);
        }
    }
    return lines;
}

// returns number of printed lines
int fold_and_print(WINDOW* w, int begin_y, int begin_x, int width, nc_color color, const char *mes, ...)
{
    va_list ap;
    va_start(ap,mes);
    char buff[6000];    //TODO replace Magic Number
    vsprintf(buff, mes, ap);
    va_end(ap);

    std::vector<std::string> textformatted;
    textformatted = foldstring(buff, width);
    for (int line_num=0; line_num<textformatted.size(); line_num++)
    {
        mvwprintz(w, line_num+begin_y, begin_x, color, "%s", textformatted[line_num].c_str());
    }
    return textformatted.size();
};

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
 printw("%s", buff);
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
 wprintw(w, "%s", buff);
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
 int win_width = utf8_width(buff) + 26;

 WINDOW *w = newwin(3, win_width, (TERMY-3)/2, (TERMX > win_width) ? (TERMX-win_width)/2 : 0);

 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s (%s)", buff, (force_uc ? "Y/N - Case Sensitive" : "y/n"));
 wrefresh(w);
 char ch;
 do
  ch = getch();
 while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE && ch != 'Y' && ch != 'N' && (force_uc || (ch != 'y' && ch != 'n')));
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
 int win_width = utf8_width(buff) + 10;

 WINDOW *w = newwin(3, win_width, (TERMY-3)/2, 11+((TERMX > win_width) ? (TERMX-win_width)/2 : 0));

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

 int startx = utf8_width(title.c_str()) + 2;
 int iPopupWidth = (max_length == 0) ? FULL_SCREEN_WIDTH : max_length + title.size() + 4;
 if (iPopupWidth > FULL_SCREEN_WIDTH) {
     iPopupWidth = FULL_SCREEN_WIDTH;
     max_length = FULL_SCREEN_WIDTH - title.size() - 4;
 }

 WINDOW *w = newwin(3, iPopupWidth, (TERMY-3)/2,
                    ((TERMX > iPopupWidth) ? (TERMX-iPopupWidth)/2 : 0));

 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 for (int i = startx + 1; i < iPopupWidth-1; i++)
  mvwputch(w, 1, i, c_ltgray, '_');

 mvwprintz(w, 1, 1, c_ltred, "%s", title.c_str());

 if (input != "")
  mvwprintz(w, 1, startx, c_magenta, "%s", input.c_str());

 int posx = startx + utf8_width(input.c_str());
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
  } else if (ch == KEY_BACKSPACE || ch == 127) {
// Move the cursor back and re-draw it
   if( posx > startx ) { // but silently drop input if we're at 0, instead of adding '^'
       ret = ret.substr(0, ret.size() - 1);
       mvwputch(w, 1, posx, c_ltgray, '_');
       posx--;
       mvwputch(w, 1, posx, h_ltgray, '_');
   }
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
 if (height > FULL_SCREEN_HEIGHT)
  height = FULL_SCREEN_HEIGHT;
 WINDOW *w = newwin(height+1, width, (TERMY-(height+1))/2, (TERMX > width) ? (TERMX-width)/2 : 0);
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

int menu_vec(bool cancelable, const char *mes, std::vector<std::string> options) { // compatibility stub for uimenu(cancelable, mes, options)
  return (int)uimenu(cancelable, mes, options);
}

int menu(bool cancelable, const char *mes, ...) { // compatibility stub for uimenu(cancelable, mes, ...)
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
    } else {
      done = true;
    }
  }
  return (uimenu(cancelable, mes, options));
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
 WINDOW *w = newwin(height+1, width, 0, (TERMX > width) ? (TERMX-width)/2 : 0);
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
 if (height > FULL_SCREEN_HEIGHT)
  height = FULL_SCREEN_HEIGHT;
 WINDOW *w = newwin(height+1, width, (TERMY-(height+1))/2, (TERMX > width) ? (TERMX-width)/2 : 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 fold_and_print(w,1,1,width,c_white, "%s", buff);

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
 if (height > FULL_SCREEN_HEIGHT)
  height = FULL_SCREEN_HEIGHT;
 WINDOW *w = newwin(height+1, width, (TERMY-(height+1))/2, (TERMX > width) ? (TERMX-width)/2 : 0);
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
 std::vector<std::string> textformatted;

 WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 fold_and_print(w,1,2,80-3,c_white,"%s",mes);
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

//note that passing in iteminfo instances with sType == "MENU" or "DESCRIPTION" does special things
//if sType == "MENU", sFmt == "iOffsetY" or "iOffsetX" also do special things
//otherwise if sType == "MENU", iValue can be used to control color
//all this should probably be cleaned up at some point, rather than using a function for things it wasn't meant for
// well frack, half the game uses it so: optional (int)selected argument causes entry highlight, and enter to return entry's key. Also it now returns int
int compare_split_screen_popup(int iLeft, int iWidth, int iHeight, std::string sItemName, std::vector<iteminfo> vItemDisplay, std::vector<iteminfo> vItemCompare, int selected)
{
 WINDOW* w = newwin(iHeight, iWidth, VIEW_OFFSET_Y, iLeft + VIEW_OFFSET_X);

 mvwprintz(w, 1, 2, c_white, sItemName.c_str());
 int line_num = 3;
 int iStartX = 0;
 bool bStartNewLine = true;
 int selected_ret='\n';
 std::string spaces(iWidth-2, ' ');
 for (int i = 0; i < vItemDisplay.size(); i++) {
  if (vItemDisplay[i].sType == "MENU") {
   if (vItemDisplay[i].sFmt == "iOffsetY") {
    line_num += vItemDisplay[i].iValue;
   } else if (vItemDisplay[i].sFmt == "iOffsetX") {
    iStartX = vItemDisplay[i].iValue;
   } else {
    nc_color nameColor = c_ltgreen; //pre-existing behavior, so make it the default
    //patched to allow variable "name" coloring, e.g. for item examining
    nc_color bgColor = c_white;     //yes the name makes no sense
    if (vItemDisplay[i].iValue >= 0) {
     nameColor = vItemDisplay[i].iValue == 0 ? c_ltgray : c_ltred;
    }
    if ( i == selected && vItemDisplay[i].sName != "" ) {
      bgColor = h_white;
      selected_ret=(int)vItemDisplay[i].sName.c_str()[0]; // fixme: sanity check(?)
    }
    mvwprintz(w, line_num, 1, bgColor, "%s", spaces.c_str() );
    shortcut_print(w, line_num, iStartX, bgColor, nameColor, vItemDisplay[i].sFmt.c_str());
    line_num++;
   }
  } else if (vItemDisplay[i].sType == "DESCRIPTION") {
    std::string sText = word_rewrap(vItemDisplay[i].sName, iWidth-4);
	std::stringstream ss(sText);
	std::string l;
    while (std::getline(ss, l, '\n')) {
      line_num++;
	  mvwprintz(w, line_num, 2, c_white, l.c_str());
    }
  } else {
   if (bStartNewLine) {
    mvwprintz(w, line_num, 2, c_white, "%s", (vItemDisplay[i].sName).c_str());
    bStartNewLine = false;
   } else {
    wprintz(w, c_white, "%s", (vItemDisplay[i].sName).c_str());
   }

   std::string sPlus = vItemDisplay[i].sPlus;
   std::string sFmt = vItemDisplay[i].sFmt;
   std::string sNum = " ";
   std::string sPost = "";

    //A bit tricky, find %d and split the string
   size_t pos = sFmt.find("<num>");
   if(pos != std::string::npos)
   {
        wprintz(w, c_white, sFmt.substr(0, pos).c_str());
        sPost = sFmt.substr(pos+5);
   }
   else
   {
        wprintz(w, c_white, sFmt.c_str());
   }

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
    wprintz(w, thisColor, "%s%d", sPlus.c_str(), vItemDisplay[i].iValue);
   }
    wprintz(w, c_white, sPost.c_str());

   if (vItemDisplay[i].bNewLine) {
    line_num++;
    bStartNewLine = true;
   }
  }
 }

 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 int ch = (int)' ';

 wrefresh(w);
 if (iLeft > 0) {
  ch = (int)getch();
  if ( selected > 0 && ( ch == '\n' || ch == KEY_RIGHT ) && selected_ret != 0 ) {
    ch=selected_ret;
  } else if ( selected == KEY_LEFT ) {
    ch=(int)' ';
  }
  //werase(w);
  //wrefresh(w);
  delwin(w);
  //refresh();
 }

 return ch;
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
long special_symbol (long sym)
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

// utf-8 version
std::string word_rewrap (const std::string &ins, int width){
    std::ostringstream o;
	std::string in = ins;
	std::replace(in.begin(), in.end(), '\n', ' ');
	int lastwb = 0; //last word break
	int lastout = 0;
	int x=0;
	int j=0;
	const char *instr = in.c_str();

	while(j<in.size())
	{
		const char* ins = instr+j;
		int len = ANY_LENGTH;
		unsigned uc = UTF8_getch(&ins, &len);
		int cw = mk_wcwidth((wchar_t)uc);
		len = ANY_LENGTH-len;

		j+=len;
		x+=cw;

		if(x>width)
		{
			for(int k=lastout; k<lastwb; k++)
				o << in[k];
			o<<'\n';
			x=0;
			lastout=j=lastwb;
		}
		else if(uc==' '|| uc=='\n')
		{
			lastwb=j;
		}
		else if(uc>=0x2E80)
		{
			lastwb=j;
		}
	}
	for(int k=lastout; k<in.size(); k++)
		o << in[k];

    return o.str();
}

void draw_tab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected)
{
 int iOffsetXRight = iOffsetX + utf8_width(sText.c_str()) + 1;

 mvwputch(w, 0, iOffsetX,      c_ltgray, LINE_OXXO); // |^
 mvwputch(w, 0, iOffsetXRight, c_ltgray, LINE_OOXX); // ^|
 mvwputch(w, 1, iOffsetX,      c_ltgray, LINE_XOXO); // |
 mvwputch(w, 1, iOffsetXRight, c_ltgray, LINE_XOXO); // |

 mvwprintz(w, 1, iOffsetX+1, (bSelected) ? h_ltgray : c_ltgray, sText.c_str());

 for (int i = iOffsetX+1; i < iOffsetXRight; i++)
  mvwputch(w, 0, i, c_ltgray, LINE_OXOX); // -

 if (bSelected) {
  mvwputch(w, 1, iOffsetX-1,      h_ltgray, '<');
  mvwputch(w, 1, iOffsetXRight+1, h_ltgray, '>');

  for (int i = iOffsetX+1; i < iOffsetXRight; i++)
   mvwputch(w, 2, i, c_black, ' ');

  mvwputch(w, 2, iOffsetX,      c_ltgray, LINE_XOOX); // _|
  mvwputch(w, 2, iOffsetXRight, c_ltgray, LINE_XXOO); // |_

 } else {
  mvwputch(w, 2, iOffsetX,      c_ltgray, LINE_XXOX); // _|_
  mvwputch(w, 2, iOffsetXRight, c_ltgray, LINE_XXOX); // _|_
 }
}

void hit_animation(int iX, int iY, nc_color cColor, char cTile, int iTimeout)
{
    WINDOW *w_hit = newwin(1, 1, iY+VIEW_OFFSET_Y, iX+VIEW_OFFSET_X);
    if (w_hit == NULL) {
        return; //we passed in negative values (semi-expected), so let's not segfault
    }

    mvwputch(w_hit, 0, 0, cColor, cTile);
    wrefresh(w_hit);

    if (iTimeout <= 0 || iTimeout > 999) {
        iTimeout = 70;
    }

    timeout(iTimeout);
    getch(); //useing this, because holding down a key with nanosleep can get yourself killed
    timeout(-1);
}

std::string from_sentence_case (const std::string &kingston)
{
    if (kingston.size()>0) {
        std::string montreal = kingston;
        if(montreal.empty()) {
            return "";
        } else {
            montreal.replace(0,1,1,tolower(kingston.at(0)));
            return montreal;
        }
    }
    return "";
}

std::string string_format(std::string pattern, ...)
{
    va_list ap;
    va_start(ap,pattern);
    char buff[3000];    //TODO replace Magic Number
    vsprintf(buff, pattern.c_str(), ap);
    va_end(ap);
    
    //drop contents behind $, this trick is there to skip certain arguments
    char* break_pos = strchr(buff, '$');
    if(break_pos) break_pos[0] = '\0';

    return buff;
}

//wrap if for i18n 
std::string& capitalize_first_letter(std::string &str)
{
    char c= str[0];
    if(str.length()>0 && c>='a' && c<='z')
    {
       c += 'A'-'a';
       str[0] = c;
    }

    return str;
}

// draw a menu item like strign with highlighted shortcut character
// Example: <w>ield, m<o>ve
// returns: output length (in console cells)
size_t shortcut_print(WINDOW* w, int y, int x, nc_color color, nc_color colork, const char* fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    char buff[3000];    //TODO replace Magic Number
    vsprintf(buff, fmt, ap);
    va_end(ap);
    
    std::string tmp = buff;
    size_t pos = tmp.find_first_of('<');
    size_t pos2 = tmp.find_first_of('>');
    size_t len = 0;
    if(pos2!=std::string::npos && pos<pos2)
    {
        tmp.erase(pos,1);
        tmp.erase(pos2-1,1);
        mvwprintz(w, y, x, color, tmp.c_str());
        mvwprintz(w, y, x+pos, colork, "%s", tmp.substr(pos, pos2-pos-1).c_str());
        len = utf8_width(tmp.c_str());
    }
    else
    {
        // no shutcut? 
        mvwprintz(w, y, x, color, buff);
        len = utf8_width(buff);
    }
    return len;
}
