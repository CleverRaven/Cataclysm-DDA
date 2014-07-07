#ifndef _WEATHER_DATA_H_
#define _WEATHER_DATA_H_

#include "weather.h"
#include "game.h"
#include "translations.h"
#include "path_info.h"
#include <fstream>

/**
 * @ingroup Weather
 * @{
 */

std::string season_name[4];

/**
 * Weather types data definition.
 * Name, color in UI, {seasonal temperatures}, ranged penalty, sight penalty,
 * light_modifier, minimum time (in minutes), max time (in minutes), warn player?
 * Note that max time is NOT when the weather is guaranteed to stop; it is
 * simply when the weather is guaranteed to be recalculated.  Most weather
 * patterns have "stay the same" as a highly likely transition; see below
 * Note light modifier assumes baseline of DAYLIGHT_LEVEL at 60
 */
weather_datum weather_data[NUM_WEATHER_TYPES];

/**
 * Weather animation settings
 */
void game::init_weather_anim() {
    mapWeatherAnim.clear();
    mapWeatherAnim[WEATHER_ACID_DRIZZLE] =  clWeatherAnim('.', c_ltgreen, 0.01f);
    mapWeatherAnim[WEATHER_ACID_RAIN] =     clWeatherAnim(',', c_ltgreen, 0.02f);
    mapWeatherAnim[WEATHER_DRIZZLE] =       clWeatherAnim('.', c_ltblue, 0.01f);
    mapWeatherAnim[WEATHER_RAINY] =         clWeatherAnim(',', c_ltblue, 0.02f);
    mapWeatherAnim[WEATHER_THUNDER] =       clWeatherAnim('.', c_ltblue, 0.02f);
    mapWeatherAnim[WEATHER_LIGHTNING] =     clWeatherAnim(',', c_ltblue, 0.04f);
    mapWeatherAnim[WEATHER_FLURRIES] =      clWeatherAnim('*', c_white, 0.01f);
    mapWeatherAnim[WEATHER_SNOW] =          clWeatherAnim('*', c_white, 0.02f);
    mapWeatherAnim[WEATHER_SNOWSTORM] =     clWeatherAnim('*', c_white, 0.04f);
}

/**
 * Weather change bias table.
 * Chances for each season, for the weather listed on the left to shift to the
 * weather listed across the top.
 */
