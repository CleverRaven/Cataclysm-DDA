#pragma once
#ifndef CATA_SRC_PROJECTILE_H
#define CATA_SRC_PROJECTILE_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "compatibility.h"
#include "coordinates.h"
#include "damage.h"
#include "point.h"
#include "type_id.h"

class Creature;
class item;
struct explosion_data;

struct projectile {
        damage_instance impact;
        // how hard is it to dodge? essentially rolls to-hit,
        // bullets have arbitrarily high values but thrown objects have dodgeable values.
        int speed = 0;
        int range = 0;
        // Number of projectiles fired at a time, one except in cases like shotgun rounds.
        int count = 1;
        // True when count > 1 && distance > 1
        bool multishot = false;
        // The potential dispersion between different projectiles fired from one round.
        int shot_spread = 0;
        // Damage dealt by a single shot.
        damage_instance shot_impact;
        float critical_multiplier = 0.0f;

        std::set<ammo_effect_str_id> proj_effects;

        /**
         * Returns an item that should be dropped or an item for which is_null() is true
         *  when item to drop is unset.
         */
        const item &get_drop() const;
        /** Copies item `it` as a drop for this projectile. */
        void set_drop( const item &it );
        void set_drop( item &&it );
        void unset_drop();

        const explosion_data &get_custom_explosion() const;
        void set_custom_explosion( const explosion_data &ex );
        void unset_custom_explosion();

        // applies proj_effects to a damaged creature
        void apply_effects_damage( Creature &target, Creature *source,
                                   const dealt_damage_instance &dealt_dam,
                                   bool critical ) const;

        projectile();
        projectile( const projectile & );
        projectile( projectile && ) noexcept( set_is_noexcept );
        projectile &operator=( const projectile & );
        ~projectile();

    private:
        // Actual item used (to drop contents etc.).
        // Null in case of bullets (they aren't "made of cartridges").
        std::unique_ptr<item> drop;
        std::unique_ptr<explosion_data> custom_explosion;
};

struct dealt_projectile_attack {
    projectile proj; // What we used to deal the attack
    Creature *last_hit_critter; // The critter that stopped the projectile or null
    dealt_damage_instance dealt_dam; // If last_hit_critter isn't null, hit data is written here
    tripoint_bub_ms end_point; // Last hit tile (if last_hit_critter is null, drops should spawn here)
    double missed_by; // Accuracy of dealt attack
    bool headshot = false; // Headshot or not;
    bool shrapnel = false; // True if the projectile is generated from an explosive
    // Critters that hit by the projectile or null
    std::map<Creature *, std::pair<int, int>> targets_hit;
};

void apply_ammo_effects( Creature *source, const tripoint_bub_ms &p,
                         const std::set<ammo_effect_str_id> &effects, int dealt_damage );
int max_aoe_size( const std::set<ammo_effect_str_id> &tags );

void multi_projectile_hit_message( Creature *critter, int hit_count, int damage_taken,
                                   const std::string &projectile_name );

#endif // CATA_SRC_PROJECTILE_H
