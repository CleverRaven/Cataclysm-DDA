#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"

#include <cstring> // strlen

//***********************************
//Globals                           *
//***********************************

WINDOW *mainwin;
WINDOW *stdscr;
pairs *colorpairs;   //storage for pair'ed colored, should be dynamic, meh
int echoOn;     //1 = getnstr shows input, 0 = doesn't show. needed for echo()-ncurses compatibility.

//***********************************
//Pseudo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *initscr(void)
{
    stdscr = curses_init();
    return stdscr;
}

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
    if (begin_y < 0 || begin_x < 0) {
        return NULL; //it's the caller's problem now (since they have logging functions declared)
    }

    // default values
    if (ncols == 0) {
        ncols = TERMX - begin_x;
    }
    if (nlines == 0) {
        nlines = TERMY - begin_y;
    }

    int i, j;
    WINDOW *newwindow = new WINDOW;
    newwindow->x = begin_x;
    newwindow->y = begin_y;
    newwindow->width = ncols;
    newwindow->height = nlines;
    newwindow->inuse = true;
    newwindow->draw = false;
    newwindow->BG = 0;
    newwindow->FG = 8;
    newwindow->cursorx = 0;
    newwindow->cursory = 0;
    newwindow->line = new curseline[nlines];

    for (j = 0; j < nlines; j++) {
        newwindow->line[j].chars = new char[ncols * 4];
        newwindow->line[j].FG = new char[ncols];
        newwindow->line[j].BG = new char[ncols];
        newwindow->line[j].touched = true; //Touch them all !?
        newwindow->line[j].width_in_bytes = ncols;
        for (i = 0; i < ncols; i++) {
            newwindow->line[j].chars[i] = 0;
            newwindow->line[j].chars[i + 1] = 0;
            newwindow->line[j].chars[i + 2] = 0;
            newwindow->line[j].chars[i + 3] = 0;
            newwindow->line[j].FG[i] = 0;
            newwindow->line[j].BG[i] = 0;
        }
    }
    return newwindow;
}

//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
    if (win == NULL) {
        return 1;
    }

    int j;
    win->inuse = false;
    win->draw = false;
    for (j = 0; j < win->height; j++) {
        delete[] win->line[j].chars;
        delete[] win->line[j].FG;
        delete[] win->line[j].BG;
    }
    delete[] win->line;
    delete win;
    win = NULL;
    return 1;
}

inline int newline(WINDOW *win)
{
    if (win->cursory < win->height - 1) {
        win->cursory++;
        win->cursorx = 0;
        return 1;
    }
    return 0;
}

inline void addedchar(WINDOW *win)
{
    win->cursorx++;
    win->line[win->cursory].touched = true;
    if (win->cursorx > win->width) {
        newline(win);
    }
}


//Borders the window with fancy lines!
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
            chtype bl, chtype br)
{
    /*
    ncurses does not do this, and this prevents: wattron(win, c_customBordercolor); wborder(win, ...); wattroff(win, c_customBorderColor);
        wattron(win, c_white);
    */
    int i, j;
    int oldx = win->cursorx; //methods below move the cursor, save the value!
    int oldy = win->cursory; //methods below move the cursor, save the value!

    if (ls) {
        for (j = 1; j < win->height - 1; j++) {
            mvwaddch(win, j, 0, ls);
        }
    } else {
        for (j = 1; j < win->height - 1; j++) {
            mvwaddch(win, j, 0, LINE_XOXO);
        }
    }

    if (rs) {
        for (j = 1; j < win->height - 1; j++) {
            mvwaddch(win, j, win->width - 1, rs);
        }
    } else {
        for (j = 1; j < win->height - 1; j++) {
            mvwaddch(win, j, win->width - 1, LINE_XOXO);
        }
    }

    if (ts) {
        for (i = 1; i < win->width - 1; i++) {
            mvwaddch(win, 0, i, ts);
        }
    } else {
        for (i = 1; i < win->width - 1; i++) {
            mvwaddch(win, 0, i, LINE_OXOX);
        }
    }

    if (bs) {
        for (i = 1; i < win->width - 1; i++) {
            mvwaddch(win, win->height - 1, i, bs);
        }
    } else {
        for (i = 1; i < win->width - 1; i++) {
            mvwaddch(win, win->height - 1, i, LINE_OXOX);
        }
    }

    if (tl) {
        mvwaddch(win, 0, 0, tl);
    } else {
        mvwaddch(win, 0, 0, LINE_OXXO);
    }

    if (tr) {
        mvwaddch(win, 0, win->width - 1, tr);
    } else {
        mvwaddch(win, 0, win->width - 1, LINE_OOXX);
    }

    if (bl) {
        mvwaddch(win, win->height - 1, 0, bl);
    } else {
        mvwaddch(win, win->height - 1, 0, LINE_XXOO);
    }

    if (br) {
        mvwaddch(win, win->height - 1, win->width - 1, br);
    } else {
        mvwaddch(win, win->height - 1, win->width - 1, LINE_XOOX);
    }

    //methods above move the cursor, put it back
    wmove(win, oldy, oldx);
    wattroff(win, c_white);
    return 1;
}

