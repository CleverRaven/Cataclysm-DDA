#pragma once
#ifndef CATA_SRC_RANGED_H
#define CATA_SRC_RANGED_H

#include <iosfwd>
#include <vector>

#include "creature.h"
#include "point.h"

class aim_activity_actor;
class avatar;
class Character;
class gun_mode;
class item;
class item_location;
class map;
class spell;
class turret_data;
class vehicle;
struct vehicle_part;

// Recoil change less or equal to this value (in MoA) stops further aiming
constexpr double MIN_RECOIL_IMPROVEMENT = 0.01;

namespace target_handler
{
// Trajectory to target. Empty if selection was aborted or player ran out of moves
using trajectory = std::vector<tripoint>;

/** Generic target select without fire something */
trajectory mode_select_only( avatar &you, int range );

/**
 * Firing ranged weapon. This mode allows spending moves on aiming.
 */
trajectory mode_fire( avatar &you, aim_activity_actor &activity );

/** Throwing item */
trajectory mode_throw( avatar &you, item &relevant, bool blind_throwing );

/** Reach attacking */
trajectory mode_reach( avatar &you, item_location weapon );

/** Manually firing vehicle turret */
trajectory mode_turret_manual( avatar &you, turret_data &turret );

/** Selecting target for turrets (when using vehicle controls) */
trajectory mode_turrets( avatar &you, vehicle &veh, const std::vector<vehicle_part *> &turrets );

/** Casting a spell */
trajectory mode_spell( avatar &you, spell &casting, bool no_fail, bool no_mana );
} // namespace target_handler

void practice_archery_proficiency( Character &p, const item &relevant );

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

int throw_cost( const Character &c, const item &to_throw );

// check for steadiness for a given pos
double calc_steadiness( const Character &you, const item &weapon, const tripoint &pos,
                        double predicted_recoil );

double calculate_aim_cap( const Character &you, const tripoint &target );

double occupied_tile_fraction( creature_size target_size );

struct Target_attributes {
    int range = 1;
    double size = 0.5;
    double size_in_moa = 10800.0;
    float light = 0.0f;
    bool visible = true;
    explicit Target_attributes() = default;
    explicit Target_attributes( tripoint src, tripoint target );
    explicit Target_attributes( int rng, double size, float l, bool can_see );
};

#endif // CATA_SRC_RANGED_H
