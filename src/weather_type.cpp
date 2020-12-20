#include "weather_type.h"

#include <cstdlib>
#include <set>

#include "assign.h"
#include "debug.h"
#include "generic_factory.h"

namespace
{
generic_factory<weather_type> weather_type_factory( "weather_type" );
} // namespace

namespace io
{
template<>
std::string enum_to_string<precip_class>( precip_class data )
{
    switch( data ) {
        case precip_class::none:
            return "none";
        case precip_class::very_light:
            return "very_light";
        case precip_class::light:
            return "light";
        case precip_class::heavy:
            return "heavy";
        case precip_class::last:
            break;
    }
    debugmsg( "Invalid precip_class" );
    abort();
}

template<>
std::string enum_to_string<sun_intensity_type>( sun_intensity_type data )
{
    switch( data ) {
        case sun_intensity_type::none:
            return "none";
        case sun_intensity_type::light:
            return "light";
        case sun_intensity_type::normal:
            return "normal";
        case sun_intensity_type::high:
            return "high";
        case sun_intensity_type::last:
            break;
    }
    debugmsg( "Invalid sun_intensity_type" );
    abort();
}

template<>
std::string enum_to_string<weather_time_requirement_type>( weather_time_requirement_type data )
{
    switch( data ) {
        case weather_time_requirement_type::day:
            return "day";
        case weather_time_requirement_type::night:
            return "night";
        case weather_time_requirement_type::both:
            return "both";
        case weather_time_requirement_type::last:
            break;
    }
    debugmsg( "Invalid time_requirement_type" );
    abort();
}

template<>
std::string enum_to_string<weather_sound_category>( weather_sound_category data )
{
    switch( data ) {
        case weather_sound_category::drizzle:
            return "drizzle";
        case weather_sound_category::flurries:
            return "flurries";
        case weather_sound_category::rainy:
            return "rainy";
        case weather_sound_category::snow:
            return "snow";
        case weather_sound_category::snowstorm:
            return "snowstorm";
        case weather_sound_category::thunder:
            return "thunder";
        case weather_sound_category::silent:
            return "silent";
        case weather_sound_category::last:
            break;
    }
    debugmsg( "Invalid time_requirement_type" );
    abort();
}

} // namespace io

