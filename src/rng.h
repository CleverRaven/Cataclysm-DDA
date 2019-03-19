#pragma once
#ifndef RNG_H
#define RNG_H

#include <array>
#include <functional>
#include <random>

#include "optional.h"

// Some of the RNG functions are based on an engine.
// By default, that engine is seeded by time on first call to such a function.
// If this function is called with a non-zero seed then the engine will be
// seeded (or re-seeded) with the given seed.
void rng_set_engine_seed( uintmax_t seed );

std::default_random_engine &rng_get_engine();

long rng( long val1, long val2 );
double rng_float( double val1, double val2 );
bool one_in( int chance );
bool one_in_improved( double chance );
bool x_in_y( double x, double y );
int dice( int number, int sides );

// Returns x + x_in_y( x-int(x), 1 )
int roll_remainder( double value );
// Returns x/y + x_in_y( (x/y)-int(x/y), 1 )
int divide_roll_remainder( double dividend, double divisor );

int djb2_hash( const unsigned char *input );

double rng_normal( double lo, double hi );

inline double rng_normal( double hi )
{
    return rng_normal( 0.0, hi );
}

double normal_roll( double mean, double stddev );

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
template<typename C, typename V = const typename C::value_type>
inline cata::optional<std::reference_wrapper<V>> random_entry_opt( const C &container )
{
    if( container.empty() ) {
        return cata::nullopt;
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
    auto iter = container.begin();
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
class is_std_array : public is_std_array_helper<typename std::decay<T>::type>
{
};

/**
 * Same as above, but with a statically allocated default value (using the default
 * constructor). This allows to return a reference, either into the given container
 * or to the default value.
 */
template<typename C, typename V = typename C::value_type>
inline typename std::enable_if < !is_std_array<C>::value,
       const V & >::type random_entry_ref( const C &container )
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
    const V result = std::move( *iter ); // Copy because the original is removed and thereby destroyed
    container.erase( iter );
    return result;
}

namespace cata
{
template<typename T>
class optional;
} // namespace cata

class map;
struct tripoint;
class tripoint_range;

/// Returns a range enclosing all valid points of the map.
tripoint_range points_in_range( const map &m );
/// Returns a random point in the given range that satisfies the given predicate ( if any ).
cata::optional<tripoint> random_point( const tripoint_range &range,
                                       const std::function<bool( const tripoint & )> &predicate );
/// Same as other random_point with a range enclosing all valid points of the map.
cata::optional<tripoint> random_point( const map &m,
                                       const std::function<bool( const tripoint & )> &predicate );

#endif
