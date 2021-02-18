#ifndef CATA_SRC_RANGED_H
#define CATA_SRC_RANGED_H

#include <iosfwd>
#include <vector>

#include "point.h"

class aim_activity_actor;
class avatar;
class gun_mode;
class item;
class map;
class player;
class spell;
class turret_data;
class vehicle;
struct vehicle_part;

namespace target_handler
{
// Trajectory to target. Empty if selection was aborted or player ran out of moves
using trajectory = std::vector<tripoint>;

/**
 * Firing ranged weapon. This mode allows spending moves on aiming.
 */
trajectory mode_fire( avatar &you, aim_activity_actor &activity );

/** Throwing item */
trajectory mode_throw( avatar &you, item &relevant, bool blind_throwing );

/** Reach attacking */
trajectory mode_reach( avatar &you, item &weapon );

/** Manually firing vehicle turret */
trajectory mode_turret_manual( avatar &you, turret_data &turret );

/** Selecting target for turrets (when using vehicle controls) */
trajectory mode_turrets( avatar &you, vehicle &veh, const std::vector<vehicle_part *> &turrets );

/** Casting a spell */
trajectory mode_spell( avatar &you, spell &casting, bool no_fail, bool no_mana );
} // namespace target_handler

int range_with_even_chance_of_good_hit( int dispersion );

/**
 * Common checks for gunmode (when firing a weapon / manually firing turret)
 * @param messages Used to store messages describing failed checks
 * @return True if all conditions are true
 */
bool gunmode_checks_common( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode );

/**
 * Various checks for gunmode when firing a weapon
 * @param messages Used to store messages describing failed checks
 * @return True if all conditions are true
 */
bool gunmode_checks_weapon( avatar &you, const map &m, std::vector<std::string> &messages,
                            const gun_mode &gmode );

int throw_cost( const player &c, const item &to_throw );

#endif // CATA_SRC_RANGED_H
