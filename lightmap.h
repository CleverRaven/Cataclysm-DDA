#ifndef _LIGHTMAP_H_
#define _LIGHTMAP_H_

#include "mapdata.h"
#include "map.h"

#define LIGHT_MAX_SOURCE    50 // ^shrugs^
#define LIGHT_SOURCE_BRIGHT 10

#define LIGHT_AMBIENT_LOW   1
#define LIGHT_AMBIENT_LIT   5

#define LIGHTMAP_X 2 * SEEX + 1
#define LIGHTMAP_Y 2 * SEEY + 1

enum lit_level {
 LL_DARK = 0,
 LL_LOW,    // hard to see
 LL_LIT,
 LL_BRIGHT  // probably only for light sources
};

class light_map
{
 public:
  light_map();

  void generate(map& m, int x, int y);
  int /*lit_level*/ at(int dx, int dy); // assumes 0,0 is light map center

 private:
  float lm[LIGHTMAP_X][LIGHTMAP_Y];

  void apply_light_source(map& m, int x, int y, int cx, int cy, float luminance);

  void apply_light_ray(map& m, bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy,
                       int ex, int ey, int cx, int cy, float luminance);
};

#endif
