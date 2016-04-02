#ifndef HELP_H
#define HELP_H

#include "json.h"
#include <string>

void display_help();

void load_hint( JsonObject &jsobj );
void clear_hints();
std::string get_hint(); // return a random hint about the game

#endif
