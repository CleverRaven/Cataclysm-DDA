#include <cassert>

#include "mapdata.h"
#include "map.h"
#include "lightmap.h"

#define LIGHT_RANGE(b) static_cast<int>(sqrt(b / LIGHT_AMBIENT_LOW) + 1)
#define INBOUNDS(x, y) (x >= -SEEX && x <= SEEX && y >= -SEEY && y <= SEEY)

template <typename T, size_t X, size_t Y>
void fill(T (&array)[X][Y], T const& value)
{
 for(size_t x = 0; x < X; ++x) {
  for(size_t y = 0; y < Y; ++y) {
   array[x][y] = value;
  }
 }
}

light_map::light_map()
 : lm()
 , sm()
{
 fill(lm, 0.0f);
 fill(sm, 0.0f);
}

void light_map::generate(map& m, int x, int y, float natural_light)
{
 fill(lm, 0.0f);
 fill(sm, 0.0f);

 // TODO: Apply player light sources (torch, biolight etc)
 //       maybe this should be past in light natural_light for easy reuse

 for(int sx = x - SEEX - LIGHT_RANGE(LIGHT_MAX_SOURCE); sx <= x + SEEX + LIGHT_RANGE(LIGHT_MAX_SOURCE); ++sx) {
  for(int sy = y - SEEY - LIGHT_RANGE(LIGHT_MAX_SOURCE); sy <= y + SEEY + LIGHT_RANGE(LIGHT_MAX_SOURCE); ++sy) {
   if (natural_light > LIGHT_AMBIENT_LOW && m.is_outside(sx, sy)) {
    // Apply sunlight, first light source so just assign
    if (INBOUNDS(sx, sy)) 
     lm[sx - x + SEEX][sy - y + SEEY] = natural_light;
    
    // TODO: Apply light sources for external/internal divide
    // foreach axial directions 
    //  if inside
    //   apply directional light towards axial direction based on natural light
   }

   // TODO: Attach light brightness to fields
   switch(m.field_at(sx, sy).type) {
    case fd_fire:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 50);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 20);
     else
      apply_light_source(m, sx, sy, x, y, 5);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_light_source(m, sx, sy, x, y, 20);
    case fd_electricity:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 10);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 2);
     else
      apply_light_source(m, sx, sy, x, y, LIGHT_SOURCE_LOCAL);  // kinda a hack as the square will still get marked
     break;
   }

   // TODO: Apply any vehicle light sources
  }
 } 
}

lit_level light_map::at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return LL_DARK; // Out of bounds

 if (sm[dx + SEEX][dy + SEEY] >= LIGHT_SOURCE_BRIGHT)
  return LL_BRIGHT;

 if (lm[dx + SEEX][dy + SEEY] >= LIGHT_AMBIENT_LIT)
  return LL_LIT;

 if (lm[dx + SEEX][dy + SEEY] >= LIGHT_AMBIENT_LOW)
  return LL_LOW;

 return LL_DARK;
}

void light_map::apply_light_source(map& m, int x, int y, int cx, int cy, float luminance)
{
 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 fill(lit, false);

 if (INBOUNDS(x - cx, y - cy)) {
  lit[x - cx + SEEX][y - cy + SEEY] = true;
  lm[x - cx + SEEX][y - cy + SEEY] += luminance;
  sm[x - cx + SEEX][y - cy + SEEY] += luminance;
 }

 if (luminance > LIGHT_SOURCE_LOCAL) {
  int range = LIGHT_RANGE(luminance);
  int sx = std::max(-SEEX, x - cx - range); int ex = std::min(SEEX, x - cx + range);
  int sy = std::max(-SEEY, y - cy - range); int ey = std::min(SEEY, y - cy + range);

  for(int off = sx; off <= ex; ++off) {
   apply_light_ray(m, lit, x, y, cx + off, cy + sy, cx, cy, luminance);
   apply_light_ray(m, lit, x, y, cx + off, cy + ey, cx, cy, luminance);
  }

  // Skip corners with + 1 and < as they were done
  for(int off = sy + 1; off < ey; ++off) {
   apply_light_ray(m, lit, x, y, cx + sx, cy + off, cx, cy, luminance);
   apply_light_ray(m, lit, x, y, cx + ex, cy + off, cx, cy, luminance);
  }
 }
}

void light_map::apply_light_ray(map& m, bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy, 
                                int ex, int ey, int cx, int cy, float luminance)
{
 int ax = abs(ex - sx) << 1;
 int ay = abs(ey - sy) << 1;
 int dx = (sx < ex) ? 1 : -1;
 int dy = (sy < ey) ? 1 : -1;
 int x = sx;
 int y = sy;

 // TODO: Pull out the common code here into something funky rather than duplication
 if (ax > ay) {
  int t = ay - (ax >> 1);
  do {
   if(t >= 0) {
    y += dy;
    t -= ax;
   }

   x += dx;
   t += ay;

   assert(x - cx >= -SEEX && x - cx <= SEEX);
   assert(y - cy >= -SEEY && y - cy <= SEEY);

   if (!lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know x is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sx - x) * (sx - x));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   // TODO: Rather than blocking light have a square check to see if light can be defused
   if (!m.trans(x, y))
    break;

  } while(!(x == ex && y == ey));
 } else {
  int t = ax - (ay >> 1);
  do {
   if(t >= 0) {
    x += dx;
    t -= ay;
   }
 
   y += dy;
   t += ax;

   assert(x - cx >= -SEEX && x - cx <= SEEX);
   assert(y - cy >= -SEEY && y - cy <= SEEY);

   if (!lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know y is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sy - y) * (sy - y));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   // TODO: Rather than blocking light have a square check to see if light can be defused
   if (!m.trans(x, y))
    break;

  } while(!(x == ex && y == ey));
 }
}

