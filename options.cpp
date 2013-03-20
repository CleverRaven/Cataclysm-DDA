#include "game.h"
#include "options.h"
#include "output.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>

option_table OPTIONS;

option_key lookup_option_key(std::string id);
bool option_is_bool(option_key id);
void create_default_options();
std::string options_header();

void game::show_options()
{
 int iMaxX = (VIEWX < 12) ? 80 : (VIEWX*2)+56;
 int iMaxY = (VIEWY < 12) ? 25 : (VIEWY*2)+1;

 WINDOW* w_options_border = newwin(25, 80, (iMaxY > 25) ? (iMaxY-25)/2 : 0, (iMaxX > 80) ? (iMaxX-80)/2 : 0);
 WINDOW* w_options = newwin(23, 78, 1 + (int)((iMaxY > 25) ? (iMaxY-25)/2 : 0), 1 + (int)((iMaxX > 80) ? (iMaxX-80)/2 : 0));

 int offset = 1;
 int line = 0;
 char ch = ' ';
 bool changed_options = false;
 bool needs_refresh = true;
 do {
  wborder(w_options_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
  mvwprintz(w_options_border, 0, 36, c_ltred, " OPTIONS ");
  wrefresh(w_options_border);

// TODO: change instructions
  if (needs_refresh) {
    werase(w_options);
    mvwprintz(w_options, 0, 40, c_white, "Use up/down keys to scroll through");
    mvwprintz(w_options, 1, 40, c_white, "available options.");
    mvwprintz(w_options, 2, 40, c_white, "Use left/right keys to toggle.");
    mvwprintz(w_options, 3, 40, c_white, "Press ESC or q to return.             ");
// highlight options for option descriptions
    std::string tmp = option_desc(option_key(offset + line));
    std::string out;
    size_t pos;
    int displayline = 5;
    do {
      pos = tmp.find_first_of('\n');
      out = tmp.substr(0, pos);
      mvwprintz(w_options, displayline, 40, c_white, out.c_str());
      tmp = tmp.substr(pos + 1);
      displayline++;
    } while (pos != std::string::npos && displayline < 12);
   needs_refresh = false;
  }

// Clear the lines
  for (int i = 0; i < 25; i++)
   mvwprintz(w_options, i, 0, c_black, "                                        ");
  int valid_option_count = 0;

// display options
  for (int i = 0; i < 25 && offset + i < NUM_OPTION_KEYS; i++)
  {
       valid_option_count++;
       mvwprintz(w_options, i, 0, c_white, "%s: ",
                option_name( option_key(offset + i) ).c_str());

      if (option_is_bool(option_key(offset + i)))
      {
        bool on = OPTIONS[ option_key(offset + i) ];
        if (i == line)
          mvwprintz(w_options, i, 30, hilite(c_ltcyan), (on ? "True" : "False"));
        else
          mvwprintz(w_options, i, 30, (on ? c_ltgreen : c_ltred), (on ? "True" : "False"));
      } else {
        char option_val = OPTIONS[ option_key(offset + i) ];
        if (i == line)
          mvwprintz(w_options, i, 30, hilite(c_ltcyan), "%d", option_val );
        else
          mvwprintz(w_options, i, 30, c_ltgreen, "%d", option_val );
      }
  }
  wrefresh(w_options);
  refresh();
  ch = input();
  needs_refresh = true;
  refresh();

 switch (ch) {
// move up and down
  case 'j':
   line++;
   if (line == NUM_OPTION_KEYS - 1)
    line = 0;
   break;
  case 'k':
   line--;
   if (line < 0)
    line = NUM_OPTION_KEYS - 2;
   break;
// toggle options with left/right keys
  case 'h':
      if (option_is_bool(option_key(offset + line)))
        OPTIONS[ option_key(offset + line) ] = !(OPTIONS[ option_key(offset + line) ]);
      else
      {
        OPTIONS[ option_key(offset + line) ]--;
        if ((OPTIONS[ option_key(offset + line) ]) < 0 )
          OPTIONS[ option_key(offset + line) ] = option_max_options(option_key(offset + line)) - 1;
      }
      changed_options = true;
  break;
  case 'l':
    if (option_is_bool(option_key(offset + line)))
      OPTIONS[ option_key(offset + line) ] = !(OPTIONS[ option_key(offset + line) ]);
    else
    {
      OPTIONS[ option_key(offset + line) ]++;
      if ((OPTIONS[ option_key(offset + line) ]) >= option_max_options(option_key(offset + line)))
        OPTIONS[ option_key(offset + line) ] = 0;
    }
    changed_options = true;
  break;
  }
 } while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

 if (changed_options && query_yn(this->VIEWX, this->VIEWY, "Save changes?"))
  save_options();
 werase(w_options);
}

void load_options()
{
 std::ifstream fin;
 fin.open("data/options.txt");
 if (!fin.is_open()) {
  fin.close();
  create_default_options();
  fin.open("data/options.txt");
  if (!fin.is_open()) {
   debugmsg("Could neither read nor create ./data/options.txt");
   return;
  }
 }

 while (!fin.eof()) {
  std::string id;
  fin >> id;
  if (id == "")
   getline(fin, id); // Empty line, chomp it
  else if (id[0] == '#') // # indicates a comment
   getline(fin, id);
  else {
   option_key key = lookup_option_key(id);
   if (key == OPT_NULL) {
    debugmsg("Bad option: %s", id.c_str());
    getline(fin, id);
   } else if (option_is_bool(key)) {
    std::string val;
    fin >> val;
    if (val == "T")
     OPTIONS[key] = 1.;
    else
     OPTIONS[key] = 0.;
   } else {
    std::string check;
    double val;
    fin >> check;

    if (check == "T" || check == "F")
     val = (check == "T") ? 1.: 0.;
    else
     val = atoi(check.c_str());

    OPTIONS[key] = val;
   }
  }
  if (fin.peek() == '\n')
   getline(fin, id); // Chomp
 }
 fin.close();
}

option_key lookup_option_key(std::string id)
{
 if (id == "use_celsius")
  return OPT_USE_CELSIUS;
 if (id == "use_metric_system")
  return OPT_USE_METRIC_SYS;
 if (id == "force_capital_yn")
  return OPT_FORCE_YN;
 if (id == "no_bright_backgrounds")
  return OPT_NO_CBLINK;
 if (id == "24_hour")
  return OPT_24_HOUR;
 if (id == "snap_to_target")
  return OPT_SNAP_TO_TARGET;
 if (id == "safemode")
  return OPT_SAFEMODE;
 if (id == "safemodeproximity")
  return OPT_SAFEMODEPROXIMITY;
 if (id == "autosafemode")
  return OPT_AUTOSAFEMODE;
 if (id == "autosafemodeturns")
  return OPT_AUTOSAFEMODETURNS;
 if (id == "autosave")
  return OPT_AUTOSAVE;
 if (id == "gradual_night_light")
  return OPT_GRADUAL_NIGHT_LIGHT;
 if (id == "query_disassemble")
  return OPT_QUERY_DISASSEMBLE;
 if (id == "drop_empty")
  return OPT_DROP_EMPTY;
 if (id == "skill_rust")
  return OPT_SKILL_RUST;
 if (id == "delete_world")
  return OPT_DELETE_WORLD;
 if (id == "initial_points")
  return OPT_INITIAL_POINTS;
 if(id == "initial_time")
  return OPT_INITIAL_TIME;
 if (id == "viewport_x")
  return OPT_VIEWPORT_X;
 if (id == "viewport_y")
  return OPT_VIEWPORT_Y;
 if (id == "static_spawn")
  return OPT_STATIC_SPAWN;
 return OPT_NULL;
}

std::string option_string(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "use_celsius";
  case OPT_USE_METRIC_SYS: return "use_metric_system";
  case OPT_FORCE_YN:		return "force_capital_yn";
  case OPT_NO_CBLINK:		return "no_bright_backgrounds";
  case OPT_24_HOUR:		return "24_hour";
  case OPT_SNAP_TO_TARGET:	return "snap_to_target";
  case OPT_SAFEMODE:		return "safemode";
  case OPT_SAFEMODEPROXIMITY: return "safemodeproximity";
  case OPT_AUTOSAFEMODE:	return "autosafemode";
  case OPT_AUTOSAFEMODETURNS: return "autosafemodeturns";
  case OPT_AUTOSAVE:    	return "autosave";
  case OPT_GRADUAL_NIGHT_LIGHT: return "gradual_night_light";
  case OPT_QUERY_DISASSEMBLE: return "query_disassemble";
  case OPT_DROP_EMPTY: return "drop_empty";
  case OPT_SKILL_RUST: return "skill_rust";
  case OPT_DELETE_WORLD: return "delete_world";
  case OPT_INITIAL_POINTS: return "initial_points";
  case OPT_INITIAL_TIME: return "initial_time";
  case OPT_VIEWPORT_X: return "viewport_x";
  case OPT_VIEWPORT_Y: return "viewport_y";
  case OPT_STATIC_SPAWN: return "static_spawn";
  default:			return "unknown_option";
 }
 return "unknown_option";
}

