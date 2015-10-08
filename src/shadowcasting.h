#ifndef SHADOWCASTING_H
#define SHADOWCASTING_H

#include "enums.h"
#include "game_constants.h"

// Hoisted to header and inlined so the test in tests/shadowcasting_test.cpp can use it.
// Beer–Lambert law says attenuation is going to be equal to
// 1 / (e^al) where a = coefficient of absorption and l = length.
// Factoring out length, we get 1 / (e^((a1*a2*a3*...*an)*l))
// We merge all of the absorption values by taking their cumulative average.
inline float sight_calc( const float &numerator, const float &transparency, const int &distance ) {
    return numerator / (float)exp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float &/*intensity*/ ) {
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}


template<int xx, int xy, int yx, int yy,
         float(*calc)(const float &, const float &, const int &),
         bool(*check)(const float &, const float &)>
void castLight(
    float (&output_cache)[MAPSIZE*SEEX][MAPSIZE*SEEY],
    const float (&input_array)[MAPSIZE*SEEX][MAPSIZE*SEEY],
    const int offsetX, const int offsetY, const int offsetDistance,
    const float numerator = 1.0, const int row = 1,
    float start = 1.0f, const float end = 0.0f,
    double cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

// TODO: Generalize the floor check, allow semi-transparent floors
template<int xx, int xy, int xz, int yx, int yy, int yz, int zz,
         float(*calc)(const float &, const float &, const int &),
         bool(*check)(const float &, const float &)>
void cast_zlight(
    const std::array<float (*)[MAPSIZE*SEEX][MAPSIZE*SEEY], OVERMAP_LAYERS> &output_caches,
    const std::array<const float (*)[MAPSIZE*SEEX][MAPSIZE*SEEY], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool (*)[MAPSIZE*SEEX][MAPSIZE*SEEY], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const float numerator = 1.0f, const int row = 1,
    float start_major = 0.0f, const float end_major = 1.0f,
    float start_minor = 0.0f, const float end_minor = 1.0f,
    double cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

#endif
