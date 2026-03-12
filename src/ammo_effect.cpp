#include "ammo_effect.h"

#include "debug.h"
#include "generic_factory.h"

generic_factory<ammo_effect> &get_all_ammo_effects()
{
    static generic_factory<ammo_effect> all_ammo_effects( "ammo effects" );
    return all_ammo_effects;
}

/** @relates int_id */
template<>
bool int_id<ammo_effect>::is_valid() const
{
    return get_all_ammo_effects().is_valid( *this );
}

/** @relates int_id */
template<>
const ammo_effect &int_id<ammo_effect>::obj() const
{
    return get_all_ammo_effects().obj( *this );
}

/** @relates int_id */
template<>
const ammo_effect_str_id &int_id<ammo_effect>::id() const
{
    return get_all_ammo_effects().convert( *this );
}

/** @relates string_id */
template<>
bool ammo_effect_str_id::is_valid() const
{
    return get_all_ammo_effects().is_valid( *this );
}

/** @relates string_id */
template<>
const ammo_effect &ammo_effect_str_id::obj() const
{
    return get_all_ammo_effects().obj( *this );
}

/** @relates string_id */
template<>
int_id<ammo_effect> ammo_effect_str_id::id() const
{
    return get_all_ammo_effects().convert( *this, AE_NULL );
}

/** @relates int_id */
template<>
int_id<ammo_effect>::int_id( const ammo_effect_str_id &id ) : _id( id.id() )
{
}

void aoe_effect::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "effect", effect );
    mandatory( jo, was_loaded, "duration", duration );
    optional( jo, was_loaded, "intensity_min", intensity_min, numeric_bound_reader<int> {0}, 1 );
    optional( jo, was_loaded, "intensity_max", intensity_max, numeric_bound_reader<int> { intensity_min },
              intensity_min );
    optional( jo, was_loaded, "chance", chance, numeric_bound_reader<int> {0, 100}, 100 );
    optional( jo, was_loaded, "radius", radius, numeric_bound_reader<int> {0}, 1 );
}

void on_hit_effect::deserialize( const JsonObject &jo )
{
    optional( jo, was_loaded, "need_touch_skin", need_touch_skin, false );
    mandatory( jo, was_loaded, "duration", duration );
    mandatory( jo, was_loaded, "effect", effect );
    mandatory( jo, was_loaded, "intensity", intensity );
}

void aoe_field_effect::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "field_type", field_type );
    optional( jo, was_loaded, "intensity_min", intensity_min, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "intensity_max", intensity_max, numeric_bound_reader<int> { intensity_min },
              intensity_min );
    optional( jo, was_loaded, "radius", radius, numeric_bound_reader<int> {0}, 1 );
    optional( jo, was_loaded, "radius_z", radius_z, numeric_bound_reader<int> {0},  0 );
    optional( jo, was_loaded, "chance", chance, numeric_bound_reader<int> {0, 100}, 100 );
    optional( jo, was_loaded, "size", size, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "check_passable", check_passable, false );
}

void trail_field_effect::deserialize( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "field_type", field_type );
    optional( jo, was_loaded, "intensity_min", intensity_min, numeric_bound_reader<int> {0}, 0 );
    optional( jo, was_loaded, "intensity_max", intensity_max, numeric_bound_reader<int> { intensity_min },
              intensity_min );
    optional( jo, was_loaded, "chance", chance, numeric_bound_reader<int> {0, 100}, 100 );
}

void ammo_effect::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "trigger_chance", trigger_chance, 100 );
    optional( jo, was_loaded, "aoe", aoe_field_types );
    optional( jo, was_loaded, "trail", trail_field_types );
    optional( jo, was_loaded, "on_hit_effects", on_hit_effects );
    optional( jo, was_loaded, "aoe_effects", aoe_effects );
    optional( jo, was_loaded, "explosion", aoe_explosion_data );
    optional( jo, was_loaded, "do_flashbang", do_flashbang, false );
    optional( jo, was_loaded, "do_emp_blast", do_emp_blast, false );
    optional( jo, was_loaded, "foamcrete_build", foamcrete_build, false );
    optional( jo, was_loaded, "eoc", eoc );
    optional( jo, was_loaded, "spell_data", spell_data );
    optional( jo, was_loaded, "always_cast_spell", always_cast_spell, false );
}

void ammo_effect::check() const
{
    for( const aoe_field_effect &aoe : aoe_field_types ) {
        if( !aoe.field_type.is_valid() ) {
            debugmsg( "Ammo effect %s aoe section uses invalid field type %s", id.str(), aoe.field_type.str() );
        }
    }

    for( const trail_field_effect &trail : trail_field_types ) {
        if( !trail.field_type.is_valid() ) {
            debugmsg( "Ammo effect %s trail section uses invalid field type %s", id.str(),
                      trail.field_type.str() );
        }
    }

    for( const fake_spell &spell : spell_data ) {
        if( !spell.is_valid() ) {
            debugmsg( "Ammo effect %s spell section uses invalid spell id %s", id.str(), spell.id.str() );
        }
    }
}

size_t ammo_effect::count()
{
    return get_all_ammo_effects().size();
}

void ammo_effects::load( const JsonObject &jo, const std::string &src )
{
    get_all_ammo_effects().load( jo, src );
}

void ammo_effects::finalize_all()
{
    get_all_ammo_effects().finalize();
}

void ammo_effects::check_consistency()
{
    get_all_ammo_effects().check();
}

void ammo_effects::reset()
{
    get_all_ammo_effects().reset();
}

const std::vector<ammo_effect> &ammo_effects::get_all()
{
    return get_all_ammo_effects().get_all();
}

ammo_effect_id AE_NULL;
