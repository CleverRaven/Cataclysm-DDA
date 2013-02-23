#ifndef __CATACURSE__
#define __CATACURSE__
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
//#define VC_EXTRALEAN
#include <windows.h>
#include <stdio.h>
typedef int	chtype;
typedef unsigned short	attr_t;
typedef unsigned int u_int32_t;

#define __CHARTEXT	0x000000ff	/* bits for 8-bit characters */             //<---------not used
#define __NORMAL	0x00000000	/* Added characters are normal. */          //<---------not used
#define __STANDOUT	0x00000100	/* Added characters are standout. */        //<---------not used
#define __UNDERSCORE	0x00000200	/* Added characters are underscored. */ //<---------not used
#define __REVERSE	0x00000400	/* Added characters are reverse             //<---------not used
					   video. */
#define __BLINK		0x00000800	/* Added characters are blinking. */
#define __DIM		0x00001000	/* Added characters are dim. */             //<---------not used
#define __BOLD		0x00002000	/* Added characters are bold. */
#define __BLANK		0x00004000	/* Added characters are blanked. */         //<---------not used
#define __PROTECT	0x00008000	/* Added characters are protected. */       //<---------not used
#define __ALTCHARSET	0x00010000	/* Added characters are ACS */          //<---------not used
#define __COLOR		0x03fe0000	/* Color bits */
#define __ATTRIBUTES	0x03ffff00	/* All 8-bit attribute bits */          //<---------not used

//a pair of colors[] indexes, foreground and background
typedef struct
{
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
typedef struct{
bool touched;
char *chars;
char *FG;
char *BG;
//cursechar chars [80];
} curseline;
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
  curseline *line;

} WINDOW;

#define	A_NORMAL	__NORMAL
#define	A_STANDOUT	__STANDOUT
#define	A_UNDERLINE	__UNDERSCORE
#define	A_REVERSE	__REVERSE
#define	A_BLINK		__BLINK
#define	A_DIM		__DIM
#define	A_BOLD		__BOLD
#define	A_BLANK		__BLANK
#define	A_PROTECT	__PROTECT
#define	A_ALTCHARSET	__ALTCHARSET
#define	A_ATTRIBUTES	__ATTRIBUTES
#define	A_CHARTEXT	__CHARTEXT
#define	A_COLOR		__COLOR

#define	COLOR_BLACK	0x00        //RGB{0,0,0}
#define	COLOR_RED	0x01        //RGB{196, 0, 0}
#define	COLOR_GREEN	0x02        //RGB{0,196,0}
#define	COLOR_YELLOW	0x03    //RGB{196,180,30}
#define	COLOR_BLUE	0x04        //RGB{0, 0, 196}
#define	COLOR_MAGENTA	0x05    //RGB{196, 0, 180}
#define	COLOR_CYAN	0x06        //RGB{0, 170, 200}
#define	COLOR_WHITE	0x07        //RGB{196, 196, 196}

#define	COLOR_PAIR(n)	((((u_int32_t)n) << 17) & A_COLOR)
//#define	PAIR_NUMBER(n)	((((u_int32_t)n) & A_COLOR) >> 17)

#define    KEY_MIN        0x101    /* minimum extended key value */ //<---------not used
#define    KEY_BREAK      0x101    /* break key */                  //<---------not used
#define    KEY_DOWN       0x102    /* down arrow */
#define    KEY_UP         0x103    /* up arrow */
#define    KEY_LEFT       0x104    /* left arrow */
#define    KEY_RIGHT      0x105    /* right arrow*/
#define    KEY_HOME       0x106    /* home key */                   //<---------not used
#define    KEY_BACKSPACE  0x107    /* Backspace */                  //<---------not used

/* Curses external declarations. */

#define stdscr 0
//WINDOW *stdscr; //Define this for portability, strscr will basically be window[0] anyways

#define getmaxyx(w, y, x)  (y = getmaxy(w), x = getmaxx(w))

//Curses Functions
#define	ERR	(-1)			/* Error return. */
#define	OK	(0)			/* Success return. */
WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
int delwin(WINDOW *win);
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br);
int wrefresh(WINDOW *win);
int refresh(void);
int getch(void);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int werase(WINDOW *win);
int start_color(void);
int init_pair(short pair, short f, short b);
int wmove(WINDOW *win, int y, int x);

int clear(void);
int erase(void);
int endwin(void);
int mvwaddch(WINDOW *win, int y, int x, const chtype ch);
int wclear(WINDOW *win);
int wprintw(WINDOW *win, const char *fmt, ...);
WINDOW *initscr(void);
int noecho(void);//PORTABILITY, DUMMY FUNCTION
int cbreak(void);//PORTABILITY, DUMMY FUNCTION
int keypad(WINDOW *faux, bool bf);//PORTABILITY, DUMMY FUNCTION
int curs_set(int visibility);//PORTABILITY, DUMMY FUNCTION
int mvaddch(int y, int x, const chtype ch);
int wattron(WINDOW *win, int attrs);
int wattroff(WINDOW *win, int attrs);
int attron(int attrs);
int attroff(int attrs);
int waddch(WINDOW *win, const chtype ch);
int printw(const char *fmt,...);
int getmaxx(WINDOW *win);
int getmaxy(WINDOW *win);
int move(int y, int x);
void timeout(int delay);//PORTABILITY, DUMMY FUNCTION

//Window Functions, Do not call these outside of catacurse.cpp
void WinDestroy();
bool WinCreate(bool initgl);
void CheckMessages();
int FindWin(WINDOW *wnd);
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,u_int32_t Msg,WPARAM wParam, LPARAM lParam);
#endif
