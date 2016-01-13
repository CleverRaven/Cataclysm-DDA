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
#include "generic_factory.h"

std::map<matype_id, martialart> martialarts;
std::map<mabuff_id, ma_buff> ma_buffs;
namespace {
generic_factory<ma_technique> ma_techniques( "martial art technique" );
}

void load_technique(JsonObject &jo)
{
    ma_techniques.load( jo );
}

void ma_technique::load( JsonObject &jo )
{
    optional( jo, was_loaded, "name", name, translated_string_reader );

    if( jo.has_member( "messages" ) ) {
        JsonArray jsarr = jo.get_array("messages");
        player_message = jsarr.get_string( 0 );
        if( !player_message.empty() ) {
            player_message = _(player_message.c_str());
        }
        npc_message = jsarr.get_string( 1 );
        if( !npc_message.empty() ) {
            npc_message = _(npc_message.c_str());
        }
    }

    optional( jo, was_loaded, "unarmed_allowed", reqs.unarmed_allowed, false );
    optional( jo, was_loaded, "melee_allowed", reqs.melee_allowed, false );
    optional( jo, was_loaded, "min_melee", reqs.min_melee, 0 );
    optional( jo, was_loaded, "min_unarmed", reqs.min_unarmed, 0 );

    optional( jo, was_loaded, "min_bashing", reqs.min_bashing, 0 );
    optional( jo, was_loaded, "min_cutting", reqs.min_cutting, 0 );
    optional( jo, was_loaded, "min_stabbing", reqs.min_stabbing, 0 );

    optional( jo, was_loaded, "min_bashing_damage", reqs.min_bashing_damage, 0 );
    optional( jo, was_loaded, "min_cutting_damage", reqs.min_cutting_damage, 0 );

    optional( jo, was_loaded, "req_buffs", reqs.req_buffs, auto_flags_reader<mabuff_id>{} );
    optional( jo, was_loaded, "req_flags", reqs.req_flags, auto_flags_reader<>{} );

    optional( jo, was_loaded, "crit_tec", crit_tec, false );
    optional( jo, was_loaded, "defensive", defensive, false );
    optional( jo, was_loaded, "disarms", disarms, false );
    optional( jo, was_loaded, "dodge_counter", dodge_counter, false );
    optional( jo, was_loaded, "block_counter", block_counter, false );
    optional( jo, was_loaded, "miss_recovery", miss_recovery, false );
    optional( jo, was_loaded, "grab_break", grab_break, false );
    optional( jo, was_loaded, "flaming", flaming, false );

    optional( jo, was_loaded, "hit", hit, 0 );
    optional( jo, was_loaded, "bash", bash, 0 );
    optional( jo, was_loaded, "cut", cut, 0 );
    optional( jo, was_loaded, "pain", pain, 0 );

    optional( jo, was_loaded, "weighting", weighting, 1 );

    optional( jo, was_loaded, "bash_mult", bash_mult, 1.0 );
    optional( jo, was_loaded, "cut_mult", cut_mult, 1.0 );
    optional( jo, was_loaded, "speed_mult", speed_mult, 1.0 );

    optional( jo, was_loaded, "down_dur", down_dur, 0 );
    optional( jo, was_loaded, "stun_dur", stun_dur, 0 );
    optional( jo, was_loaded, "knockback_dist", knockback_dist, 0 );
    optional( jo, was_loaded, "knockback_spread", knockback_spread, 0 );

    optional( jo, was_loaded, "aoe", aoe, "" );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<>{} );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

template<>
const ma_technique &string_id<ma_technique>::obj() const
{
    return ma_techniques.obj( *this );
}

template<>
bool string_id<ma_technique>::is_valid() const
{
    return ma_techniques.is_valid( *this );
}

mabuff_id load_buff(JsonObject &jo)
{
    ma_buff buff;
    buff.id = mabuff_id( jo.get_string("id") );

    buff.load( jo );

    ma_buffs[buff.id] = buff;
    return buff.id;
}

