#ifndef CURSESDEF_H
#define CURSESDEF_H

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#elif (defined __CYGWIN__)
#include "ncurses/curses.h"
void init_interface();
#else
#include <curses.h>
void init_interface();
#endif

// provide a forward declaration so curses and non-curses can use the same code
// curses will refer to a dummy method in sdldummy.cpp
void try_sdl_update();

#endif
