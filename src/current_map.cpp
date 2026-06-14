#include <memory>

#include "current_map.h"
#include "game.h"

map &current_map::get_map()
{
    return *current;
}

swap_map::swap_map( map &new_map )
{
    previous = g->current_map.current;
    g->current_map.set( &new_map );
}

swap_map::~swap_map()
{
    g->current_map.set( previous );
}

