#pragma once
#ifndef CATA_SRC_TEMP_CRAFTING_INVENTORY_H
#define CATA_SRC_TEMP_CRAFTING_INVENTORY_H

// IWYU pragma: no_include <memory>  // IWYU being silly
#include <cstddef>

#include "colony.h"
#include "item.h"
#include "visitable.h"

/**
 * A transient add-only list of item references and temporary pseudo-items, that implements `read_only_visitable`
 * and thus can answer crafting queries (see crafting.cpp and requirement_data::can_make_with_inventory).
 */
class temp_crafting_inventory : public read_only_visitable
{
    public:
        temp_crafting_inventory() = default;
        explicit temp_crafting_inventory( const read_only_visitable & );

        size_t size() const;

        void clear();

        /**
         * Adds item reference to this container.
         * @note container doesn't own the added reference, meaning added item should outlive this container.
         */
        void add_item_ref( item &item );

        /**
         * Adds all (top-level) items from the given visitable.
         * @note container doesn't own the added references, meaning added items should outlive this container.
         */
        void add_all_ref( const read_only_visitable &v );

        /**
        * Adds item copy to this container.
        * Container will own a copy of the given item.
        * @return reference to the added item within the container
        */
        item &add_item_copy( const item &item );

        // inherited from visitable
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const
        override;

    private:
        // list of all items in this container
        cata::colony<item *> items;
        // copies of "owned" items added by `add_item_copy`
        cata::colony<item> temp_owned_items;
};

#endif // CATA_SRC_TEMP_CRAFTING_INVENTORY_H
