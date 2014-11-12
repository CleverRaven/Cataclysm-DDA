#ifndef CURSESDEF_H
#define CURSESDEF_H

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#elif (defined __CYGWIN__)
#include "ncurses/curses.h"
void init_interface();
#else
#include <curses.h>
void init_interface();
#endif

#endif
