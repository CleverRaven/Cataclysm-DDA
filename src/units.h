#pragma once
#ifndef UNITS_H
#define UNITS_H

#include <cstddef>
#include <limits>
#include <ostream>
#include <utility>

#include "calendar.h"
#include "json.h"

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
        constexpr quantity( const quantity<other_value_type, unit_type> &other ) : value_( other.value() ) {
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
         *   assert( a == quantity<int, foo>( 10 + 0.5, foo{} ) );
         *   assert( a == quantity<int, foo>( 10, foo{} ) );
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
        void deserialize( JsonIn &jsin );

    private:
        value_type value_;
};

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
 *   assert( a == quantity<int, foo>( 10 * 4.52, foo{} ) );
 *   assert( a != quantity<int, foo>( 10 * (int)4.52, foo{} ) );
 *   assert( a == quantity<int, foo>( 45, foo{} ) );
 * \endcode
 *
 * Division of a quantity with a quantity of the same unit yields a dimensionless
 * scalar value, with the same type as the division of the contained `value_type`s:
 * \code
 *   quantity<int, foo> a{ 10, foo{} };
 *   quantity<double, foo> b{ 20, foo{} };
 *   auto proportion = a / b;
 *   static_assert(std::is_same<decltype(proportion), double>::value);
 *   assert( proportion == 10 / 20.0 );
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

class volume_in_milliliter_tag
{
};

using volume = quantity<int, volume_in_milliliter_tag>;

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
static constexpr volume legacy_volume_factor = from_milliliter( 250 );

class mass_in_milligram_tag
{
};

using mass = quantity<std::int64_t, mass_in_milligram_tag>;

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
inline constexpr value_type to_gram( const quantity<value_type, mass_in_milligram_tag> &v )
{
    return v.value() / 1000.0;
}

inline constexpr double to_kilogram( const mass &v )
{
    return v.value() / 1000000.0;
}

class energy_in_millijoule_tag
{
};

using energy = quantity<std::int64_t, energy_in_millijoule_tag>;

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
    return from_millijoule<value_type>( v * 1000 );
}

template<typename value_type>
inline constexpr quantity<value_type, energy_in_millijoule_tag> from_kilojoule( const value_type v )
{
    return from_joule<value_type>( v * 1000 );
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

class money_in_cent_tag
{
};

using money = quantity<int, money_in_cent_tag>;

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

inline std::ostream &operator<<( std::ostream &o, money_in_cent_tag )
{
    return o << "cent";
}

template<typename value_type, typename tag_type>
inline std::ostream &operator<<( std::ostream &o, const quantity<value_type, tag_type> &v )
{
    return o << v.value() << tag_type{};
}

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

namespace units
{
static const std::vector<std::pair<std::string, energy>> energy_units = { {
        { "mJ", 1_mJ },
        { "J", 1_J },
        { "kJ", 1_kJ },
    }
};
static const std::vector<std::pair<std::string, mass>> mass_units = { {
        { "mg", 1_milligram },
        { "g", 1_gram },
        { "kg", 1_kilogram },
    }
};
static const std::vector<std::pair<std::string, money>> money_units = { {
        { "cent", 1_cent },
        { "USD", 1_USD },
        { "kUSD", 1_kUSD },
    }
};
} // namespace units

template<typename T>
T read_from_json_string( JsonIn &jsin, const std::vector<std::pair<std::string, T>> &units )
{
    const size_t pos = jsin.tell();
    size_t i = 0;
    const auto error = [&]( const char *const msg ) {
        jsin.seek( pos + i );
        jsin.error( msg );
    };

    const std::string s = jsin.get_string();
    // returns whether we are at the end of the string
    const auto skip_spaces = [&]() {
        while( i < s.size() && s[i] == ' ' ) {
            ++i;
        }
        return i >= s.size();
    };
    const auto get_unit = [&]() {
        if( skip_spaces() ) {
            error( "invalid quantity string: missing unit" );
        }
        for( const auto &pair : units ) {
            const std::string &unit = pair.first;
            if( s.size() >= unit.size() + i && s.compare( i, unit.size(), unit ) == 0 ) {
                i += unit.size();
                return pair.second;
            }
        }
        error( "invalid quantity string: unknown unit" );
        throw; // above always throws
    };

    if( skip_spaces() ) {
        error( "invalid quantity string: empty string" );
    }
    T result{};
    do {
        int sign_value = +1;
        if( s[i] == '-' ) {
            sign_value = -1;
            ++i;
        } else if( s[i] == '+' ) {
            ++i;
        }
        if( i >= s.size() || !isdigit( s[i] ) ) {
            error( "invalid quantity string: number expected" );
        }
        int value = 0;
        for( ; i < s.size() && isdigit( s[i] ); ++i ) {
            value = value * 10 + ( s[i] - '0' );
        }
        result += sign_value * value * get_unit();
    } while( !skip_spaces() );
    return result;
}

#endif
