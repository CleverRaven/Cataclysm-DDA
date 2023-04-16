#include "weather_type.h"

#include <cstdlib>
#include <set>

#include "assign.h"
#include "condition.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include <weather.h>

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
    cata_fatal( "Invalid precip_class" );
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
        case weather_sound_category::rainstorm:
            return "rainstorm";
        case weather_sound_category::snow:
            return "snow";
        case weather_sound_category::snowstorm:
            return "snowstorm";
        case weather_sound_category::thunder:
            return "thunder";
        case weather_sound_category::silent:
            return "silent";
        case weather_sound_category::portal_storm:
            return "portal_storm";
        case weather_sound_category::clear:
            return "clear";
        case weather_sound_category::sunny:
            return "sunny";
        case weather_sound_category::cloudy:
            return "cloudy";
        case weather_sound_category::last:
            break;
    }
    cata_fatal( "Invalid weather sound category." );
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
    for( const auto &type : required_weathers ) {
        if( !type.is_valid() ) {
            debugmsg( "Weather type %s does not exist.", type.c_str() );
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
    mandatory( jo, was_loaded, "priority", priority );
    optional( jo, was_loaded, "sun_multiplier", sun_multiplier, 1.f );

    mandatory( jo, was_loaded, "sound_attn", sound_attn );
    mandatory( jo, was_loaded, "dangerous", dangerous );
    mandatory( jo, was_loaded, "precip", precip );
    mandatory( jo, was_loaded, "rains", rains );
    optional( jo, was_loaded, "tiles_animation", tiles_animation, "" );
    optional( jo, was_loaded, "sound_category", sound_category, weather_sound_category::silent );
    optional( jo, was_loaded, "duration_min", duration_min, 5_minutes );
    optional( jo, was_loaded, "duration_max", duration_max, 5_minutes );
    if( duration_min > duration_max ) {
        jo.throw_error( "duration_min must be less than or equal to duration_max" );
    }
    optional( jo, was_loaded, "debug_cause_eoc", debug_cause_eoc );
    optional( jo, was_loaded, "debug_leave_eoc", debug_leave_eoc );

    if( jo.has_member( "weather_animation" ) ) {
        JsonObject weather_animation_jo = jo.get_object( "weather_animation" );
        mandatory( weather_animation_jo, was_loaded, "factor", weather_animation.factor );
        if( !assign( weather_animation_jo, "color", weather_animation.color ) ) {
            weather_animation_jo.throw_error( "missing mandatory member \"color\"" );
        }
        mandatory( weather_animation_jo, was_loaded, "sym", weather_animation.symbol,
                   unicode_codepoint_from_symbol_reader );
    }
    optional( jo, was_loaded, "required_weathers", required_weathers );
    read_condition( jo, "condition", condition, true );
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
        cata_fatal( "Weather type clear is required." );
    }
    if( !WEATHER_NULL.is_valid() ) {
        cata_fatal( "Weather type null is required." );
    }
    weather_type_factory.check();
}

void weather_types::load( const JsonObject &jo, const std::string &src )
{
    weather_type_factory.load( jo, src );
}
