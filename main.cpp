/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#elif (defined __CYGWIN__)
  #include "ncurses/curses.h"
#else
	#include <curses.h>
#endif

#include <ctime>
#include "game.h"
#include "color.h"
#include "options.h"
#include "mapbuffer.h"
#include "debug.h"
//#include <sys/stat.h>

int main(int argc, char *argv[])
{
#ifdef ENABLE_LOGGING
  setupDebug();
#endif
// ncurses stuff
 initscr(); // Initialize ncurses
 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
 curs_set(0); // Invisible cursor

 std::srand(time(NULL));

 bool quit_game = false;
 bool delete_world = false;
 game *g = new game;
 MAPBUFFER = mapbuffer(g);
 MAPBUFFER.load();
 load_options();
 do {
  g->setup();
  while (!g->do_turn()) ;
  if (g->uquit == QUIT_DELETE_WORLD)
    delete_world = true;
  if (g->game_quit())
   quit_game = true;
 } while (!quit_game);
 MAPBUFFER.save();

  if (delete_world && (remove("save/") != 0))
  {
    #if (defined _WIN32 || defined __WIN32__)
      system("rmdir /s /q save");
    #else
      system("rm -rf save/*");
    #endif
  }

 erase(); // Clear screen
 endwin(); // End ncurses
#if (defined _WIN32 || defined WINDOWS)
 system("cls"); // Tell the terminal to clear itself
 system("color 07");
#else
 system("clear"); // Tell the terminal to clear itself
#endif
 return 0;
}
