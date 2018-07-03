#pragma once
#ifndef PICKUP_H
#define PICKUP_H

#include "enums.h"

#include <list>

class vehicle;
class item;
class Character;

namespace Pickup
{
/**
 * Returns `false` if the player was presented a prompt and decided to cancel the pickup.
 * `true` in other cases.
 */
bool do_pickup( const tripoint &pickup_target, bool from_vehicle,
                std::list<int> &indices, std::list<int> &quantities, bool autopickup );

/** Pick up items; ',' or via examine() */
void pick_up( const tripoint &p, int min );
/** Determines the cost of moving an item by a character. */
int cost_to_move_item( const Character &who, const item &it );
};

#endif
