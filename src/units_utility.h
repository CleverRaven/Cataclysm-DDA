#pragma once
#include <cmath>
#include <iosfwd>
#include <type_traits>
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
units::angle normalize( units::angle a, const units::angle &mod = 360_degrees );

template<typename T, typename U, std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
units::quantity<T, U> round_to_multiple_of( units::quantity<T, U> val, units::quantity<T, U> of )
{
    int multiple = std::lround( val / of );
    return multiple * of;
}

struct lat_long {
    units::angle latitude;
    units::angle longitude;
};

constexpr lat_long location_boston{ 42.36_degrees, -71.06_degrees };

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

/** Convert length to inches or cm. Used in pickup UI */
double convert_length_cm_in( const units::length &length );

/** convert a mass unit to a string readable by a human */
std::string weight_to_string( const units::mass &weight, bool compact = false,
                              bool remove_trailing_zeroes = false );

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
