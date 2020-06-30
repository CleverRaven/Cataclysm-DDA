#pragma once
#ifndef CATA_SRC_LIGHTMAP_H
#define CATA_SRC_LIGHTMAP_H

#include <cmath>
#include <ostream>

static constexpr float LIGHT_SOURCE_LOCAL = 0.1f;
static constexpr float LIGHT_SOURCE_BRIGHT = 10.0f;

// Just enough light that you can see the current and adjacent squares with normal vision.
static constexpr float LIGHT_AMBIENT_MINIMAL = 3.7f;
// The threshold between not being able to see anything and things appearing shadowy.
static constexpr float LIGHT_AMBIENT_LOW = 3.5f;
// The lower threshold for seeing well enough to do detail work such as reading or crafting.
static constexpr float LIGHT_AMBIENT_DIM = 5.0f;
// The threshold between things being shadowed and being brightly lit.
static constexpr float LIGHT_AMBIENT_LIT = 10.0f;

static constexpr float LIGHT_TRANSPARENCY_SOLID = 0.0f;
// Calculated to run out at 60 squares.
// Cumulative transparency should drop to 0.1 or lower over 60 squares,
// Bright sunlight should drop to LIGHT_AMBIENT_LOW over 60 squares.
static constexpr float LIGHT_TRANSPARENCY_OPEN_AIR = 0.038376418216f;
static constexpr float LIGHT_TRANSPARENCY_CLEAR = 1.0f;

#define LIGHT_RANGE(b) static_cast<int>( -std::log(LIGHT_AMBIENT_LOW / static_cast<float>(b)) * (1.0 / LIGHT_TRANSPARENCY_OPEN_AIR) )

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
