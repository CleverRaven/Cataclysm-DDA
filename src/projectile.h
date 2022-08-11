#pragma once
#ifndef CATA_SRC_PROJECTILE_H
#define CATA_SRC_PROJECTILE_H

#include <iosfwd>
#include <memory>
#include <set>

#include "compatibility.h"
#include "damage.h"
#include "point.h"

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
        // The potential dispersion between different projectiles fired from one round.
        int shot_spread = 0;
        // Damage dealt by a single shot.
        damage_instance shot_impact;
        float critical_multiplier = 0.0f;

        std::set<std::string> proj_effects;

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
        // pplies proj_effects to a creature that was hit but not damaged
        void apply_effects_nodamage( Creature &target, Creature *source ) const;

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
    Creature *hit_critter; // The critter that stopped the projectile or null
    dealt_damage_instance dealt_dam; // If hit_critter isn't null, hit data is written here
    tripoint end_point; // Last hit tile (is hit_critter is null, drops should spawn here)
    double missed_by; // Accuracy of dealt attack
};

void apply_ammo_effects( const tripoint &p, const std::set<std::string> &effects );
int max_aoe_size( const std::set<std::string> &tags );

void multi_projectile_hit_message( Creature *critter, int hit_count, int damage_taken,
                                   const std::string &projectile_name );

#endif // CATA_SRC_PROJECTILE_H
