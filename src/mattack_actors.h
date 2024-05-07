#pragma once
#ifndef CATA_SRC_MATTACK_ACTORS_H
#define CATA_SRC_MATTACK_ACTORS_H

#include <climits>
#include <iosfwd>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "damage.h"
#include "magic.h"
#include "mattack_common.h"
#include "translations.h"
#include "weighted_list.h"

class Creature;
class JsonObject;
class monster;
struct mon_effect_data;

class leap_actor : public mattack_actor
{
    public:
        float max_range = 0.0f;
        // Jump has to be at least this tiles long
        float min_range = 0.0f;
        // Don't leap without a hostile target creature
        bool allow_no_target = false;
        // Always leap, even when already adjacent to target
        bool prefer_leap = false;
        // Leap completely randomly regardless of target distance/direction
        bool random_leap = false;
        // Leap including terrains it doesn't usually move
        // i.e. Aquatic monsters leap onto land
        bool ignore_dest_terrain = false;
        // Leap including tiles where there is something it would usually avoid,
        // such as open air, fire, or traps
        bool ignore_dest_danger = false;
        int move_cost = 0;
        // Range to target below which we don't consider jumping at all
        float min_consider_range = 0.0f;
        // Don't jump if distance to target is more than this
        float max_consider_range = 0.0f;

        std::vector<mon_effect_data> self_effects;

        translation message;

