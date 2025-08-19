#pragma once
#ifndef CATA_SRC_ACTIVE_ITEM_CACHE_H
#define CATA_SRC_ACTIVE_ITEM_CACHE_H

#include <cstddef>
#include <list>
#include <unordered_map>
#include <vector>

#include "coordinates.h"
#include "item_pocket.h"
#include "point.h"
#include "safe_reference.h"

class item;

// A struct used to uniquely identify an item within a submap or vehicle.
struct item_reference {
    point_rel_ms location;
    safe_reference<item> item_ref;
    // parent invalidating would also invalidate item_ref so it's safe to use a raw pointers here
    item *parent = nullptr;
    std::vector<item_pocket const *> pocket_chain;

    float spoil_multiplier() const;
    bool has_watertight_container() const;
};

enum class special_item_type : int {
    none,
    corpse,
    explosive
};

template<typename T>
constexpr bool operator==( safe_reference<T> const &lhs, safe_reference<T> const &rhs )
{
    // To ensure they hasn't expired
    return !!lhs && !!rhs && lhs.get() == rhs.get();
}

namespace std
{
template <>
struct hash<special_item_type> {
    std::size_t operator()( const special_item_type &k ) const noexcept {
        return static_cast<size_t>( k );
    }
};
} // namespace std

class active_item_cache
{
    private:
        std::unordered_map<int, std::list<item_reference>> active_items;
        std::unordered_map<special_item_type, std::list<item_reference>> special_items;
        std::unordered_map<int, std::unordered_map<item *, safe_reference<item>>> active_items_index;
    public:
        /**
         * Adds the reference to the cache. Does nothing if the reference is already in the cache.
         * Relies on the fact that item::processing_speed() is a constant.
         * These two operations are really the same, but tailored to their usages.
         * The submap coordinates are for submaps, and the relative ones are for vehicles.
         */
        bool add( item &it, point_sm_ms location, item *parent = nullptr,
                  std::vector<item_pocket const *> const &pocket_chain = {} );
        bool add( item &it, point_rel_ms location, item *parent = nullptr,
                  std::vector<item_pocket const *> const &pocket_chain = {} );

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
        void subtract_locations( const point_rel_ms &delta );
        void rotate_locations( int turns, const point_rel_ms &dim );
        void mirror( const point_rel_ms &dim, bool horizontally );
};

#endif // CATA_SRC_ACTIVE_ITEM_CACHE_H
