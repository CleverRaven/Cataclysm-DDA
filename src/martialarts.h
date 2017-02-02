#pragma once
#ifndef MARTIALARTS_H
#define MARTIALARTS_H

#include "pldata.h"
#include "json.h"
#include "string_id.h"
#include "bonuses.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class effect;
class player;
class item;
class martialart;
using matype_id = string_id<martialart>;
class ma_buff;
using mabuff_id = string_id<ma_buff>;
class ma_technique;
using matec_id = string_id<ma_technique>;
class effect_type;
using efftype_id = string_id<effect_type>;
class Skill;
using skill_id = string_id<Skill>;

struct ma_requirements {
    bool was_loaded = false;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?
    bool strictly_unarmed; // If unarmed, what about unarmed weapons?

    /** Minimum amount of given skill to trigger this bonus */
    std::map<skill_id, int> min_skill;

    /** Minimum amount of given damage type on the weapon
     *  Note: DT_FIRE currently won't work, not even on flaming weapons!
     */
    std::map<damage_type, int> min_damage;

    std::set<mabuff_id> req_buffs; // other buffs required to trigger this bonus
    std::set<std::string> req_flags; // any item flags required for this technique

    ma_requirements() {
        unarmed_allowed = false;
        melee_allowed = false;
        strictly_unarmed = false;
    }

    bool is_valid_player( const player &u ) const;
    bool is_valid_weapon( const item &i ) const;

    void load( JsonObject &jo, const std::string &src );
};

class ma_technique
{
    public:
        ma_technique();

        void load( JsonObject &jo, const std::string &src );

        matec_id id;
        bool was_loaded = false;
        std::string name;

        std::string goal; // the melee goal this achieves

        // given a player's state, does this bonus apply to him?
        bool is_valid_player( const player &u ) const;

        std::set<std::string> flags;

        // message to be displayed when player or npc uses the technique
        std::string player_message;
        std::string npc_message;

        bool defensive;
        bool dummy;
        bool crit_tec;

        ma_requirements reqs;

        int down_dur;
        int stun_dur;
        int knockback_dist;
        float knockback_spread; // adding randomness to knockback, like tec_throw
        std::string aoe; // corresponds to an aoe shape, defaults to just the target

        // offensive
        bool disarms; // like tec_disarm
        bool dodge_counter; // counter move activated on a dodge
        bool block_counter; // counter move activated on a block

        bool miss_recovery; // allows free recovery from misses, like tec_feint
        bool grab_break; // allows grab_breaks, like tec_break

        int weighting; //how often this technique is used

        /** All kinds of bonuses by types to damage, hit etc. */
        bonus_container bonuses;

        float damage_bonus( const player &u, damage_type type ) const;
        float damage_multiplier( const player &u, damage_type type ) const;
        float move_cost_multiplier( const player &u ) const;
        float move_cost_penalty( const player &u ) const;
        float armor_penetration( const player &u, damage_type type ) const;
};

class ma_buff
{
    public:
        ma_buff();

        // utility function so to prevent duplicate buff copies, we use this
        // instead of add_disease (since all buffs have the same distype)
        void apply_buff( player &u ) const;

        // given a player's state, does this bonus apply to him?
        bool is_valid_player( const player &u ) const;

        // apply static bonuses to a player
        void apply_player( player &u ) const;

        // returns the stat bonus for the on-hit stat (for rolls)
        int hit_bonus( const player &u ) const;
        int dodge_bonus( const player &u ) const;
        int speed_bonus( const player &u ) const;
        int block_bonus( const player &u ) const;

        // returns the armor bonus for various armor stats (equivalent to armor)
        int armor_bonus( const player &u, damage_type type ) const;

        // returns the stat bonus for the various damage stats (for rolls)
        float damage_bonus( const player &u, damage_type type ) const;

        // returns damage multipliers for the various damage stats (applied after
        // bonuses)
        float damage_mult( const player &u, damage_type type ) const;

        /** Stamina cost multiplier */
        float stamina_mult() const;

        // returns various boolean flags
        bool is_throw_immune() const;
        bool is_quiet() const;
        bool can_melee() const;
        bool can_unarmed_weapon() const;

        // The ID of the effect that is used to store this buff
        efftype_id get_effect_id() const;
        // If the effects represents an ma_buff effect, return the ma_buff, otherwise return null.
        static const ma_buff *from_effect( const effect &eff );

        mabuff_id id;
        bool was_loaded = false;
        std::string name;
        std::string description;

        ma_requirements reqs;

        // mapped as buff_id -> min stacks of buff

        int buff_duration; // total length this buff lasts
        int max_stacks; // total number of stacks this buff can have

        int dodges_bonus; // extra dodges, like karate
        int blocks_bonus; // extra blocks, like karate

        /** All kinds of bonuses by types to damage, hit, armor etc. */
        bonus_container bonuses;

        bool quiet;
        bool melee_allowed;
        bool throw_immune; // are we immune to throws/grabs?
        bool strictly_unarmed; // can we use unarmed weapons?

        void load( JsonObject &jo, const std::string &src );
};

class martialart
{
    public:
        martialart();

        void load( JsonObject &jo, const std::string &src );

        // modifies a player's "current" stats with various types of bonuses
        void apply_static_buffs( player &u ) const;

        void apply_onmove_buffs( player &u ) const;

        void apply_onhit_buffs( player &u ) const;

        void apply_onattack_buffs( player &u ) const;

        void apply_ondodge_buffs( player &u ) const;

        void apply_onblock_buffs( player &u ) const;

        void apply_ongethit_buffs( player &u ) const;

        // determines if a technique is valid or not for this style
        bool has_technique( const player &u, const matec_id &tech ) const;
        // determines if a weapon is valid for this style
        bool has_weapon( const std::string &item ) const;
        // Is this weapon OK with this art?
        bool weapon_valid( const item &u ) const;

        matype_id id;
        bool was_loaded = false;
        std::string name;
        std::string description;
        int arm_block;
        int leg_block;
        bool arm_block_with_bio_armor_arms;
        bool leg_block_with_bio_armor_legs;
        std::set<matec_id> techniques; // all available techniques
        std::set<std::string> weapons; // all style weapons
        bool strictly_unarmed; // Punch daggers etc.
        std::vector<mabuff_id> static_buffs; // all buffs triggered by each condition
        std::vector<mabuff_id> onmove_buffs;
        std::vector<mabuff_id> onhit_buffs;
        std::vector<mabuff_id> onattack_buffs;
        std::vector<mabuff_id> ondodge_buffs;
        std::vector<mabuff_id> onblock_buffs;
        std::vector<mabuff_id> ongethit_buffs;
};

void load_technique( JsonObject &jo, const std::string &src );
void load_martial_art( JsonObject &jo, const std::string &src );
void check_martialarts();
void clear_techniques_and_martial_arts();
void finialize_martial_arts();

std::vector<matype_id> all_martialart_types();

#endif
