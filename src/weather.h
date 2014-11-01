#ifndef WEATHER_H
#define WEATHER_H

/**
 * @name BODYTEMP
 * Body temperature.
 * Bodytemp is measured on a scale of 0u to 10000u, where 10u = 0.02C and 5000u is 37C
 * Outdoor temperature uses similar numbers, but on a different scale: 2200u = 22C, where 10u = 0.1C.
 * Most values can be changed with no impact on calculations. Because of calculations done in disease.cpp,
 * maximum heat cannot pass 15000u, otherwise the player will vomit to death.
 */
///@{
#define BODYTEMP_FREEZING 500   //!< More aggressive cold effects.
#define BODYTEMP_VERY_COLD 2000 //!< This value means frostbite occurs at the warmest temperature of 1C. If changed, the temp_conv calculation should be reexamined.
#define BODYTEMP_COLD 3500      //!< Frostbite timer will not improve while below this point.
#define BODYTEMP_NORM 5000      //!< Do not change this value, it is an arbitrary anchor on which other calculations are made.
#define BODYTEMP_HOT 6500       //!< Level 1 hotness.
#define BODYTEMP_VERY_HOT 8000  //!< Level 2 hotness.
#define BODYTEMP_SCORCHING 9500 //!< Level 3 hotness.
///@}

/**
 * How far into the future we should generate weather, in hours.
 * 168 hours in a week.
 */
#define MAX_FUTURE_WEATHER 168

#include "color.h"
#include <string>
#include "calendar.h"
#include "overmap.h"

class game;

/**
 * Weather type enum.
 */
enum weather_type {
    WEATHER_NULL,         //!< For data and stuff
    WEATHER_CLEAR,        //!< No effects
    WEATHER_SUNNY,        //!< Glare if no eye protection
    WEATHER_CLOUDY,       //!< No effects
    WEATHER_DRIZZLE,      //!< Light rain
    WEATHER_RAINY,        //!< Lots of rain, sight penalties
    WEATHER_THUNDER,      //!< Warns of lightning to come
    WEATHER_LIGHTNING,    //!< Rare lightning strikes!
    WEATHER_ACID_DRIZZLE, //!< No real effects; warning of acid rain
    WEATHER_ACID_RAIN,    //!< Minor acid damage
    WEATHER_FLURRIES,     //!< Light snow
    WEATHER_SNOW,         //!< Medium snow
    WEATHER_SNOWSTORM,    //!< Heavy snow
    NUM_WEATHER_TYPES     //!< Sentinel value
};

/**
 * Weather animation class.
 */
class clWeatherAnim
{
    public:
        char cGlyph;
        nc_color colGlyph;
        float fFactor;

        clWeatherAnim()
        {
            cGlyph = '?';
            colGlyph = c_white;
            fFactor = 0.0f;
        };
        ~clWeatherAnim() {};

        clWeatherAnim(const char p_cGlyph, const nc_color p_colGlyph, const float p_fFactor)
        {
            cGlyph = p_cGlyph;
            colGlyph = p_colGlyph;
            fFactor = p_fFactor;
        };
};

/**
 * Weather animation settings container.
 */
extern std::map<weather_type, clWeatherAnim> mapWeatherAnim;

/**
 * Weather drawing tracking.
 * Used for redrawing the view coordinates overwritten by the previous frame's animation bits (raindrops, snowflakes, etc,) and to draw this frame's weather animation.
 * @see game::get_player_input
 */
struct weather_printable {
    weather_type wtype; //!< Weather type in use.
    std::vector<std::pair<int, int> > vdrops; //!< Coordinates targeted for droplets.
    nc_color colGlyph; //!< Color to draw glyph this animation frame.
    char cGlyph; //!< Glyph to draw this animation frame.
    int startx, starty, endx, endy;
};

/**
 * Environmental effects and ramifications of weather.
 * Visibility range changes are done elsewhere.
 */
struct weather_effect {
    void none       () {}; //!< Fallback weather.
    void glare      ();
    void wet        ();
    void very_wet   ();
    void thunder    ();
    void lightning  ();
    void light_acid ();
    void acid       ();
    void flurry     () {}; //!< Currently flurries have no additional effects.
    void snow       () {}; //!< Currently snow has no additional effects.
    void snowstorm  () {}; //!< Currently snowstorms have no additional effects.
};

// All the weather conditions at some time
struct weather_segment {
    signed char temperature;
    weather_type weather;
    calendar deadline;
};

struct weather_datum {
    std::string name;       //!< UI name of weather type.
    nc_color color;         //!< UI color of weather type.
    int avg_temperature[4]; //!< Spring, Summer, Winter, Fall.
    int ranged_penalty;     //!< Penalty to ranged attacks.
    int sight_penalty;      //!< Penalty to max sight range.
    int light_modifier;     //!< Modification to ambient light.
    int mintime,
        maxtime;   //!< min/max time it lasts, in minutes. Note that this is a *recalculation* deadline.
    bool dangerous;         //!< If true, our activity gets interrupted.
    void (weather_effect::*effect)(); //!< Function pointer for weather effects.
};

extern std::string season_name[4];
extern std::string season_name_uc[4];
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
std::string print_windspeed(float windspeed, int decimals = 0);
std::string print_humidity(float humidity, int decimals = 0);
std::string print_pressure(float pressure, int decimals = 0);

int get_local_windchill(double temperature, double humidity, double windpower);
int get_local_humidity(double humidity, weather_type weather, bool sheltered = false);
int get_local_windpower(double windpower, std::string omtername = "no name",
                        bool sheltered = false);

void retroactively_fill_from_funnel( item *it, const trap_id t, const calendar &, const point &);

extern const std::vector<int> rot_chart;
int get_hourly_rotpoints_at_temp (const int &temp);
//int get_rot_since( const int since, const int endturn );
int get_rot_since( const int since, const int endturn, const point &);

#endif