int hline(chtype ch, int n)
{
    return whline(mainwin, ch, n);
}

int vline(chtype ch, int n)
{
    return wvline(mainwin, ch, n);
}

int whline(WINDOW *win, chtype ch, int n)
{
    return mvwvline(mainwin, win->cursory, win->cursorx, ch, n);
}

int wvline(WINDOW *win, chtype ch, int n)
{
    return mvwvline(mainwin, win->cursory, win->cursorx, ch, n);
}

int mvhline(int y, int x, chtype ch, int n)
{
    return mvwhline(mainwin, y, x, ch, n);
}

int mvvline(int y, int x, chtype ch, int n)
{
    return mvwvline(mainwin, y, x, ch, n);
}

int mvwhline(WINDOW *win, int y, int x, chtype ch, int n)
{
    if (ch) {
        for (int i = 0; i < n; i++) {
            mvwaddch(win, y, x + i, ch);
        }
    } else {
        for (int i = 0; i < n; i++) {
            mvwaddch(win, y, x + i, LINE_OXOX);
        }
    }

    return 0;
}

int mvwvline(WINDOW *win, int y, int x, chtype ch, int n)
{
    if (ch) {
        for (int j = 0; j < n; j++) {
            mvwaddch(win, y + j, x, ch);
        }
    } else {
        for (int j = 0; j < n; j++) {
            mvwaddch(win, y + j, x, LINE_XOXO);
        }
    }

    return 0;
}

//Refreshes a window, causing it to redraw on top.
int wrefresh(WINDOW *win)
{
    if (win->draw) {
        curses_drawwindow(win);
    }
    return 1;
}

//Refreshes the main window, causing it to redraw on top.
int refresh(void)
{
    return wrefresh(mainwin);
}

int getch(void)
{
    return wgetch(mainwin);
}

int wgetch(WINDOW *win)
{
    return curses_getch(win);
}

int mvgetch(int y, int x)
{
    move(y, x);
    return getch();
}

int mvwgetch(WINDOW *win, int y, int x)
{
    move(y, x);
    return wgetch(win);
}

int getnstr(char *str, int size)
{
    int startX = mainwin->cursorx;
    int count = 0;
    char input;
    while(true) {
        input = getch();
        // Carriage return, Line feed and End of File terminate the input.
        if( input == '\r' || input == '\n' || input == '\x04' ) {
            str[count] = '\x00';
            return count;
        } else if( input == 127 ) { // Backspace, remapped from \x8 in ProcessMessages()
            if( count == 0 ) {
                continue;
            }
            str[count] = '\x00';
            if(echoOn == 1) {
                mvaddch(mainwin->cursory, startX + count, ' ');
            }
            --count;
            if(echoOn == 1) {
                move(mainwin->cursory, startX + count);
            }
        } else {
            if( count >= size - 1 ) { // Still need space for trailing 0x00
                continue;
            }
            str[count] = input;
            ++count;
            if(echoOn == 1) {
                move(mainwin->cursory, startX + count);
                mvaddch(mainwin->cursory, startX + count, input);
            }
        }
    }
    return count;
}

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
    int size = strlen(fmt);
    if(size > 0) {
        int j, i, p = win->cursorx;
        int x = (win->line[win->cursory].width_in_bytes == win->width) ? win->cursorx : cursorx_to_position(
                    win->line[win->cursory].chars, win->cursorx, &p);

        if(p != x) { //so we start inside a wide character, erase it for good
            const char *ts = win->line[win->cursory].chars + p;
            int len = ANY_LENGTH;
            unsigned tc = UTF8_getch(&ts, &len);
            int tw = mk_wcwidth(tc);
            erease_utf8_by_cw(win->line[win->cursory].chars + p, tw, tw, win->width * 4 - p - 1);
            x = p + tw - 1;
        }
        for (j = 0; j < size; j++) {
            if (!(fmt[j] == 10)) { //check that this isnt a newline char
                const char *utf8str = fmt + j;
                int len = ANY_LENGTH;
                unsigned ch = UTF8_getch(&utf8str, &len);
                int cw = mk_wcwidth(ch);
                len = ANY_LENGTH - len;
                if(cw < 1) {
                    cw = 1;
                }
                if(len < 1) {
                    len = 1;
                }
                if (win->cursorx + cw <= win->width && win->cursory <= win->height - 1) {
                    win->line[win->cursory].width_in_bytes += erease_utf8_by_cw(win->line[win->cursory].chars + x, cw,
                            len, win->width * 4 - x - 1);
                    for(i = 0; i < len; i++) {
                        win->line[win->cursory].chars[x + i] = fmt[j + i];
                    }
                    for(i = 0; i < cw; i++) {
                        win->line[win->cursory].FG[win->cursorx] = win->FG;
                        win->line[win->cursory].BG[win->cursorx] = win->BG;
                        addedchar(win);
                    }
                    win->line[win->cursory].touched = true;
                    j += len - 1;
                    x += len;
                } else if (win->cursory <= win->height - 1) {
                    // don't write outside the window, but don't abort if there are still lines to write.
                } else {
                    return 0; //if we try and write anything outside the window, abort completely
                }
            } else {// if the character is a newline, make sure to move down a line
                if (newline(win) == 0) {
                    return 0;
                }
                x = 0;
            }
        }
    }
    win->draw = true;
    return 1;
}

