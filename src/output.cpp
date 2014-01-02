#include <string>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "output.h"

#include "color.h"
#include "input.h"
#include "rng.h"
#include "keypress.h"
#include "options.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "debug.h"
#include "uistate.h"
#include "translations.h"

// Display data
int TERMX;
int TERMY;
int POSX;
int POSY;
int VIEW_OFFSET_X;
int VIEW_OFFSET_Y;
int TERRAIN_WINDOW_WIDTH;
int TERRAIN_WINDOW_HEIGHT;
int FULL_SCREEN_WIDTH;
int FULL_SCREEN_HEIGHT;

// utf8 version
std::vector<std::string> foldstring ( std::string str, int width )
{
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

std::vector<std::string> split_by_color(const std::string &s)
{
    std::vector<std::string> ret;
    std::vector<size_t> tag_positions = get_tag_positions(s);
    size_t last_pos = 0;
    std::vector<size_t>::iterator it;
    for (it = tag_positions.begin(); it != tag_positions.end(); ++it) {
        ret.push_back(s.substr(last_pos, *it - last_pos));
        last_pos = *it;
    }
    // and the last (or only) one
    ret.push_back(s.substr(last_pos, std::string::npos));
    return ret;
}

// returns number of printed lines
int fold_and_print(WINDOW *w, int begin_y, int begin_x, int width, nc_color base_color,
                   const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[6000];    //TODO replace Magic Number
    vsprintf(buff, mes, ap);
    va_end(ap);

    nc_color color = base_color;
    std::vector<std::string> textformatted;
    textformatted = foldstring(buff, width);
    for (int line_num = 0; line_num < textformatted.size(); line_num++) {
        wmove(w, line_num + begin_y, begin_x);
        // split into colourable sections
        std::vector<std::string> color_segments = split_by_color(textformatted[line_num]);
        // for each section, get the colour, and print it
        std::vector<std::string>::iterator it;
        for (it = color_segments.begin(); it != color_segments.end(); ++it) {
            if (!it->empty() && it->at(0) == '<') {
                color = get_color_from_tag(*it, base_color);
            }
            wprintz(w, color, "%s", rm_prefix(*it).c_str());
        }
    }
    return textformatted.size();
};

void center_print(WINDOW *w, int y, nc_color FG, const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[6000];    //TODO replace Magic Number
    vsprintf(buff, mes, ap);
    va_end(ap);

    int window_width = getmaxx(w);
    int string_width = utf8_width(buff);
    int x;
    if (string_width >= window_width) {
        x = 0;
    } else {
        x = (window_width - string_width) / 2;
    }
    mvwprintz(w, y, x, FG, buff);
}

void mvputch(int y, int x, nc_color FG, long ch)
{
    attron(FG);
    mvaddch(y, x, ch);
    attroff(FG);
}

void wputch(WINDOW *w, nc_color FG, long ch)
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

void mvwputch_inv(WINDOW *w, int y, int x, nc_color FG, long ch)
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

void mvwputch_hi(WINDOW *w, int y, int x, nc_color FG, long ch)
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
    mvprintw(y, x, "%s", buff);
    attroff(FG);
}

void mvwprintz(WINDOW *w, int y, int x, nc_color FG, const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[6000];
    vsprintf(buff, mes, ap);
    va_end(ap);
    wattron(w, FG);
    mvwprintw(w, y, x, "%s", buff);
    wattroff(w, FG);
}

void printz(nc_color FG, const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
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
    va_start(ap, mes);
    char buff[6000];
    vsprintf(buff, mes, ap);
    va_end(ap);
    wattron(w, FG);
    wprintw(w, "%s", buff);
    wattroff(w, FG);
}

void draw_border(WINDOW *w, nc_color FG)
{
    wattron(w, FG);
    wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
               LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
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
    while ((tmp = va_arg(ap, char *))) {
        labels.push_back((std::string)(tmp));
    }
    va_end(ap);

    // Draw the line under the tabs
    for (int x = 0; x < win_width; x++) {
        mvwputch(w, 2, x, c_white, LINE_OXOX);
    }

    int total_width = 0;
    for (int i = 0; i < labels.size(); i++) {
        total_width += labels[i].length() + 6;    // "< |four| >"
    }

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
            for (int x = xpos + 1; x <= xpos + length; x++) {
                mvwputch(w, 0, x, c_white, LINE_OXOX);
            }
        }
        xpos += length + 1 + buffer;
    }
}

