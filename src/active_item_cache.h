#pragma once
#ifndef ACTIVE_ITEM_CACHE_H
#define ACTIVE_ITEM_CACHE_H

#include <list>
#include <unordered_map>

#include "enums.h"

class item;

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference {
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
        // Key is item_id, value is whether it was returned in the last call to get
        std::unordered_map<item *, bool> active_item_set;

    public:
        void remove( std::list<item>::iterator it, point location );
        void add( std::list<item>::iterator it, point location );
        bool has( std::list<item>::iterator it, point ) const;
        // Use this one if there's a chance that the item being referenced has been invalidated.
        bool has( const item_reference &itm ) const;
        bool empty() const;
        std::list<item_reference> get();

        /** Subtract delta from every item_reference's location */
        void subtract_locations( const point &delta );
};

#endif