void ma_buff::load( JsonObject &jo )
{
    const bool was_loaded = false;

    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );

    optional( jo, was_loaded, "buff_duration", buff_duration, 2 );
    optional( jo, was_loaded, "max_stacks", max_stacks, 1 );

    optional( jo, was_loaded, "unarmed_allowed", reqs.unarmed_allowed, false );
    optional( jo, was_loaded, "melee_allowed", reqs.melee_allowed, false );

    optional( jo, was_loaded, "min_melee", reqs.min_melee, 0 );
    optional( jo, was_loaded, "min_unarmed", reqs.min_unarmed, 0 );

    optional( jo, was_loaded, "bonus_dodges", dodges_bonus, 0 );
    optional( jo, was_loaded, "bonus_blocks", blocks_bonus, 0 );

    optional( jo, was_loaded, "hit", hit, 0 );
    optional( jo, was_loaded, "bash", bash, 0 );
    optional( jo, was_loaded, "cut", cut, 0 );
    optional( jo, was_loaded, "dodge", dodge, 0 );
    optional( jo, was_loaded, "speed", speed, 0 );
    optional( jo, was_loaded, "block", block, 0 );

    optional( jo, was_loaded, "arm_bash", arm_bash, 0 );
    optional( jo, was_loaded, "arm_cut", arm_cut, 0 );

    optional( jo, was_loaded, "bash_mult", bash_stat_mult, 1.0 );
    optional( jo, was_loaded, "cut_mult", cut_stat_mult, 1.0 );

    optional( jo, was_loaded, "hit_str", hit_str, 0.0 );
    optional( jo, was_loaded, "hit_dex", hit_dex, 0.0 );
    optional( jo, was_loaded, "hit_int", hit_int, 0.0 );
    optional( jo, was_loaded, "hit_per", hit_per, 0.0 );

    optional( jo, was_loaded, "bash_str", bash_str, 0.0 );
    optional( jo, was_loaded, "bash_dex", bash_dex, 0.0 );
    optional( jo, was_loaded, "bash_int", bash_int, 0.0 );
    optional( jo, was_loaded, "bash_per", bash_per, 0.0 );

    optional( jo, was_loaded, "cut_str", cut_str, 0.0 );
    optional( jo, was_loaded, "cut_dex", cut_dex, 0.0 );
    optional( jo, was_loaded, "cut_int", cut_int, 0.0 );
    optional( jo, was_loaded, "cut_per", cut_per, 0.0 );

    optional( jo, was_loaded, "dodge_str", dodge_str, 0.0 );
    optional( jo, was_loaded, "dodge_dex", dodge_dex, 0.0 );
    optional( jo, was_loaded, "dodge_int", dodge_int, 0.0 );
    optional( jo, was_loaded, "dodge_per", dodge_per, 0.0 );

    optional( jo, was_loaded, "block_str", block_str, 0.0 );
    optional( jo, was_loaded, "block_dex", block_dex, 0.0 );
    optional( jo, was_loaded, "block_int", block_int, 0.0 );
    optional( jo, was_loaded, "block_per", block_per, 0.0 );

    optional( jo, was_loaded, "quiet", quiet, false );
    optional( jo, was_loaded, "throw_immune", throw_immune, false );

    optional( jo, was_loaded, "req_buffs", reqs.req_buffs, auto_flags_reader<mabuff_id>{} );
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
    ma.id = matype_id( jo.get_string("id") );
    ma.load( jo );

    martialarts[ma.id] = ma;
}

class ma_buff_reader : public generic_typed_reader<mabuff_id>
{
    private:
        mabuff_id get_next( JsonIn &jin ) const override {
            if( jin.test_string() ) {
                return mabuff_id( jin.get_string() );
    }
            JsonObject jsobj = jin.get_object();
            return load_buff( jsobj );
    }
};

void martialart::load( JsonObject &jo )
{
    const bool was_loaded = false;
    JsonArray jsarr;

    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );

    optional( jo, was_loaded, "static_buffs", static_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onmove_buffs", onmove_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onhit_buffs", onhit_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onattack_buffs", onattack_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ondodge_buffs", ondodge_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onblock_buffs", onblock_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ongethit_buffs", ongethit_buffs, ma_buff_reader{} );

    optional( jo, was_loaded, "techniques", techniques, auto_flags_reader<matec_id>{} );
    optional( jo, was_loaded, "weapons", weapons, auto_flags_reader<itype_id>{} );

    optional( jo, was_loaded, "leg_block", leg_block, 99 );
    optional( jo, was_loaded, "arm_block", arm_block, 99 );

    optional( jo, was_loaded, "arm_block_with_bio_armor_arms", arm_block_with_bio_armor_arms, false );
    optional( jo, was_loaded, "leg_block_with_bio_armor_legs", leg_block_with_bio_armor_legs, false );
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
    for( auto & t : ma_techniques.all_ref() ) {
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
    ma_techniques.reset();
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