void realDebugmsg(const char *filename, const char *line, const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[4096];
//[1024];
    vsprintf(buff, mes, ap);
    va_end(ap);
    fold_and_print(stdscr, 0, 0, getmaxx(stdscr), c_red, "DEBUG: %s\n  Press spacebar...", buff);
    std::ofstream fout;
    fout.open("debug.log", std::ios_base::app | std::ios_base::out);
    fout << filename << "[" << line << "]: " << buff << "\n";
    fout.close();
    while (getch() != ' ') {
        // wait for spacebar
    }
    werase(stdscr);
}

bool query_yn(const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[1024];
    vsprintf(buff, mes, ap);
    va_end(ap);

    bool force_uc = OPTIONS["FORCE_CAPITAL_YN"];
    std::string query;
    if (force_uc) {
        query = string_format(_("%s (Y/N - Case Sensitive)"), buff);
    } else {
        query = string_format(_("%s (y/n)"), buff);
    }

    int win_width = utf8_width(query.c_str()) + 2;
    win_width = (win_width < FULL_SCREEN_WIDTH - 2 ? win_width : FULL_SCREEN_WIDTH - 2);

    std::vector<std::string> textformatted;
    textformatted = foldstring(query, win_width);
    WINDOW *w = newwin(textformatted.size() + 2, win_width, (TERMY - 3) / 2,
                       (TERMX > win_width) ? (TERMX - win_width) / 2 : 0);

    fold_and_print(w, 1, 1, win_width, c_ltred, query.c_str());

    draw_border(w);

    wrefresh(w);
    char ch;
    do {
        ch = getch();
    } while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE && ch != 'Y'
             && ch != 'N' && (force_uc || (ch != 'y' && ch != 'n')));
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
    if (ch == 'Y' || ch == 'y') {
        return true;
    }
    return false;
}

int query_int(const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[1024];
    vsprintf(buff, mes, ap);
    va_end(ap);

    std::string raw_input = string_input_popup(std::string(buff));

    //Note that atoi returns 0 for anything it doesn't like.
    return atoi(raw_input.c_str());
}

std::string string_input_popup(std::string title, int width, std::string input, std::string desc,
                               std::string identifier, int max_length, bool only_digits )
{
    nc_color title_color = c_ltred;
    nc_color desc_color = c_green;

    std::vector<std::string> descformatted;

    int titlesize = utf8_width(title.c_str());
    int startx = titlesize + 2;
    if ( max_length == 0 ) {
        max_length = width;
    }
    int w_height = 3;
    int iPopupWidth = (width == 0) ? FULL_SCREEN_WIDTH : width + titlesize + 4;
    if (iPopupWidth > FULL_SCREEN_WIDTH) {
        iPopupWidth = FULL_SCREEN_WIDTH;
    }
    if ( desc.size() > 0 ) {
        int twidth = utf8_width(desc.c_str());
        if ( twidth > iPopupWidth - 4 ) {
            twidth = iPopupWidth - 4;
        }
        descformatted = foldstring(desc, twidth);
        w_height += descformatted.size();
    }
    int starty = 1 + descformatted.size();

    if ( max_length == 0 ) {
        max_length = 1024;
    }
    int w_y = (TERMY - w_height) / 2;
    int w_x = ((TERMX > iPopupWidth) ? (TERMX - iPopupWidth) / 2 : 0);
    WINDOW *w = newwin(w_height, iPopupWidth, w_y,
                       ((TERMX > iPopupWidth) ? (TERMX - iPopupWidth) / 2 : 0));

    draw_border(w);

    int endx = iPopupWidth - 3;

    for(int i = 0; i < descformatted.size(); i++ ) {
        mvwprintz(w, 1 + i, 1, desc_color, "%s", descformatted[i].c_str() );
    }
    mvwprintz(w, starty, 1, title_color, "%s", title.c_str() );
    long key = 0;
    int pos = -1;
    std::string ret = string_input_win(w, input, max_length, startx, starty, endx, true, key, pos,
                                       identifier, w_x, w_y, true, only_digits);
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
    return ret;
}

