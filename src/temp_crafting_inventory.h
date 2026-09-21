#pragma once
#ifndef CATA_SRC_TEMP_CRAFTING_INVENTORY_H
#define CATA_SRC_TEMP_CRAFTING_INVENTORY_H

// IWYU pragma: no_include <memory>  // IWYU being silly
#include <climits>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "colony.h"
#include "coords_fwd.h"
#include "item.h"
#include "item_location.h"
#include "type_id.h"
#include "visitable.h"

class Character;
class map;
class map_cursor;
class vehicle_cursor;

/**
 * A transient add-only list of item references and temporary pseudo-items, that implements `read_only_visitable`
 * and thus can answer crafting queries (see crafting.cpp and requirement_data::can_make_with_inventory).
 */
class temp_crafting_inventory : public read_only_visitable
{
    public:
        temp_crafting_inventory() = default;
        temp_crafting_inventory( const temp_crafting_inventory &v );
        temp_crafting_inventory &operator=( const temp_crafting_inventory &v );

        size_t size() const;

        void clear();

        /**
         * Adds item reference to this container.
         * @note container doesn't own the added reference, meaning added item should outlive this container.
         */
        void add_item_ref( item &item );
        void add_item_loc( const item_location &loc );

        /**
         * Adds all (top-level) items from the given visitable.
         * @note container doesn't own the added references, meaning added items should outlive this container.
         * the other functions are so that the read_only_visitable inherited classes that have item_location can add item_location to the other colony
         */
        void add_all_ref( const read_only_visitable &v );
        void add_all_ref( const Character &guy );
        void add_all_ref( const map_cursor &cur );
        void add_all_ref( const vehicle_cursor &cur );

        /**
        * Adds item copy to this container.
        * Container will own a copy of the given item.
        * @return reference to the added item within the container
        */
        item &add_item_copy( const item &item );
        item &add_pseudo_item( const itype_id &id );
        item &add_pseudo_item( const item &item );

        // inherited from visitable. note: temp_owned_items are copied into items
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;
        VisitResponse visit_items_of_type( const itype_id &type,
                const std::function<VisitResponse( item *, item * )> &func ) const override;
        bool has_provider_quality( const quality_id &qual, int level, int qty,
                                   const Character *who,
                                   quality_count mode = quality_count::providers ) const override;

        // these functions are identical to inventory
        int count_item( const itype_id &item_type ) const;
        int charges_of( const itype_id &what, int limit = INT_MAX,
                        const std::function<bool( const item & )> &filter = return_true<item>,
                        const std::function<void( int )> &visitor = nullptr,
                        bool in_tools = false ) const override;
        int amount_of(
            const itype_id &what, bool pseudo = true, int limit = INT_MAX,
            const std::function<bool( const item & )> &filter = return_true<item> ) const override;

        void form_from_zone( map &m, std::unordered_set<tripoint_abs_ms> &zone_pts,
                             const Character *pl = nullptr );
        void form_from_map( const tripoint_bub_ms &origin, int range, const Character *pl = nullptr,
                            bool clear_path = true );
        void form_from_map( map *here, const tripoint_bub_ms &origin, int range,
                            const Character *pl = nullptr,
                            bool clear_path = true );
        void form_from_map( map &m, std::vector<tripoint_bub_ms> pts, const Character *pl );
        std::list<item> remove_items_with( const std::function<bool( const item & )> &filter,
                                           int count = INT_MAX );

        void dump( std::vector<item *> &dest ) const;

        // moved from inventory
        bool must_use_liq_container( const itype_id &id, int to_use ) const;
        // specifically used to for displaying non-empty liquid container color in crafting screen
        bool must_use_hallu_poison( const itype_id &id, int to_use ) const;
        void update_liq_container_count( const itype_id &id, int count );
        void replace_liq_container_count( const std::map<itype_id, int> &newmap, bool use_max = false );
    private:
        // list of all items in this container that don't know their parent
        cata::colony<item *> items;
        // list of all items in this crafting inventory that know their parent
        cata::colony<item_location> items_loc;
        // copies of "owned" items added by `add_item_copy`
        cata::colony<item *> item_copies;
        // this is only walked in visitable for tool qualities!
        cata::colony<item> temp_owned_items;

        using item_bin = std::unordered_map<itype_id, std::vector<const item *>>;
        mutable bool binned = false;
        mutable item_bin binned_items;
        mutable item_bin binned_tool_items;
        mutable std::map<std::pair<itype_id, bool>, int> unfiltered_amount;
        mutable std::map<std::pair<itype_id, bool>, int> unfiltered_charges;
        mutable std::map<std::tuple<quality_id, int, int, const Character *, quality_count>, bool>
        provider_quality_cache;

        void build_item_bins() const;
        bool can_cache() const;
        void invalidate_caches();

        // moved from inventory
        std::map<itype_id, int> max_empty_liq_cont;

        std::map<itype_id, item *> pseudo_items;
};

#endif // CATA_SRC_TEMP_CRAFTING_INVENTORY_H
