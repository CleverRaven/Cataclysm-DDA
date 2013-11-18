#include "overmapbuffer.h"

overmapbuffer overmap_buffer;

overmapbuffer::overmapbuffer()
{
}

overmap &overmapbuffer::get( game *g, const int x, const int y )
{
    // Check list first
    for(std::list<overmap>::iterator candidate = overmap_list.begin();
        candidate != overmap_list.end(); ++candidate)
    {
        if(candidate->pos().x == x && candidate->pos().y == y)
        {
            return *candidate;
        }
    }
    // If we don't have one, make it, stash it in the list, and return it.
    overmap new_overmap(g, x, y);
    overmap_list.push_back( new_overmap );
    return overmap_list.back();
}

void overmapbuffer::save()
{
    for(std::list<overmap>::iterator current_map = overmap_list.begin();
        current_map != overmap_list.end(); ++current_map)
    {
        current_map->save();
    }
}

void overmapbuffer::clear()
{
    overmap_list.clear();
}
