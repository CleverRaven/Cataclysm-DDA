#ifndef _LIGHTMAP_H_
#define _LIGHTMAP_H_

#include "mapdata.h"
#include "map.h"

// Raging fire value, also the most we want to check for
#define LIGHT_MAX_SOURCE    250

#define LIGHT_SOURCE_LOCAL  1
#define LIGHT_SOURCE_BRIGHT 50

#define LIGHT_AMBIENT_LOW   5
#define LIGHT_AMBIENT_LIT   25

#define LIGHTMAP_X (2 * SEEX + 1)
#define LIGHTMAP_Y (2 * SEEY + 1)

enum lit_level {
 LL_DARK = 0,
 LL_LOW,    // Hard to see
 LL_LIT,
 LL_BRIGHT  // Probably only for light sources
};

class light_map
{
 public:
  light_map();

  void generate(map& m, int x, int y, float natural_light);
  
  // TODO: For the tileset version this should provide the raw data so we get gradient light
  lit_level at(int dx, int dy); // Assumes 0,0 is light map center

 private:
  float lm[LIGHTMAP_X][LIGHTMAP_Y];
  float sm[LIGHTMAP_X][LIGHTMAP_Y];

  void apply_light_source(map& m, int x, int y, int cx, int cy, float luminance);

  void apply_light_ray(map& m, bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy,
                       int ex, int ey, int cx, int cy, float luminance);
};

#endif
