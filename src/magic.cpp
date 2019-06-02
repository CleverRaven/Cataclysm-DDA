#include "magic.h"

#include "avatar.h"
#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"

#include <set>

namespace
{
const std::map<std::string, valid_target> target_map = {
    { "ally", valid_target::target_ally },
    { "hostile", valid_target::target_hostile },
    { "self", valid_target::target_self },
    { "ground", valid_target::target_ground },
    { "none", valid_target::target_none }
};
const std::map<std::string, body_part> bp_map = {
    { "TORSO", body_part::bp_torso },
    { "HEAD", body_part::bp_head },
    { "EYES", body_part::bp_eyes },
    { "MOUTH", body_part::bp_mouth },
    { "ARM_L", body_part::bp_arm_l },
    { "ARM_R", body_part::bp_arm_r },
    { "HAND_L", body_part::bp_hand_l },
    { "HAND_R", body_part::bp_hand_r },
    { "LEG_L", body_part::bp_leg_l },
    { "LEG_R", body_part::bp_leg_r },
    { "FOOT_L", body_part::bp_foot_l },
    { "FOOT_R", body_part::bp_foot_r }
};
}

namespace io
{
template<>
valid_target string_to_enum<valid_target>( const std::string &trigger )
{
    return string_to_enum_look_up( target_map, trigger );
}
template<>
body_part string_to_enum<body_part>( const std::string &trigger )
{
    return string_to_enum_look_up( bp_map, trigger );
}
}

// LOADING
// spell_type

namespace
{
generic_factory<spell_type> spell_factory( "spell" );
}

template<>
const spell_type &string_id<spell_type>::obj() const
{
    return spell_factory.obj( *this );
}

template<>
bool string_id<spell_type>::is_valid() const
{
    return spell_factory.is_valid( *this );
}

void spell_type::load_spell( JsonObject &jo, const std::string &src )
{
    spell_factory.load( jo, src );
}

static energy_type energy_source_from_string( const std::string &str )
{
    if( str == "MANA" ) {
        return mana_energy;
    } else if( str == "HP" ) {
        return hp_energy;
    } else if( str == "BIONIC" ) {
        return bionic_energy;
    } else if( str == "STAMINA" ) {
        return stamina_energy;
    } else if( str == "NONE" ) {
        return none_energy;
    } else {
        debugmsg( _( "ERROR: Invalid energy string.  Defaulting to NONE" ) );
        return none_energy;
    }
}

static damage_type damage_type_from_string( const std::string &str )
{
    if( str == "fire" ) {
        return DT_HEAT;
    } else if( str == "acid" ) {
        return DT_ACID;
    } else if( str == "bash" ) {
        return DT_BASH;
    } else if( str == "bio" ) {
        return DT_BIOLOGICAL;
    } else if( str == "cold" ) {
        return DT_COLD;
    } else if( str == "cut" ) {
        return DT_CUT;
    } else if( str == "electric" ) {
        return DT_ELECTRIC;
    } else if( str == "stab" ) {
        return DT_STAB;
    } else if( str == "none" || str == "NONE" ) {
        return DT_TRUE;
    } else {
        debugmsg( _( "ERROR: Invalid damage type string.  Defaulting to none" ) );
        return DT_TRUE;
    }
}

