#ifndef _LIGHTMAP_H_
#define _LIGHTMAP_H_

#include "mapdata.h"
#include "map.h"

#define LIGHT_MAX_SOURCE    50 // ^shrugs^
#define LIGHT_SOURCE_BRIGHT 10

#define LIGHT_AMBIENT_LOW   1
#define LIGHT_AMBIENT_LIT   5

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

  void generate(map* m, int x, int y);
  lit_level at(int dx, int dy); // assumes 0,0 is light map center

 private:
  // TODO: sneak c++0x requirement into catalysm
  //std::array<lit_level, (2 * SEEX + 1) * (2 * SEEY + 1)> light_map;
  lit_level lm[(2 * SEEX + 1) * (2 * SEEY + 1)];

  void apply_source(map* m, int cx, int cy, int dx, int dy, int brightness);
  void apply_light_mask(map* m, int cx, int cy, int dx, int dy, int lit_range, int low_range);
};

#endif
