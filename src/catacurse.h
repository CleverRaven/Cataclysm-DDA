#ifndef CATACURSE_H
#define CATACURSE_H

#if (defined _WIN32 || defined WINDOWS)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
//#define VC_EXTRALEAN
#include "windows.h"
#include "mmsystem.h"
#endif
#include <stdio.h>
#include <map>
#include <vector>
#include <array>
#include <string>
typedef int chtype;
typedef unsigned short attr_t;
typedef unsigned int u_int32_t;

//a pair of colors[] indexes, foreground and background
typedef struct {
    int FG;//foreground index in colors[]
    int BG;//foreground index in colors[]
} pairs;

//The curse character struct, just a char, attribute, and color pair
//typedef struct
//{
// char character;//the ascii actual character
// int attrib;//attributes, mostly for A_BLINK and A_BOLD
// pairs color;//pair of foreground/background, indexed into colors[]
//} cursechar;

//Individual lines, so that we can track changed lines
struct cursecell {
    std::string ch;
    char FG;
    char BG;
    cursecell(std::string ch) : ch(ch), FG(0), BG(0) { }
    cursecell() : cursecell(" ") { }

    bool operator==(const cursecell &b) const {
        return ch == b.ch && FG == b.FG && BG == b.BG;
    }

    cursecell& operator=(const cursecell &b) {
        ch = b.ch;
        FG = b.FG;
        BG = b.BG;
        return *this;
    }
};
struct curseline {
    bool touched;
    std::vector<cursecell> chars;
};

//The curses window struct
typedef struct {
    int x;//left side of window
    int y;//top side of window
    int width;//width of the curses window
    int height;//height of the curses window
    int FG;//current foreground color from attron
    int BG;//current background color from attron
    bool inuse;// Does this window actually exist?
    bool draw;//Tracks if the window text has been changed
    int cursorx;//x location of the cursor
    int cursory;//y location of the cursor
    std::vector<curseline> line;

} WINDOW;

#define A_NORMAL 0x00000000 /* Added characters are normal.         <---------not used */
#define A_STANDOUT 0x00000100 /* Added characters are standout.     <---------not used */
#define A_UNDERLINE 0x00000200 /* Added characters are underscored. <---------not used */
#define A_REVERSE 0x00000400 /* Added characters are reverse        <---------not used */
#define A_BLINK  0x00000800 /* Added characters are blinking. */
#define A_DIM  0x00001000 /* Added characters are dim.              <---------not used */
#define A_BOLD  0x00002000 /* Added characters are bold. */
#define A_BLANK  0x00004000 /* Added characters are blanked.        <---------not used */
#define A_PROTECT 0x00008000 /* Added characters are protected.     <---------not used */
#define A_ALTCHARSET 0x00010000 /* Added characters are ACS         <---------not used */
#define A_ATTRIBUTES 0x03ffff00 /* All 8-bit attribute bits         <---------not used */
#define A_CHARTEXT 0x000000ff /* bits for 8-bit characters          <---------not used */
#define A_COLOR  0x03fe0000 /* Color bits */

#define COLOR_BLACK 0x00        //RGB{0,0,0}
#define COLOR_RED 0x01        //RGB{196, 0, 0}
#define COLOR_GREEN 0x02        //RGB{0,196,0}
#define COLOR_YELLOW 0x03    //RGB{196,180,30}
#define COLOR_BLUE 0x04        //RGB{0, 0, 196}
#define COLOR_MAGENTA 0x05    //RGB{196, 0, 180}
#define COLOR_CYAN 0x06        //RGB{0, 170, 200}
#define COLOR_WHITE 0x07        //RGB{196, 196, 196}

#define COLOR_PAIR(n) ((((u_int32_t)n) << 17) & A_COLOR)
//#define PAIR_NUMBER(n) ((((u_int32_t)n) & A_COLOR) >> 17)

