#pragma once
#ifndef ACTIVE_ITEM_CACHE_H
#define ACTIVE_ITEM_CACHE_H

#include <list>
#include <unordered_map>
#include <vector>

#include "safe_reference.h"
#include "point.h"

class item;

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference {
    point location;
    safe_reference<item> item_ref;
};

enum class special_item_type : char {
    none,
    corpse,
    explosive
};

class active_item_cache
{
    private:
        std::unordered_map<int, std::list<item_reference>> active_items;
        std::unordered_map<special_item_type, std::list<item_reference>> special_items;

    public:
        /**
         * Removes the item if it is in the cache. Does nothing if the item is not in the cache.
         * Relies on the fact that item::processing_speed() is a constant.
         * Also removes any items that have been destroyed in the list containing it
         */
        void remove( const item *it );

        /**
         * Adds the reference to the cache. Does nothing if the reference is already in the cache.
         * Relies on the fact that item::processing_speed() is a constant.
         */
        void add( item &it, point location );

        /**
         * Returns true if the cache is empty
         */
        bool empty() const;

        /**
         * Returns a vector of all cached active item references.
         * Broken references are removed from the cache.
         */
        std::vector<item_reference> get();

        /**
         * Returns the first size() / processing_speed() elements of each list, rounded up.
         * Items returned are rotated to the back of their respective lists, otherwise only the
         * first n items will ever be processed.
         * Broken references encountered when collecting the items to be processed are removed from
         * the cache.
         * Relies on the fact that item::processing_speed() is a constant.
         */
        std::vector<item_reference> get_for_processing();

        /**
         * Returns the currently tracked list of special active items.
         */
        std::vector<item_reference> get_special( special_item_type type );
        /** Subtract delta from every item_reference's location */
        void subtract_locations( const point &delta );
        void rotate_locations( int turns, const point &dim );
};

#endif
