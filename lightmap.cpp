#include <cassert>

#include "mapdata.h"
#include "map.h"
#include "lightmap.h"

// TODO: move these to const functions
#define XY2LM(x, y) (((y + SEEY) * (2 * SEEX + 1)) + x + SEEX)
#define LIGHT_RANGE(b) static_cast<int>(sqrt(b / LIGHT_AMBIENT_LOW) + 1);
#define INBOUNDS(x, y) (x >= -SEEX && x <= SEEX && y >= -SEEY && y <= SEEY)

int max_light_range()
{
 static int max = LIGHT_RANGE(LIGHT_MAX_SOURCE);
 return max;
}

// TODO: replace with std::array::fill
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
{
 fill(lm, 0.0f);
}

void light_map::generate(map& m, int x, int y)
{
 fill(lm, 0.0f);

 for(int sx = x - SEEX - max_light_range(); sx <= x + SEEX + max_light_range(); ++sx) {
  for(int sy = y - SEEY - max_light_range(); sy <= y + SEEY + max_light_range(); ++sy) {
   // TODO: attach light brightness to fields
   switch(m.field_at(sx, sy).type) {
    case fd_fire:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 50);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 25);
     else
      apply_light_source(m, sx, sy, x, y, 5);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_light_source(m, sx, sy, x, y, 10);
    case fd_electricity:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 5);
	 else if (2 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 1);
	 else
	  // kinda a hack as the square will still get marked
      apply_light_source(m, sx, sy, x, y, 0);
     break;
   }
  }
 } 
}

int light_map::at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return LL_DARK; // out of bounds

 // TODO: add source

 if (lm[dx][dy] >= LIGHT_AMBIENT_LIT)
  return LL_LIT;

 if (lm[dx][dy] >= LIGHT_AMBIENT_LOW)
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
 }

 int range = LIGHT_RANGE(luminance);
 int sx = std::max(-SEEX, x - cx - range); int ex = std::min(SEEX, x - cx + range);
 int sy = std::max(-SEEY, y - cy - range); int ey = std::min(SEEY, y - cy + range);

 for(int off = sx; off <= ex; ++off) {
  apply_light_ray(m, lit, x, y, cx + off, cy + sy, cx, cy, luminance);
  apply_light_ray(m, lit, x, y, cx + off, cy + ey, cx, cy, luminance);
 }

 for(int off = sy; off <= ey; ++off) {
  apply_light_ray(m, lit, x, y, cx + sx, cy + off, cx, cy, luminance);
  apply_light_ray(m, lit, x, y, cx + ex, cy + off, cx, cy, luminance);
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

 // TODO: pull out the common code here into some funky template rather than duplication
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
    // multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

   	// We know x is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sx - x) * (sx - x));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   // TODO: rather than blocking light have a square check to see if light can be defused
   if (m.trans(x, y))
   	break;

  } while(x != ex && y != ey);
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
    // multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know y is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sy - y) * (sy - y));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   // TODO: rather than blocking light have a square check to see if light can be defused
   if (m.trans(x, y))
   	break;

  } while(x != ex && y != ey);
 }
}

