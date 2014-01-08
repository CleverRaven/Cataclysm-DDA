#ifndef _WEATHER_H_
#define _WEATHER_H_

#define BODYTEMP_FREEZING 500
#define BODYTEMP_VERY_COLD 2000 // This value means frostbite occurs at the warmest temperature of 1C. If changed, the temp_conv calculation should be reexamined.
#define BODYTEMP_COLD 3500
#define BODYTEMP_NORM 5000 // Do not change this value, it is an arbitrary anchor on which other calculations are made.
#define BODYTEMP_HOT 6500
#define BODYTEMP_VERY_HOT 8000
#define BODYTEMP_SCORCHING 9500
/*
Bodytemp is measured on a scale of 0u to 10000u, where 10u = 0.02C and 5000u is 37C
Outdoor temperature uses simliar numbers, but on a different scale: 2200u = 22C, where 10u = 0.1C.
Most values can be changed with no impact on calculations. Because of caluclations done in disease.cpp,
maximum heat cannot pass 15000u, otherwise the player will vomit to death.
*/

// How far into the future we should generate weather, in hours.
// 168 hours in a week.
#define MAX_FUTURE_WEATHER 168

#include "color.h"
#include <string>
#include "calendar.h"
#include "overmap.h"

class game;

enum weather_type {
    WEATHER_NULL,       // For data and stuff
    WEATHER_CLEAR,      // No effects
    WEATHER_SUNNY,      // Glare if no eye protection
    WEATHER_CLOUDY,     // No effects
    WEATHER_DRIZZLE,    // Light rain
    WEATHER_RAINY,      // Lots of rain, sight penalties
    WEATHER_THUNDER,    // Warns of lightning to come
    WEATHER_LIGHTNING,  // Rare lightning strikes!
    WEATHER_ACID_DRIZZLE, // No real effects; warning of acid rain
    WEATHER_ACID_RAIN,  // Minor acid damage
    WEATHER_FLURRIES,   // Light snow
    WEATHER_SNOW,       // Medium snow
    WEATHER_SNOWSTORM,  // Heavy snow
    NUM_WEATHER_TYPES
};
// used for drawing weather bits to the screen
struct weather_printable
{
    weather_type wtype;
    std::vector<std::pair<int, int> > vdrops;
    nc_color colGlyph;
    char cGlyph;
    int startx, starty, endx, endy;
};
struct weather_effect
{
    void none       () {};
    void glare      ();
    void wet        ();
    void very_wet   ();
    void thunder    ();
    void lightning  ();
    void light_acid ();
    void acid       ();
    void flurry     () {};
    void snow       () {};
    void snowstorm  () {};
};

// All the weather conditions at some time
struct weather_segment
{
    signed char temperature;
    weather_type weather;
    calendar deadline;
};

struct weather_datum
{
 std::string name;
 nc_color color;
 int avg_temperature[4]; // Spring, Summer, Winter, Fall
 int ranged_penalty;
 int sight_penalty; // Penalty to max sight range
 int light_modifier; // Modification to ambient light
 int mintime, maxtime; // min/max time it lasts, in minutes
 bool dangerous; // If true, our activity gets interrupted
 void (weather_effect::*effect)();
};

extern std::string season_name[4];
extern weather_datum weather_data[NUM_WEATHER_TYPES];
extern int weather_shift[4][NUM_WEATHER_TYPES][NUM_WEATHER_TYPES];

std::string weather_forecast(radio_tower tower);

// Returns input value (in fahrenheit) converted to whatever temperature scale set in options.
//
// If scale is Celsius:    temperature(100) will return "37C"
// If scale is Fahrenheit: temperature(100) will return "100F"
//
// Use the decimals parameter to set number of decimal places returned in string.
std::string print_temperature(float fahrenheit, int decimals = 0);

void retroactively_fill_from_funnel( item *it, const trap_id t, const int endturn );

extern const std::vector<int> rot_chart;
int get_hourly_rotpoints_at_temp (const int & temp);
int get_rot_since( const int since, const int endturn );
#endif // _WEATHER_H_
