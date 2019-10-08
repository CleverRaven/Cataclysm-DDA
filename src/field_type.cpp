#include "field_type.h"

#include <set>

#include "bodypart.h"
#include "debug.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"
#include "int_id.h"

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
    return all_field_types.convert( *this, fd_null );
}

/** @relates int_id */
template<>
int_id<field_type>::int_id( const string_id<field_type> &id ) : _id( id.id() )
{
}

const field_intensity_level &field_type::get_intensity_level( int level ) const
{
    if( level < 0 || static_cast<size_t>( level ) >= intensity_levels.size() ) {
        // level + 1 for the original intensity number
        debugmsg( "Unknown intensity level %d for field type %s.", level + 1, id.str() );
        return intensity_levels.back();
    }
    return intensity_levels[level];
}

void field_type::load( JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "legacy_enum_id", legacy_enum_id, -1 );
    JsonArray ja = jo.get_array( "intensity_levels" );
    if( !jo.has_array( "intensity_levels" ) || ja.empty() ) {
        jo.throw_error( "No intensity levels defined for field type", "id" );
    }
    for( size_t i = 0; i < ja.size(); ++i ) {
        field_intensity_level intensity_level;
        field_intensity_level fallback_intensity_level = i > 0 ? intensity_levels[i - 1] : intensity_level;
        JsonObject jao = ja.get_object( i );
        optional( jao, was_loaded, "name", intensity_level.name, fallback_intensity_level.name );
        optional( jao, was_loaded, "sym", intensity_level.symbol, unicode_codepoint_from_symbol_reader,
                  fallback_intensity_level.symbol );
        intensity_level.color = jao.has_member( "color" ) ? color_from_string( jao.get_string( "color" ) ) :
                                fallback_intensity_level.color;
        optional( jao, was_loaded, "transparent", intensity_level.transparent,
                  fallback_intensity_level.transparent );
        optional( jao, was_loaded, "dangerous", intensity_level.dangerous,
                  fallback_intensity_level.dangerous );
        optional( jao, was_loaded, "move_cost", intensity_level.move_cost,
                  fallback_intensity_level.move_cost );
        optional( jao, was_loaded, "extra_radiation_min", intensity_level.extra_radiation_min,
                  fallback_intensity_level.extra_radiation_min );
        optional( jao, was_loaded, "extra_radiation_max", intensity_level.extra_radiation_max,
                  fallback_intensity_level.extra_radiation_max );
        optional( jao, was_loaded, "radiation_hurt_damage_min", intensity_level.radiation_hurt_damage_min,
                  fallback_intensity_level.radiation_hurt_damage_min );
        optional( jao, was_loaded, "radiation_hurt_damage_max", intensity_level.radiation_hurt_damage_max,
                  fallback_intensity_level.radiation_hurt_damage_max );
        optional( jao, was_loaded, "radiation_hurt_message", intensity_level.radiation_hurt_message,
                  fallback_intensity_level.radiation_hurt_message );
        optional( jao, was_loaded, "intensity_upgrade_chance", intensity_level.intensity_upgrade_chance,
                  fallback_intensity_level.intensity_upgrade_chance );
        optional( jao, was_loaded, "intensity_upgrade_duration", intensity_level.intensity_upgrade_duration,
                  fallback_intensity_level.intensity_upgrade_duration );
        optional( jao, was_loaded, "monster_spawn_chance", intensity_level.monster_spawn_chance,
                  fallback_intensity_level.monster_spawn_chance );
        optional( jao, was_loaded, "monster_spawn_count", intensity_level.monster_spawn_count,
                  fallback_intensity_level.monster_spawn_count );
        optional( jao, was_loaded, "monster_spawn_radius", intensity_level.monster_spawn_radius,
                  fallback_intensity_level.monster_spawn_radius );
        optional( jao, was_loaded, "monster_spawn_group", intensity_level.monster_spawn_group,
                  fallback_intensity_level.monster_spawn_group );
        optional( jao, was_loaded, "light_emitted", intensity_level.light_emitted,
                  fallback_intensity_level.light_emitted );
        optional( jao, was_loaded, "translucency", intensity_level.translucency,
                  fallback_intensity_level.translucency );
        optional( jao, was_loaded, "convection_temperature_mod", intensity_level.convection_temperature_mod,
                  fallback_intensity_level.convection_temperature_mod );
        optional( jao, was_loaded, "effect_id", intensity_level.field_effect.id,
                  fallback_intensity_level.field_effect.id );
        optional( jao, was_loaded, "effect_min_duration", intensity_level.field_effect.min_duration,
                  fallback_intensity_level.field_effect.min_duration );
        optional( jao, was_loaded, "effect_max_duration", intensity_level.field_effect.max_duration,
                  fallback_intensity_level.field_effect.max_duration );
        optional( jao, was_loaded, "effect_intensity", intensity_level.field_effect.intensity,
                  fallback_intensity_level.field_effect.intensity );
        optional( jao, was_loaded, "effect_body_part", intensity_level.field_effect.bp,
                  fallback_intensity_level.field_effect.bp );
        optional( jao, was_loaded, "inside_immune", intensity_level.field_effect.inside_immune,
                  fallback_intensity_level.field_effect.inside_immune );
        intensity_levels.emplace_back( intensity_level );
    }

    if( jo.has_object( "npc_complain" ) ) {
        JsonObject joc = jo.get_object( "npc_complain" );
        int chance;
        std::string issue;
        time_duration duration;
        std::string speech;
        optional( joc, was_loaded, "chance", chance, 0 );
        optional( joc, was_loaded, "issue", issue );
        optional( joc, was_loaded, "duration", duration, 0_turns );
        optional( joc, was_loaded, "speech", speech );
        npc_complain_data = std::make_tuple( chance, issue, duration, speech );
    }

    JsonObject jid = jo.get_object( "immunity_data" );
    JsonArray jidt = jid.get_array( "traits" );
    while( jidt.has_more() ) {
        immunity_data_traits.emplace_back( trait_id( jidt.next_string() ) );
    }
    JsonArray jidr = jid.get_array( "body_part_env_resistance" );
    while( jidr.has_more() ) {
        JsonArray jao = jidr.next_array();
        immunity_data_body_part_env_resistance.emplace_back( std::make_pair( get_body_part_token(
                    jao.get_string( 0 ) ), jao.get_int( 1 ) ) );
    }
    optional( jo, was_loaded, "underwater_age_speedup", underwater_age_speedup, 0_turns );
    optional( jo, was_loaded, "outdoor_age_speedup", outdoor_age_speedup, 0_turns );
    optional( jo, was_loaded, "decay_amount_factor", decay_amount_factor, 0 );
    optional( jo, was_loaded, "percent_spread", percent_spread, 0 );
    optional( jo, was_loaded, "apply_slime_factor", apply_slime_factor, 0 );
    optional( jo, was_loaded, "gas_absorption_factor", gas_absorption_factor, 0 );
    optional( jo, was_loaded, "is_splattering", is_splattering, false );
    optional( jo, was_loaded, "dirty_transparency_cache", dirty_transparency_cache, false );
    optional( jo, was_loaded, "has_fire", has_fire, false );
    optional( jo, was_loaded, "has_acid", has_acid, false );
    optional( jo, was_loaded, "has_elec", has_elec, false );
    optional( jo, was_loaded, "has_fume", has_fume, false );
    optional( jo, was_loaded, "priority", priority, 0 );
    optional( jo, was_loaded, "half_life", half_life, 0_turns );
    if( jo.has_member( "phase" ) ) {
        phase = jo.get_enum_value<phase_id>( "phase", PNULL );
    }
    optional( jo, was_loaded, "accelerated_decay", accelerated_decay, false );
    optional( jo, was_loaded, "display_items", display_items, true );
    optional( jo, was_loaded, "display_field", display_field, false );
    optional( jo, was_loaded, "wandering_field", wandering_field_id, "fd_null" );
}

