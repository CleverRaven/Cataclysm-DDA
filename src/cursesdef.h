#ifndef CURSESDEF_H
#define CURSESDEF_H

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
extern void try_sdl_update();
#elif (defined __CYGWIN__)
#include "ncurses/curses.h"
void init_interface();
void try_sdl_update(){}
#else
#include <curses.h>
void init_interface();
void try_sdl_update(){}
#endif

#endif
