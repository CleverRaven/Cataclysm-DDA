#pragma once
#ifndef LIGHTMAP_H
#define LIGHTMAP_H

#include <cmath>

#define LIGHT_SOURCE_LOCAL  0.1f
#define LIGHT_SOURCE_BRIGHT 10

// Just enough light that you can see the current and adjacent squares with normal vision.
#define LIGHT_AMBIENT_MINIMAL 3.7f
// The threshold between not being able to see anything and things appearing shadowy.
#define LIGHT_AMBIENT_LOW 3.5
// The lower threshold for seeing well enough to do detail work such as reading or crafting.
#define LIGHT_AMBIENT_DIM 5.0
// The threshold between things being shadowed and being brightly lit.
#define LIGHT_AMBIENT_LIT 10.0

#define LIGHT_TRANSPARENCY_SOLID 0
// Calculated to run out at 60 squares.
// Cumulative transparency should drop to 0.1 or lower over 60 squares,
// Bright sunlight should drop to LIGHT_AMBIENT_LOW over 60 squares.
#define LIGHT_TRANSPARENCY_OPEN_AIR 0.038376418216
#define LIGHT_TRANSPARENCY_CLEAR 1

#define LIGHT_RANGE(b) static_cast<int>( -log(LIGHT_AMBIENT_LOW / static_cast<float>(b)) * (1.0 / LIGHT_TRANSPARENCY_OPEN_AIR) )

enum lit_level {
    LL_DARK = 0,
    LL_LOW, // Hard to see
    LL_BRIGHT_ONLY, // bright but indistinct
    LL_LIT,
    LL_BRIGHT, // Probably only for light sources
    LL_MEMORIZED, // Not a light level but behaves similarly
    LL_BLANK // blank space, not an actual light level
};

#endif
