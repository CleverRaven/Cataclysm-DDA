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
bool feed_plants( weather_type const type )
{
    return weather_data_internal( type ).datum.feed_plants;
}
std::string tiles_animation( weather_type const type )
{
    return weather_data_internal( type ).datum.tiles_animation;
}
weather_animation_t get_weather_animation( weather_type const type )
{
    return weather_data_internal( type ).datum.weather_animation;
}
int sound_category( weather_type const type )
{
    return weather_data_internal( type ).datum.sound_category;
}
bool sunny( weather_type const type )
{
    return weather_data_internal( type ).datum.sunny;
}
weather_type get_bad_weather()
{
    weather_type bad_weather = weather_type::WEATHER_NULL;
    int current_weather = 0;
    for each( weather_datum weather in weather_datums ) {
        current_weather++;
        if( weather.precip == precip_class::HEAVY ) {
            bad_weather = static_cast<weather_type>( current_weather );
        }
    }
    return bad_weather;
}
} // namespace weather

///@}