//Prints a formatted string to a window at the current cursor, base function
int wprintw(WINDOW *win, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(win, printbuf);
}

//Prints a formatted string to a window, moves the cursor
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (wmove(win, y, x) == 0) {
        return 0;
    }
    return printstring(win, printbuf);
}

//Prints a formatted string to the main window, moves the cursor
int mvprintw(int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (move(y, x) == 0) {
        return 0;
    }
    return printstring(mainwin, printbuf);
}

//Prints a formatted string to the main window at the current cursor
int printw(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2078];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(mainwin, printbuf);
}

//erases a window of all text and attributes
int werase(WINDOW *win)
{
    int j, i;
    for (j = 0; j < win->height; j++) {
        for (i = 0; i < win->width; i++)   {
            win->line[j].chars[i] = 0;
            win->line[j].chars[i + 1] = 0;
            win->line[j].chars[i + 2] = 0;
            win->line[j].chars[i + 3] = 0;
            win->line[j].FG[i] = 0;
            win->line[j].BG[i] = 0;
        }
        win->line[j].touched = true;
        win->line[j].width_in_bytes = win->width;
    }
    win->draw = true;
    wmove(win, 0, 0);
    //    wrefresh(win);
    return 1;
}

//erases the main window of all text and attributes
int erase(void)
{
    return werase(mainwin);
}

//pairs up a foreground and background color and puts it into the array of pairs
int init_pair(short pair, short f, short b)
{
    colorpairs[pair].FG = f;
    colorpairs[pair].BG = b;
    return 1;
}

//moves the cursor in a window
int wmove(WINDOW *win, int y, int x)
{
    if (x >= win->width) {
        return 0;   //FIXES MAP CRASH -> >= vs > only
    }
    if (y >= win->height) {
        return 0;   // > crashes?
    }
    if (y < 0) {
        return 0;
    }
    if (x < 0) {
        return 0;
    }
    win->cursorx = x;
    win->cursory = y;
    return 1;
}

//Clears the main window     I'm not sure if its suppose to do this?
int clear(void)
{
    return wclear(mainwin);
}

//Ends the terminal, destroy everything
int endwin(void)
{
    return curses_destroy();
}

//adds a character to the window
int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
    if (wmove(win, y, x) == 0) {
        return 0;
    }
    return waddch(win, ch);
}

//clears a window
int wclear(WINDOW *win)
{
    werase(win);
    clearok(win);
    return 1;
}

int clearok(WINDOW *win)
{
    for (int i = 0; i < win->y; i++) {
        win->line[i].touched = true;
    }
    return 1;
}

//gets the max x of a window (the width)
int getmaxx(WINDOW *win)
{
    return win->width;
}

//gets the max y of a window (the height)
int getmaxy(WINDOW *win)
{
    return win->height;
}

//gets the beginning x of a window (the x pos)
int getbegx(WINDOW *win)
{
    return win->x;
}

//gets the beginning y of a window (the y pos)
int getbegy(WINDOW *win)
{
    return win->y;
}

//gets the current cursor x position in a window
int getcurx(WINDOW *win)
{
    return win->cursorx;
}

