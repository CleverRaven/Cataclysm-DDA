#ifndef _LIGHTMAP_H_
#define _LIGHTMAP_H_

#define LIGHT_SOURCE_LOCAL  0.1f
#define LIGHT_SOURCE_BRIGHT 10

#define LIGHT_AMBIENT_LOW   1
#define LIGHT_AMBIENT_LIT   2

#define LIGHT_TRANSPARENCY_SOLID 0
#define LIGHT_TRANSPARENCY_CLEAR 1

#define LIGHT_RANGE(b) static_cast<int>(sqrt(b / LIGHT_AMBIENT_LOW) + 1)

enum lit_level {
    LL_DARK = 0,
    LL_LOW,    // Hard to see
    LL_LIT,
    LL_BRIGHT  // Probably only for light sources
};

#endif