std::string string_input_win(WINDOW *w, std::string input, int max_length, int startx, int starty,
                             int endx, bool loop, long &ch, int &pos, std::string identifier, int w_x, int w_y, bool dorefresh, bool only_digits )
{
    std::string ret = input;
    nc_color string_color = c_magenta;
    nc_color cursor_color = h_ltgray;
    nc_color underscore_color = c_ltgray;
    if ( pos == -1 ) {
        pos = utf8_width(input.c_str());
    }
    int lastpos = pos;
    int scrmax = endx - startx;
    int shift = 0;
    int lastshift = shift;
    bool redraw = true;

    do {

        if ( pos < 0 ) {
            pos = 0;
        }

        if ( pos < shift ) {
            shift = pos;
        } else if ( pos > shift + scrmax ) {
            shift = pos - scrmax;
        }

        if (shift < 0 ) {
            shift = 0;
        }

        if( redraw || lastshift != shift ) {
            redraw = false;
            for ( int reti = shift, scri = 0; scri <= scrmax; reti++, scri++ ) {
                if( reti < ret.size() ) {
                    mvwputch(w, starty, startx + scri, (reti == pos ? cursor_color : string_color ), ret[reti] );
                } else {
                    mvwputch(w, starty, startx + scri, (reti == pos ? cursor_color : underscore_color ), '_');
                }
            }
        } else if ( lastpos != pos ) {
            if ( lastpos >= shift && lastpos <= shift + scrmax ) {
                if ( lastpos - shift >= 0 && lastpos - shift < ret.size() ) {
                    mvwputch(w, starty, startx + lastpos, string_color, ret[lastpos - shift]);
                } else {
                    mvwputch(w, starty, startx + lastpos, underscore_color, '_' );
                }
            }
            if (pos < ret.size() ) {
                mvwputch(w, starty, startx + pos, cursor_color, ret[pos - shift]);
            } else {
                mvwputch(w, starty, startx + pos, cursor_color, '_' );
            }
        }

        lastpos = pos;
        lastshift = shift;
        if (dorefresh) {
            wrefresh(w);
        }
        ch = getch();
        bool return_key = false;
        if (ch == 27) { // Escape
            return "";
        } else if (ch == '\n') {
            return_key = true;
        } else if (ch == KEY_UP ) {
            if(identifier.size() > 0) {
                std::vector<std::string> *hist = uistate.gethistory(identifier);
                if(hist != NULL) {
                    uimenu hmenu;
                    hmenu.w_height = 3 + hist->size();
                    if (w_y - hmenu.w_height < 0 ) {
                        hmenu.w_y = 0;
                        hmenu.w_height = ( w_y - hmenu.w_y < 4 ? 4 : w_y - hmenu.w_y );
                    } else {
                        hmenu.w_y = w_y - hmenu.w_height;
                    }
                    hmenu.w_x = w_x;
                    hmenu.title = _("d: delete history");
                    hmenu.return_invalid = true;
                    for(int h = 0; h < hist->size(); h++) {
                        hmenu.addentry(h, true, -2, (*hist)[h].c_str());
                    }
                    if ( ret.size() > 0 && ( hmenu.entries.size() == 0 ||
                                             hmenu.entries[hist->size() - 1].txt != ret ) ) {
                        hmenu.addentry(hist->size(), true, -2, ret);
                        hmenu.selected = hist->size();
                    } else {
                        hmenu.selected = hist->size() - 1;
                    }

                    hmenu.query();
                    if ( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != ret ) {
                        ret = hmenu.entries[hmenu.ret].txt;
                        if( hmenu.ret < hist->size() ) {
                            hist->erase(hist->begin() + hmenu.ret);
                            hist->push_back(ret);
                        }
                        pos = ret.size();
                        redraw = true;
                    } else if ( hmenu.keypress == 'd' ) {
                        hist->clear();
                    }
                }
            }
        } else if (ch == KEY_DOWN || ch == KEY_NPAGE || ch == KEY_PPAGE ) {
            /* absolutely nothing */
        } else if (ch == KEY_RIGHT ) {
            if( pos + 1 <= ret.size() ) {
                pos++;
            }
            redraw = true;
        } else if (ch == KEY_LEFT ) {
            if ( pos > 0 ) {
                pos--;
            }
            redraw = true;
        } else if (ch == 0x15 ) {                      // ctrl-u: delete all the things
            pos = 0;
            ret.erase(0);
            redraw = true;
        } else if (ch == KEY_BACKSPACE || ch == 127) { // Move the cursor back and re-draw it
            if( pos > 0 &&
                pos <= ret.size() ) {         // but silently drop input if we're at 0, instead of adding '^'
                pos--;                                     //TODO: it is safe now since you only input ascii chars
                ret.erase(pos, 1);
                redraw = true;
            }
        } else if(ch == KEY_F(2)) {
            std::string tmp = get_input_string_from_file();
            int tmplen = utf8_width(tmp.c_str());
            if(tmplen > 0 && (tmplen + utf8_width(ret.c_str()) <= max_length || max_length == 0)) {
                ret.append(tmp);
            }
        } else if( ch != 0 && ch != ERR && (ret.size() < max_length || max_length == 0) ) {
            if ( only_digits && !isdigit(ch) ) {
                return_key = true;
            } else {
                if ( pos == ret.size() ) {
                    ret += ch;
                } else {
                    ret.insert(pos, 1, ch);
                }
                redraw = true;
                pos++;
            }
        }
        if (return_key) {//"/n" return code
            {
                if(identifier.size() > 0 && ret.size() > 0 ) {
                    std::vector<std::string> *hist = uistate.gethistory(identifier);
                    if( hist != NULL ) {
                        if ( hist->size() == 0 || (*hist)[hist->size() - 1] != ret ) {
                            hist->push_back(ret);
                        }
                    }
                }
                return ret;
            }
        }
    } while ( loop == true );
    return ret;
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
    std::vector<std::string> folded = foldstring(tmp, FULL_SCREEN_WIDTH - 2);
    height += folded.size();
    for(int i = 0; i < folded.size(); i++) {
        int cw = utf8_width(folded[i].c_str());
        if(cw > width) {
            width = cw;
        }
    }
    width += 2;
    if (height > FULL_SCREEN_HEIGHT) {
        height = FULL_SCREEN_HEIGHT;
    }
    WINDOW *w = newwin(height + 1, width, (TERMY - (height + 1)) / 2,
                       (TERMX > width) ? (TERMX - width) / 2 : 0);
    draw_border(w);

    for(int i = 0; i < folded.size(); i++) {
        mvwprintz(w, i + 1, 1, c_white, folded[i].c_str());
    }

    wrefresh(w);
    char ch = getch();;
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
    return ch;
}

