#include "weather.h" // IWYU pragma: associated

#include <cstddef>
#include <array>
#include <map>
#include <iterator>

#include "color.h"
#include "translations.h"

/**
 * @ingroup Weather
 * @{
 */

weather_animation_t get_weather_animation( weather_type const type )
{
    static const std::map<weather_type, weather_animation_t> map {
        {WEATHER_ACID_DRIZZLE, weather_animation_t {0.01f, c_light_green, '.'}},
        {WEATHER_ACID_RAIN,    weather_animation_t {0.02f, c_light_green, ','}},
        {WEATHER_LIGHT_DRIZZLE, weather_animation_t{0.01f, c_light_blue, ','}},
        {WEATHER_DRIZZLE,      weather_animation_t {0.01f, c_light_blue,  '.'}},
        {WEATHER_RAINY,        weather_animation_t {0.02f, c_light_blue,  ','}},
        {WEATHER_THUNDER,      weather_animation_t {0.02f, c_light_blue,  '.'}},
        {WEATHER_LIGHTNING,    weather_animation_t {0.04f, c_light_blue,  ','}},
        {WEATHER_FLURRIES,     weather_animation_t {0.01f, c_white,   '.'}},
        {WEATHER_SNOW,         weather_animation_t {0.02f, c_white,   ','}},
        {WEATHER_SNOWSTORM,    weather_animation_t {0.04f, c_white,   '*'}}
    };

    const auto it = map.find( type );
    if( it != std::end( map ) ) {
        return it->second;
    }

    return {0.0f, c_white, '?'};
}
struct weather_result {
    weather_datum datum;
    bool is_valid;
};

static std::vector<weather_datum> weather_datums;

static weather_result weather_data_internal( weather_type const type )
{
    /**
     * Weather types data definition.
     * Name, color in UI, color and glyph on map, ranged penalty, sight penalty,
     * light modifier, sound attenuation, warn player?
     * Note light modifier assumes baseline of default_daylight_level() at 60
     */
    // TODO: but it actually isn't 60, it's 100. Fix this comment or fix the value    

    const size_t i = static_cast<size_t>( type );
    if( i < NUM_WEATHER_TYPES ) {
        return { weather_datums[i], i > 0 };
    }

    return { weather_datums[0], false };
}

static weather_datum weather_data_interal_localized( weather_type const type )
{
    weather_result res = weather_data_internal( type );
    if( res.is_valid ) {
        res.datum.name = _( res.datum.name );
    }
    return res.datum;
}

weather_datum weather_data( weather_type const type )
{
    return weather_data_interal_localized( type );
}

namespace weather
{
void add_weather_datum( weather_datum new_weather )
{
    weather_datums.push_back( new_weather );
}
std::string name( weather_type const type )
{
    return weather_data_interal_localized( type ).name;
}
nc_color color( weather_type const type )
{
    return weather_data_internal( type ).datum.color;
}
nc_color map_color( weather_type const type )
{
    return weather_data_internal( type ).datum.map_color;
}
char glyph( weather_type const type )
{
    return weather_data_internal( type ).datum.glyph;
}
int ranged_penalty( weather_type const type )
{
    return weather_data_internal( type ).datum.ranged_penalty;
}
float sight_penalty( weather_type const type )
{
    return weather_data_internal( type ).datum.sight_penalty;
}
int light_modifier( weather_type const type )
{
    return weather_data_internal( type ).datum.light_modifier;
}
int sound_attn( weather_type const type )
{
    return weather_data_internal( type ).datum.sound_attn;
}
bool dangerous( weather_type const type )
{
    return weather_data_internal( type ).datum.dangerous;
}
precip_class precip( weather_type const type )
{
    return weather_data_internal( type ).datum.precip;
}
bool rains( weather_type const type )
{
    return weather_data_internal( type ).datum.rains;
}
bool acidic( weather_type const type )
{
    return weather_data_internal( type ).datum.acidic;
}
std::vector<std::pair<std::string, int>> effects( weather_type const type )
{
    return weather_data_internal( type ).datum.effects;
}
} // namespace weather

///@}
