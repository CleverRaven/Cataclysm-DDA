#pragma once
#ifndef CATA_SRC_EXPLOSION_H
#define CATA_SRC_EXPLOSION_H

#include <map>
#include <string>
#include "optional.h"
#include "projectile.h"

using itype_id = std::string;

struct tripoint;
class JsonObject;
class nc_color;

struct explosion_data {
    int damage              = 0;
    float radius            = 0;
    bool fire               = false;
    cata::optional<projectile> fragment;

    /** Returns the range at which blast damage is 0 and shrapnel is out of range. */
    int safe_range() const;
    explicit operator bool() const;
};

// handles explosion related functions
namespace explosion_handler
{
/**
 * Legacy explosion function.
 * Updated values are calculated from distance factor.
 * If power > 0, blast is produced.
 * If casing mass > 0, shrapnel is produced.
 */
void explosion(
    const tripoint &p, float power, float factor = 0.8f,
    bool fire = false, int legacy_casing_mass = 0, float legacy_frag_mass = 0.05
);

void explosion( const tripoint &p, const explosion_data &ex );

constexpr float power_to_dmg_mult = 2.0f / 15.0f;

/** Triggers a flashbang explosion at p. */
void flashbang( const tripoint &p, bool player_immune = false );
/** Triggers a resonance cascade at p. */
void resonance_cascade( const tripoint &p );
/** Triggers a scrambler blast at p. */
void scrambler_blast( const tripoint &p );
/** Triggers an EMP blast at p. */
void emp_blast( const tripoint &p );
// shockwave applies knockback to all targets within radius of p
// parameters force, stun, and dam_mult are passed to knockback()
// ignore_player determines if player is affected, useful for bionic, etc.
void shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult,
                bool ignore_player );

void draw_explosion( const tripoint &p, int radius, const nc_color &col );
void draw_custom_explosion( const tripoint &p, const std::map<tripoint, nc_color> &area );

projectile shrapnel_from_legacy( int power, float blast_radius );
float blast_radius_from_legacy( int power, float distance_factor );
} // namespace explosion_handler

explosion_data load_explosion_data( const JsonObject &jo );

#endif // CATA_SRC_EXPLOSION_H
