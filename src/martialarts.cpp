#include "player.h"
#include "game.h"
#include "martialarts.h"
#include "json.h"
#include "translations.h"
#include <map>
#include <string>
#include <algorithm>

std::map<matype_id, martialart> martialarts;
std::map<mabuff_id, ma_buff> ma_buffs;
std::map<matec_id, ma_technique> ma_techniques;

void load_technique(JsonObject &jo)
{
    ma_technique tec;

    tec.id = jo.get_string("id");
    tec.name = jo.get_string("name", "");
    if (!tec.name.empty()) {
        tec.name = _(tec.name.c_str());
    }

    JsonArray jsarr = jo.get_array("messages");
    while (jsarr.has_more()) {
        tec.messages.push_back(_(jsarr.next_string().c_str()));
    }

    tec.reqs.unarmed_allowed = jo.get_bool("unarmed_allowed", false);
    tec.reqs.melee_allowed = jo.get_bool("melee_allowed", false);
    tec.reqs.min_melee = jo.get_int("min_melee", 0);
    tec.reqs.min_unarmed = jo.get_int("min_unarmed", 0);

    tec.reqs.min_bashing = jo.get_int("min_bashing", 0);
    tec.reqs.min_cutting = jo.get_int("min_cutting", 0);
    tec.reqs.min_stabbing = jo.get_int("min_stabbing", 0);

    tec.reqs.min_bashing_damage = jo.get_int("min_bashing_damage", 0);
    tec.reqs.min_cutting_damage = jo.get_int("min_cutting_damage", 0);

    tec.reqs.req_buffs = jo.get_tags("req_buffs");
    tec.reqs.req_flags = jo.get_tags("req_flags");

    tec.crit_tec = jo.get_bool("crit_tec", false);
    tec.defensive = jo.get_bool("defensive", false);
    tec.disarms = jo.get_bool("disarms", false);
    tec.dodge_counter = jo.get_bool("dodge_counter", false);
    tec.block_counter = jo.get_bool("block_counter", false);
    tec.miss_recovery = jo.get_bool("miss_recovery", false);
    tec.grab_break = jo.get_bool("grab_break", false);
    tec.flaming = jo.get_bool("flaming", false);

    tec.hit = jo.get_int("pain", 0);
    tec.bash = jo.get_int("bash", 0);
    tec.cut = jo.get_int("cut", 0);
    tec.pain = jo.get_int("pain", 0);

    tec.weighting = jo.get_int("weighting", 1);

    tec.bash_mult = jo.get_float("bash_mult", 1.0);
    tec.cut_mult = jo.get_float("cut_mult", 1.0);
    tec.speed_mult = jo.get_float("speed_mult", 1.0);

    tec.down_dur = jo.get_int("down_dur", 0);
    tec.stun_dur = jo.get_int("stun_dur", 0);
    tec.knockback_dist = jo.get_int("knockback_dist", 0);
    tec.knockback_spread = jo.get_int("knockback_spread", 0);

    tec.aoe = jo.get_string("aoe", "");
    tec.flags = jo.get_tags("flags");

    ma_techniques[tec.id] = tec;
}

