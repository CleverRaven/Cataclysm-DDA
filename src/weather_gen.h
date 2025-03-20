#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

class JsonObject;

struct w_point {
    units::temperature temperature = 0_K;
    double humidity = 0;
    double pressure = 0;
    double windpower = 0;
    std::string wind_desc;
    int winddirection = 0;
    season_effective_time time;
    tripoint_abs_ms location;
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
        //Average yearly windspeed
        double base_wind = 0;
        //How much the wind peaks above average
        int base_wind_distrib_peaks = 0;
        // TODO: Store as arrays?
        int summer_temp_manual_mod = 0;
        int spring_temp_manual_mod = 0;
        int autumn_temp_manual_mod = 0;
        int winter_temp_manual_mod = 0;
        int spring_humidity_manual_mod = 0;
        int summer_humidity_manual_mod = 0;
        int autumn_humidity_manual_mod = 0;
        int winter_humidity_manual_mod = 0;
        //How much the wind follows seasonal variation ( lower means more change )
        int base_wind_season_variation = 0;
        static int current_winddir;
        // TODO: Use std::vector<weather_type_id> and finalise in region settings instead?
        std::vector<std::string> weather_black_list;
        std::vector<std::string> weather_white_list;
        /** All the current weather types based on white or black list and sorted by load order */
        std::vector<weather_type_id> sorted_weather;
        weather_generator();

        /**
         * Input should be an absolute position in the map square system (the one used
         * by the @ref map). You can use @ref map::getabs to get an absolute position from a
         * relative position (relative to the map you called getabs on).
         */
        w_point get_weather( const tripoint_abs_ms &, const time_point &, unsigned ) const;
        weather_type_id get_weather_conditions( const tripoint_abs_ms &, const time_point &,
                                                unsigned seed ) const;
        weather_type_id get_weather_conditions( const w_point & ) const;
        int get_wind_direction( season_type ) const;
        int convert_winddir( int ) const;
        units::temperature get_water_temperature() const;
        void test_weather( unsigned seed ) const;
        void sort_weather();
        units::temperature get_weather_temperature( const tripoint_abs_ms &, const time_point &,
                unsigned ) const;

        void load( const JsonObject &jo, bool was_loaded );
};

#endif // CATA_SRC_WEATHER_GEN_H
