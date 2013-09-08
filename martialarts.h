#ifndef _MARTIALARTS_H_
#define _MARTIALARTS_H_

#include "player.h"
#include "pldata.h"
#include "itype.h"
#include <string>
#include <vector>
#include <map>
#include <set>

typedef std::string matype_id;

typedef std::string mabuff_id;

typedef std::string matec_id;

class ma_technique {
  public:
    ma_technique();

    // given a player's state, does this bonus apply to him?
    bool is_valid_player(player& u);

    std::set<std::string> flags;

    std::string verb_you;
    std::string verb_npc;

    // technique info
    style_move move;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?

    int min_melee; // minimum amount of unarmed to trigger this technique
    int min_unarmed; // etc
    int min_bashing;
    int min_cutting;
    int min_stabbing;
};

class ma_buff {
  public:
    ma_buff();

    // utility function so to prevent duplicate buff copies, we use this
    // instead of add_disease (since all buffs have the same distype)
    void apply_buff(std::vector<disease>& dVec);

    // given a player's state, does this bonus apply to him?
    bool is_valid_player(player& u);

    // apply static bonuses to a player
    void apply_player(player& u);

    // returns the stat bonus for the on-hit stat (for rolls)
    int hit_bonus(player& u);
    int dodge_bonus(player& u);
    int speed_bonus(player& u);
    int block_bonus(player& u);

    // returns the stat bonus for the various damage stats (for rolls)
    int bash_bonus(player& u);
    int cut_bonus(player& u);

    // returns damage multipliers for the various damage stats (applied after
    // bonuses)
    float bash_mult();
    float cut_mult();

    // returns various boolean flags
    bool is_throw_immune();

    std::string id;
    std::string name;
    std::string desc;

    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?

    int buff_duration; // total length this buff lasts
    int max_stacks; // total number of stacks this buff can have

    int min_melee; // minimum amount of unarmed to trigger this bonus
    int min_unarmed; // minimum amount of unarmed to trigger this bonus
    int min_bashing; // minimum amount of unarmed to trigger this bonus
    int min_cutting; // minimum amount of unarmed to trigger this bonus
    int min_stabbing; // minimum amount of unarmed to trigger this bonus

    std::set<mabuff_id> req_buffs; // other buffs required to trigger this bonus
    // mapped as buff_id -> min stacks of buff

    int dodges_bonus; // extra dodges, like karate
    int blocks_bonus; // extra blocks, like karate

    int hit; // flat bonus to hit
    int bash; // flat bonus to bash
    int cut; // flat bonus to cut
    int dodge; // flat dodge bonus
    int speed; // flat speed bonus
    int block; // unarmed block damage reduction

    float bash_stat_mult; // bash damage multiplier, like aikido
    float cut_stat_mult; // cut damage multiplier

    float bash_str; // bonus damage to add per str point
    float bash_dex; // "" dex point
    float bash_int; // "" int point
    float bash_per; // "" per point

    float cut_str; // bonus cut damage to add per str point
    float cut_dex; // "" dex point
    float cut_int; // "" int point
    float cut_per; // "" per point

    float hit_str; // bonus to-hit to add per str point
    float hit_dex; // "" dex point
    float hit_int; // "" int point
    float hit_per; // "" per point

    float dodge_str; // bonus dodge to add per str point
    float dodge_dex; // "" dex point
    float dodge_int; // "" int point
    float dodge_per; // "" per point

    float block_str; // bonus block DR per str point
    float block_dex; // "" dex point
    float block_int; // "" int point
    float block_per; // "" per point

    bool throw_immune; // are we immune to throws/grabs?
};

class martialart {
  public:
    martialart();

    // modifies a player's "current" stats with various types of bonuses
    void apply_static_buffs(player& u, std::vector<disease>& dVec);

    void apply_onhit_buffs(player& u, std::vector<disease>& dVec);

    void apply_onmove_buffs(player& u, std::vector<disease>& dVec);

    void apply_ondodge_buffs(player& u, std::vector<disease>& dVec);

    // determines if a technique is valid or not for this style
    bool has_technique(player& u, technique_id tech);

    // gets custom melee string for a technique under this style
    std::string melee_verb(technique_id tech, player& u);

    std::string id;
    std::string name;
    std::string desc;
    std::vector<ma_technique> techniques; // all available techniques
    std::vector<ma_buff> static_buffs; // all buffs triggered by each condition
    std::vector<ma_buff> onmove_buffs;
    std::vector<ma_buff> onhit_buffs;
    std::vector<ma_buff> ondodge_buffs;

};

#endif
