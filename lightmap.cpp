#include "mapdata.h"
#include "map.h"
#include "game.h"
#include "lightmap.h"

#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * MAPSIZE && y >= 0 && y < SEEY * MAPSIZE)
#define LIGHTMAP_CACHE_X SEEX * MAPSIZE
#define LIGHTMAP_CACHE_Y SEEY * MAPSIZE

void map::generate_lightmap(game* g)
{
 memset(lm, 0, sizeof(lm));
 memset(sm, 0, sizeof(sm));

 const int dir_x[] = { 1, 0 , -1,  0 };
 const int dir_y[] = { 0, 1 ,  0, -1 };
 const int dir_d[] = { 180, 270, 0, 90 };
 const float luminance = g->u.active_light();
 const float natural_light = g->natural_light_level();

 // Daylight vision handling returned back to map due to issues it causes here
 if (natural_light > LIGHT_SOURCE_BRIGHT) {
  // Apply sunlight, first light source so just assign
  for(int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx) {
   for(int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy) {
    // In bright light indoor light exists to some degree
    if (!g->m.is_outside(sx, sy))
     lm[sx][sy] = LIGHT_AMBIENT_LOW;
   }
  }
 }

 // Apply player light sources
 if (luminance > LIGHT_AMBIENT_LOW)
  apply_light_source(g->u.posx, g->u.posy, luminance);

  for(int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx) {
   for(int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy) {
    const ter_id terrain = g->m.ter(sx, sy);
    const std::vector<item> items = g->m.i_at(sx, sy);
    const field current_field = g->m.field_at(sx, sy);
    // When underground natural_light is 0, if this changes we need to revisit
    if (natural_light > LIGHT_AMBIENT_LOW) {
     if (!g->m.is_outside(sx, sy)) {
      // Apply light sources for external/internal divide
      for(int i = 0; i < 4; ++i) {
       if (INBOUNDS(sx + dir_x[i], sy + dir_y[i]) &&
           g->m.is_outside(sx + dir_x[i], sy + dir_y[i])) {
        if (INBOUNDS(sx, sy) && g->m.is_outside(0, 0))
         lm[sx][sy] = natural_light;

        if (g->m.light_transparency(sx, sy) > LIGHT_TRANSPARENCY_SOLID)
         apply_light_arc(sx, sy, dir_d[i], natural_light);
       }
      }
     }
    }

    if (items.size() == 1 &&
        items[0].has_flag(IF_LIGHT_20))
     apply_light_source(sx, sy, 20);

   if (items.size() == 1 &&
       items[0].has_flag(IF_LIGHT_1))
    apply_light_source(sx, sy, 1);

   if(terrain == t_lava)
    apply_light_source(sx, sy, 50);

   if(terrain == t_console)
    apply_light_source(sx, sy, 3);

   if (items.size() == 1 &&
       items[0].has_flag(IF_LIGHT_4))
    apply_light_source(sx, sy, 4);

   if(terrain == t_emergency_light)
    apply_light_source(sx, sy, 3);

   // TODO: [lightmap] Attach light brightness to fields
  switch(current_field.type) {
    case fd_fire:
     if (3 == current_field.density)
      apply_light_source(sx, sy, 160);
     else if (2 == current_field.density)
      apply_light_source(sx, sy, 60);
     else
      apply_light_source(sx, sy, 16);
     break;
    case fd_fire_vent:
    case fd_flame_burst:
     apply_light_source(sx, sy, 8);
     break;
    case fd_electricity:
     if (3 == current_field.density)
      apply_light_source(sx, sy, 8);
     else if (2 == current_field.density)
      apply_light_source(sx, sy, 1);
     else
      apply_light_source(sx, sy, LIGHT_SOURCE_LOCAL);  // kinda a hack as the square will still get marked
     break;
   }
  }
 }

 for (int i = 0; i < g->z.size(); ++i) {
  int mx = g->z[i].posx;
  int my = g->z[i].posy;
  if (INBOUNDS(mx, my)) {
   if (g->z[i].has_effect(ME_ONFIRE)) {
    apply_light_source(mx, my, 3);
   }
   // TODO: [lightmap] Attach natural light brightness to creatures
   // TODO: [lightmap] Allow creatures to have light attacks (ie: eyebot)
   // TODO: [lightmap] Allow creatures to have facing and arc lights
   switch (g->z[i].type->id) {
    case mon_zombie_electric:
     apply_light_source(mx, my, 1);
     break;
    case mon_turret:
     apply_light_source(mx, my, 2);
     break;
    case mon_flaming_eye:
     apply_light_source(mx, my, LIGHT_SOURCE_BRIGHT);
     break;
    case mon_manhack:
     apply_light_source(mx, my, LIGHT_SOURCE_LOCAL);
     break;
   }
  }
 }

 // Apply any vehicle light sources
 VehicleList vehs = g->m.get_vehicles();
 for(int v = 0; v < vehs.size(); ++v) {
  if(vehs[v].v->lights_on) {
   int dir = vehs[v].v->face.dir();
   for (std::vector<int>::iterator part = vehs[v].v->external_parts.begin();
        part != vehs[v].v->external_parts.end(); ++part) {
    int px = vehs[v].x + vehs[v].v->parts[*part].precalc_dx[0];
    int py = vehs[v].y + vehs[v].v->parts[*part].precalc_dy[0];
    if(INBOUNDS(px, py)) {
     int dpart = vehs[v].v->part_with_feature(*part , vpf_light);
     if (dpart >= 0) {
      float luminance = vehs[v].v->part_info(dpart).power;
      if (luminance > LL_LIT) {
        apply_light_arc(px, py, dir, luminance);
      }
     }
    }
   }
  }
 }
}

