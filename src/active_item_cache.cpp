#include "active_item_cache.h"

void active_item_cache::remove( std::list<item>::iterator it, point location )
{
    active_items.remove_if( [&] (const item_reference &active_item) {
            return location == active_item.location && active_item.item_iterator == it; } );
    active_item_set.erase(it);
}

void active_item_cache::add( std::list<item>::iterator it, point location )
{
    active_items.push_back( item_reference{ location, it } );
    active_item_set.insert( it );
}

bool active_item_cache::has( std::list<item>::iterator it, point ) const
{
    return active_item_set.count( it ) != 0;
}

bool active_item_cache::empty() const
{
    return active_items.empty();
}

const std::list<item_reference> &active_item_cache::get()
{
    return active_items;
}
