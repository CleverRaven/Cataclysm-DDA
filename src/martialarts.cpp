#include "martialarts.h"
#include "player.h"
#include "debug.h"
#include "effect.h"
#include "json.h"
#include "translations.h"
#include "itype.h"
#include "damage.h"

#include <map>
#include <string>
#include <algorithm>
#include "generic_factory.h"
#include "string_formatter.h"

const skill_id skill_melee( "melee" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_unarmed( "unarmed" );

namespace
{
generic_factory<ma_technique> ma_techniques( "martial art technique" );
generic_factory<martialart> martialarts( "martial art style" );
generic_factory<ma_buff> ma_buffs( "martial art buff" );
}

void load_technique( JsonObject &jo, const std::string &src )
{
    ma_techniques.load( jo, src );
}

// To avoid adding empty entries
template <typename Container>
void add_if_exists( JsonObject &jo, Container &cont, bool was_loaded,
                    const std::string &json_key, const typename Container::key_type &id )
{
    if( jo.has_member( json_key ) ) {
        mandatory( jo, was_loaded, json_key, cont[id] );
    }
}

void ma_requirements::load( JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "unarmed_allowed", unarmed_allowed, false );
    optional( jo, was_loaded, "melee_allowed", melee_allowed, false );

    optional( jo, was_loaded, "req_buffs", req_buffs, auto_flags_reader<mabuff_id> {} );
    optional( jo, was_loaded, "req_flags", req_flags, auto_flags_reader<> {} );

    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );

    // TODO: De-hardcode the skills and damage types here
    add_if_exists( jo, min_skill, was_loaded, "min_melee", skill_melee );
    add_if_exists( jo, min_skill, was_loaded, "min_unarmed", skill_unarmed );
    add_if_exists( jo, min_skill, was_loaded, "min_bashing", skill_bashing );
    add_if_exists( jo, min_skill, was_loaded, "min_cutting", skill_cutting );
    add_if_exists( jo, min_skill, was_loaded, "min_stabbing", skill_stabbing );

    add_if_exists( jo, min_damage, was_loaded, "min_bashing_damage", DT_BASH );
    add_if_exists( jo, min_damage, was_loaded, "min_cutting_damage", DT_CUT );
    add_if_exists( jo, min_damage, was_loaded, "min_stabbing_damage", DT_STAB );
}

