#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <string>
#include <vector>
#include "calendar.h"

struct tripoint;
class JsonObject;

enum weather_type : int;

struct w_point {
    double temperature = 0;
    double humidity = 0;
    double pressure = 0;
    double windpower = 0;
    std::string wind_desc;
    int winddirection = 0;
    bool acidic = false;
};

struct weather_spawn_info {
    bool hallucinations;
    std::vector<std::string> spawns;
    time_duration time_between_spawns;
    int chance_to_spawn;
    int max_spawns;
    int max_radius;
    int min_radius;
    std::string message;
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
        //what monsters to spawn for the weather
        std::map<int, weather_spawn_info> weather_spawn_info_map;
        //Regional settings can disable mist in a region
        bool mist_active = true;
        //increase mist difficulty
        float mist_scaling = 1.0;
        //average number of days between mist
        int mist_frequency = 7;
        //number of hours on average the mist lasts
        int mist_length = 7;
        //amount mist intensity increases before ending
        int mist_increases_per = 10;
        //the intensity at which mist switches to thick
        int mist_thick_threshold = 10;
        //the intensity at which mist switches to stifling
        int mist_stifling_threshold = 20;

        weather_generator();

        /**
         * Input should be an absolute position in the map square system (the one used
         * by the @ref map). You can use @ref map::getabs to get an absolute position from a
         * relative position (relative to the map you called getabs on).
         */
        w_point get_weather( const tripoint &, const time_point &, unsigned ) const;
        weather_type get_weather_conditions( const tripoint &, const time_point &, unsigned seed ) const;
        weather_type get_weather_conditions( const w_point & ) const;
        int get_wind_direction( season_type ) const;
        int convert_winddir( int ) const;
        int get_water_temperature() const;
        void test_weather( unsigned ) const;

        double get_weather_temperature( const tripoint &, const time_point &, unsigned ) const;

        static weather_generator load( const JsonObject &jo );
};

#endif // CATA_SRC_WEATHER_GEN_H
