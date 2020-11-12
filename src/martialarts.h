#pragma once
#ifndef CATA_SRC_MARTIALARTS_H
#define CATA_SRC_MARTIALARTS_H

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bonuses.h"
#include "calendar.h"
#include "input.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"

class input_context;
struct input_event;

enum class damage_type : int;
class Character;
class JsonObject;
class effect;
class item;
struct itype;

matype_id martial_art_learned_from( const itype & );

struct ma_requirements {
    bool was_loaded = false;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?
    bool unarmed_weapons_allowed; // If unarmed, what about unarmed weapons?
    bool strictly_unarmed; // Ignore force_unarmed?
    bool wall_adjacent; // Does it only work near a wall?

    /** Minimum amount of given skill to trigger this bonus */
    std::vector<std::pair<skill_id, int>> min_skill;

    /** Minimum amount of given damage type on the weapon
     *  Note: damage_type::FIRE currently won't work, not even on flaming weapons!
     */
    std::vector<std::pair<damage_type, int>> min_damage;

    std::set<mabuff_id> req_buffs; // other buffs required to trigger this bonus
    std::set<flag_str_id> req_flags; // any item flags required for this technique

    ma_requirements() {
        unarmed_allowed = false;
        melee_allowed = false;
        unarmed_weapons_allowed = true;
        strictly_unarmed = false;
        wall_adjacent = false;
    }

    std::string get_description( bool buff = false ) const;

    bool is_valid_character( const Character &u ) const;
    bool is_valid_weapon( const item &i ) const;

    void load( const JsonObject &jo, const std::string &src );
};

class ma_technique
{
    public:
        ma_technique();

        void load( const JsonObject &jo, const std::string &src );

        matec_id id;
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

        ma_requirements reqs;

        int down_dur = 0;
        int stun_dur = 0;
        int knockback_dist = 0;
        float knockback_spread = 0.0f;  // adding randomness to knockback, like tec_throw
        bool powerful_knockback = false;
        std::string aoe;                // corresponds to an aoe shape, defaults to just the target
        bool knockback_follow = false;  // Character follows the knocked-back party into their former tile

        // offensive
        bool disarms = false;       // like tec_disarm
        bool take_weapon = false;   // disarms and equips weapon if hands are free
        bool dodge_counter = false; // counter move activated on a dodge
        bool block_counter = false; // counter move activated on a block

        bool miss_recovery = false; // allows free recovery from misses, like tec_feint
        bool grab_break = false;    // allows grab_breaks, like tec_break

        int weighting = 0; //how often this technique is used

        // conditional
        bool downed_target = false; // only works on downed enemies
        bool stunned_target = false;// only works on stunned enemies
        bool wall_adjacent = false; // only works near a wall
        bool human_target = false;  // only works on humanoid enemies

        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_container bonuses;

        float damage_bonus( const Character &u, damage_type type ) const;
        float damage_multiplier( const Character &u, damage_type type ) const;
        float move_cost_multiplier( const Character &u ) const;
        float move_cost_penalty( const Character &u ) const;
        float armor_penetration( const Character &u, damage_type type ) const;
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

        // returns the armor bonus for various armor stats (equivalent to armor)
        int armor_bonus( const Character &guy, damage_type dt ) const;

        // returns the stat bonus for the various damage stats (for rolls)
        float damage_bonus( const Character &u, damage_type dt ) const;

        // returns damage multipliers for the various damage stats (applied after
        // bonuses)
        float damage_mult( const Character &u, damage_type dt ) const;

        // returns various boolean flags
        bool is_throw_immune() const;
        bool is_quiet() const;
        bool can_melee() const;
        bool is_stealthy() const;

        // The ID of the effect that is used to store this buff
        efftype_id get_effect_id() const;
        // If the effects represents an ma_buff effect, return the ma_buff, otherwise return null.
        static const ma_buff *from_effect( const effect &eff );

        mabuff_id id;
        bool was_loaded = false;
        translation name;
        translation description;
        std::string get_description( bool passive = false ) const;

        ma_requirements reqs;

        // mapped as buff_id -> min stacks of buff

        time_duration buff_duration = 0_turns; // total length this buff lasts
        int max_stacks = 0; // total number of stacks this buff can have

        int dodges_bonus = 0; // extra dodges, like karate
        int blocks_bonus = 0; // extra blocks, like karate

        /** All kinds of bonuses by types to damage, hit, armor etc. */
        bonus_container bonuses;

        bool quiet = false;
        bool melee_allowed = false;
        bool throw_immune = false; // are we immune to throws/grabs?
        bool strictly_melee = false; // can we only use it with weapons?
        bool stealthy = false; // do we make less noise when moving?

        void load( const JsonObject &jo, const std::string &src );
};

class martialart
{
    public:
        martialart();

        void load( const JsonObject &jo, const std::string &src );

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

        // determines if a technique is valid or not for this style
        bool has_technique( const Character &u, const matec_id &tec_id ) const;
        // determines if a weapon is valid for this style
        bool has_weapon( const itype_id & ) const;
        // Is this weapon OK with this art?
        bool weapon_valid( const item &it ) const;
        // Getter for Character style change message
        std::string get_initiate_avatar_message() const;
        // Getter for NPC style change message
        std::string get_initiate_npc_message() const;

        matype_id id;
        bool was_loaded = false;
        translation name;
        translation description;
        std::vector<translation> initiate;
        std::vector<std::pair<std::string, int>> autolearn_skills;
        skill_id primary_skill;
        int learn_difficulty = 0;
        int arm_block = 0;
        int leg_block = 0;
        bool arm_block_with_bio_armor_arms = false;
        bool leg_block_with_bio_armor_legs = false;
        std::set<matec_id> techniques; // all available techniques
        std::set<itype_id> weapons; // all style weapons
        bool strictly_unarmed = false; // Punch daggers etc.
        bool strictly_melee = false; // Must have a weapon.
        bool allow_melee = false; // Can use unarmed or with ANY weapon
        bool force_unarmed = false; // Don't use ANY weapon - punch or kick if needed
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

void load_technique( const JsonObject &jo, const std::string &src );
void load_martial_art( const JsonObject &jo, const std::string &src );
void check_martialarts();
void clear_techniques_and_martial_arts();
void finialize_martial_arts();
std::string martialart_difficulty( const matype_id &mstyle );

std::vector<matype_id> all_martialart_types();
std::vector<matype_id> autolearn_martialart_types();

#endif // CATA_SRC_MARTIALARTS_H
