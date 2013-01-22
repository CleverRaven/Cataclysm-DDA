#ifndef _LIGHTMAP_H_
#define _LIGHTMAP_H_

#include "mapdata.h"
#include "map.h"

#define LIGHT_SOURCE_LOCAL  0.1
#define LIGHT_SOURCE_BRIGHT 10

#define LIGHT_AMBIENT_LOW   1
#define LIGHT_AMBIENT_LIT   2

#define LIGHT_TRANSPARENCY_SOLID 0
#define LIGHT_TRANSPARENCY_CLEAR 1

#define LIGHTMAP_X (2 * SEEX + 1)
#define LIGHTMAP_Y (2 * SEEY + 1)

#define LIGHT_RANGE(b) static_cast<int>(sqrt(b / LIGHT_AMBIENT_LOW) + 1)
#define LIGHT_MAX_RANGE_X (SEEX*(MAPSIZE/2))
#define LIGHT_MAX_RANGE_Y (SEEX*(MAPSIZE/2))
#define LIGHTMAP_RANGE_X (SEEX + LIGHT_MAX_RANGE_X)
#define LIGHTMAP_RANGE_Y (SEEY + LIGHT_MAX_RANGE_Y)

#define LIGHTMAP_CACHE_X (2 * LIGHTMAP_RANGE_X + 1)
#define LIGHTMAP_CACHE_Y (2 * LIGHTMAP_RANGE_Y + 1)

enum lit_level {
 LL_DARK = 0,
 LL_LOW,    // Hard to see
 LL_LIT,
 LL_BRIGHT  // Probably only for light sources
};

struct light_map_cache {
 float transparency;
 vehicle* veh;
 int veh_part;
 int veh_light;
 int mon;
};

class light_map
{
 public:
  light_map();

  void generate(game* g, int x, int y, float natural_light, float luminance);
  
  lit_level at(int dx, int dy); // Assumes 0,0 is light map center
  float ambient_at(int dx, int dy); // Raw values for tilesets

  bool is_outside(int dx, int dy);
  bool sees(int fx, int fy, int tx, int ty, int max_range);

 private:
  typedef light_map_cache light_cache[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
  float lm[LIGHTMAP_X][LIGHTMAP_Y];
  float sm[LIGHTMAP_X][LIGHTMAP_Y];
  bool outside_cache[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
  light_cache c;

  void apply_light_source(int x, int y, int cx, int cy, float luminance);
  void apply_light_arc(int x, int y, int angle, int cx, int cy, float luminance);

  void apply_light_ray(bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy,
                       int ex, int ey, int cx, int cy, float luminance);

  void build_outside_cache(map *m, const int x, const int y, const int sx, const int sy);
  void build_light_cache(game* g, int x, int y);
};

#endif
