#ifndef MATTACK_ACTORS_H
#define MATTACK_ACTORS_H

#include "mtype.h"
#include <tuple>
#include <vector>
#include <map>

class JsonObject;
class monster;

class leap_actor : public mattack_actor
{
    public:
        float max_range;
        // Jump has to be at least this tiles long
        float min_range;
        // Don't leap without a hostile target creature
        bool allow_no_target;
        int move_cost;
        // Range below which we don't consider jumping at all
        float min_consider_range;
        // Don't jump if distance to target is more than this
        float max_consider_range;

        leap_actor() { }
        ~leap_actor() { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        virtual mattack_actor *clone() const override;
};

class bite_actor : public mattack_actor
{
    public:
        // Maximum damage (and possible tags) from the attack
        damage_instance damage_max_instance;
        // Minimum multiplier on damage above (rolled per attack)
        float min_mul;
        // Maximum multiplier on damage above (also per attack)
        float max_mul;
        // Cost in moves (for attacker)
        int move_cost;
        // If set, the attack will use a different accuracy from mon's
        // regular melee attack.
        int accuracy;
        // one_in( this - damage dealt ) chance of getting infected
        // ie. the higher is this, the lower chance of infection
        int no_infection_chance;

        bite_actor();
        ~bite_actor() { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        virtual mattack_actor *clone() const override;
};

class gun_actor : public mattack_actor
{
    public:
        // Item type of the gun we're using
        itype_id gun_type;

        /** Specific ammo type to use or for gun default if unspecified */
        ammotype ammo_type;

        /*@{*/
        /** Balanced against player. If fake_skills unspecified defaults to GUN 4, WEAPON 8 */
        std::map<skill_id, int> fake_skills;
        int fake_str = 8;
        int fake_dex = 8;
        int fake_int = 8;
        int fake_per = 8;
        /*@}*/

        /*@{*/
        /** Additional caps above any imposed by the base gun */
        int range;
        int range_no_burst; /** only burst fire at or below this distance */
        int burst_limit;
        /*@}*/

        /** Description of the attack being run */
        std::string description = "The %s fires its %s";

        // Move cost of executing the attack
        int move_cost;
        // If true, gives "grace period" to player
        bool require_targeting_player;
        // As above, but to npcs
        bool require_targeting_npc;
        // As above, but to monsters
        bool require_targeting_monster;
        // "Remember" targeting for this many turns
        int targeting_timeout;
        // Extend the above timeout by this number of turns when
        // attacking a targeted critter
        int targeting_timeout_extend;
        // Waste this many moves on targeting
        int targeting_cost;
        // Targeting isn't enough, needs to laser lock too
        // Prevents quickly changing targets
        bool laser_lock;

        // Sound of targeting u
        std::string targeting_sound;
        int targeting_volume;
        // Sounds of no ammo
        std::string no_ammo_sound;

        void shoot( monster &z, Creature &target ) const;

        gun_actor();
        ~gun_actor() { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        virtual mattack_actor *clone() const override;
};

#endif