#define    KEY_MIN        0x101    /* minimum extended key value */ //<---------not used
#define    KEY_BREAK      0x101    /* break key */                  //<---------not used
#define    KEY_DOWN       0x102    /* down arrow */
#define    KEY_UP         0x103    /* up arrow */
#define    KEY_LEFT       0x104    /* left arrow */
#define    KEY_RIGHT      0x105    /* right arrow*/
#define    KEY_HOME       0x106    /* home key */                   //<---------not used
#define    KEY_BACKSPACE  0x107    /* Backspace */                  //<---------not used
#define    KEY_DC         0x151    /* Delete Character */
#define    KEY_F(n)      (0x108+n) /* F1, F2, etc*/
#define    KEY_NPAGE      0x152    /* page down */
#define    KEY_PPAGE      0x153    /* page up */
#define    KEY_ENTER      0x157    /* enter */
#define    KEY_BTAB       0x161    /* back-tab = shift + tab */
#define    KEY_END        0x168    /* End */

#define ERR (-1) // Error return.
#define OK (0)   // Success return.

/* Curses external declarations. */

extern WINDOW *stdscr;

#define getmaxyx(w, y, x)  (y = getmaxy(w), x = getmaxx(w))

//Curses Functions
WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
int delwin(WINDOW *win);
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
            chtype bl, chtype br);

int hline(chtype ch, int n);
int vline(chtype ch, int n);
int whline(WINDOW *win, chtype ch, int n);
int wvline(WINDOW *win, chtype ch, int n);
int mvhline(int y, int x, chtype ch, int n);
int mvvline(int y, int x, chtype ch, int n);
int mvwhline(WINDOW *win, int y, int x, chtype ch, int n);
int mvwvline(WINDOW *win, int y, int x, chtype ch, int n);

int wrefresh(WINDOW *win);
int refresh(void);
int getch(void);
int wgetch(WINDOW *win);
int mvgetch(int y, int x);
int mvwgetch(WINDOW *win, int y, int x);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int werase(WINDOW *win);
int start_color(void);
int init_pair(short pair, short f, short b);
int wmove(WINDOW *win, int y, int x);
int getnstr(char *str, int size);
int clear(void);
int clearok(WINDOW *win);
int erase(void);
int endwin(void);
int mvwaddch(WINDOW *win, int y, int x, const chtype ch);
int wclear(WINDOW *win);
int wprintw(WINDOW *win, const char *fmt, ...);
WINDOW *initscr(void);
int cbreak(void);//PORTABILITY, DUMMY FUNCTION
int keypad(WINDOW *faux, bool bf);//PORTABILITY, DUMMY FUNCTION
int curs_set(int visibility);//PORTABILITY, DUMMY FUNCTION
int mvaddch(int y, int x, const chtype ch);
int wattron(WINDOW *win, int attrs);
int wattroff(WINDOW *win, int attrs);
int attron(int attrs);
int attroff(int attrs);
int waddch(WINDOW *win, const chtype ch);
int printw(const char *fmt, ...);
int getmaxx(WINDOW *win);
int getmaxy(WINDOW *win);
int getbegx(WINDOW *win);
int getbegy(WINDOW *win);
int getcurx(WINDOW *win);
int getcury(WINDOW *win);
int move(int y, int x);
void timeout(int delay);//PORTABILITY, DUMMY FUNCTION
void set_escdelay(int delay);//PORTABILITY, DUMMY FUNCTION
int echo(void);
int noecho(void);
//non-curses functions, Do not call these in the main game code
extern WINDOW *mainwin;
extern pairs *colorpairs;
// key is a color name from main_color_names,
// value is a color in *BGR*. each vector has exactly 3 values.
// see load_colors(Json...)
extern std::map< std::string, std::vector<int> > consolecolors;
// color names as read from the json file
extern std::array<std::string, 16> main_color_names;
WINDOW *curses_init();
int curses_destroy();
void curses_drawwindow(WINDOW *win);
void curses_delay(int delay);
void curses_timeout(int t);
int curses_getch(WINDOW *win);
int curses_start_color();

// Add interface specific (SDL/ncurses/wincurses) initializations here
void init_interface();

int projected_window_width(int column_count);
int projected_window_height(int row_count);

#endif
