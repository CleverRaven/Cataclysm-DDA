#pragma once
#ifndef BALLISTICS_H
#define BALLISTICS_H

class Creature;
class dispersion_sources;
class vehicle;
struct dealt_projectile_attack;
struct projectile;
struct tripoint;

/** Aim result for a single projectile attack */
struct projectile_attack_aim {
    double missed_by;       ///< Hit quality, where 0.0 is a perfect hit and 1.0 is a miss
    double missed_by_tiles; ///< Number of tiles the attack missed by
    double dispersion;      ///< Dispersion of this particular shot in arcminutes
};

/**
 * Evaluates dispersion sources, range, and target to determine attack trajectory.
 **/
projectile_attack_aim projectile_attack_roll( const dispersion_sources &dispersion, double range,
        double target_size );

/**
 *  Fires a projectile at the target point from the source point with total_dispersion
 *  dispersion.
 *  Returns the rolled dispersion of the shot and the actually hit point.
 */
dealt_projectile_attack projectile_attack( const projectile &proj, const tripoint &source,
        const tripoint &target, const dispersion_sources &dispersion,
        Creature *origin = nullptr, const vehicle *in_veh = nullptr );

#endif
