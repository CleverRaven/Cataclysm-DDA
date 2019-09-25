#include "relic.h"

#include "creature.h"
#include "magic_enchantment.h"
#include "json.h"
#include "point.h"
#include "magic.h"
#include "translations.h"

#include <cmath>

void relic::add_active_effect( const fake_spell &sp )
{
    active_effects.emplace_back( sp );
}

void relic::add_passive_effect( const enchantment &nench )
{
    for( enchantment &ench : passive_effects ) {
        if( ench.add( nench ) ) {
            return;
        }
    }
    passive_effects.emplace_back( nench );
}

void relic::load( JsonObject &jo )
{
    if( jo.has_array( "active_effects" ) ) {
        JsonArray jarray = jo.get_array( "active_effects" );
        while( jarray.has_more() ) {
            fake_spell sp;
            JsonObject jobj = jarray.next_object();
            sp.load( jobj );
            add_active_effect( sp );
        }
    }
    if( jo.has_array( "passive_effects" ) ) {
        JsonArray jarray = jo.get_array( "passive_effects" );
        while( jarray.has_more() ) {
            enchantment ench;
            JsonObject jobj = jarray.next_object();
            ench.load( jobj );
            add_passive_effect( ench );
        }
    }
    jo.read( "name", item_name_override );
    charges_per_activation = jo.get_int( "charges_per_activation", 1 );
    moves = jo.get_int( "moves", 100 );
}

void relic::deserialize( JsonIn &jsin )
{
    JsonObject jobj = jsin.get_object();
    load( jobj );
}

void relic::serialize( JsonOut &jsout ) const
{
    jsout.start_object();

    jsout.member( "moves", moves );
    jsout.member( "charges_per_activation", charges_per_activation );
    // item_name_override is not saved, in case the original json text changes:
    // in such case names read back from a save wouold no longer be properly translated.

    if( !passive_effects.empty() ) {
        jsout.member( "passive_effects" );
        jsout.start_array();
        for( const enchantment &ench : passive_effects ) {
            ench.serialize( jsout );
        }
        jsout.end_array();
    }

    if( !active_effects.empty() ) {
        jsout.member( "active_effects" );
        jsout.start_array();
        for( const fake_spell &sp : active_effects ) {
            sp.serialize( jsout );
        }
        jsout.end_array();
    }

    jsout.end_object();
}

int relic::activate( Creature &caster, const tripoint &target ) const
{
    caster.moves -= moves;
    for( const fake_spell &sp : active_effects ) {
        sp.get_spell().cast_all_effects( caster, target );
    }
    return charges_per_activation;
}

int relic::modify_value( const enchantment::mod value_type, const int value ) const
{
    int add_modifier = 0;
    double multiply_modifier = 0.0;
    for( const enchantment &ench : passive_effects ) {
        add_modifier += ench.get_value_add( value_type );
        multiply_modifier += ench.get_value_multiply( value_type );
    }
    multiply_modifier = std::max( multiply_modifier + 1.0, 0.0 );
    int modified_value;
    if( multiply_modifier < 1.0 ) {
        modified_value = floor( multiply_modifier * value );
    } else {
        modified_value = ceil( multiply_modifier * value );
    }
    return modified_value + add_modifier;
}

std::string relic::name() const
{
    return item_name_override.translated();
}

std::vector<enchantment> relic::get_enchantments() const
{
    return passive_effects;
}
