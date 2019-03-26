#pragma once
#ifndef VEH_UTILS_H
#define VEH_UTILS_H

#include "string_id.h"

class vehicle;
class Character;
class vpart_info;
struct vehicle_part;

class Skill;
using skill_id = string_id<Skill>;

namespace veh_utils
{
/** Calculates xp for interacting with given part. */
int calc_xp_gain( const vpart_info &vp, const skill_id &sk );
int calc_xp_gain( const vpart_info &vp, const skill_id &sk, const Character &who );
/**
 * Returns a part on a given vehicle that a given character can repair.
 * Prefers the most damaged parts that don't need replacements.
 * If no such part exists, returns a null part.
 */
vehicle_part &most_repairable_part( vehicle &veh, const Character &who_arg,
                                    bool only_repairable = false );
/**
 * Repairs a given part on a given vehicle by given character.
 * Awards xp and consumes components.
 */
bool repair_part( vehicle &veh, vehicle_part &part, Character &who );
}

#endif
