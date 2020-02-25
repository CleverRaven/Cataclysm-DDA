#pragma once
#ifndef ITEM_STACK_H
#define ITEM_STACK_H

#include <cstddef>
#include <vector>

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
    private:
        class item_stack_const_iterator : public std::vector<cata::colony<item>::iterator>::const_iterator
        {
            public:
                item_stack_const_iterator() : std::vector<cata::colony<item>::iterator>::const_iterator() {};

                item_stack_const_iterator( const std::vector<cata::colony<item>::iterator>::const_iterator &it ) :
                    std::vector<cata::colony<item>::iterator>::const_iterator( it ) {};

                item_stack_const_iterator( const std::vector<cata::colony<item>::iterator>::const_iterator &&it ) :
                    std::vector<cata::colony<item>::iterator>::const_iterator( it ) {};

                cata::colony<item>::const_iterator::reference operator*() const {
                    return *std::vector<cata::colony<item>::iterator>::const_iterator::operator*();
                }

                cata::colony<item>::const_iterator::pointer operator->() const {
                    return std::vector<cata::colony<item>::iterator>::const_iterator::operator->()->operator->();
                }
        };

        class item_stack_iterator : public std::vector<cata::colony<item>::iterator>::iterator
        {
            public:
                item_stack_iterator() : std::vector<cata::colony<item>::iterator>::iterator() {};

                item_stack_iterator( const std::vector<cata::colony<item>::iterator>::iterator &it ) :
                    std::vector<cata::colony<item>::iterator>::iterator( it ) {};

                item_stack_iterator( std::vector<cata::colony<item>::iterator>::iterator &&it ) :
                    std::vector<cata::colony<item>::iterator>::iterator( it ) {};

                operator item_stack_const_iterator() const {
                    return item_stack_const_iterator( std::vector<cata::colony<item>::iterator>::const_iterator(
                                                          *this ) );
                }

                cata::colony<item>::iterator::reference operator*() const {
                    return *std::vector<cata::colony<item>::iterator>::iterator::operator*();
                }

                cata::colony<item>::iterator::pointer operator->() const {
                    return std::vector<cata::colony<item>::iterator>::iterator::operator->()->operator->();
                }
        };

    protected:
        std::vector<cata::colony<item>::iterator> *items;

    public:
        using iterator = item_stack_iterator;
        using const_iterator = item_stack_const_iterator;
        using reverse_iterator = std::reverse_iterator<item_stack_iterator>;
        using const_reverse_iterator = std::reverse_iterator<item_stack_const_iterator>;

        item_stack( std::vector<cata::colony<item>::iterator> *items ) : items( items ) { }

        size_t size() const;
        bool empty() const;
        virtual void insert( const item &newitem ) = 0;
        virtual iterator erase( const_iterator it ) = 0;
        virtual void clear();
        // Will cause a debugmsg if there is not exactly one item at the location
        item &only_item();

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

#endif