lit_level map::light_at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return LL_DARK; // Out of bounds

 if (sm[dx][dy] >= LIGHT_SOURCE_BRIGHT)
  return LL_BRIGHT;

 if (lm[dx][dy] >= LIGHT_AMBIENT_LIT)
  return LL_LIT;

 if (lm[dx][dy] >= LIGHT_AMBIENT_LOW)
  return LL_LOW;

 return LL_DARK;
}

float map::ambient_light_at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return 0.0f;

 return lm[dx][dy];
}

bool map::pl_sees(int fx, int fy, int tx, int ty, int max_range)
{
  if (!INBOUNDS(tx, ty)) return false;

 if (max_range >= 0 && (abs(tx - fx) > max_range || abs(ty - fy) > max_range))
  return false; // Out of range!

 return seen_cache[tx][ty];
}

void map::cache_seen(int fx, int fy, int tx, int ty, int max_range)
{
   if (!INBOUNDS(fx, fy) || !INBOUNDS(tx, ty)) return;

   const int ax = abs(tx - fx) << 1;
   const int ay = abs(ty - fy) << 1;
   const int dx = (fx < tx) ? 1 : -1;
   const int dy = (fy < ty) ? 1 : -1;
   int x = fx;
   int y = fy;
   bool seen = true;

   // TODO: [lightmap] Pull out the common code here rather than duplication
   if (ax > ay)
   {
      int t = ay - (ax >> 1);
      do
      {
         if(t >= 0 && ((y + dy != ty) || (x + dx == tx)))
         {
            y += dy;
            t -= ax;
         }

         x += dx;
         t += ay;

         seen_cache[x][y] |= seen;
         if(light_transparency(x, y) == LIGHT_TRANSPARENCY_SOLID) seen = false;

      } while(!(x == tx && y == ty));
   }
   else
   {
      int t = ax - (ay >> 1);
      do
      {
         if(t >= 0 && ((x + dx != tx) || (y + dy == ty)))
         {
            x += dx;
            t -= ay;
         }

         y += dy;
         t += ax;

         seen_cache[x][y] |= seen;
         if(light_transparency(x, y) == LIGHT_TRANSPARENCY_SOLID) seen = false;

      } while(!(x == tx && y == ty));
   }
}