ma_buff load_buff(JsonObject &jo)
{
    ma_buff buff;

    buff.id = jo.get_string("id");

    buff.name = _(jo.get_string("name").c_str());
    buff.description = _(jo.get_string("description").c_str());

    buff.buff_duration = jo.get_int("buff_duration", 2);
    buff.max_stacks = jo.get_int("max_stacks", 1);

    buff.reqs.unarmed_allowed = jo.get_bool("unarmed_allowed", false);
    buff.reqs.melee_allowed = jo.get_bool("melee_allowed", false);

    buff.reqs.min_melee = jo.get_int("min_melee", 0);
    buff.reqs.min_unarmed = jo.get_int("min_unarmed", 0);

    buff.dodges_bonus = jo.get_int("bonus_dodges", 0);
    buff.blocks_bonus = jo.get_int("bonus_blocks", 0);

    buff.hit = jo.get_int("hit", 0);
    buff.bash = jo.get_int("bash", 0);
    buff.cut = jo.get_int("cut", 0);
    buff.dodge = jo.get_int("dodge", 0);
    buff.speed = jo.get_int("speed", 0);
    buff.block = jo.get_int("block", 0);

    buff.arm_bash = jo.get_int("arm_bash", 0);
    buff.arm_cut = jo.get_int("arm_cut", 0);

    buff.bash_stat_mult = jo.get_float("bash_mult", 1.0);
    buff.cut_stat_mult = jo.get_float("cut_mult", 1.0);

    buff.hit_str = jo.get_float("hit_str", 0.0);
    buff.hit_dex = jo.get_float("hit_dex", 0.0);
    buff.hit_int = jo.get_float("hit_int", 0.0);
    buff.hit_per = jo.get_float("hit_per", 0.0);

    buff.bash_str = jo.get_float("bash_str", 0.0);
    buff.bash_dex = jo.get_float("bash_dex", 0.0);
    buff.bash_int = jo.get_float("bash_int", 0.0);
    buff.bash_per = jo.get_float("bash_per", 0.0);

    buff.cut_str = jo.get_float("cut_str", 0.0);
    buff.cut_dex = jo.get_float("cut_dex", 0.0);
    buff.cut_int = jo.get_float("cut_int", 0.0);
    buff.cut_per = jo.get_float("cut_per", 0.0);

    buff.dodge_str = jo.get_float("dodge_str", 0.0);
    buff.dodge_dex = jo.get_float("dodge_dex", 0.0);
    buff.dodge_int = jo.get_float("dodge_int", 0.0);
    buff.dodge_per = jo.get_float("dodge_per", 0.0);

    buff.block_str = jo.get_float("block_str", 0.0);
    buff.block_dex = jo.get_float("block_dex", 0.0);
    buff.block_int = jo.get_float("block_int", 0.0);
    buff.block_per = jo.get_float("block_per", 0.0);

    buff.quiet = jo.get_bool("quiet", false);
    buff.throw_immune = jo.get_bool("throw_immune", false);

    buff.reqs.req_buffs = jo.get_tags("req_buffs");

    ma_buffs[buff.id] = buff;

    return buff;
}