void field_type::finalize()
{
    wandering_field = field_type_id( wandering_field_id );
    wandering_field_id.clear();
}

void field_type::check() const
{
    int i = 0;
    for( auto &intensity_level : intensity_levels ) {
        i++;
        if( intensity_level.name.empty() ) {
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
    all_field_types.finalize();
    for( const field_type &fd : all_field_types.get_all() ) {
        const_cast<field_type &>( fd ).finalize();
    }
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

field_type_id fd_null,
              fd_blood,
              fd_bile,
              fd_gibs_flesh,
              fd_gibs_veggy,
              fd_web,
              fd_slime,
              fd_acid,
              fd_sap,
              fd_sludge,
              fd_fire,
              fd_rubble,
              fd_smoke,
              fd_toxic_gas,
              fd_tear_gas,
              fd_nuke_gas,
              fd_gas_vent,
              fd_fire_vent,
              fd_flame_burst,
              fd_electricity,
              fd_fatigue,
              fd_push_items,
              fd_shock_vent,
              fd_acid_vent,
              fd_plasma,
              fd_laser,
              fd_spotlight,
              fd_dazzling,
              fd_blood_veggy,
              fd_blood_insect,
              fd_blood_invertebrate,
              fd_gibs_insect,
              fd_gibs_invertebrate,
              fd_cigsmoke,
              fd_weedsmoke,
              fd_cracksmoke,
              fd_methsmoke,
              fd_bees,
              fd_incendiary,
              fd_relax_gas,
              fd_fungal_haze,
              fd_cold_air1,
              fd_cold_air2,
              fd_cold_air3,
              fd_cold_air4,
              fd_hot_air1,
              fd_hot_air2,
              fd_hot_air3,
              fd_hot_air4,
              fd_fungicidal_gas,
              fd_insecticidal_gas,
              fd_smoke_vent,
              fd_tindalos_rift
              ;

void field_types::set_field_type_ids()
{
    fd_null = field_type_id( "fd_null" );
    fd_blood = field_type_id( "fd_blood" );
    fd_bile = field_type_id( "fd_bile" );
    fd_gibs_flesh = field_type_id( "fd_gibs_flesh" );
    fd_gibs_veggy = field_type_id( "fd_gibs_veggy" );
    fd_web = field_type_id( "fd_web" );
    fd_slime = field_type_id( "fd_slime" );
    fd_acid = field_type_id( "fd_acid" );
    fd_sap = field_type_id( "fd_sap" );
    fd_sludge = field_type_id( "fd_sludge" );
    fd_fire = field_type_id( "fd_fire" );
    fd_rubble = field_type_id( "fd_rubble" );
    fd_smoke = field_type_id( "fd_smoke" );
    fd_toxic_gas = field_type_id( "fd_toxic_gas" );
    fd_tear_gas = field_type_id( "fd_tear_gas" );
    fd_nuke_gas = field_type_id( "fd_nuke_gas" );
    fd_gas_vent = field_type_id( "fd_gas_vent" );
    fd_fire_vent = field_type_id( "fd_fire_vent" );
    fd_flame_burst = field_type_id( "fd_flame_burst" );
    fd_electricity = field_type_id( "fd_electricity" );
    fd_fatigue = field_type_id( "fd_fatigue" );
    fd_push_items = field_type_id( "fd_push_items" );
    fd_shock_vent = field_type_id( "fd_shock_vent" );
    fd_acid_vent = field_type_id( "fd_acid_vent" );
    fd_plasma = field_type_id( "fd_plasma" );
    fd_laser = field_type_id( "fd_laser" );
    fd_spotlight = field_type_id( "fd_spotlight" );
    fd_dazzling = field_type_id( "fd_dazzling" );
    fd_blood_veggy = field_type_id( "fd_blood_veggy" );
    fd_blood_insect = field_type_id( "fd_blood_insect" );
    fd_blood_invertebrate = field_type_id( "fd_blood_invertebrate" );
    fd_gibs_insect = field_type_id( "fd_gibs_insect" );
    fd_gibs_invertebrate = field_type_id( "fd_gibs_invertebrate" );
    fd_cigsmoke = field_type_id( "fd_cigsmoke" );
    fd_weedsmoke = field_type_id( "fd_weedsmoke" );
    fd_cracksmoke = field_type_id( "fd_cracksmoke" );
    fd_methsmoke = field_type_id( "fd_methsmoke" );
    fd_bees = field_type_id( "fd_bees" );
    fd_incendiary = field_type_id( "fd_incendiary" );
    fd_relax_gas = field_type_id( "fd_relax_gas" );
    fd_fungal_haze = field_type_id( "fd_fungal_haze" );
    fd_cold_air1 = field_type_id( "fd_cold_air1" );
    fd_cold_air2 = field_type_id( "fd_cold_air2" );
    fd_cold_air3 = field_type_id( "fd_cold_air3" );
    fd_cold_air4 = field_type_id( "fd_cold_air4" );
    fd_hot_air1 = field_type_id( "fd_hot_air1" );
    fd_hot_air2 = field_type_id( "fd_hot_air2" );
    fd_hot_air3 = field_type_id( "fd_hot_air3" );
    fd_hot_air4 = field_type_id( "fd_hot_air4" );
    fd_fungicidal_gas = field_type_id( "fd_fungicidal_gas" );
    fd_insecticidal_gas = field_type_id( "fd_insecticidal_gas" );
    fd_smoke_vent = field_type_id( "fd_smoke_vent" );
    fd_tindalos_rift = field_type_id( "fd_tindalos_rift" );

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