void map::apply_light_source(int x, int y, float luminance)
{
 bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
 memset(lit, 0, sizeof(lit));

 if (INBOUNDS(x, y)) {
  lit[x][y] = true;
  lm[x][y] += std::max(luminance, static_cast<float>(LL_LOW));
  sm[x][y] += luminance;
 }

 if (luminance > LIGHT_SOURCE_LOCAL) {
  int range = LIGHT_RANGE(luminance);
  int sx = x - range; int ex = x + range;
  int sy = y - range; int ey = y + range;

  for(int off = sx; off <= ex; ++off) {
   apply_light_ray(lit, x, y, off, sy, luminance);
   apply_light_ray(lit, x, y, off, ey, luminance);
  }

  // Skip corners with + 1 and < as they were done
  for(int off = sy + 1; off < ey; ++off) {
   apply_light_ray(lit, x, y, sx, off, luminance);
   apply_light_ray(lit, x, y, ex, off, luminance);
  }
 }
}

void map::apply_light_arc(int x, int y, int angle, float luminance)
{
 if (luminance <= LIGHT_SOURCE_LOCAL)
  return;

 bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y];
 memset(lit, 0, sizeof(lit));

 int range = LIGHT_RANGE(luminance);
 apply_light_source(x, y, LIGHT_SOURCE_LOCAL);

 // Normalise (should work with negative values too)
 angle = angle % 360;

 // East side
 if (angle < 90 || angle > 270) {
  int sy = y - ((angle <  90) ? range * (( 45 - angle) / 45.0f) : range);
  int ey = y + ((angle > 270) ? range * ((angle - 315) / 45.0f) : range);

  int ox = x + range;
  for(int oy = sy; oy <= ey; ++oy)
   apply_light_ray(lit, x, y, ox, oy, luminance);
 }

 // South side
 if (angle < 180) {
  int sx = x - ((angle < 90) ? range * (( angle - 45) / 45.0f) : range);
  int ex = x + ((angle > 90) ? range * ((135 - angle) / 45.0f) : range);

  int oy = y + range;
  for(int ox = sx; ox <= ex; ++ox)
   apply_light_ray(lit, x, y, ox, oy, luminance);
 }

 // West side
 if (angle > 90 && angle < 270) {
  int sy = y - ((angle < 180) ? range * ((angle - 135) / 45.0f) : range);
  int ey = y + ((angle > 180) ? range * ((225 - angle) / 45.0f) : range);

  int ox = x - range;
  for(int oy = sy; oy <= ey; ++oy)
   apply_light_ray(lit, x, y, ox, oy, luminance);
 }

 // North side
 if (angle > 180) {
  int sx = x - ((angle > 270) ? range * ((315 - angle) / 45.0f) : range);
  int ex = x + ((angle < 270) ? range * ((angle - 225) / 45.0f) : range);

  int oy = y - range;
  for(int ox = sx; ox <= ex; ++ox)
   apply_light_ray(lit, x, y, ox, oy, luminance);
 }
}

void map::apply_light_ray(bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y],
                          int sx, int sy, int ex, int ey, float luminance)
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

   if (INBOUNDS(x, y) && !lit[x][y]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x][y] = true;

    // We know x is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sx - x) * (sx - x));
    lm[x][y] += light * transparency;
   }

   if (INBOUNDS(x, y))
     transparency *= light_transparency(x, y);

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

   if (INBOUNDS(x, y) && !lit[x][y]) {
    // Multiple rays will pass through the same squares so we need to record that
    lit[x][y] = true;

    // We know y is the longest angle here and squares can ignore the abs calculation
    float light = luminance / ((sy - y) * (sy - y));
    lm[x][y] += light;
   }

   if (INBOUNDS(x, y))
    transparency *= light_transparency(x, y);

   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == ex && y == ey));
 }
}