void spell_type::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "description", description, translated_string_reader );
    mandatory( jo, was_loaded, "effect", effect );

    const auto trigger_reader = enum_flags_reader<valid_target> { "valid_targets" };
    mandatory( jo, was_loaded, "valid_targets", valid_targets, trigger_reader );

    const auto bp_reader = enum_flags_reader<body_part> { "affected_bps" };
    optional( jo, was_loaded, "affected_body_parts", affected_bps, bp_reader );

    optional( jo, was_loaded, "effect_str", effect_str, "" );

    optional( jo, was_loaded, "min_damage", min_damage, 0 );
    optional( jo, was_loaded, "damage_increment", damage_increment, 0.0f );
    optional( jo, was_loaded, "max_damage", max_damage, 0 );

    optional( jo, was_loaded, "min_range", min_range, 0 );
    optional( jo, was_loaded, "range_increment", range_increment, 0.0f );
    optional( jo, was_loaded, "max_range", max_range, 0 );

    optional( jo, was_loaded, "min_aoe", min_aoe, 0 );
    optional( jo, was_loaded, "aoe_increment", aoe_increment, 0.0f );
    optional( jo, was_loaded, "max_aoe", max_aoe, 0 );

    optional( jo, was_loaded, "min_dot", min_dot, 0 );
    optional( jo, was_loaded, "dot_increment", dot_increment, 0.0f );
    optional( jo, was_loaded, "max_dot", max_dot, 0 );

    optional( jo, was_loaded, "min_duration", min_duration, 0 );
    optional( jo, was_loaded, "duration_increment", duration_increment, 0.0f );
    optional( jo, was_loaded, "max_duration", max_duration, 0 );

    optional( jo, was_loaded, "min_pierce", min_pierce, 0 );
    optional( jo, was_loaded, "pierce_increment", pierce_increment, 0.0f );
    optional( jo, was_loaded, "max_pierce", max_pierce, 0 );

    optional( jo, was_loaded, "base_energy_cost", base_energy_cost, 0 );
    optional( jo, was_loaded, "final_energy_cost", final_energy_cost, base_energy_cost );
    optional( jo, was_loaded, "energy_increment", energy_increment, 0.0f );

    std::string temp_string;
    optional( jo, was_loaded, "spell_class", temp_string, "NONE" );
    spell_class = trait_id( temp_string );
    optional( jo, was_loaded, "energy_source", temp_string, "NONE" );
    energy_source = energy_source_from_string( temp_string );
    optional( jo, was_loaded, "damage_type", temp_string, "NONE" );
    dmg_type = damage_type_from_string( temp_string );
    optional( jo, was_loaded, "difficulty", difficulty, 0 );
    optional( jo, was_loaded, "max_level", max_level, 0 );

    optional( jo, was_loaded, "base_casting_time", base_casting_time, 0 );
    optional( jo, was_loaded, "final_casting_time", final_casting_time, base_casting_time );
    optional( jo, was_loaded, "casting_time_increment", casting_time_increment, 0.0f );
}

void spell_type::check_consistency()
{
    for( const spell_type &sp_t : get_all() ) {
        if( sp_t.min_aoe > sp_t.max_aoe ) {
            debugmsg( string_format( "ERROR: %s has higher min_aoe than max_aoe", sp_t.id.c_str() ) );
        }
        if( abs( sp_t.min_damage ) > abs( sp_t.max_damage ) ) {
            debugmsg( string_format( "ERROR: %s has higher min_damage than max_damage", sp_t.id.c_str() ) );
        }
        if( sp_t.min_range > sp_t.max_range ) {
            debugmsg( string_format( "ERROR: %s has higher min_range than max_range", sp_t.id.c_str() ) );
        }
        if( sp_t.min_dot > sp_t.max_dot ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot than max_dot", sp_t.id.c_str() ) );
        }
        if( sp_t.min_duration > sp_t.max_duration ) {
            debugmsg( string_format( "ERROR: %s has higher min_dot_time than max_dot_time", sp_t.id.c_str() ) );
        }
        if( sp_t.min_pierce > sp_t.max_pierce ) {
            debugmsg( string_format( "ERROR: %s has higher min_pierce than max_pierce", sp_t.id.c_str() ) );
        }
        if( sp_t.casting_time_increment < 0.0f && sp_t.base_casting_time < sp_t.final_casting_time ) {
            debugmsg( string_format( "ERROR: %s has negative increment and base_casting_time < final_casting_time",
                                     sp_t.id.c_str() ) );
        }
        if( sp_t.casting_time_increment > 0.0f && sp_t.base_casting_time > sp_t.final_casting_time ) {
            debugmsg( string_format( "ERROR: %s has positive increment and base_casting_time > final_casting_time",
                                     sp_t.id.c_str() ) );
        }
    }
}

const std::vector<spell_type> &spell_type::get_all()
{
    return spell_factory.get_all();
}

void spell_type::reset_all()
{
    spell_factory.reset();
}

bool spell_type::is_valid() const
{
    return spell_factory.is_valid( this->id );
}

// spell

spell::spell( const spell_type *sp, int xp )
{
    type = sp;
    experience = xp;
}

spell::spell( spell_id sp, int xp ) : spell( &sp.obj(), xp ) {}

spell_id spell::id() const
{
    return type->id;
}

