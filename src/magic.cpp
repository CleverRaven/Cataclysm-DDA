#include "magic.h"

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

namespace
{
const std::map<std::string, valid_target> target_map = {
    { "ally", valid_target::target_ally },
    { "hostile", valid_target::target_hostile },
    { "self", valid_target::target_self },
    { "ground", valid_target::target_ground },
    { "none", valid_target::target_none }
};
}
namespace io
{
template<>
valid_target string_to_enum<valid_target>( const std::string &trigger )
{
    return string_to_enum_look_up( target_map, trigger );
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

energy_type energy_source_from_string( const std::string &str )
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

damage_type damage_type_from_string( const std::string &str )
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