void load_martial_art(JsonObject &jo)
{
    martialart ma;
    JsonArray jsarr;

    ma.id = jo.get_string("id");
    ma.name = _(jo.get_string("name").c_str());
    ma.description = _(jo.get_string("description").c_str());

    jsarr = jo.get_array("static_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.static_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onmove_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onmove_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onhit_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onhit_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onattack_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onattack_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("ondodge_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.ondodge_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("onblock_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onblock_buffs.push_back(load_buff(jsobj));
    }

    jsarr = jo.get_array("ongethit_buffs");
    while (jsarr.has_more()) {
        JsonObject jsobj = jsarr.next_object();
        ma.onblock_buffs.push_back(load_buff(jsobj));
    }

    ma.techniques = jo.get_tags("techniques");
    ma.weapons = jo.get_tags("weapons");

    ma.leg_block = jo.get_int("leg_block", -1);
    ma.arm_block = jo.get_int("arm_block", -1);

    ma.arm_block_with_bio_armor_arms = jo.get_bool("arm_block_with_bio_armor_arms", false);
    ma.leg_block_with_bio_armor_legs = jo.get_bool("leg_block_with_bio_armor_legs", false);

    martialarts[ma.id] = ma;
}

void check_martialarts()
{
    for( auto style = martialarts.cbegin(); style != martialarts.cend(); ++style ) {
        for( auto technique = style->second.techniques.cbegin();
             technique != style->second.techniques.cend(); ++technique ) {
            if( ma_techniques.find( *technique ) == ma_techniques.end() ) {
                debugmsg( "Technique with id %s in style %s doesn't exist.",
                          technique->c_str(), style->second.name.c_str() );
            }
        }
        for( auto weapon = style->second.weapons.cbegin();
             weapon != style->second.weapons.cend(); ++weapon ) {
            if( !item::type_is_defined( *weapon ) ) {
                debugmsg( "Weapon %s in style %s doesn't exist.",
                          weapon->c_str(), style->second.name.c_str() );
            }
        }
    }
}

void clear_techniques_and_martial_arts()
{
    martialarts.clear();
    ma_buffs.clear();
    ma_techniques.clear();
}

bool ma_requirements::is_valid_player(player &u)
{
    for( auto buff_id : req_buffs ) {

        if (!u.has_mabuff(buff_id)) {
            return false;
        }
    }

    //A technique is valid if it applies to unarmed strikes, if it applies generally
    //to all weapons (such as Ninjutsu sneak attacks or innate weapon techniques like RAPID)
    //or if the weapon is flagged as being compatible with the style. Some techniques have
    //further restrictions on required weapon properties (is_valid_weapon).
    bool cqb = u.has_active_bionic("bio_cqb");
    bool valid = ((unarmed_allowed && u.unarmed_attack()) ||
                  (melee_allowed && !u.unarmed_attack() && is_valid_weapon(u.weapon)) ||
                  (u.has_weapon() && martialarts[u.style_selected].has_weapon(u.weapon.type->id) &&
                   is_valid_weapon(u.weapon))) &&
                 ((u.skillLevel("melee") >= min_melee &&
                   u.skillLevel("unarmed") >= min_unarmed &&
                   u.skillLevel("bashing") >= min_bashing &&
                   u.skillLevel("cutting") >= min_cutting &&
                   u.skillLevel("stabbing") >= min_stabbing) || cqb);

    return valid;
}

bool ma_requirements::is_valid_weapon(item &i)
{
    for( auto flag : req_flags ) {

        if (!i.has_flag(flag)) {
            return false;
        }
    }
    bool valid = i.damage_bash() >= min_bashing_damage
                 && i.damage_cut() >= min_cutting_damage;

    return valid;
}

ma_technique::ma_technique()
{

    crit_tec = false;
    defensive = false;
    dummy = false;

    down_dur = 0;
    stun_dur = 0;
    knockback_dist = 0;
    knockback_spread = 0; // adding randomness to knockback, like tec_throw

    // offensive
    disarms = false; // like tec_disarm
    dodge_counter = false; // like tec_grab
    block_counter = false; // like tec_counter

    miss_recovery = false; // allows free recovery from misses, like tec_feint
    grab_break = false; // allows grab_breaks, like tec_break

    flaming = false; // applies fire effects etc

    hit = 0; // flat bonus to hit
    bash = 0; // flat bonus to bash
    cut = 0; // flat bonus to cut
    pain = 0; // causes pain

    bash_mult = 1.0f; // bash damage multiplier
    cut_mult = 1.0f; // cut damage multiplier
    speed_mult = 1.0f; // speed multiplier (fractional is faster, old rapid aka quick = 0.5)
}

bool ma_technique::is_valid_player(player &u)
{
    return reqs.is_valid_player(u);
}




ma_buff::ma_buff()
{

    buff_duration = 2; // total length this buff lasts
    max_stacks = 1; // total number of stacks this buff can have

    dodges_bonus = 0; // extra dodges, like karate
    blocks_bonus = 0; // extra blocks, like karate

    hit = 0; // flat bonus to hit
    bash = 0; // flat bonus to bash
    cut = 0; // flat bonus to cut
    dodge = 0; // flat dodge bonus
    speed = 0; // flat speed bonus

    bash_stat_mult = 1.f; // bash damage multiplier, like aikido
    cut_stat_mult = 1.f; // cut damage multiplier

    bash_str = 0.f; // bonus damage to add per str point
    bash_dex = 0.f; // "" dex point
    bash_int = 0.f; // "" int point
    bash_per = 0.f; // "" per point

    cut_str = 0.f; // bonus cut damage to add per str point
    cut_dex = 0.f; // "" dex point
    cut_int = 0.f; // "" int point
    cut_per = 0.f; // "" per point

    hit_str = 0.f; // bonus to-hit to add per str point
    hit_dex = 0.f; // "" dex point
    hit_int = 0.f; // "" int point
    hit_per = 0.f; // "" per point

    dodge_str = 0.f; // bonus dodge to add per str point
    dodge_dex = 0.f; // "" dex point
    dodge_int = 0.f; // "" int point
    dodge_per = 0.f; // "" per point

    throw_immune = false;

}

void ma_buff::apply_buff(std::list<disease> &dVec)
{
    for( auto &elem : dVec ) {
        if( elem.is_mabuff() && elem.buff_id == id ) {
            elem.duration = buff_duration;
            elem.intensity++;
            if( elem.intensity > max_stacks ) {
                elem.intensity = max_stacks;
            }
            return;
        }
    }
    disease d(id);
    d.duration = buff_duration;
    d.intensity = 1;
    d.permanent = false;
    dVec.push_back(d);
}

bool ma_buff::is_valid_player(player &u)
{
    return reqs.is_valid_player(u);
}

void ma_buff::apply_player(player &u)
{
    u.dodges_left += dodges_bonus;
    u.blocks_left += blocks_bonus;
}

int ma_buff::hit_bonus(player &u)
{
    return hit + u.str_cur * hit_str +
           u.dex_cur * hit_dex +
           u.int_cur * hit_int +
           u.per_cur * hit_per;
}
int ma_buff::dodge_bonus(player &u)
{
    return dodge + u.str_cur * dodge_str +
           u.dex_cur * dodge_dex +
           u.int_cur * dodge_int +
           u.per_cur * dodge_per;
}
int ma_buff::block_bonus(player &u)
{
    return block + u.str_cur * block_str +
           u.dex_cur * block_dex +
           u.int_cur * block_int +
           u.per_cur * block_per;
}
int ma_buff::speed_bonus(player &u)
{
    (void)u; //unused
    return speed;
}
int ma_buff::arm_bash_bonus(player &u)
{
    (void)u; //unused
    return arm_bash;
}
int ma_buff::arm_cut_bonus(player &u)
{
    (void)u; //unused
    return arm_cut;
}
float ma_buff::bash_mult()
{
    return bash_stat_mult;
}
int ma_buff::bash_bonus(player &u)
{
    return bash + u.str_cur * bash_str +
           u.dex_cur * bash_dex +
           u.int_cur * bash_int +
           u.per_cur * bash_per;
}
float ma_buff::cut_mult()
{
    return cut_stat_mult;
}
int ma_buff::cut_bonus(player &u)
{
    return cut + u.str_cur * cut_str +
           u.dex_cur * cut_dex +
           u.int_cur * cut_int +
           u.per_cur * cut_per;
}
bool ma_buff::is_throw_immune()
{
    return throw_immune;
}
bool ma_buff::is_quiet()
{
    return quiet;
}

bool ma_buff::can_melee()
{
    return melee_allowed;
}



martialart::martialart()
{
    leg_block = -1;
    arm_block = -1;
}

// simultaneously check and add all buffs. this is so that buffs that have
// buff dependencies added by the same event trigger correctly
void simultaneous_add(player &u, std::vector<ma_buff> &buffs, std::list<disease> &dVec)
{
    std::vector<ma_buff> buffer; // hey get it because it's for buffs????
    for( auto &buff : buffs ) {
        if( buff.is_valid_player( u ) ) {
            buffer.push_back( buff );
        }
    }
    for( auto &elem : buffer ) {
        elem.apply_buff( dVec );
    }
}

void martialart::apply_static_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, static_buffs, dVec);
}

