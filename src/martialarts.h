#pragma once
#ifndef CATA_SRC_MARTIALARTS_H
#define CATA_SRC_MARTIALARTS_H

#include <cstddef>
#include <iosfwd>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bonuses.h"
#include "effect_on_condition.h"
#include "calendar.h"
#include "flat_set.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"

class input_context;
struct input_event;

class Character;
class JsonObject;
class effect;
class item;
struct itype;

class weapon_category
{
    public:
        static void load_weapon_categories( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, std::string_view src );

        static const std::vector<weapon_category> &get_all();

        const weapon_category_id &getId() const {
            return id;
        }

        const translation &name() const {
            return name_;
        }

    private:
        friend class generic_factory<weapon_category>;
        friend struct mod_tracker;

        weapon_category_id id;
        std::vector<std::pair<weapon_category_id, mod_id>> src;
        bool was_loaded = false;

        translation name_;
};

matype_id martial_art_learned_from( const itype & );

struct ma_requirements {
    bool was_loaded = false;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?
    bool unarmed_weapons_allowed; // If unarmed, what about unarmed weapons?
    bool strictly_unarmed; // Ignore force_unarmed?
    bool wall_adjacent; // Does it only work near a wall?

    /** Weapon categories compatible with this requirement. If empty, allow any weapon category. */
    std::vector<weapon_category_id> weapon_categories_allowed;

    /** Minimum amount of given skill to trigger this bonus */
    std::vector<std::pair<skill_id, int>> min_skill;

    /** Minimum amount of given damage type on the weapon
     *  Note: damage_type_id == "fire" currently won't work, not even on flaming weapons!
     */
    std::vector<std::pair<damage_type_id, int>> min_damage;

    std::set<mabuff_id> req_buffs_all; // all listed buffs required to trigger this bonus
    std::set<mabuff_id> req_buffs_any; // any listed buffs required to trigger this bonus
    std::set<mabuff_id> forbid_buffs_all; // all listed buffs prevent triggering this bonus
    std::set<mabuff_id> forbid_buffs_any; // any listed buffs prevent triggering this bonus

    std::set<flag_id> req_flags; // any item flags required for this technique
    cata::flat_set<json_character_flag> req_char_flags; // any listed character flags required
    cata::flat_set<json_character_flag> req_char_flags_all; // all listed character flags required
    cata::flat_set<json_character_flag> forbidden_char_flags; // Character flags disabling the technique

    ma_requirements() {
        unarmed_allowed = false;
        melee_allowed = false;
        unarmed_weapons_allowed = true;
        strictly_unarmed = false;
        wall_adjacent = false;
    }

    std::string get_description( bool buff = false ) const;

    bool buff_requirements_satisfied( const Character &u ) const;
    bool is_valid_character( const Character &u ) const;
    bool is_valid_weapon( const item &i ) const;

    void load( const JsonObject &jo, std::string_view src );
};

struct tech_effect_data {
    efftype_id id;
    int duration;
    bool permanent;
    bool on_damage;
    int chance;
    std::string message;
    json_character_flag req_flag;

    tech_effect_data( const efftype_id &nid, int dur, bool perm, bool ondmg,
                      int nchance, std::string message, json_character_flag req_flag ) :
        id( nid ), duration( dur ), permanent( perm ), on_damage( ondmg ),
        chance( nchance ), message( std::move( message ) ), req_flag( req_flag ) {}
};

class ma_technique
{
    public:
        ma_technique();

        void load( const JsonObject &jo, const std::string &src );

        matec_id id;
        std::vector<std::pair<matec_id, mod_id>> src;
        bool was_loaded = false;
        translation name;

        translation description;
        std::string get_description() const;

        std::string goal; // the melee goal this achieves

        // given a Character's state, does this bonus apply to him?
        bool is_valid_character( const Character &u ) const;

        std::set<std::string> flags;

        // message to be displayed when Character or npc uses the technique
        translation avatar_message;
        translation npc_message;

        bool defensive = false;
        bool side_switch = false; // moves the target behind user
        bool dummy = false;
        bool crit_tec = false;
        bool crit_ok = false;
        bool reach_tec = false; // only possible to use during a reach attack
        bool reach_ok = false; // possible to use during a reach attack
        bool attack_override = false; // The attack replaces the one it triggered off of

        ma_requirements reqs;

        // What way is the technique delivered to the target?
        std::vector<std::string> attack_vectors; // by priority
        std::vector<std::string> attack_vectors_random; // randomly

        int repeat_min = 1;    // Number of times the technique is repeated on a successful proc
        int repeat_max = 1;
        int down_dur = 0;
        int stun_dur = 0;
        int knockback_dist = 0;
        float knockback_spread = 0.0f;  // adding randomness to knockback, like tec_throw
        std::string aoe;                // corresponds to an aoe shape, defaults to just the target
        bool knockback_follow = false;  // Character follows the knocked-back party into their former tile

