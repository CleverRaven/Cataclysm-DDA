#pragma once
#ifndef CATA_SRC_PICKUP_H
#define CATA_SRC_PICKUP_H

#include <vector>

#include "cuboid_rectangle.h"
#include "point.h"

class Character;
class item;
class item_location;

namespace Pickup
{
/**
 * Returns `false` if the player was presented a prompt and decided to cancel the pickup.
 * `true` in other cases.
 */
bool do_pickup( std::vector<item_location> &targets, std::vector<int> &quantities,
                bool autopickup, bool &stash_successful );
bool query_thief();

enum from_where : int {
    from_cargo = 0,
    from_ground,
    prompt
};

/** Pick up items; 'g' or ',' or via examine() */
void autopickup( const tripoint &p );
/** Determines the cost of moving an item by a character. */
int cost_to_move_item( const Character &who, const item &it );

struct pickup_rect : inclusive_rectangle<point> {
    pickup_rect() = default;
    pickup_rect( const point &P_MIN, const point &P_MAX ) : inclusive_rectangle( P_MIN, P_MAX ) {}
    int cur_it;
    static std::vector<pickup_rect> list;
    static pickup_rect *find_by_coordinate( const point &p );
};

} // namespace Pickup
#endif // CATA_SRC_PICKUP_H
