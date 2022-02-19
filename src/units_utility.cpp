#include "units_utility.h"

#include <string>

#include "cata_utility.h"
#include "options.h"
#include "string_formatter.h"
#include "translations.h"

units::angle normalize( units::angle a, const units::angle &mod )
{
    a = units::fmod( a, mod );
    if( a < 0_degrees ) {
        a += mod;
    }
    return a;
}

const char *weight_units()
{
    return get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "lbs" ? _( "lbs" ) : _( "kg" );
}

const char *volume_units_abbr()
{
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        return pgettext( "Volume unit", "c" );
    } else if( vol_units == "l" ) {
        return pgettext( "Volume unit", "L" );
    } else {
        return pgettext( "Volume unit", "qt" );
    }
}

const char *volume_units_long()
{
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        return _( "cup" );
    } else if( vol_units == "l" ) {
        return _( "liter" );
    } else {
        return _( "quart" );
    }
}

double convert_velocity( int velocity, const units_type vel_units )
{
    const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
    // internal units to mph conversion
    double ret = static_cast<double>( velocity ) / 100;

    if( type == "km/h" ) {
        switch( vel_units ) {
            case VU_VEHICLE:
                // mph to km/h conversion
                ret *= 1.609f;
                break;
            case VU_WIND:
                // mph to m/s conversion
                ret *= 0.447f;
                break;
        }
    } else if( type == "t/t" ) {
        ret /= 4;
    }

    return ret;
}
double convert_weight( const units::mass &weight )
{
    double ret = to_gram( weight );
    if( get_option<std::string>( "USE_METRIC_WEIGHTS" ) == "kg" ) {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

double convert_length_cm_in( const units::length &length )
{

    double ret = to_millimeter( length );
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
    if( metric ) {
        ret /= 10;
    } else {
        // imperial's a doozy, we can only try to approximate
        // so first we convert it to inches which are the smallest unit
        ret /= 25.4;
    }
    return ret;
}

int convert_length( const units::length &length )
{
    int ret = to_millimeter( length );
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
    if( metric ) {
        if( ret % 1'000'000 == 0 ) {
            // kilometers
            ret /= 1'000'000;
        } else if( ret % 1'000 == 0 ) {
            // meters
            ret /= 1'000;
        } else if( ret % 10 == 0 ) {
            // centimeters
            ret /= 10;
        }
    } else {
        // imperial's a doozy, we can only try to approximate
        // so first we convert it to inches which are the smallest unit
        ret /= 25.4;
        if( ret % 63360 == 0 ) {
            ret /= 63360;
        } else if( ret % 36 == 0 ) {
            ret /= 36;
        } else if( ret % 12 == 0 ) {
            ret /= 12;
        }
    }
    return ret;
}

std::string length_units( const units::length &length )
{
    int length_mm = to_millimeter( length );
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
    if( metric ) {
        if( length_mm % 1'000'000 == 0 ) {
            //~ kilometers
            return _( "km" );
        } else if( length_mm % 1'000 == 0 ) {
            //~ meters
            return _( "m" );
        } else if( length_mm % 10 == 0 ) {
            //~ centimeters
            return _( "cm" );
        } else {
            //~ millimeters
            return _( "mm" );
        }
    } else {
        // imperial's a doozy, we can only try to approximate
        // so first we convert it to inches which are the smallest unit
        length_mm /= 25.4;
        if( length_mm == 0 ) {
            //~ inches
            return _( "in." );
        }
        if( length_mm % 63360 == 0 ) {
            //~ miles
            return _( "mi" );
        } else if( length_mm % 36 == 0 ) {
            //~ yards (length)
            return _( "yd" );
        } else if( length_mm % 12 == 0 ) {
            //~ feet (length)
            return _( "ft" );
        } else {
            //~ inches
            return _( "in." );
        }
    }
}

std::string length_to_string( const units::length &length, const bool compact )
{
    const int converted_length = convert_length( length );
    std::string string_to_format = "%u%s%s";
    return string_format( string_to_format, converted_length, compact ? "" : " ",
                          length_units( length ) );
}

std::string weight_to_string( const units::mass &weight, const bool compact,
                              const bool remove_trailing_zeroes )
{
    const int default_decimal_places = 2;
    const double converted_weight = round_with_places( convert_weight( weight ),
                                    default_decimal_places );
    std::string string_to_format = remove_trailing_zeroes ? "%g%s%s" : "%." +
                                   std::to_string( default_decimal_places ) + "f%s%s";
    return string_format( string_to_format, converted_weight, compact ? "" : " ", weight_units() );
}

double convert_volume( int volume )
{
    return convert_volume( volume, nullptr );
}

double convert_volume( int volume, int *out_scale )
{
    double ret = volume;
    int scale = 0;
    const std::string vol_units = get_option<std::string>( "VOLUME_UNITS" );
    if( vol_units == "c" ) {
        ret *= 0.004;
        scale = 1;
    } else if( vol_units == "l" ) {
        ret *= 0.001;
        scale = 2;
    } else {
        ret *= 0.00105669;
        scale = 2;
    }
    if( out_scale != nullptr ) {
        *out_scale = scale;
    }
    return ret;
}

std::string vol_to_string( const units::volume &vol, const bool compact,
                           const bool remove_trailing_zeroes )
{
    int converted_volume_scale = 0;
    const int default_decimal_places = 3;
    const double converted_volume =
        round_with_places( convert_volume( vol.value(),
                                           &converted_volume_scale ), default_decimal_places );
    std::string string_to_format = remove_trailing_zeroes ? "%g%s%s" : "%." +
                                   std::to_string( default_decimal_places ) + "f%s%s";
    return string_format( string_to_format, converted_volume, compact ? "" : " ", volume_units_abbr() );
}

std::string unit_to_string( const units::volume &unit, const bool compact,
                            const bool remove_trailing_zeroes )
{
    return vol_to_string( unit, compact, remove_trailing_zeroes );
}
std::string unit_to_string( const units::mass &unit, const bool compact,
                            const bool remove_trailing_zeroes )
{
    return weight_to_string( unit, compact, remove_trailing_zeroes );
}
std::string unit_to_string( const units::length &unit, const bool compact )
{
    return length_to_string( unit, compact );
}

/**
 * round a float @value, with int @decimal_places limitation
*/
double round_with_places( double value, int decimal_places )
{
    const double multiplier = std::pow( 10.0, decimal_places );
    return std::round( value * multiplier ) / multiplier;
}