        // offensive
        bool disarms = false;       // like tec_disarm
        bool take_weapon = false;   // disarms and equips weapon if hands are free
        bool dodge_counter = false; // counter move activated on a dodge
        bool block_counter = false; // counter move activated on a block

        bool miss_recovery = false; // reduces the total move cost of a miss by 50%, post stumble modifier
        bool grab_break = false;    // allows grab_breaks, like tec_break

        int weighting = 0; //how often this technique is used

        // conditional
        bool wall_adjacent = false; // only works near a wall

        bool needs_ammo = false;    // technique only works if the item is loaded with ammo

        // Dialogue conditions of the attack
        std::function<bool( dialogue & )> condition;
        std::string condition_desc;
        bool has_condition = false;

        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_container bonuses;

        std::vector<tech_effect_data> tech_effects;

        float damage_bonus( const Character &u, const damage_type_id &type ) const;
        float damage_multiplier( const Character &u, const damage_type_id &type ) const;
        float move_cost_multiplier( const Character &u ) const;
        float move_cost_penalty( const Character &u ) const;
        float armor_penetration( const Character &u, const damage_type_id &type ) const;

        std::vector<effect_on_condition_id> eocs;
};

class ma_buff
{
    public:
        ma_buff();

        // utility function so to prevent duplicate buff copies, we use this
        // instead of add_disease (since all buffs have the same distype)
        void apply_buff( Character &u ) const;

        // given a Character's state, does this bonus apply to him?
        bool is_valid_character( const Character &u ) const;

        // apply static bonuses to a Character
        void apply_character( Character &u ) const;

        // returns the stat bonus for the on-hit stat (for rolls)
        int block_effectiveness_bonus( const Character &u ) const;
        int hit_bonus( const Character &u ) const;
        int critical_hit_chance_bonus( const Character &u ) const;
        int dodge_bonus( const Character &u ) const;
        int speed_bonus( const Character &u ) const;
        int block_bonus( const Character &u ) const;
        int arpen_bonus( const Character &u, const damage_type_id &dt ) const;

        // returns the armor bonus for various armor stats (equivalent to armor)
        int armor_bonus( const Character &guy, const damage_type_id &dt ) const;

        // returns the stat bonus for the various damage stats (for rolls)
        float damage_bonus( const Character &u, const damage_type_id &dt ) const;

        // returns damage multipliers for the various damage stats (applied after
        // bonuses)
        float damage_mult( const Character &u, const damage_type_id &dt ) const;

        // returns various boolean flags
        bool is_melee_bash_damage_cap_bonus() const;
        bool is_quiet() const;
        bool can_melee() const;
        bool is_stealthy() const;
        bool has_flag( const json_character_flag &flag ) const;

        // The ID of the effect that is used to store this buff
        efftype_id get_effect_id() const;
        // If the effects represents an ma_buff effect, return the ma_buff, otherwise return null.
        static const ma_buff *from_effect( const effect &eff );

        mabuff_id id;
        std::vector<std::pair<mabuff_id, mod_id>> src;
        bool was_loaded = false;
        translation name;
        translation description;
        std::string get_description( bool passive = false ) const;

        ma_requirements reqs;

        cata::flat_set<json_character_flag> flags;

        // mapped as buff_id -> min stacks of buff

        time_duration buff_duration = 0_turns; // total length this buff lasts
        int max_stacks = 0; // total number of stacks this buff can have
        bool persists = false; // prevent buff removal when switching styles

        int dodges_bonus = 0; // extra dodges, like karate
        int blocks_bonus = 0; // extra blocks, like karate

        /** All kinds of bonuses by types to damage, hit, armor etc. */
        bonus_container bonuses;

        bool quiet = false;
        bool melee_allowed = false;
        bool melee_bash_damage_cap_bonus = false;
        bool strictly_melee = false; // can we only use it with weapons?
        bool stealthy = false; // do we make less noise when moving?

        void load( const JsonObject &jo, std::string_view src );
};

class martialart
{
    public:
        martialart();

        void load( const JsonObject &jo, const std::string &src );

        void remove_all_buffs( Character &u ) const;

        // modifies a Character's "current" stats with various types of bonuses
        void apply_static_buffs( Character &u ) const;

        void apply_onmove_buffs( Character &u ) const;

        void apply_onpause_buffs( Character &u ) const;

        void apply_onhit_buffs( Character &u ) const;

        void apply_onattack_buffs( Character &u ) const;

        void apply_ondodge_buffs( Character &u ) const;

        void apply_onblock_buffs( Character &u ) const;

        void apply_ongethit_buffs( Character &u ) const;

