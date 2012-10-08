#include "keypress.h"
#include "action.h"
#include "game.h"

long input()
{
 long ch = getch();
 switch (ch) {
  case KEY_UP:    return 'k';
  case KEY_LEFT:  return 'h';
  case KEY_RIGHT: return 'l';
  case KEY_DOWN:  return 'j';
  case 459: return '\n';
  default:  return ch;
 }
}

bool input_wait(char & ret_ch, int delay_ms)
{
 for(;;)
 {
  ret_ch = '\0';
  timeout(delay_ms);
  long ch = getch();
  switch (ch) {
   case KEY_UP:
    ret_ch = 'k';
    break;
   case KEY_LEFT:
    ret_ch = 'h';
    break;
   case KEY_RIGHT:
    ret_ch = 'l';
    break;
   case KEY_DOWN:
    ret_ch = 'j';
    break;
   case 459:
    ret_ch = '\n';
    break;
   case ERR:
    if(errno == EINTR)
     return input_wait(ret_ch, delay_ms);
    break;
   default:
    ret_ch = ch;
    break;
  }
  timeout(-1);
  if( ret_ch != '\0' )
   return true;
  return false;
 }
}

void get_direction(int &x, int &y, char ch)
{
 x = 0;
 y = 0;
 switch (ch) {
 case 'y':
  x = -1;
  y = -1;
  return;
 case 'u':
  x = 1;
  y = -1;
  return;
 case 'h':
  x = -1;
  return;
 case 'j':
  y = 1;
  return;
 case 'k':
  y = -1;
  return;
 case 'l':
  x = 1;
  return;
 case 'b':
  x = -1;
  y = 1;
  return;
 case 'n':
  x = 1;
  y = 1;
  return;
 case '.':
 case ',':
 case 'g':
  x = 0;
  y = 0;
  return;
 default:
  x = -2;
  y = -2;
 }
}

void get_direction(game *g, int &x, int &y, char ch)
{
 x = 0;
 y = 0;
 action_id act;
 if (g->keymap.find(ch) == g->keymap.end())
  act = ACTION_NULL;
 else
  act = g->keymap[ch];

 switch (act) {
 case ACTION_MOVE_NW:
  x = -1;
  y = -1;
  return;
 case ACTION_MOVE_NE:
  x = 1;
  y = -1;
  return;
 case ACTION_MOVE_W:
  x = -1;
  return;
 case ACTION_MOVE_S:
  y = 1;
  return;
 case ACTION_MOVE_N:
  y = -1;
  return;
 case ACTION_MOVE_E:
  x = 1;
  return;
 case ACTION_MOVE_SW:
  x = -1;
  y = 1;
  return;
 case ACTION_MOVE_SE:
  x = 1;
  y = 1;
  return;
 case ACTION_PAUSE:
 case ACTION_PICKUP:
  x = 0;
  y = 0;
  return;
 default:
  x = -2;
  y = -2;
 }
}

std::string default_keymap_txt()
{
 return "\
# This is the keymapping for Cataclysm.\n\
# You can start a line with # to make it a comment--it will be ignored.\n\
# Blank lines are ignored too.\n\
# Extra whitespace, including tab, is ignored, so format things how you like.\n\
# If you wish to restore defaults, simply remove this file.\n\
\n\
# The format for each line is an action identifier, followed by several\n\
# keys.  Any action may have an unlimited number of keys bound to it.\n\
# If you bind the same key to multiple actions, the second and subsequent\n\
# bindings will be ignored--and you'll get a warning when the game starts.\n\
# Keys are case-sensitive, of course; c and C are different.\n\
 \n\
# WARNING: If you skip an action identifier, there will be no key bound to\n\
# that action!  You will be NOT be warned of this when the game starts.\n\
# If you're going to mess with stuff, maybe you should back this file up?\n\
\n\
# It is okay to split commands across lines.\n\
# pause . 5      is equivalent to:\n\
# pause .\n\
# pause 5\n\
\n\
# Note that movement keybindings ONLY apply to movement (for now).\n\
# That is, binding w to move_n will let you use w to move north, but you\n\
# cannot use w to smash north, examine to the north, etc.\n\
# For now, you must use vikeys, the numpad, or arrow keys for those actions.\n\
# This is planned to change in the future.\n\
\n\
# Finally, there is no support for special keys, like spacebar, Home, and\n\
# so on.  This is not a planned feature, but if it's important to you, please\n\
# let me know.\n\
\n\
# MOVEMENT:\n\
pause     . 5\n\
move_n    k 8\n\
move_ne   u 9\n\
move_e    l 6\n\
move_se   n 3\n\
move_s    j 2\n\
move_sw   b 1\n\
move_w    h 4\n\
move_nw   y 7\n\
move_down >\n\
move_up   <\n\
\n\
# ENVIRONMENT INTERACTION\n\
open  o\n\
close c\n\
smash s\n\
examine e\n\
pickup , g\n\
butcher B\n\
chat C\n\
look ; x\n\
\n\
# INVENTORY & QUASI-INVENTORY INTERACTION\n\
inventory i\n\
organize =\n\
apply a\n\
wear W\n\
take_off T\n\
eat E\n\
read R\n\
wield w\n\
pick_style _\n\
reload r\n\
unload U\n\
throw t\n\
fire f\n\
fire_burst F\n\
drop d\n\
drop_adj D\n\
bionics p\n\
\n\
# LONG TERM & SPECIAL ACTIONS\n\
wait ^\n\
craft &\n\
construct *\n\
sleep $\n\
safemode !\n\
autosafe \"\n\
ignore_enemy '\n\
save S\n\
quit Q\n\
\n\
# INFO SCREENS\n\
player_data @\n\
map m :\n\
missions M\n\
factions #\n\
morale %\n\
messages P\n\
help ?\n\
\n\
# DEBUG FUNCTIONS\n\
debug_mode ~\n\
# debug Z\n\
# debug_scent -\n\
";
}
