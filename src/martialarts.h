#ifndef _MARTIALARTS_H_
#define _MARTIALARTS_H_

#include "player.h"
#include "pldata.h"
#include "json.h"
#include <string>
#include <vector>
#include <map>
#include <set>

struct ma_requirements {
    bool unarmed_allowed; // does this bonus work when unarmed?
    bool melee_allowed; // what about with a melee weapon?

    int min_melee; // minimum amount of unarmed to trigger this bonus
    int min_unarmed; // minimum amount of unarmed to trigger this bonus
    int min_bashing; // minimum amount of unarmed to trigger this bonus
    int min_cutting; // minimum amount of unarmed to trigger this bonus
    int min_stabbing; // minimum amount of unarmed to trigger this bonus

    std::set<mabuff_id> req_buffs; // other buffs required to trigger this bonus

    ma_requirements() {
      unarmed_allowed = false; // does this bonus work when unarmed?
      melee_allowed = false; // what about with a melee weapon?

      min_melee = 0; // minimum amount of unarmed to trigger this technique
      min_unarmed = 0; // etc
      min_bashing = 0;
      min_cutting = 0;
      min_stabbing = 0;
    }

    bool is_valid_player(player& u);

};

class ma_technique {
  public:
    ma_technique();

    std::string id;

    std::string goal; // the melee goal this achieves

    // given a player's state, does this bonus apply to him?
    bool is_valid_player(player& u);

    std::set<std::string> flags;

    // message to be displayed when player (0) or npc (1) uses the technique
    std::vector<std::string> messages;

    bool defensive;
    bool crit_tec;

    ma_requirements reqs;

    int down_dur;
    int stun_dur;
    int knockback_dist;
    float knockback_spread; // adding randomness to knockback, like tec_throw
    std::string aoe; // corresponds to an aoe shape, defaults to just the target

    // offensive
    bool disarms; // like tec_disarm
    bool grabs; // like tec_grab
    bool counters; // like tec_counter

    bool miss_recovery; // allows free recovery from misses, like tec_feint
    bool grab_break; // allows grab_breaks, like tec_break

    bool flaming; // applies fire effects etc
    bool quick; // moves discount based on attack speed, like tec_rapid

    int hit; // flat bonus to hit
    int bash; // flat bonus to bash
    int cut; // flat bonus to cut
    int pain; // attacks cause pain

    float bash_mult; // bash damage multiplier
    float cut_mult; // cut damage multiplier

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

    // returns the armor bonus for various armor stats (equivalent to armor)
    int arm_bash_bonus(player& u);
    int arm_cut_bonus(player& u);

    // returns the stat bonus for the various damage stats (for rolls)
    int bash_bonus(player& u);
    int cut_bonus(player& u);

    // returns damage multipliers for the various damage stats (applied after
    // bonuses)
    float bash_mult();
    float cut_mult();

    // returns various boolean flags
    bool is_throw_immune();
    bool is_quiet();
    bool can_melee();

    std::string id;
    std::string name;
    std::string description;

    ma_requirements reqs;

    // mapped as buff_id -> min stacks of buff

    int buff_duration; // total length this buff lasts
    int max_stacks; // total number of stacks this buff can have

    int dodges_bonus; // extra dodges, like karate
    int blocks_bonus; // extra blocks, like karate

    int arm_bash; // passive bonus to bash armor
    int arm_cut; // passive bonus to cut armor

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

    bool quiet;
    bool melee_allowed;
    bool throw_immune; // are we immune to throws/grabs?
};

class martialart {
  public:
    martialart();

    // modifies a player's "current" stats with various types of bonuses
    void apply_static_buffs(player& u, std::vector<disease>& dVec);

    void apply_onmove_buffs(player& u, std::vector<disease>& dVec);

    void apply_onhit_buffs(player& u, std::vector<disease>& dVec);

    void apply_onattack_buffs(player& u, std::vector<disease>& dVec);

    void apply_ondodge_buffs(player& u, std::vector<disease>& dVec);

    void apply_onblock_buffs(player& u, std::vector<disease>& dVec);

    void apply_ongethit_buffs(player& u, std::vector<disease>& dVec);

    // determines if a technique is valid or not for this style
    bool has_technique(player& u, matec_id tech);
    // gets custom melee string for a technique under this style
    std::string melee_verb(matec_id tech, player& u);

    std::string id;
    std::string name;
    std::string description;
    int arm_block;
    int leg_block;
    std::set<matec_id> techniques; // all available techniques
    std::vector<ma_buff> static_buffs; // all buffs triggered by each condition
    std::vector<ma_buff> onmove_buffs;
    std::vector<ma_buff> onhit_buffs;
    std::vector<ma_buff> onattack_buffs;
    std::vector<ma_buff> ondodge_buffs;
    std::vector<ma_buff> onblock_buffs;
    std::vector<ma_buff> ongethit_buffs;

};

void load_technique(JsonObject &jo);
void load_martial_art(JsonObject &jo);

void init_martial_arts();

extern std::map<matype_id, martialart> martialarts;
extern std::map<mabuff_id, ma_buff> ma_buffs;
extern std::map<matec_id, ma_technique> ma_techniques;

#endif
