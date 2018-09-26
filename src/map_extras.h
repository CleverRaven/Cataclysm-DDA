#pragma once
#ifndef MAP_EXTRAS_H
#define MAP_EXTRAS_H

#include "mapgen.h"

namespace MapExtras
{
map_special_pointer get_function( const std::string &name );
int generate_special_size(const std::string& name);
tripoint resize_special(const tripoint& tripoint, const std::string& cs);
};

#endif
