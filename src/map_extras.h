#pragma once
#ifndef MAP_EXTRAS_H
#define MAP_EXTRAS_H

#include <string>

#include "mapgen.h"

namespace MapExtras
{
typedef std::unordered_map<std::string, map_special_pointer> FunctionMap;

map_special_pointer get_function( const std::string &name );
FunctionMap all_functions();
}

#endif
