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
#ifdef LOCALIZE
#include <libintl.h>
#endif
#include "translations.h"
#if (defined OSX_SDL_FW)
#include "SDL.h"
#elif (defined OSX_SDL_LIBS)
#include "SDL/SDL.h"
#endif

void exit_handler(int s);

#ifdef USE_WINMAIN
int APIENTRY	WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
 int argc = __argc;
 char **argv = __argv;
#else
int main(int argc, char *argv[])
{
#endif
#ifdef ENABLE_LOGGING
  setupDebug();
#endif
 int seed = time(NULL);

// set locale to system default
 setlocale(LC_ALL, "");
#ifdef LOCALIZE
 bindtextdomain("cataclysm-dda", "lang/mo");
 bind_textdomain_codeset("cataclysm-dda", "UTF-8");
 textdomain("cataclysm-dda");
#endif

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
 initOptions();
 load_options(); // For getting size options
 initscr(); // Initialize ncurses
 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
// curs_set(0); // Invisible cursor
 set_escdelay(10); // Make escape actually responsive

 std::srand(seed);

 bool quit_game = false;
 bool delete_world = false;
 g = new game;
 if(g->game_error())
  exit_handler(-999);
 g->init_ui();
 MAPBUFFER.set_game(g);
 g->load_artifacts(); //artifacts have to be loaded before any items are created
 if(g->game_error())
  exit_handler(-999);
 MAPBUFFER.load();

 curs_set(0); // Invisible cursor here, because MAPBUFFER.load() is crash-prone

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
  if (g->game_quit() || g->game_error())
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
  if (query_yn(_("Really Quit? All unsaved changes will be lost."))) {
   bExit = true;
  }
 } else if (s == -999) {
  bExit = true;
 } else {
  //query_yn(g, "Signal received: %d", s);
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

  if(g != NULL)
   if(g->game_error())
    exit(1);
  exit(0);
 }
}
