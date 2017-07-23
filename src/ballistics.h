#pragma once
#ifndef BALLISTICS_H
#define BALLISTICS_H

class Creature;
class dispersion_sources;
class vehicle;
struct dealt_projectile_attack;
struct projectile;
struct tripoint;

/**
 *  Fires a projectile at the target point from the source point with total_dispersion
 *  dispersion.
 *  Returns the rolled dispersion of the shot and the actually hit point.
 */
dealt_projectile_attack projectile_attack( const projectile &proj, const tripoint &source,
                                           const tripoint &target, dispersion_sources dispersion,
                                           Creature *origin = nullptr, const vehicle *in_veh = nullptr );

#endif
