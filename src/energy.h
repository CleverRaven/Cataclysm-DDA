#ifndef ENERGY_H
#define ENERGY_H

#include <utility>

#include "rng.h"

class JsonIn;
class JsonOut;

class energy_quantity
{
    private:
        int millijoules;
        explicit constexpr energy_quantity( const int mJ ) : millijoules( mJ ) { }

    public:
        energy_quantity() = default;
        energy_quantity( std::nullptr_t ) : millijoules( 0 ) {}

        // Allows writing `energy_quantity e = 0;`
        static energy_quantity read_from_json_string( JsonIn &jsin );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        /**
         * Named constructors to get a quantity representing a multiple of the named time
         * units. Quantity is stored as an integer number of millijoules.
         * The template type is used for the conversion from given energy unit to millijoules, so
         * `from_joules( 0.5 )` will yield "500 millijoules".
         */
        /**@{*/
        template<typename T>
        static constexpr energy_quantity from_millijoules( const T mJ ) {
            return energy_quantity( mJ );
        }
        template<typename T>
        static constexpr energy_quantity from_joules( const T J ) {
            return from_millijoules( J * 1000 );
        }
        template<typename T>
        static constexpr energy_quantity from_kilojoules( const T kJ ) {
            return from_joules( kJ * 1000 );
        }
        /**@}*/

        /**
         * Converts the quantity to an amount of the given units. The conversions is
         * done with values of the given template type. That means using an integer
         * type (e.g. `int`) will return a truncated value (amount of *full* kJ
         * that make up the quantity, discarding the remainder).
         * Calling `to_kilojoules<double>` will return a precise number.
         * Example:
         * `to_kilojoules<int>( from_joules( 1500 ) ) == 1`
         * `to_kilojoules<double>( from_joules( 1500 ) ) == 1.5`
         */
        /**@{*/
        template<typename T>
        friend constexpr T to_millijoules( const energy_quantity &energy ) {
            return energy.millijoules;
        }
        template<typename T>
        friend constexpr T to_joules( const energy_quantity &energy ) {
            return static_cast<T>( energy.millijoules ) / static_cast<T>( 1000 );
        }
        template<typename T>
        friend constexpr T to_kilojoules( const energy_quantity &energy ) {
            return static_cast<T>( energy.millijoules ) / static_cast<T>( 1000 * 1000 );
        }
        /**@{*/

        constexpr bool operator<( const energy_quantity &rhs ) const {
            return millijoules < rhs.millijoules;
        }
        constexpr bool operator<=( const energy_quantity &rhs ) const {
            return millijoules <= rhs.millijoules;
        }
        constexpr bool operator>( const energy_quantity &rhs ) const {
            return millijoules > rhs.millijoules;
        }
        constexpr bool operator>=( const energy_quantity &rhs ) const {
            return millijoules >= rhs.millijoules;
        }
        constexpr bool operator==( const energy_quantity &rhs ) const {
            return millijoules == rhs.millijoules;
        }
        constexpr bool operator!=( const energy_quantity &rhs ) const {
            return millijoules != rhs.millijoules;
        }

        friend constexpr energy_quantity operator-( const energy_quantity &energy ) {
            return energy_quantity( -energy.millijoules );
        }
        friend constexpr energy_quantity operator+( const energy_quantity &lhs, const energy_quantity &rhs ) {
            return energy_quantity( lhs.millijoules + rhs.millijoules );
        }
        friend energy_quantity &operator+=( energy_quantity &lhs, const energy_quantity &rhs ) {
            return lhs = energy_quantity( lhs.millijoules + rhs.millijoules );
        }
        friend constexpr energy_quantity operator-( const energy_quantity &lhs, const energy_quantity &rhs ) {
            return energy_quantity( lhs.millijoules - rhs.millijoules );
        }
        friend energy_quantity &operator-=( energy_quantity &lhs, const energy_quantity &rhs ) {
            return lhs = energy_quantity( lhs.millijoules - rhs.millijoules );
        }
        // Using double here because it has the highest precision. Callers can cast it to whatever they want.
        friend double operator/( const energy_quantity &lhs, const energy_quantity &rhs ) {
            return static_cast<double>( lhs.millijoules ) / static_cast<double>( rhs.millijoules );
        }
        template<typename T>
        friend constexpr energy_quantity operator/( const energy_quantity &lhs, const T rhs ) {
            return energy_quantity( lhs.millijoules / rhs );
        }
        template<typename T>
        friend energy_quantity &operator/=( energy_quantity &lhs, const T rhs ) {
            return lhs = energy_quantity( lhs.millijoules / rhs );
        }
        template<typename T>
        friend constexpr energy_quantity operator*( const energy_quantity &lhs, const T rhs ) {
            return energy_quantity( lhs.millijoules * rhs );
        }
        template<typename T>
        friend constexpr energy_quantity operator*( const T lhs, const energy_quantity &rhs ) {
            return energy_quantity( lhs * rhs.millijoules );
        }
        template<typename T>
        friend energy_quantity &operator*=( energy_quantity &lhs, const T rhs ) {
            return lhs = energy_quantity( lhs.millijoules * rhs );
        }
        friend energy_quantity operator%( const energy_quantity &lhs, const energy_quantity &rhs ) {
            return energy_quantity( lhs.millijoules % rhs.millijoules );
        }

        friend energy_quantity rng( energy_quantity lo, energy_quantity hi );
};

energy_quantity rng( energy_quantity lo, energy_quantity hi )
{
    return energy_quantity( rng( lo.millijoules, hi.millijoules ) );
}

/**
 * Convert the given number into an energy quantity by calling the matching
 * `energy_quantity::from_*` function.
 */
/**@{*/
constexpr energy_quantity operator"" _millijoules( const unsigned long long int v )
{
    return energy_quantity::from_millijoules( v );
}
constexpr energy_quantity operator"" _joules( const unsigned long long int v )
{
    return energy_quantity::from_joules( v );
}
constexpr energy_quantity operator"" _kilojoules( const unsigned long long int v )
{
    return energy_quantity::from_kilojoules( v );
}
/**@}*/

#endif