void ma_technique::load( JsonObject &jo, const std::string &src )
{
    mandatory( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description, "" );

    if( jo.has_member( "messages" ) ) {
        JsonArray jsarr = jo.get_array( "messages" );
        player_message = jsarr.get_string( 0 );
        npc_message = jsarr.get_string( 1 );
    }

    optional( jo, was_loaded, "crit_tec", crit_tec, false );
    optional( jo, was_loaded, "defensive", defensive, false );
    optional( jo, was_loaded, "disarms", disarms, false );
    optional( jo, was_loaded, "dummy", dummy, false );
    optional( jo, was_loaded, "dodge_counter", dodge_counter, false );
    optional( jo, was_loaded, "block_counter", block_counter, false );
    optional( jo, was_loaded, "miss_recovery", miss_recovery, false );
    optional( jo, was_loaded, "grab_break", grab_break, false );

    optional( jo, was_loaded, "weighting", weighting, 1 );

    optional( jo, was_loaded, "down_dur", down_dur, 0 );
    optional( jo, was_loaded, "stun_dur", stun_dur, 0 );
    optional( jo, was_loaded, "knockback_dist", knockback_dist, 0 );
    optional( jo, was_loaded, "knockback_spread", knockback_spread, 0 );

    optional( jo, was_loaded, "aoe", aoe, "" );
    optional( jo, was_loaded, "flags", flags, auto_flags_reader<> {} );

    reqs.load( jo, src );
    bonuses.load( jo );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

/** @relates string_id */
template<>
const ma_technique &string_id<ma_technique>::obj() const
{
    return ma_techniques.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<ma_technique>::is_valid() const
{
    return ma_techniques.is_valid( *this );
}

void ma_buff::load( JsonObject &jo, const std::string &src )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "buff_duration", buff_duration, 2_turns );
    optional( jo, was_loaded, "max_stacks", max_stacks, 1 );

    optional( jo, was_loaded, "bonus_dodges", dodges_bonus, 0 );
    optional( jo, was_loaded, "bonus_blocks", blocks_bonus, 0 );

    optional( jo, was_loaded, "quiet", quiet, false );
    optional( jo, was_loaded, "throw_immune", throw_immune, false );

    reqs.load( jo, src );
    bonuses.load( jo );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<mabuff>::id() const;

/** @relates string_id */
template<>
const ma_buff &string_id<ma_buff>::obj() const
{
    return ma_buffs.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<ma_buff>::is_valid() const
{
    return ma_buffs.is_valid( *this );
}

void load_martial_art( JsonObject &jo, const std::string &src )
{
    martialarts.load( jo, src );
}

class ma_buff_reader : public generic_typed_reader<ma_buff_reader>
{
    public:
        mabuff_id get_next( JsonIn &jin ) const {
            if( jin.test_string() ) {
                return mabuff_id( jin.get_string() );
            }
            JsonObject jsobj = jin.get_object();
            ma_buffs.load( jsobj, "" );
            return mabuff_id( jsobj.get_string( "id" ) );
        }
};

void martialart::load( JsonObject &jo, const std::string & )
{
    JsonArray jsarr;

    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "static_buffs", static_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onmove_buffs", onmove_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onhit_buffs", onhit_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onattack_buffs", onattack_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ondodge_buffs", ondodge_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "onblock_buffs", onblock_buffs, ma_buff_reader{} );
    optional( jo, was_loaded, "ongethit_buffs", ongethit_buffs, ma_buff_reader{} );

    optional( jo, was_loaded, "techniques", techniques, auto_flags_reader<matec_id> {} );
    optional( jo, was_loaded, "weapons", weapons, auto_flags_reader<itype_id> {} );

    optional( jo, was_loaded, "strictly_unarmed", strictly_unarmed, false );
    optional( jo, was_loaded, "force_unarmed", force_unarmed, false );

    optional( jo, was_loaded, "leg_block", leg_block, 99 );
    optional( jo, was_loaded, "arm_block", arm_block, 99 );

    optional( jo, was_loaded, "arm_block_with_bio_armor_arms", arm_block_with_bio_armor_arms, false );
    optional( jo, was_loaded, "leg_block_with_bio_armor_legs", leg_block_with_bio_armor_legs, false );
}

// Not implemented on purpose (martialart objects have no integer id)
// int_id<T> string_id<martialart>::id() const;

/** @relates string_id */
template<>
const martialart &string_id<martialart>::obj() const
{
    return martialarts.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<martialart>::is_valid() const
{
    return martialarts.is_valid( *this );
}

std::vector<matype_id> all_martialart_types()
{
    std::vector<matype_id> result;
    for( const auto &ma : martialarts.get_all() ) {
        result.push_back( ma.id );
    }
    return result;
}

void check( const ma_requirements &req, const std::string &display_text )
{
    for( auto &r : req.req_buffs ) {
        if( !r.is_valid() ) {
            debugmsg( "ma buff %s of %s does not exist", r.c_str(), display_text.c_str() );
        }
    }
}

void check_martialarts()
{
    for( const auto &ma : martialarts.get_all() ) {
        for( auto technique = ma.techniques.cbegin();
             technique != ma.techniques.cend(); ++technique ) {
            if( !technique->is_valid() ) {
                debugmsg( "Technique with id %s in style %s doesn't exist.",
                          technique->c_str(), ma.name.c_str() );
            }
        }
        for( auto weapon = ma.weapons.cbegin();
             weapon != ma.weapons.cend(); ++weapon ) {
            if( !item::type_is_defined( *weapon ) ) {
                debugmsg( "Weapon %s in style %s doesn't exist.",
                          weapon->c_str(), ma.name.c_str() );
            }
        }
    }
    for( const auto &t : ma_techniques.get_all() ) {
        ::check( t.reqs, string_format( "technique %s", t.id.c_str() ) );
    }
    for( const auto &b : ma_buffs.get_all() ) {
        ::check( b.reqs, string_format( "buff %s", b.id.c_str() ) );
    }
}

/**
 * This is a wrapper class to get access to the protected members of effect_type, it creates
 * the @ref effect_type that is used to store a @ref ma_buff in the creatures effect map.
 * Note: this class must not contain any new members, it will be converted to a plain
 * effect_type later and that would slice the new members of.
 */
class ma_buff_effect_type : public effect_type
{
    public:
        ma_buff_effect_type( const ma_buff &buff ) {
            id = buff.get_effect_id();
            max_intensity = buff.max_stacks;
            // add_effect add the duration to an existing effect, but it must never be
            // above buff_duration, this keeps the old ma_buff behavior
            max_duration = buff.buff_duration;
            dur_add_perc = 100;
            // each add_effect call increases the intensity by 1
            int_add_val = 1;
            // effect intensity increases by -1 each turn.
            int_decay_step = -1;
            int_decay_tick = 1;
            int_dur_factor = 0_turns;
            name.push_back( buff.name );
            desc.push_back( buff.description );
            rating = e_good;
        }
};

void finialize_martial_arts()
{
    // This adds an effect type for each ma_buff, so we can later refer to it and don't need a
    // redundant definition of those effects in json.
    for( const auto &buff : ma_buffs.get_all() ) {
        const ma_buff_effect_type new_eff( buff );
        // Note the slicing here: new_eff is converted to a plain effect_type, but this doesn't
        // bother us because ma_buff_effect_type does not have any members that can be sliced.
        effect_type::register_ma_buff_effect( new_eff );
    }
}

void clear_techniques_and_martial_arts()
{
    martialarts.reset();
    ma_buffs.reset();
    ma_techniques.reset();
}

#include "messages.h"
bool ma_requirements::is_valid_player( const player &u ) const
{
    for( const auto &buff_id : req_buffs ) {
        if( !u.has_mabuff( buff_id ) ) {
            return false;
        }
    }

    //A technique is valid if it applies to unarmed strikes, if it applies generally
    //to all weapons (such as Ninjutsu sneak attacks or innate weapon techniques like RAPID)
    //or if the weapon is flagged as being compatible with the style. Some techniques have
    //further restrictions on required weapon properties (is_valid_weapon).
    bool cqb = u.has_active_bionic( bionic_id( "bio_cqb" ) );
    // There are 4 different cases of "armedness":
    // Truly unarmed, unarmed weapon, style-allowed weapon, generic weapon
    bool valid_weapon =
        ( unarmed_allowed && u.unarmed_attack() &&
          ( !strictly_unarmed || !u.is_armed() ) ) ||
        ( is_valid_weapon( u.weapon ) &&
          ( melee_allowed || u.style_selected.obj().has_weapon( u.weapon.typeId() ) ) );
    if( !valid_weapon ) {
        return false;
    }

    for( const auto &pr : min_skill ) {
        if( ( cqb ? 5 : u.get_skill_level( pr.first ) ) < pr.second ) {
            return false;
        }
    }

    return true;
}

bool ma_requirements::is_valid_weapon( const item &i ) const
{
    for( auto flag : req_flags ) {
        if( !i.has_flag( flag ) ) {
            return false;
        }
    }
    for( const auto &pr : min_damage ) {
        if( i.damage_melee( pr.first ) < pr.second ) {
            return false;
        }
    }

    return true;
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
}

bool ma_technique::is_valid_player( const player &u ) const
{
    return reqs.is_valid_player( u );
}


ma_buff::ma_buff()
    : buff_duration( 2_turns )
{
    max_stacks = 1; // total number of stacks this buff can have

    dodges_bonus = 0; // extra dodges, like karate
    blocks_bonus = 0; // extra blocks, like karate

    throw_immune = false;

}

efftype_id ma_buff::get_effect_id() const
{
    return efftype_id( std::string( "mabuff:" ) + id.str() );
}

const ma_buff *ma_buff::from_effect( const effect &eff )
{
    const std::string &id = eff.get_effect_type()->id.str();
    // Same as in get_effect_id!
    if( id.compare( 0, 7, "mabuff:" ) != 0 ) {
        return nullptr;
    }
    return &mabuff_id( id.substr( 7 ) ).obj();
}

void ma_buff::apply_buff( player &u ) const
{
    u.add_effect( get_effect_id(), time_duration::from_turns( buff_duration ) );
}

bool ma_buff::is_valid_player( const player &u ) const
{
    return reqs.is_valid_player( u );
}

void ma_buff::apply_player( player &u ) const
{
    u.dodges_left += dodges_bonus;
    u.blocks_left += blocks_bonus;
}

int ma_buff::hit_bonus( const player &u ) const
{
    return bonuses.get_flat( u, AFFECTED_HIT );
}
int ma_buff::dodge_bonus( const player &u ) const
{
    return bonuses.get_flat( u, AFFECTED_DODGE );
}
int ma_buff::block_bonus( const player &u ) const
{
    return bonuses.get_flat( u, AFFECTED_BLOCK );
}
int ma_buff::speed_bonus( const player &u ) const
{
    return bonuses.get_flat( u, AFFECTED_SPEED );
}
int ma_buff::armor_bonus( const player &u, damage_type dt ) const
{
    return bonuses.get_flat( u, AFFECTED_ARMOR, dt );
}
float ma_buff::damage_bonus( const player &u, damage_type dt ) const
{
    return bonuses.get_flat( u, AFFECTED_DAMAGE, dt );
}
float ma_buff::damage_mult( const player &u, damage_type dt ) const
{
    return bonuses.get_mult( u, AFFECTED_DAMAGE, dt );
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
void simultaneous_add( player &u, const std::vector<mabuff_id> &buffs )
{
    std::vector<const ma_buff *> buffer; // hey get it because it's for buffs????
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

void martialart::apply_static_buffs( player &u ) const
{
    simultaneous_add( u, static_buffs );
}

void martialart::apply_onmove_buffs( player &u ) const
{
    simultaneous_add( u, onmove_buffs );
}

void martialart::apply_onhit_buffs( player &u ) const
{
    simultaneous_add( u, onhit_buffs );
}

void martialart::apply_onattack_buffs( player &u ) const
{
    simultaneous_add( u, onattack_buffs );
}

void martialart::apply_ondodge_buffs( player &u ) const
{
    simultaneous_add( u, ondodge_buffs );
}

void martialart::apply_onblock_buffs( player &u ) const
{
    simultaneous_add( u, onblock_buffs );
}

void martialart::apply_ongethit_buffs( player &u ) const
{
    simultaneous_add( u, ongethit_buffs );
}


bool martialart::has_technique( const player &u, const matec_id &tec_id ) const
{
    for( const auto &elem : techniques ) {
        const ma_technique &tec = elem.obj();
        if( tec.is_valid_player( u ) && tec.id == tec_id ) {
            return true;
        }
    }
    return false;
}

bool martialart::has_weapon( const itype_id &itt ) const
{
    return weapons.count( itt ) > 0;
}

bool martialart::weapon_valid( const item &it ) const
{
    if( it.is_null() ) {
        return true;
    }

    if( has_weapon( it.typeId() ) ) {
        return true;
    }

    return !strictly_unarmed && it.has_flag( "UNARMED_WEAPON" );
}

// Player stuff

// technique
std::vector<matec_id> player::get_all_techniques( const item &weap ) const
{
    std::vector<matec_id> tecs;
    // Grab individual item techniques
    const auto &weapon_techs = weap.get_techniques();
    tecs.insert( tecs.end(), weapon_techs.begin(), weapon_techs.end() );
    // and martial art techniques
    const auto &style = style_selected.obj();
    tecs.insert( tecs.end(), style.techniques.begin(), style.techniques.end() );

    return tecs;
}

// defensive technique-related
bool player::has_miss_recovery_tec( const item &weap ) const
{
    for( auto &technique : get_all_techniques( weap ) ) {
        if( technique.obj().miss_recovery ) {
            return true;
        }
    }
    return false;
}

// This one isn't used with a weapon
bool player::has_grab_break_tec() const
{
    for( auto &technique : get_all_techniques( item() ) ) {
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
    int unarmed_skill = has_active_bionic( bionic_id( "bio_cqb" ) ) ? 5 : get_skill_level(
                            skill_id( "unarmed" ) );

    // Success conditions.
    if( hp_cur[hp_leg_l] > 0 || hp_cur[hp_leg_r] > 0 ) {
        if( unarmed_skill >= ma.leg_block ) {
            return true;
        } else if( ma.leg_block_with_bio_armor_legs && has_bionic( bionic_id( "bio_armor_legs" ) ) ) {
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
    int unarmed_skill = has_active_bionic( bionic_id( "bio_cqb" ) ) ? 5 : get_skill_level(
                            skill_id( "unarmed" ) );

    // Success conditions.
    if( hp_cur[hp_arm_l] > 0 || hp_cur[hp_arm_r] > 0 ) {
        if( unarmed_skill >= ma.arm_block ) {
            return true;
        } else if( ma.arm_block_with_bio_armor_arms && has_bionic( bionic_id( "bio_armor_arms" ) ) ) {
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
    style_selected.obj().apply_static_buffs( *this );
}
void player::ma_onmove_effects()
{
    style_selected.obj().apply_onmove_buffs( *this );
}
void player::ma_onhit_effects()
{
    style_selected.obj().apply_onhit_buffs( *this );
}
void player::ma_onattack_effects()
{
    style_selected.obj().apply_onattack_buffs( *this );
}
void player::ma_ondodge_effects()
{
    style_selected.obj().apply_ondodge_buffs( *this );
}
void player::ma_onblock_effects()
{
    style_selected.obj().apply_onblock_buffs( *this );
}
void player::ma_ongethit_effects()
{
    style_selected.obj().apply_ongethit_buffs( *this );
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
float player::mabuff_tohit_bonus() const
{
    float ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & ) {
        ret += b.hit_bonus( *this );
    } );
    return ret;
}
float player::mabuff_dodge_bonus() const
{
    float ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.dodge_bonus( *this );
    } );
    return ret;
}
int player::mabuff_block_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.block_bonus( *this );
    } );
    return ret;
}
int player::mabuff_speed_bonus() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.speed_bonus( *this );
    } );
    return ret;
}
int player::mabuff_armor_bonus( damage_type type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.armor_bonus( *this, type );
    } );
    return ret;
}
float player::mabuff_damage_mult( damage_type type ) const
{
    float ret = 1.f;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.damage_mult( *this, type ) - 1 ) + 1;
    } );
    return ret;
}
int player::mabuff_damage_bonus( damage_type type ) const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, type, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.damage_bonus( *this, type );
    } );
    return ret;
}
int player::mabuff_attack_cost_penalty() const
{
    int ret = 0;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        ret += d.get_intensity() * b.bonuses.get_flat( *this, AFFECTED_MOVE_COST );
    } );
    return ret;
}
float player::mabuff_attack_cost_mult() const
{
    float ret = 1.0f;
    accumulate_ma_buff_effects( *effects, [&ret, this]( const ma_buff & b, const effect & d ) {
        // This is correct, so that a 20% buff (1.2) plus a 20% buff (1.2)
        // becomes 1.4 instead of 2.4 (which would be a 240% buff)
        ret *= d.get_intensity() * ( b.bonuses.get_mult( *this, AFFECTED_MOVE_COST ) - 1 ) + 1;
    } );
    return ret;
}

