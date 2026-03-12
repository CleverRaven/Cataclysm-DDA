#include "options_helpers.h"

#include <optional>

#include "calendar.h"
#include "options.h"
#include "weather.h"
#include "weather_type.h"

override_option::override_option( const std::string &option, const std::string &value ) :
    option_( option )
{
    options_manager::cOpt &opt = get_options().get_option( option_ );
    old_value_ = opt.getValue( true );
    opt.setValue( value );
}

override_option::~override_option()
{
    get_options().get_option( option_ ).setValue( old_value_ );
}

scoped_weather_override::scoped_weather_override( const weather_type_id &weather )
{
    get_weather().weather_override = weather;
    get_weather().set_nextweather( calendar::turn );
}

void scoped_weather_override::with_windspeed( const int windspeed_override )
{
    get_weather().windspeed_override = windspeed_override;
    get_weather().set_nextweather( calendar::turn );
}

scoped_weather_override::~scoped_weather_override()
{
    weather_manager &weather = get_weather();
    weather.weather_override = WEATHER_NULL;
    weather.windspeed_override.reset();
    get_weather().set_nextweather( calendar::turn );
}