int weather_shift[4][NUM_WEATHER_TYPES][NUM_WEATHER_TYPES] = {
{ // SPRING
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* NUL */ {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLR */ {  0,  5,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* SUN */ {  0,  4,  7,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLD */ {  0,  3,  0,  4,  3,  1,  0,  0,  1,  0,  1,  0,  0},
/* DRZ */ {  0,  1,  0,  3,  6,  3,  1,  0,  2,  0,  0,  0,  0},
/* RAI */ {  0,  0,  0,  4,  5,  7,  3,  1,  0,  0,  0,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* TND */ {  0,  0,  0,  2,  2,  4,  5,  3,  0,  0,  0,  0,  0},
/* LGT */ {  0,  0,  0,  1,  1,  4,  5,  5,  0,  0,  0,  0,  0},
/* AC1 */ {  0,  1,  0,  1,  1,  1,  0,  0,  3,  3,  0,  0,  0},
/* AC2 */ {  0,  0,  0,  1,  1,  1,  0,  0,  4,  2,  0,  0,  0},
/* SN1 */ {  0,  1,  0,  3,  2,  1,  1,  0,  0,  0,  2,  1,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* SN2 */ {  0,  0,  0,  1,  1,  2,  1,  0,  0,  0,  3,  1,  1},
/* SN3 */ {  0,  0,  0,  0,  1,  3,  2,  1,  0,  0,  1,  1,  1}
},

{ // SUMMER
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* NUL */ {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLR */ {  0,  5,  5,  2,  2,  1,  1,  0,  1,  0,  1,  0,  0},
/* SUN */ {  0,  3,  7,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLD */ {  0,  1,  1,  6,  5,  2,  1,  0,  2,  0,  1,  0,  0},
/* DRZ */ {  0,  2,  2,  3,  6,  3,  1,  0,  2,  0,  0,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* RAI */ {  0,  1,  1,  3,  4,  5,  4,  2,  0,  0,  0,  0,  0},
/* TND */ {  0,  0,  0,  2,  3,  5,  4,  5,  0,  0,  0,  0,  0},
/* LGT */ {  0,  0,  0,  0,  0,  3,  3,  5,  0,  0,  0,  0,  0},
/* AC1 */ {  0,  1,  1,  2,  1,  1,  0,  0,  3,  4,  0,  0,  0},
/* AC2 */ {  0,  1,  0,  1,  1,  1,  0,  0,  5,  3,  0,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* SN1 */ {  0,  4,  0,  4,  2,  2,  1,  0,  0,  0,  2,  1,  0},
/* SN2 */ {  0,  0,  0,  2,  2,  4,  2,  0,  0,  0,  3,  1,  1},
/* SN3 */ {  0,  0,  0,  2,  1,  3,  3,  1,  0,  0,  2,  2,  0}
},

{ // AUTUMN
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* NUL */ {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLR */ {  0,  6,  3,  3,  3,  1,  1,  0,  1,  0,  1,  0,  0},
/* SUN */ {  0,  4,  5,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLD */ {  0,  1,  1,  8,  5,  2,  0,  0,  2,  0,  1,  0,  0},
/* DRZ */ {  0,  1,  0,  3,  6,  3,  1,  0,  2,  0,  0,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* RAI */ {  0,  1,  1,  3,  4,  5,  4,  2,  0,  0,  0,  0,  0},
/* TND */ {  0,  0,  0,  2,  3,  5,  4,  5,  0,  0,  0,  0,  0},
/* LGT */ {  0,  0,  0,  0,  0,  3,  3,  5,  0,  0,  0,  0,  0},
/* AC1 */ {  0,  1,  1,  2,  1,  1,  0,  0,  3,  4,  0,  0,  0},
/* AC2 */ {  0,  0,  0,  1,  1,  1,  0,  0,  4,  4,  0,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* SN1 */ {  0,  2,  0,  4,  2,  1,  0,  0,  0,  0,  2,  1,  0},
/* SN2 */ {  0,  0,  0,  2,  2,  5,  2,  0,  0,  0,  2,  1,  1},
/* SN3 */ {  0,  0,  0,  2,  1,  5,  2,  0,  0,  0,  2,  1,  1}
},

{ // WINTER
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* NUL */ {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
/* CLR */ {  0,  9,  3,  4,  1,  0,  0,  0,  1,  0,  2,  0,  0},
/* SUN */ {  0,  4,  8,  1,  0,  0,  0,  0,  0,  0,  1,  0,  0},
/* CLD */ {  0,  1,  1,  8,  1,  0,  0,  0,  1,  0,  4,  2,  1},
/* DRZ */ {  0,  1,  0,  4,  3,  1,  0,  0,  1,  0,  3,  0,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* RAI */ {  0,  0,  0,  3,  2,  2,  1,  1,  0,  0,  4,  4,  0},
/* TND */ {  0,  0,  0,  2,  1,  2,  2,  1,  0,  0,  2,  4,  1},
/* LGT */ {  0,  0,  0,  3,  0,  3,  3,  1,  0,  0,  2,  4,  4},
/* AC1 */ {  0,  1,  1,  4,  1,  0,  0,  0,  3,  1,  1,  0,  0},
/* AC2 */ {  0,  0,  0,  2,  1,  1,  0,  0,  4,  1,  1,  1,  0},
//         NUL CLR SUN CLD DRZ RAI THN LGT AC1 AC2 SN1 SN2 SN3
/* SN1 */ {  0,  1,  0,  5,  1,  0,  0,  0,  0,  0,  7,  2,  0},
/* SN2 */ {  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  2,  7,  3},
/* SN3 */ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  4,  6}
}
};

/**
 * Initialize the global weather types data definition table.
 * @see weather_data
 */
void game::init_weather()
{
    std::ifstream jsonstream(FILENAMES["weather"].c_str(), std::ifstream::binary);
    if (jsonstream.good()) {
        JsonIn json(jsonstream);
        JsonArray config = json.get_array();
        // fontsize, fontblending, map_* are ignored in wincurse.		

		std::vector<std::string> season_names = config.get_object(0).get_string_array("season_names");

        std::string tmp_season_name[4] = {
            _(season_names[0].c_str()), _(season_names[1].c_str()), _(season_names[2].c_str()), _(season_names[3].c_str())
        };

        for (int i = 0; i < 4; i++) {
            season_name[i] = tmp_season_name[i];
        }

		JsonArray wd = config.get_object(1).get_array("weather_datum");

        // Nm, UICol, {Temp by season}, RangedPEN, SightPEN, light_mod, MinTIME, MaxTIME(to recalc)
        weather_datum tmp_weather_data[] = {
            {
                "NULL Weather - BUG (weather_data.cpp:weather_data)", c_magenta,
                { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, false,
                &weather_effect::none
            },

            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::none
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::glare
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::none
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::wet
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::very_wet
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::thunder
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::lightning
            },
            // Nm, UICol, Temp, RangedPEN, SightPEN, light_mod, MinTIME, MaxTIME(to recalc)
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::light_acid
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::acid
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::flurry
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::snow
            },
            {
                _(wd.next_string().c_str()), color_from_string(wd.next_string()),
                { wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int() }, wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_int(), wd.next_bool(),
                &weather_effect::snowstorm
            }
        };
        for (int i = 0; i < NUM_WEATHER_TYPES; i++) {
            weather_data[i] = tmp_weather_data[i];
        }

        jsonstream.close();
    }
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
int calc_hourly_rotpoints_at_temp(const int temp) {
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
        return int(( 35.91 * std::pow(2.0,(float)temp / 16.0)) + 0.5);
    }
}

/**
 * Initialize the rot table.
 * @see rot_chart
 */
std::vector<int> calc_rot_array(const int cap) {
    std::vector<int> ret;
    for (int i = 0; i < cap; i++  ) {
        ret.push_back(calc_hourly_rotpoints_at_temp(i));
    }
    return ret;
}

/**
 * Precomputed rot lookup table.
 */
const std::vector<int> rot_chart = calc_rot_array(200);

/**
 * Get the hourly rot for a given temperature from the precomputed table.
 * @see rot_chart
 */
int get_hourly_rotpoints_at_temp (const int & temp) {
    if ( temp < 0 || temp < -1 ) return 0;
    if ( temp > 150 ) return 3540;
    return rot_chart[temp];
}

///@}
#endif // _WEATHER_DATA_H_
