#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"

#include <cstring> // strlen

/**
 * Whoever cares, btw. not my base design, but this is how it works:
 * In absent of a native curses library, this is a simple implementation to
 * store the data that would be given to the curses system.
 * It is later displayed using code in sdltiles.cpp. This file only contains
 * the curses interface.
 *
 * The struct WINDOW is the base. It acts as the normal curses window, having
 * width/height, current cursor location, current coloring (background/foreground)
 * and the actual text.
 * The text is split into lines (curseline), which contains cells (cursecell).
 * Each cell has individual foreground and background, and a character. The
 * character is an UTF-8 encoded string. It should be one or two console cells
 * width. If it's two cells width, the next cell in the line must be completely
 * empty (the string must not contain anything). Also the last cell of a line
 * must not contain a two cell width string.
 */

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
    newwindow->line.resize(nlines);

    for (int j = 0; j < nlines; j++) {
        newwindow->line[j].chars.resize(ncols);
        newwindow->line[j].touched = true; //Touch them all !?
    }
    return newwindow;
}

//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
    if (win == NULL) {
        return 1;
    }
    delete win;
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

// move the cursor a single cell, jumps to the next line if the
// end of a line has been reached, also sets the touched flag.
inline void addedchar(WINDOW *win)
{
    win->cursorx++;
    win->line[win->cursory].touched = true;
    if (win->cursorx >= win->width) {
        newline(win);
    }
}


//Borders the window with fancy lines!
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
            chtype bl, chtype br)
{
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
    wattron(win, BORDER_COLOR);
    if (ch) {
        for (int i = 0; i < n; i++) {
            mvwaddch(win, y, x + i, ch);
        }
    } else {
        for (int i = 0; i < n; i++) {
            mvwaddch(win, y, x + i, LINE_OXOX);
        }
    }
    wattroff(win, BORDER_COLOR);
    return 0;
}

