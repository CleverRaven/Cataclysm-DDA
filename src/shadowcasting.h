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

template< int xx, int xy, int yx, int yy, typename T,
          float( *calc )( const float &, const float &, const int & ),
          bool( *check )( const float &, const float & ) >
void castLight(
    T( &output_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const T( &input_array )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const int offsetX, const int offsetY, const int offsetDistance,
    const T numerator = 1.0, const int row = 1,
    float start = 1.0f, const float end = 0.0f,
    T cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

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