std::string option_desc(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "If true, use C not F";
  case OPT_USE_METRIC_SYS:	return "If true, use Km/h not mph";
  case OPT_FORCE_YN:		return "If true, y/n prompts are case-sensitive\nand y and n are not accepted";
  case OPT_NO_CBLINK:		return "If true, bright backgrounds are not\nused--some consoles are not compatible";
  case OPT_24_HOUR:		return "12h/24h Time:\n0 - AM/PM\n1 - 24h military\n2 - 24h normal";
  case OPT_SNAP_TO_TARGET:	return "If true, automatically follow the\ncrosshair when firing/throwing";
  case OPT_SAFEMODE:		return "If true, safemode will be on after\nstarting a new game or loading";
  case OPT_SAFEMODEPROXIMITY: return "If safemode is enabled,\ndistance to hostiles when safemode\nshould show a warning (0=Viewdistance)";
  case OPT_AUTOSAFEMODE:	return "If true, auto-safemode will be on\nafter starting a new game or loading";
  case OPT_AUTOSAFEMODETURNS: return "Number of turns after safemode\nis reenabled if no hostiles are\nin safemodeproximity distance";
  case OPT_AUTOSAVE:    	return "If true, game will periodically\nsave the map";
  case OPT_GRADUAL_NIGHT_LIGHT: return "If true will add nice gradual-lighting\n(should only make a difference @night)";
  case OPT_QUERY_DISASSEMBLE: return "If true, will query before disassembling\nitems";
  case OPT_DROP_EMPTY: return "Set to drop empty containers after use\n0 - don't drop any\n1 - all except watertight containers\n2 - all containers";
  case OPT_SKILL_RUST: return "Set the level of skill rust\n0 - vanilla Cataclysm\n1 - capped at skill levels\n2 - none at all";
  case OPT_DELETE_WORLD: return "Delete saves upon player death\n0 - no\n1 - yes\n2 - query";
  case OPT_INITIAL_POINTS: return "Initial points available on character\ngeneration.  Default is 6";
  case OPT_INITIAL_TIME: return "Initial starting time of day on character\ngeneration.  Default is 8:00";
  case OPT_VIEWPORT_X: return "WINDOWS ONLY: Set the expansion of the viewport along\nthe X axis.  Must restart for changes\nto take effect.  Default is 12. POSIX\nsystems will use terminal size at startup.";
  case OPT_VIEWPORT_Y: return "WINDOWS ONLY: Set the expansion of the viewport along\nthe Y axis.  Must restart for changes\nto take effect.  Default is 12. POSIX\nsystems will use terminal size at startup.";
  case OPT_STATIC_SPAWN: return "Spawn zombies at game start instead of\nduring game. Must delete save directory\nafter changing for it to take effect.\nDefault is F";
  default:			return " ";
 }
 return "Big ol Bug";
}

