#ifndef _WEATHER_H_
#define _WEATHER_H_

#include "color.h"
#include <string>

class game;

enum season_type {
 SPRING = 0,
 SUMMER = 1,
 WINTER = 2,
 AUTUMN = 3
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
 void (weather_effect::*effect)(game *);
};

#endif // _WEATHER_H_
