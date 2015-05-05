#ifndef ACTIVE_ITEM_CACHE_H
#define ACTIVE_ITEM_CACHE_H

#include "enums.h"
#include "item.h"
#include <list>
#include <unordered_map>
#include <unordered_set>

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference
{
    point location;
    std::list<item>::iterator item_iterator;
    // Do not access this from outside this module, it is only used as an ID for active_item_set.
    item *item_id;
};

class active_item_cache
{
private:
    std::unordered_map<int, std::list<item_reference>> active_items;
    // Cache for fast lookup when we're iterating over the active items to verify the item is present.
    std::unordered_set<item *> active_item_set;

public:
    void remove( std::list<item>::iterator it, point location );
    void add( std::list<item>::iterator it, point location );
    bool has( std::list<item>::iterator it, point ) const;
    // Use this one if there's a chance that the item being referenced has been invalidated.
    bool has( item_reference const &itm ) const;
    bool empty() const;
    std::list<item_reference> get();
};

#endif