std::string option_name(option_key key)
{
 switch (key) {
  case OPT_USE_CELSIUS:		return "Use Celsius";
  case OPT_USE_METRIC_SYS:	return "Use Metric System";
  case OPT_FORCE_YN:		return "Force Y/N in prompts";
  case OPT_NO_CBLINK:		return "No Bright Backgrounds";
  case OPT_24_HOUR:		return "24 Hour Time";
  case OPT_SNAP_TO_TARGET:	return "Snap to Target";
  case OPT_SAFEMODE:		return "Safemode on by default";
  case OPT_SAFEMODEPROXIMITY: return "Safemode proximity distance";
  case OPT_AUTOSAFEMODE:	return "Auto-Safemode on by default";
  case OPT_AUTOSAFEMODETURNS: return "Turns to reenable safemode";
  case OPT_AUTOSAVE:    	return "Periodically Autosave";
  case OPT_GRADUAL_NIGHT_LIGHT: return "Gradual night light";
  case OPT_QUERY_DISASSEMBLE: return "Query on disassembly";
  case OPT_DROP_EMPTY: return "Drop empty containers";
  case OPT_SKILL_RUST: return "Skill Rust";
  case OPT_DELETE_WORLD: return "Delete World";
  case OPT_INITIAL_POINTS: return "Initial points";
  case OPT_INITIAL_TIME: return "Initial time";
  case OPT_VIEWPORT_X: return "Viewport width";
  case OPT_VIEWPORT_Y: return "Viewport height";
  case OPT_STATIC_SPAWN: return "Static spawn";
  default:			return "Unknown Option (BUG)";
 }
 return "Big ol Bug";
}

