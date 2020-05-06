#pragma once
#ifndef CATA_SRC_ITEM_LOCATION_H
#define CATA_SRC_ITEM_LOCATION_H

#include <memory>
#include <string>

#include "map_selector.h"

struct tripoint;
class item;
class Character;
class vehicle_cursor;
class JsonIn;
class JsonOut;

/**
 * A lightweight handle to an item independent of it's location
 * Unlike a raw pointer can be (de-)serialized to/from JSON
 * Provides a generic interface of querying, obtaining and removing an item
 * Is invalidated by many operations (including copying of the item)
 */
class item_location
{
    public:
        enum class type : int {
            invalid = 0,
            character = 1,
            map = 2,
            vehicle = 3,
            container = 4
        };

        item_location();

        static const item_location nowhere;

        item_location( Character &ch, item *which );
        item_location( const map_cursor &mc, item *which );
        item_location( const vehicle_cursor &vc, item *which );
        item_location( const item_location &container, item *which );

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

        bool operator==( const item_location &rhs ) const;
        bool operator!=( const item_location &rhs ) const;

        explicit operator bool() const;

        item &operator*();
        const item &operator*() const;

        item *operator->();
        const item *operator->() const;

        /** Returns the type of location where the item is found */
        type where() const;

        /** Returns the position where the item is found */
        tripoint position() const;

        /** Describes the item location
         *  @param ch if set description is relative to character location */
        std::string describe( const Character *ch = nullptr ) const;

        /** Move an item from the location to the character inventory
         *  @param ch Character who's inventory gets the item
         *  @param qty if specified limits maximum obtained charges
         *  @warning caller should restack inventory if item is to remain in it
         *  @warning all further operations using this class are invalid
         *  @warning it is unsafe to call this within unsequenced operations (see #15542)
         *  @return item_location for the item */
        item_location obtain( Character &ch, int qty = -1 );

        /** Calculate (but do not deduct) number of moves required to obtain an item
         *  @see item_location::obtain */
        int obtain_cost( const Character &ch, int qty = -1 ) const;

        /** Removes the selected item from the game
         *  @warning all further operations using this class are invalid */
        void remove_item();

        /** Gets the selected item or nullptr */
        item *get_item();
        const item *get_item() const;

        void set_should_stack( bool should_stack ) const;

        /** returns the parent item, or an invalid location if it has no parent */
        item_location parent_item() const;

    private:
        class impl;

        std::shared_ptr<impl> ptr;
};

#endif // CATA_SRC_ITEM_LOCATION_H
