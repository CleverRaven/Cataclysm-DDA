#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <string>
#include <climits>

#include "calendar.h"
#include "color.h"
#include "weather_type.h"

struct tripoint;
class JsonObject;

struct w_point {
    double temperature = 0;
    double humidity = 0;
    double pressure = 0;
    double windpower = 0;
    std::string wind_desc;
    int winddirection = 0;
    bool acidic = false;
    time_point time;
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
        std::vector<std::string> weather_types;
        weather_generator();

        /**
         * Input should be an absolute position in the map square system (the one used
         * by the @ref map). You can use @ref map::getabs to get an absolute position from a
         * relative position (relative to the map you called getabs on).
         */
        w_point get_weather( const tripoint &, const time_point &, unsigned ) const;
        weather_type_id get_weather_conditions( const tripoint &, const time_point &,
                                                unsigned seed, std::map<weather_type_id, time_point> &next_instance_allowed ) const;
        weather_type_id get_weather_conditions( const w_point &,
                                                std::map<weather_type_id, time_point> &next_instance_allowed ) const;
        int get_wind_direction( season_type ) const;
        int convert_winddir( int ) const;
        int get_water_temperature() const;
        void test_weather( unsigned seed,
                           std::map<weather_type_id, time_point> &next_instance_allowed ) const;

        double get_weather_temperature( const tripoint &, const time_point &, unsigned ) const;

        static weather_generator load( const JsonObject &jo );
};

#endif // CATA_SRC_WEATHER_GEN_H
