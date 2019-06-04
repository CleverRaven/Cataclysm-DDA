#pragma once
#ifndef MAP_EXTRAS_H
#define MAP_EXTRAS_H

#include <string>

#include "mapgen.h"

namespace MapExtras
{
using FunctionMap = std::unordered_map<std::string, map_extra_pointer>;

map_extra_pointer get_function( const std::string &name );
FunctionMap all_functions();
}

#endif
