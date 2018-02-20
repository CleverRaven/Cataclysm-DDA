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
    return ( ( distance - 1 ) * cumulative_transparency + current_transparency) / distance;
}

struct fragment_cloud
{
    fragment_cloud() : velocity( 0.0 ), density( 0.0 ) {}
    fragment_cloud( float initial_value ) : velocity( initial_value ), density( initial_value ) {}
    fragment_cloud( float initial_velocity, float initial_density )
        : velocity( initial_velocity ), density( initial_density ) {
    }
    fragment_cloud& operator=( const float &value );
    bool operator==( const fragment_cloud &that );
    float velocity;
    float density;
};

// This is only ever used to zero the cloud values, which is what makes it work.
inline fragment_cloud &fragment_cloud::operator=( const float &value )
{
    velocity = value;
    density = value;

    return *this;
}

inline bool fragment_cloud::operator==( const fragment_cloud &that )
{
    return velocity == that.velocity && density == that.density;
}

inline bool operator<( const fragment_cloud &us, const fragment_cloud &them )
{
    // TODO: think about this a bit more...
    return us.velocity < them.velocity && us.density < them.density;
}

inline int ballistic_damage( float velocity, float mass )
{
    // Damage is square root of Joules, dividing by 2000 because it's dividing by 2 and converting mass from grams to kg.
    return std::sqrt( ( velocity * velocity  * mass ) / 2000.0 );
}
// Projectile velocity in air.
// see https://fas.org/man/dod-101/navy/docs/es310/warheads/Warheads.htm for a writeup of this exact calculation.
inline fragment_cloud shrapnel_calc( const fragment_cloud &initial, const fragment_cloud &cloud, const int &distance )
{
    // Cross-sectionl area of fragment.
    constexpr float A = 0.0001;
    // Mass of fragment.
    constexpr float mass = 0.002;
    // SWAG coefficient of drag.
    // This is inflated to scale down the radius of blasts.
    constexpr float Cd = 2.0;
    fragment_cloud new_cloud;
    // Doubling distance to scale down effective fragment range.
    new_cloud.velocity = initial.velocity * exp( -cloud.velocity * ( ( Cd * A * distance ) / ( 2.0 * mass ) ) );
    // Two effects, the accumulated proportion of blocked fragments,
    // and the inverse-square dilution of fragments with distance.
    new_cloud.density = ( initial.density * cloud.density ) / ( distance * distance );
    return new_cloud;
}
// Minimum velocity resulting in skin perforation according to https://www.ncbi.nlm.nih.gov/pubmed/7304523
constexpr float MIN_EFFECTIVE_VELOCITY = 70.0;
// Pretty arbitrary minimum density.  1/1,000 change of a fragment passing through the given square.
constexpr float MIN_FRAGMENT_DENSITY = 0.0001;
inline bool shrapnel_check( const fragment_cloud &cloud, const fragment_cloud &intensity )
{
    return cloud.density > 0.0 && intensity.velocity > MIN_EFFECTIVE_VELOCITY &&
      intensity.density > MIN_FRAGMENT_DENSITY;
}

inline fragment_cloud accumulate_fragment_cloud( const fragment_cloud &cumulative_cloud,
                                                 const fragment_cloud &current_cloud, const int &distance )
{
    // Velocity is the cumulative and continuous decay of speed,
    // so it is accumulated the same way as light attentuation.
    // Density is the accumulation of discrete attenuaton events encountered in the traversed squares,
    // so each term is added to the series via multiplication.
    return fragment_cloud( ( ( distance - 1 ) * cumulative_cloud.velocity + current_cloud.velocity ) / distance,
                           cumulative_cloud.density * current_cloud.density );
}

template< int xx, int xy, int yx, int yy, typename T,
          float( *calc )( const float &, const float &, const int & ),
          bool( *check )( const float &, const float & ) >
void castLight(
    T ( &output_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const T ( &input_array )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const int offsetX, const int offsetY, const int offsetDistance,
    const T numerator = 1.0, const int row = 1,
    float start = 1.0f, const float end = 0.0f,
    T cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

// TODO: Generalize the floor check, allow semi-transparent floors
template< typename T, T( *calc )( const T &, const T &, const int & ),
  bool( *check )( const T &, const T & ),
  T( *accumulate )( const T &, const T &, const int & ) >
void cast_zlight(
    const std::array<T ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &output_caches,
    const std::array<const T ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance, const T numerator );

#endif
