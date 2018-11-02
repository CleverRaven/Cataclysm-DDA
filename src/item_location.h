#pragma once
#ifndef ITEM_LOCATION_H
#define ITEM_LOCATION_H

#include <memory>
#include <list>

struct tripoint;
class item;
class Character;
class map_cursor;
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
            vehicle = 3
        };

        item_location();
        item_location( const item_location & ) = delete;
        item_location &operator= ( const item_location & ) = delete;
        item_location( item_location && );
        item_location &operator=( item_location && );
        ~item_location();

        static const item_location nowhere;

        item_location( Character &ch, item *which );
        item_location( Character &ch, std::list<item> *which );
        item_location( const map_cursor &mc, item *which );
        item_location( const map_cursor &mc, std::list<item> *which );
        item_location( const vehicle_cursor &vc, item *which );
        item_location( const vehicle_cursor &vc, std::list<item> *which );

        void serialize( JsonOut &js ) const;
        void deserialize( JsonIn &js );

        long charges_in_stack( unsigned int countOnly ) const;

        bool operator==( const item_location &rhs ) const;
        bool operator!=( const item_location &rhs ) const;

        operator bool() const;

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
         *  @return inventory position for the item */
        int obtain( Character &ch, long qty = -1 );

        /** Calculate (but do not deduct) number of moves required to obtain an item
         *  @see item_location::obtain */
        int obtain_cost( const Character &ch, long qty = -1 ) const;

        /** Removes the selected item from the game
         *  @warning all further operations using this class are invalid */
        void remove_item();

        /** Gets the selected item or nullptr */
        item *get_item();
        const item *get_item() const;

        /**
         * Clones this instance
         * @warning usage should be restricted to implementing custom copy-constructors
         */
        item_location clone() const;

    private:
        class impl;
        std::shared_ptr<impl> ptr;

        /* Not implemented on purpose. This triggers a compiler / linker
         * error when used in any implicit conversion. It prevents the
         * implicit conversion to int. */
        template<typename T>
        operator T() const;
};

#endif