int spell::damage() const
{
    if( type->min_damage >= 0 ) {
        return std::min( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    } else { // if it's negative, min and max work differently
        return std::max( static_cast<int>( type->min_damage + round( get_level() *
                                           type->damage_increment ) ), type->max_damage );
    }
}

int spell::aoe() const
{
    return std::min( static_cast<int>( type->min_aoe + round( get_level() * type->aoe_increment ) ),
                     type->max_aoe );
}

int spell::range() const
{
    return std::min( static_cast<int>( type->min_range + round( get_level() * type->range_increment ) ),
                     type->max_range );
}

int spell::duration() const
{
    return std::min( static_cast<int>( type->min_duration + round( get_level() *
                                       type->duration_increment ) ),
                     type->max_duration );
}

time_duration spell::duration_turns() const
{
    return 1_turns * duration() / 100;
}

void spell::gain_level()
{
    gain_exp( exp_to_next_level() );
}

bool spell::is_max_level() const
{
    return get_level() >= type->max_level;
}

bool spell::can_learn( const player &p ) const
{
    if( type->spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return p.has_trait( type->spell_class );
}

int spell::energy_cost() const
{
    if( type->base_energy_cost < type->final_energy_cost ) {
        return std::min( type->final_energy_cost,
                         static_cast<int>( round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else if( type->base_energy_cost > type->final_energy_cost ) {
        return std::max( type->final_energy_cost,
                         static_cast<int>( round( type->base_energy_cost + type->energy_increment * get_level() ) ) );
    } else {
        return type->base_energy_cost;
    }
}

bool spell::can_cast( const player &p ) const
{
    if( !p.magic.knows_spell( type->id ) ) {
        // how in the world can this happen?
        debugmsg( "ERROR: owner of spell does not know spell" );
        return false;
    }
    switch( type->energy_source ) {
        case mana_energy:
            return p.magic.available_mana() >= energy_cost();
        case stamina_energy:
            return p.stamina >= energy_cost();
        case hp_energy: {
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( energy_cost() < p.hp_cur[i] ) {
                    return true;
                }
            }
            return false;
        }
        case bionic_energy:
            return p.power_level >= energy_cost();
        case none_energy:
        default:
            return true;
    }
}

int spell::get_difficulty() const
{
    return type->difficulty;
}

int spell::casting_time() const
{
    if( type->base_casting_time < type->final_casting_time ) {
        return std::min( type->final_casting_time,
                         static_cast<int>( round( type->base_casting_time + type->casting_time_increment * get_level() ) ) );
    } else if( type->base_casting_time > type->final_casting_time ) {
        return std::max( type->final_casting_time,
                         static_cast<int>( round( type->base_casting_time + type->casting_time_increment * get_level() ) ) );
    } else {
        return type->base_casting_time;
    }
}

std::string spell::name() const
{
    return _( type->name );
}

float spell::spell_fail( const player &p ) const
{
    // formula is based on the following:
    // exponential curve
    // effective skill of 0 or less is 100% failure
    // effective skill of 8 (8 int, 0 spellcraft, 0 spell level, spell difficulty 0) is ~50% failure
    // effective skill of 30 is 0% failure
    const float effective_skill = 2 * ( get_level() - get_difficulty() ) + p.get_int() +
                                  p.get_skill_level( skill_id( "SPELLCRAFT" ) );
    // add an if statement in here because sufficiently large numbers will definitely overflow because of exponents
    if( effective_skill > 30.0f ) {
        return 0.0f;
    } else if( effective_skill < 0.0f ) {
        return 1.0f;
    }
    const float fail_chance = pow( ( effective_skill - 30.0f ) / 30.0f, 2 );
    return clamp( fail_chance, 0.0f, 1.0f );
}

std::string spell::colorized_fail_percent( const player &p ) const
{
    const float fail_fl = spell_fail( p ) * 100.0f;
    std::string fail_str;
    fail_fl == 100.0f ? fail_str = _( "Difficult!" ) : fail_str =  string_format( "%.1f %% %s",
                                   fail_fl, _( "Failure Chance" ) );
    nc_color color;
    if( fail_fl > 90.0f ) {
        color = c_magenta;
    } else if( fail_fl > 75.0f ) {
        color = c_red;
    } else if( fail_fl > 60.0f ) {
        color = c_light_red;
    } else if( fail_fl > 35.0f ) {
        color = c_yellow;
    } else if( fail_fl > 15.0f ) {
        color = c_green;
    } else {
        color = c_light_green;
    }
    return colorize( fail_str, color );
}

int spell::xp() const
{
    return experience;
}

void spell::gain_exp( int nxp )
{
    experience += nxp;
}

std::string spell::energy_string() const
{
    switch( type->energy_source ) {
        case hp_energy:
            return _( "health" );
        case mana_energy:
            return _( "mana" );
        case stamina_energy:
            return _( "stamina" );
        case bionic_energy:
            return _( "bionic power" );
        default:
            return "";
    }
}

std::string spell::energy_cost_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "none" );
    }
    if( energy_source() == bionic_energy || energy_source() == mana_energy ) {
        return colorize( to_string( energy_cost() ), c_light_blue );
    }
    if( energy_source() == hp_energy ) {
        auto pair = get_hp_bar( energy_cost(), p.get_hp_max() / num_hp_parts );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( energy_cost(), p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

std::string spell::energy_cur_string( const player &p ) const
{
    if( energy_source() == none_energy ) {
        return _( "infinite" );
    }
    if( energy_source() == bionic_energy ) {
        return colorize( to_string( p.power_level ), c_light_blue );
    }
    if( energy_source() == mana_energy ) {
        return colorize( to_string( p.magic.available_mana() ), c_light_blue );
    }
    if( energy_source() == stamina_energy ) {
        auto pair = get_hp_bar( p.stamina, p.get_stamina_max() );
        return colorize( pair.first, pair.second );
    }
    if( energy_source() == hp_energy ) {
        return "";
    }
    debugmsg( "ERROR: Spell %s has invalid energy source.", id().c_str() );
    return _( "error: energy_type" );
}

bool spell::is_valid() const
{
    if( type == nullptr ) {
        return false;
    }
    return type->is_valid();
}

bool spell::bp_is_affected( body_part bp ) const
{
    return type->affected_bps[bp];
}

std::string spell::effect() const
{
    return type->effect;
}

energy_type spell::energy_source() const
{
    return type->energy_source;
}

bool spell::is_valid_target( valid_target t ) const
{
    return type->valid_targets[t];
}

bool spell::is_valid_target( const tripoint &p ) const
{
    bool valid = false;
    if( Creature *const cr = g->critter_at<Creature>( p ) ) {
        Creature::Attitude cr_att = cr->attitude_to( g->u );
        valid = valid || ( cr_att != Creature::A_FRIENDLY && is_valid_target( target_hostile ) ) ||
                ( cr_att == Creature::A_FRIENDLY && is_valid_target( target_ally ) );
    }
    if( p == g->u.pos() ) {
        valid = valid || is_valid_target( target_self );
    }
    return valid || is_valid_target( target_ground );
}

std::string spell::description() const
{
    return _( type->description );
}

nc_color spell::damage_type_color() const
{
    switch( dmg_type() ) {
        case DT_HEAT:
            return c_red;
        case DT_ACID:
            return c_light_green;
        case DT_BASH:
            return c_magenta;
        case DT_BIOLOGICAL:
            return c_green;
        case DT_COLD:
            return c_white;
        case DT_CUT:
            return c_light_gray;
        case DT_ELECTRIC:
            return c_light_blue;
        case DT_STAB:
            return c_light_red;
        case DT_TRUE:
            return c_dark_gray;
        default:
            return c_black;
    }
}

// constants defined below are just for the formula to be used,
// in order for the inverse formula to be equivalent
static const float a = 6200.0;
static const float b = 0.146661;
static const float c = -62.5;

int spell::get_level() const
{
    // you aren't at the next level unless you have the requisite xp, so floor
    return std::max( static_cast<int>( floor( log( experience + a ) / b + c ) ), 0 );
}

int spell::get_max_level() const
{
    return type->max_level;
}

// helper function to calculate xp needed to be at a certain level
// pulled out as a helper function to make it easier to either be used in the future
// or easier to tweak the formula
static int exp_for_level( int level )
{
    // level 0 never needs xp
    if( level == 0 ) {
        return 0;
    }
    return ceil( exp( ( level - c ) * b ) ) - a;
}

int spell::exp_to_next_level() const
{
    return exp_for_level( get_level() + 1 ) - xp();
}

std::string spell::exp_progress() const
{
    const int level = get_level();
    const int this_level_xp = exp_for_level( level );
    const int next_level_xp = exp_for_level( level + 1 );
    const int denominator = next_level_xp - this_level_xp;
    const float progress = static_cast<float>( xp() - this_level_xp ) / std::max( 1.0f,
                           static_cast<float>( denominator ) );
    return string_format( "%i%%", clamp( static_cast<int>( round( progress * 100 ) ), 0, 99 ) );
}

float spell::exp_modifier( const player &p ) const
{
    const float int_modifier = ( p.get_int() - 8.0f ) / 8.0f;
    const float difficulty_modifier = get_difficulty() / 20.0f;
    const float spellcraft_modifier = p.get_skill_level( skill_id( "SPELLCRAFT" ) ) / 10.0f;

    return int_modifier + difficulty_modifier + spellcraft_modifier + 1.0f;
}

int spell::casting_exp( const player &p ) const
{
    // the amount of xp you would get with no modifiers
    const int base_casting_xp = 75;

    return round( p.adjust_for_focus( base_casting_xp * exp_modifier( p ) ) );
}

damage_type spell::dmg_type() const
{
    return type->dmg_type;
}

damage_instance spell::get_damage_instance() const
{
    damage_instance dmg;
    dmg.add_damage( dmg_type(), damage() );
    return dmg;
}

dealt_damage_instance spell::get_dealt_damage_instance() const
{
    dealt_damage_instance dmg;
    dmg.set_damage( dmg_type(), damage() );
    return dmg;
}

std::string spell::effect_data() const
{
    return type->effect_str;
}

int spell::heal( const tripoint &target ) const
{
    monster *const mon = g->critter_at<monster>( target );
    if( mon ) {
        return mon->heal( -damage() );
    }
    player *const p = g->critter_at<player>( target );
    if( p ) {
        p->healall( -damage() );
        return -damage();
    }
    return -1;
}

// player

known_magic::known_magic()
{
    mana_base = 1000;
    mana = mana_base;
}

void known_magic::serialize( JsonOut &json ) const
{
    json.start_object();

    json.member( "mana", mana );

    json.member( "spellbook" );
    json.start_array();
    for( auto pair : spellbook ) {
        json.start_object();
        json.member( "id", pair.second.id() );
        json.member( "xp", pair.second.xp() );
        json.end_object();
    }
    json.end_array();

    json.end_object();
}

void known_magic::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "mana", mana );

    JsonArray parray = data.get_array( "spellbook" );
    while( parray.has_more() ) {
        JsonObject jo = parray.next_object();
        std::string id = jo.get_string( "id" );
        spell_id sp = spell_id( id );
        int xp = jo.get_int( "xp" );
        spellbook.emplace( sp, spell( sp, xp ) );
    }
}

bool known_magic::knows_spell( const std::string &sp ) const
{
    return knows_spell( spell_id( sp ) );
}

bool known_magic::knows_spell( spell_id sp ) const
{
    return spellbook.count( sp ) == 1;
}

void known_magic::learn_spell( const std::string &sp, player &p, bool force )
{
    learn_spell( spell_id( sp ), p, force );
}

void known_magic::learn_spell( spell_id sp, player &p, bool force )
{
    learn_spell( &sp.obj(), p, force );
}

void known_magic::learn_spell( const spell_type *sp, player &p, bool force )
{
    if( !sp->is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    spell temp_spell( sp );
    if( !temp_spell.is_valid() ) {
        debugmsg( "Tried to learn invalid spell" );
        return;
    }
    if( !force && sp->spell_class != trait_id( "NONE" ) ) {
        if( can_learn_spell( p, sp->id ) && !p.has_trait( sp->spell_class ) ) {
            if( query_yn(
                    _( "Learning this spell will make you a %s and lock you out of other unique spells.\nContinue?" ),
                    sp->spell_class.obj().name() ) ) {
                p.set_mutation( sp->spell_class );
                p.add_msg_if_player( sp->spell_class.obj().desc() );
            } else {
                return;
            }
        }
    }
    if( force || can_learn_spell( p, sp->id ) ) {
        spellbook.emplace( sp->id, temp_spell );
        p.add_msg_if_player( m_good, _( "You learned %s!" ), _( sp->name ) );
    } else {
        p.add_msg_if_player( m_bad, _( "You can't learn this spell." ) );
    }
}

void known_magic::forget_spell( const std::string &sp )
{
    forget_spell( spell_id( sp ) );
}

void known_magic::forget_spell( spell_id sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "Can't forget a spell you don't know!" );
        return;
    }
    spellbook.erase( sp );
}

bool known_magic::can_learn_spell( const player &p, spell_id sp ) const
{
    const spell_type sp_t = sp.obj();
    if( sp_t.spell_class == trait_id( "NONE" ) ) {
        return true;
    }
    return !p.has_opposite_trait( sp_t.spell_class );
}

spell &known_magic::get_spell( spell_id sp )
{
    if( !knows_spell( sp ) ) {
        debugmsg( "ERROR: Tried to get unknown spell" );
    }
    spell &temp_spell = spellbook[ sp ];
    return temp_spell;
}

std::vector<spell *> known_magic::get_spells()
{
    std::vector<spell *> spells;
    for( auto &spell_pair : spellbook ) {
        spells.emplace_back( &spell_pair.second );
    }
    return spells;
}

int known_magic::available_mana() const
{
    return mana;
}

void known_magic::set_mana( int new_mana )
{
    mana = new_mana;
}

void known_magic::mod_mana( const player &p, int add_mana )
{
    set_mana( clamp( mana + add_mana, 0, max_mana( p ) ) );
}

int known_magic::max_mana( const player &p ) const
{
    const float int_bonus = ( ( 0.2f + p.get_int() * 0.1f ) - 1.0f ) * mana_base;
    return std::max( 0.0f, ( ( mana_base + int_bonus ) * p.mutation_value( "mana_multiplier" ) ) +
                     p.mutation_value( "mana_modifier" ) - p.power_level );
}

void known_magic::update_mana( const player &p, float turns )
{
    // mana should replenish in 8 hours.
    const float full_replenish = to_turns<float>( 8_hours );
    const float ratio = turns / full_replenish;
    mod_mana( p, floor( ratio * max_mana( p ) * p.mutation_value( "mana_regen_multiplier" ) ) );
}

std::vector<spell_id> known_magic::spells() const
{
    std::vector<spell_id> spell_ids;
    for( auto pair : spellbook ) {
        spell_ids.emplace_back( pair.first );
    }
    return spell_ids;
}

// does the player have enough energy (of the type of the spell) to cast the spell?
bool known_magic::has_enough_energy( const player &p, spell &sp ) const
{
    int cost = sp.energy_cost();
    switch( sp.energy_source() ) {
        case mana_energy:
            return available_mana() >= cost;
        case bionic_energy:
            return p.power_level >= cost;
        case stamina_energy:
            return p.stamina >= cost;
        case hp_energy:
            for( int i = 0; i < num_hp_parts; i++ ) {
                if( p.hp_cur[i] > cost ) {
                    return true;
                }
            }
            return false;
        case none_energy:
            return true;
        default:
            return false;
    }
}

int known_magic::time_to_learn_spell( const player &p, const std::string &str ) const
{
    return time_to_learn_spell( p, spell_id( str ) );
}

int known_magic::time_to_learn_spell( const player &p, spell_id sp ) const
{
    const int base_time = 30000;
    return base_time * ( 1.0 + sp.obj().difficulty / ( 1.0 + ( p.get_int() - 8.0 ) / 8.0 ) +
                         ( p.get_skill_level( skill_id( "SPELLCRAFT" ) ) / 10.0 ) );
}

// spell_effect

namespace spell_effect
{

static tripoint random_point( int min_distance, int max_distance, const tripoint &player_pos )
{
    const int angle = rng( 0, 360 );
    const int dist = rng( min_distance, max_distance );
    const int x = round( dist * cos( angle ) );
    const int y = round( dist * sin( angle ) );
    return tripoint( x + player_pos.x, y + player_pos.y, player_pos.z );
}

void teleport( int min_distance, int max_distance )
{
    if( min_distance > max_distance || min_distance < 0 || max_distance < 0 ) {
        debugmsg( "ERROR: Teleport argument(s) invalid" );
        return;
    }
    const tripoint player_pos = g->u.pos();
    tripoint target;
    // limit the loop just in case it's impossble to find a valid point in the range
    int tries = 0;
    do {
        target = random_point( min_distance, max_distance, player_pos );
        tries++;
    } while( g->m.impassable( target ) && tries < 20 );
    if( tries == 20 ) {
        add_msg( m_bad, _( "Unable to find a valid target for teleport." ) );
        return;
    }
    g->place_player( target );
}

void pain_split()
{
    player &p = g->u;
    add_msg( m_info, _( "Your injuries even out." ) );
    int num_limbs = 0; // number of limbs effected (broken don't count)
    int total_hp = 0; // total hp among limbs

    for( const int &part : p.hp_cur ) {
        if( part != 0 ) {
            num_limbs++;
            total_hp += part;
        }
    }
    for( int &part : p.hp_cur ) {
        const int hp_each = total_hp / num_limbs;
        if( part != 0 ) {
            part = hp_each;
        }
    }
}

void move_earth( const tripoint &target )
{
    ter_id ter_here = g->m.ter( target );

    std::set<ter_id> empty_air = { t_hole };
    std::set<ter_id> deep_pit = { t_pit, t_slope_down };
    std::set<ter_id> shallow_pit = { t_pit_corpsed, t_pit_covered, t_pit_glass, t_pit_glass_covered, t_pit_shallow, t_pit_spiked, t_pit_spiked, t_pit_spiked_covered, t_rootcellar };
    std::set<ter_id> soft_dirt = { t_grave, t_dirt, t_sand, t_clay, t_dirtmound, t_grass, t_grass_long, t_grass_tall, t_grass_golf, t_grass_dead, t_grass_white, t_dirtfloor, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_sandbox };
    // rock: can still be dug through with patience, converts to sand upon completion
    std::set<ter_id> hard_dirt = { t_pavement, t_pavement_y, t_sidewalk, t_concrete, t_thconc_floor, t_thconc_floor_olight, t_strconc_floor, t_floor, t_floor_waxed, t_carpet_red, t_carpet_yellow, t_carpet_purple, t_carpet_green, t_linoleum_white, t_linoleum_gray, t_slope_up, t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue, t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp };

    if( empty_air.count( ter_here ) == 1 ) {
        add_msg( m_bad, _( "All the dust in the air here falls to the ground." ) );
    } else if( deep_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_hole );
        add_msg( _( "The pit has deepened further." ) );
    } else if( shallow_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit );
        add_msg( _( "More debris shifts out of the pit." ) );
    } else if( soft_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit_shallow );
        add_msg( _( "The earth moves out of the way for you." ) );
    } else if( hard_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_sand );
        add_msg( _( "The rocks here are ground into sand." ) );
    } else {
        add_msg( m_bad, _( "The earth here does not listen to your command to move." ) );
    }
}

