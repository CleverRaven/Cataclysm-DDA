#include "field_type.h"

#include "debug.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"
#include "optional.h"

namespace
{

generic_factory<field_type> all_field_types( "field types" );

} // namespace

/** @relates int_id */
template<>
bool int_id<field_type>::is_valid() const
{
    return all_field_types.is_valid( *this );
}

/** @relates int_id */
template<>
const field_type &int_id<field_type>::obj() const
{
    return all_field_types.obj( *this );
}

/** @relates int_id */
template<>
const string_id<field_type> &int_id<field_type>::id() const
{
    return all_field_types.convert( *this );
}

/** @relates string_id */
template<>
bool string_id<field_type>::is_valid() const
{
    return all_field_types.is_valid( *this );
}

/** @relates string_id */
template<>
const field_type &string_id<field_type>::obj() const
{
    return all_field_types.obj( *this );
}

/** @relates string_id */
template<>
int_id<field_type> string_id<field_type>::id() const
{
    return all_field_types.convert( *this, x_fd_null );
}

/** @relates int_id */
template<>
int_id<field_type>::int_id( const string_id<field_type> &id ) : _id( id.id() )
{
}

void field_type::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "legacy_enum_id", legacy_enum_id );
    JsonArray ja = jo.get_array( "intensity_levels" );
    for( size_t i = 0; i < ja.size(); ++i ) {
        field_intensity_level fil;
        field_intensity_level fallback = i > 0 ? intensity_levels[i - 1] : fil;
        JsonObject jao = ja.get_object( i );
        optional( jao, was_loaded, "name", fil.name, fallback.name );
        optional( jao, was_loaded, "sym", fil.symbol, unicode_codepoint_from_symbol_reader,
                  fallback.symbol );
        fil.color = jao.has_member( "color" ) ? color_from_string( jao.get_string( "color" ) ) :
                    fallback.color;
        optional( jao, was_loaded, "transparent", fil.transparent, fallback.transparent );
        optional( jao, was_loaded, "dangerous", fil.dangerous, fallback.dangerous );
        optional( jao, was_loaded, "move_cost", fil.move_cost, fallback.move_cost );
        intensity_levels.emplace_back( fil );
    }
    optional( jo, was_loaded, "priority", priority );
    optional( jo, was_loaded, "half_life", half_life );
    if( jo.has_member( "phase" ) ) {
        phase = jo.get_enum_value<phase_id>( "phase" );
    }
    optional( jo, was_loaded, "accelerated_decay", accelerated_decay );
    optional( jo, was_loaded, "display_items", display_items );
    optional( jo, was_loaded, "display_field", display_field );
}

void field_type::check() const
{
    if( intensity_levels.empty() ) {
        debugmsg( "No intensity levels defined for field type \"%s\".", id.c_str() );
    }
    int i = 0;
    for( auto &il : intensity_levels ) {
        i++;
        if( il.name.empty() ) {
            debugmsg( "Intensity level %d defined for field type \"%s\" has empty name.", i, id.c_str() );
        }
    }
}

size_t field_type::count()
{
    return all_field_types.size();
}

void field_types::load( JsonObject &jo, const std::string &src )
{
    all_field_types.load( jo, src );
}

void field_types::finalize_all()
{
    set_field_type_ids();
}

void field_types::check_consistency()
{
    all_field_types.check();
}

void field_types::reset()
{
    all_field_types.reset();
}

const std::vector<field_type> &field_types::get_all()
{
    return all_field_types.get_all();
}

field_type_id x_fd_null,
              x_fd_blood,
              x_fd_bile,
              x_fd_gibs_flesh,
              x_fd_gibs_veggy,
              x_fd_web,
              x_fd_slime,
              x_fd_acid,
              x_fd_sap,
              x_fd_sludge,
              x_fd_fire,
              x_fd_rubble,
              x_fd_smoke,
              x_fd_toxic_gas,
              x_fd_tear_gas,
              x_fd_nuke_gas,
              x_fd_gas_vent,
              x_fd_fire_vent,
              x_fd_flame_burst,
              x_fd_electricity,
              x_fd_fatigue,
              x_fd_push_items,
              x_fd_shock_vent,
              x_fd_acid_vent,
              x_fd_plasma,
              x_fd_laser,
              x_fd_spotlight,
              x_fd_dazzling,
              x_fd_blood_veggy,
              x_fd_blood_insect,
              x_fd_blood_invertebrate,
              x_fd_gibs_insect,
              x_fd_gibs_invertebrate,
              x_fd_cigsmoke,
              x_fd_weedsmoke,
              x_fd_cracksmoke,
              x_fd_methsmoke,
              x_fd_bees,
              x_fd_incendiary,
              x_fd_relax_gas,
              x_fd_fungal_haze,
              x_fd_cold_air1,
              x_fd_cold_air2,
              x_fd_cold_air3,
              x_fd_cold_air4,
              x_fd_hot_air1,
              x_fd_hot_air2,
              x_fd_hot_air3,
              x_fd_hot_air4,
              x_fd_fungicidal_gas,
              x_fd_smoke_vent
              ;

