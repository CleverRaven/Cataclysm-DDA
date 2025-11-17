#pragma once
#ifndef CATA_SRC_ITEM_TRANSFORMATION_H
#define CATA_SRC_ITEM_TRANSFORMATION_H

#include <string>
#include <vector>

#include "calendar.h"
#include "type_id.h"

class item;
class Character;
class JsonObject;

/** Deals with how to transform an item, but not its trigger or the processing
* associated with the trigger (such as consumption of resources). This is a helper
* to deal with the functionality common to activated and timed transformations.
*/
struct item_transformation {
    /** type of the resulting item */
    itype_id target;

    /** or one of items from itemgroup */
    item_group_id target_group;

    /** if set transform item to container and place new item (of type @ref target) inside */
    itype_id container;

    /** if set choose this variant when transforming */
    std::string variant_type = "<any>";

    /** whether the transformed container is sealed */
    bool sealed = true;

    /** if zero or positive set remaining ammo of @ref target to this (after transformation) */
    int ammo_qty = -1;

    /** if this has values, set remaining ammo of @ref target to one of them chosen at random (after transformation) */
    std::vector<int> random_ammo_qty;

    /** if positive set transformed item active and start countdown for countdown_action*/
    time_duration target_timer = 0_seconds;

    /** if both this and ammo_qty are specified then set @ref target to this specific ammo */
    itype_id ammo_type;

    /** used to set the active property of the transformed @ref target */
    bool active = false;

    void deserialize( const JsonObject &jo );
    // Transforms the item.
    // dont_take_off blocks the removal of transformed "armor" when the caller relies
    // on the item remaing after the call (taking items off creates a copy intead of moving the original)
    void transform( Character *carrier, item &it, bool dont_take_off = false ) const;
};

#endif // CATA_SRC_ITEM_TRANSFORMATION_H