//gets the current cursor y position in a window
int getcury(WINDOW *win)
{
    return win->cursory;
}

int start_color(void)
{
    return curses_start_color();
}

int keypad(WINDOW *faux, bool bf)
{
    return 1;
}

int cbreak(void)
{
    return 1;
}
int keypad(int faux, bool bf)
{
    return 1;
}
int curs_set(int visibility)
{
    return 1;
}

int mvaddch(int y, int x, const chtype ch)
{
    return mvwaddch(mainwin, y, x, ch);
}

int wattron(WINDOW *win, int attrs)
{
    bool isBold = !!(attrs & A_BOLD);
    bool isBlink = !!(attrs & A_BLINK);
    int pairNumber = (attrs & A_COLOR) >> 17;
    win->FG = colorpairs[pairNumber].FG;
    win->BG = colorpairs[pairNumber].BG;
    if (isBold) {
        win->FG += 8;
    }
    if (isBlink) {
        win->BG += 8;
    }
    return 1;
}
int wattroff(WINDOW *win, int attrs)
{
    win->FG = 8;                                //reset to white
    win->BG = 0;                                //reset to black
    return 1;
}
int attron(int attrs)
{
    return wattron(mainwin, attrs);
}
int attroff(int attrs)
{
    return wattroff(mainwin, attrs);
}
int waddch(WINDOW *win, const chtype ch)
{
    char charcode;
    charcode = ch;

    switch (ch) {       //LINE_NESW  - X for on, O for off
        case LINE_XOXO:   //#define LINE_XOXO 4194424
            charcode = LINE_XOXO_C;
            break;
        case LINE_OXOX:   //#define LINE_OXOX 4194417
            charcode = LINE_OXOX_C;
            break;
        case LINE_XXOO:   //#define LINE_XXOO 4194413
            charcode = LINE_XXOO_C;
            break;
        case LINE_OXXO:   //#define LINE_OXXO 4194412
            charcode = LINE_OXXO_C;
            break;
        case LINE_OOXX:   //#define LINE_OOXX 4194411
            charcode = LINE_OOXX_C;
            break;
        case LINE_XOOX:   //#define LINE_XOOX 4194410
            charcode = LINE_XOOX_C;
            break;
        case LINE_XXOX:   //#define LINE_XXOX 4194422
            charcode = LINE_XXOX_C;
            break;
        case LINE_XXXO:   //#define LINE_XXXO 4194420
            charcode = LINE_XXXO_C;
            break;
        case LINE_XOXX:   //#define LINE_XOXX 4194421
            charcode = LINE_XOXX_C;
            break;
        case LINE_OXXX:   //#define LINE_OXXX 4194423
            charcode = LINE_OXXX_C;
            break;
        case LINE_XXXX:   //#define LINE_XXXX 4194414
            charcode = LINE_XXXX_C;
            break;
        default:
            charcode = (char)ch;
            break;
    }


    int cury = win->cursory;
    int p = win->cursorx;
    int curx = (win->line[cury].width_in_bytes == win->width) ? win->cursorx : cursorx_to_position(
                   win->line[cury].chars, win->cursorx, &p);

    if(curx != p) {
        const char *ts = win->line[cury].chars + p;
        int len = ANY_LENGTH;
        unsigned tc = UTF8_getch(&ts, &len);
        int tw = mk_wcwidth(tc);
        win->line[cury].width_in_bytes += erease_utf8_by_cw(win->line[cury].chars + p, tw, tw,
                                          win->width * 4 - p - 1);
        curx = p + tw - 1;
    } else if(win->line[cury].width_in_bytes != win->width) {
        win->line[cury].width_in_bytes += erease_utf8_by_cw(win->line[cury].chars + p, 1, 1,
                                          win->width * 4 - p - 1);
    }

    win->line[cury].chars[curx] = charcode;
    win->line[cury].FG[win->cursorx] = win->FG;
    win->line[cury].BG[win->cursorx] = win->BG;


    win->draw = true;
    addedchar(win);
    return 1;
}

//Move the cursor of the main window
int move(int y, int x)
{
    return wmove(mainwin, y, x);
}

//Set the amount of time getch waits for input
void timeout(int delay)
{
    curses_timeout(delay);
}
void set_escdelay(int delay) { } //PORTABILITY, DUMMY FUNCTION


int echo()
{
    echoOn = 1;
    return 0; // 0 = OK, -1 = ERR
}

int noecho()
{
    echoOn = 0;
    return 0;
}
#endif
