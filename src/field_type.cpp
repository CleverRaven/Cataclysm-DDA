#include "field_type.h"

#include <cstdlib>

#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "generic_factory.h"
#include "json.h"

const field_type_str_id fd_null = field_type_str_id::NULL_ID();
const field_type_str_id fd_acid( "fd_acid" );
const field_type_str_id fd_acid_vent( "fd_acid_vent" );
const field_type_str_id fd_bile( "fd_bile" );
const field_type_str_id fd_blood( "fd_blood" );
const field_type_str_id fd_blood_insect( "fd_blood_insect" );
const field_type_str_id fd_blood_invertebrate( "fd_blood_invertebrate" );
const field_type_str_id fd_blood_veggy( "fd_blood_veggy" );
const field_type_str_id fd_churned_earth( "fd_churned_earth" );
const field_type_str_id fd_cold_air2( "fd_cold_air2" );
const field_type_str_id fd_cold_air3( "fd_cold_air3" );
const field_type_str_id fd_cold_air4( "fd_cold_air4" );
const field_type_str_id fd_dazzling( "fd_dazzling" );
const field_type_str_id fd_electricity( "fd_electricity" );
const field_type_str_id fd_electricity_unlit( "fd_electricity_unlit" );
const field_type_str_id fd_extinguisher( "fd_extinguisher" );
const field_type_str_id fd_fatigue( "fd_fatigue" );
const field_type_str_id fd_fire( "fd_fire" );
const field_type_str_id fd_fire_vent( "fd_fire_vent" );
const field_type_str_id fd_flame_burst( "fd_flame_burst" );
const field_type_str_id fd_fungal_haze( "fd_fungal_haze" );
const field_type_str_id fd_fungicidal_gas( "fd_fungicidal_gas" );
const field_type_str_id fd_gas_vent( "fd_gas_vent" );
const field_type_str_id fd_gibs_flesh( "fd_gibs_flesh" );
const field_type_str_id fd_gibs_insect( "fd_gibs_insect" );
const field_type_str_id fd_gibs_invertebrate( "fd_gibs_invertebrate" );
const field_type_str_id fd_gibs_veggy( "fd_gibs_veggy" );
const field_type_str_id fd_hot_air1( "fd_hot_air1" );
const field_type_str_id fd_hot_air2( "fd_hot_air2" );
const field_type_str_id fd_hot_air3( "fd_hot_air3" );
const field_type_str_id fd_hot_air4( "fd_hot_air4" );
const field_type_str_id fd_incendiary( "fd_incendiary" );
const field_type_str_id fd_insecticidal_gas( "fd_insecticidal_gas" );
const field_type_str_id fd_laser( "fd_laser" );
const field_type_str_id fd_last_known( "fd_last_known" );
const field_type_str_id fd_nuke_gas( "fd_nuke_gas" );
const field_type_str_id fd_plasma( "fd_plasma" );
const field_type_str_id fd_push_items( "fd_push_items" );
const field_type_str_id fd_relax_gas( "fd_relax_gas" );
const field_type_str_id fd_sap( "fd_sap" );
const field_type_str_id fd_shock_vent( "fd_shock_vent" );
const field_type_str_id fd_slime( "fd_slime" );
const field_type_str_id fd_sludge( "fd_sludge" );
const field_type_str_id fd_smoke( "fd_smoke" );
const field_type_str_id fd_smoke_vent( "fd_smoke_vent" );
const field_type_str_id fd_tear_gas( "fd_tear_gas" );
const field_type_str_id fd_tindalos_rift( "fd_tindalos_rift" );
const field_type_str_id fd_toxic_gas( "fd_toxic_gas" );
const field_type_str_id fd_web( "fd_web" );

namespace io
{

template<>
std::string enum_to_string<game_message_type>( game_message_type data )
{
    switch( data ) {
            // *INDENT-OFF*
        case game_message_type::m_good: return "good";
        case game_message_type::m_bad: return "bad";
        case game_message_type::m_mixed: return "mixed";
        case game_message_type::m_warning: return "warning";
        case game_message_type::m_info: return "info";
        case game_message_type::m_neutral: return "neutral";
        case game_message_type::m_debug: return "debug";
        case game_message_type::m_headshot: return "headshot";
        case game_message_type::m_critical: return "critical";
        case game_message_type::m_grazing: return "grazing";
            // *INDENT-ON*
        case game_message_type::num_game_message_type:
            break;
    }
    cata_fatal( "Invalid game_message_type" );
}

template<>
std::string enum_to_string<description_affix>( description_affix data )
{
    switch( data ) {
        // *INDENT-OFF*
        case description_affix::DESCRIPTION_AFFIX_IN: return "in";
        case description_affix::DESCRIPTION_AFFIX_COVERED_IN: return "covered_in";
        case description_affix::DESCRIPTION_AFFIX_ON: return "on";
        case description_affix::DESCRIPTION_AFFIX_UNDER: return "under";
        case description_affix::DESCRIPTION_AFFIX_ILLUMINATED_BY: return "illuminated_by";
        // *INDENT-ON*
        case description_affix::DESCRIPTION_AFFIX_NUM:
            break;
    }
    debugmsg( "Invalid description affix value '%d'.", data );
    return "invalid";
}

} // namespace io

