#pragma once
#ifndef SHADOWCASTING_H
#define SHADOWCASTING_H

#include "enums.h"
#include "game_constants.h"

inline float fast_invexp( const float in )
{
    /* Use a table of f(x) = e^(-x).
     * x takes steps of 1/4 starts at 0 and ends on 4.
     * Between table entries, precision is improved by making linear estimates using the
     * derivative (-e^(-x)).
     * The estimate can made continuous by rounding at the intersection of the lines of the each
     * table entry and preceeding next entry.
     * This rounding point can be found by solving for r (rounding) in
     *      (1 - r)*e^(-x) = (1 + r)*e^(-x-s)
     * where s is the step size.
     * The solution is
     *      r = (1 - e^(-s))/(1 + e^(s)) or r = tanh(.5*s)
     */
    constexpr float STEP_SIZE = 1 / 4.0f;
    const float ROUNDING = STEP_SIZE - tanh( .5 * STEP_SIZE );
    static constexpr float table[17] = {
        1.000000, 0.778801, 0.606531, 0.472367,
        0.367879, 0.286505, 0.223130, 0.173774,
        0.135335, 0.105399, 0.082085, 0.063928,
        0.049787, 0.038774, 0.030197, 0.023518,
        0.018316
    };
    // Check whether the input is in the bounds of the table.
    if( in >= 0.0f && in <= 4.0f ) {
        // Add a constant for rounding then multiply the inverse of the step size and floor.
        float index = floor( ( 1 / STEP_SIZE ) * ( in + ROUNDING ) );
        // Find the difference between index and the actual input.
        float off = in - STEP_SIZE * index;
        // Load an initial estimate from the table.
        float est = table[( int )index];
        // Estimate further by subtracting the derivate.
        est = est - off * est;
        return est;
    }
    return ( float )exp( -in );
}

// Hoisted to header and inlined so the test in tests/shadowcasting_test.cpp can use it.
// Beer-Lambert law says attenuation is going to be equal to
// 1 / (e^al) where a = coefficient of absorption and l = length.
// Factoring out length, we get 1 / (e^((a1*a2*a3*...*an)*l))
// We merge all of the absorption values by taking their cumulative average.
inline float sight_calc( const float &numerator, const float &transparency, const int &distance )
{
    return numerator * ( float )fast_invexp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float &/*intensity*/ )
{
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}


template<int xx, int xy, int yx, int yy,
         float( *calc )( const float &, const float &, const int & ),
         bool( *check )( const float &, const float & )>
void castLight(
    float ( &output_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const float ( &input_array )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const int offsetX, const int offsetY, const int offsetDistance,
    const float numerator = 1.0, const int row = 1,
    float start = 1.0f, const float end = 0.0f,
    double cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

// TODO: Generalize the floor check, allow semi-transparent floors
template<int xx, int xy, int xz, int yx, int yy, int yz, int zz,
         float( *calc )( const float &, const float &, const int & ),
         bool( *check )( const float &, const float & )>
void cast_zlight(
    const std::array<float ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &output_caches,
    const std::array<const float ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const float numerator = 1.0f, const int row = 1,
    float start_major = 0.0f, const float end_major = 1.0f,
    float start_minor = 0.0f, const float end_minor = 1.0f,
    double cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

#endif