void field_types::set_field_type_ids()
{
    x_fd_null = field_type_id( "fd_null" );
    x_fd_blood = field_type_id( "fd_blood" );
    x_fd_bile = field_type_id( "fd_bile" );
    x_fd_gibs_flesh = field_type_id( "fd_gibs_flesh" );
    x_fd_gibs_veggy = field_type_id( "fd_gibs_veggy" );
    x_fd_web = field_type_id( "fd_web" );
    x_fd_slime = field_type_id( "fd_slime" );
    x_fd_acid = field_type_id( "fd_acid" );
    x_fd_sap = field_type_id( "fd_sap" );
    x_fd_sludge = field_type_id( "fd_sludge" );
    x_fd_fire = field_type_id( "fd_fire" );
    x_fd_rubble = field_type_id( "fd_rubble" );
    x_fd_smoke = field_type_id( "fd_smoke" );
    x_fd_toxic_gas = field_type_id( "fd_toxic_gas" );
    x_fd_tear_gas = field_type_id( "fd_tear_gas" );
    x_fd_nuke_gas = field_type_id( "fd_nuke_gas" );
    x_fd_gas_vent = field_type_id( "fd_gas_vent" );
    x_fd_fire_vent = field_type_id( "fd_fire_vent" );
    x_fd_flame_burst = field_type_id( "fd_flame_burst" );
    x_fd_electricity = field_type_id( "fd_electricity" );
    x_fd_fatigue = field_type_id( "fd_fatigue" );
    x_fd_push_items = field_type_id( "fd_push_items" );
    x_fd_shock_vent = field_type_id( "fd_shock_vent" );
    x_fd_acid_vent = field_type_id( "fd_acid_vent" );
    x_fd_plasma = field_type_id( "fd_plasma" );
    x_fd_laser = field_type_id( "fd_laser" );
    x_fd_spotlight = field_type_id( "fd_spotlight" );
    x_fd_dazzling = field_type_id( "fd_dazzling" );
    x_fd_blood_veggy = field_type_id( "fd_blood_veggy" );
    x_fd_blood_insect = field_type_id( "fd_blood_insect" );
    x_fd_blood_invertebrate = field_type_id( "fd_blood_invertebrate" );
    x_fd_gibs_insect = field_type_id( "fd_gibs_insect" );
    x_fd_gibs_invertebrate = field_type_id( "fd_gibs_invertebrate" );
    x_fd_cigsmoke = field_type_id( "fd_cigsmoke" );
    x_fd_weedsmoke = field_type_id( "fd_weedsmoke" );
    x_fd_cracksmoke = field_type_id( "fd_cracksmoke" );
    x_fd_methsmoke = field_type_id( "fd_methsmoke" );
    x_fd_bees = field_type_id( "fd_bees" );
    x_fd_incendiary = field_type_id( "fd_incendiary" );
    x_fd_relax_gas = field_type_id( "fd_relax_gas" );
    x_fd_fungal_haze = field_type_id( "fd_fungal_haze" );
    x_fd_cold_air1 = field_type_id( "fd_cold_air1" );
    x_fd_cold_air2 = field_type_id( "fd_cold_air2" );
    x_fd_cold_air3 = field_type_id( "fd_cold_air3" );
    x_fd_cold_air4 = field_type_id( "fd_cold_air4" );
    x_fd_hot_air1 = field_type_id( "fd_hot_air1" );
    x_fd_hot_air2 = field_type_id( "fd_hot_air2" );
    x_fd_hot_air3 = field_type_id( "fd_hot_air3" );
    x_fd_hot_air4 = field_type_id( "fd_hot_air4" );
    x_fd_fungicidal_gas = field_type_id( "fd_fungicidal_gas" );
    x_fd_smoke_vent = field_type_id( "fd_smoke_vent" );
}

field_type field_types::get_field_type_by_legacy_enum( int legacy_enum_id )
{
    for( const auto &ft : all_field_types.get_all() ) {
        if( legacy_enum_id == ft.legacy_enum_id ) {
            return ft;
        }
    }
    debugmsg( "Cannot find field_type for legacy enum: %d.", legacy_enum_id );
    return field_type();
}
