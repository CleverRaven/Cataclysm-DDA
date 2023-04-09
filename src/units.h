#pragma once
#ifndef CATA_SRC_UNITS_H
#define CATA_SRC_UNITS_H

#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "json.h"
#include "math_defines.h"
#include "translations.h"
#include "units_fwd.h" // IWYU pragma: export

class time_duration;

namespace units
{

template<typename V, typename U>
class quantity
{
    public:
        using value_type = V;
        using unit_type = U;
        using this_type = quantity<value_type, unit_type>;

        /**
         * Create an empty quantity - its @ref value_ is value initialized.
         * It does not need an explicitly named unit, it's always 0: 0 l == 0 ml == 0 Ml.
         */
        constexpr quantity() : value_() {
        }
        /**
         * Construct from value. This is supposed to be wrapped into a static
         * function (e.g. `from_liter(int)` ) to provide context.
         */
        constexpr quantity( const value_type &v, unit_type ) : value_( v ) {
        }
        /**
         * Conversion from other value type, e.g. from `quantity<int, foo>` to
         * `quantity<float, foo>`. The unit type stays the same!
         */
        template<typename other_value_type>
        // NOLINTNEXTLINE(google-explicit-constructor)
        constexpr quantity( const quantity<other_value_type, unit_type> &other ) :
            value_( other.value() ) {
        }

        /**
         * Access the raw dimensionless value. Use it in a properly named wrapper function only.
         */
        constexpr const value_type &value() const {
            return value_;
        }

        /**
         * The usual comparators, they compare the base value only.
         */
        /**@{*/
        constexpr bool operator==( const this_type &rhs ) const {
            return value_ == rhs.value_;
        }
        constexpr bool operator!=( const this_type &rhs ) const {
            return !operator==( rhs );
        }
        constexpr bool operator<( const this_type &rhs ) const {
            return value_ < rhs.value_;
        }
        constexpr bool operator>=( const this_type &rhs ) const {
            return !operator<( rhs );
        }
        constexpr bool operator>( const this_type &rhs ) const {
            return value_ > rhs.value_;
        }
        constexpr bool operator<=( const this_type &rhs ) const {
            return !operator>( rhs );
        }
        /**@}*/

        /**
         * Addition and subtraction of quantities of the same unit type. Result is
         * a quantity with the same unit as the input.
         * Functions are templated to allow combining quantities with different `value_type`, e.g.
         * \code
         *   quantity<int, foo> a = ...;
         *   quantity<double, foo> b = ...;
         *   auto sum = a + b;
         *   static_assert(std::is_same<decltype(sum), quantity<double, foo>>::value);
         * \endcode
         *
         * Note that `+=` and `-=` accept any type as `value_type` for the other operand, but
         * they convert this back to the type of the right hand, like in `int a; a += 0.4;`
         * \code
         *   quantity<int, foo> a( 10, foo{} );
         *   quantity<double, foo> b( 0.5, foo{} );
         *   a += b;
         *   cata_assert( a == quantity<int, foo>( 10 + 0.5, foo{} ) );
         *   cata_assert( a == quantity<int, foo>( 10, foo{} ) );
         * \endcode
         */
        /**@{*/
        template<typename other_value_type>
        constexpr quantity < decltype( std::declval<value_type>() + std::declval<other_value_type>() ),
                  unit_type >
        operator+( const quantity<other_value_type, unit_type> &rhs ) const {
            return { value_ + rhs.value(), unit_type{} };
        }
        template<typename other_value_type>
        constexpr quantity < decltype( std::declval<value_type>() + std::declval<other_value_type>() ),
                  unit_type >
        operator-( const quantity<other_value_type, unit_type> &rhs ) const {
            return { value_ - rhs.value(), unit_type{} };
        }

        template<typename other_value_type>
        this_type &operator+=( const quantity<other_value_type, unit_type> &rhs ) {
            value_ += rhs.value();
            return *this;
        }
        template<typename other_value_type>
        this_type &operator-=( const quantity<other_value_type, unit_type> &rhs ) {
            value_ -= rhs.value();
            return *this;
        }
        /**@}*/

        constexpr this_type operator-() const {
            return this_type( -value_, unit_type{} );
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonValue &jv );

