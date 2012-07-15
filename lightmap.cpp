
#include "mapdata.h"
#include "map.h"
#include "lightmap.h"

#define INBOUNDS(x, y) (x >= -SEEX && x <= SEEX && y >= -SEEY && y <= SEEY)
#define INBOUNDS_LARGE(x, y) (x >= -LIGHTMAP_RANGE_X && x <= LIGHTMAP_RANGE_X &&\
                               y >= -LIGHTMAP_RANGE_Y && y <= LIGHTMAP_RANGE_Y)

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

 int dir_x[] = { 1, 0 , -1,  0 };
 int dir_y[] = { 0, 1 ,  0, -1 };

 light_cache c;
 build_light_cache(m, x, y, c);

 if (natural_light > LIGHT_AMBIENT_LOW) {
  // Apply sunlight, first light source so just assign
  for(int sx = x - SEEX; sx <= x + SEEX; ++sx) {
   for(int sy = y - SEEY; sy <= y + SEEY; ++sy) {
    // Outside always has natural_light
    // In bright light indoor light exists to some degree
    if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].is_outside)
     lm[sx - x + SEEX][sy - y + SEEY] = natural_light;
    else if (natural_light > LIGHT_SOURCE_BRIGHT)
     lm[sx - x + SEEX][sy - y + SEEY] = LIGHT_AMBIENT_LOW;
   }
  }
 }

 // Apply player light sources
 if (luminance > LIGHT_AMBIENT_LOW)
  apply_light_source(c, x, y, x, y, luminance);

 for(int sx = x - LIGHTMAP_RANGE_X; sx <= x + LIGHTMAP_RANGE_X; ++sx) {
  for(int sy = y - LIGHTMAP_RANGE_Y; sy <= y + LIGHTMAP_RANGE_Y; ++sy) {
   // When underground natural_light is 0, if this changes we need to revisit
   if (natural_light > LIGHT_AMBIENT_LOW) {
    if (!c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].is_outside) {
     // Apply light sources for external/internal divide
     for(int i = 0; i < 4; ++i) {
      if (INBOUNDS_LARGE(sx - x + dir_x[i], sy - y + dir_y[i]) &&
          c[sx - x + LIGHTMAP_RANGE_X + dir_x[i]][sy - y + LIGHTMAP_RANGE_Y + dir_y[i]].is_outside) {
       if (INBOUNDS(sx - x, sy - y) && c[LIGHTMAP_RANGE_X][LIGHTMAP_RANGE_Y].is_outside)
        lm[sx - x + SEEX][sy - y + SEEY] = natural_light;
       
       if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].transparency > LIGHT_TRANSPARENCY_SOLID)
       	apply_light_arc(c, sx, sy, -dir_x[i], -dir_y[i], x, y, natural_light);
      }
	    }
    }
   }

   if (m.i_at(sx, sy).size() == 1 &&
       m.i_at(sx, sy)[0].type->id == itm_flashlight_on)
    apply_light_source(c, sx, sy, x, y, 20);

   if(m.ter(sx, sy) == t_lava)
    apply_light_source(c, sx, sy, x, y, 50);

   // TODO: [lightmap] Attach light brightness to fields
   switch(m.field_at(sx, sy).type) {
    case fd_fire:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(c, sx, sy, x, y, 50);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(c, sx, sy, x, y, 20);
     else
      apply_light_source(c, sx, sy, x, y, 3);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_light_source(c, sx, sy, x, y, 8);
     break;
    case fd_electricity:
     if (3 == m.field_at(sx, sy).density)
      apply_light_source(c, sx, sy, x, y, 8);
     else if (2 == m.field_at(sx, sy).density)
      apply_light_source(c, sx, sy, x, y, 1);
     else
      apply_light_source(c, sx, sy, x, y, LIGHT_SOURCE_LOCAL);  // kinda a hack as the square will still get marked
     break;
   }

   // Apply any vehicle light sources
   if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh &&
       c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light > LL_DARK) {
    if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light > LL_LIT) {
     // TODO: [lightmap] Improve for more than 4 directions
     int dir = c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh->face.dir4();
     float luminance = c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light;
     apply_light_arc(c, sx, sy, dir_x[dir], dir_y[dir], x, y, luminance);
    }
    apply_light_source(c, sx, sy, x, y, LIGHT_SOURCE_LOCAL);
   }

   // TODO: [lightmap] Apply creature light sources
   //       mon_zombie_electric - sometimes
   //       mon_flaming_eye
   //       manhack - maybe (glowing red eye?)
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

float light_map::ambient_at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return 0.0f;

 return lm[dx + SEEX][dy + SEEY];
}

void light_map::apply_light_source(light_cache& c, int x, int y, int cx, int cy, float luminance)
{
 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 fill(lit, false);

 if (INBOUNDS(x - cx, y - cy)) {
  lit[x - cx + SEEX][y - cy + SEEY] = true;
  lm[x - cx + SEEX][y - cy + SEEY] += std::max(luminance, static_cast<float>(LL_LOW));
  sm[x - cx + SEEX][y - cy + SEEY] += luminance;
 }

 if (luminance > LIGHT_SOURCE_LOCAL) {
  int range = LIGHT_RANGE(luminance);
  int sx = x - cx - range; int ex = x - cx + range;
  int sy = y - cy - range; int ey = y - cy + range;

  for(int off = sx; off <= ex; ++off) {
   apply_light_ray(c, lit, x, y, cx + off, cy + sy, cx, cy, luminance);
   apply_light_ray(c, lit, x, y, cx + off, cy + ey, cx, cy, luminance);
  }

  // Skip corners with + 1 and < as they were done
  for(int off = sy + 1; off < ey; ++off) {
   apply_light_ray(c, lit, x, y, cx + sx, cy + off, cx, cy, luminance);
   apply_light_ray(c, lit, x, y, cx + ex, cy + off, cx, cy, luminance);
  }
 }
}

