
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

void light_map::generate(map& m, int x, int y, float natural_light, float luminance)
{
 fill(lm, 0.0f);
 fill(sm, 0.0f);

 int dir_x[] = {  0, 1, 0 , -1 };
 int dir_y[] = { -1, 0, 1 ,  0 };

 if (natural_light > LIGHT_AMBIENT_LOW) {
  // Apply sunlight, first light source so just assign
  for(int sx = x - SEEX; sx <= x + SEEX; ++sx) {
   for(int sy = y - SEEY; sy <= y + SEEY; ++sy) {
    // Outside always has natural_light
    // In bright light indoor light exists to some degree
    if (m.is_outside(sx, sy))
     lm[sx - x + SEEX][sy - y + SEEY] = natural_light;
    else if (natural_light > LIGHT_SOURCE_BRIGHT)
     lm[sx - x + SEEX][sy - y + SEEY] = LIGHT_AMBIENT_LOW;
   }
  }
 }

 for(int sx = x - SEEX - LIGHT_RANGE(LIGHT_MAX_SOURCE); sx <= x + SEEX + LIGHT_RANGE(LIGHT_MAX_SOURCE); ++sx) {
  for(int sy = y - SEEY - LIGHT_RANGE(LIGHT_MAX_SOURCE); sy <= y + SEEY + LIGHT_RANGE(LIGHT_MAX_SOURCE); ++sy) {
   // When underground natural_light is 0, if this changes we need to revisit
   if (natural_light > LIGHT_AMBIENT_LOW) {
   	if (!m.is_outside(sx, sy)) {
     // Apply light sources for external/internal divide
     for(int i = 0; i < 4; ++i) {
      if (m.is_outside(sx + dir_x[i], sy + dir_y[i])) {
       if (INBOUNDS(sx - x, sy - y) && m.is_outside(x, y))
        lm[sx - x + SEEX][sy - y + SEEY] = natural_light;
       
       // This is 2 because it turns out everything is too dark otherwise
       if (check_opacity(m, sx, sy) > LIGHT_TRANSPARENCY_SOLID)
       	apply_light_arc(m, sx, sy, -dir_x[i], -dir_y[i], x, y, 2 * natural_light);
      }
	    }
    }
   }

   // Apply player light sources
   if (luminance > LIGHT_AMBIENT_LOW)
    apply_light_source(m, x, y, x, y, luminance);

   // TODO: Attach light brightness to fields
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
     break;
    case fd_electricity:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 10);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(m, sx, sy, x, y, 1);
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

float light_map::ambience_at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return 0.0f;

 return lm[dx + SEEX][dy + SEEY];
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
  int sx = x - cx - range; int ex = x - cx + range;
  int sy = y - cy - range; int ey = y - cy + range;

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

void light_map::apply_light_arc(map& m, int x, int y, int dx, int dy, int cx, int cy, float luminance)
{
 if (luminance <= LIGHT_SOURCE_LOCAL)
  return;

 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 fill(lit, false);

 int range = LIGHT_RANGE(luminance);

 if (0 == dy) {
  int sx = x - cx; int ex = x - cx + (range * dx);
  int sy = y - cy - range; int ey = y - cy + range;

  for(int off = sx; off != ex + dx; off += dx) {
   apply_light_ray(m, lit, x, y, cx + off, cy + sy, cx, cy, luminance);
   apply_light_ray(m, lit, x, y, cx + off, cy + ey, cx, cy, luminance);
  }
  for(int off = sy; off <= ey; ++off)
   apply_light_ray(m, lit, x, y, cx + ex, cy + off, cx, cy, luminance);
 } else if (0 == dx) {
  int sx = x - cx - range; int ex = x - cx + range;
  int sy = y - cy; int ey = y - cy + (range * dy);

  for(int off = sy; off != ey + dy; off += dy) {
   apply_light_ray(m, lit, x, y, cx + sx, cy + off, cx, cy, luminance);
   apply_light_ray(m, lit, x, y, cx + ex, cy + off, cx, cy, luminance);
  }
  for(int off = sx; off <= ex; ++off)
   apply_light_ray(m, lit, x, y, cx + off, cy + ey, cx, cy, luminance);
 } else {
  // TODO: diagonal lights, will be needed for vehicles
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

 float transparency = LIGHT_TRANSPARENCY_CLEAR;

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

   if (!INBOUNDS(x - cx, y - cy))
   	continue;

   if (!lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know x is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sx - x) * (sx - x));
    lm[x - cx + SEEX][y - cy + SEEY] += light * transparency;
   }

   transparency = check_opacity(m, x, y, transparency); 
   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
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

   if (!INBOUNDS(x - cx, y - cy))
   	continue;

   if (!lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know y is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sy - y) * (sy - y));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   transparency = check_opacity(m, x, y, transparency); 
   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == ex && y == ey));
 }
}

float light_map::check_opacity(map& m, int x, int y, float transparency)
{
 int vpart = -1;
 vehicle *veh = NULL;

 // Vehicles aren't found indoors
 if (m.is_outside(x, y))
  m.veh_at(x, y, vpart);

 if (veh) {
  if (veh->part_flag(vpart, vpf_opaque) && veh->parts[vpart].hp > 0) {
   int dpart = veh->part_with_feature(vpart, vpf_openable);
   if (dpart < 0 || !veh->parts[dpart].open)
   	transparency = LIGHT_TRANSPARENCY_SOLID;
  }
 } else if (!(terlist[m.ter(x, y)].flags & mfb(transparent)))
  transparency = LIGHT_TRANSPARENCY_SOLID;

 if(m.field_at(x, y).type > 0) {
  if(!fieldlist[m.field_at(x, y).type].transparent[m.field_at(x, y).density - 1]) {
   // Fields are either transparent or not, however we want some to be translucent
   switch(m.field_at(x, y).type) {
    case fd_smoke:
    case fd_toxic_gas:
	   case fd_tear_gas:
     if(m.field_at(x, y).density == 3)
      transparency = LIGHT_TRANSPARENCY_SOLID;
     if(m.field_at(x, y).density == 2)
      transparency *= 0.5;
     break;
    case fd_nuke_gas:
     transparency *= 0.5;
     break;
    default:
     transparency = LIGHT_TRANSPARENCY_SOLID;
     break;
   }
  }
 }

 // TODO: Have glass reduce light as well

 return transparency;
}
