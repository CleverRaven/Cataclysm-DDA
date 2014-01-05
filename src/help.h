#ifndef _HELP_H_
#define _HELP_H_

#include "json.h"
#include <string>

void display_help();

void load_hint(JsonObject &jsobj);
std::string get_hint(); // return a random hint about the game

#endif // _HELP_H_
