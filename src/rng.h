#pragma once
#ifndef CATA_SRC_RNG_H
#define CATA_SRC_RNG_H

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "units.h"

class map;
class time_duration;
template<typename Tripoint>
class tripoint_range;

// All PRNG functions use an engine, see the C++11 <random> header
// By default, that engine is seeded by time on first call to such a function.
// If this function is called with a non-zero seed then the engine will be
// seeded (or re-seeded) with the given seed.
void rng_set_engine_seed( unsigned int seed );

using cata_default_random_engine = std::minstd_rand0;
cata_default_random_engine::result_type rng_get_first_seed();
cata_default_random_engine &rng_get_engine();
unsigned int rng_bits();

int rng( int lo, int hi );
double rng_float( double lo, double hi );

template<typename U>
units::quantity<double, U> rng_float( units::quantity<double, U> lo,
                                      units::quantity<double, U> hi )
{
    return { rng_float( lo.value(), hi.value() ), U{} };
}

units::angle random_direction();

bool one_in( int chance );
bool one_turn_in( const time_duration &duration );
bool x_in_y( double x, double y );
int dice( int number, int sides );

// Returns x + x_in_y( x-int(x), 1 )
int roll_remainder( double value );
inline int roll_remainder( float value )
{
    return roll_remainder( static_cast<double>( value ) );
}

int djb2_hash( const unsigned char *input );

// Generates a deterministic sequence of uniform ints.
// Note that this doesn't use or modify the global rng state but uses the seed given as parameter.
// @param count length of sequence to generate
// @param lo minimum value in sequence
// @param hi maximum value in sequence
// @param seed seed to use
// @returns deterministic vector of uniform ints
std::vector<int> rng_sequence( size_t count, int lo, int hi, int seed = 42 );

double rng_normal( double lo, double hi );

inline double rng_normal( double hi )
{
    return rng_normal( 0.0, hi );
}

float normal_roll_chance( float center, float stddev, float difficulty );

double normal_roll( double mean, double stddev );

double chi_squared_roll( double trial_num );

double rng_exponential( double min, double mean );

inline double rng_exponential( double mean )
{
    return rng_exponential( 0.0, mean );
}

double exponential_roll( double lambda );

// Return a random string of [A-Za-z] characters of the given length
std::string random_string( size_t length );

/**
 * Returns a random entry in the container.
 * The container must have a `size()` function and must support iterators as usual.
 * For empty containers it returns the given default object.
 * `C` is the container type,
 * `D` is the type of the default value (which may differ from the return type)
 * `V` is the type of the elements in the container.
 * Note that this function does not return a reference because the default value could be
 * a temporary object that is not valid after this function has left:
 * \code random_entry( vect, std::string("default") ); \endcode
 */
template<typename C, typename D, typename V = typename C::value_type>
inline V random_entry( const C &container, D default_value )
{
    if( container.empty() ) {
        return default_value;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}
/**
 * Same as above but returns a reference to the chosen entry (if the container
 * is not empty) or an empty `optional` object (if the container is empty).
 * This function handles empty containers without requiring an instance of the
 * contained type when container is empty.
 */
template<typename C>
inline auto random_entry_opt( C &container ) ->
std::optional<decltype( std::ref( *container.begin() ) )>
{
    if( container.empty() ) {
        return std::nullopt;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return std::ref( *iter );
}
/**
 * Same as above, but returns a default constructed value if the container
 * is empty.
 */
template<typename C, typename V = typename C::value_type>
inline V random_entry( const C &container )
{
    if( container.empty() ) {
        return V();
    }
    typename C::const_iterator iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}

template<typename ...T>
class is_std_array_helper : public std::false_type
{
};
template<typename T, std::size_t N>
class is_std_array_helper<std::array<T, N>> : public std::true_type
{
};
template<typename T>
class is_std_array : public is_std_array_helper<std::decay_t<T>>
{
};

/**
 * Same as above, but with a statically allocated default value (using the default
 * constructor). This allows to return a reference, either into the given container
 * or to the default value.
 */
template<typename C, typename V = typename C::value_type>
inline std::enable_if_t < !is_std_array<C>::value,
       const V & > random_entry_ref( const C &container )
{
    if( container.empty() ) {
        static const V default_value = V();
        return default_value;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
}
template<typename V, std::size_t N>
inline const V &random_entry_ref( const std::array<V, N> &container )
{
    static_assert( N > 0, "Need a non-empty array to get a random value from it" );
    return container[rng( 0, N - 1 )];
}
/**
 * Returns a random entry in the container and removes it from the container.
 * The container must not be empty!
 */
template<typename C, typename V = typename C::value_type>
inline V random_entry_removed( C &container )
{
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    V result = std::move( *iter ); // Copy because the original is removed and thereby destroyed
    container.erase( iter );
    return result;
}

/// Returns a range enclosing all valid points of the map.
tripoint_range<tripoint_bub_ms> points_in_range_bub( const map &m );
// Restricts the points to the specified Z level.
tripoint_range<tripoint_bub_ms> points_in_level_range( const map &m, int z );
/// Returns a random point in the given range that satisfies the given predicate ( if any ).
std::optional<tripoint_bub_ms> random_point( const tripoint_range<tripoint_bub_ms> &range,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );
/// Same as other random_point with a range enclosing all valid points of the map.
std::optional<tripoint_bub_ms> random_point( const map &m,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );
std::optional<tripoint_bub_ms> random_point_on_level( const map &m, int z,
        const std::function<bool( const tripoint_bub_ms & )> &predicate );

#endif // CATA_SRC_RNG_H
