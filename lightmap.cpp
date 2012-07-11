#include "mapdata.h"
#include "map.h"
#include "lightmap.h"

// TODO: move these to const functions
#define XY2LM(x, y) (((y + SEEY) * (2 * SEEX + 1)) + x + SEEX)
#define LIGHT_RANGE(b) static_cast<int>(sqrt(b / LIGHT_AMBIENT_LOW));
#define INBOUNDS(x, y) (x >= -SEEX && x <= SEEX && y >= -SEEY && y <= SEEY)

int max_light_range()
{
 static int max = LIGHT_RANGE(LIGHT_MAX_SOURCE);
 return max;
}

// TODO: replace with std::array::fill
template <typename T, size_t N>
void fill(T (&array)[N], T const& value)
{
 for(size_t i = 0; i < N; ++i) {
  array[i] = value;
 }
}

light_map::light_map()
 : lm()
{
 fill(lm, LL_DARK);
}

void light_map::generate(map* m, int x, int y)
{
 fill(lm, LL_DARK);

 for(int sx = x - SEEX - max_light_range(); sx <= x + SEEX + max_light_range(); ++sx) {
  for(int sy = y - SEEY - max_light_range(); sy <= y + SEEY + max_light_range(); ++sy) {
   // TODO: attach light brightness to fields
   switch(m->field_at(sx, sy).type) {
    case fd_fire:
     if (3 == m->field_at(sx, sy).density)
      apply_source(m, x, y, sx - x, sy - y, 50);
     else if (2 == m->field_at(sx, sy).density)
      apply_source(m, x, y, sx - x, sy - y, 25);
     else
      apply_source(m, x, y, sx - x, sy - y, 5);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_source(m, x, y, sx - x, sy - y, 10);
    case fd_electricity:
     if (3 == m->field_at(sx, sy).density)
      apply_source(m, x, y, sx - x, sy - y, 5);
	 else if (2 == m->field_at(sx, sy).density)
      apply_source(m, x, y, sx - x, sy - y, 1);
	 else
	  // kinda a hack as the square will still get marked
      apply_source(m, x, y, sx - x, sy - y, 0);
     break;
   }
  }
 } 
}

lit_level light_map::at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy)) {
  return LL_DARK; // out of bounds
 }

 int off = XY2LM(dx, dy);
 int foo = lm[XY2LM(dx, dy)];

 return lm[XY2LM(dx, dy)];
}

void light_map::apply_source(map* m, int cx, int cy, int dx, int dy, int brightness)
{
 int range = LIGHT_RANGE(brightness);
 int bright_range = LIGHT_RANGE(brightness / 2.0f);
 apply_light_mask(m, cx, cy, dx, dy, bright_range, range);

 if (INBOUNDS(dx, dy)) 
  lm[XY2LM(dx, dy)] = (brightness >= LIGHT_SOURCE_BRIGHT) ? LL_BRIGHT : LL_LIT;
}

void light_map::apply_light_mask(map* m, int cx, int cy, int dx, int dy, int lit_range, int low_range)
{
 int range = std::max(lit_range, low_range);
 int sx = std::max(-SEEX, dx - range); int ex = std::min(SEEX, dx + range);
 int sy = std::max(-SEEY, dy - range); int ey = std::min(SEEY, dy + range);
 
 int sbx = std::max(-SEEX, dx - lit_range); int ebx = std::min(SEEX, dx + lit_range);
 int sby = std::max(-SEEY, dy - lit_range); int eby = std::min(SEEY, dy + lit_range);

 // [f]ire coords = center + delta
 int fx = cx + dx;
 int fy = cy + dy;

 int t;
 for(int x = sx; x <= ex; ++x) {
  for(int y = sy; y <= ey; ++y) {
   if (!m->sees(fx, fy, cx + x, cy + y, max_light_range(), t))
    continue;
   int off = XY2LM(x, y);
   if (LL_BRIGHT == lm[off])
    continue; // skip sources

   if (x >= sbx && x <= ebx && y >= sby && y <= eby)
    lm[off] = LL_LIT; // areas in the bright range get lit
   else if (LL_DARK == lm[off])
    lm[off] = LL_LOW; // dark areas are now marked low
  }
 }
}