        void apply_onmiss_buffs( Character &u ) const;

        void apply_oncrit_buffs( Character &u ) const;

        void apply_onkill_buffs( Character &u ) const;

        void activate_eocs( Character &u, const std::vector<effect_on_condition_id> &eocs ) const;

        // activates eocs when conditions are met
        void apply_static_eocs( Character &u ) const;

        void apply_onmove_eocs( Character &u ) const;

        void apply_onpause_eocs( Character &u ) const;

        void apply_onhit_eocs( Character &u ) const;

        void apply_onattack_eocs( Character &u ) const;

        void apply_ondodge_eocs( Character &u ) const;

        void apply_onblock_eocs( Character &u ) const;

        void apply_ongethit_eocs( Character &u ) const;

        void apply_onmiss_eocs( Character &u ) const;

        void apply_oncrit_eocs( Character &u ) const;

        void apply_onkill_eocs( Character &u ) const;

        // determines if a technique is valid or not for this style
        bool has_technique( const Character &u, const matec_id &tec_id ) const;
        // determines if a weapon is valid for this style
        bool has_weapon( const itype_id & ) const;
        // Is this weapon OK with this art?
        bool weapon_valid( const item_location &it ) const;
        // Getter for Character style change message
        std::string get_initiate_avatar_message() const;
        // Getter for NPC style change message
        std::string get_initiate_npc_message() const;

        matype_id id;
        std::vector<std::pair<matype_id, mod_id>> src;
        bool was_loaded = false;
        translation name;
        translation description;
        std::vector<translation> initiate;
        int priority = 0;
        std::vector<std::pair<std::string, int>> autolearn_skills;
        skill_id primary_skill;
        bool teachable = true;
        int learn_difficulty = 0;
        int arm_block = 0;
        int leg_block = 0;
        int nonstandard_block = 0;
        bool arm_block_with_bio_armor_arms = false;
        bool leg_block_with_bio_armor_legs = false;
        std::set<matec_id> techniques; // all available techniques
        std::set<itype_id> weapons; // all style weapons
        std::set<weapon_category_id> weapon_category; // all style weapon categories
        bool strictly_unarmed = false; // Punch daggers etc.
        bool strictly_melee = false; // Must have a weapon.
        bool allow_all_weapons = false; // Can use unarmed or with ANY weapon
        bool force_unarmed = false; // Don't use ANY weapon - punch or kick if needed
        bool prevent_weapon_blocking = false; // Cannot block with weapons
        std::vector<mabuff_id> static_buffs; // all buffs triggered by each condition
        std::vector<mabuff_id> onmove_buffs;
        std::vector<mabuff_id> onpause_buffs;
        std::vector<mabuff_id> onhit_buffs;
        std::vector<mabuff_id> onattack_buffs;
        std::vector<mabuff_id> ondodge_buffs;
        std::vector<mabuff_id> onblock_buffs;
        std::vector<mabuff_id> ongethit_buffs;
        std::vector<mabuff_id> onmiss_buffs;
        std::vector<mabuff_id> oncrit_buffs;
        std::vector<mabuff_id> onkill_buffs;
        std::vector<effect_on_condition_id> static_eocs; // all eocs triggered by each condition
        std::vector<effect_on_condition_id> onmove_eocs;
        std::vector<effect_on_condition_id> onpause_eocs;
        std::vector<effect_on_condition_id> onhit_eocs;
        std::vector<effect_on_condition_id> onattack_eocs;
        std::vector<effect_on_condition_id> ondodge_eocs;
        std::vector<effect_on_condition_id> onblock_eocs;
        std::vector<effect_on_condition_id> ongethit_eocs;
        std::vector<effect_on_condition_id> onmiss_eocs;
        std::vector<effect_on_condition_id> oncrit_eocs;
        std::vector<effect_on_condition_id> onkill_eocs;
};

class ma_style_callback : public uilist_callback
{
    private:
        size_t offset;
        const std::vector<matype_id> &styles;

    public:
        ma_style_callback( int style_offset, const std::vector<matype_id> &selectable_styles )
            : offset( style_offset )
            , styles( selectable_styles )
        {}

        bool key( const input_context &ctxt, const input_event &event, int entnum, uilist *menu ) override;
        ~ma_style_callback() override = default;
};

tech_effect_data load_tech_effect_data( const JsonObject &e );
void load_technique( const JsonObject &jo, const std::string &src );
void load_martial_art( const JsonObject &jo, const std::string &src );
void check_martialarts();
void clear_techniques_and_martial_arts();
void finalize_martial_arts();
std::string martialart_difficulty( const matype_id &mstyle );

std::vector<matype_id> all_martialart_types();
std::vector<matype_id> autolearn_martialart_types();

#endif // CATA_SRC_MARTIALARTS_H
