#ifndef CURSESDEF_H
#define CURSESDEF_H

#include "platform_win.h"

#if (defined TILES || defined SDLTILES || defined CATA_OS_WINDOWS)
#include "catacurse.h"
#elif (defined __CYGWIN__)
#include "ncurses/curses.h"
void init_interface();
#else
#include <curses.h>
void init_interface();
#endif

#endif
