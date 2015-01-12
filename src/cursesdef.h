#ifndef CURSESDEF_H
#define CURSESDEF_H

#include "platform.h"

#if defined(TILES) || defined(SDLTILES) || defined(CATA_OS_WINDOWS)
#include "catacurse.h"
#elif defined(CATA_PLATFORM_CYGWIN)
#include "ncurses/curses.h"
void init_interface();
#else
#include <curses.h>
void init_interface();
#endif

#endif
