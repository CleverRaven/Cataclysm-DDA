#pragma once
#ifndef CATA_SRC_LIGHT_H
#define CATA_SRC_LIGHT_H

#include <cmath>
#include <ostream>


// More compact representation
enum class lit_level : int {
    DARK = 0,
    LOW, // Hard to see
    BRIGHT_ONLY, // bright but indistinct
    LIT,
    BRIGHT, // Probably only for light sources
    MEMORIZED, // Not a light level but behaves similarly
    BLANK // blank space, not an actual light level
};

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
static constexpr float LIGHT_TRANSPARENCY_SOLID = 0.0f;
// Calculated to run out at 60 squares.
// Cumulative transparency should drop to 0.1 or lower over 60 squares,
// Bright sunlight should drop to LIGHT_AMBIENT_LOW over 60 squares.
static constexpr float LIGHT_TRANSPARENCY_OPEN_AIR = 0.038376418216f;

// indicates starting (full) visibility (for seen_cache)
static constexpr float VISIBILITY_FULL = 1.0f;

class light
{
    public:
        float value;
        light() : value( 0.0f ) {};
        explicit light( float value ) : value( value ) {};

        // Sight range at a given transparency
        // default threshold corresponds to AMBIENT_LIGHT_LOW defined below
        int sight_range(light threshold = light( 3.5f ), float transparency = LIGHT_TRANSPARENCY_OPEN_AIR) const;

        light operator+( const light &other ) const {
            return light( value + other.value );
        }
        void operator+=( const light &other ) {
            value += other.value;
        }
        friend light operator*( const light &l, const float x ) {
            return light( l.value * x );
        }
        friend light operator*( const float x, const light &l ) {
            return light( l.value * x );
        }

        // Relative brightness as a fraction
        float operator/( const light &other ) const {
            return value / other.value;
        }

        bool operator<=( const light &other ) const {
            return value <= other.value;
        }
        bool operator>=( const light &other ) const {
            return value >= other.value;
        }
        bool operator==( const light &other ) const {
            return value == other.value;
        }
        bool operator<( const light &other ) const {
            return value < other.value;
        }
        bool operator>( const light &other ) const {
            return value > other.value;
        }
        bool operator<=( const lit_level other ) const {
            return value <= static_cast<float>( other );
        }
        bool operator>=( const lit_level other ) const {
            return value >= static_cast<float>( other );
        }
        bool operator<( const lit_level other ) const {
            return value < static_cast<float>( other );
        }
        bool operator>( const lit_level other ) const {
            return value > static_cast<float>( other );
        }

        friend std::ostream &operator<<( std::ostream &os, const light &light ) {
            return os << light.value;
        }
};

static light LIGHT_SOURCE_LOCAL = light( 0.1f );
static light LIGHT_SOURCE_BRIGHT = light( 10.0f );

// Absolute darkness. All lights are brighter than this threshold.
static light LIGHT_AMBIENT_PITCH_BLACK = light( 0.0f );
// The threshold between not being able to see anything and things appearing shadowy.
static light LIGHT_AMBIENT_LOW = light( 3.5f );
// Just enough light that you can see the current and adjacent squares with normal vision.
static light LIGHT_AMBIENT_MINIMAL = light( 3.7f );
// The lower threshold for seeing well enough to do detail work such as reading or crafting.
static light LIGHT_AMBIENT_DIM = light( 5.0f );
// The threshold between dim and cloudy.
// Also used by nyctophobia
static light LIGHT_AMBIENT_CLOUDY = light( 7.0f );
// The threshold between things being shadowed and being brightly lit.
static light LIGHT_AMBIENT_LIT = light( 10.0f );
// How much light is provided in full daylight
// TODO As previously defined by calendar.h. Check for consistency
static light LIGHT_DAY = light( 100.0f );

float sight_calc(const float &numerator, const float &transparency, const int &distance);

#endif // CATA_SRC_LIGHT_H
