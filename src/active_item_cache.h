#ifndef ACTIVE_ITEM_CACHE_H
#define ACTIVE_ITEM_CACHE_H

#include "enums.h"
#include "item.h"
#include <list>
#include <unordered_map>
#include <unordered_set>

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference {
    point location;
    std::list<item>::iterator item_iterator;
    // Do not access this from outside this module, it is only used as an ID for active_item_set.
    const unsigned item_id;
};

class active_item_cache
{
    private:
        std::unordered_map<int, std::list<item_reference>> active_items;
        // Cache for fast lookup when we're iterating over the active items to verify the item is present.
        std::unordered_set<unsigned> active_item_set;
        unsigned next_free_id = 0;

        /**
         * Helper function for remove and for the more dangerous version of has.
         *
         * @param found: This output will be undefined if not found, otherwise it will be the found element.
         * @ret: INT_MIN if not found, otherwise found is in active_items[ret]
        */
        int find( std::list<item_reference>::const_iterator &found,
                  const std::list<item>::iterator target, const point &target_loc ) const;

    public:
        void remove( const std::list<item>::iterator it, const point &location );
        void add( const std::list<item>::iterator it, const point &location );

        // WARNING: Do not call this version of has from the item processing code!
        bool has( const std::list<item>::iterator it, const point &p ) const;
        bool has( item_reference const &itm ) const;
        bool empty() const;
        std::list<item_reference> get();
};

#endif
