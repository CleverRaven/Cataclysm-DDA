#include "player.h"
#include "game.h"
#include "debug.h"
#include "martialarts.h"
#include "json.h"
#include "translations.h"
#include "itype.h"
#include <map>
#include <string>
#include <algorithm>

std::map<matype_id, martialart> martialarts;
std::map<mabuff_id, ma_buff> ma_buffs;
std::map<matec_id, ma_technique> ma_techniques;

void load_technique(JsonObject &jo)
{
    ma_technique tec;

    tec.id = matec_id( jo.get_string("id") );
    tec.name = jo.get_string("name", "");
    if (!tec.name.empty()) {
        tec.name = _(tec.name.c_str());
    }

    if( jo.has_member( "messages" ) ) {
        JsonArray jsarr = jo.get_array("messages");
        tec.player_message = jsarr.get_string( 0 );
        if( !tec.player_message.empty() ) {
            tec.player_message = _(tec.player_message.c_str());
        }
        tec.npc_message = jsarr.get_string( 1 );
        if( !tec.npc_message.empty() ) {
            tec.npc_message = _(tec.npc_message.c_str());
        }
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

    for( auto & s :jo.get_tags( "req_buffs" ) ) {
        tec.reqs.req_buffs.insert( mabuff_id( s ) );
    }
    tec.reqs.req_flags = jo.get_tags("req_flags");

    tec.crit_tec = jo.get_bool("crit_tec", false);
    tec.defensive = jo.get_bool("defensive", false);
    tec.disarms = jo.get_bool("disarms", false);
    tec.dodge_counter = jo.get_bool("dodge_counter", false);
    tec.block_counter = jo.get_bool("block_counter", false);
    tec.miss_recovery = jo.get_bool("miss_recovery", false);
    tec.grab_break = jo.get_bool("grab_break", false);
    tec.flaming = jo.get_bool("flaming", false);

    tec.hit = jo.get_int("hit", 0);
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

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

template<>
const ma_technique &string_id<ma_technique>::obj() const
{
    const auto iter = ma_techniques.find( *this );
    if( iter == ma_techniques.end() ) {
        debugmsg( "invalid martial art technique id %s", _id.c_str() );
        static const ma_technique dummy;
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<ma_technique>::is_valid() const
{
    return ma_techniques.count( *this ) > 0;
}

mabuff_id load_buff(JsonObject &jo)
{
    ma_buff buff;

    buff.id = mabuff_id( jo.get_string("id") );

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

    for( auto & s :jo.get_tags( "req_buffs" ) ) {
        buff.reqs.req_buffs.insert( mabuff_id( s ) );
    }

    ma_buffs[buff.id] = buff;

    return buff.id;
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

template<>
const ma_buff &string_id<ma_buff>::obj() const
{
    const auto iter = ma_buffs.find( *this );
    if( iter == ma_buffs.end() ) {
        debugmsg( "invalid martial art buff id %s", _id.c_str() );
        static const ma_buff dummy;
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<ma_buff>::is_valid() const
{
    return ma_buffs.count( *this ) > 0;
}

void load_martial_art(JsonObject &jo)
{
    martialart ma;
    JsonArray jsarr;

    ma.id = matype_id( jo.get_string("id") );
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

    for( auto & s :jo.get_tags( "techniques" ) ) {
        ma.techniques.insert( matec_id( s ) );
    }
    ma.weapons = jo.get_tags("weapons");

    ma.leg_block = jo.get_int("leg_block", 99);
    ma.arm_block = jo.get_int("arm_block", 99);

    ma.arm_block_with_bio_armor_arms = jo.get_bool("arm_block_with_bio_armor_arms", false);
    ma.leg_block_with_bio_armor_legs = jo.get_bool("leg_block_with_bio_armor_legs", false);

    martialarts[ma.id] = ma;
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<martialart>::id() const;

template<>
const martialart &string_id<martialart>::obj() const
{
    const auto iter = martialarts.find( *this );
    if( iter == martialarts.end() ) {
        debugmsg( "invalid martial art id %s", _id.c_str() );
        static const martialart dummy;
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<martialart>::is_valid() const
{
    return martialarts.count( *this ) > 0;
}

std::vector<matype_id> all_martialart_types()
{
    std::vector<matype_id> result;
    for( auto & e : martialarts ) {
        result.push_back( e.first );
    }
    return result;
}

void check( const ma_requirements & req, const std::string &display_text )
{
    for( auto & r : req.req_buffs ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text.c_str() );
        }
    }
}

void check_martialarts()
{
    for( auto style = martialarts.cbegin(); style != martialarts.cend(); ++style ) {
        for( auto technique = style->second.techniques.cbegin();
             technique != style->second.techniques.cend(); ++technique ) {
            if( !technique->is_valid() ) {
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
    for( auto & t : ma_techniques ) {
        ::check( t.second.reqs, string_format( "technique %s", t.first.c_str() ) );
    }
    for( auto & b : ma_buffs ) {
        ::check( b.second.reqs, string_format( "buff %s", b.first.c_str() ) );
    }
}

/**
 * This is a wrapper class to get access to the protected members of effect_type, it creates
 * the @ref effect_type that is used to store a @ref ma_buff in the creatures effect map.
 * Note: this class must not contain any new members, it will be converted to a plain
 * effect_type later and that would slice the new members of.
 */
class ma_buff_effect_type : public effect_type {
public:
    ma_buff_effect_type( const ma_buff &buff ) {
        // Unused members of effect_type are commented out:
        id = buff.get_effect_id();
        max_intensity = buff.max_stacks;
        // add_effect add the duration to an existing effect, but it must never be
        // above buff_duration, this keeps the old ma_buff behavior
        max_duration = buff.buff_duration;
        dur_add_perc = 1;
        // each add_effect call increases the intensity by 1
        int_add_val = 1;
        // effect intensity increases by -1 each turn.
        int_decay_step = -1;
        int_decay_tick = 1;
//        int int_dur_factor;
//        bool main_parts_only;
//        std::string resist_trait;
//        std::string resist_effect;
//        std::vector<std::string> removes_effects;
//        std::vector<std::string> blocks_effects;
//        std::vector<std::pair<std::string, int>> miss_msgs;
//        bool pain_sizing;
//        bool hurt_sizing;
//        bool harmful_cough;
//        bool pkill_addict_reduces;
        name.push_back( buff.name );
//        std::string speed_mod_name;
        desc.push_back( buff.description );
//        std::vector<std::string> reduced_desc;
//        bool part_descs;
//        std::vector<std::pair<std::string, game_message_type>> decay_msgs;
        rating = e_good;
//        std::string apply_message;
//        std::string apply_memorial_log;
//        std::string remove_message;
//        std::string remove_memorial_log;
//        std::unordered_map<std::tuple<std::string, bool, std::string, std::string>, double> mod_data;
    }
};

void finialize_martial_arts()
{
    // This adds an effect type for each ma_buff, so we can later refer to it and don't need a
    // redundant definition of those effects in json.
    for( auto &buff : ma_buffs ) {
        const ma_buff_effect_type new_eff( buff.second );
        // Note the slicing here: new_eff is converted to a plain effect_type, but this doesn't
        // bother us because ma_buff_effect_type does not have any members that can be sliced.
        effect_types[new_eff.id] = new_eff;
    }
}

void clear_techniques_and_martial_arts()
{
    martialarts.clear();
    ma_buffs.clear();
    ma_techniques.clear();
}

bool ma_requirements::is_valid_player( const player &u ) const
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
                  (u.has_weapon() && u.style_selected.obj().has_weapon(u.weapon.type->id) &&
                   is_valid_weapon(u.weapon))) &&
                   // TODO: same list as in player.cpp
                   ///\EFFECT_MELEE determines which melee martial arts are available
                 ((u.get_skill_level(skill_id("melee")) >= min_melee &&
                   ///\EFFECT_UNARMED determines which unarmed martial arts are available
                   u.get_skill_level(skill_id("unarmed")) >= min_unarmed &&
                   ///\EFFECT_BASHING determines which bashing martial arts are available
                   u.get_skill_level(skill_id("bashing")) >= min_bashing &&
                   ///\EFFECT_CUTTING determines which cutting martial arts are available
                   u.get_skill_level(skill_id("cutting")) >= min_cutting &&
                   ///\EFFECT_STABBING determines which stabbing martial arts are available
                   u.get_skill_level(skill_id("stabbing")) >= min_stabbing) || cqb);

    return valid;
}

bool ma_requirements::is_valid_weapon( const item &i ) const
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

bool ma_technique::is_valid_player( const player &u ) const
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

std::string ma_buff::get_effect_id() const
{
    return std::string( "mabuff:" ) + id.str();
}

const ma_buff *ma_buff::from_effect( const effect &eff )
{
    const std::string &id = eff.get_effect_type()->id;
    // Same as in get_effect_id!
    if( id.compare( 0, 7, "mabuff:" ) != 0 ) {
        return nullptr;
    }
    return &mabuff_id( id.substr( 7 ) ).obj();
}

void ma_buff::apply_buff( player &u ) const
{
    u.add_effect( get_effect_id(), buff_duration );
}

bool ma_buff::is_valid_player( const player &u ) const
{
    return reqs.is_valid_player(u);
}

void ma_buff::apply_player(player &u) const
{
    u.dodges_left += dodges_bonus;
    u.blocks_left += blocks_bonus;
}

int ma_buff::hit_bonus( const player &u ) const
{
    ///\EFFECT_STR increases martial arts hit bonus
    return hit + u.str_cur * hit_str +
    ///\EFFECT_DEX increases martial arts hit bonus
           u.dex_cur * hit_dex +
    ///\EFFECT_INT increases martial arts hit bonus
           u.int_cur * hit_int +
    ///\EFFECT_PER increases martial arts hit bonus
           u.per_cur * hit_per;
}
int ma_buff::dodge_bonus( const player &u ) const
{
    ///\EFFECT_STR increases martial arts dodge bonus
    return dodge + u.str_cur * dodge_str +
    ///\EFFECT_DEX increases martial arts dodge bonus
           u.dex_cur * dodge_dex +
    ///\EFFECT_INT increases martial arts dodge bonus
           u.int_cur * dodge_int +
    ///\EFFECT_PER increases martial arts dodge bonus
           u.per_cur * dodge_per;
}
int ma_buff::block_bonus( const player &u ) const
{
    ///\EFFECT_STR increases martial arts block bonus
    return block + u.str_cur * block_str +
    ///\EFFECT_DEX increases martial arts block bonus
           u.dex_cur * block_dex +
    ///\EFFECT_INT increases martial arts block bonus
           u.int_cur * block_int +
    ///\EFFECT_PER increases martial arts block bonus
           u.per_cur * block_per;
}
int ma_buff::speed_bonus( const player &u ) const
{
    (void)u; //unused
    return speed;
}
int ma_buff::arm_bash_bonus( const player &u ) const
{
    (void)u; //unused
    return arm_bash;
}
int ma_buff::arm_cut_bonus( const player &u ) const
{
    (void)u; //unused
    return arm_cut;
}
float ma_buff::bash_mult() const
{
    return bash_stat_mult;
}
int ma_buff::bash_bonus( const player &u ) const
{
    ///\EFFECT_STR increases martial arts bash bonus
    return bash + u.str_cur * bash_str +
    ///\EFFECT_DEX increases martial arts bash bonus
           u.dex_cur * bash_dex +
    ///\EFFECT_INT increases martial arts bash bonus
           u.int_cur * bash_int +
    ///\EFFECT_PER increases martial arts bash bonus
           u.per_cur * bash_per;
}
float ma_buff::cut_mult() const
{
    return cut_stat_mult;
}
int ma_buff::cut_bonus( const player &u ) const
{
    ///\EFFECT_STR increases martial arts cut bonus
    return cut + u.str_cur * cut_str +
    ///\EFFECT_DEX increases martial arts cut bonus
           u.dex_cur * cut_dex +
    ///\EFFECT_INT increases martial arts cut bonus
           u.int_cur * cut_int +
    ///\EFFECT_PER increases martial arts cut bonus
           u.per_cur * cut_per;
}
bool ma_buff::is_throw_immune() const
{
    return throw_immune;
}
bool ma_buff::is_quiet() const
{
    return quiet;
}

bool ma_buff::can_melee() const
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
void simultaneous_add(player &u, const std::vector<mabuff_id> &buffs)
{
    std::vector<const ma_buff*> buffer; // hey get it because it's for buffs????
    for( auto &buffid : buffs ) {
        const ma_buff &buff = buffid.obj();
        if( buff.is_valid_player( u ) ) {
            buffer.push_back( &buff );
        }
    }
    for( auto &elem : buffer ) {
        elem->apply_buff( u );
    }
}

void martialart::apply_static_buffs(player &u) const
{
    simultaneous_add(u, static_buffs);
}

void martialart::apply_onmove_buffs(player &u) const
{
    simultaneous_add(u, onmove_buffs);
}

void martialart::apply_onhit_buffs(player &u) const
{
    simultaneous_add(u, onhit_buffs);
}

void martialart::apply_onattack_buffs(player &u) const
{
    simultaneous_add(u, onattack_buffs);
}

void martialart::apply_ondodge_buffs(player &u) const
{
    simultaneous_add(u, ondodge_buffs);
}

void martialart::apply_onblock_buffs(player &u) const
{
    simultaneous_add(u, onblock_buffs);
}

void martialart::apply_ongethit_buffs(player &u) const
{
    simultaneous_add(u, ongethit_buffs);
}


bool martialart::has_technique( const player &u , matec_id tec_id) const
{
    for( const auto &elem : techniques ) {
        const ma_technique &tec = elem.obj();
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

// Player stuff

// technique
std::vector<matec_id> player::get_all_techniques() const
{
    std::vector<matec_id> tecs;
    // Grab individual item techniques
    const auto &weapon_techs = weapon.get_techniques();
    tecs.insert( tecs.end(), weapon_techs.begin(), weapon_techs.end() );
    // and martial art techniques
    const auto &style = style_selected.obj();
    tecs.insert( tecs.end(), style.techniques.begin(), style.techniques.end() );

    return tecs;
}

// defensive technique-related
bool player::has_miss_recovery_tec() const
{
    for( auto &technique : get_all_techniques() ) {
        if( technique.obj().miss_recovery ) {
            return true;
        }
    }
    return false;
}

bool player::has_grab_break_tec() const
{
    for( auto &technique : get_all_techniques() ) {
        if( technique.obj().grab_break ) {
            return true;
        }
    }
    return false;
}

bool player::can_leg_block() const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform leg block
    int unarmed_skill = has_active_bionic("bio_cqb") ? 5 : (int)get_skill_level(skill_id("unarmed"));

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

bool player::can_arm_block() const
{
    const martialart &ma = style_selected.obj();
    ///\EFFECT_UNARMED increases ability to perform arm block
    int unarmed_skill = has_active_bionic("bio_cqb") ? 5 : (int)get_skill_level(skill_id("unarmed"));

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

bool player::can_limb_block() const
{
    return can_arm_block() || can_leg_block();
}

// event handlers
void player::ma_static_effects()
{
    style_selected.obj().apply_static_buffs(*this);
}
void player::ma_onmove_effects()
{
    style_selected.obj().apply_onmove_buffs(*this);
}
void player::ma_onhit_effects()
{
    style_selected.obj().apply_onhit_buffs(*this);
}
void player::ma_onattack_effects()
{
    style_selected.obj().apply_onattack_buffs(*this);
}
void player::ma_ondodge_effects()
{
    style_selected.obj().apply_ondodge_buffs(*this);
}
void player::ma_onblock_effects()
{
    style_selected.obj().apply_onblock_buffs(*this);
}
void player::ma_ongethit_effects()
{
    style_selected.obj().apply_ongethit_buffs(*this);
}

template<typename C, typename F>
static void accumulate_ma_buff_effects( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            if( auto buff = ma_buff::from_effect( eff.second ) ) {
                f( *buff, eff.second );
            }
        }
    }
}

template<typename C, typename F>
static bool search_ma_buff_effect( const C &container, F f )
{
    for( auto &elem : container ) {
        for( auto &eff : elem.second ) {
            if( auto buff = ma_buff::from_effect( eff.second ) ) {
                if( f( *buff, eff.second ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

// bonuses
int player::mabuff_tohit_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect & ) {
        ret += b.hit_bonus( *this );
    } );
    return ret;
}
int player::mabuff_dodge_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.dodge_bonus( *this );
    } );
    return ret;
}
int player::mabuff_block_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.block_bonus( *this );
    } );
    return ret;
}
int player::mabuff_speed_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.speed_bonus( *this );
    } );
    return ret;
}
int player::mabuff_arm_bash_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.arm_bash_bonus( *this );
    } );
    return ret;
}
int player::mabuff_arm_cut_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.arm_cut_bonus( *this );
    } );
    return ret;
}
float player::mabuff_bash_mult() const
{
    float ret = 1.f;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.bash_mult() - 1 ) + 1;
    } );
    return ret;
}
int player::mabuff_bash_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.bash_bonus( *this );
    } );
    return ret;
}
float player::mabuff_cut_mult() const
{
    float ret = 1.f;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.cut_mult() - 1 ) + 1;
    } );
    return ret;
}
int player::mabuff_cut_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( effects, [&ret, this]( const ma_buff &b, const effect &d ) {
        ret += d.get_intensity() * b.cut_bonus( *this );
    } );
    return ret;
}
bool player::is_throw_immune() const
{
    return search_ma_buff_effect( effects, []( const ma_buff &b, const effect & ) {
        return b.is_throw_immune();
    } );
}
bool player::is_quiet() const
{
    return search_ma_buff_effect( effects, []( const ma_buff &b, const effect & ) {
        return b.is_quiet();
    } );
}

bool player::can_melee() const
{
    return search_ma_buff_effect( effects, []( const ma_buff &b, const effect & ) {
        return b.can_melee();
    } );
}

bool player::has_mabuff(mabuff_id id) const
{
    return search_ma_buff_effect( effects, [&id]( const ma_buff &b, const effect & ) {
        return b.id == id;
    } );
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
