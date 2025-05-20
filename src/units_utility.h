#pragma once
#include <cmath>
#include <string>
#include <type_traits>
#include <utility>
#ifndef CATA_SRC_UNITS_UTILITY_H
#define CATA_SRC_UNITS_UTILITY_H

#include "units.h"

/** Divide @p num by @p den, rounding up
 *
 * @p num must be non-negative, @p den must be positive, and @c num+den must not overflow.
 */
template<typename T, typename U>
T divide_round_up( units::quantity<T, U> num, units::quantity<T, U> den )
{
    return divide_round_up( num.value(), den.value() );
}

/** Given an angle, add or subtract multiples of 360_degrees until it's in the
 * range [0, 360_degrees).
 *
 * With a second argument, can use a different maximum.
 */
units::angle normalize( units::angle a, units::angle mod = 360_degrees );

// @example angle_delta( 270_degrees, 0_degrees ) == 90_degrees
// @example angle_delta( 90_degrees, 0_degrees ) == 90_degrees
// @returns the smaller difference angle between \p a and \p b angles
units::angle angle_delta( units::angle a, units::angle b );

// convert angle to nearest of 0=north, 1=west, 2=south, 3=east, NE/NW return N, SE/SW return S,
// this is used to render vehicle parts in tiles mode where they only have 4 orientations and
// biased to north / south.
int angle_to_dir4( units::angle direction );

// convert angle to nearest of 0=north 1=NE 2=east 3=SE...
int angle_to_dir8( units::angle direction );

template<typename T, typename U, std::enable_if_t<std::is_floating_point_v<T>>* = nullptr>
units::quantity<T, U> round_to_multiple_of( units::quantity<T, U> val, units::quantity<T, U> of )
{
    int multiple = std::lround( val / of );
    return multiple * of;
}

struct lat_long {
    units::angle latitude;
    units::angle longitude;
};

/**
 * Create a units label for a weight value.
 *
 * Gives the name of the weight unit in the user selected unit system, either
 * "kgs" or "lbs".  Used to add unit labels to the output of @ref convert_weight.
 *
 * @return name of unit
 */
const char *weight_units();

/**
 * Create an abbreviated units label for a volume value.
 *
 * Returns the abbreviated name for the volume unit for the user selected unit system,
 * i.e. "c", "L", or "qt". Used to add unit labels to the output of @ref convert_volume.
 *
 * @return name of unit.
 */
const char *volume_units_abbr();

/**
 * Create a units label for a volume value.
 *
 * Returns the abbreviated name for the volume unit for the user selected unit system,
 * ie "cup", "liter", or "quart". Used to add unit labels to the output of @ref convert_volume.
 *
 * @return name of unit.
 */
const char *volume_units_long();

/**
 * Convert weight in grams to units defined by user (kg or lbs)
 *
 * @param weight to be converted.
 *
 * @returns Weight converted to user selected unit
 */
double convert_weight( const units::mass &weight );

/**
 * converts length to largest unit available
 * 1000 mm = 1 meter for example
 * assumed to be used in conjunction with unit string functions
 * also works for imperial units
 */
int convert_length( const units::length &length );
std::string length_units( const units::length &length );
std::string length_to_string( const units::length &length, bool compact = false );

/**
* Rounds length so that reasonably large units can be used (kilometers/meters or miles/yards).
* If value is >0.5km or >0.5mi then uses km/mi respectively, otherwise uses meters/yards.
* Outputs to two decimal places (km/mi) or as integer (meters/yards).
* Should always be accessed through length_to_string_approx()
*/
double convert_length_approx( const units::length &length, bool &display_as_integer );
std::string length_units_approx( const units::length &length );
std::string length_to_string_approx( const units::length &length );

/** Convert length to inches or cm. Used in pickup UI */
double convert_length_cm_in( const units::length &length );

/** convert a mass unit to a string readable by a human */
std::string weight_to_string( const units::mass &weight, bool compact = false,
                              bool remove_trailing_zeroes = false );

/**
 * Convert high-definition weight/mass to readable format
 * Always metric units. First is value as string, second is unit
 */
std::pair<std::string, std::string> weight_to_string( const
        units::quantity<int, units::mass_in_microgram_tag> &weight );

/**
 * Convert volume from ml to units defined by user.
 */
double convert_volume( int volume );

/**
 * Convert volume from ml to units defined by user,
 * optionally returning the units preferred scale.
 */
double convert_volume( int volume, int *out_scale );

/** convert a volume unit to a string readable by a human */
std::string vol_to_string( const units::volume &vol, bool compact = false,
                           bool remove_trailing_zeroes = false );

/** convert any type of unit to a string readable by a human */
std::string unit_to_string( const units::volume &unit, bool compact = false,
                            bool remove_trailing_zeroes = false );
std::string unit_to_string( const units::mass &unit, bool compact = false,
                            bool remove_trailing_zeroes = false );
std::string unit_to_string( const units::length &unit, bool compact = false );

/** utility function to round with specified decimal places */
double round_with_places( double value, int decimal_places );
#endif // CATA_SRC_UNITS_UTILITY_H
