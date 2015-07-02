#ifndef ITEM_LOCATION_H
#define ITEM_LOCATION_H

#include "enums.h"

class item;
class Character;
class vehicle;

/**
 * A class for easy removal of used items.
 * Ensures the item exists, but not that the character/vehicle does.
 * Should not be kept, but removed before the end of turn.
 */
class item_location {
protected:
    const item *what;
public:
    /** Removes the selected item from the game */
    virtual void remove_item() = 0;
    /** Gets the selected item or nullptr */
    virtual item *get_item() = 0;
    /** Gets the position of item in character's inventory or INT_MIN */
    virtual int get_inventory_position();
};

class item_is_null : public item_location {
public:
    item_is_null();

    void remove_item() override;
    item *get_item() override;
};

class item_on_map : public item_location {
private:
    tripoint location;
public:
    item_on_map( const tripoint &p, const item *which );

    void remove_item() override;
    item *get_item() override;
};

class item_on_person : public item_location {
private:
    Character *who;
public:
    item_on_person( Character &ch, const item *which );
    int get_inventory_position() override;

    void remove_item() override;
    item *get_item() override;
};

class item_on_vehicle : public item_location {
private:
    vehicle *veh;
    point local_coords;
public:
    item_on_vehicle( vehicle &v, const point &where, const item *which );

    void remove_item() override;
    item *get_item() override;
};

#endif
