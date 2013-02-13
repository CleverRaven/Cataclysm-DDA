#ifndef _WEATHER_H_
#define _WEATHER_H_

#define BODYTEMP_FREEZING 50
#define BODYTEMP_VERY_COLD 200
#define BODYTEMP_COLD 350
#define BODYTEMP_NORM 500
#define BODYTEMP_HOT 650
#define BODYTEMP_VERY_HOT 800
#define BODYTEMP_SCORCHING 950
/*
Bodytemp is measured on a scale of 0u to 1000u, where 1u = 0.02C and 500u is 37C
Outdoor temperature uses simliar numbers, but on a different scale: 220u = 22C, where 1u = 0.1C.
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