generic_factory<field_type> &get_all_field_types()
{
    static generic_factory<field_type> all_field_types( "field types" );
    return all_field_types;
}

/** @relates int_id */
template<>
bool int_id<field_type>::is_valid() const
{
    return get_all_field_types().is_valid( *this );
}

/** @relates int_id */
template<>
const field_type &int_id<field_type>::obj() const
{
    return get_all_field_types().obj( *this );
}

/** @relates int_id */
template<>
const string_id<field_type> &int_id<field_type>::id() const
{
    return get_all_field_types().convert( *this );
}

/** @relates string_id */
template<>
bool string_id<field_type>::is_valid() const
{
    return get_all_field_types().is_valid( *this );
}

/** @relates string_id */
template<>
const field_type &string_id<field_type>::obj() const
{
    return get_all_field_types().obj( *this );
}

template<>
int_id<field_type> string_id<field_type>::id_or( const int_id<field_type> &fallback ) const
{
    if( get_all_field_types().initialized ) {
        return get_all_field_types().convert( *this, fallback, false );
    }
    return fallback;
}

/** @relates string_id */
template<>
int_id<field_type> string_id<field_type>::id() const
{
    return get_all_field_types().convert( *this, fd_null.id_or( int_id<field_type>() ) );
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

void field_type::load( const JsonObject &jo, const std::string_view )
{
    optional( jo, was_loaded, "legacy_enum_id", legacy_enum_id, -1 );
    for( const JsonObject jao : jo.get_array( "intensity_levels" ) ) {
        field_intensity_level intensity_level;
        field_intensity_level fallback_intensity_level = !intensity_levels.empty() ? intensity_levels.back()
                : intensity_level;
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
        optional( jao, was_loaded, "light_override", intensity_level.local_light_override,
                  fallback_intensity_level.local_light_override );
        optional( jao, was_loaded, "translucency", intensity_level.translucency,
                  fallback_intensity_level.translucency );
        optional( jao, was_loaded, "concentration", intensity_level.concentration,
                  1 );
        optional( jao, was_loaded, "convection_temperature_mod", intensity_level.convection_temperature_mod,
                  fallback_intensity_level.convection_temperature_mod );
        if( jao.has_array( "effects" ) ) {
            for( const JsonObject joe : jao.get_array( "effects" ) ) {
                field_effect fe;
                mandatory( joe, was_loaded, "effect_id", fe.id );
                optional( joe, was_loaded, "min_duration", fe.min_duration );
                optional( joe, was_loaded, "max_duration", fe.max_duration );
                optional( joe, was_loaded, "intensity", fe.intensity );
                optional( joe, was_loaded, "body_part", fe.bp, bodypart_str_id::NULL_ID() );
                optional( joe, was_loaded, "is_environmental", fe.is_environmental );
                optional( joe, was_loaded, "immune_in_vehicle", fe.immune_in_vehicle );
                optional( joe, was_loaded, "immune_inside_vehicle", fe.immune_inside_vehicle );
                optional( joe, was_loaded, "immune_outside_vehicle", fe.immune_outside_vehicle );
                optional( joe, was_loaded, "chance_in_vehicle", fe.chance_in_vehicle );
                optional( joe, was_loaded, "chance_inside_vehicle", fe.chance_inside_vehicle );
                optional( joe, was_loaded, "chance_outside_vehicle", fe.chance_outside_vehicle );
                optional( joe, was_loaded, "message", fe.message );
                optional( joe, was_loaded, "message_npc", fe.message_npc );
                const auto game_message_type_reader = enum_flags_reader<game_message_type> { "game message types" };
                optional( joe, was_loaded, "message_type", fe.env_message_type, game_message_type_reader );
                JsonObject jid = joe.get_object( "immunity_data" );
                field_types::load_immunity( jid, fe.immunity_data );

                intensity_level.field_effects.emplace_back( fe );
            }
        } else {
            // Use effects from previous intensity level
            intensity_level.field_effects = fallback_intensity_level.field_effects;
        }
        optional( jao, was_loaded, "scent_neutralization", intensity_level.scent_neutralization,
                  fallback_intensity_level.scent_neutralization );
        intensity_levels.emplace_back( intensity_level );
    }
    if( intensity_levels.empty() ) {
        jo.throw_error_at( "id", "No intensity levels defined for field type" );
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
    field_types::load_immunity( jid, immunity_data );

    optional( jo, was_loaded, "immune_mtypes", immune_mtypes );
    optional( jo, was_loaded, "underwater_age_speedup", underwater_age_speedup, 0_turns );
    optional( jo, was_loaded, "outdoor_age_speedup", outdoor_age_speedup, 0_turns );
    optional( jo, was_loaded, "decay_amount_factor", decay_amount_factor, 0 );
    optional( jo, was_loaded, "percent_spread", percent_spread, 0 );
    optional( jo, was_loaded, "apply_slime_factor", apply_slime_factor, 0 );
    optional( jo, was_loaded, "gas_absorption_factor", gas_absorption_factor, 0_turns );
    optional( jo, was_loaded, "is_splattering", is_splattering, false );
    optional( jo, was_loaded, "dirty_transparency_cache", dirty_transparency_cache, false );
    optional( jo, was_loaded, "has_fire", has_fire, false );
    optional( jo, was_loaded, "has_acid", has_acid, false );
    optional( jo, was_loaded, "has_elec", has_elec, false );
    optional( jo, was_loaded, "has_fume", has_fume, false );
    optional( jo, was_loaded, "priority", priority, 0 );
    optional( jo, was_loaded, "half_life", half_life, 0_turns );
    const auto description_affix_reader = enum_flags_reader<description_affix> { "description affixes" };
    optional( jo, was_loaded, "description_affix", desc_affix, description_affix_reader,
              description_affix::DESCRIPTION_AFFIX_IN );
    if( jo.has_member( "phase" ) ) {
        phase = jo.get_enum_value<phase_id>( "phase", phase_id::PNULL );
    }
    optional( jo, was_loaded, "accelerated_decay", accelerated_decay, false );
    optional( jo, was_loaded, "display_items", display_items, true );
    optional( jo, was_loaded, "display_field", display_field, false );
    optional( jo, was_loaded, "legacy_make_rubble", legacy_make_rubble, false );
    optional( jo, was_loaded, "wandering_field", wandering_field, field_type_str_id::NULL_ID() );

    optional( jo, was_loaded, "mopsafe", mopsafe, false );

    optional( jo, was_loaded, "decrease_intensity_on_contact", decrease_intensity_on_contact, false );

    bash_info.load( jo, "bash", map_bash_info::field, "field " + id.str() );
    if( was_loaded && jo.has_member( "copy-from" ) && looks_like.empty() ) {
        looks_like = jo.get_string( "copy-from" );
    }
    jo.read( "looks_like", looks_like );
}

void field_type::finalize()
{
    dangerous = std::any_of( intensity_levels.begin(), intensity_levels.end(),
    []( const field_intensity_level & elem ) {
        return elem.dangerous;
    } );
    transparent = std::all_of( intensity_levels.begin(), intensity_levels.end(),
    []( const field_intensity_level & elem ) {
        return elem.transparent;
    } );

    if( !wandering_field.is_valid() ) {
        debugmsg( "Invalid wandering_field_id %s in field %s.", wandering_field.c_str(), id.c_str() );
        wandering_field = fd_null;
    }

    for( const mtype_id &m_id : immune_mtypes ) {
        if( !m_id.is_valid() ) {
            debugmsg( "Invalid mtype_id %s in immune_mtypes for field %s.", m_id.c_str(), id.c_str() );
        }
    }

    // should be the last operation for the type
    processors = map_field_processing::processors_for_type( *this );
}

void field_type::check() const
{
    int i = 0;
    for( const field_intensity_level &intensity_level : intensity_levels ) {
        i++;
        if( intensity_level.name.empty() ) {
            debugmsg( "Intensity level %d defined for field type \"%s\" has empty name.", i, id.c_str() );
        }
    }
}

size_t field_type::count()
{
    return get_all_field_types().size();
}

void field_types::load( const JsonObject &jo, const std::string &src )
{
    get_all_field_types().load( jo, src );
}

void field_types::finalize_all()
{
    get_all_field_types().finalize();
    for( const field_type &fd : get_all_field_types().get_all() ) {
        const_cast<field_type &>( fd ).finalize();
    }
}

void field_types::check_consistency()
{
    get_all_field_types().check();
}

void field_types::reset()
{
    get_all_field_types().reset();
}

void field_types::load_immunity( const JsonObject &jid, field_immunity_data &fd )
{
    for( const std::string id : jid.get_array( "flags" ) ) {
        fd.immunity_data_flags.emplace_back( id );
    }
    for( JsonArray jao : jid.get_array( "body_part_env_resistance" ) ) {
        fd.immunity_data_body_part_env_resistance.emplace_back(
            io::string_to_enum<body_part_type::type>
            ( jao.get_string( 0 ) ), jao.get_int( 1 ) );
    }
    for( JsonArray jao : jid.get_array( "immunity_flags_worn" ) ) {
        fd.immunity_data_part_item_flags.emplace_back( io::string_to_enum<body_part_type::type>
                ( jao.get_string( 0 ) ), jao.get_string( 1 ) );
    }

    for( JsonArray jao : jid.get_array( "immunity_flags_worn_any" ) ) {
        fd.immunity_data_part_item_flags_any.emplace_back(
            io::string_to_enum<body_part_type::type>
            ( jao.get_string( 0 ) ), jao.get_string( 1 ) );
    }
}

const std::vector<field_type> &field_types::get_all()
{
    return get_all_field_types().get_all();
}

field_type field_types::get_field_type_by_legacy_enum( int legacy_enum_id )
{
    for( const field_type &ft : get_all_field_types().get_all() ) {
        if( legacy_enum_id == ft.legacy_enum_id ) {
            return ft;
        }
    }
    debugmsg( "Cannot find field_type for legacy enum: %d.", legacy_enum_id );
    return field_type();
}