bool player::is_throw_immune() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_throw_immune();
    } );
}
bool player::is_quiet() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.is_quiet();
    } );
}

bool player::can_melee() const
{
    return search_ma_buff_effect( *effects, []( const ma_buff & b, const effect & ) {
        return b.can_melee();
    } );
}

bool player::has_mabuff( mabuff_id id ) const
{
    return search_ma_buff_effect( *effects, [&id]( const ma_buff & b, const effect & ) {
        return b.id == id;
    } );
}

bool player::has_martialart( const matype_id &ma ) const
{
    return std::find( ma_styles.begin(), ma_styles.end(), ma ) != ma_styles.end();
}

void player::add_martialart( const matype_id &ma_id )
{
    if( has_martialart( ma_id ) ) {
        return;
    }
    ma_styles.push_back( ma_id );
}

float ma_technique::damage_bonus( const player &u, damage_type type ) const
{
    return bonuses.get_flat( u, AFFECTED_DAMAGE, type );
}

float ma_technique::damage_multiplier( const player &u, damage_type type ) const
{
    return bonuses.get_mult( u, AFFECTED_DAMAGE, type );
}

float ma_technique::move_cost_multiplier( const player &u ) const
{
    return bonuses.get_mult( u, AFFECTED_MOVE_COST );
}

float ma_technique::move_cost_penalty( const player &u ) const
{
    return bonuses.get_flat( u, AFFECTED_MOVE_COST );
}

float ma_technique::armor_penetration( const player &u, damage_type type ) const
{
    return bonuses.get_flat( u, AFFECTED_ARMOR_PENETRATION, type );
}
