/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#else
	#include <curses.h>
#endif

#include <ctime>
#include "game.h"
#include "color.h"
#include "options.h"
#include "mapbuffer.h"
#include "debug.h"

int main(int argc, char *argv[])
{
#ifdef ENABLE_LOGGING
    setupDebug();
#endif
 srand(time(NULL));

// ncurses stuff
 initscr(); // Initialize ncurses
 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
 curs_set(0); // Invisible cursor

 rand();  // For some reason a call to rand() seems to be necessary to avoid
          // repetion.
 bool quit_game = false;
 game *g = new game;
 MAPBUFFER = mapbuffer(g);
 MAPBUFFER.load();
 load_options();
 do {
  g->setup();
  while (!g->do_turn());
  if (g->game_quit())
   quit_game = true;
 } while (!quit_game);
 MAPBUFFER.save();
 erase(); // Clear screen
 endwin(); // End ncurses
 system("clear"); // Tell the terminal to clear itself
 return 0;
}

