#pragma once
#ifndef CATACURSE_H
#define CATACURSE_H

#include <stdio.h>
#include <map>
#include <vector>
#include <array>
#include <string>
#include <cstdint>

#include "printf_check.h"

class nc_color;

typedef int chtype;
typedef unsigned short attr_t;

//a pair of colors[] indexes, foreground and background
typedef struct {
    int FG;
    int BG;
} pairs;

//Individual lines, so that we can track changed lines
struct cursecell {
    std::string ch;
    char FG = 0;
    char BG = 0;

    cursecell( std::string ch ) : ch( std::move( ch ) ) { }
    cursecell() : cursecell( std::string( 1, ' ' ) ) { }

    bool operator==( const cursecell &b ) const {
        return FG == b.FG && BG == b.BG && ch == b.ch;
    }
};

struct curseline {
    bool touched;
    std::vector<cursecell> chars;
};

//The curses window struct
struct WINDOW {
    int x;//left side of window
    int y;//top side of window
    int width;
    int height;
    int FG;//current foreground color from attron
    int BG;//current background color from attron
    bool inuse;// Does this window actually exist?
    bool draw;//Tracks if the window text has been changed
    int cursorx;
    int cursory;
    std::vector<curseline> line;
};

#define COLOR_BLACK 0x00    // RGB{0, 0, 0}
#define COLOR_RED 0x01      // RGB{196, 0, 0}
#define COLOR_GREEN 0x02    // RGB{0, 196, 0}
#define COLOR_YELLOW 0x03   // RGB{196, 180, 30}
#define COLOR_BLUE 0x04     // RGB{0, 0, 196}
#define COLOR_MAGENTA 0x05  // RGB{196, 0, 180}
#define COLOR_CYAN 0x06     // RGB{0, 170, 200}
#define COLOR_WHITE 0x07    // RGB{196, 196, 196}

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

#define OK (0)   // Success return.

/* Curses external declarations. */

extern WINDOW *stdscr;

//Curses Functions
WINDOW *newwin( int nlines, int ncols, int begin_y, int begin_x );
int delwin( WINDOW *win );
int wborder( WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
             chtype bl, chtype br );

int mvwhline( WINDOW *win, int y, int x, chtype ch, int n );
int mvwvline( WINDOW *win, int y, int x, chtype ch, int n );

int wrefresh( WINDOW *win );
int refresh( void );
int wredrawln( WINDOW *win, int beg_line, int num_lines );
int getch( void );
int wgetch( WINDOW *win );
int mvgetch( int y, int x );
int mvwgetch( WINDOW *win, int y, int x );
int mvwprintw( WINDOW *win, int y, int x, const char *fmt, ... ) PRINTF_LIKE( 4, 5 );
int werase( WINDOW *win );
int init_pair( short pair, short f, short b );
int wmove( WINDOW *win, int y, int x );
int clear( void );
int erase( void );
int endwin( void );
int mvwaddch( WINDOW *win, int y, int x, const chtype ch );
int wclear( WINDOW *win );
int wprintw( WINDOW *win, const char *fmt, ... ) PRINTF_LIKE( 2, 3 );
int curs_set( int visibility ); //PORTABILITY, DUMMY FUNCTION
int wattron( WINDOW *win, const nc_color &attrs );
int wattroff( WINDOW *win, int attrs );
int waddch( WINDOW *win, const chtype ch );
int getmaxx( WINDOW *win );
int getmaxy( WINDOW *win );
int getbegx( WINDOW *win );
int getbegy( WINDOW *win );
int getcurx( WINDOW *win );
int getcury( WINDOW *win );
//non-curses functions, Do not call these in the main game code
extern std::array<pairs, 100> colorpairs;
void curses_drawwindow( WINDOW *win );

int projected_window_width( int column_count );
int projected_window_height( int row_count );

//used only in SDL mode for clearing windows using rendering
void clear_window_area( WINDOW *win );

#endif
