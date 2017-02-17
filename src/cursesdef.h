#pragma once
#ifndef CURSESDEF_H
#define CURSESDEF_H

#include <memory>

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#elif (defined __CYGWIN__)
#include "ncurses/curses.h"
void init_interface();
#else
#include <curses.h>
void init_interface();
#endif

struct delwin_functor {
    void operator()( WINDOW *w ) const;
};
/**
 * A Wrapper around the WINDOW pointer, it automatically deletes the
 * window (see delwin_functor) when the variable gets out of scope.
 * This includes calling werase, wrefresh and delwin.
 * Usage:
 * 1. Acquire a WINDOW pointer via @ref newwin like normal, store it in a pointer variable.
 * 2. Create a variable of type WINDOW_PTR *on the stack*, initialize it with the pointer from 1.
 * 3. Do the usual stuff with window, print, update, etc. but do *not* call delwin on it.
 * 4. When the function is left, the WINDOW_PTR variable is destroyed, and its destructor is called,
 *    it calls werase, wrefresh and most importantly delwin to free the memory associated wit the pointer.
 * To trigger the delwin call earlier call some_window_ptr.reset().
 * To prevent the delwin call when the function is left (because the window is already deleted or, it should
 * not be deleted), call some_window_ptr.release().
 */
using WINDOW_PTR = std::unique_ptr<WINDOW, delwin_functor>;

#endif
