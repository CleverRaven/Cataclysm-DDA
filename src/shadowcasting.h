#pragma once
#ifndef SHADOWCASTING_H
#define SHADOWCASTING_H

#include "enums.h"
#include "game_constants.h"

#include <cmath>

// Hoisted to header and inlined so the test in tests/shadowcasting_test.cpp can use it.
// Beer-Lambert law says attenuation is going to be equal to
// 1 / (e^al) where a = coefficient of absorption and l = length.
// Factoring out length, we get 1 / (e^((a1*a2*a3*...*an)*l))
// We merge all of the absorption values by taking their cumulative average.
inline float sight_calc( const float &numerator, const float &transparency, const int &distance )
{
    return numerator / ( float )exp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float &/*intensity*/ )
{
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}
inline float accumulate_transparency( const float &cumulative_transparency,
                                      const float &current_transparency, const int &distance )
{
    return ( ( distance - 1 ) * cumulative_transparency + current_transparency ) / distance;
}

template<typename T, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLightAll( T( &output_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
                   const T( &input_array )[MAPSIZE * SEEX][MAPSIZE * SEEY],
                   const int offsetX, const int offsetY, int offsetDistance = 0,
                   T numerator = 1.0 );

// TODO: Generalize the floor check, allow semi-transparent floors
template< typename T, T( *calc )( const T &, const T &, const int & ),
          bool( *check )( const T &, const T & ),
          T( *accumulate )( const T &, const T &, const int & ) >
void cast_zlight(
    const std::array<T( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &output_caches,
    const std::array<const T( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance, const T numerator );

#endif