void martialart::apply_onmove_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, onmove_buffs, dVec);
}

void martialart::apply_onhit_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, onhit_buffs, dVec);
}

void martialart::apply_onattack_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, onattack_buffs, dVec);
}

void martialart::apply_ondodge_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, ondodge_buffs, dVec);
}

void martialart::apply_onblock_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, onblock_buffs, dVec);
}

void martialart::apply_ongethit_buffs(player &u, std::list<disease> &dVec)
{
    simultaneous_add(u, ongethit_buffs, dVec);
}


bool martialart::has_technique(player &u, matec_id tec_id)
{
    for( const auto &elem : techniques ) {
        ma_technique tec = ma_techniques[elem];
        if (tec.is_valid_player(u) && tec.id == tec_id) {
            return true;
        }
    }
    return false;
}

bool martialart::has_weapon(itype_id item) const
{
    return weapons.count(item);
}

std::string martialart::melee_verb(matec_id tec_id, player &u)
{
    for( const auto &elem : techniques ) {
        ma_technique tec = ma_techniques[elem];
        if (tec.id == tec_id) {
            if (u.is_npc()) {
                return tec.messages[1];
            } else {
                return tec.messages[0];
            }
        }
    }
    return std::string("%s is attacked by bugs");
}

// Player stuff

// technique
std::vector<matec_id> player::get_all_techniques()
{
    std::vector<matec_id> tecs;
    tecs.insert(tecs.end(), weapon.type->techniques.begin(), weapon.type->techniques.end());
    tecs.insert(tecs.end(), martialarts[style_selected].techniques.begin(),
                martialarts[style_selected].techniques.end());

    return tecs;
}

// defensive technique-related
bool player::has_miss_recovery_tec()
{
    std::vector<matec_id> techniques = get_all_techniques();
    for( auto &technique : techniques ) {
        if( ma_techniques[technique].miss_recovery == true ) {
            return true;
        }
    }
    return false;
}

bool player::has_grab_break_tec()
{
    std::vector<matec_id> techniques = get_all_techniques();
    for( auto &technique : techniques ) {
        if( ma_techniques[technique].grab_break == true ) {
            return true;
        }
    }
    return false;
}

