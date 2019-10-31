#pragma once
#ifndef WEATHER_GEN_H
#define WEATHER_GEN_H

#include <string>

#include "calendar.h"

struct tripoint;
class JsonObject;

enum weather_type : int;

struct w_point {
    double temperature;
    double humidity;
    double pressure;
    double windpower;
    std::string wind_desc;
    int winddirection;
    bool   acidic;
};

class weather_generator
{
    public:
        // Average temperature
        double base_temperature;
        // Average humidity
        double base_humidity;
        // Average atmospheric pressure
        double base_pressure;
        double base_acid;
        //Average yearly windspeed
        double base_wind;
        //How much the wind peaks above average
        int base_wind_distrib_peaks;
        int summer_temp_manual_mod = 0;
        int spring_temp_manual_mod = 0;
        int autumn_temp_manual_mod = 0;
        int winter_temp_manual_mod = 0;
        int spring_humidity_manual_mod = 0;
        int summer_humidity_manual_mod = 0;
        int autumn_humidity_manual_mod = 0;
        int winter_humidity_manual_mod = 0;
        //How much the wind folows seasonal variation ( lower means more change )
        int base_wind_season_variation;
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
        int get_wind_direction( season_type, unsigned seed ) const;
        int convert_winddir( int ) const;
        int get_water_temperature() const;
        void test_weather( unsigned ) const;

        double get_weather_temperature( const tripoint &, const time_point &, unsigned ) const;

        static weather_generator load( JsonObject &jo );
};

#endif
