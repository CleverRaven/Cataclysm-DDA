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

light_map::light_map(map *map)
 : lm()
 , m(map)
{
 fill(lm, LL_DARK);
}

void light_map::generate(int sight_range, int x, int y)
{
 cx = x;
 cy = y;
 fill(lm, LL_DARK);

 apply_light_mask(0, 0, sight_range, sight_range); //sight_range, sight_range);
 bool cast_light = (sight_range < std::max(SEEX, SEEY));

 for(int sx = cx - SEEX - max_light_range(); sx <= cx + SEEX + max_light_range(); ++sx) {
  for(int sy = cy - SEEY - max_light_range(); sy <= cy + SEEY + max_light_range(); ++sy) {
   // TODO: attach light brightness to fields
   switch(m->field_at(sx, sy).type) {
    case fd_fire:
     if (3 == m->field_at(sx, sy).density)
      apply_source(sx - cx, sy - cy, 50, cast_light);
     else if (2 == m->field_at(sx, sy).density)
      apply_source(sx - cx, sy - cy, 25, cast_light);
     else
      apply_source(sx - cx, sy - cy, 5, cast_light);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_source(sx - cx, sy - cy, 10, cast_light);
    case fd_electricity:
     apply_source(sx - cx, sy - cy, 5, cast_light);
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

void light_map::apply_source(int dx, int dy, int brightness, bool apply_mask)
{
 if (apply_mask) {
  int range = LIGHT_RANGE(brightness);
  int bright_range = LIGHT_RANGE(brightness / 2.0f);
  apply_light_mask(dx, dy, bright_range, range);
 }

 if(brightness >= LIGHT_SOURCE_BRIGHT && INBOUNDS(dx, dy))
  lm[XY2LM(dx, dy)] = LL_BRIGHT;
}

void light_map::apply_light_mask(int dx, int dy, int lit_range, int low_range)
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
//   if (!m->sees(fx, fy, fx + x, fy + y, max_light_range(), t))
//    continue;
   int off = XY2LM(x, y);
   if (x >= sbx && x <= ebx && y >= sby && y <= eby)
    lm[off] = LL_LIT; // areas in the bright range get lit
// TODO: map floating point lights
//   else if (LL_LOW == lm[off])
//    lm[off] = LL_LIT; // areas in mulitple low ranges get lit
   else if (LL_DARK == lm[off])
    lm[off] = LL_LOW; // dark areas are now marked low
  }
 }
}

