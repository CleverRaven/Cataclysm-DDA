#include "weather.h" // IWYU pragma: associated

#include <cstddef>
#include <array>
#include <map>
#include <iterator>

#include "color.h"
#include "debug.h"
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
    const size_t i = static_cast<size_t>( type );
    if( i < weather_datums.size() ) {
        return { weather_datums[i], i > 0 };
    }
    debugmsg( "Invalid weather requested." );
    return { weather_datums[WEATHER_NULL], false };
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
sun_intensity_type sun_intensity( weather_type type )
{
    return weather_data_internal( type ).datum.sun_intensity;
}
weather_requirements requirements( weather_type type )
{
    return weather_data_internal( type ).datum.requirements;
}
weather_type get_bad_weather()
{
    weather_type bad_weather = WEATHER_NULL;
    int current_weather = 0;
    for each( weather_datum weather in weather_datums ) {
        current_weather++;
        if( weather.precip == precip_class::HEAVY ) {
            bad_weather = current_weather;
        }
    }
    return bad_weather;
}
int get_weather_count()
{
    return weather_datums.size();
}
} // namespace weather

///@}