int menu_vec(bool cancelable, const char *mes,
             std::vector<std::string> options)   // compatibility stub for uimenu(cancelable, mes, options)
{
    return (int)uimenu(cancelable, mes, options);
}

int menu(bool cancelable, const char *mes,
         ...)   // compatibility stub for uimenu(cancelable, mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char *tmp;
    std::vector<std::string> options;
    bool done = false;
    while (!done) {
        tmp = va_arg(ap, char *);
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
    char buff[4096];
    vsprintf(buff, mes, ap);
    va_end(ap);
    std::string tmp = buff;
    int width = 0;
    int height = 2;
    std::vector<std::string> folded = foldstring(tmp, FULL_SCREEN_WIDTH - 2);
    height += folded.size();
    for(int i = 0; i < folded.size(); i++) {
        int cw = utf8_width(folded[i].c_str());
        if(cw > width) {
            width = cw;
        }
    }
    width += 2;
    WINDOW *w = newwin(height, width, 0, (TERMX > width) ? (TERMX - width) / 2 : 0);
    draw_border(w);

    for(int i = 0; i < folded.size(); i++) {
        mvwprintz(w, i + 1, 1, c_white, folded[i].c_str());
    }

    wrefresh(w);
    char ch;
    do {
        ch = getch();
    } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
}

void popup(const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[4096];
    vsprintf(buff, mes, ap);
    va_end(ap);
    std::string tmp = buff;
    int width = 0;
    int height = 2;
    std::vector<std::string> folded = foldstring(tmp, FULL_SCREEN_WIDTH - 2);
    height += folded.size();
    for(int i = 0; i < folded.size(); i++) {
        int cw = utf8_width(folded[i].c_str());
        if(cw > width) {
            width = cw;
        }
    }
    width += 2;
    if (height > FULL_SCREEN_HEIGHT) {
        height = FULL_SCREEN_HEIGHT;
    }
    WINDOW *w = newwin(height, width, (TERMY - (height + 1)) / 2,
                       (TERMX > width) ? (TERMX - width) / 2 : 0);
    draw_border(w);

    for(int i = 0; i < folded.size(); i++) {
        mvwprintz(w, i + 1, 1, c_white, folded[i].c_str());
    }

    wrefresh(w);
    char ch;
    do {
        ch = getch();
    } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
}

void popup_nowait(const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[4096];
    vsprintf(buff, mes, ap);
    va_end(ap);
    std::string tmp = buff;
    int width = 0;
    int height = 2;
    std::vector<std::string> folded = foldstring(tmp, FULL_SCREEN_WIDTH - 2);
    height += folded.size();
    for(int i = 0; i < folded.size(); i++) {
        int cw = utf8_width(folded[i].c_str());
        if(cw > width) {
            width = cw;
        }
    }
    width += 2;
    if (height > FULL_SCREEN_HEIGHT) {
        height = FULL_SCREEN_HEIGHT;
    }
    WINDOW *w = newwin(height, width, (TERMY - (height + 1)) / 2,
                       (TERMX > width) ? (TERMX - width) / 2 : 0);
    draw_border(w);

    for(int i = 0; i < folded.size(); i++) {
        mvwprintz(w, i + 1, 1, c_white, folded[i].c_str());
    }
    wrefresh(w);
    delwin(w);
    refresh();
}