template<>
const weather_type &weather_type_id::obj() const
{
    return weather_type_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<weather_type>::is_valid() const
{
    return weather_type_factory.is_valid( *this );
}

void weather_type::finalize()
{

}

void weather_type::check() const
{
    for( const weather_type_id &required : requirements.required_weathers ) {
        if( !required.is_valid() ) {
            debugmsg( "Required weather type %s does not exist.", required.c_str() );
            abort();
        }
    }
    for( const weather_effect &effect : effects ) {
        if( !effect.effect_id.is_empty() && !effect.effect_id.is_valid() ) {
            debugmsg( "Effect type %s does not exist.", effect.effect_id.c_str() );
            abort();
        }
        if( !effect.trait_id_to_add.is_empty() && !effect.trait_id_to_add.is_valid() ) {
            debugmsg( "Trait %s does not exist.", effect.trait_id_to_add.c_str() );
            abort();
        }
        if( !effect.trait_id_to_remove.is_empty() && !effect.trait_id_to_remove.is_valid() ) {
            debugmsg( "Trait %s does not exist.", effect.trait_id_to_remove.c_str() );
            abort();
        }
        if( !effect.target_part.is_empty() && !effect.target_part.is_valid() ) {
            debugmsg( "Target part %s does not exist.", effect.target_part.c_str() );
            abort();
        }
        for( const spawn_type &spawn : effect.spawns ) {
            if( !spawn.target.is_empty() && !spawn.target.is_valid() ) {
                debugmsg( "Spawn target %s does not exist.", spawn.target.c_str() );
                abort();
            }
        }
        for( const weather_field &field : effect.fields ) {
            if( !field.type.is_valid() ) {
                debugmsg( "field type %s does not exist.", field.type.c_str() );
                abort();
            }
        }
    }
}

void weather_type::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "id",  id );

    assign( jo, "color", color );
    assign( jo, "map_color", map_color );

    mandatory( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader );

    mandatory( jo, was_loaded, "ranged_penalty", ranged_penalty );
    mandatory( jo, was_loaded, "sight_penalty", sight_penalty );
    mandatory( jo, was_loaded, "light_modifier", light_modifier );

    mandatory( jo, was_loaded, "sound_attn", sound_attn );
    mandatory( jo, was_loaded, "dangerous", dangerous );
    mandatory( jo, was_loaded, "precip", precip );
    mandatory( jo, was_loaded, "rains", rains );
    optional( jo, was_loaded, "acidic", acidic, false );
    optional( jo, was_loaded, "tiles_animation", tiles_animation, "" );
    optional( jo, was_loaded, "sound_category", sound_category, weather_sound_category::silent );
    mandatory( jo, was_loaded, "sun_intensity", sun_intensity );
    optional( jo, was_loaded, "duration_min", duration_min, 5_minutes );
    optional( jo, was_loaded, "duration_max", duration_max, 5_minutes );
    if( duration_min > duration_max ) {
        jo.throw_error( "duration_min must be less than or equal to duration_max" );
    }
    optional( jo, was_loaded, "time_between_min", time_between_min, 0_seconds );
    optional( jo, was_loaded, "time_between_max", time_between_max, 0_seconds );
    if( time_between_min > time_between_max ) {
        jo.throw_error( "time_between_min must be less than or equal to time_between_max" );
    }
    for( const JsonObject weather_effect_jo : jo.get_array( "effects" ) ) {

        weather_effect effect;

        optional( weather_effect_jo, was_loaded, "message", effect.message );
        optional( weather_effect_jo, was_loaded, "sound_message", effect.sound_message );
        optional( weather_effect_jo, was_loaded, "sound_effect", effect.sound_effect, "" );
        mandatory( weather_effect_jo, was_loaded, "must_be_outside", effect.must_be_outside );
        optional( weather_effect_jo, was_loaded, "one_in_chance", effect.one_in_chance, -1 );
        optional( weather_effect_jo, was_loaded, "time_between", effect.time_between );
        optional( weather_effect_jo, was_loaded, "lightning", effect.lightning, false );
        optional( weather_effect_jo, was_loaded, "rain_proof", effect.rain_proof, false );
        optional( weather_effect_jo, was_loaded, "pain_max", effect.pain_max, INT_MAX );
        optional( weather_effect_jo, was_loaded, "pain", effect.pain, 0 );
        optional( weather_effect_jo, was_loaded, "wet", effect.wet, 0 );
        optional( weather_effect_jo, was_loaded, "radiation", effect.radiation, 0 );
        optional( weather_effect_jo, was_loaded, "healthy", effect.healthy, 0 );
        optional( weather_effect_jo, was_loaded, "effect_id", effect.effect_id );
        optional( weather_effect_jo, was_loaded, "effect_duration", effect.effect_duration );
        optional( weather_effect_jo, was_loaded, "trait_id_to_add", effect.trait_id_to_add );
        optional( weather_effect_jo, was_loaded, "trait_id_to_remove", effect.trait_id_to_remove );
        optional( weather_effect_jo, was_loaded, "target_part", effect.target_part );
        assign( weather_effect_jo, "damage", effect.damage );

        for( const JsonObject field_jo : weather_effect_jo.get_array( "fields" ) ) {
            weather_field new_field;
            mandatory( field_jo, was_loaded, "type", new_field.type );
            mandatory( field_jo, was_loaded, "intensity", new_field.intensity );
            mandatory( field_jo, was_loaded, "age", new_field.age );
            optional( field_jo, was_loaded, "outdoor_only", new_field.outdoor_only, true );
            optional( field_jo, was_loaded, "radius", new_field.radius, 10000000 );

            effect.fields.emplace_back( new_field );
        }
        for( const JsonObject spawn_jo : weather_effect_jo.get_array( "spawns" ) ) {
            spawn_type spawn;
            mandatory( spawn_jo, was_loaded, "max_radius", spawn.max_radius );
            mandatory( spawn_jo, was_loaded, "min_radius", spawn.min_radius );
            if( spawn.min_radius > spawn.max_radius ) {
                spawn_jo.throw_error( "min_radius must be less than or equal to max_radius" );
            }
            optional( spawn_jo, was_loaded, "hallucination_count", spawn.hallucination_count, 0 );
            optional( spawn_jo, was_loaded, "real_count", spawn.real_count, 0 );
            optional( spawn_jo, was_loaded, "target", spawn.target );
            optional( spawn_jo, was_loaded, "target_range", spawn.target_range, 30 );

            effect.spawns.emplace_back( spawn );
        }
        effects.emplace_back( effect );
    }
    if( jo.has_member( "weather_animation" ) ) {
        JsonObject weather_animation_jo = jo.get_object( "weather_animation" );
        mandatory( weather_animation_jo, was_loaded, "factor", weather_animation.factor );
        if( !assign( weather_animation_jo, "color", weather_animation.color ) ) {
            weather_animation_jo.throw_error( "missing mandatory member \"color\"" );
        }
        mandatory( weather_animation_jo, was_loaded, "sym", weather_animation.symbol,
                   unicode_codepoint_from_symbol_reader );
    }

    requirements = {};
    if( jo.has_member( "requirements" ) ) {
        JsonObject weather_requires = jo.get_object( "requirements" );
        weather_requirements new_requires;

        optional( weather_requires, was_loaded, "pressure_min", new_requires.pressure_min, INT_MIN );
        optional( weather_requires, was_loaded, "pressure_max", new_requires.pressure_max, INT_MAX );
        optional( weather_requires, was_loaded, "humidity_min", new_requires.humidity_min, INT_MIN );
        optional( weather_requires, was_loaded, "humidity_max", new_requires.humidity_max, INT_MAX );
        optional( weather_requires, was_loaded, "temperature_min", new_requires.temperature_min, INT_MIN );
        optional( weather_requires, was_loaded, "temperature_max", new_requires.temperature_max, INT_MAX );
        optional( weather_requires, was_loaded, "windpower_min", new_requires.windpower_min, INT_MIN );
        optional( weather_requires, was_loaded, "windpower_max", new_requires.windpower_max, INT_MAX );
        optional( weather_requires, was_loaded, "humidity_and_pressure", new_requires.humidity_and_pressure,
                  true );
        optional( weather_requires, was_loaded, "time", new_requires.time,
                  weather_time_requirement_type::both );
        for( const std::string &required_weather :
             weather_requires.get_string_array( "required_weathers" ) ) {
            new_requires.required_weathers.push_back( weather_type_id( required_weather ) );
        }
        optional( weather_requires, was_loaded, "time_passed_min", new_requires.time_passed_min,
                  0_seconds );
        optional( weather_requires, was_loaded, "time_passed_max", new_requires.time_passed_max,
                  0_seconds );
        optional( weather_requires, was_loaded, "one_in_chance", new_requires.one_in_chance, 0 );
        requirements = new_requires;
    }
}

void weather_types::reset()
{
    weather_type_factory.reset();
}

void weather_types::finalize_all()
{
    weather_type_factory.finalize();
    for( const weather_type &wt : weather_type_factory.get_all() ) {
        const_cast<weather_type &>( wt ).finalize();
    }
}

const std::vector<weather_type> &weather_types::get_all()
{
    return weather_type_factory.get_all();
}

void weather_types::check_consistency()
{
    if( !WEATHER_CLEAR.is_valid() ) {
        debugmsg( "Weather type clear is required." );
        abort();
    }
    if( !WEATHER_NULL.is_valid() ) {
        debugmsg( "Weather type null is required." );
        abort();
    }
    weather_type_factory.check();
}

void weather_types::load( const JsonObject &jo, const std::string &src )
{
    weather_type_factory.load( jo, src );
}
