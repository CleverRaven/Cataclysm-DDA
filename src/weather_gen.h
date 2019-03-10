#pragma once
#ifndef WEATHER_GEN_H
#define WEATHER_GEN_H

#include "calendar.h"
struct point;
struct tripoint;
class time_point;
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
        double base_temperature = 6.5; // Average temperature of New England
        double base_humidity = 66.0; // Average humidity
        double base_pressure = 1015.0; // Average atmospheric pressure
        double base_acid = 0.0;
        double base_wind = 5.7; //Average yearly windspeed of New England
        int base_wind_distrib_peaks = 30; //How much the wind peaks above average
        int base_wind_season_variation =
            64; //How much the wind folows seasonal variation ( lower means more change )
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
        int get_wind_direction( const season_type, unsigned seed ) const;
        int convert_winddir( const int ) const;
        std::string get_wind_desc( double ) const;
        int get_water_temperature() const;
        void test_weather() const;

        static weather_generator load( JsonObject &jo );
};

#endif
