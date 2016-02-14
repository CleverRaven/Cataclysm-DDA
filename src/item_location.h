#ifndef ITEM_LOCATION_H
#define ITEM_LOCATION_H

#include <memory>

class item;
class Character;
class map_cursor;
class vehicle_cursor;

/**
 * A class for easy removal of used items.
 * Ensures the item exists, but not that the character/vehicle does.
 * Should not be kept, but removed before the end of turn.
 */
class item_location
{
    public:
        item_location();
        item_location( const item_location & ) = delete;
        item_location &operator= ( const item_location & ) = delete;
        item_location( item_location && );
        item_location &operator=( item_location && );
        ~item_location();

        item_location( Character &ch, item *which );
        item_location( const map_cursor &mc, item *which );
        item_location( const vehicle_cursor &vc, item *which );

        /** Describes the item location
         *  @param ch if set description is relative to character location */
        std::string describe( const Character *ch = nullptr ) const;

        /** Move an item from the location to the character inventory
         *  @warning caller should restack inventory if item is to remain in it
         *  @warning all further operations using this class are invalid
         *  @return inventory position for the item */
        int obtain( Character &ch );

        /** Removes the selected item from the game
         *  @warning all further operations using this class are invalid */
        void remove_item();

        /** Gets the selected item or nullptr */
        item *get_item();
        const item *get_item() const;

    private:
        class impl;
        std::unique_ptr<impl> ptr;

        class item_is_null;
        class item_on_map;
        class item_on_person;
        class item_on_vehicle;
};

#endif
