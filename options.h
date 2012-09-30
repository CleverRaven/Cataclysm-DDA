#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>

enum option_key {
OPT_NULL = 0,
OPT_FORCE_YN, // Y/N versus y/n
OPT_USE_CELSIUS, // Display temp as C not F
OPT_USE_METRIC_SYS, // Display speed as Km/h not mph
OPT_NO_CBLINK, // No bright backgrounds
OPT_24_HOUR, // 24 hour time
OPT_SNAP_TO_TARGET, // game::firing snaps to target
OPT_SAFEMODE, // Safemode on by default?
OPT_AUTOSAFEMODE, // Autosafemode on by default?
OPT_AUTOSAVE, // Automatically save the game on intervals.
NUM_OPTION_KEYS
};

struct option_table
{
 double options[NUM_OPTION_KEYS];

 option_table() { for (int i = 0; i < NUM_OPTION_KEYS; i++) options[i] = 0; };

 double& operator[] (option_key i) { return options[i]; };
 double& operator[] (int i) { return options[i]; };
};

extern option_table OPTIONS;

void load_options();
void save_options();
std::string option_string(option_key key);
std::string option_name(option_key key);

#endif