void full_screen_popup(const char *mes, ...)
{
    va_list ap;
    va_start(ap, mes);
    char buff[8192];
    vsprintf(buff, mes, ap);
    va_end(ap);
    std::string tmp = buff;
    int width = 0;
    int height = 2;
    std::vector<std::string> folded = foldstring(tmp, FULL_SCREEN_WIDTH - 3);
    height += folded.size();
    for(int i = 0; i < folded.size(); i++) {
        int cw = utf8_width(folded[i].c_str());
        if(cw > width) {
            width = cw;
        }
    }
    width += 2;

    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    draw_border(w);

    for(int i = 0; i < folded.size(); i++) {
        mvwprintz(w, i + 1, 2, c_white, folded[i].c_str());
    }

    wrefresh(w);
    char ch;
    do {
        ch = getch();
    } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
}

//note that passing in iteminfo instances with sType == "MENU" or "DESCRIPTION" does special things
//if sType == "MENU", sFmt == "iOffsetY" or "iOffsetX" also do special things
//otherwise if sType == "MENU", dValue can be used to control color
//all this should probably be cleaned up at some point, rather than using a function for things it wasn't meant for
// well frack, half the game uses it so: optional (int)selected argument causes entry highlight, and enter to return entry's key. Also it now returns int
//@param without_getch don't wait getch, return = (int)' ';
int compare_split_screen_popup(int iLeft, int iWidth, int iHeight, std::string sItemName,
                               std::vector<iteminfo> vItemDisplay, std::vector<iteminfo> vItemCompare, int selected,
                               bool without_getch)
{
    WINDOW *w = newwin(iHeight, iWidth, VIEW_OFFSET_Y, iLeft + VIEW_OFFSET_X);

    mvwprintz(w, 1, 2, c_white, sItemName.c_str());
    int line_num = 3;
    int iStartX = 0;
    bool bStartNewLine = true;
    int selected_ret = '\n';
    std::string spaces(iWidth - 2, ' ');
    for (int i = 0; i < vItemDisplay.size(); i++) {
        if (vItemDisplay[i].sType == "MENU") {
            if (vItemDisplay[i].sFmt == "iOffsetY") {
                line_num += int(vItemDisplay[i].dValue);
            } else if (vItemDisplay[i].sFmt == "iOffsetX") {
                iStartX = int(vItemDisplay[i].dValue);
            } else {
                nc_color nameColor = c_ltgreen; //pre-existing behavior, so make it the default
                //patched to allow variable "name" coloring, e.g. for item examining
                nc_color bgColor = c_white;     //yes the name makes no sense
                if (vItemDisplay[i].dValue >= 0) {
                    if (vItemDisplay[i].dValue < .1 && vItemDisplay[i].dValue > -.1) {
                        nameColor = c_ltgray;
                    } else {
                        nameColor = c_ltred;
                    }
                }
                if ( i == selected && vItemDisplay[i].sName != "" ) {
                    bgColor = h_white;
                    selected_ret = (int)vItemDisplay[i].sName.c_str()[0]; // fixme: sanity check(?)
                }
                mvwprintz(w, line_num, 1, bgColor, "%s", spaces.c_str() );
                shortcut_print(w, line_num, iStartX, bgColor, nameColor, vItemDisplay[i].sFmt.c_str());
                line_num++;
            }
        } else if (vItemDisplay[i].sType == "DESCRIPTION") {
            line_num += fold_and_print(w, line_num, 2, iWidth - 4, c_white, "%s", vItemDisplay[i].sName.c_str());
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
            if(pos != std::string::npos) {
                wprintz(w, c_white, sFmt.substr(0, pos).c_str());
                sPost = sFmt.substr(pos + 5);
            } else {
                wprintz(w, c_white, sFmt.c_str());
            }

            if (vItemDisplay[i].sValue != "-999") {
                nc_color thisColor = c_white;
                for (int k = 0; k < vItemCompare.size(); k++) {
                    if (vItemCompare[k].sValue != "-999") {
                        if (vItemDisplay[i].sName == vItemCompare[k].sName) {
                            if (vItemDisplay[i].dValue > vItemCompare[k].dValue - .1 &&
                                vItemDisplay[i].dValue < vItemCompare[k].dValue + .1) {
                                thisColor = c_white;
                            } else if (vItemDisplay[i].dValue > vItemCompare[k].dValue) {
                                if (vItemDisplay[i].bLowerIsBetter) {
                                    thisColor = c_ltred;
                                } else {
                                    thisColor = c_ltgreen;
                                }
                            } else if (vItemDisplay[i].dValue < vItemCompare[k].dValue) {
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
                if (vItemDisplay[i].is_int == true) {
                    wprintz(w, thisColor, "%s%.0f", sPlus.c_str(), vItemDisplay[i].dValue);
                } else {
                    wprintz(w, thisColor, "%s%.1f", sPlus.c_str(), vItemDisplay[i].dValue);
                }
            }
            wprintz(w, c_white, sPost.c_str());

            if (vItemDisplay[i].bNewLine) {
                line_num++;
                bStartNewLine = true;
            }
        }
    }

    draw_border(w);

    int ch = (int)' ';

    wrefresh(w);
    if (!without_getch) {
        ch = (int)getch();
        if ( selected > 0 && ( ch == '\n' || ch == KEY_RIGHT ) && selected_ret != 0 ) {
            ch = selected_ret;
        } else if ( selected == KEY_LEFT ) {
            ch = (int)' ';
        }
        delwin(w);
    }

    return ch;
}

char rand_char()
{
    switch (rng(0, 9)) {
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

// this translates symbol y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
long special_symbol (long sym)
{
    switch (sym) {
        case 'j':
            return LINE_XOXO;
        case 'h':
            return LINE_OXOX;
        case 'c':
            return LINE_XXXX;
        case 'y':
            return LINE_OXXO;
        case 'u':
            return LINE_OOXX;
        case 'n':
            return LINE_XOOX;
        case 'b':
            return LINE_XXOO;
        default:
            return sym;
    }
}

// find the position of each non-printing tag in a string
std::vector<size_t> get_tag_positions(const std::string &s)
{
    std::vector<size_t> ret;
    size_t pos = s.find("<color_", 0, 7);
    while (pos != std::string::npos) {
        ret.push_back(pos);
        pos = s.find("<color_", pos + 1, 7);
    }
    pos = s.find("</color>", 0, 8);
    while (pos != std::string::npos) {
        ret.push_back(pos);
        pos = s.find("</color>", pos + 1, 8);
    }
    std::sort(ret.begin(), ret.end());
    return ret;
}

// utf-8 version
std::string word_rewrap (const std::string &ins, int width)
{
    std::ostringstream o;
    std::string in = ins;
    std::replace(in.begin(), in.end(), '\n', ' ');

    // find non-printing tags
    std::vector<size_t> tag_positions = get_tag_positions(in);

    int lastwb  = 0; //last word break
    int lastout = 0;
    const char *instr = in.c_str();
    bool skipping_tag = false;

    for (int j = 0, x = 0; j < in.size(); ) {
        const char *ins = instr + j;
        int len = ANY_LENGTH;
        unsigned uc = UTF8_getch(&ins, &len);

        if (uc == '<') { // maybe skip non-printing tag
            std::vector<size_t>::iterator it;
            for (it = tag_positions.begin(); it != tag_positions.end(); ++it) {
                if (*it == j) {
                    skipping_tag = true;
                    break;
                }
            }
        }

        j += ANY_LENGTH - len;

        if (skipping_tag) {
            if (uc == '>') {
                skipping_tag = false;
            }
            continue;
        }

        x += mk_wcwidth(uc);

        if (x >= width) {
            if (lastwb == lastout) {
                lastwb = j;
            }
            for(int k = lastout; k < lastwb; k++) {
                o << in[k];
            }
            o << '\n';
            x = 0;
            lastout = j = lastwb;
        }

        if (uc == ' ' || uc >= 0x2E80) {
            lastwb = j;
        }
    }
    for (int k = lastout; k < in.size(); k++) {
        o << in[k];
    }

    return o.str();
}

void draw_tab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected)
{
    int iOffsetXRight = iOffsetX + utf8_width(sText.c_str()) + 1;

    mvwputch(w, 0, iOffsetX,      c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 0, iOffsetXRight, c_ltgray, LINE_OOXX); // ^|
    mvwputch(w, 1, iOffsetX,      c_ltgray, LINE_XOXO); // |
    mvwputch(w, 1, iOffsetXRight, c_ltgray, LINE_XOXO); // |

    mvwprintz(w, 1, iOffsetX + 1, (bSelected) ? h_ltgray : c_ltgray, sText.c_str());

    for (int i = iOffsetX + 1; i < iOffsetXRight; i++) {
        mvwputch(w, 0, i, c_ltgray, LINE_OXOX);    // -
    }

    if (bSelected) {
        mvwputch(w, 1, iOffsetX - 1,      h_ltgray, '<');
        mvwputch(w, 1, iOffsetXRight + 1, h_ltgray, '>');

        for (int i = iOffsetX + 1; i < iOffsetXRight; i++) {
            mvwputch(w, 2, i, c_black, ' ');
        }

        mvwputch(w, 2, iOffsetX,      c_ltgray, LINE_XOOX); // _|
        mvwputch(w, 2, iOffsetXRight, c_ltgray, LINE_XXOO); // |_

    } else {
        mvwputch(w, 2, iOffsetX,      c_ltgray, LINE_XXOX); // _|_
        mvwputch(w, 2, iOffsetXRight, c_ltgray, LINE_XXOX); // _|_
    }
}

void draw_subtab(WINDOW *w, int iOffsetX, std::string sText, bool bSelected)
{
    int iOffsetXRight = iOffsetX + utf8_width(sText.c_str()) + 1;

    mvwprintz(w, 0, iOffsetX + 1, (bSelected) ? h_ltgray : c_ltgray, sText.c_str());

    if (bSelected) {
        mvwputch(w, 0, iOffsetX - 1,      h_ltgray, '<');
        mvwputch(w, 0, iOffsetXRight + 1, h_ltgray, '>');

        for (int i = iOffsetX + 1; i < iOffsetXRight; i++) {
            mvwputch(w, 1, i, c_black, ' ');
        }
    }
}

void draw_scrollbar(WINDOW *window, const int iCurrentLine, const int iContentHeight,
                    const int iNumEntries, const int iOffsetY, const int iOffsetX,
                    nc_color bar_color)
{
    //Clear previous scrollbar
    for(int i = iOffsetY; i < iOffsetY + iContentHeight; i++) {
        mvwputch(window, i, iOffsetX, bar_color, LINE_XOXO);
    }

    if (iContentHeight >= iNumEntries) {
        wrefresh(window);
        return;
    }

    if (iNumEntries > 0) {
        mvwputch(window, iOffsetY, iOffsetX, c_ltgreen, '^');
        mvwputch(window, iOffsetY + iContentHeight - 1, iOffsetX, c_ltgreen, 'v');

        int iSBHeight = ((iContentHeight - 2) * (iContentHeight - 2)) / iNumEntries;

        if (iSBHeight < 2) {
            iSBHeight = 2;
        }

        int iStartY = (iCurrentLine * (iContentHeight - 3 - iSBHeight)) / iNumEntries;
        if (iCurrentLine == 0) {
            iStartY = -1;
        } else if (iCurrentLine == iNumEntries - 1) {
            iStartY = iContentHeight - 3 - iSBHeight;
        }

        for (int i = 0; i < iSBHeight; i++) {
            mvwputch(window, i + iOffsetY + 2 + iStartY, iOffsetX, c_cyan_cyan, LINE_XOXO);
        }
    }

    wrefresh(window);
}

void calcStartPos(int &iStartPos, const int iCurrentLine, const int iContentHeight,
                  const int iNumEntries)
{
    if( OPTIONS["MENU_SCROLL"] ) {
        if (iNumEntries > iContentHeight) {
            iStartPos = iCurrentLine - (iContentHeight - 1) / 2;

            if (iStartPos < 0) {
                iStartPos = 0;
            } else if (iStartPos + iContentHeight > iNumEntries) {
                iStartPos = iNumEntries - iContentHeight;
            }
        }
    } else {
        if( iCurrentLine < iStartPos ) {
            iStartPos = iCurrentLine;
        } else if( iCurrentLine >= iStartPos + iContentHeight ) {
            iStartPos = 1 + iCurrentLine - iContentHeight;
        }
    }
}


void hit_animation(int iX, int iY, nc_color cColor, char cTile, int iTimeout)
{
    WINDOW *w_hit = newwin(1, 1, iY + VIEW_OFFSET_Y, iX + VIEW_OFFSET_X);
    if (w_hit == NULL) {
        return; //we passed in negative values (semi-expected), so let's not segfault
    }

    mvwputch(w_hit, 0, 0, cColor, cTile);
    wrefresh(w_hit);

    if (iTimeout <= 0 || iTimeout > 999) {
        iTimeout = 70;
    }

    timeout(iTimeout);
    getch(); //using this, because holding down a key with nanosleep can get yourself killed
    timeout(-1);
}

std::string from_sentence_case (const std::string &kingston)
{
    if (kingston.size() > 0) {
        std::string montreal = kingston;
        if(montreal.empty()) {
            return "";
        } else {
            montreal.replace(0, 1, 1, tolower(kingston.at(0)));
            return montreal;
        }
    }
    return "";
}

std::string string_format(std::string pattern, ...)
{
    va_list ap;
    va_start(ap, pattern);
    char buff[3000];    //TODO replace Magic Number
    vsprintf(buff, pattern.c_str(), ap);
    va_end(ap);

    //drop contents behind $, this trick is there to skip certain arguments
    char *break_pos = strchr(buff, '\003');
    if(break_pos) {
        break_pos[0] = '\0';
    }

    return buff;
}

//wrap if for i18n
std::string &capitalize_letter(std::string &str, size_t n)
{
    char c = str[n];
    if(str.length() > 0 && c >= 'a' && c <= 'z') {
        c += 'A' - 'a';
        str[n] = c;
    }

    return str;
}

//remove prefix of a strng, between c1 and c2, ie, "<prefix>remove it"
std::string rm_prefix(std::string str, char c1, char c2)
{
    if(str.size() > 0 && str[0] == c1) {
        size_t pos = str.find_first_of(c2);
        if(pos != std::string::npos) {
            str = str.substr(pos + 1);
        }
    }
    return str;
}

// draw a menu item like strign with highlighted shortcut character
// Example: <w>ield, m<o>ve
// returns: output length (in console cells)
size_t shortcut_print(WINDOW *w, int y, int x, nc_color color, nc_color colork, const char *fmt,
                      ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buff[3000];    //TODO replace Magic Number
    vsprintf(buff, fmt, ap);
    va_end(ap);

    std::string tmp = buff;
    size_t pos = tmp.find_first_of('<');
    size_t pos2 = tmp.find_first_of('>');
    size_t len = 0;
    if(pos2 != std::string::npos && pos < pos2) {
        tmp.erase(pos, 1);
        tmp.erase(pos2 - 1, 1);
        mvwprintz(w, y, x, color, tmp.c_str());
        mvwprintz(w, y, x + pos, colork, "%s", tmp.substr(pos, pos2 - pos - 1).c_str());
        len = utf8_width(tmp.c_str());
    } else {
        // no shutcut?
        mvwprintz(w, y, x, color, buff);
        len = utf8_width(buff);
    }
    return len;
}

//same as above, from current position
size_t shortcut_print(WINDOW *w, nc_color color, nc_color colork, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buff[3000];    //TODO replace Magic Number
    vsprintf(buff, fmt, ap);
    va_end(ap);

    std::string tmp = buff;
    size_t pos = tmp.find_first_of('<');
    size_t pos2 = tmp.find_first_of('>');
    size_t len = 0;
    if(pos2 != std::string::npos && pos < pos2) {
        tmp.erase(pos, 1);
        tmp.erase(pos2 - 1, 1);
        wprintz(w, color, tmp.substr(0, pos).c_str());
        wprintz(w, colork, "%s", tmp.substr(pos, pos2 - pos - 1).c_str());
        wprintz(w, color, tmp.substr(pos2 - 1).c_str());
        len = utf8_width(tmp.c_str());
    } else {
        // no shutcut?
        wprintz(w, color, buff);
        len = utf8_width(buff);
    }
    return len;
}

void get_HP_Bar(const int current_hp, const int max_hp, nc_color &color, std::string &text, const bool bMonster)
{
    if (current_hp == max_hp){
      color = c_green;
      text = "|||||";
    } else if (current_hp > max_hp * .9 && !bMonster) {
      color = c_green;
      text = "||||\\";
    } else if (current_hp > max_hp * .8) {
      color = c_ltgreen;
      text = "||||";
    } else if (current_hp > max_hp * .7 && !bMonster) {
      color = c_ltgreen;
      text = "|||\\";
    } else if (current_hp > max_hp * .6) {
      color = c_yellow;
      text = "|||";
    } else if (current_hp > max_hp * .5 && !bMonster) {
      color = c_yellow;
      text = "||\\";
    } else if (current_hp > max_hp * .4 && !bMonster) {
      color = c_ltred;
      text = "||";
    } else if (current_hp > max_hp * .3) {
      color = c_ltred;
      text = "|\\";
    } else if (current_hp > max_hp * .2 && !bMonster) {
      color = c_red;
      text = "|";
    } else if (current_hp > max_hp * .1) {
      color = c_red;
      text = "\\";
    } else if (current_hp > 0) {
      color = c_red;
      text = ":";
    } else {
      color = c_ltgray;
      text = "-----";
    }
}

