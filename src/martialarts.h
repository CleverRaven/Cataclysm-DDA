#pragma once
#ifndef MARTIALARTS_H
#define MARTIALARTS_H

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "bonuses.h"
#include "calendar.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "input.h"

enum damage_type : int;
class JsonObject;
class effect;
class player;
class item;
struct itype;

matype_id martial_art_learned_from( const itype & );

struct ma_requirements {
    bool was_loaded = false;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?
    bool unarmed_weapons_allowed; // If unarmed, what about unarmed weapons?
    bool wall_adjacent; // Does it only work near a wall?

    /** Minimum amount of given skill to trigger this bonus */
    std::vector<std::pair<skill_id, int>> min_skill;

    /** Minimum amount of given damage type on the weapon
     *  Note: DT_FIRE currently won't work, not even on flaming weapons!
     */
    std::vector<std::pair<damage_type, int>> min_damage;

    std::set<mabuff_id> req_buffs; // other buffs required to trigger this bonus
    std::set<std::string> req_flags; // any item flags required for this technique

    ma_requirements() {
        unarmed_allowed = false;
        melee_allowed = false;
        unarmed_weapons_allowed = true;
        wall_adjacent = false;
    }

    std::string get_description( bool buff = false ) const;

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

        std::string description;
        std::string get_description() const;

        std::string goal; // the melee goal this achieves

        // given a player's state, does this bonus apply to him?
        bool is_valid_player( const player &u ) const;

        std::set<std::string> flags;

        // message to be displayed when player or npc uses the technique
        std::string player_message;
        std::string npc_message;

        bool defensive;
        bool side_switch; // moves the target behind user
        bool dummy;
        bool crit_tec;
        bool crit_ok;

        ma_requirements reqs;

        int down_dur;
        int stun_dur;
        int knockback_dist;
        float knockback_spread; // adding randomness to knockback, like tec_throw
        bool powerful_knockback;
        std::string aoe; // corresponds to an aoe shape, defaults to just the target
        bool knockback_follow; // player follows the knocked-back party into their former tile

        // offensive
        bool disarms; // like tec_disarm
        bool dodge_counter; // counter move activated on a dodge
        bool block_counter; // counter move activated on a block

        bool miss_recovery; // allows free recovery from misses, like tec_feint
        bool grab_break; // allows grab_breaks, like tec_break

        int weighting; //how often this technique is used

        // conditional
        bool downed_target; // only works on downed enemies
        bool stunned_target; // only works on stunned enemies
        bool wall_adjacent; // only works near a wall
        bool human_target;  // only works on humanoid enemies

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
        int armor_bonus( const Character &guy, damage_type dt ) const;

        // returns the stat bonus for the various damage stats (for rolls)
        float damage_bonus( const player &u, damage_type dt ) const;

        // returns damage multipliers for the various damage stats (applied after
        // bonuses)
        float damage_mult( const player &u, damage_type dt ) const;

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
        std::string name;
        std::string description;
        std::string get_description( bool passive = false ) const;

        ma_requirements reqs;

        // mapped as buff_id -> min stacks of buff

        time_duration buff_duration; // total length this buff lasts
        int max_stacks; // total number of stacks this buff can have

        int dodges_bonus; // extra dodges, like karate
        int blocks_bonus; // extra blocks, like karate

        /** All kinds of bonuses by types to damage, hit, armor etc. */
        bonus_container bonuses;

        bool quiet;
        bool melee_allowed;
        bool throw_immune; // are we immune to throws/grabs?
        bool strictly_unarmed; // can we use unarmed weapons?
        bool strictly_melee; // can we use it without weapons?
        bool stealthy; // do we make less noise when moving?

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

        void apply_onpause_buffs( player &u ) const;

        void apply_onhit_buffs( player &u ) const;

        void apply_onattack_buffs( player &u ) const;

        void apply_ondodge_buffs( player &u ) const;

        void apply_onblock_buffs( player &u ) const;

        void apply_ongethit_buffs( player &u ) const;

        void apply_onmiss_buffs( player &u ) const;

        void apply_oncrit_buffs( player &u ) const;

        void apply_onkill_buffs( player &u ) const;

        // determines if a technique is valid or not for this style
        bool has_technique( const player &u, const matec_id &tec_id ) const;
        // determines if a weapon is valid for this style
        bool has_weapon( const std::string &itt ) const;
        // Is this weapon OK with this art?
        bool weapon_valid( const item &it ) const;
        // Getter for player style change message
        std::string get_initiate_player_message() const;
        // Getter for NPC style change message
        std::string get_initiate_npc_message() const;

        matype_id id;
        bool was_loaded = false;
        translation name;
        translation description;
        std::vector<std::string> initiate;
        std::vector<std::pair<std::string, int>> autolearn_skills;
        skill_id primary_skill;
        int learn_difficulty = 0;
        int arm_block;
        int leg_block;
        bool arm_block_with_bio_armor_arms;
        bool leg_block_with_bio_armor_legs;
        std::set<matec_id> techniques; // all available techniques
        std::set<std::string> weapons; // all style weapons
        bool strictly_unarmed; // Punch daggers etc.
        bool strictly_melee; // Must have a weapon.
        bool allow_melee; // Can use unarmed or with ANY weapon
        bool force_unarmed; // Don't use ANY weapon - punch or kick if needed
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

void load_technique( JsonObject &jo, const std::string &src );
void load_martial_art( JsonObject &jo, const std::string &src );
void check_martialarts();
void clear_techniques_and_martial_arts();
void finialize_martial_arts();
std::string martialart_difficulty( matype_id mstyle );

std::vector<matype_id> all_martialart_types();
std::vector<matype_id> autolearn_martialart_types();

#endif
