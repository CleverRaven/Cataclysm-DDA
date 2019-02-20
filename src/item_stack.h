#pragma once
#ifndef ITEM_STACK_H
#define ITEM_STACK_H

#include <cstddef>
#include <list>

#include "units.h"

class item;

// A wrapper class to bundle up the references needed for a caller to safely manipulate
// items and obtain information about items at a particular map x/y location.
// Note this does not expose the container itself,
// which means you cannot call e.g. vector::erase() directly.

// Pure virtual base class for a collection of items with origin information.
// Only a subset of the functionality is callable without casting to the specific
// subclass, e.g. not begin()/end() or range loops.
class item_stack
{
    protected:
        std::list<item> *mystack;

    public:
        item_stack( std::list<item> *mystack ) : mystack( mystack ) { }

        size_t size() const;
        bool empty() const;
        virtual std::list<item>::iterator erase( std::list<item>::iterator it ) = 0;
        virtual void push_back( const item &newitem ) = 0;
        virtual void insert_at( std::list<item>::iterator, const item &newitem ) = 0;
        virtual void clear();
        item &front();
        item &operator[]( size_t index );

        std::list<item>::iterator begin();
        std::list<item>::iterator end();
        std::list<item>::const_iterator begin() const;
        std::list<item>::const_iterator end() const;
        std::list<item>::reverse_iterator rbegin();
        std::list<item>::reverse_iterator rend();
        std::list<item>::const_reverse_iterator rbegin() const;
        std::list<item>::const_reverse_iterator rend() const;

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
        long amount_can_fit( const item &it ) const;
        /** Return the item (or nullptr) that stacks with the argument */
        item *stacks_with( const item &it );
        const item *stacks_with( const item &it ) const;
};

#endif
