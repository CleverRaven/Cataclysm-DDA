#ifndef _IUSE_SOFTWARE_H_
#define _IUSE_SOFTWARE_H_

#include <string>
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "catacharset.h"
#include "crafting.h"
#include "options.h"
#include "debug.h"
#include "iuse.h"
bool play_videogame(std::string function_name, std::map<std::string, std::string> & game_data, int & score);

#endif