    private:
        value_type value_;
};

template<typename V, typename U>
inline quantity<V, U> fabs( quantity<V, U> q )
{
    return quantity<V, U>( std::fabs( q.value() ), U{} );
}

template<typename V, typename U>
inline quantity<V, U> fmod( quantity<V, U> num, quantity<V, U> den )
{
    return quantity<V, U>( std::fmod( num.value(), den.value() ), U{} );
}

/**
 * Multiplication and division with scalars. Result is a quantity with the same unit
 * as the input.
 * Functions are templated to allow scaling with different types:
 * \code
 *   quantity<int, foo> a{ 10, foo{} };
 *   auto b = a * 4.52;
 *   static_assert(std::is_same<decltype(b), quantity<double, foo>>::value);
 * \endcode
 *
 * Note that the result for `*=` and `/=` is calculated using the given types, but is
 * implicitly converted back to `value_type` as it is stored in the operand.
 * \code
 *   quantity<int, foo> a{ 10, foo{} };
 *   a *= 4.52;
 *   cata_assert( a == quantity<int, foo>( 10 * 4.52, foo{} ) );
 *   cata_assert( a != quantity<int, foo>( 10 * (int)4.52, foo{} ) );
 *   cata_assert( a == quantity<int, foo>( 45, foo{} ) );
 * \endcode
 *
 * Division of a quantity with a quantity of the same unit yields a dimensionless
 * scalar value, with the same type as the division of the contained `value_type`s:
 * \code
 *   quantity<int, foo> a{ 10, foo{} };
 *   quantity<double, foo> b{ 20, foo{} };
 *   auto proportion = a / b;
 *   static_assert(std::is_same<decltype(proportion), double>::value);
 *   cata_assert( proportion == 10 / 20.0 );
 * \endcode
 *
 */
/**@{*/
// the decltype in the result type ensures the returned type has the same scalar type
// as you would get when performing the operation directly:
// `int * double` => `double` and `char * int` => `int`
// st is scalar type (dimensionless)
// lvt is the value type (of a quantity) on the left side, rvt is the value type on the right side
// ut is unit type (same for left and right side)
// The enable_if ensures no ambiguity, the compiler may otherwise not be able to decide whether
// "quantity / scalar" or "quantity / other_quanity" is meant.

// scalar * quantity<foo, unit> == quantity<decltype(foo * scalar), unit>
template<typename lvt, typename ut, typename st, typename = typename std::enable_if<std::is_arithmetic<st>::value>::type>
inline constexpr quantity<decltype( std::declval<lvt>() * std::declval<st>() ), ut>
operator*( const st &factor, const quantity<lvt, ut> &rhs )
{
    return { factor * rhs.value(), ut{} };
}

// same as above only with inverse order of operands: quantity * scalar
template<typename lvt, typename ut, typename st, typename = typename std::enable_if<std::is_arithmetic<st>::value>::type>
inline constexpr quantity<decltype( std::declval<st>() * std::declval<lvt>() ), ut>
operator*( const quantity<lvt, ut> &lhs, const st &factor )
{
    return { lhs.value() *factor, ut{} };
}

// quantity<foo, unit> * quantity<bar, unit> is not supported
template<typename lvt, typename ut, typename rvt, typename = typename std::enable_if<std::is_arithmetic<lvt>::value>::type>
inline void operator*( quantity<lvt, ut>, quantity<rvt, ut> ) = delete;

// operator *=
template<typename lvt, typename ut, typename st, typename = typename std::enable_if<std::is_arithmetic<st>::value>::type>
inline quantity<lvt, ut> &
operator*=( quantity<lvt, ut> &lhs, const st &factor )
{
    lhs = lhs * factor;
    return lhs;
}

// and the revers of the multiplication above:
// quantity<foo, unit> / scalar == quantity<decltype(foo / scalar), unit>
template<typename lvt, typename ut, typename rvt, typename = typename std::enable_if<std::is_arithmetic<rvt>::value>::type>
inline constexpr quantity<decltype( std::declval<lvt>() * std::declval<rvt>() ), ut>
operator/( const quantity<lvt, ut> &lhs, const rvt &divisor )
{
    return { lhs.value() / divisor, ut{} };
}

// scalar / quantity<foo, unit> is not supported
template<typename lvt, typename ut, typename rvt, typename = typename std::enable_if<std::is_arithmetic<lvt>::value>::type>
inline void operator/( lvt, quantity<rvt, ut> ) = delete;

// quantity<foo, unit> / quantity<bar, unit> == decltype(foo / bar)
template<typename lvt, typename ut, typename rvt>
inline constexpr decltype( std::declval<lvt>() / std::declval<rvt>() )
operator/( const quantity<lvt, ut> &lhs, const quantity<rvt, ut> &rhs )
{
    return lhs.value() / rhs.value();
}

// operator /=
template<typename lvt, typename ut, typename st, typename = typename std::enable_if<std::is_arithmetic<st>::value>::type>
inline quantity<lvt, ut> &
operator/=( quantity<lvt, ut> &lhs, const st &divisor )
{
    lhs = lhs / divisor;
    return lhs;
}

// remainder:
// quantity<foo, unit> % scalar == quantity<decltype(foo % scalar), unit>
template<typename lvt, typename ut, typename rvt, typename = typename std::enable_if<std::is_arithmetic<rvt>::value>::type>
inline constexpr quantity < decltype( std::declval<lvt>() % std::declval<rvt>() ), ut >
operator%( const quantity<lvt, ut> &lhs, const rvt &divisor )
{
    return { lhs.value() % divisor, ut{} };
}

// scalar % quantity<foo, unit> is not supported
template<typename lvt, typename ut, typename rvt, typename = typename std::enable_if<std::is_arithmetic<lvt>::value>::type>
inline void operator%( lvt, quantity<rvt, ut> ) = delete;

// quantity<foo, unit> % quantity<bar, unit> == decltype(foo % bar)
template<typename lvt, typename ut, typename rvt>
inline constexpr quantity < decltype( std::declval<lvt>() % std::declval<rvt>() ), ut >
operator%( const quantity<lvt, ut> &lhs, const quantity<rvt, ut> &rhs )
{
    return { lhs.value() % rhs.value(), ut{} };
}

// operator %=
template<typename lvt, typename ut, typename st, typename = typename std::enable_if<std::is_arithmetic<st>::value>::type>
inline quantity<lvt, ut> &
operator%=( quantity<lvt, ut> &lhs, const st &divisor )
{
    lhs = lhs % divisor;
    return lhs;
}
template<typename lvt, typename ut, typename rvt>
inline quantity<lvt, ut> &
operator%=( quantity<lvt, ut> &lhs, const quantity<rvt, ut> &rhs )
{
    lhs = lhs % rhs;
    return lhs;
}
/**@}*/

const volume volume_min = units::volume( std::numeric_limits<units::volume::value_type>::min(),
                          units::volume::unit_type{} );

const volume volume_max = units::volume( std::numeric_limits<units::volume::value_type>::max(),
                          units::volume::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, volume_in_milliliter_tag> from_milliliter(
    const value_type v )
{
    return quantity<value_type, volume_in_milliliter_tag>( v, volume_in_milliliter_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, volume_in_milliliter_tag> from_liter( const value_type v )
{
    return from_milliliter<value_type>( v * 1000 );
}

template<typename value_type>
inline constexpr value_type to_milliliter( const quantity<value_type, volume_in_milliliter_tag> &v )
{
    return v / from_milliliter<value_type>( 1 );
}

inline constexpr double to_liter( const volume &v )
{
    return v.value() / 1000.0;
}

// Legacy conversions factor for old volume values.
// Don't use in new code! Use one of the from_* functions instead.
constexpr volume legacy_volume_factor = from_milliliter( 250 );

const mass mass_min = units::mass( std::numeric_limits<units::mass::value_type>::min(),
                                   units::mass::unit_type{} );

const mass mass_max = units::mass( std::numeric_limits<units::mass::value_type>::max(),
                                   units::mass::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, mass_in_milligram_tag> from_milligram(
    const value_type v )
{
    return quantity<value_type, mass_in_milligram_tag>( v, mass_in_milligram_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, mass_in_milligram_tag> from_gram(
    const value_type v )
{
    return from_milligram( v * 1000 );
}

template<typename value_type>
inline constexpr quantity<value_type, mass_in_milligram_tag> from_kilogram(
    const value_type v )
{
    return from_gram( v * 1000 );
}

template<typename value_type>
inline constexpr value_type to_milligram( const quantity<value_type, mass_in_milligram_tag> &v )
{
    return v.value();
}

template<typename value_type>
inline constexpr value_type to_gram( const quantity<value_type, mass_in_milligram_tag> &v )
{
    return v.value() / 1000.0;
}

inline constexpr double to_kilogram( const mass &v )
{
    return v.value() / 1000000.0;
}

// Specific energy
const specific_energy specific_energy_min = units::specific_energy(
            std::numeric_limits<units::specific_energy::value_type>::min(), units::specific_energy::unit_type{} );

const specific_energy specific_energy_max = units::specific_energy(
            std::numeric_limits<units::specific_energy::value_type>::max(), units::specific_energy::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, specific_energy_in_joule_per_gram_tag>
from_joule_per_gram( const value_type v )
{
    return quantity<value_type, specific_energy_in_joule_per_gram_tag>( v,
            specific_energy_in_joule_per_gram_tag{} );
}

template<typename value_type>
inline constexpr value_type to_joule_per_gram( const
        quantity<value_type, specific_energy_in_joule_per_gram_tag> &v )
{
    return v.value();
}

// Temperature
// Absolute zero - possibly should just be INT_MIN
const temperature temperature_min = units::temperature( 0, units::temperature::unit_type{} );

const temperature temperature_max = units::temperature(
                                        std::numeric_limits<units::temperature::value_type>::max(), units::temperature::unit_type{} );

template<typename value_type>
inline constexpr quantity<units::temperature::value_type, temperature_in_kelvin_tag> from_kelvin(
    const value_type v )
{
    return quantity<value_type, temperature_in_kelvin_tag>( v, temperature_in_kelvin_tag{} );
}

template<typename value_type>
inline constexpr value_type to_kelvin( const
                                       quantity<value_type, temperature_in_kelvin_tag> &v )
{
    return v.value();
}

constexpr temperature freezing_point = from_kelvin( 273.150f );

template<typename value_type>
inline constexpr quantity<units::temperature::value_type, temperature_in_kelvin_tag> from_celsius(
    const value_type v )
{
    return from_kelvin( v ) + freezing_point;
}

template<typename value_type>
inline constexpr quantity<units::temperature::value_type, temperature_in_kelvin_tag>
from_fahrenheit(
    const value_type v )
{
    return from_kelvin( ( v + 459.67f ) / 1.8f );
}

template<typename value_type>
inline constexpr value_type to_celsius( const
                                        quantity<value_type, temperature_in_kelvin_tag> &v )
{
    return ( v - freezing_point ).value();
}

template<typename value_type>
inline constexpr value_type to_fahrenheit( const
        quantity<value_type, temperature_in_kelvin_tag> &v )
{
    return ( v * 1.8f - from_kelvin( 459.67f ) ).value();
}

// Energy

const energy energy_min = units::energy( std::numeric_limits<units::energy::value_type>::min(),
                          units::energy::unit_type{} );

const energy energy_max = units::energy( std::numeric_limits<units::energy::value_type>::max(),
                          units::energy::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, energy_in_millijoule_tag> from_millijoule(
    const value_type v )
{
    return quantity<value_type, energy_in_millijoule_tag>( v, energy_in_millijoule_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, energy_in_millijoule_tag> from_joule( const value_type v )
{
    const value_type max_energy_joules = std::numeric_limits<value_type>::max() / 1000;
    // Check for overflow - if the energy provided is greater than max energy, then it
    // if overflow when converted to millijoules
    const value_type energy = v > max_energy_joules ? max_energy_joules : v;
    return from_millijoule<value_type>( energy * 1000 );
}

template<typename value_type>
inline constexpr quantity<value_type, energy_in_millijoule_tag> from_kilojoule( const value_type v )
{
    const value_type max_energy_joules = std::numeric_limits<value_type>::max() / 1000;
    // This checks for value_type overflow - if the energy we are given in Joules is greater
    // than the max energy in Joules, overflow will occur when it is converted to millijoules
    // The value we are given is in kJ, multiply by 1000 to convert it to joules, for use in from_joule
    value_type energy = v * 1000 > max_energy_joules ? max_energy_joules : v * 1000;
    return from_joule<value_type>( energy );
}

template<typename value_type>
inline constexpr value_type to_millijoule( const quantity<value_type, energy_in_millijoule_tag> &v )
{
    return v / from_millijoule<value_type>( 1 );
}

template<typename value_type>
inline constexpr value_type to_joule( const quantity<value_type, energy_in_millijoule_tag> &v )
{
    return to_millijoule( v ) / 1000.0;
}

template<typename value_type>
inline constexpr value_type to_kilojoule( const quantity<value_type, energy_in_millijoule_tag> &v )
{
    return to_joule( v ) / 1000.0;
}

// Power (watts)

const power power_min = units::power( std::numeric_limits<units::power::value_type>::min(),
                                      units::power::unit_type{} );

const power power_max = units::power( std::numeric_limits<units::power::value_type>::max(),
                                      units::power::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, power_in_milliwatt_tag> from_milliwatt(
    const value_type v )
{
    return quantity<value_type, power_in_milliwatt_tag>( v, power_in_milliwatt_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, power_in_milliwatt_tag> from_watt( const value_type v )
{
    constexpr value_type max_power_watts = std::numeric_limits<value_type>::max() / 1000;
    // Check for overflow - if the power provided is greater than max power, then it
    // will overflow when converted to milliwatt
    const value_type power = v > max_power_watts ? max_power_watts : v;
    return from_milliwatt<value_type>( power * 1000 );
}

template<typename value_type>
inline constexpr quantity<value_type, power_in_milliwatt_tag> from_kilowatt( const value_type v )
{
    constexpr value_type max_power_watts = std::numeric_limits<value_type>::max() / 1000;
    // This checks for value_type overflow - if the power we are given in watts is greater
    // than the max power in watts, overflow will occur when it is converted to milliwatts
    // The value we are given is in kilowatts, multiply by 1000 to convert it to watts, for use in from_watt
    value_type power = v * 1000 > max_power_watts ? max_power_watts : v * 1000;
    return from_watt<value_type>( power );
}

template<typename value_type>
inline constexpr value_type to_milliwatt( const quantity<value_type, power_in_milliwatt_tag> &v )
{
    return v / from_milliwatt<value_type>( 1 );
}

template<typename value_type>
inline constexpr value_type to_watt( const quantity<value_type, power_in_milliwatt_tag> &v )
{
    return to_milliwatt( v ) / 1000.0;
}

template<typename value_type>
inline constexpr value_type to_kilowatt( const quantity<value_type, power_in_milliwatt_tag> &v )
{
    return to_watt( v ) / 1000.0;
}

// Money(cents)

const money money_min = units::money( std::numeric_limits<units::money::value_type>::min(),
                                      units::money::unit_type{} );

const money money_max = units::money( std::numeric_limits<units::money::value_type>::max(),
                                      units::money::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, money_in_cent_tag> from_cent(
    const value_type v )
{
    return quantity<value_type, money_in_cent_tag>( v, money_in_cent_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, money_in_cent_tag> from_usd( const value_type v )
{
    return from_cent<value_type>( v * 100 );
}

template<typename value_type>
inline constexpr quantity<value_type, money_in_cent_tag> from_kusd( const value_type v )
{
    return from_usd<value_type>( v * 1000 );
}

template<typename value_type>
inline constexpr value_type to_cent( const quantity<value_type, money_in_cent_tag> &v )
{
    return v / from_cent<value_type>( 1 );
}

template<typename value_type>
inline constexpr value_type to_usd( const quantity<value_type, money_in_cent_tag> &v )
{
    return to_cent( v ) / 100.0;
}

template<typename value_type>
inline constexpr value_type to_kusd( const quantity<value_type, money_in_cent_tag> &v )
{
    return to_usd( v ) / 1000.0;
}

const length length_min = units::length( std::numeric_limits<units::length::value_type>::min(),
                          units::length::unit_type{} );

const length length_max = units::length( std::numeric_limits<units::length::value_type>::max(),
                          units::length::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, length_in_millimeter_tag> from_millimeter(
    const value_type v )
{
    return quantity<value_type, length_in_millimeter_tag>( v, length_in_millimeter_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, length_in_millimeter_tag> from_centimeter(
    const value_type v )
{
    return from_millimeter<value_type>( v * 10 );
}

template<typename value_type>
inline constexpr quantity<value_type, length_in_millimeter_tag> from_meter(
    const value_type v )
{
    return from_millimeter<value_type>( v * 1000 );
}

template<typename value_type>
inline constexpr quantity<value_type, length_in_millimeter_tag> from_kilometer(
    const value_type v )
{
    return from_millimeter<value_type>( v * 1'000'000 );
}

template<typename value_type>
inline constexpr value_type to_millimeter( const quantity<value_type, length_in_millimeter_tag> &v )
{
    return v / from_millimeter<value_type>( 1 );
}

template<typename value_type>
inline constexpr value_type to_centimeter( const quantity<value_type, length_in_millimeter_tag> &v )
{
    return to_millimeter( v ) / 10.0;
}

template<typename value_type>
inline constexpr value_type to_meter( const quantity<value_type, length_in_millimeter_tag> &v )
{
    return to_millimeter( v ) / 1'000.0;
}

template<typename value_type>
inline constexpr value_type to_kilometer( const quantity<value_type, length_in_millimeter_tag> &v )
{
    return to_millimeter( v ) / 1'000'000.0;
}

template<typename value_type>
inline constexpr quantity<value_type, angle_in_radians_tag> from_radians( const value_type v )
{
    return quantity<value_type, angle_in_radians_tag>( v, angle_in_radians_tag{} );
}

inline constexpr double to_radians( const units::angle v )
{
    return v.value();
}

template<typename value_type>
inline constexpr quantity<double, angle_in_radians_tag> from_degrees( const value_type v )
{
    return from_radians( v * M_PI / 180 );
}

inline constexpr double to_degrees( const units::angle v )
{
    return to_radians( v ) * 180 / M_PI;
}

template<typename value_type>
inline constexpr quantity<double, angle_in_radians_tag> from_arcmin( const value_type v )
{
    return from_degrees( v / 60.0 );
}

inline constexpr double to_arcmin( const units::angle v )
{
    return to_degrees( v ) * 60;
}

// converts a volume as if it were a cube to the length of one side
template<typename value_type>
inline constexpr quantity<value_type, length_in_millimeter_tag> default_length_from_volume(
    const quantity<value_type, volume_in_milliliter_tag> &v )
{
    return units::from_centimeter<int>(
               std::round(
                   std::cbrt( units::to_milliliter( v ) ) ) );
}

// Streaming operators for debugging and tests
// (for UI output other functions should be used which render in the user's
// chosen units)
inline std::ostream &operator<<( std::ostream &o, mass_in_milligram_tag )
{
    return o << "mg";
}

inline std::ostream &operator<<( std::ostream &o, volume_in_milliliter_tag )
{
    return o << "ml";
}

inline std::ostream &operator<<( std::ostream &o, energy_in_millijoule_tag )
{
    return o << "mJ";
}

inline std::ostream &operator<<( std::ostream &o, power_in_milliwatt_tag )
{
    return o << "mW";
}

inline std::ostream &operator<<( std::ostream &o, money_in_cent_tag )
{
    return o << "cent";
}

inline std::ostream &operator<<( std::ostream &o, length_in_millimeter_tag )
{
    return o << "mm";
}

inline std::ostream &operator<<( std::ostream &o, angle_in_radians_tag )
{
    return o << "rad";
}

template<typename value_type, typename tag_type>
inline std::ostream &operator<<( std::ostream &o, const quantity<value_type, tag_type> &v )
{
    return o << v.value() << tag_type{};
}

template<typename value_type, typename tag_type>
inline std::string quantity_to_string( const quantity<value_type, tag_type> &v )
{
    std::ostringstream os;
    os << v;
    return os.str();
}

std::string display( const units::energy &v );

std::string display( units::power v );

} // namespace units

// Implicitly converted to volume, which has int as value_type!
inline constexpr units::volume operator"" _ml( const unsigned long long v )
{
    return units::from_milliliter( v );
}

inline constexpr units::quantity<double, units::volume_in_milliliter_tag> operator"" _ml(
    const long double v )
{
    return units::from_milliliter( v );
}

// Implicitly converted to volume, which has int as value_type!
inline constexpr units::volume operator"" _liter( const unsigned long long v )
{
    return units::from_milliliter( v * 1000 );
}

inline constexpr units::quantity<double, units::volume_in_milliliter_tag> operator"" _liter(
    const long double v )
{
    return units::from_milliliter( v * 1000 );
}

// Implicitly converted to mass, which has int as value_type!
inline constexpr units::mass operator"" _milligram( const unsigned long long v )
{
    return units::from_milligram( v );
}
inline constexpr units::mass operator"" _gram( const unsigned long long v )
{
    return units::from_gram( v );
}

inline constexpr units::mass operator"" _kilogram( const unsigned long long v )
{
    return units::from_kilogram( v );
}

inline constexpr units::quantity<double, units::mass_in_milligram_tag> operator"" _milligram(
    const long double v )
{
    return units::from_milligram( v );
}

inline constexpr units::quantity<double, units::mass_in_milligram_tag> operator"" _gram(
    const long double v )
{
    return units::from_gram( v );
}

inline constexpr units::quantity<double, units::mass_in_milligram_tag> operator"" _kilogram(
    const long double v )
{
    return units::from_kilogram( v );
}

inline constexpr units::temperature operator"" _K( const unsigned long long v )
{
    return units::from_kelvin( v );
}

inline constexpr units::quantity<double, units::temperature_in_kelvin_tag> operator"" _K(
    const long double v )
{
    return units::from_kelvin( v );
}

inline constexpr units::energy operator"" _mJ( const unsigned long long v )
{
    return units::from_millijoule( v );
}

inline constexpr units::quantity<double, units::energy_in_millijoule_tag> operator"" _mJ(
    const long double v )
{
    return units::from_millijoule( v );
}

inline constexpr units::energy operator"" _J( const unsigned long long v )
{
    return units::from_joule( v );
}

inline constexpr units::quantity<double, units::energy_in_millijoule_tag> operator"" _J(
    const long double v )
{
    return units::from_joule( v );
}

inline constexpr units::energy operator"" _kJ( const unsigned long long v )
{
    return units::from_kilojoule( v );
}

inline constexpr units::quantity<double, units::energy_in_millijoule_tag> operator"" _kJ(
    const long double v )
{
    return units::from_kilojoule( v );
}

inline constexpr units::power operator"" _mW( const unsigned long long v )
{
    return units::from_milliwatt( v );
}

inline constexpr units::power operator"" _W( const unsigned long long v )
{
    return units::from_watt( v );
}

inline constexpr units::power operator"" _kW( const unsigned long long v )
{
    return units::from_kilowatt( v );
}

inline constexpr units::money operator"" _cent( const unsigned long long v )
{
    return units::from_cent( v );
}

inline constexpr units::quantity<double, units::money_in_cent_tag> operator"" _cent(
    const long double v )
{
    return units::from_cent( v );
}

inline constexpr units::money operator"" _USD( const unsigned long long v )
{
    return units::from_usd( v );
}

inline constexpr units::quantity<double, units::money_in_cent_tag> operator"" _USD(
    const long double v )
{
    return units::from_usd( v );
}

inline constexpr units::money operator"" _kUSD( const unsigned long long v )
{
    return units::from_kusd( v );
}

inline constexpr units::quantity<double, units::money_in_cent_tag> operator"" _kUSD(
    const long double v )
{
    return units::from_kusd( v );
}

inline constexpr units::quantity<double, units::length_in_millimeter_tag> operator"" _mm(
    const long double v )
{
    return units::from_millimeter( v );
}

inline constexpr units::length operator"" _mm( const unsigned long long v )
{
    return units::from_millimeter( v );
}

inline constexpr units::quantity<double, units::length_in_millimeter_tag> operator"" _cm(
    const long double v )
{
    return units::from_centimeter( v );
}

inline constexpr units::length operator"" _cm( const unsigned long long v )
{
    return units::from_centimeter( v );
}

inline constexpr units::quantity<double, units::length_in_millimeter_tag> operator"" _meter(
    const long double v )
{
    return units::from_meter( v );
}

inline constexpr units::length operator"" _meter( const unsigned long long v )
{
    return units::from_meter( v );
}

inline constexpr units::quantity<double, units::length_in_millimeter_tag> operator"" _km(
    const long double v )
{
    return units::from_kilometer( v );
}

inline constexpr units::length operator"" _km( const unsigned long long v )
{
    return units::from_kilometer( v );
}

inline constexpr units::angle operator"" _radians( const long double v )
{
    return units::from_radians( v );
}

inline constexpr units::angle operator"" _radians( const unsigned long long v )
{
    return units::from_radians( v );
}

inline constexpr units::angle operator"" _pi_radians( const long double v )
{
    return units::from_radians( v * M_PI );
}

inline constexpr units::angle operator"" _pi_radians( const unsigned long long v )
{
    return units::from_radians( v * M_PI );
}

inline constexpr units::angle operator"" _degrees( const long double v )
{
    return units::from_degrees( v );
}

inline constexpr units::angle operator"" _degrees( const unsigned long long v )
{
    return units::from_degrees( v );
}

inline constexpr units::angle operator"" _arcmin( const long double v )
{
    return units::from_arcmin( v );
}

inline constexpr units::angle operator"" _arcmin( const unsigned long long v )
{
    return units::from_arcmin( v );
}

namespace units
{

inline double sin( angle a )
{
    return std::sin( to_radians( a ) );
}

inline double cos( angle a )
{
    return std::cos( to_radians( a ) );
}

inline double tan( angle a )
{
    return std::tan( to_radians( a ) );
}

inline double cot( angle a )
{
    return std::tan( M_PI_2 - to_radians( a ) );
}

inline units::angle atan2( double y, double x )
{
    return from_radians( std::atan2( y, x ) );
}

inline units::angle asin( double x )
{
    return from_radians( std::asin( x ) );
}

inline units::angle acos( double x )
{
    return from_radians( std::acos( x ) );
}

const std::vector<std::pair<std::string, energy>> energy_units = { {
        { "mJ", 1_mJ },
        { "J", 1_J },
        { "kJ", 1_kJ },
    }
};
const std::vector<std::pair<std::string, power>> power_units = { {
        { "mW", 1_mW },
        { "W", 1_W },
        { "kW", 1_kW },
    }
};
const std::vector<std::pair<std::string, mass>> mass_units = { {
        { "mg", 1_milligram },
        { "g", 1_gram },
        { "kg", 1_kilogram },
    }
};
const std::vector<std::pair<std::string, money>> money_units = { {
        { "cent", 1_cent },
        { "USD", 1_USD },
        { "kUSD", 1_kUSD },
    }
};
const std::vector<std::pair<std::string, volume>> volume_units = { {
        { "ml", 1_ml },
        { "L", 1_liter }
    }
};
const std::vector<std::pair<std::string, length>> length_units = { {
        { "mm", 1_mm },
        { "cm", 1_cm },
        { "meter", 1_meter },
        { "km", 1_km }
    }
};
const std::vector<std::pair<std::string, angle>> angle_units = { {
        { "arcmin", 1_arcmin },
        { "°", 1_degrees },
        { "rad", 1_radians },
    }
};
const std::vector<std::pair<std::string, temperature>> temperature_units = { {
        { "K", 1_K }
    }
};

units::energy operator*( const units::power &power, const time_duration &time );
units::energy operator*( const time_duration &time, const units::power &power );

units::power operator/( const units::energy &energy, const time_duration &time );
time_duration operator/( const units::energy &energy, const units::power &power );

} // namespace units

namespace detail
{

template<typename T, typename Error>
T read_from_json_string_common( const std::string_view s,
                                const std::vector<std::pair<std::string, T>> &units, Error &&error )
{
    size_t i = 0;
    // returns whether we are at the end of the string
    const auto skip_spaces = [&]() {
        while( i < s.size() && s[ i ] == ' ' ) {
            ++i;
        }
        return i >= s.size();
    };
    const auto get_unit = [&]() {
        if( skip_spaces() ) {
            error( "invalid quantity string: missing unit", i );
        }
        for( const std::pair<std::string, T> &pair : units ) {
            const std::string &unit = pair.first;
            if( s.size() >= unit.size() + i && s.compare( i, unit.size(), unit ) == 0 ) {
                i += unit.size();
                return pair.second;
            }
        }
        error( "invalid quantity string: unknown unit", i );
        // above always throws but lambdas cannot be marked [[noreturn]]
        throw std::string( "Exceptionally impossible" );
    };

    if( skip_spaces() ) {
        error( "invalid quantity string: empty string", i );
        // above always throws but lambdas cannot be marked [[noreturn]]
        throw std::string( "Exceptionally impossible" );
    }
    T result{};
    do {
        int sign_value = +1;
        if( s[ i ] == '-' ) {
            sign_value = -1;
            ++i;
        } else if( s[ i ] == '+' ) {
            ++i;
        }
        if( i >= s.size() || !isdigit( s[ i ] ) ) {
            error( "invalid quantity string: number expected", i );
            // above always throws but lambdas cannot be marked [[noreturn]]
            throw std::string( "Exceptionally impossible" );
        }
        int value = 0;
        for( ; i < s.size() && isdigit( s[ i ] ); ++i ) {
            value = value * 10 + ( s[ i ] - '0' );
        }
        result += sign_value * value * get_unit();
    } while( !skip_spaces() );
    return result;
}

} // namespace detail

template<typename T>
T read_from_json_string( const JsonValue &jv, const std::vector<std::pair<std::string, T>> &units )
{
    const auto error = [&]( const char *const msg, size_t offset ) {
        jv.throw_error( offset, msg );
    };

    const std::string s = jv;

    return detail::read_from_json_string_common<T>( s, units, error );
}

template<typename T>
void dump_to_json_string( T t, JsonOut &jsout,
                          const std::vector<std::pair<std::string, T>> &units )
{
    // deduplicate unit strings and choose the shortest representations
    std::map<T, std::string> sorted_units;
    for( const std::pair<std::string, T> &p : units ) {
        const auto it = sorted_units.find( p.second );
        if( it != sorted_units.end() ) {
            if( p.first.length() < it->second.length() ) {
                it->second = p.first;
            }
        } else {
            sorted_units.emplace( p.second, p.first );
        }
    }
    std::string str;
    bool written = false;
    for( auto it = sorted_units.rbegin(); it != sorted_units.rend(); ++it ) {
        const int val = static_cast<int>( t / it->first );
        if( val != 0 ) {
            if( written ) {
                str += ' ';
            }
            int tmp = val;
            if( tmp < 0 ) {
                str += '-';
                tmp = -tmp;
            }
            const size_t val_beg = str.size();
            while( tmp != 0 ) {
                str += static_cast<char>( '0' + tmp % 10 );
                tmp /= 10;
            }
            std::reverse( str.begin() + val_beg, str.end() );
            str += ' ';
            str += it->second;
            written = true;
            t -= it->first * val;
        }
    }
    if( str.empty() ) {
        str = "0 " + sorted_units.begin()->second;
    }
    jsout.write( str );
}

#endif // CATA_SRC_UNITS_H
