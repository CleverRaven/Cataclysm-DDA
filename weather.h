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

#include "color.h"
#include <string>

class game;

enum season_type {
 SPRING = 0,
 SUMMER = 1,
 AUTUMN = 2,
 WINTER = 3
#define FALL AUTUMN
};

enum weather_type {
 WEATHER_NULL,		// For data and stuff
 WEATHER_CLEAR,		// No effects
 WEATHER_SUNNY,		// Glare if no eye protection
 WEATHER_CLOUDY,	// No effects
 WEATHER_DRIZZLE,	// Light rain
 WEATHER_RAINY,		// Lots of rain, sight penalties
 WEATHER_THUNDER,	// Warns of lightning to come
 WEATHER_LIGHTNING,	// Rare lightning strikes!
 WEATHER_ACID_DRIZZLE,	// No real effects; warning of acid rain
 WEATHER_ACID_RAIN,	// Minor acid damage
 WEATHER_FLURRIES,	// Light snow
 WEATHER_SNOW,		// Medium snow
 WEATHER_SNOWSTORM,	// Heavy snow
 NUM_WEATHER_TYPES
};

struct weather_effect
{
 void none		(game *) {};
 void glare		(game *);
 void wet		(game *);
 void very_wet		(game *);
 void thunder		(game *);
 void lightning		(game *);
 void light_acid	(game *);
 void acid		(game *);
 void flurry		(game *) {};
 void snow		(game *) {};
 void snowstorm		(game *) {};
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
 void (weather_effect::*effect)(game *);
};

#endif // _WEATHER_H_
