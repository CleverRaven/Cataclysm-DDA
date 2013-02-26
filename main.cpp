/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#include "cursesdef.h"
#include <ctime>
#include "game.h"
#include "color.h"
#include "options.h"
#include "mapbuffer.h"
#include "debug.h"
#include <sys/stat.h>
#include <cstdlib>
#include <signal.h>

void exit_handler(int s);

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
 load_options(); // For getting size options
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

 #if (!(defined _WIN32 || defined WINDOWS))
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = exit_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);
 #endif

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

 exit_handler(-999);

 return 0;
}

void exit_handler(int s) {
 bool bExit = false;

 if (s == 2) {
  if (query_yn("Really Quit without saving?")) {
   bExit = true;
  }
 } else if (s == -999) {
  bExit = true;
 } else {
  //query_yn("Signal received: %d", s);
  bExit = true;
 }

 if (bExit) {
  erase(); // Clear screen
  endwin(); // End ncurses
  #if (defined _WIN32 || defined WINDOWS)
   system("cls"); // Tell the terminal to clear itself
   system("color 07");
  #else
   system("clear"); // Tell the terminal to clear itself
  #endif

  exit(1);
 }
}
