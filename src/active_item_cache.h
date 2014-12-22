#ifndef ACTIVE_ITEM_CACHE_H
#define ACTIVE_ITEM_CACHE_H

#include <list>
#include <unordered_set>
#include "enums.h"
#include "item.h"

// Provides hashing operator for item list iterator.
struct list_iterator_hash
{
    size_t operator()(const std::list<item>::iterator &i) const {
        return (size_t)&*i;
    }
};

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference
{
    point location;
    std::list<item>::iterator item_iterator;
};

class active_item_cache
{
private:
    std::list<item_reference> active_items;
    // Cache for fast lookup when we're iterating over the active items to verify the item is present.
    std::unordered_set<std::list<item>::iterator, list_iterator_hash> active_item_set;

public:
    void remove( std::list<item>::iterator it, point location );
    void add( std::list<item>::iterator it, point location );
    bool has( std::list<item>::iterator it, point ) const;
    bool empty() const;
    const std::list<item_reference> &get();
};

#endif
