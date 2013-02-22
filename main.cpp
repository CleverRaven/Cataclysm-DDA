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
#include <sys/stat.h>
#include <cstdlib>

int main(int argc, char *argv[])
{
#ifdef ENABLE_LOGGING
  setupDebug();
#endif
 int seed = time(NULL);

//args: world seeding only.
 argc--; argv++;
 while (argc){
  if(std::string(argv[0]) == "--seed"){
   argc--; argv++;
   if(argc){
    seed = djb2_hash((unsigned char*)argv[0]);
    argc--; argv++;
   }
  }
  else // ignore unknown args.
   argc--; argv++;
 }

// ncurses stuff
 initscr(); // Initialize ncurses
 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
 curs_set(0); // Invisible cursor

 std::srand(seed);

 bool quit_game = false;
 bool delete_world = false;
 game *g = new game;
 MAPBUFFER.set_game(g);
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

 if (delete_world)
 {
   g->delete_save();
 } else {
  MAPBUFFER.save_if_dirty();
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
