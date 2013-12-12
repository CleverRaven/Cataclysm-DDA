#ifndef _WEATHER_DATA_H_
#define _WEATHER_DATA_H_

#include "weather.h"
#include "game.h"
#include "translations.h"

std::string season_name[4];

/* Name, color in UI, {seasonal temperatures}, ranged penalty, sight penalty,
 * light_modifier, minimum time (in minutes), max time (in minutes), warn player?
 * Note that max time is NOT when the weather is guaranteed to stop; it is
 *  simply when the weather is guaranteed to be recalculated.  Most weather
 *  patterns have "stay the same" as a highly likely transition; see below
 * Note light modifier assumes baseline of DAYLIGHT_LEVEL at 60
 */
weather_datum weather_data[NUM_WEATHER_TYPES];

/* Chances for each season, for the weather listed on the left to shift to the
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

void game::init_weather()
{
    std::string tmp_season_name[4] = {
    _("Spring"), _("Summer"), _("Autumn"), _("Winter")
    };
    for(int i=0; i<4; i++) {season_name[i]=tmp_season_name[i];}

    weather_datum tmp_weather_data[] = {
    {"NULL Weather - BUG (weather_data.cpp:weather_data)", c_magenta,
     {0, 0, 0, 0}, 0, 0, 0, 0, 0, false,
     &weather_effect::none},
    {_("Clear"), c_cyan,
     {55, 85, 60, 30}, 0, 0, 0, 30, 120, false,
     &weather_effect::none},
    {_("Sunny"), c_ltcyan,
     {70, 100, 70, 40}, 0, 0, 20, 60, 300, false,
     &weather_effect::glare},
    {_("Cloudy"), c_ltgray,
     {50, 75, 60, 20}, 0, 2, -20, 60, 300, false,
     &weather_effect::none},
    {_("Drizzle"), c_ltblue,
     {45, 70, 45, 35}, 1, 3, -30, 10, 60, true,
     &weather_effect::wet},
    {_("Rain"), c_blue,
     {42, 65, 40, 30}, 3, 5, -40, 30, 180, true,
     &weather_effect::very_wet},
    {_("Thunder Storm"), c_dkgray,
     {42, 70, 40, 30}, 4, 7, -50, 30, 120, true,
     &weather_effect::thunder},
    {_("Lightning Storm"), c_yellow,
     {45, 52, 42, 32}, 4, 8, -50, 10, 30, true,
     &weather_effect::lightning},
    {_("Acidic Drizzle"), c_ltgreen,
     {45, 70, 45, 35}, 2, 3, -30, 10, 30, true,
     &weather_effect::light_acid},
    {_("Acid Rain"), c_green,
     {45, 70, 45, 30}, 4, 6, -40, 10, 30, true,
     &weather_effect::acid},
    {_("Flurries"), c_white,
     {30, 30, 30, 20}, 2, 4, -30, 10, 60, true,
     &weather_effect::flurry},
    {_("Snowing"), c_white,
     {25, 25, 20, 10}, 4, 7, -30, 30, 360, true,
     &weather_effect::snow},
    {_("Snowstorm"), c_white,
     {20, 20, 20,  5}, 6, 10, -55, 60, 180, true,
     &weather_effect::snowstorm}
    };
    for(int i=0; i<NUM_WEATHER_TYPES; i++) {weather_data[i]=tmp_weather_data[i];}
}

////////////////////////////////////////////////
//////// food decay, based on weather. static chart

// calculate how much food rots per hour, based on 10 = 1 minute of decay @ 65 F
// IRL this tends to double every 10c a few degrees above freezing, but past a certian
// point the rate decreases until even extremophiles find it too hot. Here we just stop
// further acceleration at 105 F. This should only need to run once when the game starts.
int calc_hourly_rotpoints_at_temp(const int temp) {
  const int base=600;       // default temp = 65, so generic->rotten() assumes 600 decay points per hour
  const int dropoff=38;     // ditch our fancy equation and do a linear approach to 0 rot at 31f
  const int cutoff=105;     // stop torturing the player at this temperature, which is
  const int cutoffrot=3540; // ..almost 6 times the base rate. bacteria hate the heat too

  const int dsteps=dropoff - 32;
  const int dstep=(35.91*pow(2.0,(float)dropoff/16) / dsteps);

  if ( temp < 32 ) {
    return 0;
  } else if ( temp > cutoff ) {
    return cutoffrot;
  } else if ( temp < dropoff ) {
    return ( ( temp - 32 ) * dstep );
  } else {
    return int((35.91*pow(2.0,(float)temp/16))+0.5);
  }
}

// generate vector based on above function
std::vector<int> calc_rot_array(const int cap) {
  std::vector<int> ret;
  for (int i = 0; i < cap; i++  ) {
    ret.push_back(calc_hourly_rotpoints_at_temp(i));
  }
  return ret;
}

const std::vector<int> rot_chart=calc_rot_array(200);

// fetch value from rot_chart for temperature
int get_hourly_rotpoints_at_temp (const int & temp) {
  if ( temp < 0 || temp < -1 ) return 0;
  if ( temp > 150 ) return 3540;
  return rot_chart[temp];
}

#endif // _WEATHER_DATA_H_
