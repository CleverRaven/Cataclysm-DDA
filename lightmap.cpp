#include "mapdata.h"
#include "map.h"
#include "lightmap.h"

// TODO: move these to const functions
#define XY2LM(x, y) (((y + SEEY) * SEEX) + dx + SEEX)
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
 : m()
{
 fill(m, LL_DARK);
}

void light_map::generate(int sight_range, map *map, int cx, int cy)
{
 fill(m, LL_DARK);
 apply_light_mask(0, 0, sight_range, sight_range);
 bool cast_light = (sight_range < std::max(SEEX, SEEY));

 for(int x = cx - SEEX - max_light_range(); x <= cx + SEEX + max_light_range(); ++x) {
  for(int y = cy - SEEY - max_light_range(); y <= cy + SEEX + max_light_range(); ++y) {
   // TODO: attach light brightness to fields
   switch(map->field_at(x, y).type) {
    case fd_fire:
     if (3 == map->field_at(x,y).density)
      apply_source(x, y, 50, cast_light);
     else if (2 == map->field_at(x, y).density)
      apply_source(x, y, 25, cast_light);
     else
      apply_source(x, y, 5, cast_light);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_source(x, y, 10, cast_light);
    case fd_electricity:
     apply_source(x, y, 5, cast_light);
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

 return m[XY2LM(dx, dy)];
}

void light_map::apply_source(int dx, int dy, int brightness, bool apply_mask)
{
 if (apply_mask) {
  int range = LIGHT_RANGE(brightness);
  int bright_range = LIGHT_RANGE(brightness / 2.0f);
  apply_light_mask(dx, dy, bright_range, range);
 }

 if(brightness >= LIGHT_SOURCE_BRIGHT && INBOUNDS(dx, dy))
  m[XY2LM(dx, dy)] = LL_BRIGHT;
}

void light_map::apply_light_mask(int dx, int dy, int lit_range, int low_range)
{
 int range = std::max(lit_range, low_range);
 int sx = std::max(0, dx - range); int ex = std::min(2 * SEEX + 1, dx + range);
 int sy = std::max(0, dy - range); int ey = std::min(2 * SEEY + 1, dy + range);
 
 int sbx = std::max(0, dx - lit_range); int ebx = std::min(2 * SEEX + 1, dx + lit_range);
 int sby = std::max(0, dy - lit_range); int eby = std::min(2 * SEEY + 1, dy + lit_range);

 for(int x = sx; x < ex; ++x) {
  for(int y = sy; y < ey; ++y) {
   if (x >= sbx && x <= ebx && y >= sby && y <= eby)
    m[XY2LM(x, y)] = LL_LIT; // areas in the bright range get lit
   else if (LL_LOW == m[XY2LM(x, y)])
    m[XY2LM(x, y)] = LL_LIT; // areas in mulitple low ranges get lit
   else if (LL_DARK == m[XY2LM(x, y)])
    m[XY2LM(x, y)] = LL_LOW; // dark areas are now marked low
  }
 }
}

