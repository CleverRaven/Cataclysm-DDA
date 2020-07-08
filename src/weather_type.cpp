#include "weather_type.h"
#include "weather.h"
#include "game_constants.h"

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
}

void weather_type::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "id",  id );

    assign( jo, "color", color );
    assign( jo, "map_color", map_color );

    std::string glyph;
    mandatory( jo, was_loaded, "glyph", glyph );
    if( glyph.size() != 1 ) {
        jo.throw_error( "glyph must be only one character" );
    } else {
        glyph = glyph[0];
    }
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

    for( const JsonObject weather_effect : jo.get_array( "effects" ) ) {

        std::pair<std::string, int> pair = std::make_pair( weather_effect.get_string( "name" ),
                                           weather_effect.get_int( "intensity" ) );

        static const std::map<std::string, weather_effect_fn> all_weather_effects = {
            { "wet", &weather_effect::wet_player },
            { "thunder", &weather_effect::thunder },
            { "lightning", &weather_effect::lightning },
            { "light_acid", &weather_effect::light_acid },
            { "acid", &weather_effect::acid }
        };
        const auto iter = all_weather_effects.find( pair.first );
        if( iter == all_weather_effects.end() ) {
            weather_effect.throw_error( "Invalid weather effect", "name" );
        }
        effects.emplace_back( iter->second, pair.second );
    }
    weather_animation = { 0.0f, c_white, '?' };
    if( jo.has_member( "weather_animation" ) ) {
        JsonObject weather_animation_jo = jo.get_object( "weather_animation" );
        weather_animation_t animation;
        mandatory( weather_animation_jo, was_loaded, "factor", animation.factor );
        if( !assign( weather_animation_jo, "color", animation.color ) ) {
            weather_animation_jo.throw_error( "missing mandatory member \"color\"" );
        }
        mandatory( weather_animation_jo, was_loaded, "glyph", glyph );
        if( glyph.size() != 1 ) {
            weather_animation_jo.throw_error( "glyph must be only one character" );
        } else {
            animation.glyph = glyph[0];
        }
        weather_animation = animation;
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
        optional( weather_requires, was_loaded, "acidic", new_requires.acidic, false );
        optional( weather_requires, was_loaded, "time", new_requires.time,
                  weather_time_requirement_type::both );
        for( const std::string &required_weather :
             weather_requires.get_string_array( "required_weathers" ) ) {
            new_requires.required_weathers.push_back( weather_type_id( required_weather ) );
        }
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

