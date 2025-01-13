#pragma once
#ifndef CATA_SRC_COMMON_TYPES_H
#define CATA_SRC_COMMON_TYPES_H

#include <limits>
#include <type_traits>

#include "json.h"

/**
 * An interval of numeric values between @ref min and @ref max (including both).
 * By default it's [0, 0].
 */
template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>, T>>
struct numeric_interval {
    T min = static_cast<T>( 0 );
    T max = static_cast<T>( 0 );

    numeric_interval() = default;
    numeric_interval( T min, T max ) : min( min ), max( max ) { }
    numeric_interval( T middle, T lower_margin, T upper_margin ) :
        min( middle - lower_margin ), max( middle + upper_margin ) {
    }

    bool operator==( const numeric_interval<T> &rhs ) const {
        return min == rhs.min && max == rhs.max;
    }

    bool contains( T val ) const {
        return val >= min && val <= max;
    }

    bool empty() const {
        return max == 0;
    }

    // TODO: break deserialize out into its own thing so that
    // common_types.h isn't dragging json.h into things unnecessarily
    void deserialize( const JsonValue &jsin ) {
        JsonArray ja = jsin.get_array();
        if( ja.size() != 2 ) {
            ja.throw_error( "Intervals should be in format [min, max]." );
        }
        ja.read_next( min );
        ja.read_next( max );
        if( max < min ) {
            if( max >= 0 ) {
                ja.throw_error( "Intervals should be in format [min, max]." );
            }
            max = std::numeric_limits<T>::max();
        }
    }
};

#endif // CATA_SRC_COMMON_TYPES_H
