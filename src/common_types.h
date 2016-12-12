#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include "json.h"

#include <limits>

/**
 * An interval of numeric values between @var min and @var max (including both).
 * By default it's [0, 0].
 */
template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct numeric_interval : public JsonDeserializer {
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

    using JsonDeserializer::deserialize;
    void deserialize( JsonIn &jsin ) override {
        JsonArray ja = jsin.get_array();
        if( ja.size() != 2 ) {
            ja.throw_error( "Intervals should be in format [min, max]." );
        }
        ja.read_next( min );
        ja.read_next( max );
        if( max < min ) {
            max = std::numeric_limits<T>::max();
        }
    }
};

#endif // COMMON_TYPES_Hs
