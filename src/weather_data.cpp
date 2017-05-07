#include "weather.h"
#include "translations.h"
#include "color.h"

#include <map>
#include <vector>
#include <string>
#include <array>
#include <cmath>

/**
 * @ingroup Weather
 * @{
 */

weather_animation_t get_weather_animation(weather_type const type)
{
    static std::map<weather_type, weather_animation_t> const map {
        {WEATHER_ACID_DRIZZLE, weather_animation_t {0.01f, c_ltgreen, '.'}},
        {WEATHER_ACID_RAIN,    weather_animation_t {0.02f, c_ltgreen, ','}},
        {WEATHER_DRIZZLE,      weather_animation_t {0.01f, c_ltblue,  '.'}},
        {WEATHER_RAINY,        weather_animation_t {0.02f, c_ltblue,  ','}},
        {WEATHER_THUNDER,      weather_animation_t {0.02f, c_ltblue,  '.'}},
        {WEATHER_LIGHTNING,    weather_animation_t {0.04f, c_ltblue,  ','}},
        {WEATHER_FLURRIES,     weather_animation_t {0.01f, c_white,   '.'}},
        {WEATHER_SNOW,         weather_animation_t {0.02f, c_white,   ','}},
        {WEATHER_SNOWSTORM,    weather_animation_t {0.04f, c_white,   '*'}}
    };

    auto const it = map.find(type);
    if (it != std::end(map)) {
        return it->second;
    }

    return {0.0f, c_white, '?'};
}

const std::string season_name(int const season)
{
    static std::string const invalid {"bad season!"};
    static std::array<std::string, 4> const untranslated_names {{
        "spring", "summer", "autumn", "winter"
    }};

    if (!(season >= 0 && season < 4)) {
        return invalid;
    }

    return pgettext( "season_name", untranslated_names[season].c_str() );
}

const std::string season_name_upper(int const season)
{
    if (!(season >= 0 && season < 4)) {
        return season_name(season);
    }

    std::string name = season_name( season );
    name[0] = toupper( name[0] );
    return name;
}

weather_datum const& weather_data(weather_type const type)
{
    /**
     * Weather types data definition.
     * Name, color in UI, ranged penalty, sight penalty,
     * light modifier, sound attenuation, warn player?
     * Note light modifier assumes baseline of DAYLIGHT_LEVEL at 60
     */
    static std::array<weather_datum, NUM_WEATHER_TYPES> const data {{
        weather_datum {
            "NULL Weather - BUG (weather_data.cpp:weather_data)", c_magenta,
            0, 0.0f, 0, 0, false,
            &weather_effect::none
        },
        weather_datum {
            _("Clear"), c_cyan, 0, 1.0f, 0, 0, false,
            &weather_effect::none
        },
        weather_datum {
            _("Sunny"), c_ltcyan, 0, 1.0f, 2, 0, false,
            &weather_effect::glare
        },
        weather_datum {
            _("Cloudy"), c_ltgray, 0, 1.0f, -20, 0, false,
            &weather_effect::none
        },
        weather_datum {
            _("Drizzle"), c_ltblue, 1, 1.03f, -20, 1, false,
            &weather_effect::wet
        },
        weather_datum {
            _("Rain"), c_blue, 3, 1.1f, -30, 4, false,
            &weather_effect::very_wet
        },
        weather_datum {
            _("Thunder Storm"), c_dkgray, 4, 1.2f, -40, 8, false,
            &weather_effect::thunder
        },
        weather_datum {
            _("Lightning Storm"), c_yellow, 4, 1.25f, -45, 8, false,
            &weather_effect::lightning
        },
        weather_datum {
            _("Acidic Drizzle"), c_ltgreen, 2, 1.03f, -20, 1, true,
            &weather_effect::light_acid
        },
        weather_datum {
            _("Acid Rain"), c_green, 4, 1.1f, -30, 4, true,
            &weather_effect::acid
        },
        weather_datum {
            _("Flurries"), c_white, 2, 1.12f, -15, 2, false,
            &weather_effect::flurry
        },
        weather_datum {
            _("Snowing"), c_white, 4, 1.13f, -20, 4, false,
            &weather_effect::snow
        },
        weather_datum {
            _("Snowstorm"), c_white, 6, 1.2f, -30, 6, false,
            &weather_effect::snowstorm
        }
    }};

    auto const i = static_cast<size_t>(type);
    if (i < NUM_WEATHER_TYPES) {
        return data[i];
    }

    return data[0];
}

////////////////////////////////////////////////
//////// food decay, based on weather. static chart

/**
 * Food decay calculation.
 * Calculate how much food rots per hour, based on 10 = 1 minute of decay @ 65 F.
 * IRL this tends to double every 10c a few degrees above freezing, but past a certian
 * point the rate decreases until even extremophiles find it too hot. Here we just stop
 * further acceleration at 105 F. This should only need to run once when the game starts.
 * @see calc_rot_array
 * @see rot_chart
 */
int calc_hourly_rotpoints_at_temp(const int temp)
{
    // default temp = 65, so generic->rotten() assumes 600 decay points per hour
    const int dropoff = 38;     // ditch our fancy equation and do a linear approach to 0 rot at 31f
    const int cutoff = 105;     // stop torturing the player at this temperature, which is
    const int cutoffrot = 3540; // ..almost 6 times the base rate. bacteria hate the heat too

    const int dsteps = dropoff - 32;
    const int dstep = (35.91 * std::pow(2.0, (float)dropoff / 16.0) / dsteps);

    if ( temp < 32 ) {
        return 0;
    } else if ( temp > cutoff ) {
        return cutoffrot;
    } else if ( temp < dropoff ) {
        return ( ( temp - 32 ) * dstep );
    } else {
        return int(( 35.91 * std::pow(2.0, (float)temp / 16.0)) + 0.5);
    }
}

/**
 * Initialize the rot table.
 * @see rot_chart
 */
std::vector<int> calc_rot_array(size_t const cap)
{
    std::vector<int> ret;
    ret.reserve(cap);
    for (size_t i = 0; i < cap; ++i) {
        ret.push_back(calc_hourly_rotpoints_at_temp(static_cast<int>(i)));
    }
    return ret;
}

/**
 * Get the hourly rot for a given temperature from the precomputed table.
 * @see rot_chart
 */
int get_hourly_rotpoints_at_temp (const int temp)
{
    /**
     * Precomputed rot lookup table.
     */
    static std::vector<int> const rot_chart = calc_rot_array(200);

    if ( temp < 0 ) {
        return 0;
    }
    if ( temp > 150 ) {
        return 3540;
    }
    return rot_chart[temp];
}

///@}
