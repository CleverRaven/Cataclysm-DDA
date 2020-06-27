#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <string>
#include <climits>

#include "calendar.h"
#include "color.h"

struct tripoint;
class JsonObject;
typedef int weather_type;

enum class precip_class : int {
    NONE,
    VERY_LIGHT,
    LIGHT,
    HEAVY
};

enum class sun_intensity_type : int {
    none,
    light,
    normal,
    high,
};

enum class time_requirement_type : int {
    day,
    night,
    both
};

/**
 * Weather animation class.
 */
struct weather_animation_t {
    float    factor;
    nc_color color;
    char     glyph;
};

struct weather_requirements {
    int windpower_min = INT_MIN;
    int windpower_max = INT_MAX;
    int temperature_min = INT_MIN;
    int temperature_max = INT_MAX;
    int pressure_min = INT_MIN;
    int pressure_max = INT_MAX;
    int humidity_min = INT_MIN;
    int humidity_max = INT_MAX;
    bool humidity_and_pressure = true;
    bool acidic = false;
    time_requirement_type time;
    std::vector<std::string> required_weathers;
};

struct weather_datum {
    std::string name;             //!< UI name of weather type.
    nc_color color;               //!< UI color of weather type.
    nc_color map_color;           //!< Map color of weather type.
    char glyph;                   //!< Map glyph of weather type.
    int ranged_penalty;           //!< Penalty to ranged attacks.
    float sight_penalty;          //!< Penalty to per-square visibility, applied in transparency map.
    int light_modifier;           //!< Modification to ambient light.
    int sound_attn;               //!< Sound attenuation of a given weather type.
    bool dangerous;               //!< If true, our activity gets interrupted.
    precip_class precip;          //!< Amount of associated precipitation.
    bool rains;                   //!< Whether said precipitation falls as rain.
    bool acidic;                  //!< Whether said precipitation is acidic.
    std::vector<std::pair<std::string, int>> effects;     //!< vector for weather effects.
    std::string tiles_animation;  //!< string for tiles animation
    weather_animation_t weather_animation; //!< Information for weather animations
    int sound_category;           //!< if playing sound effects what to use
    sun_intensity_type sun_intensity; //!< strength of the sun
    weather_requirements requirements; //!< when this weather should happen

};

struct w_point {
    double temperature = 0;
    double humidity = 0;
    double pressure = 0;
    double windpower = 0;
    std::string wind_desc;
    int winddirection = 0;
    bool acidic = false;
};

class weather_generator
{
    public:
        // Average temperature
        double base_temperature = 0;
        // Average humidity
        double base_humidity = 0;
        // Average atmospheric pressure
        double base_pressure = 0;
        double base_acid = 0;
        //Average yearly windspeed
        double base_wind = 0;
        //How much the wind peaks above average
        int base_wind_distrib_peaks = 0;
        int summer_temp_manual_mod = 0;
        int spring_temp_manual_mod = 0;
        int autumn_temp_manual_mod = 0;
        int winter_temp_manual_mod = 0;
        int spring_humidity_manual_mod = 0;
        int summer_humidity_manual_mod = 0;
        int autumn_humidity_manual_mod = 0;
        int winter_humidity_manual_mod = 0;
        //How much the wind folows seasonal variation ( lower means more change )
        int base_wind_season_variation = 0;
        static int current_winddir;

        weather_generator();

        /**
         * Input should be an absolute position in the map square system (the one used
         * by the @ref map). You can use @ref map::getabs to get an absolute position from a
         * relative position (relative to the map you called getabs on).
         */
        w_point get_weather( const tripoint &, const time_point &, unsigned ) const;
        weather_type get_weather_conditions( const tripoint &, const time_point &, unsigned seed ) const;
        weather_type get_weather_conditions( const w_point & ) const;
        bool check_weather_requirements( weather_type t );
        int get_wind_direction( season_type ) const;
        int convert_winddir( int ) const;
        int get_water_temperature() const;
        void test_weather( unsigned ) const;

        double get_weather_temperature( const tripoint &, const time_point &, unsigned ) const;

        static weather_generator load( const JsonObject &jo );
};

#endif // CATA_SRC_WEATHER_GEN_H