bool player::can_leg_block()
{
    martialart ma = martialarts[style_selected];
    int unarmed_skill = has_active_bionic("bio_cqb") ? 5 : (int)skillLevel("unarmed");

    // Success conditions.
    if(hp_cur[hp_leg_l] > 0 || hp_cur[hp_leg_r] > 0) {
        if( unarmed_skill >= ma.leg_block ) {
            return true;
        } else if( ma.leg_block_with_bio_armor_legs && has_bionic("bio_armor_legs") ) {
            return true;
        }
    }
    // if not above, can't block.
    return false;
}

bool player::can_arm_block()
{
    martialart ma = martialarts[style_selected];
    int unarmed_skill = has_active_bionic("bio_cqb") ? 5 : (int)skillLevel("unarmed");

    // Success conditions.
    if (hp_cur[hp_arm_l] > 0 || hp_cur[hp_arm_r] > 0) {
        if( unarmed_skill >= ma.arm_block ) {
            return true;
        } else if( ma.arm_block_with_bio_armor_arms && has_bionic("bio_armor_arms") ) {
            return true;
        }
    }
    // if not above, can't block.
    return false;
}

bool player::can_limb_block()
{
    return can_arm_block() || can_leg_block();
}

// event handlers
void player::ma_static_effects()
{
    martialarts[style_selected].apply_static_buffs(*this, illness);
}
void player::ma_onmove_effects()
{
    martialarts[style_selected].apply_onmove_buffs(*this, illness);
}
void player::ma_onhit_effects()
{
    martialarts[style_selected].apply_onhit_buffs(*this, illness);
}
void player::ma_onattack_effects()
{
    martialarts[style_selected].apply_onattack_buffs(*this, illness);
}
void player::ma_ondodge_effects()
{
    martialarts[style_selected].apply_ondodge_buffs(*this, illness);
}
void player::ma_onblock_effects()
{
    martialarts[style_selected].apply_onblock_buffs(*this, illness);
}
void player::ma_ongethit_effects()
{
    martialarts[style_selected].apply_ongethit_buffs(*this, illness);
}

// bonuses
int player::mabuff_tohit_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += ma_buffs[elem.buff_id].hit_bonus( *this );
        }
    }
    return ret;
}
int player::mabuff_dodge_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].dodge_bonus( *this );
        }
    }
    return ret;
}
int player::mabuff_block_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].block_bonus( *this );
        }
    }
    return ret;
}
int player::mabuff_speed_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].speed_bonus( *this );
        }
    }
    return ret;
}
int player::mabuff_arm_bash_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].arm_bash_bonus( *this );
        }
    }
    return ret;
}
int player::mabuff_arm_cut_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].arm_cut_bonus( *this );
        }
    }
    return ret;
}
float player::mabuff_bash_mult()
{
    float ret = 1.f;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
            // becomes 1.4 instead of 2.4 (which would be a 240% buff)
            ret *= elem.intensity * ( ma_buffs[elem.buff_id].bash_mult() - 1 ) + 1;
        }
    }
    return ret;
}
int player::mabuff_bash_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].bash_bonus( *this );
        }
    }
    return ret;
}
float player::mabuff_cut_mult()
{
    float ret = 1.f;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
            // becomes 1.4 instead of 2.4 (which would be a 240% buff)
            ret *= elem.intensity * ( ma_buffs[elem.buff_id].cut_mult() - 1 ) + 1;
        }
    }
    return ret;
}
int player::mabuff_cut_bonus()
{
    int ret = 0;
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            ret += elem.intensity * ma_buffs[elem.buff_id].cut_bonus( *this );
        }
    }
    return ret;
}
bool player::is_throw_immune()
{
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            if( ma_buffs[elem.buff_id].is_throw_immune() ) {
                return true;
            }
        }
    }
    return false;
}
bool player::is_quiet()
{
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            if( ma_buffs[elem.buff_id].is_quiet() ) {
                return true;
            }
        }
    }
    return false;
}

bool player::can_melee()
{
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && ma_buffs.find( elem.buff_id ) != ma_buffs.end() ) {
            if( ma_buffs[elem.buff_id].can_melee() ) {
                return true;
            }
        }
    }
    return false;
}

bool player::has_mabuff(mabuff_id id)
{
    for( auto &elem : illness ) {
        if( elem.is_mabuff() && elem.buff_id == id ) {
            return true;
        }
    }
    return false;
}

bool player::has_martialart(const matype_id &ma) const
{
    return std::find(ma_styles.begin(), ma_styles.end(), ma) != ma_styles.end();
}

void player::add_martialart(const matype_id &ma_id)
{
    if (has_martialart(ma_id)) {
        return;
    }
    ma_styles.push_back(ma_id);
}
