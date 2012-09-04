#include "options.h"
#include "output.h"

#include <fstream>
#include <string>

option_table OPTIONS;

option_key lookup_option_key(std::string id);
bool option_is_bool(option_key id);
void create_default_options();
std::string options_header();

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
    double val;
    fin >> val;
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
 if (id == "autosafemode")
  return OPT_AUTOSAFEMODE;
 if (id == "autosave")
  return OPT_AUTOSAVE;

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
  case OPT_AUTOSAFEMODE:	return "autosafemode";
  case OPT_AUTOSAVE:    	return "autosave";
  default:			return "unknown_option";
 }
 return "unknown_option";
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
  case OPT_AUTOSAFEMODE:	return "Auto-Safemode on by default";
  case OPT_AUTOSAVE:    	return "Periodically Autosave";
  default:			return "Unknown Option (BUG)";
 }
 return "Big ol Bug";
}

bool option_is_bool(option_key id)
{
 switch (id) {
  default:
   return true;
 }
 return true;
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
# If true, use military time, not AM/PM\n\
24_hour F\n\
# If true, automatically follow the crosshair when firing/throwing\n\
snap_to_target F\n\
# If true, safemode will be on after starting a new game or loading\n\
safemode T\n\
# If true, auto-safemode will be on after starting a new game or loading\n\
autosafemode F\n\
# If true, game will periodically save the map\n\
autosave F\n\
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