void light_map::apply_light_arc(light_cache& c, int x, int y, int dx, int dy, int cx, int cy, float luminance)
{
 if (luminance <= LIGHT_SOURCE_LOCAL)
  return;

 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 fill(lit, false);

 int range = LIGHT_RANGE(luminance);

 if (0 == dy) {
  int ex = x - cx + (range * dx);
  int sy = y - cy - range; int ey = y - cy + range;

  for(int off = sy; off <= ey; ++off)
   apply_light_ray(c, lit, x, y, cx + ex, cy + off, cx, cy, luminance);

 } else if (0 == dx) {
  int sx = x - cx - range; int ex = x - cx + range;
  int ey = y - cy + (range * dy);

  for(int off = sx; off <= ex; ++off)
   apply_light_ray(c, lit, x, y, cx + off, cy + ey, cx, cy, luminance);

 } else {
  // TODO: [lightmap] Diagonal lights, will be needed for vehicles
 }
}

void light_map::apply_light_ray(light_cache& c, bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy,
                                int ex, int ey, int cx, int cy, float luminance)
{
 int ax = abs(ex - sx) << 1;
 int ay = abs(ey - sy) << 1;
 int dx = (sx < ex) ? 1 : -1;
 int dy = (sy < ey) ? 1 : -1;
 int x = sx;
 int y = sy;

 float transparency = LIGHT_TRANSPARENCY_CLEAR;

 // TODO: [lightmap] Pull out the common code here rather than duplication
 if (ax > ay) {
  int t = ay - (ax >> 1);
  do {
   if(t >= 0) {
    y += dy;
    t -= ax;
   }

   x += dx;
   t += ay;

   if (INBOUNDS(x - cx, y - cy) && !lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know x is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sx - x) * (sx - x));
    lm[x - cx + SEEX][y - cy + SEEY] += light * transparency;
   }

   if (INBOUNDS_LARGE(x - cx, y - cy))
    transparency *= c[x - cx + LIGHTMAP_RANGE_X][y - cy + LIGHTMAP_RANGE_Y].transparency;

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

   if (INBOUNDS(x - cx, y - cy) && !lit[x - cx + SEEX][y - cy + SEEY]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x - cx + SEEX][y - cy + SEEY] = true;

    // We know y is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sy - y) * (sy - y));
    lm[x - cx + SEEX][y - cy + SEEY] += light;
   }

   if (INBOUNDS_LARGE(x - cx, y - cy))
    transparency *= c[x - cx + LIGHTMAP_RANGE_X][y - cy + LIGHTMAP_RANGE_Y].transparency;

   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == ex && y == ey));
 }
}

// We only do this once now so we don't make 100k calls to is_outside for each
// generation. As well as close to that for the veh_at function.
void light_map::build_light_cache(map& m, int cx, int cy, light_cache& c)
{
 for(int sx = cx - LIGHTMAP_RANGE_X; sx <= cx + LIGHTMAP_RANGE_X; ++sx) {
  for(int sy = cy - LIGHTMAP_RANGE_Y; sy <= cy + LIGHTMAP_RANGE_Y; ++sy) {
   int x = sx - cx + LIGHTMAP_RANGE_X;
   int y = sy - cy + LIGHTMAP_RANGE_Y;

   c[x][y].is_outside = m.is_outside(sx, sy);
   c[x][y].transparency = LIGHT_TRANSPARENCY_CLEAR;
   c[x][y].veh = NULL;
   c[x][y].veh_light = 0;

   // TODO: [lightmap] As cache generation takes a lot of our time and vehicles
   //                  cover multiple squares then pull out of loop and do once
   //                  per vehicle in the wider area.
   int vpart = -1;
   vehicle *veh = NULL;

   if (c[x][y].is_outside)
    veh = m.veh_at(sx, sy, vpart);

   if (veh) {
    c[x][y].veh = veh;
    if (veh->part_flag(vpart, vpf_opaque) && veh->parts[vpart].hp > 0) {
     int dpart = veh->part_with_feature(vpart, vpf_openable);
     if (dpart < 0 || !veh->parts[dpart].open)
      c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
    }

    // Check for vehicle lights
    for(size_t i = 0; i < veh->parts.size(); ++i) {
     // We already know vpart is on our square
     if (veh->part_flag(i, vpf_light) &&
         veh->parts[i].precalc_dx[0] == veh->parts[vpart].precalc_dx[0] &&
         veh->parts[i].precalc_dy[0] == veh->parts[vpart].precalc_dy[0]) {
      c[x][y].veh_light = veh->part_info(i).power;
     }
    }
   } else if (!(terlist[m.ter(sx, sy)].flags & mfb(transparent)))
    c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;

   field& f = m.field_at(sx, sy);
   if(f.type > 0) {
    if(!fieldlist[f.type].transparent[f.density - 1]) {
     // Fields are either transparent or not, however we want some to be translucent
     switch(f.type) {
      case fd_smoke:
      case fd_toxic_gas:
      case fd_tear_gas:
       if(f.density == 3)
        c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
       if(f.density == 2)
        c[x][y].transparency *= 0.5;
       break;
      case fd_nuke_gas:
       c[x][y].transparency *= 0.5;
       break;
      default:
       c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
       break;
     }
    }

    // TODO: [lightmap] Have glass reduce light as well
   }
  }
 }
}
