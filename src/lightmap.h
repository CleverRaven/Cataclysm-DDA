#pragma once
#ifndef CATA_SRC_LIGHTMAP_H
#define CATA_SRC_LIGHTMAP_H

#include <cmath> // IWYU pragma: keep
#include <ostream>

constexpr float LIGHT_SOURCE_LOCAL = 0.1f;
constexpr float LIGHT_SOURCE_BRIGHT = 10.0f;

// Just enough light that you can see the current and adjacent squares with normal vision.
constexpr float LIGHT_AMBIENT_MINIMAL = 3.7f;
// The threshold between not being able to see anything and things appearing shadowy.
constexpr float LIGHT_AMBIENT_LOW = 3.5f;
// The lower threshold for seeing well enough to do detail work such as reading or crafting.
constexpr float LIGHT_AMBIENT_DIM = 5.0f;
// The threshold between things being shadowed and being brightly lit.
constexpr float LIGHT_AMBIENT_LIT = 10.0f;

/**
 * Transparency 101:
 * Transparency usually ranges between 0.038 (open air) and 0.38 (regular smoke).
 * The bigger the value, the more opaque it is (less light goes through).
 * To make sense of the specific value:
 * For transparency t, vision becomes "obstructed" (see map::apparent_light_helper) after
 *   ≈ (2.3 / t) consequent tiles of transparency `t`  ( derived from 1 / e^(dist * t) = 0.1 )
 *   e.g. for smoke with effective transparency 0.38  it's 2.3/0.38 ≈ 6 tiles
 *  for clean air with t=0.038  dist = 2.3/0.038 ≈ 60 tiles
 *
 * Note:  LIGHT_TRANSPARENCY_SOLID=0 is a special case (it indicates completely opaque tile)
 * */
constexpr float LIGHT_TRANSPARENCY_SOLID = 0.0f;
// Calculated to run out at 60 squares.
// Cumulative transparency should drop to 0.1 or lower over 60 squares,
// Bright sunlight should drop to LIGHT_AMBIENT_LOW over 60 squares.
constexpr float LIGHT_TRANSPARENCY_OPEN_AIR = 0.038376418216f;

// indicates starting (full) visibility (for seen_cache)
constexpr float VISIBILITY_FULL = 1.0f;

constexpr inline int LIGHT_RANGE( float b )
{
    if( b <= LIGHT_AMBIENT_LOW ) {
        return 0;
    }
    return static_cast<int>( -std::log( LIGHT_AMBIENT_LOW / b ) * ( 1.0 /
                             LIGHT_TRANSPARENCY_OPEN_AIR ) );
}

enum class lit_level : int {
    DARK = 0,
    LOW, // Hard to see
    BRIGHT_ONLY, // bright but indistinct
    LIT,
    BRIGHT, // Probably only for light sources
    MEMORIZED, // Not a light level but behaves similarly
    BLANK // blank space, not an actual light level
};

template<typename T>
constexpr inline bool operator>( const T &lhs, const lit_level &rhs )
{
    return lhs > static_cast<T>( rhs );
}

template<typename T>
constexpr inline bool operator<=( const T &lhs, const lit_level &rhs )
{
    return !operator>( lhs, rhs );
}

template<typename T>
constexpr inline bool operator!=( const lit_level &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) != rhs;
}

inline std::ostream &operator<<( std::ostream &os, const lit_level &ll )
{
    return os << static_cast<int>( ll );
}

#endif // CATA_SRC_LIGHTMAP_H
