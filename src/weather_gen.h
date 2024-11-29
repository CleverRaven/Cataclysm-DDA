#pragma once
#ifndef CATA_SRC_WEATHER_GEN_H
#define CATA_SRC_WEATHER_GEN_H

#include <iosfwd>
#include <map>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "regional_settings.h"
#include "type_id.h"
#include "units.h"

class JsonObject;
struct tripoint;

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
        static int current_winddir;
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
        units::temperature get_weather_temperature( const tripoint &, const time_point &, unsigned ) const;

        static weather_generator load( const JsonObject &jo );
    private:
        const overmap_weather_settings *s;
};

#endif // CATA_SRC_WEATHER_GEN_H
