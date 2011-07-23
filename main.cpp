/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#include <curses.h>
#include <ctime>
#include "game.h"
#include "color.h"

int main(int argc, char *argv[])
{
 srand(time(NULL));

 initscr();
 noecho();
 cbreak();
 keypad(stdscr, true);
 init_colors();
 curs_set(0);

 rand();
 bool quit_game = false;
 game *g;
 do {
  g = new game();
  while (!g->do_turn());
  if (g->game_quit())
   quit_game = true;
  delete g;
 } while (!quit_game);
 erase();
 endwin();
 system("clear");
 return 0;
}

