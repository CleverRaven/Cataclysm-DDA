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
        ~leap_actor() override { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        mattack_actor *clone() const override;
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
        ~bite_actor() override { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        mattack_actor *clone() const override;
};

class gun_actor : public mattack_actor
{
    public:
        // Item type of the gun we're using
        itype_id gun_type;

        /** Specific ammo type to use or for gun default if unspecified */
        itype_id ammo_type = "null";

        /*@{*/
        /** Balanced against player. If fake_skills unspecified defaults to GUN 4, WEAPON 8 */
        std::map<skill_id, int> fake_skills;
        int fake_str = 16;
        int fake_dex = 8;
        int fake_int = 8;
        int fake_per = 12;
        /*@}*/

        /** Specify weapon mode to use at different engagement distances */
        std::map<std::pair<int, int>, std::string> ranges;

        int max_ammo = INT_MAX; /** limited also by monster starting_ammo */

        /** Description of the attack being run */
        std::string description = "The %s fires its %s";

        /** Message to display (if any) for failures to fire excluding lack of ammo */
        std::string failure_msg;

        /** Sound (if any) when either starting_ammo depleted or max_ammo reached */
        std::string no_ammo_sound;

        /** Number of moves required for each attack */
        int move_cost = 150;

        /*@{*/
        /** Turrets may need to expend moves targeting before firing on certain targets */

        int targeting_cost = 100; /** Moves consumed before first attack can be made */

        bool require_targeting_player = true; /** By default always give player some warning */
        bool require_targeting_npc = false;
        bool require_targeting_monster = false;

        int targeting_timeout = 8; /** Default turns afer which targeting is lsot and needs repeating */
        int targeting_timeout_extend = 3; /** Increase timeout by this many turns after each shot */

        std::string targeting_sound = "beep-beep-beep!";
        int targeting_volume = 6; /** If set to zero don't emit any targetting sounds */

        bool laser_lock = false; /** Does switching between targets incur further targeting penalty */
        /*@}*/

        /** If true then disable this attack completely if not brightly lit */
        bool require_sunlight = false;

        void shoot( monster &z, Creature &target, const std::string &mode ) const;

        gun_actor();
        ~gun_actor() override { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        mattack_actor *clone() const override;
};

#endif
