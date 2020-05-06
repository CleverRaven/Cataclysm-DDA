#pragma once
#ifndef CATA_SRC_PROJECTILE_H
#define CATA_SRC_PROJECTILE_H

#include <memory>
#include <set>
#include <string>

#include "damage.h"
#include "point.h"

class Creature;
struct explosion_data;
class item;

struct projectile {
        damage_instance impact;
        // how hard is it to dodge? essentially rolls to-hit,
        // bullets have arbitrarily high values but thrown objects have dodgeable values.
        int speed;
        int range;
        float critical_multiplier;

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

        projectile();
        projectile( const projectile & );
        projectile( projectile && );
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

#endif // CATA_SRC_PROJECTILE_H
