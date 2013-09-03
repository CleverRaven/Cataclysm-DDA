#ifndef _MARTIALARTS_H_
#define _MARTIALARTS_H_

#include "player.h"
#include "pldata.h"
#include <string>
#include <vector>
#include <map>

typedef std::string matype_id;

typedef std::string mabuff_id;

class technique {
    technique();
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

    // returns the stat bonus for the various damage stats (for rolls)
    int bash_bonus(player& u);
    int cut_bonus(player& u);

    // returns damage multipliers for the various damage stats (applied after
    // bonuses)
    float bash_mult();
    float cut_mult();

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

    int dodges_bonus; // extra dodges, like karate
    int blocks_bonus; // extra blocks, like karate

    int hit; // flat bonus to hit
    int bash; // flat bonus to bash
    int cut; // flat bonus to cut

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
};

class martialart {
  public:
    martialart();

    // modifies a player's "current" stats with all on-move bonuses
    void apply_static_buffs(player& u, std::vector<disease>& dVec);

    void apply_onhit_buffs(player& u, std::vector<disease>& dVec);

    /*
    // modifies a player's "current" stats with all on-move bonuses
    void apply_move_bonus(player& u);

    // "" on-attack bonuses
    void apply_attack_bonus(player& u);

    // "" on-incoming-hit bonuses
    void apply_defend_bonus(player& u);

    // "" on-hit bonuses
    void apply_hit_bonus(player& u);

    // "" on-dodge bonuses
    void apply_dodge_bonus(player& u);
    */

    std::string id;
    std::string name;
    std::string desc;
    std::vector<technique> techniques; // all available techniques
    std::vector<ma_buff> static_buffs;
    std::vector<ma_buff> onatk_buffs;
    std::vector<ma_buff> onhit_buffs;
    std::vector<ma_buff> ondef_buffs;
    std::vector<ma_buff> ondodge_buffs;

};

#endif
