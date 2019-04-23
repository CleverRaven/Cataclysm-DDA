#pragma once
#ifndef PICKUP_H
#define PICKUP_H

#include <list>

class item;
class Character;
class map;
struct tripoint;

namespace Pickup
{
/**
 * Returns `false` if the player was presented a prompt and decided to cancel the pickup.
 * `true` in other cases.
 */
bool do_pickup( const tripoint &pickup_target_arg, bool from_vehicle,
                std::list<int> &indices, std::list<int> &quantities, bool autopickup );

enum from_where : int {
    from_cargo = 0,
    from_ground,
    prompt
};
/** Pick up items; ',' or via examine() */
void pick_up( const tripoint &p, int min, from_where get_items_from = prompt );
/** Determines the cost of moving an item by a character. */
int cost_to_move_item( const Character &who, const item &it );

/**
 * If character is handling a potentially spillable bucket, gracefully handle what
 * to do with the contents.
 *
 * Returns true if we handled the container, false if we chose to spill the
 * contents and the container still needs to be put somewhere.
 * @param c Character handling the spillable item
 * @param it item to handle
 * @param m map they are on
 */
bool handle_spillable_contents( Character &c, item &it, map &m );
}

#endif