        leap_actor() = default;
        ~leap_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class mon_spellcasting_actor : public mattack_actor
{
    public:
        fake_spell spell_data;

        bool allow_no_target = false;

        mon_spellcasting_actor() = default;
        ~mon_spellcasting_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

struct grab {
    // Intensity of grab effect applied, defaults to the monster's defined grab_strength unless specified
    int grab_strength;
    // Percent chance to initiate a pull
    int pull_chance;
    // Ratio of pullee:puller weight
    float pull_weight_ratio;
    // Which effect should we apply on a successful grab to our target (bp)
    // Limited to one GRAB-flagged effect per bp
    efftype_id grab_effect;
    // If true will attempt to remove all other GRAB flagged effects from the target and cancel the attack on failure
    bool exclusive_grab = false;
    // If true drags/pulls fail when targeting a character in a seat with seatbelts
    bool respect_seatbelts = true;
    // Distance the enemy drags you on successful drag attempt (also enable dragging in the first place)
    int drag_distance;
    // Deviation of each dragging step from a straight line away from the opponent
    int drag_deviation;
    // Number of drag steps which give you a grab break attempt
    int drag_grab_break_distance;
    // Movecost modifier for drag-related movements
    float drag_movecost_mod;
    // Messages for pulls and drags, those are mutually exclusive
    translation pull_msg_u;
    translation pull_fail_msg_u;
    translation pull_msg_npc;
    translation pull_fail_msg_npc;
    void load_grab( const JsonObject &jo );
    bool was_loaded = false;
};

class melee_actor : public mattack_actor
{
    public:
        // Maximum damage from the attack
        damage_instance damage_max_instance;
        // Minimum multiplier on damage above (rolled per attack)
        float min_mul = 0.5f;
        // Maximum multiplier on damage above (also per attack)
        float max_mul = 1.0f;
        // Cost in moves (for attacker)
        int move_cost = 100;
        // If non-negative, the attack will use a different accuracy from mon's
        // regular melee attack.
        int accuracy = INT_MIN;
        // Attack range, 1 means melee only
        int range = 1;
        // Attack fails if aimed at adjacent targets
        bool no_adjacent = false;
        // Determines if a special attack can be dodged
        bool dodgeable = true;
        // Determines if UNCANNY_DODGE (or the bionic) can be used to dodge this attack
        bool uncanny_dodgeable = true;
        // Determines if a special attack can be blocked
        bool blockable = true;
        // Determines if effects are only applied on damagin attacks
        bool effects_require_dmg = true;
        // Determines if effects are only applied on non bionic limbs
        bool effects_require_organic = false;
        // If non-zero, the attack will fling targets, 10 throw_strength = 1 tile range
        int throw_strength = 0;
        // Limits on target bodypart hit sizes
        int hitsize_min = -1;
        int hitsize_max = -1;
        bool attack_upper = true;
        grab grab_data;
        bool is_grab = false;

        /**
         * If empty, regular melee roll body part selection is used.
         * If non-empty, a body part is selected from the map to be targeted,
         * with a chance proportional to the value.
         */
        weighted_float_list<bodypart_str_id> body_parts;

        /** Extra effects applied on hit. */
        std::vector<mon_effect_data> effects;
        // Set of effects applied to the monster itself on attack/hit/damage
        std::vector<mon_effect_data> self_effects_always;
        std::vector<mon_effect_data> self_effects_onhit;
        std::vector<mon_effect_data> self_effects_ondmg;

        /** Message for missed attack against the player. */
        translation miss_msg_u;
        /** Message for 0 damage hit against the player. */
        translation no_dmg_msg_u;
        /** Message for damaging hit against the player. */
        translation hit_dmg_u;
        /** Message for throwing the player. */
        translation throw_msg_u;

        /** Message for missed attack against a non-player. */
        translation miss_msg_npc;
        /** Message for 0 damage hit against a non-player. */
        translation no_dmg_msg_npc;
        /** Message for damaging hit against a non-player. */
        translation hit_dmg_npc;
        /** Message for throwing a non-player. */
        translation throw_msg_npc;

        melee_actor();
        ~melee_actor() override = default;

        virtual Creature *find_target( monster &z ) const;
        virtual void on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        /* Dedicated grab faction. Possible returns: -1 fails silently (attempting another attack instead)
        0 fails loudly (resetting the cooldown), 1 succeeds */
        int do_grab( monster &, Creature *target, bodypart_id bp_id ) const;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class bite_actor : public melee_actor
{
    public:
        // one_in( this - damage dealt ) chance of getting infected
        // i.e. the higher is this, the lower chance of infection
        int infection_chance = 0;

        bite_actor();
        ~bite_actor() override = default;

        void on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const override;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        std::unique_ptr<mattack_actor> clone() const override;
};

class gun_actor : public mattack_actor
{
    public:
        // Item type of the gun we're using
        itype_id gun_type;

        /** Specific ammo type to use or for gun default if unspecified */
        itype_id ammo_type = itype_id::NULL_ID();

        /*@{*/
        /** Balanced against player. If fake_skills unspecified defaults to GUN 4, WEAPON 8 */
        std::map<skill_id, int> fake_skills;
        int fake_str = 16;
        int fake_dex = 8;
        int fake_int = 8;
        int fake_per = 12;
        /*@}*/

        /** Specify weapon mode to use at different engagement distances */
        std::map<std::pair<int, int>, gun_mode_id> ranges;

        int max_ammo = INT_MAX; /** limited also by monster starting ammo */

        /** Description of the attack being run */
        translation description;

        /** Message to display (if any) for failures to fire excluding lack of ammo */
        translation failure_msg;

        /** Sound (if any) when either ammo depleted or max_ammo reached */
        translation no_ammo_sound;

        /** Number of moves required for each attack */
        int move_cost = 150;

        /** Should moving vehicles be targeted */
        bool target_moving_vehicles = false;
        /*@{*/
        /** Turrets may need to expend moves targeting before firing on certain targets */

        int targeting_cost = 100; /** Moves consumed before first attack can be made */

        bool require_targeting_player = true; /** By default always give player some warning */
        bool require_targeting_npc = false;
        bool require_targeting_monster = false;

        int targeting_timeout = 8; /** Default turns after which targeting is lost and needs repeating */
        int targeting_timeout_extend = 3; /** Increase timeout by this many turns after each shot */

        translation targeting_sound;
        int targeting_volume = 6; /** If set to zero don't emit any targeting sounds */

        bool laser_lock = false; /** Does switching between targets incur further targeting penalty */
        /*@}*/

        /** If true then disable this attack completely if not brightly lit */
        bool require_sunlight = false;

        bool try_target( monster &z, Creature &target ) const;
        void shoot( monster &z, const tripoint &target, const gun_mode_id &mode,
                    int inital_recoil = 0 ) const;
        int get_max_range() const;

        gun_actor();
        ~gun_actor() override = default;

        void load_internal( const JsonObject &obj, const std::string &src ) override;
        bool call( monster & ) const override;
        std::unique_ptr<mattack_actor> clone() const override;
};

#endif // CATA_SRC_MATTACK_ACTORS_H
