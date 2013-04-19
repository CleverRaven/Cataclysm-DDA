#ifndef _WEATHER_DATA_H_
#define _WEATHER_DATA_H_

#include "weather.h"

std::string season_name[4] = {
"Spring", "Summer", "Autumn", "Winter"
};

/* Name, color in UI, {seasonal temperatures}, ranged penalty, sight penalty,
 * light_modifier, minimum time (in minutes), max time (in minutes), warn player?
 * Note that max time is NOT when the weather is guaranteed to stop; it is
 *  simply when the weather is guaranteed to be recalculated.  Most weather
 *  patterns have "stay the same" as a highly likely transition; see below
 * Note light modifier assumes baseline of DAYLIGHT_LEVEL at 60
 */
weather_datum weather_data[] = {
{"NULL Weather - BUG", c_magenta,
 {0, 0, 0, 0}, 0, 0, 0, 0, 0, false,
 &weather_effect::none},
{"Clear", c_cyan,
 {55, 85, 60, 30}, 0, 0, 0, 30, 120, false,
 &weather_effect::none},
{"Sunny", c_ltcyan,
 {70, 100, 70, 40}, 0, 0, 20, 60, 300, false,
 &weather_effect::glare},
{"Cloudy", c_ltgray,
 {50, 75, 60, 20}, 0, 2, -20, 60, 300, false,
 &weather_effect::none},
{"Drizzle", c_ltblue,
 {45, 70, 45, 35}, 1, 3, -30, 10, 60, true,
 &weather_effect::wet},
{"Rain", c_blue,
 {42, 65, 40, 30}, 3, 5, -40, 30, 180, true,
 &weather_effect::very_wet},
{"Thunder Storm", c_dkgray,
 {42, 70, 40, 30}, 4, 7, -50, 30, 120, true,
 &weather_effect::thunder},
{"Lightning Storm", c_yellow,
 {45, 52, 42, 32}, 4, 8, -50, 10, 30, true,
 &weather_effect::lightning},
{"Acidic Drizzle", c_ltgreen,
 {45, 70, 45, 35}, 2, 3, -30, 10, 30, true,
 &weather_effect::light_acid},
{"Acid Rain", c_green,
 {45, 70, 45, 30}, 4, 6, -40, 10, 30, true,
 &weather_effect::acid},
{"Flurries", c_white,
 {30, 30, 30, 20}, 2, 4, -30, 10, 60, true,
 &weather_effect::flurry},
{"Snowing", c_white,
 {25, 25, 20, 10}, 4, 7, -30, 30, 360, true,
 &weather_effect::snow},
{"Snowstorm", c_white,
 {20, 20, 20,  5}, 6, 10, -55, 60, 180, true,
 &weather_effect::snowstorm}
};

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

#endif // _WEATHER_DATA_H_
