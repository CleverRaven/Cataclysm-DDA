#ifndef RNG_H
#define RNG_H

#include "compatibility.h"

#include <functional>

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
 * Same as above, but returns a default constructed value if the container
 * is empty. This allows to return a reference, either into the given container or to the
 * (statically allocated and therefor always valid) default value.
 */
template<typename C, typename V = typename C::value_type>
inline const V & random_entry( const C &container )
{
    if( container.empty() ) {
        static const V default_value = V();
        return default_value;
    }
    auto iter = container.begin();
    std::advance( iter, rng( 0, container.size() - 1 ) );
    return *iter;
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

#endif
