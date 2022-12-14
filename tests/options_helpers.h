#pragma once
#ifndef CATA_TESTS_OPTIONS_HELPERS_H
#define CATA_TESTS_OPTIONS_HELPERS_H

#include <string> // IWYU pragma: keep
#include "type_id.h"

// RAII class to temporarily override a particular option value
// The previous value will be restored in the destructor
class override_option
{
    public:
        override_option( const std::string &option, const std::string &value );
        override_option( const override_option & ) = delete;
        override_option &operator=( const override_option & ) = delete;
        ~override_option();
    private:
        std::string option_;
        std::string old_value_;
};

// RAII class to temporarily override the game's weather and restore it to
// default weather in the destructor.
class scoped_weather_override
{
    public:
        explicit scoped_weather_override( const weather_type_id & );
        void with_windspeed( int windspeed_override );
        scoped_weather_override( const scoped_weather_override & ) = delete;
        scoped_weather_override &operator=( const scoped_weather_override & ) = delete;
        ~scoped_weather_override();
};

#endif // CATA_TESTS_OPTIONS_HELPERS_H
