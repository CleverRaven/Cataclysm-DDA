#ifndef CATA_SRC_RANGED_H
#define CATA_SRC_RANGED_H

#include <vector>

#include "type_id.h"

class aim_activity_actor;
class item;
class player;
class spell;
class turret_data;
class vehicle;
struct itype;
struct tripoint;
struct vehicle_part;

namespace target_handler
{
// Trajectory to target. Empty if selection was aborted or player ran out of moves
using trajectory = std::vector<tripoint>;

/**
 * Firing ranged weapon. This mode allows spending moves on aiming.
 */
trajectory mode_fire( player &pc, aim_activity_actor &activity );

/** Throwing item */
trajectory mode_throw( player &pc, item &relevant, bool blind_throwing );

/** Reach attacking */
trajectory mode_reach( player &pc, item &weapon );

/** Manually firing vehicle turret */
trajectory mode_turret_manual( player &pc, turret_data &turret );

/** Selecting target for turrets (when using vehicle controls) */
trajectory mode_turrets( player &pc, vehicle &veh, const std::vector<vehicle_part *> &turrets );

/** Casting a spell */
trajectory mode_spell( player &pc, spell &casting, bool no_fail, bool no_mana );
trajectory mode_spell( player &pc, spell_id sp, bool no_fail, bool no_mana );
} // namespace target_handler

int range_with_even_chance_of_good_hit( int dispersion );

#endif // CATA_SRC_RANGED_H
