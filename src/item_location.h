#ifndef ITEM_LOCATION_H
#define ITEM_LOCATION_H

#include <memory>

struct point;
struct tripoint;
class item;
class Character;
class vehicle;

/**
 * A class for easy removal of used items.
 * Ensures the item exists, but not that the character/vehicle does.
 * Should not be kept, but removed before the end of turn.
 */
class item_location
{
private:
    class impl;
    class item_is_null;
    class item_on_map;
    class item_on_person;
    class item_on_vehicle;
    std::unique_ptr<impl> ptr;
    item_location( impl* );
public:
    item_location( item_location&& );
    ~item_location();
    /** Factory functions for readability */
    /*@{*/
    static item_location nowhere();
    static item_location on_map( const tripoint &p, const item *which );
    static item_location on_character( Character &ch, const item *which );
    static item_location on_vehicle( vehicle &v, const point &where, const item *which );
    /*@}*/

    /** Removes the selected item from the game */
    void remove_item();
    /** Gets the selected item or nullptr */
    item *get_item();
    /** Gets the position of item in character's inventory or INT_MIN */
    int get_inventory_position();
};

#endif
