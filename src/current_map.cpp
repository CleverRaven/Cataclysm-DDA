#include "current_map.h"
#include "game.h"
#include "map.h"

map &current_map::get_map()
{
    return *current;
}

swap_map::swap_map( map &new_map )
{
    previous = g->current_map.current;
    g->current_map.set( &new_map );
    return;
}

swap_map::~swap_map()
{
    g->current_map.set( previous );
}