static bool in_spell_aoe( const tripoint &target, const tripoint &epicenter, const int &radius,
                          const bool ignore_walls )
{
    if( ignore_walls ) {
        return rl_dist( epicenter, target ) <= radius;
    }
    std::vector<tripoint> trajectory = line_to( epicenter, target );
    for( const tripoint &pt : trajectory ) {
        if( g->m.impassable( pt ) ) {
            return false;
        }
    }
    return rl_dist( epicenter, target ) <= radius;
}

std::set<tripoint> spell_effect_blast( spell &, const tripoint &, const tripoint &target,
                                       const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // TODO: Make this breadth-first
    for( int x = target.x - aoe_radius; x <= target.x + aoe_radius; x++ ) {
        for( int y = target.y - aoe_radius; y <= target.y + aoe_radius; y++ ) {
            for( int z = target.z - aoe_radius; z <= target.z + aoe_radius; z++ ) {
                const tripoint potential_target( x, y, z );
                if( in_spell_aoe( potential_target, target, aoe_radius, ignore_walls ) ) {
                    targets.emplace( potential_target );
                }
            }
        }
    }
    return targets;
}

std::set<tripoint> spell_effect_cone( spell &sp, const tripoint &source,
                                      const tripoint &target,
                                      const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // cones go all the way to end (if they don't hit an obstacle)
    const int range = sp.range() + 1;
    const int initial_angle = coord_to_angle( source, target );
    std::set<tripoint> end_points;
    for( int angle = initial_angle - floor( aoe_radius / 2.0 );
         angle <= initial_angle + ceil( aoe_radius / 2.0 ); angle++ ) {
        tripoint potential;
        calc_ray_end( angle, range, source, potential );
        end_points.emplace( potential );
    }
    for( const tripoint &ep : end_points ) {
        std::vector<tripoint> trajectory = line_to( ep, source );
        for( const tripoint &tp : trajectory ) {
            if( ignore_walls || g->m.passable( tp ) ) {
                targets.emplace( tp );
            } else {
                break;
            }
        }
    }
    // we don't want to hit ourselves in the blast!
    targets.erase( source );
    return targets;
}

