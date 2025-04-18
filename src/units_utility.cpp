#include "units_utility.h"

#include <string>

#include "cata_utility.h"
#include "options.h"
#include "string_formatter.h"
#include "translations.h"

units::angle normalize( units::angle a, units::angle mod )
{
    a = units::fmod( a, mod );
    if( a < 0_degrees ) {
        a += mod;
    }
    return a;
}

units::angle angle_delta( units::angle a, units::angle b )
{
    const units::angle delta = normalize( a - b );
    return delta > 180_degrees ? 360_degrees - delta : delta;
}

int angle_to_dir4( const units::angle direction )
{
    const int dir8 = angle_to_dir8( direction );
    // direction is snapped to 8 directions in angle_to_dir8, then we need to further snap it
    // to just 4 directions, used for tilesets; for consistent visuals NE/NW diagonals snap to north
    // and SE/SW diagonals snap to south
    // NOLINTBEGIN(bugprone-branch-clone)
    switch( dir8 ) {
        // *INDENT-OFF*
        case 0:  return 0; // north
        case 1:  return 0; // north <- north west
        case 2:  return 1; // west
        case 3:  return 2; // south <- south west
        case 4:  return 2; // south
        case 5:  return 2; // south <- south east
        case 6:  return 3; // east
        case 7:  return 0; // north <- north east

        default: return 0;
        // *INDENT-ON*
    }
    // NOLINTEND(bugprone-branch-clone)
}

int angle_to_dir8( const units::angle direction )
{
    // Direction is snapped to "notches" of 15-degrees on a circle, we bump direction towards the
    // middle of each notch, and lookup the 8-way direction. Cardinal directions get 5 "notches", diagonals
    // get 1 at the boundary of each 2 cardinals. This can be shorter but this way is easier to read,
    // the compilers will optimize into lookup table. This is mostly used in curses mode.
    constexpr units::angle bump = 15_degrees / 2.0;
    const int snap = normalize( direction - bump ) / 15_degrees + 1;
    // NOLINTBEGIN(bugprone-branch-clone)
    switch( snap ) {
        // *INDENT-OFF*
        case 22: return 0; // north
        case 23: return 0; // north
        case 24: return 0; // north      (   0 degrees )
        case 1:  return 0; // north
        case 2:  return 0; // north
        case 3:  return 1; // north-west (  45 degrees )
        case 4:  return 2; // west
        case 5:  return 2; // west
        case 6:  return 2; // west       (  90 degrees )
        case 7:  return 2; // west
        case 8:  return 2; // west
        case 9:  return 3; // south-west ( 135 degrees )
        case 10: return 4; // south
        case 11: return 4; // south
        case 12: return 4; // south      ( 180 degrees )
        case 13: return 4; // south
        case 14: return 4; // south
        case 15: return 5; // south-east ( 225 degrees )
        case 16: return 6; // east
        case 17: return 6; // east
        case 18: return 6; // east       ( 270 degrees )
        case 19: return 6; // east
        case 20: return 6; // east
        case 21: return 7; // north-east ( 315 degrees )

        default: return 0; // other values can't happen
        // *INDENT-ON*
    }
    // NOLINTEND(bugprone-branch-clone)
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
        if( ret % 1000000 == 0 ) {
            // kilometers
            ret /= 1000000;
        } else if( ret % 1000 == 0 ) {
            // meters
            ret /= 1000;
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
        if( length_mm % 1000000 == 0 ) {
            //~ kilometers
            return _( "km" );
        } else if( length_mm % 1000 == 0 ) {
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

double convert_length_approx( const units::length &length, bool &display_as_integer )
{
    double ret = static_cast<double>( to_millimeter( length ) );
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
    if( metric ) {
        if( ret > 500000 ) {
            // kilometers
            ret /= 1000000.0;
        } else {
            // meters
            ret /= 1000.0;
            display_as_integer = true;
        }
    } else {
        double inches_value = ret / 25.4;
        if( inches_value > 31680 ) {
            // Miles
            inches_value /= 63360.0;
        } else {
            // Yards
            inches_value /= 36.0;
            display_as_integer = true;
        }
        ret = inches_value;
    }
    return ret;
}

std::string length_units_approx( const units::length &length )
{
    int length_mm = to_millimeter( length );
    const bool metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
    if( metric ) {
        if( length_mm > 500000 ) {
            //~ kilometers
            return _( "km" );
        } else {
            //~ meters
            return _( "m" );
        }
    } else {
        int length_inches = length_mm / 25.4;
        if( length_inches > 31680 ) {
            //~ miles
            return _( "mi" );
        } else {
            //~ yards (length)
            return _( "yd" );
        }
    }
}

std::string length_to_string_approx( const units::length &length )
{
    bool display_as_integer = false;
    double approx_length = convert_length_approx( length, display_as_integer );
    std::string string_to_format = "%.2f%s";
    if( display_as_integer ) {
        string_to_format = "%u%s";
        int approx_length_as_integer = static_cast<int>( approx_length );
        return string_format( string_to_format, approx_length_as_integer, length_units_approx( length ) );
    } else {
        return string_format( string_to_format, approx_length, length_units_approx( length ) );
    }
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

std::pair<std::string, std::string> weight_to_string( const
        units::quantity<int, units::mass_in_microgram_tag> &weight )
{
    using high_res_mass = units::quantity<int, units::mass_in_microgram_tag>;
    static const high_res_mass gram = high_res_mass( 1000000, {} );
    static const high_res_mass milligram = high_res_mass( 1000, {} );

    if( weight > gram ) {
        return {string_format( "%.0f", weight.value() / 1000000.f ), "g"};
    } else if( weight > milligram ) {
        return {string_format( "%.0f", weight.value() / 1000.f ), "mg"};
    }
    return {string_format( "%d", weight.value() ), "μg"};
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
