#ifndef _CURSES_DEF_H_
#define _CURSES_DEF_H_

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    #include "catacurse.h"
#elif (defined __CYGWIN__)
    #include "ncurses/curses.h"
#else
    #include <curses.h>
    void init_interface();
#endif

#endif // CURSES_DEF_H