std::set<tripoint> spell_effect_line( spell &, const tripoint &source,
                                      const tripoint &target,
                                      const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    const int initial_angle = coord_to_angle( source, target );
    tripoint clockwise_starting_point;
    calc_ray_end( initial_angle - 90, floor( aoe_radius / 2.0 ), source, clockwise_starting_point );
    tripoint cclockwise_starting_point;
    calc_ray_end( initial_angle + 90, ceil( aoe_radius / 2.0 ), source, cclockwise_starting_point );
    tripoint clockwise_end_point;
    calc_ray_end( initial_angle - 90, floor( aoe_radius / 2.0 ), target, clockwise_end_point );
    tripoint cclockwise_end_point;
    calc_ray_end( initial_angle + 90, ceil( aoe_radius / 2.0 ), target, cclockwise_end_point );


    std::vector<tripoint> start_width = line_to( clockwise_starting_point, cclockwise_starting_point );
    start_width.insert( start_width.begin(), clockwise_end_point );
    std::vector<tripoint> end_width = line_to( clockwise_end_point, cclockwise_end_point );
    end_width.insert( end_width.begin(), clockwise_starting_point );

    std::vector<tripoint> cwise_line = line_to( clockwise_starting_point, cclockwise_starting_point );
    cwise_line.insert( cwise_line.begin(), clockwise_starting_point );
    std::vector<tripoint> ccwise_line = line_to( cclockwise_starting_point, cclockwise_end_point );
    ccwise_line.insert( ccwise_line.begin(), cclockwise_starting_point );

    for( const tripoint &start_line_pt : start_width ) {
        bool passable = true;
        for( const tripoint &potential_target : line_to( source, start_line_pt ) ) {
            passable = g->m.passable( potential_target ) || ignore_walls;
            if( passable ) {
                targets.emplace( potential_target );
            } else {
                break;
            }
        }
        if( !passable ) {
            // leading edge of line attack is very important to the whole
            // if the leading edge is blocked, none of the attack spawning
            // from that edge can propogate
            continue;
        }
        for( const tripoint &end_line_pt : end_width ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, end_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
        for( const tripoint &cwise_line_pt : cwise_line ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, cwise_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
        for( const tripoint &ccwise_line_pt : ccwise_line ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, ccwise_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
    }

    targets.erase( source );

    return targets;
}

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
static std::set<tripoint> spell_effect_area( spell &sp, const tripoint &source,
        const tripoint &target,
        std::function<std::set<tripoint>( spell &, const tripoint &, const tripoint &, const int, const bool )>
        aoe_func, bool ignore_walls = false )
{
    std::set<tripoint> targets = { target }; // initialize with epicenter
    if( sp.aoe() <= 1 ) {
        return targets;
    }

    const int aoe_radius = sp.aoe();
    targets = aoe_func( sp, source,  target, aoe_radius, ignore_walls );

    for( const tripoint &p : targets ) {
        if( !sp.is_valid_target( p ) ) {
            targets.erase( p );
        }
    }

    // Draw the explosion
    std::map<tripoint, nc_color> explosion_colors;
    for( auto &pt : targets ) {
        explosion_colors[pt] = sp.damage_type_color();
    }

    explosion_handler::draw_custom_explosion( g->u.pos(), explosion_colors );
    return targets;
}

static void add_effect_to_target( const tripoint &target, const spell &sp )
{
    const int dur_moves = sp.duration();
    const time_duration dur_td = 1_turns * dur_moves / 100;

    Creature *const critter = g->critter_at<Creature>( target );
    Character *const guy = g->critter_at<Character>( target );
    efftype_id spell_effect( sp.effect_data() );
    bool bodypart_effected = false;

    if( guy ) {
        for( const body_part bp : all_body_parts ) {
            if( sp.bp_is_affected( bp ) ) {
                guy->add_effect( spell_effect, dur_td, bp );
                bodypart_effected = true;
            }
        }
        if( bodypart_effected ) {
            return;
        }
    }
    if( !bodypart_effected ) {
        critter->add_effect( spell_effect, dur_td, num_bp );
    }
}

static void damage_targets( spell &sp, std::set<tripoint> targets )
{
    for( const tripoint target : targets ) {
        Creature *const cr = g->critter_at<Creature>( target );
        if( !cr ) {
            continue;
        }

        projectile bolt;
        bolt.impact = sp.get_damage_instance();
        bolt.proj_effects.emplace( "magic" );

        dealt_projectile_attack atk;
        atk.end_point = target;
        atk.hit_critter = cr;
        atk.proj = bolt;
        if( sp.damage() > 0 ) {
            cr->deal_projectile_attack( &g->u, atk, true );
        } else {
            sp.heal( target );
            add_msg( m_good, _( "%s wounds are closing up!" ), cr->disp_name( true ) );
        }
        if( !sp.effect_data().empty() ) {
            add_effect_to_target( target, sp );
        }
    }
}

void projectile_attack( spell &sp, const tripoint &source, const tripoint &target )
{
    std::vector<tripoint> trajectory = line_to( source, target );
    for( const tripoint &pt : trajectory ) {
        if( g->m.impassable( pt ) || pt == trajectory.back() ) {
            target_attack( sp, source, target );
        }
    }
}

void target_attack( spell &sp, const tripoint &source, const tripoint &epicenter )
{
    damage_targets( sp, spell_effect_area( sp, source, epicenter, spell_effect_blast ) );
}

void cone_attack( spell &sp, const tripoint &source, const tripoint &target )
{
    damage_targets( sp, spell_effect_area( sp, source, target, spell_effect_cone ) );
}

void line_attack( spell &sp, const tripoint &source, const tripoint &target )
{
    damage_targets( sp, spell_effect_area( sp, source, target, spell_effect_line ) );
}

void spawn_ethereal_item( spell &sp )
{
    item granted( sp.effect_data(), calendar::turn );
    if( !granted.is_comestible() ) {
        granted.set_var( "ethereal", to_turns<int>( sp.duration_turns() ) );
        granted.set_flag( "ETHEREAL_ITEM" );
    }
    if( granted.count_by_charges() && sp.damage() > 0 ) {
        granted.charges = sp.damage();
    }
    if( g->u.can_wear( granted ).success() ) {
        granted.set_flag( "FIT" );
        g->u.wear_item( granted, false );
    } else if( !g->u.is_armed() ) {
        g->u.weapon = granted;
    } else {
        g->u.i_add( granted );
    }
    if( !granted.count_by_charges() ) {
        for( int i = 1; i < sp.damage(); i++ ) {
            g->u.i_add( granted );
        }
    }
}

}
