#pragma once
#ifndef CATA_SRC_ITEM_STACK_H
#define CATA_SRC_ITEM_STACK_H

#include <cstddef>

#include "colony.h"
#include "item.h" // IWYU pragma: keep
#include "units.h"

// A wrapper class to bundle up the references needed for a caller to safely manipulate
// items and obtain information about items at a particular map x/y location.
// Note this does not expose the container itself,
// which means you cannot call e.g. cata::colony::erase() directly.

// Pure virtual base class for a collection of items with origin information.
// Only a subset of the functionality is callable without casting to the specific
// subclass, e.g. not begin()/end() or range loops.
class item_stack
{
    protected:
        cata::colony<item> *items;

    public:
        using iterator = cata::colony<item>::iterator;
        using const_iterator = cata::colony<item>::const_iterator;
        using reverse_iterator = cata::colony<item>::reverse_iterator;
        using const_reverse_iterator = cata::colony<item>::const_reverse_iterator;

        item_stack( cata::colony<item> *items ) : items( items ) { }
        virtual ~item_stack() = default;

        size_t size() const;
        bool empty() const;
        virtual void insert( const item &newitem ) = 0;
        virtual iterator erase( const_iterator it ) = 0;
        virtual void clear();
        // Will cause a debugmsg if there is not exactly one item at the location
        item &only_item();

        // While iterators to colonies are stable, indexes are not.
        // These functions should only be used for serialization/deserialization
        iterator get_iterator_from_pointer( item *it );
        iterator get_iterator_from_index( size_t idx );
        size_t get_index_from_iterator( const const_iterator &it );

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;

        /** Maximum number of items allowed here */
        virtual int count_limit() const = 0;
        /** Maximum volume allowed here */
        virtual units::volume max_volume() const = 0;
        /** Total volume of the items here */
        units::volume stored_volume() const;
        units::volume free_volume() const;
        /**
         * Returns how many of the specified item (or how many charges if it's counted by charges)
         * could be added without violating either the volume or itemcount limits.
         *
         * @returns Value of zero or greater for all items. For items counted by charges, it is always at
         * most it.charges.
         */
        int amount_can_fit( const item &it ) const;
        /** Return the item (or nullptr) that stacks with the argument */
        item *stacks_with( const item &it );
        const item *stacks_with( const item &it ) const;
};

#endif // CATA_SRC_ITEM_STACK_H