bool option_is_bool(option_key id)
{
 switch (id) {
  case OPT_24_HOUR:
  case OPT_SAFEMODEPROXIMITY:
  case OPT_AUTOSAFEMODETURNS:
  case OPT_SKILL_RUST:
  case OPT_DROP_EMPTY:
  case OPT_DELETE_WORLD:
  case OPT_INITIAL_POINTS:
  case OPT_INITIAL_TIME:
  case OPT_VIEWPORT_X:
  case OPT_VIEWPORT_Y:
   return false;
    break;
  default:
   return true;
 }
 return true;
}

char option_max_options(option_key id)
{
  char ret;
  if (option_is_bool(id))
    ret = 2;
  else
    switch (id)
    {
      case OPT_24_HOUR:
        ret = 3;
        break;
      case OPT_SAFEMODEPROXIMITY:
        ret = 61;
        break;
      case OPT_AUTOSAFEMODETURNS:
        ret = 51;
        break;
      case OPT_INITIAL_POINTS:
        ret = 25;
        break;
      case OPT_INITIAL_TIME:
        ret = 24; // 0h to 23h
        break;
      case OPT_DELETE_WORLD:
      case OPT_DROP_EMPTY:
      case OPT_SKILL_RUST:
        ret = 3;
        break;
      case OPT_VIEWPORT_X:
      case OPT_VIEWPORT_Y:
        ret = 61; // TODO Set up min/max values so weird numbers don't have to be used.
        break;
      default:
        ret = 2;
        break;
    }
  return ret;
}

void create_default_options()
{
 std::ofstream fout;
 fout.open("data/options.txt");
 if (!fout.is_open())
  return;

 fout << options_header() << "\n\
# If true, use C not F\n\
use_celsius F\n\
# If true, use Km/h not mph\
use_metric_system F\n\
# If true, y/n prompts are case-sensitive, y and n are not accepted\n\
force_capital_yn T\n\
# If true, bright backgrounds are not used--some consoles are not compatible\n\
no_bright_backgrounds F\n\
# 12h/24h Time: 0 = AM/PM, 1 = 24h military, 2 = 24h normal\n\
24_hour 0\n\
# If true, automatically follow the crosshair when firing/throwing\n\
snap_to_target F\n\
# If true, safemode will be on after starting a new game or loading\n\
safemode T\n\
# If safemode is enabled, distance to hostiles when safemode should show a warning (0=Viewdistance)\n\
safemodeproximity 0\n\
# If true, auto-safemode will be on after starting a new game or loading\n\
autosafemode F\n\
# Number of turns after safemode is reenabled when no zombies are in safemodeproximity distance\n\
autosafemodeturns 50\n\
# If true, game will periodically save the map\n\
autosave F\n\
# If true will add nice gradual-lighting (should only make a difference @night)\n\
gradual_night_light F\n\
# If true, will query beefore disassembling items\n\
query_disassemble T\n\
# Player will automatically drop empty containers after use\n\
# 0 - don't drop any, 1 - drop all except watertight containers, 2 - drop all containers\n\
drop_empty 0\n\
# \n\
# GAMEPLAY OPTIONS: CHANGING THESE OPTIONS WILL AFFECT GAMEPLAY DIFFICULTY! \n\
# Level of skill rust: 0 - vanilla Cataclysm, 1 - capped at skill levels, 2 - none at all\n\
skill_rust 0\n\
# Delete world after player death: 0 - no, 1 - yes, 2 - query\n\
delete_world 0\n\
# Initial points available in character generation\n\
initial_points 6\n\
# Initial time at character generation\n\
initial_time 8\n\
# The width of the terrain window in characters.\n\
viewport_x 12\n\
# The height of the terrain window, which is also the height of the main window, in characters.\n\
viewport_y 12\n\
# Spawn zombies at game start instead of during the game.  You must create a new world after changing\n\
static_spawn T\n\
";
 fout.close();
}

std::string options_header()
{
 return "\
# This is the options file.  It works similarly to keymap.txt: the format is\n\
# <option name> <option value>\n\
# <option value> may be any number, positive or negative.  If you use a\n\
# negative sign, do not put a space between it and the number.\n\
#\n\
# If # is at the start of a line, it is considered a comment and is ignored.\n\
# In-line commenting is not allowed.  I think.\n\
#\n\
# If you want to restore the default options, simply delete this file.\n\
# A new options.txt will be created next time you play.\n\
\n\
";
}

void save_options()
{
 std::ofstream fout;
 fout.open("data/options.txt");
 if (!fout.is_open())
  return;

 fout << options_header() << std::endl;
 for (int i = 1; i < NUM_OPTION_KEYS; i++) {
  option_key key = option_key(i);
  fout << option_string(key) << " ";
  if (option_is_bool(key))
   fout << (OPTIONS[key] ? "T" : "F");
  else
   fout << OPTIONS[key];
  fout << "\n";
 }
}