int mvwvline(WINDOW *win, int y, int x, chtype ch, int n)
{
    wattron(win, BORDER_COLOR);
    if (ch) {
        for (int j = 0; j < n; j++) {
            mvwaddch(win, y + j, x, ch);
        }
    } else {
        for (int j = 0; j < n; j++) {
            mvwaddch(win, y + j, x, LINE_XOXO);
        }
    }
    wattroff(win, BORDER_COLOR);
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

// Get a sequence of Unicode code points, store them in target
// return the display width of the extracted string.
inline int fill(char *&fmt, int &len, std::string &target)
{
    char *const start = fmt;
    int dlen = 0; // display width
    const char *tmpptr = fmt; // pointer for UTF8_getch, which increments it
    int tmplen = len;
    while( tmplen > 0 ) {
        const unsigned ch = UTF8_getch(&tmpptr, &tmplen);
        // UNKNOWN_UNICODE is most likely a (vertical/horizontal) line or similar
        const int cw = (ch == UNKNOWN_UNICODE) ? 1 : mk_wcwidth(ch);
        if( cw > 0 && dlen > 0 ) {
            // Stop at the *second* non-zero-width character
            break;
        } else if( cw == -1 && start == fmt ) {
            // First char is a control character: they only disturb the screen,
            // so replace it with a single space (e.g. instead of a '\t').
            // Newlines at the begin of a sequence are handled in printstring
            target.assign( " ", 1 );
            len = tmplen;
            fmt = const_cast<char *>(tmpptr);
            return 1; // the space
        } else if( cw == -1 ) {
            // Control character but behind some other characters, finish the sequence.
            // The character will either by handled by printstring (if it's a newline),
            // or by the next call to this function (replaced with a space).
            break;
        }
        fmt = const_cast<char *>(tmpptr);
        dlen += cw;
    }
    target.assign(start, fmt - start);
    len -= target.length();
    return dlen;
}

// The current cell of the window, pointed to by the cursor. The next character
// written to that window should go in this cell.
// Returns nullptr if the cursor is invalid (outside the window).
inline cursecell *cur_cell(WINDOW *win)
{
    if( win->cursory >= win->height || win->cursorx >= win->width ) {
        return nullptr;
    }
    return &(win->line[win->cursory].chars[win->cursorx]);
}

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
    win->draw = true;
    int len = strlen(fmt);
    if( len == 0 ) {
        return 1;
    }
    // avoid having an invalid cursorx, so that cur_cell will only return nullptr
    // when the bottom of the window has been reached.
    if( win->cursorx >= win->width ) {
        if( newline( win ) == 0 ) {
            return 0;
        }
    }
    if( win->cursory >= win->height || win->cursorx >= win->width ) {
        return 0;
    }
    if( win->cursorx > 0 && win->line[win->cursory].chars[win->cursorx].ch.empty() ) {
        // start inside a wide character, erase it for good
        win->line[win->cursory].chars[win->cursorx - 1].ch.assign(" ");
    }
    while( len > 0 ) {
        if( *fmt == '\n' ) {
            if( newline(win) == 0 ) {
                return 0;
            }
            fmt++;
            len--;
            continue;
        }
        cursecell *curcell = cur_cell( win );
        if( curcell == nullptr ) {
            return 0;
        }
        const int dlen = fill(fmt, len, curcell->ch);
        if( dlen >= 1 ) {
            curcell->FG = win->FG;
            curcell->BG = win->BG;
            addedchar( win );
        }
        if( dlen == 1 ) {
            // a wide character was converted to a narrow character leaving a null in the
            // following cell ~> clear it
            cursecell *seccell = cur_cell( win );
            if (seccell && seccell->ch.empty()) {
                seccell->ch.assign(' ', 1);
            }
        } else if( dlen == 2 ) {
            // the second cell, per definition must be empty
            cursecell *seccell = cur_cell( win );
            if( seccell == nullptr ) {
                // the previous cell was valid, this one is outside of the window
                // --> the previous was the last cell of the last line
                // --> there should not be a two-cell width character in the last cell
                curcell->ch.assign(' ', 1);
                return 0;
            }
            seccell->FG = win->FG;
            seccell->BG = win->BG;
            seccell->ch.erase();
            addedchar( win );
            // Have just written a wide-character into the last cell, it would not
            // display correctly if it was the last *cell* of a line
            if( win->cursorx == 0 ) {
                // So make that last cell a space, move the width
                // character in the first cell of the line
                seccell->ch = curcell->ch;
                curcell->ch.assign(1, ' ');
                // and make the second cell on the new line empty.
                addedchar( win );
                cursecell *thicell = cur_cell( win );
                if( thicell != nullptr ) {
                    thicell->ch.erase();
                }
            }
        }
        if( win->cursory >= win->height ) {
            return 0;
        }
    }
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
    for (int j = 0; j < win->height; j++) {
        win->line[j].chars.assign(win->width, cursecell());
        win->line[j].touched = true;
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
    for (int i = 0; i < win->y && i < stdscr->height; i++) {
        stdscr->line[i].touched = true;
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

int keypad(WINDOW *, bool)
{
    return 1;
}

int cbreak(void)
{
    return 1;
}
int keypad(int, bool)
{
    return 1;
}
int curs_set(int)
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
int wattroff(WINDOW *win, int)
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
    case LINE_XOXO:
        charcode = LINE_XOXO_C;
        break;
    case LINE_OXOX:
        charcode = LINE_OXOX_C;
        break;
    case LINE_XXOO:
        charcode = LINE_XXOO_C;
        break;
    case LINE_OXXO:
        charcode = LINE_OXXO_C;
        break;
    case LINE_OOXX:
        charcode = LINE_OOXX_C;
        break;
    case LINE_XOOX:
        charcode = LINE_XOOX_C;
        break;
    case LINE_XXOX:
        charcode = LINE_XXOX_C;
        break;
    case LINE_XXXO:
        charcode = LINE_XXXO_C;
        break;
    case LINE_XOXX:
        charcode = LINE_XOXX_C;
        break;
    case LINE_OXXX:
        charcode = LINE_OXXX_C;
        break;
    case LINE_XXXX:
        charcode = LINE_XXXX_C;
        break;
    default:
        charcode = (char)ch;
        break;
    }
    char buffer[2] = { charcode, '\0' };
    return printstring( win, buffer );
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
void set_escdelay(int) { } //PORTABILITY, DUMMY FUNCTION


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
