#pragma once
#ifndef UNITS_H
#define UNITS_H

#include <cstddef>
#include <limits>
#include <ostream>
#include <utility>

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
         * Constructor for initializing from literal 0, but not from any other literal
         * number. It allows `quantity<foo> x = 0;` but does forbid `quantity<foo> x = 1;`.
         * The former does not require an explicit unit as 0 is always 0 regardless of the unit.
         */
        constexpr quantity( std::nullptr_t ) : value_() {
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

class mass_in_gram_tag
{
};

using mass = quantity<int, mass_in_gram_tag>;

const mass mass_min = units::mass( std::numeric_limits<units::mass::value_type>::min(),
                                   units::mass::unit_type{} );

const mass mass_max = units::mass( std::numeric_limits<units::mass::value_type>::max(),
                                   units::mass::unit_type{} );

template<typename value_type>
inline constexpr quantity<value_type, mass_in_gram_tag> from_gram(
    const value_type v )
{
    return quantity<value_type, mass_in_gram_tag>( v, mass_in_gram_tag{} );
}

template<typename value_type>
inline constexpr quantity<value_type, mass_in_gram_tag> from_kilogram(
    const value_type v )
{
    return from_gram( v * 1000 );
}

template<typename value_type>
inline constexpr value_type to_gram( const quantity<value_type, mass_in_gram_tag> &v )
{
    return v / from_gram<value_type>( 1 );
}

inline constexpr double to_kilogram( const mass &v )
{
    return v.value() / 1000.0;
}

// Streaming operators for debugging and tests
// (for UI output other functions should be used which render in the user's
// chosen units)
inline std::ostream &operator<<( std::ostream &o, mass_in_gram_tag )
{
    return o << "g";
}

inline std::ostream &operator<<( std::ostream &o, volume_in_milliliter_tag )
{
    return o << "ml";
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
inline constexpr units::mass operator"" _gram( const unsigned long long v )
{
    return units::from_gram( v );
}

inline constexpr units::mass operator"" _kilogram( const unsigned long long v )
{
    return units::from_kilogram( v );
}

inline constexpr units::quantity<double, units::mass_in_gram_tag> operator"" _gram(
    const long double v )
{
    return units::from_gram( v );
}

inline constexpr units::quantity<double, units::mass_in_gram_tag> operator"" _kilogram(
    const long double v )
{
    return units::from_kilogram( v );
}

#endif
