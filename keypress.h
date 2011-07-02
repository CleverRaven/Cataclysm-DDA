#ifndef _KEYPRESS_H_
#define _KEYPRESS_H_
#include <curses.h>

/* Fix the backspace key on Windows (by jaydg) */
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
# if defined KEY_BACKSPACE
# undef KEY_BACKSPACE
# endif
# define KEY_BACKSPACE 8
#endif

// Simple text input--translates numpad to vikeys
long input();
// If ch is vikey, x & y are set to corresponding direction; ch=='y'->x=-1,y=-1
void get_direction(int &x, int &y, char ch);

#define CTRL(n) (n - 'A' + 1 < 1 ? n - 'a' + 1 : n - 'A' + 1)
#define KEY_ESCAPE 27
#endif
