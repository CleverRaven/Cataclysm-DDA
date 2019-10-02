#include "ammo_effect.h"

#include "generic_factory.h"
#include "json.h"

namespace
{

generic_factory<ammo_effect> all_ammo_effects( "ammo effects" );

} // namespace

/** @relates int_id */
template<>
bool int_id<ammo_effect>::is_valid() const
{
    return all_ammo_effects.is_valid( *this );
}

/** @relates int_id */
template<>
const ammo_effect &int_id<ammo_effect>::obj() const
{
    return all_ammo_effects.obj( *this );
}

/** @relates int_id */
template<>
const string_id<ammo_effect> &int_id<ammo_effect>::id() const
{
    return all_ammo_effects.convert( *this );
}

/** @relates string_id */
template<>
bool string_id<ammo_effect>::is_valid() const
{
    return all_ammo_effects.is_valid( *this );
}

/** @relates string_id */
template<>
const ammo_effect &string_id<ammo_effect>::obj() const
{
    return all_ammo_effects.obj( *this );
}

/** @relates string_id */
template<>
int_id<ammo_effect> string_id<ammo_effect>::id() const
{
    return all_ammo_effects.convert( *this, AE_NULL );
}

/** @relates int_id */
template<>
int_id<ammo_effect>::int_id( const string_id<ammo_effect> &id ) : _id( id.id() )
{
}

void ammo_effect::load( JsonObject &jo, const std::string & )
{
    if( jo.has_member( "aoe" ) ) {
        JsonObject joa = jo.get_object( "aoe" );
        optional( joa, was_loaded, "field_type", aoe_field_type_name, "fd_null" );
        optional( joa, was_loaded, "intensity_min", aoe_intensity_min, 0 );
        optional( joa, was_loaded, "intensity_max", aoe_intensity_max, 0 );
        optional( joa, was_loaded, "radius", aoe_radius, 1 );
        optional( joa, was_loaded, "radius_z", aoe_radius_z, 0 );
        optional( joa, was_loaded, "chance", aoe_chance, 1 );
        optional( joa, was_loaded, "size", aoe_size, 0 );
        optional( joa, was_loaded, "check_passable", aoe_check_passable, false );
        optional( joa, was_loaded, "check_sees", aoe_check_sees, false );
        optional( joa, was_loaded, "check_sees_radius", aoe_check_sees_radius, 0 );
    }
    if( jo.has_member( "explosion" ) ) {
        JsonObject joe = jo.get_object( "explosion" );
        aoe_explosion_data = load_explosion_data( joe );
    }
    optional( jo, was_loaded, "do_flashbang", do_flashbang, false );
    optional( jo, was_loaded, "do_emp_blast", do_emp_blast, false );
}

void ammo_effect::finalize()
{
    for( const ammo_effect &ae : ammo_effects::get_all() ) {
        const_cast<ammo_effect &>( ae ).aoe_field_type = field_type_id( ae.aoe_field_type_name );
    }

}

void ammo_effect::check() const
{
}

size_t ammo_effect::count()
{
    return all_ammo_effects.size();
}

void ammo_effects::load( JsonObject &jo, const std::string &src )
{
    all_ammo_effects.load( jo, src );
}

void ammo_effects::finalize_all()
{
    all_ammo_effects.finalize();
    for( const ammo_effect &ae : all_ammo_effects.get_all() ) {
        const_cast<ammo_effect &>( ae ).finalize();
    }
}

void ammo_effects::check_consistency()
{
    all_ammo_effects.check();
}

void ammo_effects::reset()
{
    all_ammo_effects.reset();
}

const std::vector<ammo_effect> &ammo_effects::get_all()
{
    return all_ammo_effects.get_all();
}

ammo_effect_id AE_NULL;
