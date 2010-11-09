#include "map.h"
#include "omdata.h"
#include "mapitems.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "line.h"

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

ter_id grass_or_dirt()
{
 if (one_in(4))
  return t_grass;
 return t_dirt;
}

enum room_type {
 room_null,
 room_closet,
 room_lobby,
 room_chemistry,
 room_teleport,
 room_goo,
 room_cloning,
 room_vivisect,
 room_dorm,
 room_living,
 room_bathroom,
 room_kitchen,
 room_bedroom,
 room_split
};

bool connects_to(oter_id there, int dir_from_here);

void house_room(map *m, room_type type, int x1, int y1, int x2, int y2);

void science_room(map *m, int x1, int y1, int x2, int y2, int rotate);

void set_science_room(map *m, int x1, int y1, bool faces_right, int turn);

void silo_rooms(map *m);

void map::generate(game *g, overmap *om, int x, int y, int turn)
{
 oter_id terrain_type, t_north, t_east, t_south, t_west, t_above;
 int overx = x / 2;
 int overy = y / 2;
 if (x >= OMAPX * 2 || x < 0 || y >= OMAPY * 2 || y < 0) {
// This happens when we're at the very edge of the overmap, and are generating
// terrain for the adjacent overmap.
  int sx = 0, sy = 0;
  overx = (x % (OMAPX * 2)) / 2;
  if (x >= OMAPX * 2)
   sx = 1;
  if (x < 0) {
   sx = -1;
   overx = (OMAPX * 2 + x) / 2;
  }
  overy = (y % (OMAPY * 2)) / 2;
  if (y >= OMAPY * 2)
   sy = 1;
  if (y < 0) {
   overy = (OMAPY * 2 + y) / 2;
   sy = -1;
  }
  overmap tmp(g, om->posx + sx, om->posy + sy, om->posz);
  terrain_type = tmp.ter(overx, overy);
  if (om->posz < 0 || om->posz == 9) {	// 9 is for tutorial overmap
   overmap tmp2 = overmap(g, om->posx, om->posy, om->posz + 1);
   t_above = tmp2.ter(overx, overy);
  } else
   t_above = ot_null;
  if (overy - 1 >= 0)
   t_north = tmp.ter(overx, overy - 1);
  else
   t_north = om->ter(overx, OMAPY - 1);
  if (overx + 1 < OMAPX)
   t_east = tmp.ter(overx + 1, overy - 1);
  else
   t_east = om->ter(0, overy);
  if (overy + 1 < OMAPY)
   t_south = tmp.ter(overx, overy + 1);
  else
   t_south = om->ter(overx, 0);
  if (overx - 1 >= 0)
   t_west = tmp.ter(overx - 1, overy);
  else
   t_west = om->ter(OMAPX - 1, overy);
  draw_map(terrain_type, t_north, t_east, t_south, t_west, t_above, turn);
  for (int i = 0; i < 2; i++) {
   for (int j = 0; j < 2; j++)
    saven(&tmp, turn, overx*2, overy*2, i, j);
  }
 } else {
  if (om->posz < 0 || om->posz == 9) {	// 9 is for tutorials
   overmap tmp = overmap(g, om->posx, om->posy, om->posz + 1);
   t_above = tmp.ter(overx, overy);
  } else
   t_above = ot_null;
  terrain_type = om->ter(overx, overy);
  if (overy - 1 >= 0)
   t_north = om->ter(overx, overy - 1);
  else {
   overmap tmp(g, om->posx, om->posy - 1, 0);
   t_north = tmp.ter(overx, OMAPY - 1);
  }
  if (overx + 1 < OMAPX)
   t_east = om->ter(overx + 1, overy);
  else {
   overmap tmp(g, om->posx + 1, om->posy, 0);
   t_east = tmp.ter(0, overy);
  }
  if (overy + 1 < OMAPY)
   t_south = om->ter(overx, overy + 1);
  else {
   overmap tmp(g, om->posx, om->posy + 1, 0);
   t_south = tmp.ter(overx, 0);
  }
  if (overx - 1 >= 0)
   t_west = om->ter(overx - 1, overy);
  else {
   overmap tmp(g, om->posx - 1, om->posy, 0);
   t_west = tmp.ter(OMAPX - 1, overy);
  }
  draw_map(terrain_type, t_north, t_east, t_south, t_west, t_above, turn);

// And finally save.
  for (int i = 0; i < 2; i++) {
   for (int j = 0; j < 2; j++)
    saven(om, turn, x, y, i, j);
  }
 }
}

void map::draw_map(oter_id terrain_type, oter_id t_north, oter_id t_east,
                   oter_id t_south, oter_id t_west, oter_id t_above, int turn)
{
// Big old switch statement with a case for each overmap terrain type.
// Many of these can be copied from another type, then rotated; for instance,
//  ot_house_east is identical to ot_house_north, just rotated 90 degrees to
//  the right.  The rotate(int) function is at the bottom of this file.

// The place_items() function takes a mapitems type (see mapitems.h and
//  mapitemsdef.cpp), an "odds" int giving the chance for a single item to be
//  placed, four ints (x1, y1, x2, y2) corresponding to the upper left corner
//  and lower right corner of a square where the items are placed, a boolean
//  that indicates whether items may spawn on grass & dirt, and finally an
//  integer that indicates on which turn the items were created.  This final
//  integer should be 0, unless the items are "fresh-grown" like wild fruit.
 int rn, lw, rw, mw, tw, bw, cw, x, y;
 int n_fac = 0, e_fac = 0, s_fac = 0, w_fac = 0;
 switch (terrain_type) {
 case ot_null:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    ter(i, j) = t_null;
    radiation(i, j) = 0;
   }
  }
  break;

 case ot_field:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = grass_or_dirt();
  }
  place_items(mi_field, 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 2, true, turn);
  break;

 case ot_forest:
 case ot_forest_thick:
 case ot_forest_water:
  switch (terrain_type) {
  case ot_forest_thick:
   n_fac = 8;
   e_fac = 8;
   s_fac = 8;
   w_fac = 8;
   break;
  case ot_forest_water:
   n_fac = 4;
   e_fac = 4;
   s_fac = 4;
   w_fac = 4;
   break;
  case ot_forest:
   n_fac = 0;
   e_fac = 0;
   s_fac = 0;
   w_fac = 0;
  }
       if (t_north == ot_forest || t_north == ot_forest_water)
   n_fac += 14;
  else if (t_north == ot_forest_thick)
   n_fac += 18;
       if (t_east == ot_forest || t_east == ot_forest_water)
   e_fac += 14;
  else if (t_east == ot_forest_thick)
   e_fac += 18;
       if (t_south == ot_forest || t_south == ot_forest_water)
   s_fac += 14;
  else if (t_south == ot_forest_thick)
   s_fac += 18;
       if (t_west == ot_forest || t_west == ot_forest_water)
   w_fac += 14;
  else if (t_west == ot_forest_thick)
   w_fac += 18;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    int forest_chance = 0, num = 0;
    if (j < n_fac) {
     forest_chance += n_fac - j;
     num++;
    }
    if (SEEX * 2 - 1 - i < e_fac) {
     forest_chance += e_fac - (SEEX * 2 - 1 - i);
     num++;
    }
    if (SEEY * 2 - 1 - j < s_fac) {
     forest_chance += s_fac - (SEEX * 2 - 1 - j);
     num++;
    }
    if (i < w_fac) {
     forest_chance += w_fac - i;
     num++;
    }
    if (num > 0)
     forest_chance /= num;
    rn = rng(0, forest_chance);
         if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance))
     ter(i, j) = t_tree;
    else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance))
     ter(i, j) = t_tree_young;
    else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance))
     ter(i, j) = t_underbrush;
    else
     ter(i, j) = t_dirt;
   }
  }
  place_items(mi_forest, 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);

  if (terrain_type == ot_forest_water) {
// Reset *_fac to handle where to place water
   if (t_north == ot_forest_water)
    n_fac = 2;
   else if (t_north >= ot_river_center && t_north <= ot_river_nw)
    n_fac = 3;
   else if (t_north == ot_forest || t_north == ot_forest_thick)
    n_fac = 1;
   else
    n_fac = 0;
   if (t_east == ot_forest_water)
    e_fac = 2;
   else if (t_east >= ot_river_center && t_east <= ot_river_nw)
    e_fac = 3;
   else if (t_east == ot_forest || t_east == ot_forest_thick)
    e_fac = 1;
   else
    e_fac = 0;
   if (t_south == ot_forest_water)
    s_fac = 2;
   else if (t_south >= ot_river_center && t_south <= ot_river_nw)
    s_fac = 3;
   else if (t_south == ot_forest || t_south == ot_forest_thick)
    s_fac = 1;
   else
    s_fac = 0;
   if (t_west == ot_forest_water)
    w_fac = 2;
   else if (t_west >= ot_river_center && t_west <= ot_river_nw)
    w_fac = 3;
   else if (t_west == ot_forest || t_west == ot_forest_thick)
    w_fac = 1;
   else
    w_fac = 0;
   x = SEEX / 2 + rng(0, SEEX), y = SEEY / 2 + rng(0, SEEY);
   for (int i = 0; i < 20; i++) {
    if (x >= 0 && x < SEEX * 2 && y >= 0 && y < SEEY * 2) {
     if (ter(x, y) == t_water_sh)
      ter(x, y) = t_water_dp;
     else if (ter(x, y) == t_dirt || ter(x, y) == t_underbrush)
      ter(x, y) = t_water_sh;
    } else
     i = 20;
    x += rng(-2, 2);
    y += rng(-2, 2);
    if (x < 0 || x >= SEEX * 2)
     x = SEEX / 2 + rng(0, SEEX);
    if (y < 0 || y >= SEEY * 2)
     y = SEEY / 2 + rng(0, SEEY);
    for (int j = 0; j < n_fac; j++) {
     int wx = rng(0, SEEX * 2 -1), wy = rng(0, SEEY - 1);
     if (ter(wx, wy) == t_dirt || ter(wx, wy) == t_underbrush)
      ter(wx, wy) = t_water_sh;
    }
    for (int j = 0; j < e_fac; j++) {
     int wx = rng(SEEX, SEEX * 2 - 1), wy = rng(0, SEEY * 2 - 1);
     if (ter(wx, wy) == t_dirt || ter(wx, wy) == t_underbrush)
      ter(wx, wy) = t_water_sh;
    }
    for (int j = 0; j < s_fac; j++) {
     int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
     if (ter(wx, wy) == t_dirt || ter(wx, wy) == t_underbrush)
      ter(wx, wy) = t_water_sh;
    }
    for (int j = 0; j < w_fac; j++) {
     int wx = rng(0, SEEX - 1), wy = rng(0, SEEY * 2 - 1);
     if (ter(wx, wy) == t_dirt || ter(wx, wy) == t_underbrush)
      ter(wx, wy) = t_water_sh;
    }
   }
   rn = rng(0, 2) * rng(0, 1) * (rng(0, 1) + rng(0, 1));// Good chance of 0
   for (int i = 0; i < rn; i++) {
    x = rng(0, SEEX * 2 - 1);
    y = rng(0, SEEY * 2 - 1);
    add_trap(x, y, tr_sinkhole);
    if (ter(x, y) != t_water_sh)
     ter(x, y) = t_dirt;
   }
  }
  break;

 case ot_hive:
 case ot_hive_center:
// Start with a basic forest pattern
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    rn = rng(0, 14);
    if (rn > 13) {
     ter(i, j) = t_tree;
    } else if (rn > 11) {
     ter(i, j) = t_tree_young;
    } else if (rn > 10) {
     ter(i, j) = t_underbrush;
    } else {
     ter(i, j) = t_dirt;
    }
   }
  }
  
  for (int j = 5; j < SEEY * 2; j += 6) {
   for (int i = (j == 5 || j == 17 ? 3 : 6); i < SEEX * 2; i += 6) {
    if (!one_in(8)) {
// Caps are always there
     ter(i    , j - 5) = t_wax;
     ter(i    , j + 5) = t_wax;
     for (int k = -2; k <= 2; k++) {
      for (int l = -1; l <= 1; l++)
       ter(i + k, j + l) = t_floor_wax;
     }
     add_spawn(mon_bee, 2, i, j);
     ter(i    , j - 3) = t_floor_wax;
     ter(i    , j + 3) = t_floor_wax;
     ter(i - 1, j - 2) = t_floor_wax;
     ter(i    , j - 2) = t_floor_wax;
     ter(i + 1, j - 2) = t_floor_wax;
     ter(i - 1, j + 2) = t_floor_wax;
     ter(i    , j + 2) = t_floor_wax;
     ter(i + 1, j + 2) = t_floor_wax;

// Up to two of these get skipped; an entrance to the cell
     int skip1 = rng(0, 23);
     int skip2 = rng(0, 23);

     ter(i - 1, j - 4) = t_wax;
     ter(i    , j - 4) = t_wax;
     ter(i + 1, j - 4) = t_wax;
     ter(i - 2, j - 3) = t_wax;
     ter(i - 1, j - 3) = t_wax;
     ter(i + 1, j - 3) = t_wax;
     ter(i + 2, j - 3) = t_wax;
     ter(i - 3, j - 2) = t_wax;
     ter(i - 2, j - 2) = t_wax;
     ter(i + 2, j - 2) = t_wax;
     ter(i + 3, j - 2) = t_wax;
     ter(i - 3, j - 1) = t_wax;
     ter(i - 3, j    ) = t_wax;
     ter(i - 3, j - 1) = t_wax;
     ter(i - 3, j + 1) = t_wax;
     ter(i - 3, j    ) = t_wax;
     ter(i - 3, j + 1) = t_wax;
     ter(i - 2, j + 3) = t_wax;
     ter(i - 1, j + 3) = t_wax;
     ter(i + 1, j + 3) = t_wax;
     ter(i + 2, j + 3) = t_wax;
     ter(i - 1, j + 4) = t_wax;
     ter(i    , j + 4) = t_wax;
     ter(i + 1, j + 4) = t_wax;

     if (skip1 ==  0 || skip2 ==  0)
      ter(i - 1, j - 4) = t_floor_wax;
     if (skip1 ==  1 || skip2 ==  1)
      ter(i    , j - 4) = t_floor_wax;
     if (skip1 ==  2 || skip2 ==  2)
      ter(i + 1, j - 4) = t_floor_wax;
     if (skip1 ==  3 || skip2 ==  3)
      ter(i - 2, j - 3) = t_floor_wax;
     if (skip1 ==  4 || skip2 ==  4)
      ter(i - 1, j - 3) = t_floor_wax;
     if (skip1 ==  5 || skip2 ==  5)
      ter(i + 1, j - 3) = t_floor_wax;
     if (skip1 ==  6 || skip2 ==  6)
      ter(i + 2, j - 3) = t_floor_wax;
     if (skip1 ==  7 || skip2 ==  7)
      ter(i - 3, j - 2) = t_floor_wax;
     if (skip1 ==  8 || skip2 ==  8)
      ter(i - 2, j - 2) = t_floor_wax;
     if (skip1 ==  9 || skip2 ==  9)
      ter(i + 2, j - 2) = t_floor_wax;
     if (skip1 == 10 || skip2 == 10)
      ter(i + 3, j - 2) = t_floor_wax;
     if (skip1 == 11 || skip2 == 11)
      ter(i - 3, j - 1) = t_floor_wax;
     if (skip1 == 12 || skip2 == 12)
      ter(i - 3, j    ) = t_floor_wax;
     if (skip1 == 13 || skip2 == 13)
      ter(i - 3, j - 1) = t_floor_wax;
     if (skip1 == 14 || skip2 == 14)
      ter(i - 3, j + 1) = t_floor_wax;
     if (skip1 == 15 || skip2 == 15)
      ter(i - 3, j    ) = t_floor_wax;
     if (skip1 == 16 || skip2 == 16)
      ter(i - 3, j + 1) = t_floor_wax;
     if (skip1 == 17 || skip2 == 17)
      ter(i - 2, j + 3) = t_floor_wax;
     if (skip1 == 18 || skip2 == 18)
      ter(i - 1, j + 3) = t_floor_wax;
     if (skip1 == 19 || skip2 == 19)
      ter(i + 1, j + 3) = t_floor_wax;
     if (skip1 == 20 || skip2 == 20)
      ter(i + 2, j + 3) = t_floor_wax;
     if (skip1 == 21 || skip2 == 21)
      ter(i - 1, j + 4) = t_floor_wax;
     if (skip1 == 22 || skip2 == 22)
      ter(i    , j + 4) = t_floor_wax;
     if (skip1 == 23 || skip2 == 23)
      ter(i + 1, j + 4) = t_floor_wax;

     if (terrain_type == ot_hive)
      place_items(mi_hive, 80, i - 2, j - 2, i + 2, j + 2, false, turn);
     else if (terrain_type == ot_hive_center && t_north == ot_hive &&
              t_east == ot_hive && t_south == ot_hive && t_west == ot_hive)
      place_items(mi_hive_center, 90, i - 2, j - 2, i + 2, j + 2, false, turn);
    }
   }
  }
  break;
   

 case ot_road_ns:
 case ot_road_ew:
  if ((t_west  >= ot_house_north && t_west  <= ot_sub_station_west) ||
      (t_east  >= ot_house_north && t_east  <= ot_sub_station_west) ||
      (t_north >= ot_house_north && t_north <= ot_sub_station_west) ||
      (t_south >= ot_house_north && t_south <= ot_sub_station_west)   )
   rn = 1;	// rn = 1 if this road has sidewalks
  else
   rn = 0;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 4 || i >= SEEX * 2 - 4) {
     if (rn == 1)
      ter(i, j) = t_sidewalk;
     else
      ter(i, j) = grass_or_dirt();
    } else {
     if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0)
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (terrain_type == ot_road_ew)
   rotate(1);
  place_items(mi_road, 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  break;

 case ot_road_ne:
 case ot_road_es:
 case ot_road_sw:
 case ot_road_wn:
  if ((t_west  >= ot_house_north && t_west  <= ot_sub_station_west) ||
      (t_east  >= ot_house_north && t_east  <= ot_sub_station_west) ||
      (t_north >= ot_house_north && t_north <= ot_sub_station_west) ||
      (t_south >= ot_house_north && t_south <= ot_sub_station_west)   )
   rn = 1;	// rn = 1 if this road has sidewalks
  else
   rn = 0;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i >= SEEX * 2 - 4 && j < 4) || i < 4 || j >= SEEY * 2 - 4) {
     if (rn == 1)
      ter(i, j) = t_sidewalk;
     else
      ter(i, j) = grass_or_dirt();
    } else {
     if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0 && j < SEEY - 1) ||
         ((j == SEEY - 1 || j == SEEY) && i % 4 != 0 && i > SEEX))
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (terrain_type == ot_road_es)
   rotate(1);
  if (terrain_type == ot_road_sw)
   rotate(2);
  if (terrain_type == ot_road_wn)
   rotate(3);
  place_items(mi_road, 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  break;

 case ot_road_nes:
 case ot_road_new:
 case ot_road_nsw:
 case ot_road_esw:
  if ((t_west  >= ot_house_north && t_west  <= ot_sub_station_west) ||
      (t_east  >= ot_house_north && t_east  <= ot_sub_station_west) ||
      (t_north >= ot_house_north && t_north <= ot_sub_station_west) ||
      (t_south >= ot_house_north && t_south <= ot_sub_station_west)   )
   rn = 1;	// rn = 1 if this road has sidewalks
  else
   rn = 0;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 4 || (i >= SEEX * 2 - 4 && (j < 4 || j >= SEEY * 2 - 4))) {
     if (rn == 1)
      ter(i, j) = t_sidewalk;
     else
      ter(i, j) = grass_or_dirt();
    } else {
     if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0) ||
         ((j == SEEY - 1 || j == SEEY) && i % 4 != 0 && i > SEEX))
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (terrain_type == ot_road_esw)
   rotate(1);
  if (terrain_type == ot_road_nsw)
   rotate(2);
  if (terrain_type == ot_road_new)
   rotate(3);
  place_items(mi_road, 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  break;

 case ot_road_nesw:
 case ot_road_nesw_manhole:
  if ((t_west  == ot_road_nesw || t_west  == ot_road_nesw_manhole) &&
      (t_east  == ot_road_nesw || t_east  == ot_road_nesw_manhole) &&
      (t_north == ot_road_nesw || t_north == ot_road_nesw_manhole) &&
      (t_south == ot_road_nesw || t_south == ot_road_nesw_manhole)   )
   rn = 2;	// rn = 2 if this is actually a plaza
  else
   rn = 1;	// rn = 1 if this road has sidewalks
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (rn == 2)
     ter(i, j) = t_sidewalk;
    else if ((i < 4 || i >= SEEX * 2 - 4) && (j < 4 || j >= SEEY * 2 - 4)) {
     if (rn == 1)
      ter(i, j) = t_sidewalk;
     else
      ter(i, j) = grass_or_dirt();
    } else {
     if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0) ||
         ((j == SEEY - 1 || j == SEEY) && i % 4 != 0))
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (rn == 2) {	// Special embellishments for a plaza
   if (one_in(10)) {	// Fountain
    for (int i = SEEX - 2; i <= SEEX + 2; i++) {
     ter(i, i) = t_water_sh;
     ter(i, SEEX * 2 - i) = t_water_sh;
    }
   }
   if (one_in(10)) {	// Small trees in center
    ter(SEEX - 1, SEEY - 2) = t_tree_young;
    ter(SEEX    , SEEY - 2) = t_tree_young;
    ter(SEEX - 1, SEEY + 2) = t_tree_young;
    ter(SEEX    , SEEY + 2) = t_tree_young;
    ter(SEEX - 2, SEEY - 1) = t_tree_young;
    ter(SEEX - 2, SEEY    ) = t_tree_young;
    ter(SEEX + 2, SEEY - 1) = t_tree_young;
    ter(SEEX + 2, SEEY    ) = t_tree_young;
   }
   if (one_in(14)) {	// Rows of small trees
    int gap = rng(2, 4);
    int start = rng(0, 4);
    for (int i = 2; i < SEEX * 2 - start; i += gap) {
     ter(i               , start) = t_tree_young;
     ter(SEEX * 2 - i - 1, start) = t_tree_young;
     ter(start, i               ) = t_tree_young;
     ter(start, SEEY * 2 - i - 1) = t_tree_young;
    }
   }
   place_items(mi_trash, 5, 0, 0, SEEX * 2 -1, SEEX * 2 - 1, true, 0);
  } else
   place_items(mi_road,  5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  if (terrain_type == ot_road_nesw_manhole)
   ter(rng(6, SEEX * 2 - 6), rng(6, SEEX * 2 - 6)) = t_manhole_cover;
  break;

 case ot_bridge_ns:
 case ot_bridge_ew:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 4 || i >= SEEX * 2 - 4)
     ter(i, j) = t_water_dp;
    else if (i == 4 || i == SEEX * 2 - 5)
     ter(i, j) = t_railing_v;
    else {
     if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0)
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (terrain_type == ot_bridge_ew)
   rotate(1);
  place_items(mi_road, 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  break;
 
 case ot_hiway_ns:
 case ot_hiway_ew:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 3 || i >= SEEX * 2 - 3)
     ter(i, j) = grass_or_dirt();
    else if (i == 3 || i == SEEX * 2 - 4)
     ter(i, j) = t_railing_v;
    else {
     if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0)
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    }
   }
  }
  if (terrain_type == ot_hiway_ew)
   rotate(1);
  place_items(mi_road, 8, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
  break;

 case ot_river_center:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_water_dp;
  }
  break;

 case ot_river_c_not_ne:
 case ot_river_c_not_se:
 case ot_river_c_not_sw:
 case ot_river_c_not_nw:
  for (int i = SEEX * 2 - 1; i >= 0; i--) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j < 4 && i >= SEEX * 2 - 4)
      ter(i, j) = t_water_sh;
    else
     ter(i, j) = t_water_dp;
   }
  }
  if (terrain_type == ot_river_c_not_se)
   rotate(1);
  if (terrain_type == ot_river_c_not_sw)
   rotate(2);
  if (terrain_type == ot_river_c_not_nw)
   rotate(3);
  break;

 case ot_river_north:
 case ot_river_east:
 case ot_river_south:
 case ot_river_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j < 4)
      ter(i, j) = t_water_sh;
    else
     ter(i, j) = t_water_dp;
   }
  }
  if (terrain_type == ot_river_east)
   rotate(1);
  if (terrain_type == ot_river_south)
   rotate(2);
  if (terrain_type == ot_river_west)
   rotate(3);
  break;

 case ot_river_ne:
 case ot_river_se:
 case ot_river_sw:
 case ot_river_nw:
  for (int i = SEEX * 2 - 1; i >= 0; i--) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i >= SEEX * 2 - 4 || j < 4)
     ter(i, j) = t_water_sh;
    else
     ter(i, j) = t_water_dp;
   }
  }
  if (terrain_type == ot_river_se)
   rotate(1);
  if (terrain_type == ot_river_sw)
   rotate(2);
  if (terrain_type == ot_river_nw)
   rotate(3);
  break;

 case ot_house_base_north:
 case ot_house_base_east:
 case ot_house_base_south:
 case ot_house_base_west:
 case ot_house_north:
 case ot_house_east:
 case ot_house_south:
 case ot_house_west:
  lw = rng(0, 4);		// West external wall
  mw = lw + rng(7, 10);		// Middle wall between bedroom & kitchen/bath
  rw = SEEX * 2 - rng(1, 5);	// East external wall
  tw = rng(1, 6);		// North external wall
  bw = SEEX * 2 - rng(2, 5);	// South external wall
  cw = tw + rng(4, 7);		// Middle wall between living room & kitchen/bed
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i > lw && i < rw && j > tw && j < bw)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
    if (i >= lw && i <= rw && (j == tw || j == bw))
     ter(i, j) = t_wall_h;
    if ((i == lw || i == rw) && j > tw && j < bw)
     ter(i, j) = t_wall_v;
   }
  }
  switch(rng(1, 3)) {
  case 1:	// Quadrants, essentially
   mw = rng(lw + 5, rw - 5);
   cw = tw + rng(4, 7);
   house_room(this, room_living,	mw, tw, rw, cw);
   house_room(this, room_kitchen,	lw, tw, mw, cw);
   ter(mw, rng(tw + 2, cw - 2)) = (one_in(3) ? t_door_c : t_floor);
   rn = rng(lw + 1, cw - 2);
   ter(rn    , tw) = t_window;
   ter(rn + 1, tw) = t_window;
   rn = rng(cw + 1, rw - 2);
   ter(rn    , tw) = t_window;
   ter(rn + 1, tw) = t_window;
   mw = rng(lw + 3, rw - 3);
   if (mw <= lw + 5) {	// Bedroom on right, bathroom on left
    rn = rng(cw + 2, rw - 2);
    if (bw - cw >= 10 && mw - lw >= 6) {
     house_room(this, room_bathroom, lw, bw - 5, mw, bw);
     house_room(this, room_bedroom, lw, cw, mw, bw - 5);
     ter(mw - 1, cw) = t_door_c;
    } else {
     if (bw - cw > 4) {	// Too big for a bathroom, not big enough for 2nd bedrm
      house_room(this, room_bathroom, lw, bw - 4, mw, bw);
      for (int i = lw + 1; i <= mw - 1; i++)
       ter(i, cw    ) = t_floor;
     } else
      house_room(this, room_bathroom, lw, cw, mw, bw);
    }
    house_room(this, room_bedroom, mw, cw, rw, bw);
    ter(mw, rng(bw - 4, bw - 1)) = t_door_c;
   } else {	// Bedroom on left, bathroom on right
    rn = rng(lw + 2, cw - 2);
    if (bw - cw >= 10 && rw - mw >= 6) {
     house_room(this, room_bathroom, mw, bw - 5, rw, bw);
     house_room(this, room_bedroom, mw, cw, rw, bw - 5);
     ter(rw - 1, cw) = t_door_c;
    } else {
     if (bw - cw > 4) {	// Too big for a bathroom, not big enough for 2nd bedrm
      house_room(this, room_bathroom, mw, bw - 4, rw, bw);
      for (int i = mw + 1; i <= rw - 1; i++)
       ter(i, cw    ) = t_floor;
     } else
      house_room(this, room_bathroom, mw, cw, rw, bw);
    }
    house_room(this, room_bedroom, lw, cw, mw, bw);
    ter(mw, rng(bw - 4, bw - 1)) = t_door_c;
   }
   ter(rn    , bw) = t_window;
   ter(rn + 1, bw) = t_window;
   if (!one_in(3)) {	// Potential side windows
    rn = rng(tw + 2, bw - 5);
    ter(rw, rn    ) = t_window;
    ter(rw, rn + 4) = t_window;
   }
   if (!one_in(3)) {	// Potential side windows
    rn = rng(tw + 2, bw - 5);
    ter(lw, rn    ) = t_window;
    ter(lw, rn + 4) = t_window;
   }
   ter(rng(lw + 1, lw + 2), cw) = t_door_c;
   if (one_in(4))
    ter(rw - 2, cw) = t_door_c;
   else
    ter(mw, rng(cw + 1, bw - 1)) = t_door_c;
   if (one_in(2)) {	// Placement of the main door
    ter(rng(lw + 2, cw - 1), tw) = (one_in(6) ? t_door_c : t_door_locked);
    if (one_in(5))
     ter(rw, rng(tw + 2, cw - 2)) = (one_in(6) ? t_door_c : t_door_locked);
   } else {
    ter(rng(cw + 1, rw - 2), tw) = (one_in(6) ? t_door_c : t_door_locked);
    if (one_in(5))
     ter(lw, rng(tw + 2, cw - 2)) = (one_in(6) ? t_door_c : t_door_locked);
   }
   break;

  case 2:	// Old-style; simple
   cw = tw + rng(3, 6);
   mw = rng(lw + 7, rw - 4);
// Plop down the rooms
   house_room(this, room_living, lw, tw, rw, cw);
   house_room(this, room_kitchen, mw, cw, rw, bw - 3);
   house_room(this, room_bedroom, lw, cw, mw, bw);
   house_room(this, room_bathroom, mw, bw - 3, rw, bw);
// Space between kitchen & living room:
   rn = rng(mw + 1, rw - 3);
   ter(rn    , cw) = t_floor;
   ter(rn + 1, cw) = t_floor;
// Front windows
   rn = rng(2, 5);
   ter(lw + rn    , tw) = t_window;
   ter(lw + rn + 1, tw) = t_window;
   ter(rw - rn    , tw) = t_window;
   ter(rw - rn + 1, tw) = t_window;
// Front door
   ter(rng(lw + 4, rw - 4), tw) = (one_in(6) ? t_door_c : t_door_locked);
   if (one_in(3)) {	// Kitchen windows
    rn = rng(cw + 1, bw - 5);
    ter(rw, rn    ) = t_window;
    ter(rw, rn + 1) = t_window;
   }
   if (one_in(3)) {	// Bedroom windows
    rn = rng(cw + 1, bw - 2);
    ter(lw, rn    ) = t_window;
    ter(lw, rn + 1) = t_window;
   }
// Door to bedroom
   if (one_in(4))
    ter(rng(lw + 1, mw - 1), cw) = t_door_c;
   else
    ter(mw, rng(cw + 3, bw - 4)) = t_door_c;
// Door to bathrom
   if (one_in(4))
    ter(mw, bw - 1) = t_door_c;
   else
    ter(rng(mw + 2, rw - 2), bw - 3) = t_door_c;
// Back windows
   rn = rng(lw + 1, mw - 2);
   ter(rn    , bw) = t_window;
   ter(rn + 1, bw) = t_window;
   rn = rng(mw + 1, rw - 1);
   ter(rn, bw) = t_window;
   break;

  case 3:	// Long center hallway
   mw = int((lw + rw) / 2);
   cw = bw - rng(5, 7);
// Hallway doors and windows
   ter(mw    , tw) = (one_in(6) ? t_door_c : t_door_locked);
   if (one_in(4)) {
    ter(mw - 1, tw) = t_window;
    ter(mw + 1, tw) = t_window;
   }
   for (int i = tw + 1; i < cw; i++) {	// Hallway walls
    ter(mw - 2, i) = t_wall_v;
    ter(mw + 2, i) = t_wall_v;
   }
   if (one_in(2)) {	// Front rooms are kitchen or living room
    house_room(this, room_living, lw, tw, mw - 2, cw);
    house_room(this, room_kitchen, mw + 2, tw, rw, cw);
   } else {
    house_room(this, room_kitchen, lw, tw, mw - 2, cw);
    house_room(this, room_living, mw + 2, tw, rw, cw);
   }
// Front windows
   rn = rng(lw + 1, mw - 4);
   ter(rn    , tw) = t_window;
   ter(rn + 1, tw) = t_window;
   rn = rng(mw + 3, rw - 2);
   ter(rn    , tw) = t_window;
   ter(rn + 1, tw) = t_window;
   if (one_in(4)) {	// Side windows?
    rn = rng(tw + 1, cw - 2);
    ter(lw, rn    ) = t_window;
    ter(lw, rn + 1) = t_window;
   }
   if (one_in(4)) {	// Side windows?
    rn = rng(tw + 1, cw - 2);
    ter(rw, rn    ) = t_window;
    ter(rw, rn + 1) = t_window;
   }
   if (one_in(2)) {	// Bottom rooms are bedroom or bathroom
    house_room(this, room_bedroom, lw, cw, rw - 3, bw);
    house_room(this, room_bathroom, rw - 3, cw, rw, bw);
    ter(rng(lw + 2, mw - 3), cw) = t_door_c;
    if (one_in(4))
     ter(rng(rw - 2, rw - 1), cw) = t_door_c;
    else
     ter(rw - 3, rng(cw + 2, bw - 2)) = t_door_c;
    rn = rng(lw + 1, rw - 5);
    ter(rn    , bw) = t_window;
    ter(rn + 1, bw) = t_window;
    if (one_in(4))
     ter(rng(rw - 2, rw - 1), bw) = t_window;
    else
     ter(rw, rng(cw + 1, bw - 1));
   } else {
    house_room(this, room_bathroom, lw, cw, lw + 3, bw);
    house_room(this, room_bedroom, lw + 3, cw, rw, bw);
    if (one_in(4))
     ter(rng(lw + 1, lw + 2), bw - 2) = t_door_c;
    else
     ter(lw + 3, rng(cw + 2, bw - 2)) = t_door_c;
    rn = rng(lw + 4, rw - 2);
    ter(rn    , bw) = t_window;
    ter(rn + 1, bw) = t_window;
    if (one_in(4))
     ter(rng(lw + 1, lw + 2), bw) = t_window;
    else
     ter(lw, rng(cw + 1, bw - 1));
   }
// Doors off the sides of the hallway
   ter(mw - 2, rng(tw + 3, cw - 3)) = t_door_c;
   ter(mw + 2, rng(tw + 3, cw - 3)) = t_door_c;
   ter(mw, cw) = t_door_c;
   break;
  }
  if (rng(2, 7) < tw) {	// Big front yard has a chance for a fence
   for (int i = lw; i <= rw; i++)
    ter(i, 0) = t_fence_h;
   for (int i = 1; i < tw; i++) {
    ter(lw, i) = t_fence_v;
    ter(rw, i) = t_fence_v;
   }
   int hole = rng(SEEX - 3, SEEX + 2);
   ter(hole, 0) = t_dirt;
   ter(hole + 1, 0) = t_dirt;
   if (one_in(tw)) {
    ter(hole - 1, 1) = t_tree_young;
    ter(hole + 2, 1) = t_tree_young;
   }
  }
  if (terrain_type >= ot_house_base_north &&
      terrain_type <= ot_house_base_west) {
   do
    rn = rng(lw + 1, rw - 1);
   while (ter(rn, bw - 1) != t_floor);
   ter(rn, bw - 1) = t_stairs_down;
  }
  if (terrain_type == ot_house_east  || terrain_type == ot_house_base_east)
   rotate(1);
  if (terrain_type == ot_house_south || terrain_type == ot_house_base_south)
   rotate(2);
  if (terrain_type == ot_house_west  || terrain_type == ot_house_base_west)
   rotate(3);
  break;

 case ot_s_lot:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEX * 2; j++) {
    if ((j == 5 || j == 9 || j == 13 || j == 17 || j == 21) &&
        ((i > 1 && i < 8) || (i > 14 && i < SEEX * 2 - 2)))
     ter(i, j) = t_pavement_y;
    else if ((j < 2 && i > 7 && i < 17) ||
             (j >= 2 && j < SEEY * 2 - 2 && i > 1 && i < SEEX * 2 - 2))
     ter(i, j) = t_pavement;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  place_items(mi_road, 8, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);
  if (t_east  >= ot_road_null && t_east  <= ot_road_nesw_manhole)
   rotate(1);
  if (t_south >= ot_road_null && t_south <= ot_road_nesw_manhole)
   rotate(2);
  if (t_west  >= ot_road_null && t_west  <= ot_road_nesw_manhole)
   rotate(3);
  break;

 case ot_s_gas_north:
 case ot_s_gas_east:
 case ot_s_gas_south:
 case ot_s_gas_west:
  tw = rng(5, 14);
  bw = SEEY * 2 - rng(1, 2);
  mw = rng(tw + 5, bw - 3);
  if (mw < bw - 5)
   mw = bw - 5;
  lw = rng(0, 3);
  rw = SEEX * 2 - rng(1, 4);
  cw = rng(lw + 4, rw - 5);
  rn = rng(3, 6);	// Frequency of gas pumps
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEX * 2; j++) {
    if (j < tw && (tw - j) % 4 == 0 && i > lw && i < rw &&
        (i - (1 + lw)) % rn == 0)
     ter(i, j) = t_gas_pump;
    else if ((j < 2 && i > 7 && i < 16) || (j < tw && i > lw && i < rw))
     ter(i, j) = t_pavement;
    else if (j == tw && (i == lw+6 || i == lw+7 || i == rw-7 || i == rw-6))
     ter(i, j) = t_window;
    else if (((j == tw || j == bw) && i >= lw && i <= rw) ||
             (j == mw && (i >= cw && i < rw)))
     ter(i, j) = t_wall_h;
    else if (((i == lw || i == rw) && j > tw && j < bw) ||
             (j > mw && j < bw && (i == cw || i == rw - 2)))
     ter(i, j) = t_wall_v;
    else if (i == lw + 1 && j > tw && j < bw)
     ter(i, j) = t_fridge;
    else if (i > lw + 2 && i < lw + 12 && i < cw && i % 2 == 1 &&
             j > tw + 1 && j < mw - 1)
     ter(i, j) = t_rack;
    else if ((i == rw - 5 && j > tw + 1 && j < tw + 4) ||
             (j == tw + 3 && i > rw - 5 && i < rw))
     ter(i, j) = t_counter;
    else if (i > lw && i < rw && j > tw && j < bw)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(cw, rng(mw + 1, bw - 1)) = t_door_c;
  ter(rw - 1, mw) = t_door_c;
  ter(rw - 1, bw - 1) = t_toilet;
  ter(rng(10, 13), tw) = t_door_c;
  if (one_in(5))
   ter(rng(lw + 1, cw - 1), bw) = (one_in(4) ? t_door_c : t_door_locked);
  for (int i = lw + (lw % 2 == 0 ? 3 : 4); i < cw && i < lw + 12; i += 2) {
   if (one_in(2))
    place_items(mi_snacks,	74, i, tw + 2, i, mw - 2, false, 0);
   else
    place_items(mi_magazines,	74, i, tw + 2, i, mw - 2, false, 0);
  }
  place_items(mi_fridgesnacks,	82, lw + 1, tw + 1, lw + 1, bw - 1, false, 0);
  place_items(mi_road,		12, 0,      0,  SEEX*2 - 1, tw - 1, false, 0);
  place_items(mi_behindcounter,	70, rw - 4, tw + 1, rw - 1, tw + 2, false, 0);
  place_items(mi_softdrugs,	12, rw - 1, bw - 2, rw - 1, bw - 2, false, 0);
  if (terrain_type == ot_s_gas_east)
   rotate(1);
  if (terrain_type == ot_s_gas_south)
   rotate(2);
  if (terrain_type == ot_s_gas_west)
   rotate(3);
  break;

 case ot_s_pharm_north:
 case ot_s_pharm_east:
 case ot_s_pharm_south:
 case ot_s_pharm_west:
  tw = rng(0, 4);
  bw = SEEY * 2 - rng(1, 5);
  mw = bw - rng(3, 4);	// Top of the storage room
  lw = rng(0, 4);
  rw = SEEX * 2 - rng(1, 5);
  cw = rng(13, rw - 5);	// Left side of the storage room
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == tw && ((i > lw + 2 && i < lw + 6) || (i > rw - 6 && i < rw - 2)))
     ter(i, j) = t_window;
    else if ((j == tw && (i == lw + 8 || i == lw + 9)) ||
             (i == cw && j == mw + 1))
     ter(i, j) = t_door_c;
    else if (((j == tw || j == bw) && i >= lw && i <= rw) ||
             (j == mw && i >= cw && i < rw))
     ter(i, j) = t_wall_h;
    else if (((i == lw || i == rw) && j > tw && j < bw) ||
             (i == cw && j > mw && j < bw))
     ter(i, j) = t_wall_v;
    else if (((i == lw + 8 || i == lw + 9 || i == rw - 4 || i == rw - 3) &&
              j > tw + 3 && j < mw - 2) ||
             (j == bw - 1 && i > lw + 1 && i < cw - 1))
     ter(i, j) = t_rack;
    else if ((i == lw + 1 && j > tw + 8 && j < mw - 1) ||
             (j == mw - 1 && i > cw + 1 && i < rw))
     ter(i, j) = t_fridge;
    else if ((j == mw     && i > lw + 1 && i < cw) ||
             (j == tw + 6 && i > lw + 1 && i < lw + 6) ||
             (i == lw + 5 && j > tw     && j < tw + 7))
     ter(i, j) = t_counter;
    else if (i > lw && i < rw && j > tw && j < bw)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  if (one_in(3))
   place_items(mi_snacks,	74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
  else if (one_in(4))
   place_items(mi_cleaning,	74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
  else
   place_items(mi_magazines,	74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
  if (one_in(5))
   place_items(mi_softdrugs,	74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
  else if (one_in(4))
   place_items(mi_cleaning,	74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
  else
   place_items(mi_snacks,	74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
  if (one_in(5))
   place_items(mi_softdrugs,	74, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
  else
   place_items(mi_snacks,	74, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
  if (one_in(3))
   place_items(mi_snacks,	70, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
  else
   place_items(mi_softdrugs,	70, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
  place_items(mi_fridgesnacks,	74, lw + 1, tw + 9, lw + 1, mw - 2, false, 0);
  place_items(mi_fridgesnacks,	74, cw + 2, mw - 1, rw - 1, mw - 1, false, 0);
  place_items(mi_harddrugs,	65, lw + 2, bw - 1, cw - 2, bw - 1, false, 0);
  place_items(mi_behindcounter,	78, lw + 1, tw + 1, lw + 4, tw + 5, false, 0);
  if (terrain_type == ot_s_pharm_east)
   rotate(1);
  if (terrain_type == ot_s_pharm_south)
   rotate(2);
  if (terrain_type == ot_s_pharm_west)
   rotate(3);
  break;

 case ot_s_grocery_north:
 case ot_s_grocery_east:
 case ot_s_grocery_south:
 case ot_s_grocery_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 2 && ((i > 4 && i < 8) || (i > 15 && i < 19)))
     ter(i, j) = t_window;
    else if ((j == 2 && (i == 11 || i == 12)) || (i == 6 && j == 20))
     ter(i, j) = t_door_c;
    else if (((j == 2 || j == SEEY * 2 - 3) && i > 1 && i < SEEX * 2 - 2) ||
               (j == 18 && i > 2 && i < 7))
     ter(i, j) = t_wall_h;
    else if (((i == 2 || i == SEEX * 2 - 3) && j > 2 && j < SEEY * 2 - 3) ||
               (i == 6 && j == 19))
     ter(i, j) = t_wall_v;
    else if (j > 4 && j < 8) {
     if (i == 5 || i == 9 || i == 13 || i == 17)
      ter(i, j) = t_counter;
     else if (i == 8 || i == 12 || i == 16 || i == 20)
      ter(i, j) = t_rack;
     else if (i > 2 && i < SEEX * 2 - 3)
      ter(i, j) = t_floor;
     else
      ter(i, j) = grass_or_dirt();
    } else if ((j == 7 && (i == 3 || i == 4)) || 
               ((j == 11 || j == 14) && (i == 18 || i == 19)) ||
               ((j > 9 && j < 16) && (i == 6 || i == 7 || i == 10 ||
                                      i == 11 || i == 14 || i == 15 ||
                                      i == 20)))
     ter(i, j) = t_rack;
    else if ((j == 18 && i > 15 && i < 21) || (j == 19 && i == 16))
     ter(i, j) = t_counter;
    else if ((i == 3 && j > 9 && j < 16) ||
             (j == 20 && ((i > 7 && i < 15) || (i > 18 && i < 21))))
     ter(i, j) = t_fridge;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 3)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  place_items(mi_fridgesnacks,	60,  3, 10,  3, 15, false, 0);
  place_items(mi_fridge,	70,  8, 20, 14, 20, false, 0);
  place_items(mi_fridge,	50, 19, 20, 20, 20, false, 0);
  place_items(mi_softdrugs,	45,  6, 10,  6, 15, false, 0);
  place_items(mi_cleaning,	68,  7, 10,  7, 15, false, 0);
  place_items(mi_kitchen,	55, 10, 10, 10, 15, false, 0);
  place_items(mi_snacks,	75, 11, 10, 11, 15, false, 0);
  place_items(mi_cannedfood,	70, 14, 10, 14, 15, false, 0);
  place_items(mi_pasta,		74, 15, 10, 15, 15, false, 0);
  place_items(mi_produce,	60, 20, 10, 20, 15, false, 0);
  place_items(mi_produce,	50, 18, 11, 19, 11, false, 0);
  place_items(mi_produce,	50, 18, 10, 20, 15, false, 0);
  for (int i = 8; i < 21; i +=4) {	// Checkout snacks & magazines
   place_items(mi_snacks, 50, i, 5, i, 6, false, 0);
   place_items(mi_magazines, 70, i, 7, i, 7, false, 0);
  }
  if (terrain_type == ot_s_grocery_east)
   rotate(1);
  if (terrain_type == ot_s_grocery_south)
   rotate(2);
  if (terrain_type == ot_s_grocery_west)
   rotate(3);
  break;

 case ot_s_hardware_north:
 case ot_s_hardware_east:
 case ot_s_hardware_south:
 case ot_s_hardware_west:
  rn = 0;	// No back door
  if (!one_in(3))
   rn = 1;	// Old-style back door
  else if (one_in(3))
   rn = 2;	// Paved back area
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 3 && ((i > 5 && i < 9) || (i > 14 && i < 18)))
     ter(i, j) = t_window;
    else if ((j == 3 && i > 1 && i < SEEX * 2 - 2) || 
             (j == 15 && i > 1 && i < 14) ||
             (j == SEEY * 2 - 3 && i > 12 && i < SEEX * 2 - 2))
     ter(i, j) = t_wall_h;
    else if ((i == 2 && j > 3 && j < 15) ||
             (i == SEEX * 2 - 3 && j > 3 && j < SEEY * 2 - 3) ||
             (i == 13 && j > 15 && j < SEEY * 2 - 3))
     ter(i, j) = t_wall_v;
    else if ((i > 3 && i < 10 && j == 6) || (i == 9 && j > 3 && j < 7))
     ter(i, j) = t_counter;
    else if (((i == 3 || i == 6 || i == 7 || i == 10 || i == 11) &&
               j > 8 && j < 15) ||
              (i == SEEX * 2 - 4 && j > 3 && j < SEEX * 2 - 4) ||
              (i > 14 && i < 18 && 
               (j == 8 || j == 9 || j == 12 || j == 13)) ||
              (j == SEEY * 2 - 4 && i > 13 && i < SEEX * 2 - 4) ||
              (i > 15 && i < 18 && j > 15 && j < 18) ||
              (i == 9 && j == 7))
     ter(i, j) = t_rack;
    else if ((i > 2 && i < SEEX * 2 - 3 && j > 3 && j < 15) ||
             (i > 13 && i < SEEX * 2 - 3 && j > 14 && j < SEEY * 2 - 3))
     ter(i, j) = t_floor;
    else if (rn == 2 && i > 1 && i < 13 && j > 15 && j < SEEY * 2 - 3)
     ter(i, j) = t_pavement;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(rng(10, 13), 3) = t_door_c;
  if (rn > 0)
   ter(13, rng(16, 19)) = (one_in(3) ? t_door_c : t_door_locked);
  if (rn == 2) {
   if (one_in(5))
    ter(rng(4, 10), 16) = t_gas_pump;
   if (one_in(3)) {	// Place a dumpster
    int startx = rng(2, 11), starty = rng(18, 19);
    if (startx == 11)
     starty = 18;
    bool hori = (starty == 18 ? false : true);
    for (int i = startx; i <= startx + (hori ? 3 : 2); i++) {
     for (int j = starty; j <= starty + (hori ? 2 : 3); j++)
      ter(i, j) = t_dumpster;
    }
    if (hori)
     place_items(mi_trash, 30, startx, starty, startx+3, starty+2, false, 0);
    else
     place_items(mi_trash, 30, startx, starty, startx+2, starty+3, false, 0);
   }
   place_items(mi_road, 30, 2, 16, 12, SEEY * 2 - 3, false, 0);
  }

  place_items(mi_magazines,	70,  9,  7,  9,  7, false, 0);
  if (one_in(4))
   place_items(mi_snacks,	70,  9,  7,  9,  7, false, 0);

  if (!one_in(3))
   place_items(mi_hardware,	80,  3,  9,  3, 14, false, 0);
  else if (!one_in(3))
   place_items(mi_tools,	80,  3,  9,  3, 14, false, 0);
  else
   place_items(mi_bigtools,	80,  3,  9,  3, 14, false, 0);

  if (!one_in(3))
   place_items(mi_hardware,	80,  6,  9,  6, 14, false, 0);
  else if (!one_in(3))
   place_items(mi_tools,	80,  6,  9,  6, 14, false, 0);
  else
   place_items(mi_bigtools,	80,  6,  9,  6, 14, false, 0);

  if (!one_in(4))
   place_items(mi_tools,	80,  7,  9,  7, 14, false, 0);
  else if (one_in(4))
   place_items(mi_mischw,	80,  7,  9,  7, 14, false, 0);
  else
   place_items(mi_hardware,	80,  7,  9,  7, 14, false, 0);
  if (!one_in(4))
   place_items(mi_tools,	80, 10,  9, 10, 14, false, 0);
  else if (one_in(4))
   place_items(mi_mischw,	80, 10,  9, 10, 14, false, 0);
  else
   place_items(mi_hardware,	80, 10,  9, 10, 14, false, 0);
   
  if (!one_in(3))
   place_items(mi_bigtools,	75, 11,  9, 11, 14, false, 0);
  else if (one_in(2))
   place_items(mi_cleaning,	75, 11,  9, 11, 14, false, 0);
  else
   place_items(mi_tools,	75, 11,  9, 11, 14, false, 0);
  if (one_in(2))
   place_items(mi_cleaning,	65, 15,  8, 17,  8, false, 0);
  else
   place_items(mi_snacks,	65, 15,  8, 17,  8, false, 0);
  if (one_in(4))
   place_items(mi_hardware,	74, 15,  9, 17,  9, false, 0);
  else
   place_items(mi_cleaning,	74, 15,  9, 17,  9, false, 0);
  if (one_in(4))
   place_items(mi_hardware,	74, 15, 12, 17, 12, false, 0);
  else
   place_items(mi_cleaning,	74, 15, 12, 17, 12, false, 0);
  place_items(mi_mischw,	90, 20,  4, 20, 19, false, 0);
  if (terrain_type == ot_s_hardware_east)
   rotate(1);
  if (terrain_type == ot_s_hardware_south)
   rotate(2);
  if (terrain_type == ot_s_hardware_west)
   rotate(3);
  break;

 case ot_s_sports_north:
 case ot_s_sports_east:
 case ot_s_sports_south:
 case ot_s_sports_west:
  lw = rng(0, 3);
  rw = SEEX * 2 - 1 - rng(0, 3);
  tw = rng(3, 10);
  bw = SEEY * 2 - 1 - rng(0, 3);
  cw = bw - rng(3, 5);
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (((j == tw || j == bw) && i >= lw && i <= rw) ||
        (j == cw && i > lw && i < rw))
     ter(i, j) = t_wall_h;
    else if ((i == lw || i == rw) && j > tw && j < bw)
     ter(i, j) = t_wall_v;
    else if ((j == cw - 1 && i > lw && i < rw - 4) ||
             (j < cw - 3 && j > tw && (i == lw + 1 || i == rw - 1)))
     ter(i, j) = t_rack;
    else if (j == cw - 3 && i > lw && i < rw - 4)
     ter(i, j) = t_counter;
    else if (j > tw && j < bw && i > lw && i < rw)
     ter(i, j) = t_floor;
    else if (tw >= 6 && j >= tw - 6 && j < tw && i >= lw && i <= rw) {
     if ((i - lw) % 4 == 0)
      ter(i, j) = t_pavement_y;
     else
      ter(i, j) = t_pavement;
    } else
     ter(i, j) = grass_or_dirt();
   }
  }
  rn = rng(tw + 2, cw - 6);
  for (int i = lw + 3; i <= rw - 5; i += 4) {
   if (cw - 6 > tw + 1) {
    ter(i    , rn + 1) = t_rack;
    ter(i    , rn    ) = t_rack;
    ter(i + 1, rn + 1) = t_rack;
    ter(i + 1, rn    ) = t_rack;
    place_items(mi_camping,	86, i, rn, i + 1, rn + 1, false, 0);
   } else if (cw - 5 > tw + 1) {
    ter(i    , cw - 5) = t_rack;
    ter(i + 1, cw - 5) = t_rack;
    place_items(mi_camping,	80, i, cw - 5, i + 1, cw - 5, false, 0);
   }
  }
  ter(rw - rng(2, 3), cw) = t_door_c;
  rn = rng(2, 4);
  for (int i = lw + 2; i <= lw + 2 + rn; i++)
   ter(i, tw) = t_window;
  for (int i = rw - 2; i >= rw - 2 - rn; i--)
   ter(i, tw) = t_window;
  ter(rng(lw + 3 + rn, rw - 3 - rn), tw) = t_door_c;
  if (one_in(4))
   ter(rng(lw + 2, rw - 2), bw) = t_door_locked;
  place_items(mi_allsporting,	90, lw + 1, cw - 1, rw - 5, cw - 1, false, 0);
  place_items(mi_sports,	82, lw + 1, tw + 1, lw + 1, cw - 4, false, 0);
  place_items(mi_sports,	82, rw - 1, tw + 1, rw - 1, cw - 4, false, 0);
  if (!one_in(4))
   place_items(mi_allsporting,	92, lw + 1, cw + 1, rw - 1, bw - 1, false, 0);
  
  if (terrain_type == ot_s_sports_east)
   rotate(1);
  if (terrain_type == ot_s_sports_south)
   rotate(2);
  if (terrain_type == ot_s_sports_west)
   rotate(3);
  break;

 case ot_s_liquor_north:
 case ot_s_liquor_east:
 case ot_s_liquor_south:
 case ot_s_liquor_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 2 && (i == 5 || i == 18))
     ter(i, j) = t_window;
    else if (((j == 2 || j == 12) && i > 2 && i < SEEX * 2 - 3) ||
             (j == 9 && i > 3 && i < 8))
     ter(i, j) = t_wall_h;
    else if (((i == 3 || i == SEEX * 2 - 4) && j > 2 && j < 12) ||
             (i == 7 && j > 9 && j < 12))
     ter(i, j) = t_wall_v;
    else if ((i == 19 && j > 6 && j < 12) || (j == 11 && i > 16 && i < 19))
     ter(i, j) = t_fridge;
    else if (((i == 4 || i == 7 || i == 8) && j > 2 && j < 8) ||
             (j == 3 && i > 8 && i < 12) ||
             (i > 10 && i < 13 && j > 4 && j < 7) ||
             (i > 10 && i < 16 && j > 7 && j < 10))
     ter(i, j) = t_rack;
    else if ((i == 16 && j > 2 && j < 6) || (j == 5 && i > 16 && i < 19))
     ter(i, j) = t_counter;
    else if ((i > 4 && i < 8 && j > 12 && j < 15) ||
             (i > 17 && i < 20 && j > 14 && j < 18))
     ter(i, j) = t_dumpster;
    else if (i > 2 && i < SEEX * 2 - 3) {
     if (j > 2 && j < 12)
      ter(i, j) = t_floor;
     else if (j > 12 && j < SEEY * 2 - 1)
      ter(i, j) = t_pavement;
     else
      ter(i, j) = grass_or_dirt();
    } else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(rng(13, 15), 2) = t_door_c;
  ter(rng(4, 6), 9) = t_door_c;
  ter(rng(9, 16), 12) = t_door_c;

  place_items(mi_alcohol,	96,  4,  3,  4,  7, false, 0);
  place_items(mi_alcohol,	96,  7,  3, 11,  3, false, 0);
  place_items(mi_alcohol,	96,  7,  4,  8,  7, false, 0);
  place_items(mi_alcohol,	96, 11,  8, 15,  9, false, 0);
  place_items(mi_snacks,	85, 11,  5, 12,  6, false, 0);
  place_items(mi_fridgesnacks,	90, 19,  7, 19, 10, false, 0);
  place_items(mi_fridgesnacks,	90, 17, 11, 19, 11, false, 0);
  place_items(mi_behindcounter,	80, 17,  3, 19,  4, false, 0);
  place_items(mi_trash,		30,  5, 14,  7, 14, false, 0);
  place_items(mi_trash,		30, 18, 15, 18, 17, false, 0);
  
  if (terrain_type == ot_s_liquor_east)
   rotate(1);
  if (terrain_type == ot_s_liquor_south)
   rotate(2);
  if (terrain_type == ot_s_liquor_west)
   rotate(3);
  break;

 case ot_s_gun_north:
 case ot_s_gun_east:
 case ot_s_gun_south:
 case ot_s_gun_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i == 2 || i == SEEX * 2 - 3) && j > 6 && j < SEEY * 2 - 1)
     ter(i, j) = t_wall_v;
    else if ((i == 8 && j > 6 && j < 13) ||
             (j == 16 && (i == 5 || i == 8 || i == 11 || i == 14 || i == 17)))
     ter(i, j) = t_counter;
    else if ((j == 6 && ((i > 4 && i < 8) || (i > 15 && i < 19))))
     ter(i, j) = t_window;
    else if ((j == 14 && i > 3 && i < 15))
     ter(i, j) = t_wall_glass_h;
    else if (j == 16 && i == SEEX * 2 - 4)
     ter(i, j) = t_door_c;
    else if (((j == 6 || j == SEEY * 2 - 1) && i > 1 && i < SEEX * 2 - 2) ||
             ((j == 16 || j == 14) && i > 2 && i < SEEX * 2 - 3))
     ter(i, j) = t_wall_h;
    else if (((i == 3 || i == SEEX * 2 - 4) && j > 6 && j < 14) ||
             ((j > 8 && j < 12) && (i == 12 || i == 13 || i == 16)) ||
             (j == 13 && i > 15 && i < SEEX * 2 - 4))
     ter(i, j) = t_rack;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 6 && j < SEEY * 2 - 1)
     ter(i, j) = t_floor;
    else if ((j > 0 && j < 6 &&
             (i == 2 || i == 6 || i == 10 || i == 17 || i == SEEX * 2 - 3)))
     ter(i, j) = t_pavement_y;
    else if (j < 6 && i > 1 && i < SEEX * 2 - 2)
     ter(i, j) = t_pavement;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(rng(11, 14), 6) = t_door_c;
  ter(rng(5, 14), 14) = t_door_c;
  place_items(mi_pistols,	70, 12,  9, 13, 11, false, 0);
  place_items(mi_shotguns,	60, 16,  9, 16, 11, false, 0);
  place_items(mi_rifles,	80, 20,  7, 20, 12, false, 0);
  place_items(mi_smg,		25,  3,  7,  3,  8, false, 0);
  place_items(mi_assault,	18,  3,  9,  3, 10, false, 0);
  place_items(mi_ammo,		93,  3, 11,  3, 13, false, 0);
  place_items(mi_allguns,	12,  5, 16, 17, 16, false, 0);
  place_items(mi_gunxtras,	67, 16, 13, 19, 13, false, 0);
  if (terrain_type == ot_s_gun_east)
   rotate(1);
  if (terrain_type == ot_s_gun_south)
   rotate(2);
  if (terrain_type == ot_s_gun_west)
   rotate(3);
  break;

 case ot_s_clothes_north:
 case ot_s_clothes_east:
 case ot_s_clothes_south:
 case ot_s_clothes_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 2 && (i == 11 || i == 12))
     ter(i, j) = t_door_c;
    else if (j == 2 && i > 3 && i < SEEX * 2 - 4)
     ter(i, j) = t_wall_glass_h;
    else if (((j == 2 || j == SEEY * 2 - 2) && i > 1 && i < SEEX * 2 - 2) ||
             (j == 4 && i > 12 && i < SEEX * 2 - 3) ||
             (j == 17 && i > 2 && i < 12) ||
             (j == 20 && i > 2 && i < 11))
     ter(i, j) = t_wall_h;
    else if (((i == 2 || i == SEEX * 2 - 3) && j > 1 && j < SEEY * 2 - 1) ||
             (i == 11 && (j == 18 || j == 20 || j == 21)) ||
             (j == 21 && (i == 5 || i == 8)))
     ter(i, j) = t_wall_v;
    else if ((i == 16 && j > 4 && j < 9) ||
             (j == 8 && (i == 17 || i == 18)) ||
             (j == 18 && i > 2 && i < 11))
     ter(i, j) = t_counter;
    else if ((i == 3 && j > 4 && j < 13) ||
             (i == SEEX * 2 - 4 && j > 9 && j < 20) ||
             ((j == 10 || j == 11) && i > 6 && i < 13) ||
             ((j == 14 || j == 15) && i > 4 && i < 13) ||
             ((i == 15 || i == 16) && j > 10 && j < 18) ||
             (j == SEEY * 2 - 3 && i > 11 && i < 18))
     ter(i, j) = t_rack;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 2)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }

  for (int i = 3; i <= 9; i += 3) {
   if (one_in(2))
    ter(i, SEEY * 2 - 4) = t_door_c;
   else
    ter(i + 1, SEEY * 2 - 4) = t_door_c;
  }

  place_items(mi_shoes,		70,  7, 10, 12, 10, false, 0);
  place_items(mi_pants,		88,  5, 14, 12, 14, false, 0);
  place_items(mi_shirts,	88,  7, 11, 12, 11, false, 0);
  place_items(mi_jackets,	80,  3,  5,  3, 12, false, 0);
  place_items(mi_winter,	60,  5, 15, 12, 15, false, 0);
  place_items(mi_bags,		70, 15, 11, 15, 17, false, 0);
  place_items(mi_dresser,	50, 12, 21, 17, 21, false, 0);
  place_items(mi_allclothes,	20,  3, 21, 10, 21, false, 0);
  place_items(mi_allclothes,	20,  3, 18, 10, 18, false, 0);
  switch (rng(0, 2)) {
   case 0:
    place_items(mi_pants,	70, 16, 11, 16, 17, false, 0);
    break;
   case 1:
    place_items(mi_shirts,	70, 16, 11, 16, 17, false, 0);
    break;
   case 2:
    place_items(mi_bags,	70, 16, 11, 16, 17, false, 0);
    break;
  }
  switch (rng(0, 2)) {
   case 0:
    place_items(mi_pants,	75, 20, 10, 20, 19, false, 0);
    break;
   case 1:
    place_items(mi_shirts,	75, 20, 10, 20, 19, false, 0);
    break;
   case 2:
    place_items(mi_jackets,	75, 20, 10, 20, 19, false, 0);
    break;
  }
  
  if (terrain_type == ot_s_clothes_east)
   rotate(1);
  if (terrain_type == ot_s_clothes_south)
   rotate(2);
  if (terrain_type == ot_s_clothes_west)
   rotate(3);
  break;

 case ot_s_library_north:
 case ot_s_library_east:
 case ot_s_library_south:
 case ot_s_library_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 2) {
     if (i == 5 || i == 6 || i == 17 || i == 18)
      ter(i, j) = t_window;
     else if (i == 11 || i == 12)
      ter(i, j) = t_door_c;
     else if (i > 1 && i < SEEX * 2 - 2)
      ter(i, j) = t_wall_h;
     else
      ter(i, j) = grass_or_dirt();
    } else if (j == 17 && i > 1 && i < SEEX * 2 - 2)
      ter(i, j) = t_wall_h;
    else if (i == 2) {
     if (j == 6 || j == 7 || j == 10 || j == 11 || j == 14 || j == 15)
      ter(i, j) = t_window;
     else if (j > 1 && j < 17)
      ter(i, j) = t_wall_v;
     else
      ter(i, j) = grass_or_dirt();
    } else if (i == SEEX * 2 - 3) {
     if (j == 6 || j == 7)
      ter(i, j) = t_window;
     else if (j > 1 && j < 17)
      ter(i, j) = t_wall_v;
     else
      ter(i, j) = grass_or_dirt();
    } else if (((j == 4 || j == 5) && i > 2 && i < 10) ||
               ((j == 8 || j == 9 || j == 12 || j == 13 || j == 16) &&
                i > 2 && i < 16) || (i == 20 && j > 7 && j < 17))
     ter(i, j) = t_bookcase;
    else if ((i == 14 && j < 6 && j > 2) || (j == 5 && i > 14 && i < 19))
     ter(i, j) = t_counter;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < 17)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  if (!one_in(3))
   ter(18, 17) = t_door_c;
  place_items(mi_magazines, 	70,  3,  4,  9,  4, false, 0);
  place_items(mi_magazines,	70, 20,  8, 20, 16, false, 0);
  place_items(mi_novels, 	96,  3,  5,  9,  5, false, 0);
  place_items(mi_novels,	96,  3,  8, 15,  9, false, 0);
  place_items(mi_manuals,	92,  3, 12, 15, 13, false, 0);
  place_items(mi_textbooks,	88,  3, 16, 15, 16, false, 0);
  if (terrain_type == ot_s_library_east)
   rotate(1);
  if (terrain_type == ot_s_library_south)
   rotate(2);
  if (terrain_type == ot_s_library_west)
   rotate(3);
  break;

 case ot_lab:
 case ot_lab_stairs:
 case ot_lab_core:
// Check for adjacent sewers; used below
  tw = 0;
  rw = 0;
  bw = 0;
  lw = 0;
  if (t_north>=ot_sewer_ns && t_north<=ot_sewer_nesw && connects_to(t_north,2))
   tw = SEEY * 2;
  if (t_east >=ot_sewer_ns && t_east <=ot_sewer_nesw && connects_to(t_east, 3))
   rw = SEEX * 2;
  if (t_south>=ot_sewer_ns && t_south<=ot_sewer_nesw && connects_to(t_south,0))
   bw = SEEY * 2;
  if (t_west >=ot_sewer_ns && t_west <=ot_sewer_nesw && connects_to(t_west, 1))
   lw = SEEX * 2;
  if (t_above == ot_null) {	// We're on ground level
   for (int i = 0; i < SEEX * 2; i++) {
    for (int j = 0; j < SEEY * 2; j++) {
     if (i <= 1 || i >= SEEX * 2 - 2 ||
         (j > 1 && j < SEEY * 2 - 2 && (i == SEEX - 2 || i == SEEX + 1)))
      ter(i, j) = t_wall_metal_v;
     else if (j <= 1 || j >= SEEY * 2 - 2)
      ter(i, j) = t_wall_metal_h;
     else
      ter(i, j) = t_floor;
    }
   }
   ter(SEEX - 1, 0) = t_dirt;
   ter(SEEX - 1, 1) = t_door_metal_locked;
   ter(SEEX    , 0) = t_dirt;
   ter(SEEX    , 1) = t_door_metal_locked;
   ter(SEEX - 2 + rng(0, 1) * 4, 0) = t_card_reader;
   ter(SEEX - 2, SEEY    ) = t_door_metal_c;
   ter(SEEX + 1, SEEY    ) = t_door_metal_c;
   ter(SEEX - 2, SEEY - 1) = t_door_metal_c;
   ter(SEEX + 1, SEEY - 1) = t_door_metal_c;
   ter(SEEX - 1, SEEY * 2 - 3) = t_stairs_down;
   ter(SEEX    , SEEY * 2 - 3) = t_stairs_down;
   science_room(this, 2       , 2, SEEX - 3    , SEEY * 2 - 3, 1);
   science_room(this, SEEX + 2, 2, SEEX * 2 - 3, SEEY * 2 - 3, 3);

   add_spawn(mon_turret, 1, SEEX, 5);

   if (t_east > ot_road_null && t_east <= ot_road_nesw_manhole)
    rotate(1);
   else if (t_south > ot_road_null && t_south <= ot_road_nesw_manhole)
    rotate(2);
   else if (t_west > ot_road_null && t_west <= ot_road_nesw_manhole)
    rotate(3);
  } else if (tw != 0 || rw != 0 || lw != 0 || bw != 0) {	// Sewers!
   for (int i = 0; i < SEEX * 2; i++) {
    for (int j = 0; j < SEEY * 2; j++) {
     ter(i, j) = t_floor;
     if (((i < lw || i > SEEX * 2 - 1 - rw) && j > SEEY - 3 && j < SEEY + 2) ||
         ((j < tw || j > SEEY * 2 - 1 - bw) && i > SEEX - 3 && i < SEEX + 2))
      ter(i, j) = t_sewage;
     if ((i == 0 && t_east >= ot_lab && t_east <= ot_lab_core) ||
         i == SEEX * 2 - 1) {
      if (ter(i, j) == t_sewage)
       ter(i, j) = t_bars;
      else if (j == SEEY - 1 || j == SEEY)
       ter(i, j) = t_door_metal_c;
      else
       ter(i, j) = t_wall_v;
     } else if ((j == 0 && t_north >= ot_lab && t_north <= ot_lab_core) ||
                j == SEEY * 2 - 1) {
      if (ter(i, j) == t_sewage)
       ter(i, j) = t_bars;
      else if (i == SEEX - 1 || i == SEEX)
       ter(i, j) = t_door_metal_c;
      else
       ter(i, j) = t_wall_h;
     }
    }
   }
  } else {	// We're below ground, and no sewers
// Set up the boudaries of walls (connect to adjacent lab squares)
   tw = (t_north >= ot_lab && t_north <= ot_lab_finale) ? 0 : 2;
   rw = (t_east  >= ot_lab && t_east  <= ot_lab_finale) ? 1 : 2;
   bw = (t_south >= ot_lab && t_south <= ot_lab_finale) ? 1 : 2;
   lw = (t_west  >= ot_lab && t_west  <= ot_lab_finale) ? 0 : 2;
   switch (rng(1, 3)) {	// Pick a random lab layout
   case 1:	// Cross shaped
    for (int i = 0; i < SEEX * 2; i++) {
     for (int j = 0; j < SEEY * 2; j++) {
      if ((i < lw || i > SEEX * 2 - 1 - rw) ||
          ((j < SEEY - 1 || j > SEEY) && (i == SEEX - 2 || i == SEEX + 1)))
       ter(i, j) = t_wall_v;
      else if ((j < tw || j > SEEY * 2 - 1 - bw) ||
               ((i < SEEX - 1 || i > SEEX) && (j == SEEY - 2 || j == SEEY + 1)))
       ter(i, j) = t_wall_h;
      else
       ter(i, j) = t_floor;
     }
    }
    if (t_above == ot_lab_stairs)
     ter(rng(SEEX - 1, SEEX), rng(SEEY - 1, SEEY)) = t_stairs_up;
// Top left
    if (one_in(2)) {
     ter(SEEX - 2, int(SEEY / 2)) = t_door_metal_c;
     science_room(this, lw, tw, SEEX - 3, SEEY - 3, 1);
    } else {
     ter(int(SEEX / 2), SEEY - 2) = t_door_metal_c;
     science_room(this, lw, tw, SEEX - 3, SEEY - 3, 2);
    }
// Top right
    if (one_in(2)) {
     ter(SEEX + 1, int(SEEY / 2)) = t_door_metal_c;
     science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, 3);
    } else {
     ter(SEEX + int(SEEX / 2), SEEY - 2) = t_door_metal_c;
     science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, 2);
    }
// Bottom left
    if (one_in(2)) {
     ter(int(SEEX / 2), SEEY + 1) = t_door_metal_c;
     science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, 0);
    } else {
     ter(SEEX - 2, SEEY + int(SEEY / 2)) = t_door_metal_c;
     science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, 1);
    }
// Bottom right
    if (one_in(2)) {
     ter(SEEX + int(SEEX / 2), SEEY + 1) = t_door_metal_c;
     science_room(this, SEEX +2, SEEY + 2, SEEX*2 - 1 - rw, SEEY*2 - 1 - bw, 0);
    } else {
     ter(SEEX + 1, SEEY + int(SEEY / 2)) = t_door_metal_c;
     science_room(this, SEEX +2, SEEY + 2, SEEX*2 - 1 - rw, SEEY*2 - 1 - bw, 3);
    }
    if (rw == 1) {
     ter(SEEX * 2 - 1, SEEY - 1) = t_door_metal_c;
     ter(SEEX * 2 - 1, SEEY    ) = t_door_metal_c;
    }
    if (bw == 1) {
     ter(SEEX - 1, SEEY * 2 - 1) = t_door_metal_c;
     ter(SEEX    , SEEY * 2 - 1) = t_door_metal_c;
    }
    if (terrain_type == ot_lab_stairs) {	// Stairs going down
     std::vector<point> stair_points;
     if (tw != 0) {
      stair_points.push_back(point(SEEX - 1, 2));
      stair_points.push_back(point(SEEX - 1, 2));
      stair_points.push_back(point(SEEX    , 2));
      stair_points.push_back(point(SEEX    , 2));
     }
     if (rw != 1) {
      stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
      stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
      stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
      stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
     }
     if (bw != 1) {
      stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
      stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
      stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
      stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
     }
     if (lw != 0) {
      stair_points.push_back(point(2, SEEY - 1));
      stair_points.push_back(point(2, SEEY - 1));
      stair_points.push_back(point(2, SEEY    ));
      stair_points.push_back(point(2, SEEY    ));
     }
     stair_points.push_back(point(int(SEEX / 2)       , SEEY    ));
     stair_points.push_back(point(int(SEEX / 2)       , SEEY - 1));
     stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY    ));
     stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY - 1));
     stair_points.push_back(point(SEEX    , int(SEEY / 2)       ));
     stair_points.push_back(point(SEEX + 2, int(SEEY / 2)       ));
     stair_points.push_back(point(SEEX    , int(SEEY / 2) + SEEY));
     stair_points.push_back(point(SEEX + 2, int(SEEY / 2) + SEEY));
     rn = rng(0, stair_points.size() - 1);
     ter(stair_points[rn].x, stair_points[rn].y) = t_stairs_down;
    }
      
    break;

   case 2:	// tic-tac-toe # layout
    for (int i = 0; i < SEEX * 2; i++) {
     for (int j = 0; j < SEEY * 2; j++) {
      if (i < lw || i > SEEX * 2 - 1 - rw || i == SEEX - 4 || i == SEEX + 3)
       ter(i, j) = t_wall_v;
      else if (j < lw || j > SEEY*2 - 1 - bw || j == SEEY - 4 || j == SEEY + 3)
       ter(i, j) = t_wall_h;
      else
       ter(i, j) = t_floor;
     }
    }
    if (t_above == ot_lab_stairs) {
     ter(SEEX - 1, SEEY - 1) = t_stairs_up;
     ter(SEEX    , SEEY - 1) = t_stairs_up;
     ter(SEEX - 1, SEEY    ) = t_stairs_up;
     ter(SEEX    , SEEY    ) = t_stairs_up;
    }
    ter(SEEX - rng(0, 1), SEEY - 4) = t_door_metal_c;
    ter(SEEX - rng(0, 1), SEEY + 3) = t_door_metal_c;
    ter(SEEX - 4, SEEY + rng(0, 1)) = t_door_metal_c;
    ter(SEEX + 3, SEEY + rng(0, 1)) = t_door_metal_c;
    ter(SEEX - 4, int(SEEY / 2)) = t_door_metal_c;
    ter(SEEX + 3, int(SEEY / 2)) = t_door_metal_c;
    ter(int(SEEX / 2), SEEY - 4) = t_door_metal_c;
    ter(int(SEEX / 2), SEEY + 3) = t_door_metal_c;
    ter(SEEX + int(SEEX / 2), SEEY - 4) = t_door_metal_c;
    ter(SEEX + int(SEEX / 2), SEEY + 3) = t_door_metal_c;
    ter(SEEX - 4, SEEY + int(SEEY / 2)) = t_door_metal_c;
    ter(SEEX + 3, SEEY + int(SEEY / 2)) = t_door_metal_c;
    science_room(this, lw, tw, SEEX - 5, SEEY - 5, rng(1, 2));
    science_room(this, SEEX - 3, tw, SEEX + 2, SEEY - 5, 2);
    science_room(this, SEEX + 4, tw, SEEX * 2 - 1 - rw, SEEY - 5, rng(2, 3));
    science_room(this, lw, SEEY - 3, SEEX - 5, SEEY + 2, 1);
    science_room(this, SEEX + 4, SEEY - 3, SEEX * 2 - 1 - rw, SEEY + 2, 3);
    science_room(this, lw, SEEY + 4, SEEX - 5, SEEY * 2 - 1 - bw, rng(0, 1));
    science_room(this, SEEX - 3, SEEY + 4, SEEX + 2, SEEY * 2 - 1 - bw, 0);
    science_room(this, SEEX+4, SEEX+4, SEEX*2-1-rw, SEEY*2-1-bw, 3 * rng(0, 1));
    if (rw == 1) {
     ter(SEEX * 2 - 1, SEEY - 1) = t_door_metal_c;
     ter(SEEX * 2 - 1, SEEY    ) = t_door_metal_c;
    }
    if (bw == 1) {
     ter(SEEX - 1, SEEY * 2 - 1) = t_door_metal_c;
     ter(SEEX    , SEEY * 2 - 1) = t_door_metal_c;
    }
    if (terrain_type == ot_lab_stairs)
     ter(SEEX - 3 + 5 * rng(0, 1), SEEY - 3 + 5 * rng(0, 1)) = t_stairs_down;
    break;
   
   case 3:	// Big room
    for (int i = 0; i < SEEX * 2; i++) {
     for (int j = 0; j < SEEY * 2; j++) {
      if (i < lw || i >= SEEX * 2 - 1 - rw)
       ter(i, j) = t_wall_v;
      else if (j < tw || j >= SEEY * 2 - 1 - bw)
       ter(i, j) = t_wall_h;
      else
       ter(i, j) = t_floor;
     }
    }
    science_room(this, lw, tw, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw, rng(0, 3));
    if (t_above == ot_lab_stairs) {
     int sx, sy;
     do {
      sx = rng(lw, SEEX * 2 - 1 - rw);
      sy = rng(tw, SEEY * 2 - 1 - bw);
     } while (ter(sx, sy) != t_floor);
     ter(sx, sy) = t_stairs_up;
    }
    if (rw == 1) {
     ter(SEEX * 2 - 1, SEEY - 1) = t_door_metal_c;
     ter(SEEX * 2 - 1, SEEY    ) = t_door_metal_c;
    }
    if (bw == 1) {
     ter(SEEX - 1, SEEY * 2 - 1) = t_door_metal_c;
     ter(SEEX    , SEEY * 2 - 1) = t_door_metal_c;
    }
    if (terrain_type == ot_lab_stairs) {
     int sx, sy;
     do {
      sx = rng(lw, SEEX * 2 - 1 - rw);
      sy = rng(tw, SEEY * 2 - 1 - bw);
     } while (ter(sx, sy) != t_floor);
     ter(sx, sy) = t_stairs_down;
    }
    break;
   }
  }
// Ants will totally wreck up the place
  tw = 0;
  rw = 0;
  bw = 0;
  lw = 0;
  if (t_north >= ot_ants_ns && t_north <=ot_ants_nesw && connects_to(t_north,2))
   tw = SEEY;
  if (t_east  >= ot_ants_ns && t_east <= ot_ants_nesw && connects_to(t_east, 3))
   rw = SEEX;
  if (t_south >= ot_ants_ns && t_south <=ot_ants_nesw && connects_to(t_south,0))
   bw = SEEY + 1;
  if (t_west  >= ot_ants_ns && t_west <= ot_ants_nesw && connects_to(t_west, 1))
   lw = SEEX + 1;
  if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
   for (int i = 0; i < SEEX * 2; i++) {
    for (int j = 0; j < SEEY * 2; j++) {
     if ((i < SEEX*2 - lw && (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
         (i > rw &&          (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
         (j > tw &&          (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5))) ||
         (j < SEEY*2 - bw && (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5)))) {
      if (one_in(5))
       ter(i, j) = t_rubble;
      else
       ter(i, j) = t_rock_floor;
     }
    }
   }
  }

// Slimes pretty much wreck up the place, too, but only underground
  if (t_above == ot_null) {
   tw = (t_north == ot_slimepit ? SEEY     : 0);
   rw = (t_east  == ot_slimepit ? SEEX + 1 : 0);
   bw = (t_south == ot_slimepit ? SEEY + 1 : 0);
   lw = (t_west  == ot_slimepit ? SEEX     : 0);
   if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
    for (int i = 0; i < SEEX * 2; i++) {
     for (int j = 0; j < SEEY * 2; j++) {
      if (((j <= tw || i >= rw) && i >= j && (SEEX * 2 - 1 - i) <= j) ||
          ((j >= bw || i <= lw) && i <= j && (SEEY * 2 - 1 - j) <= i)   ) {
       if (one_in(5))
        ter(i, j) = t_rubble;
       else if (!one_in(5))
        ter(i, j) = t_slime;
      }
     }
    }
   }
  }

 break;

 case ot_lab_finale:
  tw = (t_north >= ot_lab && t_north <= ot_lab_finale) ? 0 : 2;
  rw = (t_east  >= ot_lab && t_east  <= ot_lab_finale) ? 1 : 2;
  bw = (t_south >= ot_lab && t_south <= ot_lab_finale) ? 1 : 2;
  lw = (t_west  >= ot_lab && t_west  <= ot_lab_finale) ? 0 : 2;
// Start by setting up a large, empty room.
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < lw || i > SEEX * 2 - 1 - rw)
     ter(i, j) = t_wall_v;
    else if (j < tw || j > SEEY * 2 - 1 - bw)
     ter(i, j) = t_wall_h;
    else
     ter(i, j) = t_floor;
   }
  }
  if (rw == 1) {
   ter(SEEX * 2 - 1, SEEY - 1) = t_door_metal_c;
   ter(SEEX * 2 - 1, SEEY    ) = t_door_metal_c;
  }
  if (bw == 1) {
   ter(SEEX - 1, SEEY * 2 - 1) = t_door_metal_c;
   ter(SEEX    , SEEY * 2 - 1) = t_door_metal_c;
  }
  switch (rng(1, 2)) {
  case 1:	// Weapons testing
   add_spawn(mon_secubot, 1,            6,            6);
   add_spawn(mon_secubot, 1, SEEX * 2 - 7,            6);
   add_spawn(mon_secubot, 1,            6, SEEY * 2 - 7);
   add_spawn(mon_secubot, 1, SEEX * 2 - 7, SEEY * 2 - 7);
   add_trap(SEEX - 2, SEEY - 2, tr_dissector);
   add_trap(SEEX + 1, SEEY - 2, tr_dissector);
   add_trap(SEEX - 2, SEEY + 1, tr_dissector);
   add_trap(SEEX + 1, SEEY + 1, tr_dissector);
   if (!one_in(3)) {
    rn = dice(4, 3);
    for (int i = 0; i < rn; i++) {
     add_item(SEEX - 1, SEEY - 1, (*itypes)[itm_laser_pack], 0);
     add_item(SEEX + 1, SEEY - 1, (*itypes)[itm_laser_pack], 0);
    }
    add_item(SEEX - 1, SEEY    , (*itypes)[itm_v29], 0);
    add_item(SEEX + 1, SEEY    , (*itypes)[itm_ftk93], 0);
   } else if (!one_in(3)) {
    rn = dice(3, 6);
    for (int i = 0; i < rn; i++) {
     add_item(SEEX - 1, SEEY - 1, (*itypes)[itm_mininuke], 0);
     add_item(SEEX    , SEEY - 1, (*itypes)[itm_mininuke], 0);
     add_item(SEEX - 1, SEEY    , (*itypes)[itm_mininuke], 0);
     add_item(SEEX    , SEEY    , (*itypes)[itm_mininuke], 0);
    }
   } else {
    ter(SEEX - 2, SEEY - 1) = t_rack;
    ter(SEEX - 1, SEEY - 1) = t_rack;
    ter(SEEX    , SEEY - 1) = t_rack;
    ter(SEEX + 1, SEEY - 1) = t_rack;
    ter(SEEX - 2, SEEY    ) = t_rack;
    ter(SEEX - 1, SEEY    ) = t_rack;
    ter(SEEX    , SEEY    ) = t_rack;
    ter(SEEX + 1, SEEY    ) = t_rack;
    place_items(mi_ammo, 96, SEEX - 2, SEEY - 1, SEEX + 1, SEEY - 1, false, 0);
    place_items(mi_allguns, 96, SEEX - 2, SEEY, SEEX + 1, SEEY, false, 0);
   }
   break;
  case 2:	// Netherworld access
   if (!one_in(4)) {	// Trapped netherworld monsters
    tw = rng(SEEY + 3, SEEY + 5);
    bw = tw + 4;
    lw = rng(SEEX - 6, SEEX - 2);
    rw = lw + 6;
    for (int i = lw; i <= rw; i++) {
     for (int j = tw; j <= bw; j++) {
      if (j == tw || j == bw) {
       if ((i - lw) % 2 == 0)
        ter(i, j) = t_wall_h;
       else
        ter(i, j) = t_reinforced_glass_h;
      } else if ((i - lw) % 2 == 0)
       ter(i, j) = t_wall_v;
      else if (j == tw + 2)
       ter(i, j) = t_wall_h;
      else {	// Empty space holds monsters!
       mon_id type = mon_id(rng(mon_flying_polyp, mon_blank));
       add_spawn(type, 1, i, j);
      }
     }
    }
   }
   ter(SEEX    , 8) = t_computer_nether;
   ter(SEEX - 2, 4) = t_radio_tower;
   ter(SEEX + 1, 4) = t_radio_tower;
   ter(SEEX - 2, 7) = t_radio_tower;
   ter(SEEX + 1, 7) = t_radio_tower;
   break;
  }
  break;

 case ot_silo:
  if (t_above == ot_null) {	// We're on ground level
   for (int i = 0; i < SEEX * 2; i++) {
    for (int j = 0; j < SEEY * 2; j++) {
     if (trig_dist(i, j, SEEX, SEEY) <= 6)
      ter(i, j) = t_metal_floor;
     else
      ter(i, j) = grass_or_dirt();
    }
   }
   switch (rng(1, 4)) {	// Placement of stairs
   case 1:
    lw = 3;
    mw = 5;
    tw = 3;
    break;
   case 2:
    lw = 3;
    mw = 5;
    tw = SEEY * 2 - 4;
    break;
   case 3:
    lw = SEEX * 2 - 7;
    mw = lw;
    tw = 3;
    break;
   case 4:
    lw = SEEX * 2 - 7;
    mw = lw;
    tw = SEEY * 2 - 4;
    break;
   }
   for (int i = lw; i <= lw + 2; i++) {
    ter(i, tw    ) = t_wall_h;
    ter(i, tw + 2) = t_wall_h;
   }
   ter(lw    , tw + 1) = t_wall_v;
   ter(lw + 1, tw + 1) = t_stairs_down;
   ter(lw + 2, tw + 1) = t_wall_v;
   ter(mw    , tw + 1) = t_door_metal_locked;
   ter(mw    , tw + 2) = t_card_reader;

  } else {	// We are NOT above ground.
   for (int i = 0; i < SEEX * 2; i++) {
    for (int j = 0; j < SEEY * 2; j++) {
     if (trig_dist(i, j, SEEX, SEEY) > 7)
      ter(i, j) = t_rock;
     else if (trig_dist(i, j, SEEX, SEEY) > 5) {
      ter(i, j) = t_metal_floor;
      if (one_in(30))
       add_field(NULL, i, j, fd_nuke_gas, 2);	// NULL game; no messages
     } else if (trig_dist(i, j, SEEX, SEEY) == 5) {
      ter(i, j) = t_hole;
      add_trap(i, j, tr_ledge);
     } else
      ter(i, j) = t_missile;
    }
   }
   silo_rooms(this);
  }
  break;

 case ot_silo_finale:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i == 5) {
     if (j > 4 && j < SEEY)
      ter(i, j) = t_reinforced_glass_v;
     else if (j == SEEY * 2 - 4)
      ter(i, j) = t_door_metal_c;
     else
      ter(i, j) = t_rock;
    } else
     ter(i, j) = t_rock_floor;
   }
  }
  ter(0, 0) = t_stairs_up;
  ter(4, 5) = t_computer_silo;
  break;

 case ot_temple:
 case ot_temple_stairs:
 case ot_temple_core:
  if (t_above == ot_null) {	// We're on ground level
   switch (rng(1, 4)) {	// Temple type
   case 1:	// Swamp with stairs down
    for (int i = 0; i < SEEX * 2; i++) {
     for (int j = 0; j < SEEY * 2; j++) {
      if (one_in(6))
       ter(i, j) = t_tree;
      else if (one_in(6))
       ter(i, j) = t_tree_young;
      else if (one_in(5))
       ter(i, j) = t_underbrush;
      else if (one_in(4))
       ter(i, j) = t_water_sh;
      else if (one_in(10))
       ter(i, j) = t_water_dp;
      else
       ter(i, j) = t_dirt;
     }
    }
    ter(SEEX - 4, SEEY - 4) = t_lava;
    ter(SEEX + 3, SEEY - 4) = t_lava;
    ter(SEEX - 4, SEEY + 3) = t_lava;
    ter(SEEX + 3, SEEY + 3) = t_lava;
    ter( rng(SEEX - 3, SEEX + 2), rng(SEEY - 3, SEEY + 2)) = t_stairs_down;
    break;
   }
  }
  break;

 case ot_sub_station_north:
 case ot_sub_station_east:
 case ot_sub_station_south:
 case ot_sub_station_west:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j < 9 || j > 12 || i < 4 || i > 19)
     ter(i, j) = t_pavement;
    else if (j < 12 && j > 8 && (i == 4 || i == 19))
     ter(i, j) = t_wall_v;
    else if (i > 3 && i < 20 && j == 12)
     ter(i, j) = t_wall_h;
    else
     ter(i, j) = t_floor;
   }
  }
  ter(16, 10) = t_stairs_down;
  if (terrain_type == ot_sub_station_east)
   rotate(1);
  if (terrain_type == ot_sub_station_south)
   rotate(2);
  if (terrain_type == ot_sub_station_west)
   rotate(3);
  break;

 case ot_set_center:
  tw = rng(4, SEEY * 2 - 5);
  lw = rng(4, SEEX * 2 - 5);
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i >= lw && i < lw + 3 && j >= tw && j < tw + 3)
     ter(i, j) = t_bulletin;
    else
     ter(i, j) = t_grass;
   }
  }
  if (one_in(4)) {
   ter(lw - 1, tw - 1) = t_tree_young;
   ter(lw - 1, tw + 3) = t_tree_young;
   ter(lw + 3, tw - 1) = t_tree_young;
   ter(lw + 3, tw + 3) = t_tree_young;
  }
  if (one_in(6)) {
   ter(           0,            1) = t_tree;
   ter(           1,            0) = t_tree;
   ter(           0, SEEY * 2 - 2) = t_tree;
   ter(           1, SEEY * 2 - 1) = t_tree;
   ter(SEEX * 2 - 2,            0) = t_tree;
   ter(SEEX * 2 - 1,            1) = t_tree;
   ter(SEEX * 2 - 2, SEEY * 2 - 1) = t_tree;
   ter(SEEX * 2 - 1, SEEY * 2 - 2) = t_tree;
  }
  break;
    
 case ot_set_house:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i > 2 && i < SEEX * 2 - 3 && (j == 2 || j == SEEY * 2 - 3)) ||
        (((i > 3 && i < 11) || (i > 12 && i < 20)) &&
         (j == 6 || j == 10 || j == 14)))
     ter(i, j) = t_wall_h;
    else if (j > 2 && j < SEEY * 2 - 3 && (i == 3 || i == SEEX * 2 - 4)) {
     if (j == 4 || j == 8 || j == 12 || j == 17 || j == 18)
      ter(i, j) = t_window;
     else
      ter(i, j) = t_wall_v;
    } else if (j > 2 && j < 14 && (i == 10 || i == 13)) {
     if (j == 4 || j == 8 || j == 12)
      ter(i, j) = t_door_c;
     else
      ter(i, j) = t_wall_v;
    } else if (i > 2 && i < SEEX * 2 - 3 && j > 1 && j < SEEY * 2 - 2)
     ter(i, j) = t_floor;
    else
     ter(i, j) = t_grass;
   }
  }
  ter(11, SEEY * 2 - 3) = t_door_c;
  ter(12, SEEY * 2 - 3) = t_door_c;
  ter( 5, SEEY * 2 - 3) = t_window;
  ter( 6, SEEY * 2 - 3) = t_window;
  ter(17, SEEY * 2 - 3) = t_window;
  ter(18, SEEY * 2 - 3) = t_window;
// Rear entrance, windows, or just a wall?
  switch (rng(0, 3)) {
  case 1:
  case 2:
   ter(11, 2) = t_window;
   ter(12, 2) = t_window;
   break;
  case 3:
   ter(11, 2) = t_door_c;
   ter(12, 2) = t_door_c;
   break;
  }
// Where is the fridge?
  ter(4 + 15 * rng(0, 1), 15 + 5 * rng(0, 1)) = t_fridge;
// Now, set up the interior of each room, picking a random layout
  for (int j = 4; j <= 11; j++) {
   for (int i = 4; i <= 19; i += 15) {
    int e = 1;	// Which side of the room is our stuff?
    if (i == 19)
     e = -1;	// Right-side rooms are mirrored; no beds against the door!
    switch (rng(1, 10)) {
    case 1:
     ter(i, j) = t_bed;	
     ter(i, j + 1) = t_bed;
     ter(i, j + 2) = t_dresser;
     break;
    case 2:
     ter(i, j) = t_dresser;
     ter(i, j + 1) = t_bed;
     ter(i, j + 2) = t_bed;
     break;
    case 3:
     ter(i, j) = t_bed;
     ter(i + e, j) = t_bed;
     ter(i + e * 2, j) = t_dresser;
     break;
    case 4:
     ter(i, j) = t_bed;	
     ter(i, j + 1) = t_bed;
     ter(i + e, j) = t_bed;
     break;
    case 5:
     ter(i, j) = t_bed;		
     ter(i + e, j) = t_bed;
     ter(i, j + 2) = t_dresser;	
     break;
    }
   }
   place_items(mi_bedroom, 80,  4, j,  9, j + 2, false, rng(0, turn));
   place_items(mi_bedroom, 80, 14, j, 19, j + 2, false, rng(0, turn));
  }
  place_items(mi_livingroom, 40, 5, 15, 10, 16, false, rng(0, turn));
  place_items(mi_kitchen, 40, 14, 15, 18, 15, false, 0);
// Finally, place items on the randomly-placed special squares
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (ter(i, j) == t_dresser)
     place_items(mi_dresser, 75, i, j, i, j, false, rng(0, turn));
    else if (ter(i, j) == t_fridge)
     place_items(mi_fridge, 80, i, j, i, j, false, rng(turn - 100, turn));
   }
  }
  make_all_items_owned();
  rotate(rng(0, 3));
  break;

 case ot_set_food:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (((j == 3 || j == SEEY * 2 - 4) &&
         ((i > 2 && i < 10) || (i > 13 && i < SEEX * 2 - 3))) ||
        ((i == 3 || i == SEEX * 2 - 4) &&
         ((j > 2 && j < 10) || (j > 13 && j < SEEY * 2 - 3))))
     ter(i, j) = t_counter;
    else
     ter(i, j) = t_dirt;
   }
  }
  
  for (int i = 3; i <= 14; i += 11) {
   for (int j = 3; j <= 14; j += 11) {
    switch (rng(0, 4)) {
    case 1:
    case 2:
     place_items(mi_fridge, 88, i, j, i + 6, j + 6, false, turn);
     break;
    case 3:
    case 4:
     place_items(mi_produce, 88, i, j, i + 6, j + 6, false, turn);
     break;
    }
   }
  }
  make_all_items_owned();
  break;

 case ot_set_weapons:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 5 && i > 2 && i < SEEX * 2 - 3)
     ter(i, j) = t_wall_h;
    else if (j == 13 && i > 2 && i < SEEX * 2 - 3) {
     if (i == 6 || i == 7 || i == 16 || i == 17)
      ter(i, j) = t_window;
     else if (i == 11)
      ter(i, j) = t_door_c;
     else
      ter(i, j) = t_wall_h;
    } else if (j == 9 && i > 3 && i < SEEX * 2 - 4) {
     if (i == 11)
      ter(i, j) = t_counter;
     else if (i == 12)
      ter(i, j) = t_window;
     else
      ter(i, j) = t_wall_h;
    } else if ((i == 3 || i == SEEX * 2 - 4) && j > 5 && j < 13)
     ter(i, j) = t_wall_v;
    else if (j == 6 && i > 3 && i < SEEX * 2 - 4)
     ter(i, j) = t_rack;
    else if (j > 6 && j < 13 && i > 2 && i < SEEX * 2 - 3)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  place_items(mi_weapons, 90, 4, 6, SEEX * 2 - 5, 6, false, 0);
  make_all_items_owned();
  if (one_in(2))
   ter(SEEX * 2 - 5, 9) = t_door_c;
  else
   ter(4, 9) = t_door_c;
  rotate(rng(0, 4));
  break;

 case ot_set_guns:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((j == 1 && i > 15 && i < SEEX * 2 - 4) || (j == 8 && i > 3 && i < 16) ||
        (j == 16 && i > 3 && i < SEEX * 2 - 4))
     ter(i, j) = t_wall_h;
    else if (j == 13 && i > 4 && i < 16) {
     if (i == 5)
      ter(i, j) = t_door_locked;
     else if (i == 10)
      ter(i, j) = t_counter;
     else if (i == 11)
      ter(i, j) = t_window;
     else
      ter(i, j) = t_wall_h;
    } else if ((i == 4 && j > 8 && j < 16) ||
               (i == SEEX * 2 - 5 && j > 1 && j < 16) ||
               (i == 16 && j > 1 && j < 14))
     ter(i, j) = t_wall_v;
    else if ((j == 9 && i > 4 && i < 16) || (i == 15 && j > 9 && j < 13))
     ter(i, j) = t_rack;
    else if (j == 13 && (i == 17 || i == 18))
     ter(i, j) = t_counter;
    else if ((j > 8 && j < 16 && i > 4 && i < 16) ||
             (j > 1 && j < 16 && i > 16 && i < 19))
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(15, 16) = t_door_c;
  ter(16, 14) = t_wall_glass_v;
  place_items(mi_allguns, 88, 5, 9, 14, 9, false, 0);
  place_items(mi_ammo, 92, 15, 9, 15, 12, false, 0);
  place_items(mi_gunxtras, 80, 15, 9, 15, 12, false, 0);
  make_all_items_owned();
  rotate(rng(0, 4));
  break;

 case ot_set_clinic:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (((j == 6 || j == 17) && i > 4 && i < SEEX * 2 - 5) ||
        (j == 9 && i > 4 && i < 11))
     ter(i, j) = t_wall_h;
    else if (((i == 5 || i == SEEX * 2 - 6) && j > 6 && j < 17) ||
             (i == 10 && j == 8))
     ter(i, j) = t_wall_v;
    else if ((j == 10 || j == 12 || j == 14) && (i == 6 || i == 7 || i == 16 ||
                                                 i == 17))
     ter(i, j) = t_bed;
    else if (j == 7 && i > 13 && i < SEEX * 2 - 6)
     ter(i, j) = t_counter;
    else if ((j == 8 && i > 5 && i < 10) || (i == 6 && j == 7))
     ter(i, j) = t_rack;
    else if (i > 5 && i < SEEX * 2 - 6 && j > 6 && j < 17)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
    }
   }
   if (one_in(5))
    ter(10, 7) = t_door_c;
   else
    ter(10, 7) = t_door_locked;
   w_fac = rng(6, 11);
   ter(w_fac, 17) = t_window;
   ter(w_fac + 1, 17) = t_window;
   ter(rng(13, 16), 17) = t_door_c;
   place_items(mi_harddrugs, 80, 6, 7, 6, 8, false, 0);
   place_items(mi_softdrugs, 86, 7, 8, 9, 8, false, 0);
   place_items(mi_dissection, 60, 14, 7, 17, 7, false, 0);
   make_all_items_owned();
   rotate(rng(0, 4));
   break;

 case ot_set_clothing:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i == 5 || i == SEEX * 2 - 6) && j > 7 && j < 19)
     ter(i, j) = t_wall_v;
    else if ((j == 7 || j == 13) && i > 4 && i < SEEX * 2 - 5)
     ter(i, j) = t_wall_h;
    else if (j == 19 && i > 4 && i < SEEX * 2 - 5) {
     if ((i > 5 && i < 9) || (i > 14 && i < 18))
      ter(i, j) = t_window;
     else
      ter(i, j) = t_wall_h;
    } else if (j == 16 && i > 4 && i < SEEX * 2 - 5)
     ter(i, j) = t_counter;
    else if (i < 18 && ((j ==  8 && i > 5) || (j == 10 && i > 7) ||
                        (j == 12 && i > 9)))
     ter(i, j) = t_rack;
    else if (j > 7 && j < 19 && i > 5 && i < SEEX * 2 - 6)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(rng(10, 13), 19) = t_door_c;
  ter(rng( 6,  9), 13) = t_door_c;
  ter(8 + dice(3, 3), 16) = t_floor;
  place_items(mi_shoes,      75, 12, 14, 17, 14, false, 0);
  place_items(mi_allclothes, 90,  6,  8, 17,  8, false, 0);
  place_items(mi_allclothes, 90,  8, 10, 17, 10, false, 0);
  place_items(mi_allclothes, 90, 10, 12, 17, 12, false, 0);
  make_all_items_owned();
  rotate(rng(0, 3));
  break;

 case ot_set_general:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 4 && i > 1 && i < SEEX * 2 - 2)
     ter(i, j) = t_wall_h;
    else if (j == 17 && i > 1 && i < SEEX * 2 - 2) {
     if ((i > 4 && i < 9) || (i > 14 && i < 19))
      ter(i, j) = t_window;
     else if (i == 11 || i == 12)
      ter(i, j) = t_door_c;
     else
      ter(i, j) = t_wall_h;
    } else if ((i == 2 || i == SEEX * 2 - 3) && j > 4 && j < 17)
     ter(i, j) = t_wall_v;
    else if ((j == 13 && i > 3 && i < 10) || (i == 9 && j > 13 && j < 17))
     ter(i, j) = t_counter;
    else if (j == 5 && i > 11 && i < SEEX * 2 - 3)
     ter(i, j) = t_fridge;
    else if (((i ==  4 || i ==  5 || i ==  8 || i ==  9) && j > 5 && j < 11) ||
             ((i == 13 || i == 14 || i == 17 || i == 18) && j > 7 && j < 15))
     ter(i, j) = t_rack;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 4 && j < 17)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  place_items(mi_fridge,	92, 12,  5, 20,  5, false, turn);
  place_items(mi_softdrugs,	88,  4,  9,  5, 10, false, turn);
  place_items(mi_behindcounter,	86,  4,  6,  5,  8, false, turn);
  place_items(mi_cleaning,	88,  8,  6,  9,  7, false, turn);
  place_items(mi_alcohol,	86,  8,  8,  9,  9, false, turn);
  place_items(mi_manuals,	65,  8, 10,  9, 10, false, turn);
  place_items(mi_tools,		84, 13,  8, 13, 14, false, turn);
  place_items(mi_survival_armor,80, 14,  8, 14, 14, false, turn);
  place_items(mi_survival_tools,80, 17,  8, 17, 14, false, turn);
  place_items(mi_cannedfood,	86, 18,  8, 18, 14, false, turn);
  make_all_items_owned();
  rotate(rng(0, 4));
  break;

 case ot_set_casino:
  tw = rng(5, 6);
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == SEEY - 3 && ((i > 3 && i < 8) || (i > 15 && i < 20)))
     ter(i, j) = t_window;
    else if (j == SEEY - 3 && i > 9 && i < 14)
     ter(i, j) = t_door_c;
    else if (((j == 2 || j == SEEY * 2 - 3) && i > 1 && i < SEEX * 2 - 2) ||
             (j == tw && i > 2 && i < SEEX * 2 - 3))
     ter(i, j) = t_wall_h;
    else if (((i == 2 || i == SEEY * 2 - 3) && j > 2 && j < SEEY * 2 - 3) ||
             (i == 13 && j > 2 && j < 6) ||
             ((i == 7 || i == 12) && j > 8 && j < 17))
     ter(i, j) = t_wall_v;
    else if (((i == 3 | i == 6 || i == 8 || i == 11 || i == 13) &&
              j > 8 && j < 17) ||
             (j == 7 && i > 4 && i < 14))
     ter(i, j) = t_slot_machine;
    else if (((j == 9 || j == 11 || j == 13 || j == 15) &&
              i < SEEX * 2 - 3 && i > SEEX * 2 - 6) ||
             (i == SEEX * 2 - 5 && (j == 8 || j == 14)))
     ter(i, j) = t_counter;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 3)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  ter(14, rng(3, 5)) = t_door_c;
  ter(rng(15, 20), 6) = t_door_locked;
// TODO: What's in the back rooms?  Some goodies, presumably.  Maybe a MOB BOSS.
  
  break;

 case ot_set_library:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (((j == 2 || j == SEEY * 2 - 3) && i > 1 && i < SEEX * 2 - 2) ||
        (j == 17 && i > 2 && i < SEEX * 2 - 3))
     ter(i, j) = t_wall_h;
    else if ((i == 2 || i == SEEX * 2 - 3) && j > 2 && j < SEEY * 2 - 3)
     ter(i, j) = t_wall_v;
    else if (j > 2 && j < 17 && (j % 3 == 1 || j % 3 == 2) &&
             i > 2 && i < SEEX * 2 - 3 && (i < 10 || i > 12))
     ter(i, j) = t_bookcase;
    else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 3)
     ter(i, j) = t_floor;
    else
     ter(i, j) = grass_or_dirt();
   }
  }
  for (int j = 3; j <= 5; j += 3) {
   ter(2, j) = t_window;
   ter(SEEX * 2 - 3, j) = t_window;
  }
  ter(rng(10, 12),  2) = t_window;
  ter(rng(10, 12), 17) = t_door_c;
  ter(rng( 8, 13), SEEY * 2 - 3) = t_door_c;
  ter(rng( 3,  7), SEEY * 2 - 3) = t_window;
  ter(rng(14, 18), SEEY * 2 - 3) = t_window;
  ter(15, 19) = t_counter;
  ter(15, 20) = t_counter;
  place_items(mi_magazines,	80,  3, 16,  9, 16, false, 0);
  place_items(mi_magazines,	80, 13, 16, 20, 16, false, 0);

  place_items(mi_novels,	85,  4, 13,  9, 13, false, 0);
  place_items(mi_novels,	85, 13, 13, 20, 13, false, 0);
  place_items(mi_novels,	85,  4, 14,  9, 14, false, 0);
  place_items(mi_novels,	85, 13, 14, 20, 14, false, 0);
  place_items(mi_novels,	85,  4, 10,  9, 10, false, 0);
  place_items(mi_novels,	85, 13, 10, 20, 10, false, 0);
  place_items(mi_novels,	85,  4, 11,  9, 11, false, 0);
  place_items(mi_novels,	85, 13, 11, 20, 11, false, 0);

  place_items(mi_manuals,	80,  4,  7,  9,  7, false, 0);
  place_items(mi_manuals,	80, 13,  7, 20,  7, false, 0);
  place_items(mi_manuals,	80,  4,  8,  9,  8, false, 0);
  place_items(mi_manuals,	80, 13,  8, 20,  8, false, 0);

  place_items(mi_textbooks,	75,  4,  4,  9,  4, false, 0);
  place_items(mi_textbooks,	75, 13,  4, 20,  4, false, 0);
  place_items(mi_textbooks,	75,  4,  5,  9,  5, false, 0);
  place_items(mi_textbooks,	75, 13,  5, 20,  5, false, 0);
  
  make_all_items_owned();
  rotate(rng(0, 4));
  break;

 case ot_set_lab:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (((i == 1 || i == SEEX * 2 - 2) && j > 0 && j < SEEY * 2 - 2) ||
        ((i == 10 || i == 13) && j > 0 && j < 18))
     ter(i, j) = t_wall_v;
    else if (((j == 0 || j == SEEY * 2 - 2) && i > 0 && i < SEEX * 2 - 1) ||
             ((j == 6 || j == 12 || j == 18) && i > 1 && i < SEEX * 2 - 2 &&
              i != 11 && i != 12))
     ter(i, j) = t_wall_h;
   }
  }
  for (int j = 3; j <= 15; j += 6) {
   ter(10, j) = t_door_c;
   ter(13, j) = t_door_c;
  }
  ter(14, 19) = t_counter;
  ter(15, 19) = t_counter;
  set_science_room(this,  2,  1, true, turn);
  set_science_room(this,  2,  7, true, turn);
  set_science_room(this,  2, 13, true, turn);
  set_science_room(this, 14,  1, false, turn);
  set_science_room(this, 14,  7, false, turn);
  set_science_room(this, 14, 13, false, turn);
  make_all_items_owned();
  break;

 case ot_set_bionics:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((j ==  8 && i > 1 && i < SEEX * 2 - 2) ||
        (j == 13 && i > 2 && i < SEEX * 2 - 3))
     ter(i, j) = t_wall_h;
    else if (j == 19) {
     if (i < 2 || i > SEEX * 2 - 3)
      ter(i, j) = grass_or_dirt();
     else if ((i > 4 && i < 8) || (i > 15 && i < 19))
      ter(i, j) = t_window;
     else if (i == 11 || i == 12)
      ter(i, j) = t_door_c;
     else
      ter(i, j) = t_wall_h;
    } else if (j == 14 && i > 2 && i < 15)
     ter(i, j) = t_rack;
    else if ((j == 16 || j == 9) && i > 2 && i < 19)
     ter(i, j) = t_counter;
    else if ((i == 2 || i == SEEX * 2 - 3) && j > 8 && j < 19)
     ter(i, j) = t_wall_v;
   }
  }
  ter(3, 12) = t_bed;
  ter(4, 12) = t_bed;
  place_items(mi_dissection,	70, 3, 9, 18, 9, false, 0);
  place_items(mi_electronics,	50, 3, 9, 18, 9, false, 0);

  break;

 case ot_radio_tower:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = grass_or_dirt();
  }
  lw = rng(1, SEEX * 2 - 2);
  tw = rng(1, SEEY * 2 - 2);
  for (int i = lw; i < lw + 4; i++) {
   for (int j = tw; j < tw + 4; j++)
    ter(i, j) = t_radio_tower;
  }
  rw = -1;
  bw = -1;
  if (lw <= 4)
   rw = rng(lw + 5, 10);
  else if (lw >= 16)
   rw = rng(3, lw - 13);
  if (tw <= 3)
   bw = rng(tw + 5, 10);
  else if (tw >= 16)
   bw = rng(3, tw - 7);
  if (rw != -1 && bw != -1) {
   for (int i = rw; i < rw + 12; i++) {
    for (int j = bw; j < bw + 6; j++) {
     if (j == bw || j == bw + 5)
      ter(i, j) = t_wall_h;
     else if (i == rw || i == rw + 11)
      ter(i, j) = t_wall_v;
     else if (j == bw + 1)
      ter(i, j) = t_counter;
     else
      ter(i, j) = t_floor;
    }
   }
   cw = rng(rw + 2, rw + 8);
   ter(cw, bw + 5) = t_window;
   ter(cw + 1, bw + 5) = t_window;
   ter(rng(rw + 2, rw + 8), bw + 5) = t_door_c;
   ter(rng(rw + 2, rw + 8), bw + 1) = t_radio_controls;
   place_items(mi_radio, 60, rw + 1, bw + 2, rw + 10, bw + 4, true, 0);
  } else	// No control room... simple controls near the tower
   ter(rng(lw, lw + 3), tw + 4) = t_radio_controls;
  break;

 case ot_gate:
 case ot_wall:
  if (t_west == ot_wall)
   w_fac = 0;
  else
   w_fac = 9;
  if (t_east == ot_wall)
   e_fac = SEEX * 2;
  else
   e_fac = 15;
  if (t_north == ot_wall)
   n_fac = 0;
  else
   n_fac = 9;
  if (t_south == ot_wall)
   s_fac = SEEY * 2;
  else
   s_fac = 15;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = grass_or_dirt();
  }
  for (int i = w_fac; i < e_fac; i++) {
   for (int j = 9; j < 15; j++)
    ter(i, j) = t_rock;
  }
  for (int j = n_fac; j < s_fac; j++) {
   for (int i = 9; i < 15; i++)
    ter(i, j) = t_rock;
  }
  if (terrain_type == ot_gate) {
   if (e_fac == 0) {
    for (int i = 9; i < 15; i++) {
     for (int j = 9; j < 15; j++) {
      if (j == 11)
       ter(i, j) = t_portcullis;
      else
       ter(i, j) = grass_or_dirt();
     }
    }
   } else {	// It's safe to assume with a gate that if e_fac!=0, n_fac==0
    for (int i = 9; i < 15; i++) {
     for (int j = 9; j < 15; j++) {
      if (i == 11)
       ter(i, j) = t_portcullis;
      else
       ter(i, j) = grass_or_dirt();
     }
    }
   }
  }
  break;


 case ot_anthill:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 8 || j < 8 || i > SEEX * 2 - 9 || j > SEEY * 2 - 9)
     ter(i, j) = grass_or_dirt();
    else if ((i == 11 || i == 12) && (j == 11 || j == 12))
     ter(i, j) = t_slope_down;
    else
     ter(i, j) = t_dirtmound;
   }
  }
  break;

 case ot_rock:
  if (t_north == ot_cavern || t_north == ot_slimepit ||
      t_north == ot_slimepit_down)
   n_fac = 6;
  else
   n_fac = 0;
  if (t_east == ot_cavern || t_east == ot_slimepit ||
      t_east == ot_slimepit_down)
   e_fac = 6;
  else
   e_fac = 0;
  if (t_south == ot_cavern || t_south == ot_slimepit ||
      t_south == ot_slimepit_down)
   s_fac = 6;
  else
   s_fac = 0;
  if (t_west == ot_cavern || t_west == ot_slimepit ||
      t_west == ot_slimepit_down)
   w_fac = 6;
  else
   w_fac = 0;

  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (rng(0, n_fac) > j || rng(0, e_fac) > SEEX * 2 - 1 - i ||
        rng(0, w_fac) > i || rng(0, s_fac) > SEEY * 2 - 1 - j   )
     ter(i, j) = t_rock_floor;
    else
     ter(i, j) = t_rock;
   }
  }
  break;

 case ot_rift:
  if (t_north != ot_rift && t_north != ot_hellmouth) {
   if (connects_to(t_north, 2))
    n_fac = rng(-6, -2);
   else
    n_fac = rng(2, 6);
  }
  if (t_east != ot_rift && t_east != ot_hellmouth) {
   if (connects_to(t_east, 3))
    e_fac = rng(-6, -2);
   else
    e_fac = rng(2, 6);
  }
  if (t_south != ot_rift && t_south != ot_hellmouth) {
   if (connects_to(t_south, 0))
    s_fac = rng(-6, -2);
   else
    s_fac = rng(2, 6);
  }
  if (t_west != ot_rift && t_west != ot_hellmouth) {
   if (connects_to(t_west, 1))
    w_fac = rng(-6, -2);
   else
    w_fac = rng(2, 6);
  }
// Negative *_fac values indicate rock floor connection, otherwise solid rock
// Of course, if we connect to a rift, *_fac = 0, and thus lava extends all the
//  way.
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((n_fac < 0 && j < n_fac * -1) || (s_fac < 0 && j >= SEEY * 2 - s_fac) ||
        (w_fac < 0 && i < w_fac * -1) || (e_fac < 0 && i >= SEEX * 2 - e_fac)  )
     ter(i, j) = t_rock_floor;
    else if (j < n_fac || j >= SEEY * 2 - s_fac ||
             i < w_fac || i >= SEEX * 2 - e_fac   )
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_lava;
   }
  }
  break;

 case ot_hellmouth:
  if (t_north != ot_rift && t_north != ot_hellmouth)
   n_fac = 6;
  if (t_east != ot_rift && t_east != ot_hellmouth)
   e_fac = 6;
  if (t_south != ot_rift && t_south != ot_hellmouth)
   s_fac = 6;
  if (t_west != ot_rift && t_west != ot_hellmouth)
   w_fac = 6;

  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j < n_fac || j >= SEEY*2 - s_fac || i < w_fac || i >= SEEX*2 - e_fac ||
        (i >= 6 && i < SEEX * 2 - 6 && j >= 6 && j < SEEY * 2 - 6))
     ter(i, j) = t_rock_floor;
    else
     ter(i, j) = t_lava;
    if (i >= SEEX - 1 && i <= SEEX && j >= SEEY - 1 && j <= SEEY)
     ter(i, j) = t_slope_down;
   }
  }
  switch (rng(0, 4)) {	// Randomly chosen "altar" design
  case 0:
   for (int i = 7; i <= 16; i += 3) {
    ter(i, 6) = t_rock;
    ter(i, 17) = t_rock;
    ter(6, i) = t_rock;
    ter(17, i) = t_rock;
    if (i > 7 && i < 16) {
     ter(i, 10) = t_rock;
     ter(i, 13) = t_rock;
    } else {
     ter(i - 1, 6 ) = t_rock;
     ter(i - 1, 10) = t_rock;
     ter(i - 1, 13) = t_rock;
     ter(i - 1, 17) = t_rock;
    }
   }
   break;
  case 1:
   for (int i = 6; i < 11; i++) {
    ter(i, i) = t_lava;
    ter(SEEX * 2 - 1 - i, i) = t_lava;
    ter(i, SEEY * 2 - 1 - i) = t_lava;
    ter(SEEX * 2 - 1 - i, SEEY * 2 - 1 - i) = t_lava;
    if (i < 10) {
     ter(i + 1, i) = t_lava;
     ter(SEEX * 2 - i, i) = t_lava;
     ter(i + 1, SEEY * 2 - 1 - i) = t_lava;
     ter(SEEX * 2 - i, SEEY * 2 - 1 - i) = t_lava;

     ter(i, i + 1) = t_lava;
     ter(SEEX * 2 - 1 - i, i + 1) = t_lava;
     ter(i, SEEY * 2 - i) = t_lava;
     ter(SEEX * 2 - 1 - i, SEEY * 2 - i) = t_lava;
    }
    if (i < 9) {
     ter(i + 2, i) = t_rock;
     ter(SEEX * 2 - i + 1, i) = t_rock;
     ter(i + 2, SEEY * 2 - 1 - i) = t_rock;
     ter(SEEX * 2 - i + 1, SEEY * 2 - 1 - i) = t_rock;

     ter(i, i + 2) = t_rock;
     ter(SEEX * 2 - 1 - i, i + 2) = t_rock;
     ter(i, SEEY * 2 - i + 1) = t_rock;
     ter(SEEX * 2 - 1 - i, SEEY * 2 - i + 1) = t_rock;
    }
   }
   break;
  case 2:
   for (int i = 7; i < 17; i++) {
    ter(i, 6) = t_rock;
    ter(6, i) = t_rock;
    ter(i, 17) = t_rock;
    ter(17, i) = t_rock;
    if (i != 7 && i != 16 && i != 11 && i != 12) {
     ter(i, 8) = t_rock;
     ter(8, i) = t_rock;
     ter(i, 15) = t_rock;
     ter(15, i) = t_rock;
    }
    if (i == 11 || i == 12) {
     ter(i, 10) = t_rock;
     ter(10, i) = t_rock;
     ter(i, 13) = t_rock;
     ter(13, i) = t_rock;
    }
   }
   break;
  case 3:
   for (int i = 6; i < 11; i++) {
    for (int j = 6; j < 11; j++) {
     ter(i, j) = t_lava;
     ter(SEEX * 2 - 1 - i, j) = t_lava;
     ter(i, SEEY * 2 - 1 - j) = t_lava;
     ter(SEEX * 2 - 1 - i, SEEY * 2 - 1 - j) = t_lava;
    }
   }
   break;
  }
  break;

 case ot_slimepit:
 case ot_slimepit_down:
  n_fac = (t_north == ot_slimepit || t_north == ot_slimepit_down ? 1 : 0);
  e_fac = (t_east  == ot_slimepit || t_east  == ot_slimepit_down ? 1 : 0);
  s_fac = (t_south == ot_slimepit || t_south == ot_slimepit_down ? 1 : 0);
  w_fac = (t_west  == ot_slimepit || t_west  == ot_slimepit_down ? 1 : 0);
 
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (!one_in(10) && (j < n_fac * SEEX        || i < w_fac * SEEX || 
                        j > SEEY*2 - s_fac*SEEY || i > SEEX*2 - e_fac*SEEX))
     ter(i, j) = (!one_in(10) ? t_slime : t_rock_floor);
    else if (rng(0, SEEX) > abs(i - SEEX) && rng(0, SEEY) > abs(j - SEEY))
     ter(i, j) = t_slime;
    else if (t_above == ot_null)
     ter(i, j) = t_dirt;
    else
     ter(i, j) = t_rock_floor;
   }
  }

  if (terrain_type == ot_slimepit_down)
   ter(rng(3, SEEX * 2 - 4), rng(3, SEEY * 2 - 4)) = t_slope_down;

  if (t_above == ot_slimepit_down) {
   switch (rng(1, 4)) {
    case 1: ter(rng(0, 2), rng(0, 2)) = t_slope_up;
    case 2: ter(rng(0, 2), SEEY * 2 - rng(1, 3)) = t_slope_up;
    case 3: ter(SEEX * 2 - rng(1, 3), rng(0, 2)) = t_slope_up;
    case 4: ter(SEEX * 2 - rng(1, 3), SEEY * 2 - rng(1, 3)) = t_slope_up;
   }
  }

  add_spawn(mon_blob, 8, SEEX, SEEY);
  place_items(mi_sewer, 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);

  break;

 case ot_basement:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i == 0 || j == 0 || i == SEEX * 2 - 1 || j == SEEY * 2 - 1)
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  switch (rng(1, 4)) {	// TODO: More types!
  case 1:	// Weapons cache
   for (int i = 2; i < SEEX * 2 - 2; i++) {
    ter(i, 1) = t_rack;
    ter(i, 5) = t_rack;
    ter(i, 9) = t_rack;
   }
   place_items(mi_allguns, 92, 2, 1, SEEX * 2 - 3, 1, false, 0);
   place_items(mi_ammo,    94, 2, 5, SEEX * 2 - 3, 5, false, 0);
   place_items(mi_weapons, 88, 2, 9, SEEX * 2 - 3, 9, false, 0);
   ter(SEEX - 1, SEEY * 2 - 2) = t_stairs_up;
   ter(SEEX    , SEEY * 2 - 2) = t_stairs_up;
   break;
  case 2:	// Survival Bunker
   ter(1, 1) = t_bed;
   ter(1, 2) = t_bed;
   ter(SEEX * 2 - 2, 1) = t_bed;
   ter(SEEX * 2 - 2, 2) = t_bed;
   for (int i = 1; i < SEEY; i++) {
    ter(SEEX - 1, i) = t_rack;
    ter(SEEX    , i) = t_rack;
   }
   place_items(mi_softdrugs,	 86, SEEX - 1,  1, SEEX,  2, false, 0);
   place_items(mi_cannedfood,	 92, SEEX - 1,  3, SEEX,  6, false, 0);
   place_items(mi_homeguns,	 72, SEEX - 1,  7, SEEX,  7, false, 0);
   place_items(mi_survival_tools,83, SEEX - 1,  8, SEEX, 10, false, 0);
   place_items(mi_manuals,	 60, SEEX - 1, 11, SEEX, 11, false, 0);
   ter(SEEX - 1, SEEY * 2 - 2) = t_stairs_up;
   ter(SEEX    , SEEY * 2 - 2) = t_stairs_up;
   break;
  case 3:	// Chem lab
   for (int i = 1; i < SEEY + 4; i++) {
    ter(1           , i) = t_counter;
    ter(SEEX * 2 - 2, i) = t_counter;
   }
   place_items(mi_chemistry,	90,        1, 1,        1, SEEY + 3, false, 0);
   if (one_in(3))
    place_items(mi_chemistry,	90, SEEX*2-2, 1, SEEX*2-2, SEEY + 3, false, 0);
   else
    place_items(mi_electronics,	90, SEEX*2-2, 1, SEEX*2-2, SEEY + 3, false, 0);
   ter(SEEX - 1, SEEY * 2 - 2) = t_stairs_up;
   ter(SEEX    , SEEY * 2 - 2) = t_stairs_up;
   break;
  }
  break;
 
// TODO: Maybe subway systems could have broken down trains in them?
 case ot_subway_station:
  if (t_north >= ot_subway_ns && t_north <= ot_subway_nesw &&
      connects_to(t_north, 2))
   n_fac = 1;
  if (t_east >= ot_subway_ns && t_east <= ot_subway_nesw &&
      connects_to(t_east, 3))
   e_fac = 1;
  if (t_south >= ot_subway_ns && t_south <= ot_subway_nesw &&
      connects_to(t_south, 0))
   s_fac = 1;
  if (t_west >= ot_subway_ns && t_west <= ot_subway_nesw &&
      connects_to(t_west, 1))
   w_fac = 1;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i < 4 && (w_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
        (j < 4 && (n_fac == 0 || i < 4 || i > SEEX * 2 - 5)) ||
        (i > SEEX * 2 - 5 && (e_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
        (j > SEEY * 2 - 5 && (s_fac == 0 || i < 4 || i > SEEX * 2 - 5)))
     ter(i, j) = t_floor;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  ter(2,            2           ) = t_stairs_up;
  ter(SEEX * 2 - 3, 2           ) = t_stairs_up;
  ter(2,            SEEY * 2 - 3) = t_stairs_up;
  ter(SEEX * 2 - 3, SEEY * 2 - 3) = t_stairs_up;
  if (ter(2, SEEY) == t_floor)
   ter(2, SEEY) = t_stairs_up;
  if (ter(SEEX * 2 - 3, SEEY) == t_floor)
   ter(SEEX * 2 - 3, SEEY) = t_stairs_up;
  if (ter(SEEX, 2) == t_floor)
   ter(SEEX, 2) = t_stairs_up;
  if (ter(SEEX, SEEY * 2 - 3) == t_floor)
   ter(SEEX, SEEY * 2 - 3) = t_stairs_up;
  break;
 case ot_subway_ns:
 case ot_subway_ew:
  if (terrain_type == ot_subway_ns) {
   w_fac = (t_west  == ot_cavern ? 0 : 4);
   e_fac = (t_east  == ot_cavern ? SEEX * 2 : SEEX * 2 - 5);
  } else {
   w_fac = (t_north == ot_cavern ? 0 : 4);
   e_fac = (t_south == ot_cavern ? SEEX * 2 : SEEX * 2 - 5);
  }
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < w_fac || i > e_fac)
     ter(i, j) = t_rock;
    else if (one_in(90))
     ter(i, j) = t_rubble;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  if (t_above >= ot_sub_station_north && t_above <= ot_sub_station_west)
   ter(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4)) = t_stairs_up;
  place_items(mi_subway, 30, 4, 0, SEEX * 2 - 5, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_subway_ew)
   rotate(1);
  break;

 case ot_subway_ne:
 case ot_subway_es:
 case ot_subway_sw:
 case ot_subway_wn:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i >= SEEX * 2 - 4 && j < 4) || i < 4 || j >= SEEY * 2 - 4)
     ter(i, j) = t_rock;
    else if (one_in(30))
     ter(i, j) = t_rubble;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  if (t_above >= ot_sub_station_north && t_above <= ot_sub_station_west)
   ter(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4)) = t_stairs_up;
  place_items(mi_subway, 30, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_subway_es)
   rotate(1);
  if (terrain_type == ot_subway_sw)
   rotate(2);
  if (terrain_type == ot_subway_wn)
   rotate(3);
  break;

 case ot_subway_nes:
 case ot_subway_new:
 case ot_subway_nsw:
 case ot_subway_esw:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < 4 || (i >= SEEX * 2 - 4 && (j < 4 || j >= SEEY * 2 - 4)))
     ter(i, j) = t_rock;
    else if (one_in(30))
     ter(i, j) = t_rubble;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  if (t_above >= ot_sub_station_north && t_above <= ot_sub_station_west)
   ter(4, rng(SEEY - 5, SEEY + 4)) = t_stairs_up;
  place_items(mi_subway, 35, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_subway_esw)
   rotate(1);
  if (terrain_type == ot_subway_nsw)
   rotate(2);
  if (terrain_type == ot_subway_new)
   rotate(3);
  break;

 case ot_subway_nesw:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i < 4 || i >= SEEX * 2 - 4) &&
        (j < 4 || j >= SEEY * 2 - 4))
      ter(i, j) = t_rock;
    else if (one_in(30))
     ter(i, j) = t_rubble;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  if (t_above >= ot_sub_station_north && t_above <= ot_sub_station_west)
   ter(4 + rng(0,1)*(SEEX * 2 - 9), 4 + rng(0,1)*(SEEY * 2 - 9)) = t_stairs_up;
  place_items(mi_subway, 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  break;

 case ot_sewer_ns:
 case ot_sewer_ew:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < SEEX - 2 || i > SEEX + 1)
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_sewage;
   }
  }
  place_items(mi_sewer, 10, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_sewer_ew)
   rotate(1);
  break;

 case ot_sewer_ne:
 case ot_sewer_es:
 case ot_sewer_sw:
 case ot_sewer_wn:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i > SEEX + 1 && j < SEEY - 2) || i < SEEX - 2 || j > SEEY + 1)
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_sewage;
   }
  }
  place_items(mi_sewer, 18, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_sewer_es)
   rotate(1);
  if (terrain_type == ot_sewer_sw)
   rotate(2);
  if (terrain_type == ot_sewer_wn)
   rotate(3);
  break;

 case ot_sewer_nes:
 case ot_sewer_new:
 case ot_sewer_nsw:
 case ot_sewer_esw:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < SEEX - 2 || (i > SEEX + 1 && (j < SEEY - 2 || j > SEEY + 1)))
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_sewage;
   }
  }
  place_items(mi_sewer, 23, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_sewer_esw)
   rotate(1);
  if (terrain_type == ot_sewer_nsw)
   rotate(2);
  if (terrain_type == ot_sewer_new)
   rotate(3);
  break;

 case ot_sewer_nesw:
  rn = rng(0, 3);
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((i < SEEX - 2 || i > SEEX + 1) && (j < SEEY - 2 || j > SEEY + 1))
      ter(i, j) = t_rock;
    else
     ter(i, j) = t_sewage;
    if (rn == 0 && (trig_dist(i, j, SEEX - 1, SEEY - 1) <= 6 ||
                    trig_dist(i, j, SEEX - 1, SEEY    ) <= 6 ||
                    trig_dist(i, j, SEEX,     SEEY - 1) <= 6 ||
                    trig_dist(i, j, SEEX,     SEEY    ) <= 6   ))
     ter(i, j) = t_sewage;
    if (rn == 0 && (i == SEEX - 1 || i == SEEX) && (j == SEEY - 1 || j == SEEY))
     ter(i, j) = t_grate;
   }
  }
  place_items(mi_sewer, 28, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  break;

 case ot_ants_ns:
 case ot_ants_ew:
  x = SEEX;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_rock;
  }
  for (int j = 0; j < SEEY * 2; j++) {
   for (int i = x - 2; i <= x + 3; i++) {
    if (i >= 1 && i < SEEX * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
   x += rng(-1, 1);
   while (abs(SEEX - x) > SEEX * 2 - j - 1) {
    if (x < SEEX) x++;
    if (x > SEEX) x--;
   }
  }
  if (terrain_type == ot_ants_ew)
   rotate(1);
  break;

 case ot_ants_ne:
 case ot_ants_es:
 case ot_ants_sw:
 case ot_ants_wn:
  x = SEEX;
  y = 1;
  rn = rng(80, 100);
// First, set it all to rock
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_rock;
  }
  for (int i = SEEX - 2; i <= SEEX + 3; i++) {
   ter(i, 0) = t_rock_floor;
   ter(i, 1) = t_rock_floor;
   ter(i, 2) = t_rock_floor;
   ter(SEEX * 2 - 1, i) = t_rock_floor;
   ter(SEEX * 2 - 2, i) = t_rock_floor;
   ter(SEEX * 2 - 3, i) = t_rock_floor;
  }
  do {
   for (int i = x - 2; i <= x + 3; i++) {
    for (int j = y - 2; j <= y + 3; j++) {
     if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1)
      ter(i, j) = t_rock_floor;
    }
   }
   if (!one_in(3))
    x++;
   if (!one_in(x - SEEX))
    y += rng(-1, 1);
   if (rn <= 0) {
    if (y < SEEY) y++;
    if (y > SEEY) y--;
   } else
    rn--;
  } while (x < SEEX * 2 - 1 || y != SEEY);
  for (int i = x - 2; i <= x + 3; i++) {
   for (int j = y - 2; j <= y + 3; j++) {
    if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
  }
  if (terrain_type == ot_ants_es)
   rotate(1);
  if (terrain_type == ot_ants_sw)
   rotate(2);
  if (terrain_type == ot_ants_wn)
   rotate(3);
  break;
  
 case ot_ants_nes:
 case ot_ants_new:
 case ot_ants_nsw:
 case ot_ants_esw:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_rock;
  }
  x = SEEX;
  for (int j = 0; j < SEEY * 2; j++) {
   for (int i = x - 2; i <= x + 3; i++) {
    if (i >= 1 && i < SEEX * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
   x += rng(-1, 1);
   while (abs(SEEX - x) > SEEY * 2 - j - 1) {
    if (x < SEEX) x++;
    if (x > SEEX) x--;
   }
  }
  y = SEEY;
  for (int i = SEEX; i < SEEX * 2; i++) {
   for (int j = y - 2; j <= y + 3; j++) {
    if (j >= 1 && j < SEEY * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
   y += rng(-1, 1);
   while (abs(SEEY - y) > SEEX * 2 - 1 - i) {
    if (y < SEEY) y++;
    if (y > SEEY) y--;
   }
  }
  if (terrain_type == ot_ants_new)
   rotate(3);
  if (terrain_type == ot_ants_nsw)
   rotate(2);
  if (terrain_type == ot_ants_esw)
   rotate(1);
  break;

 case ot_ants_nesw:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_rock;
  }
  x = SEEX;
  for (int j = 0; j < SEEY * 2; j++) {
   for (int i = x - 2; i <= x + 3; i++) {
    if (i >= 1 && i < SEEX * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
   x += rng(-1, 1);
   while (abs(SEEX - x) > SEEY * 2 - j - 1) {
    if (x < SEEX) x++;
    if (x > SEEX) x--;
   }
  }

  y = SEEY;
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = y - 2; j <= y + 3; j++) {
    if (j >= 1 && j < SEEY * 2 - 1)
     ter(i, j) = t_rock_floor;
   }
   y += rng(-1, 1);
   while (abs(SEEY - y) > SEEX * 2 - i - 1) {
    if (y < SEEY) y++;
    if (y > SEEY) y--;
   }
  }
  break;

 case ot_ants_food:
 case ot_ants_larvae:
 case ot_ants_queen:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (i < SEEX - 4 || i > SEEX + 5 || j < SEEY - 4 || j > SEEY + 5)
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_rock_floor;
   }
  }
  rn = rng(10, 20);
  for (int n = 0; n < rn; n++) {
   cw = rng(1, 8);
   do {
    x = rng(1 + cw, SEEX * 2 - 2 - cw);
    y = rng(1 + cw, SEEY * 2 - 2 - cw);
   } while (ter(x, y) == t_rock);
   for (int i = x - cw; i <= x + cw; i++) {
    for (int j = y - cw; j <= y + cw; j++) {
     if (trig_dist(x, y, i, j) <= cw)
      ter(i, j) = t_rock_floor;
    }
   }
  }
  if (connects_to(t_north, 2)) {
   for (int i = SEEX - 2; i <= SEEX + 3; i++) {
    for (int j = 0; j <= SEEY; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_east, 3)) {
   for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
    for (int j = SEEY - 2; j <= SEEY + 3; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_south, 0)) {
   for (int i = SEEX - 2; i <= SEEX + 3; i++) {
    for (int j = SEEY; j <= SEEY * 2 - 1; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_west, 1)) {
   for (int i = 0; i <= SEEX; i++) {
    for (int j = SEEY - 2; j <= SEEY + 3; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (terrain_type == ot_ants_food)
   place_items(mi_ant_food, 92, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  else
   place_items(mi_ant_egg,  98, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
  if (terrain_type == ot_ants_queen)
   add_spawn(mon_ant_queen, 1, SEEX, SEEY);
  else if (terrain_type == ot_ants_larvae)
   add_spawn(mon_ant_larva, 10, SEEX, SEEY);
  break;

 case ot_tutorial:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if (j == 0 || j == SEEY * 2 - 1)
     ter(i, j) = t_wall_h;
    else if (i == 0 || i == SEEX * 2 - 1)
     ter(i, j) = t_wall_v;
    else if (j == SEEY) {
     if (i % 4 == 2)
      ter(i, j) = t_door_c;
     else if (i % 5 == 3)
      ter(i, j) = t_window;
     else
      ter(i, j) = t_wall_h;
    } else
     ter(i, j) = t_floor;
   }
  }
  ter(7, SEEY * 2 - 4) = t_rack;
  ter(SEEX * 2 - 2, SEEY * 2 - 4) = t_gas_pump;
  if (t_above != ot_null) {
   ter(SEEX - 2, SEEY + 2) = t_stairs_up;
   ter(2, 2) = t_water_sh;
   ter(2, 3) = t_water_sh;
   ter(3, 2) = t_water_sh;
   ter(3, 3) = t_water_sh;
  } else
   ter(SEEX - 2, SEEY + 2) = t_stairs_down;
  break;

 case ot_cavern:
  n_fac = (t_north == ot_cavern || t_north == ot_subway_ns ||
           t_north == ot_subway_ew ? 0 : 3);
  e_fac = (t_east  == ot_cavern || t_east  == ot_subway_ns ||
           t_east  == ot_subway_ew ? SEEX * 2 - 1 : SEEX * 2 - 4);
  s_fac = (t_south == ot_cavern || t_south == ot_subway_ns ||
           t_south == ot_subway_ew ? SEEY * 2 - 1 : SEEY * 2 - 4);
  w_fac = (t_west  == ot_cavern || t_west  == ot_subway_ns ||
           t_west  == ot_subway_ew ? 0 : 3);
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    if ((j < n_fac || j > s_fac || i < w_fac || i > e_fac) &&
        (!one_in(3) || j == 0 || j == SEEY*2 - 1 || i == 0 || i == SEEX*2 - 1))
     ter(i, j) = t_rock;
    else
     ter(i, j) = t_rock_floor;
   }
  }

  rn = rng(0, 2) * rng(0, 3) + rng(0, 1);	// Number of pillars
  for (int n = 0; n < rn; n++) {
   int px = rng(5, SEEX * 2 - 6);
   int py = rng(5, SEEY * 2 - 6);
   for (int i = px - 1; i <= px + 1; i++) {
    for (int j = py - 1; j <= py + 1; j++)
     ter(i, j) = t_rock;
   }
  }

  if (connects_to(t_north, 2)) {
   for (int i = SEEX - 2; i <= SEEX + 3; i++) {
    for (int j = 0; j <= SEEY; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_east, 3)) {
   for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
    for (int j = SEEY - 2; j <= SEEY + 3; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_south, 0)) {
   for (int i = SEEX - 2; i <= SEEX + 3; i++) {
    for (int j = SEEY; j <= SEEY * 2 - 1; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  if (connects_to(t_west, 1)) {
   for (int i = 0; i <= SEEX; i++) {
    for (int j = SEEY - 2; j <= SEEY + 3; j++)
     ter(i, j) = t_rock_floor;
   }
  }
  place_items(mi_cavern, 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, 0);
  if (one_in(6)) {	// Miner remains
   int x, y;
   do {
    x = rng(0, SEEX * 2 - 1);
    y = rng(0, SEEY * 2 - 1);
   } while (move_cost(x, y) == 0);
   if (!one_in(3))
    add_item(x, y, (*itypes)[itm_jackhammer], 0);
   if (one_in(3))
    add_item(x, y, (*itypes)[itm_mask_dust], 0);
   if (one_in(2))
    add_item(x, y, (*itypes)[itm_hat_hard], 0);
   while (!one_in(3))
    add_item(x, y, (*itypes)[rng(itm_can_beans, itm_can_tuna)], 0);
  }
  break;

 default:
  debugmsg("Error: tried to generate map for omtype %d, \"%s\"", terrain_type,
           oterlist[terrain_type].name.c_str());
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++)
    ter(i, j) = t_floor;
  }
  break;
 }

// Now, fix sewers and subways so that they interconnect.

 if (terrain_type >= ot_subway_ns && terrain_type <= ot_subway_nesw) {
  if (t_north >= ot_sewer_ns && t_north <= ot_sewer_nesw &&
      !connects_to(terrain_type, 0)) {
   if (connects_to(t_north, 2)) {
    for (int i = SEEX - 2; i < SEEX + 2; i++) {
     for (int j = 0; j < SEEY; j++)
      ter(i, j) = t_sewage;
    }
   } else {
    for (int j = 0; j < 3; j++) {
     ter(SEEX, j) = t_rock_floor;
     ter(SEEX - 1, j) = t_rock_floor;
    }
    ter(SEEX, 3) = t_door_metal_c;
    ter(SEEX - 1, 3) = t_door_metal_c;
   }
  }
  if (t_east >= ot_sewer_ns && t_east <= ot_sewer_nesw &&
      !connects_to(terrain_type, 1)) {
   if (connects_to(t_east, 3)) {
    for (int i = SEEX; i < SEEX * 2; i++) {
     for (int j = SEEY - 2; j < SEEY + 2; j++)
      ter(i, j) = t_sewage;
    }
   } else {
    for (int i = SEEX * 2 - 3; i < SEEX * 2; i++) {
     ter(i, SEEY) = t_rock_floor;
     ter(i, SEEY - 1) = t_rock_floor;
    }
    ter(SEEX * 2 - 4, SEEY) = t_door_metal_c;
    ter(SEEX * 2 - 4, SEEY - 1) = t_door_metal_c;
   }
  }
  if (t_south >= ot_sewer_ns && t_south <= ot_sewer_nesw &&
      !connects_to(terrain_type, 2)) {
   if (connects_to(t_south, 0)) {
    for (int i = SEEX - 2; i < SEEX + 2; i++) {
     for (int j = SEEY; j < SEEY * 2; j++)
      ter(i, j) = t_sewage;
    }
   } else {
    for (int j = SEEY * 2 - 3; j < SEEY * 2; j++) {
     ter(SEEX, j) = t_rock_floor;
     ter(SEEX - 1, j) = t_rock_floor;
    }
    ter(SEEX, SEEY * 2 - 4) = t_door_metal_c;
    ter(SEEX - 1, SEEY * 2 - 4) = t_door_metal_c;
   }
  }
  if (t_west >= ot_sewer_ns && t_west <= ot_sewer_nesw &&
      !connects_to(terrain_type, 3)) {
   if (connects_to(t_west, 1)) {
    for (int i = 0; i < SEEX; i++) {
     for (int j = SEEY - 2; j < SEEY + 2; j++)
      ter(i, j) = t_sewage;
    }
   } else {
    for (int i = 0; i < 3; i++) {
     ter(i, SEEY) = t_rock_floor;
     ter(i, SEEY - 1) = t_rock_floor;
    }
    ter(3, SEEY) = t_door_metal_c;
    ter(3, SEEY - 1) = t_door_metal_c;
   }
  }
 } else if (terrain_type >= ot_sewer_ns && terrain_type <= ot_sewer_nesw) {
  if (t_above == ot_road_nesw_manhole)
   ter(rng(SEEX - 2, SEEX + 1), rng(SEEY - 2, SEEY + 1)) = t_ladder;
  if (t_north >= ot_subway_ns && t_north <= ot_subway_nesw &&
      !connects_to(terrain_type, 0)) {
   for (int j = 0; j < SEEY - 3; j++) {
    ter(SEEX, j) = t_rock_floor;
    ter(SEEX - 1, j) = t_rock_floor;
   }
   ter(SEEX, SEEY - 3) = t_door_metal_c;
   ter(SEEX - 1, SEEY - 3) = t_door_metal_c;
  }
  if (t_east >= ot_subway_ns && t_east <= ot_subway_nesw &&
      !connects_to(terrain_type, 1)) {
   for (int i = SEEX + 3; i < SEEX * 2; i++) {
    ter(i, SEEY) = t_rock_floor;
    ter(i, SEEY - 1) = t_rock_floor;
   }
   ter(SEEX + 2, SEEY) = t_door_metal_c;
   ter(SEEX + 2, SEEY - 1) = t_door_metal_c;
  }
  if (t_south >= ot_subway_ns && t_south <= ot_subway_nesw &&
      !connects_to(terrain_type, 2)) {
   for (int j = SEEY + 3; j < SEEY * 2; j++) {
    ter(SEEX, j) = t_rock_floor;
    ter(SEEX - 1, j) = t_rock_floor;
   }
   ter(SEEX, SEEY + 2) = t_door_metal_c;
   ter(SEEX - 1, SEEY + 2) = t_door_metal_c;
  }
  if (t_west >= ot_subway_ns && t_west <= ot_subway_nesw &&
      !connects_to(terrain_type, 3)) {
   for (int i = 0; i < SEEX - 3; i++) {
    ter(i, SEEY) = t_rock_floor;
    ter(i, SEEY - 1) = t_rock_floor;
   }
   ter(SEEX - 3, SEEY) = t_door_metal_c;
   ter(SEEX - 3, SEEY - 1) = t_door_metal_c;
  }
 } else if (terrain_type >= ot_ants_ns && terrain_type <= ot_ants_queen) {
  if (t_above == ot_anthill) {
   bool done = false;
   do {
    int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
    if (ter(x, y) == t_rock_floor) {
     done = true;
     ter(x, y) = t_slope_up;
    }
   } while (!done);
  }
 }
}

void map::place_items(items_location loc, int chance, int x1, int y1,
                      int x2, int y2, bool ongrass, int turn)
{
 std::vector<itype_id> eligible = (*mapitems)[loc];

 if (chance >= 100) {
  debugmsg("Bad place_items called (chance = %d)", chance);
  chance = 0;
 }
 if (chance == 0 || eligible.size() == 0) // No items here! (Why was it called?)
  return;
 int item_chance = 0;	// # of items
 for (int i = 0; i < eligible.size(); i++)
   item_chance += (*itypes)[eligible[i]]->rarity;
 int selection, randnum, debugrn;
 int px, py;
 while (rng(0, 99) < chance) {
  randnum = rng(1, item_chance);
  debugrn = randnum;
  selection = -1;
  while (randnum > 0) {
   selection++;
   if (selection >= eligible.size())
    debugmsg("OOB selection (%d of %d); randnum is %d, item_chance %d",
             selection, eligible.size(), randnum, item_chance);
   randnum -= (*itypes)[eligible[selection]]->rarity;
  }
  do {
   px = rng(x1, x2);
   py = rng(y1, y2);
// Only place on valid terrain
  } while ((terlist[ter(px, py)].movecost == 0 && 
            !(terlist[ter(px, py)].flags & mfb(container))) ||
           (!ongrass && (ter(px, py) == t_dirt || ter(px, py) == t_grass)));
  add_item(px, py, (*itypes)[eligible[selection]], turn);
// Guns in the home and behind counters are generated with their ammo
// TODO: Make this less of a hack
  if ((*itypes)[eligible[selection]]->is_gun() && 
      (loc == mi_homeguns || loc == mi_behindcounter)) {
   it_gun* tmpgun = dynamic_cast<it_gun*> ((*itypes)[eligible[selection]]);
   add_item(px, py, (*itypes)[default_ammo(tmpgun->ammo)], turn);
  }
 }
}

void map::add_spawn(mon_id type, int count, int x, int y)
{
 if (x < 0 || x >= SEEX * 3 || y < 0 || y >= SEEY * 3) {
  debugmsg("Bad add_spawn(%d, %d, %d, %d)", type, count, x, y);
  return;
 }
 int nonant = int(x / SEEX) + int(y / SEEY) * 3;
 x %= SEEX;
 y %= SEEY;
 spawn_point tmp(type, count, x, y);
 grid[nonant].spawns.push_back(tmp);
}

void map::make_all_items_owned()
{
 for (int i = 0; i < SEEX * 2; i++) {
  for (int j = 0; j < SEEY * 2; j++) {
   for (int n = 0; n < i_at(i, j).size(); n++)
    i_at(i, j)[n].owned = true;
  }
 }
}
   
void map::rotate(int turns)
{
 ter_id rotated[SEEX*2][SEEY*2];
 trap_id traprot[SEEX*2][SEEY*2];
 std::vector<item> itrot[SEEX*2][SEEY*2];
 std::vector<spawn_point> sprot[9];
 switch (turns) {
 case 1:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    rotated[i][j] = ter  (j, SEEX * 2 - 1 - i);
    itrot  [i][j] = i_at (j, SEEX * 2 - 1 - i);
    traprot[i][j] = tr_at(j, SEEX * 2 - 1 - i);
    i_clear(j, SEEX * 2 - 1 - i);
   }
  }
// Now, spawn points
  for (int i = 0; i < 5; i++) {
   for (int j = 0; j < grid[i].spawns.size(); j++) {
    int n;
         if (i == 0) n = 1;
    else if (i == 1) n = 4;
    else if (i == 3) n = 0;
    else if (i == 4) n = 3;
    else             debugmsg("Found weird spawn; grid %d", i);
    spawn_point tmp = grid[i].spawns[j];
    int tmpy = tmp.posy;
    tmp.posy = tmp.posx;
    tmp.posx = SEEY - 1 - tmpy;
    sprot[n].push_back(tmp);
   }
  }
    
  break;
 case 2:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    rotated[i][j] = ter  (SEEX * 2 - 1 - i, SEEY * 2 - 1 - j);
    itrot  [i][j] = i_at (SEEX * 2 - 1 - i, SEEY * 2 - 1 - j);
    traprot[i][j] = tr_at(SEEX * 2 - 1 - i, SEEY * 2 - 1 - j);
    i_clear(SEEX * 2 - 1 - i, SEEY * 2 - 1 - j);
   }
  }
// Now, spawn points
  for (int i = 0; i < 5; i++) {
   for (int j = 0; j < grid[i].spawns.size(); j++) {
    int n;
         if (i == 0) n = 4;
    else if (i == 1) n = 3;
    else if (i == 3) n = 1;
    else if (i == 4) n = 0;
    else             debugmsg("Found weird spawn; grid %d", i);
    spawn_point tmp = grid[i].spawns[j];
    int tmpy = tmp.posy;
    tmp.posy = SEEX - 1 - tmp.posx;
    tmp.posx = SEEY - 1 - tmpy;
    sprot[n].push_back(tmp);
   }
  }
    
  break;
 case 3:
  for (int i = 0; i < SEEX * 2; i++) {
   for (int j = 0; j < SEEY * 2; j++) {
    rotated[i][j] = ter  (SEEY * 2 - 1 - j, i);
    itrot  [i][j] = i_at (SEEY * 2 - 1 - j, i);
    traprot[i][j] = tr_at(SEEY * 2 - 1 - j, i);
    i_clear(SEEY * 2 - 1 - j, i);
   }
  }
// Now, spawn points
  for (int i = 0; i < 5; i++) {
   for (int j = 0; j < grid[i].spawns.size(); j++) {
    int n;
         if (i == 0) n = 3;
    else if (i == 1) n = 0;
    else if (i == 3) n = 4;
    else if (i == 4) n = 1;
    else             debugmsg("Found weird spawn; grid %d", i);
    spawn_point tmp = grid[i].spawns[j];
    int tmpy = tmp.posy;
    tmp.posy = SEEX - 1 - tmp.posx;
    tmp.posx = tmpy;
    sprot[n].push_back(tmp);
   }
  }
    
  break;
 default:
  return;
 }
// Set the spawn points
 for (int i = 0; i < 5; i++) {
  if (i != 2)
   grid[i].spawns = sprot[i];
 }
 for (int i = 0; i < SEEX * 2; i++) {
  for (int j = 0; j < SEEY * 2; j++) {
   ter  (i, j) = rotated[i][j];
   i_at (i, j) = itrot  [i][j];
   tr_at(i, j) = traprot[i][j];
   if (turns % 2 == 1) { 	// Rotate things like walls 90 degrees
    if (ter(i, j) == t_wall_v)
     ter(i, j) = t_wall_h;
    else if (ter(i, j) == t_wall_h)
     ter(i, j) = t_wall_v;
    else if (ter(i, j) == t_wall_metal_v)
     ter(i, j) = t_wall_metal_h;
    else if (ter(i, j) == t_wall_metal_h)
     ter(i, j) = t_wall_metal_v;
    else if (ter(i, j) == t_railing_v)
     ter(i, j) = t_railing_h;
    else if (ter(i, j) == t_railing_h)
     ter(i, j) = t_railing_v;
    else if (ter(i, j) == t_wall_glass_h)
     ter(i, j) = t_wall_glass_v;
    else if (ter(i, j) == t_wall_glass_v)
     ter(i, j) = t_wall_glass_h;
    else if (ter(i, j) == t_reinforced_glass_h)
     ter(i, j) = t_reinforced_glass_v;
    else if (ter(i, j) == t_reinforced_glass_v)
     ter(i, j) = t_reinforced_glass_h;
    else if (ter(i, j) == t_fence_v)
     ter(i, j) = t_fence_h;
    else if (ter(i, j) == t_fence_h)
     ter(i, j) = t_fence_v;
   }
  }
 }
}

// Hideous function, I admit...
bool connects_to(oter_id there, int dir)
{
 switch (dir) {
 case 2:
  if (there == ot_subway_ns  || there == ot_subway_es || there == ot_subway_sw||
      there == ot_subway_nes || there == ot_subway_nsw ||
      there == ot_subway_esw || there == ot_subway_nesw ||
      there == ot_sewer_ns   || there == ot_sewer_es || there == ot_sewer_sw || 
      there == ot_sewer_nes  || there == ot_sewer_nsw || there == ot_sewer_esw||
      there == ot_sewer_nesw || there == ot_ants_ns || there == ot_ants_es ||
      there == ot_ants_sw    || there == ot_ants_nes ||  there == ot_ants_nsw ||
      there == ot_ants_esw   || there == ot_ants_nesw)
   return true;
  return false;
 case 3:
  if (there == ot_subway_ew  || there == ot_subway_sw || there == ot_subway_wn||
      there == ot_subway_new || there == ot_subway_nsw ||
      there == ot_subway_esw || there == ot_subway_nesw ||
      there == ot_sewer_ew   || there == ot_sewer_sw || there == ot_sewer_wn ||
      there == ot_sewer_new  || there == ot_sewer_nsw || there == ot_sewer_esw||
      there == ot_sewer_nesw || there == ot_ants_ew || there == ot_ants_sw ||
      there == ot_ants_wn    || there == ot_ants_new || there == ot_ants_nsw ||
      there == ot_ants_esw   || there == ot_ants_nesw)
   return true;
  return false;
 case 0:
  if (there == ot_subway_ns  || there == ot_subway_ne || there == ot_subway_wn||
      there == ot_subway_nes || there == ot_subway_new ||
      there == ot_subway_nsw || there == ot_subway_nesw ||
      there == ot_sewer_ns   || there == ot_sewer_ne ||  there == ot_sewer_wn ||
      there == ot_sewer_nes  || there == ot_sewer_new || there == ot_sewer_nsw||
      there == ot_sewer_nesw || there == ot_ants_ns || there == ot_ants_ne ||
      there == ot_ants_wn    || there == ot_ants_nes || there == ot_ants_new ||
      there == ot_ants_nsw   || there == ot_ants_nesw)
   return true;
  return false;
 case 1:
  if (there == ot_subway_ew  || there == ot_subway_ne || there == ot_subway_es||
      there == ot_subway_nes || there == ot_subway_new ||
      there == ot_subway_esw || there == ot_subway_nesw ||
      there == ot_sewer_ew   || there == ot_sewer_ne || there == ot_sewer_es ||
      there == ot_sewer_nes  || there == ot_sewer_new || there == ot_sewer_esw||
      there == ot_sewer_nesw || there == ot_ants_ew || there == ot_ants_ne ||
      there == ot_ants_es    || there == ot_ants_nes || there == ot_ants_new ||
      there == ot_ants_esw   || there == ot_ants_nesw)
   return true;
  return false;
 default:
  debugmsg("Connects_to with dir of %d", dir);
  return false;
 }
}

void house_room(map *m, room_type type, int x1, int y1, int x2, int y2)
{
 for (int i = x1; i <= x2; i++) {
  for (int j = y1; j <= y2; j++) {
   if (m->ter(i, j) == t_grass || m->ter(i, j) == t_dirt ||
       m->ter(i, j) == t_floor) {
    if (j == y1 || j == y2) {
     m->ter(i, j) = t_wall_h;
     m->ter(i, j) = t_wall_h;
    } else if (i == x1 || i == x2) {
     m->ter(i, j) = t_wall_v;
     m->ter(i, j) = t_wall_v;
    } else
     m->ter(i, j) = t_floor;
   }
  }
 }
 for (int i = y1 + 1; i <= y2 - 1; i++) {
  m->ter(x1, i) = t_wall_v;
  m->ter(x2, i) = t_wall_v;
 }

 items_location placed = mi_none;
 int chance = 0, rn;
 switch (type) {
 case room_living:
  placed = mi_livingroom;
  chance = 83;
  break;
 case room_kitchen:
  placed = mi_kitchen;
  chance = 75;
  m->place_items(mi_cleaning,  58, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
  switch (rng(1, 4)) {
  case 1:
   m->ter(x1 + 2, y1 + 1) = t_fridge;
   m->place_items(mi_fridge, 82, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
   break;
  case 2:
   m->ter(x2 - 2, y1 + 1) = t_fridge;
   m->place_items(mi_fridge, 82, x2 - 2, y1 + 1, x2 - 2, y1 + 1, false, 0);
   break;
  case 3:
   m->ter(x1 + 2, y2 - 1) = t_fridge;
   m->place_items(mi_fridge, 82, x1 + 2, y2 - 1, x1 + 2, y2 - 1, false, 0);
   break;
  case 4:
   m->ter(x2 - 2, y2 - 1) = t_fridge;
   m->place_items(mi_fridge, 82, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
   break;
  }
  break;
 case room_bedroom:
  placed = mi_bedroom;
  chance = 78;
  if (one_in(10))
   m->place_items(mi_homeguns, 58, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
  switch (rng(1, 5)) {
  case 1:
   m->ter(x1 + 1, y1 + 2) = t_bed;
   m->ter(x1 + 1, y1 + 3) = t_bed;
   break;
  case 2:
   m->ter(x1 + 2, y2 - 1) = t_bed;
   m->ter(x1 + 3, y2 - 1) = t_bed;
   break;
  case 3:
   m->ter(x2 - 1, y2 - 3) = t_bed;
   m->ter(x2 - 1, y2 - 2) = t_bed;
   break;
  case 4:
   m->ter(x2 - 3, y1 + 1) = t_bed;
   m->ter(x2 - 2, y1 + 1) = t_bed;
   break;
  case 5:
   m->ter(int((x1 + x2) / 2)    , y2 - 1) = t_bed;
   m->ter(int((x1 + x2) / 2) + 1, y2 - 1) = t_bed;
   m->ter(int((x1 + x2) / 2)    , y2 - 2) = t_bed;
   m->ter(int((x1 + x2) / 2) + 1, y2 - 2) = t_bed;
   break;
  }
  switch (rng(1, 4)) {
  case 1:
   m->ter(x1 + 2, y1 + 1) = t_dresser;
   m->place_items(mi_dresser, 80, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
   break;
  case 2:
   m->ter(x2 - 2, y2 - 1) = t_dresser;
   m->place_items(mi_dresser, 80, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
   break;
  case 3:
   rn = int((x1 + x2) / 2);
   m->ter(rn, y1 + 1) = t_dresser;
   m->place_items(mi_dresser, 80, rn, y1 + 1, rn, y1 + 1, false, 0);
   break;
  case 4:
   rn = int((y1 + y2) / 2);
   m->ter(x1 + 1, rn) = t_dresser;
   m->place_items(mi_dresser, 80, x1 + 1, rn, x1 + 1, rn, false, 0);
   break;
  }
  break;
 case room_bathroom:
  m->ter(x2 - 1, y2 - 1) = t_toilet;
  m->place_items(mi_harddrugs, 18, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
  m->place_items(mi_cleaning,  48, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
  placed = mi_softdrugs;
  chance = 72;
  break;
 }
 m->place_items(placed, chance, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
}

void science_room(map *m, int x1, int y1, int x2, int y2, int rotate)
{
 int height = y2 - y1;
 int width  = x2 - x1;
 if (rotate % 2 == 1) {	// Swamp width & height if we're a lateral room
  int tmp = height;
  height  = width;
  width   = tmp;
 }
 for (int i = x1; i <= x2; i++) {
  for (int j = y1; j <= y2; j++)
   m->ter(i, j) = t_floor;
 }
 int area = height * width;
 std::vector<room_type> valid_rooms;
 if (height < 5 && width < 5)
 valid_rooms.push_back(room_closet);
 if (height > 6 && width > 3)
  valid_rooms.push_back(room_lobby);
 if (height > 4 || width > 4)
  valid_rooms.push_back(room_chemistry);
 if ((height > 7 || width > 7) && height > 2 && width > 2)
  valid_rooms.push_back(room_teleport);
 if (height > 4 && width > 4)
  valid_rooms.push_back(room_goo);
 if (height > 7 && width > 7)
  valid_rooms.push_back(room_cloning);
 if (area >= 9)
  valid_rooms.push_back(room_vivisect);
 if (height > 5 && width > 4)
  valid_rooms.push_back(room_dorm);
 if (width > 8) {
  for (int i = 8; i < width; i += rng(1, 2))
   valid_rooms.push_back(room_split);
 }

 room_type chosen = valid_rooms[rng(0, valid_rooms.size() - 1)];
 int trapx = rng(x1 + 1, x2 - 1);
 int trapy = rng(y1 + 1, y2 - 1);
 switch (chosen) {
  case room_closet:
   m->place_items(mi_cleaning, 80, x1, y1, x2, y2, false, 0);
   break;
  case room_lobby:
   if (rotate % 2 == 0)	{	// Vertical
    int desk = y1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
    for (int x = x1 + int(width / 4); x < x2 - int(width / 4); x++)
     m->ter(x, desk) = t_counter;
    m->ter(x2 - int(width / 4), desk) = t_computer_lab;
    m->add_spawn(mon_turret, 1, int((x1 + x2) / 2), desk);
   } else {
    int desk = x1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
    for (int y = y1 + int(width / 4); y < y2 - int(width / 4); y++)
     m->ter(desk, y) = t_counter;
    m->ter(desk, y2 - int(width / 4)) = t_computer_lab;
    m->add_spawn(mon_turret, 1, desk, int((y1 + y2) / 2));
   }
   break;
  case room_chemistry:
   if (rotate % 2 == 0) {	// Vertical
    for (int x = x1; x <= x2; x++) {
     if (x % 3 == 0) {
      for (int y = y1 + 1; y <= y2 - 1; y++) {
       m->ter(x, y) = t_counter;
      }
      m->place_items(mi_chemistry, 70, x, y1 + 1, x, y2 - 1, false, 0);
     }
    }
   } else {
    for (int y = y1; y <= y2; y++) {
     if (y % 3 == 0) {
      for (int x = x1 + 1; x <= x2 - 1; x++) {
       m->ter(x, y) = t_counter;
      }
      m->place_items(mi_chemistry, 70, x1 + 1, y, x2 - 1, y, false, 0);
     }
    }
   }
   break;
  case room_teleport:
   m->ter(int((x1 + x2) / 2)    , int((y1 + y2) / 2)    ) = t_counter;
   m->ter(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2)    ) = t_counter;
   m->ter(int((x1 + x2) / 2)    , int((y1 + y2) / 2) + 1) = t_counter;
   m->ter(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2) + 1) = t_counter;
   m->add_trap(trapx, trapy, tr_telepad);
   m->place_items(mi_teleport, 70, int((x1 + x2) / 2),
                  int((y1 + y2) / 2), int((x1 + x2) / 2) + 1,
                  int((y1 + y2) / 2) + 1, false, 0);
   break;
  case room_goo:
   do {
    m->add_trap(trapx, trapy, tr_goo);
    trapx = rng(x1 + 1, x2 - 1);
    trapy = rng(y1 + 1, y2 - 1);
   } while(!one_in(5));
   if (rotate == 0) {
    m->tr_at(x1, y2) = tr_null;
    m->ter(x1, y2) = t_fridge;
    m->place_items(mi_goo, 60, x1, y2, x1, y2, false, 0);
   } else if (rotate == 1) {
    m->tr_at(x1, y1) = tr_null;
    m->ter(x1, y1) = t_fridge;
    m->place_items(mi_goo, 60, x1, y1, x1, y1, false, 0);
   } else if (rotate == 2) {
    m->tr_at(x2, y1) = tr_null;
    m->ter(x2, y1) = t_fridge;
    m->place_items(mi_goo, 60, x2, y1, x2, y1, false, 0);
   } else {
    m->tr_at(x2, y2) = tr_null;
    m->ter(x2, y2) = t_fridge;
    m->place_items(mi_goo, 60, x2, y2, x2, y2, false, 0);
   }
   break;
  case room_cloning:
   for (int x = x1 + 1; x <= x2 - 1; x++) {
    for (int y = y1 + 1; y <= y2 - 1; y++) {
     if (x % 3 == 0 && y % 3 == 0) {
      m->ter(x, y) = t_vat;
      m->place_items(mi_cloning_vat, 20, x, y, x, y, false, 0);
     }
    }
   }
   break;
  case room_vivisect:
   if        (rotate == 0) {
    for (int x = x1; x <= x2; x++)
     m->ter(x, y2 - 1) = t_counter;
    m->place_items(mi_dissection, 80, x1, y2 - 1, x2, y2 - 1, false, 0);
   } else if (rotate == 1) {
    for (int y = y1; y <= y2; y++)
     m->ter(x1 + 1, y) = t_counter;
    m->place_items(mi_dissection, 80, x1 + 1, y1, x1 + 1, y2, false, 0);
   } else if (rotate == 2) {
    for (int x = x1; x <= x2; x++)
     m->ter(x, y1 + 1) = t_counter;
    m->place_items(mi_dissection, 80, x1, y1 + 1, x2, y1 + 1, false, 0);
   } else if (rotate == 3) {
    for (int y = y1; y <= y2; y++)
     m->ter(x2 - 1, y) = t_counter;
    m->place_items(mi_dissection, 80, x2 - 1, y1, x2 - 1, y2, false, 0);
   }
   m->add_trap(int((x1 + x2) / 2), int((y1 + y2) / 2), tr_dissector);
   break;
  case room_dorm:
   if (rotate % 2 == 0) {
    for (int y = y1 + 1; y <= y2 - 1; y += 3) {
     m->ter(x1    , y) = t_bed;
     m->ter(x1 + 1, y) = t_bed;
     m->ter(x2    , y) = t_bed;
     m->ter(x2 - 1, y) = t_bed;
     m->ter(x1, y + 1) = t_dresser;
     m->ter(x2, y + 1) = t_dresser;
     m->place_items(mi_dresser, 70, x1, y + 1, x1, y + 1, false, 0);
     m->place_items(mi_dresser, 70, x2, y + 1, x2, y + 1, false, 0);
    }
   } else if (rotate % 2 == 1) {
    for (int x = x1 + 1; x <= x2 - 1; x += 3) {
     m->ter(x, y1    ) = t_bed;
     m->ter(x, y1 + 1) = t_bed;
     m->ter(x, y2    ) = t_bed;
     m->ter(x, y2 - 1) = t_bed;
     m->ter(x + 1, y1) = t_dresser;
     m->ter(x + 1, y2) = t_dresser;
     m->place_items(mi_dresser, 70, x + 1, y1, x + 1, y1, false, 0);
     m->place_items(mi_dresser, 70, x + 1, y2, x + 1, y2, false, 0);
    }
   }
   m->place_items(mi_bedroom, 84, x1, y1, x2, y2, false, 0);
   break;
  case room_split:
   if (rotate % 2 == 0) {
    int w1 = int((x1 + x2) / 2) - 2, w2 = int((x1 + x2) / 2) + 2;
    for (int y = y1; y <= y2; y++) {
     m->ter(w1, y) = t_wall_v;
     m->ter(w2, y) = t_wall_v;
    }
    m->ter(w1, int((y1 + y2) / 2)) = t_door_metal_c;
    m->ter(w2, int((y1 + y2) / 2)) = t_door_metal_c;
    science_room(m, x1, y1, w1 - 1, y2, 1);
    science_room(m, w2 + 1, y1, x2, y2, 3);
   } else {
    int w1 = int((y1 + y2) / 2) - 2, w2 = int((y1 + y2) / 2) + 2;
    for (int x = x1; x <= x2; x++) {
     m->ter(x, w1) = t_wall_h;
     m->ter(x, w2) = t_wall_h;
    }
    m->ter(int((x1 + x2) / 2), w1) = t_door_metal_c;
    m->ter(int((x1 + x2) / 2), w2) = t_door_metal_c;
    science_room(m, x1, y1, x2, w1 - 1, 2);
    science_room(m, x1, w2 + 1, x2, y2, 0);
   }
   break;
 }
}

void set_science_room(map *m, int x1, int y1, bool faces_right, int turn)
{
// TODO: More types!
 int type = rng(0, 4);
 int x2 = x1 + 7;
 int y2 = y1 + 4;
 switch (type) {
 case 0:	// Empty!
  return;
 case 1: // Chemistry.
// #######.
// #.......
// #.......
// #.......
// #######.
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    if ((i == x1 || j == y1 || j == y2) && i != x1)
     m->ter(i, j) = t_counter;
   }
  }
  m->place_items(mi_chemistry,	85, x1 + 1, y1, x2 - 1, y1, false, 0);
  m->place_items(mi_chemistry,	85, x1 + 1, y2, x2 - 1, y2, false, 0);
  m->place_items(mi_chemistry,	85, x1, y1 + 1, x1, y2 - 1, false, 0);
  break;

 case 2: // Hydroponics.
// #.......
// #.~~~~~.
// #.......
// #.~~~~~.
// #.......
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    if (i == x1)
     m->ter(i, j) = t_counter;
    else if (i > x1 + 1 && i < x2 && (j == y1 + 1 || j == y2 - 1))
     m->ter(i, j) = t_water_sh;
   }
  }
  m->place_items(mi_chemistry,	80, x1, y1, x1, y2, false, turn - 50);
  m->place_items(mi_hydro,	92, x1 + 1, y1 + 1, x2 - 1, y1 + 1, false,turn);
  m->place_items(mi_hydro,	92, x1 + 1, y2 - 1, x2 - 1, y2 - 1, false,turn);
  break;

 case 3: // Electronics.
// #######.
// #.......
// #.......
// #.......
// #######.
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    if ((i == x1 || j == y1 || j == y2) && i != x1)
     m->ter(i, j) = t_counter;
   }
  }
  m->place_items(mi_electronics,85, x1 + 1, y1, x2 - 1, y1, false, turn - 50);
  m->place_items(mi_electronics,85, x1 + 1, y2, x2 - 1, y2, false, turn - 50);
  m->place_items(mi_electronics,85, x1, y1 + 1, x1, y2 - 1, false, turn - 50);
  break;

 case 4: // Monster research.
// .|.####.
// -|......
// .|......
// -|......
// .|.####.
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    if (i == x1 + 1)
     m->ter(i, j) = t_wall_glass_v;
    else if (i == x1 && (j == y1 + 1 || j == y2 - 1))
     m->ter(i, j) = t_wall_glass_h;
    else if ((j == y1 || j == y2) && i >= x1 + 3 && i <= x2 - 1)
     m->ter(i, j) = t_counter;
   }
  }
// TODO: Place a monster in the sealed areas.
  m->place_items(mi_monparts,	70, x1 + 3, y1, 2 - 1, y1, false, turn - 100);
  m->place_items(mi_monparts,	70, x1 + 3, y2, 2 - 1, y2, false, turn - 100);
  break;
 }

 if (!faces_right) { // Flip it.
  ter_id rotated[SEEX*2][SEEY*2];
  std::vector<item> itrot[SEEX*2][SEEY*2];
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    rotated[i][j] = m->ter(i, j);
    itrot[i][j] = m->i_at(i, j);
   }
  }
  for (int i = x1; i <= x2; i++) {
   for (int j = y1; j <= y2; j++) {
    m->ter(i, j) = rotated[x2 - (i - x1)][j];
    m->i_at(i, j) = itrot[x2 - (i - x1)][j];
   }
  }
 }
}

void silo_rooms(map *m)
{
 std::vector<point> rooms;
 std::vector<point> room_sizes;
 bool okay = true;
 do {
  int x, y, height, width;
  if (one_in(2)) {	// True = top/bottom, False = left/right
   x = rng(0, SEEX * 2 - 6);
   y = rng(0, 4);
   if (one_in(2))
    y = SEEY * 2 - 2 - y;	// Bottom of the screen, not the top
   width  = rng(2, 5);
   height = 2;
   if (x + width >= SEEX * 2 - 1)
    width = SEEX * 2 - 2 - x;	// Make sure our room isn't too wide
  } else {
   x = rng(0, 4);
   y = rng(0, SEEY * 2 - 6);
   if (one_in(2))
    x = SEEX * 2 - 2 - x;	// Right side of the screen, not the left
   width  = 2;
   height = rng(2, 5);
   if (y + height >= SEEY * 2 - 1)
    height = SEEY * 2 - 2 - y;	// Make sure our room isn't too tall
  }
  if (!rooms.empty() &&	// We need at least one room!
      (m->ter(x, y) != t_rock || m->ter(x + width, y + height) != t_rock))
   okay = false;
  else {
   rooms.push_back(point(x, y));
   room_sizes.push_back(point(width, height));
   for (int i = x; i <= x + width; i++) {
    for (int j = y; j <= y + height; j++) {
     if (m->ter(i, j) == t_rock)
      m->ter(i, j) = t_floor;
    }
   }
   items_location used1 = mi_none, used2 = mi_none;
   switch (rng(1, 14)) {	// What type of items go here?
    case  1:
    case  2: used1 = mi_cannedfood;
             used2 = mi_fridge;		break;
    case  3:
    case  4: used1 = mi_tools;		break;
    case  5:
    case  6: used1 = mi_allguns;
             used2 = mi_ammo;		break;
    case  7: used1 = mi_allclothes;	break;
    case  8: used1 = mi_manuals;	break;
    case  9:
    case 10:
    case 11: used1 = mi_electronics;	break;
    case 12: used1 = mi_survival_tools;	break;
    case 13:
    case 14: used1 = mi_radio;		break;
   }
   if (used1 != mi_none)
    m->place_items(used1, 78, x, y, x + width, y + height, false, 0);
   if (used2 != mi_none)
    m->place_items(used2, 64, x, y, x + width, y + height, false, 0);
  }
 } while (okay);
 debugmsg("%d rooms", rooms.size());

 m->ter(rooms[0].x, rooms[0].y) = t_stairs_up;
 int down_room = rng(0, rooms.size() - 1);
 point dp = rooms[down_room], ds = room_sizes[down_room];
 m->ter(dp.x + ds.x, dp.y + ds.y) = t_stairs_down;
 rooms.push_back(point(SEEX, SEEY)); // So the center circle gets connected
 room_sizes.push_back(point(5, 5));

 while (rooms.size() > 1) {
  int best_dist = 999, closest = 0;
  for (int i = 1; i < rooms.size(); i++) {
   int dist = trig_dist(rooms[0].x, rooms[0].y, rooms[i].x, rooms[i].y);
   if (dist < best_dist) {
    best_dist = dist;
    closest = i;
   }
  }
// We chose the closest room; now draw a corridor there
  point origin = rooms[0], origsize = room_sizes[0], dest = rooms[closest];
  int x = origin.x + origsize.x, y = origin.y + origsize.y;
  bool x_first = (abs(origin.x - dest.x) > abs(origin.y - dest.y));
  while (x != dest.x || y != dest.y) {
   if (m->ter(x, y) == t_rock)
    m->ter(x, y) = t_floor;
   if ((x_first && x != dest.x) || (!x_first && y == dest.y)) {
    if (dest.x < x)
     x--;
    else
     x++;
   } else {
    if (dest.y < y)
     y--;
    else
     y++;
   }
  }
  rooms.erase(rooms.begin());
  room_sizes.erase(room_sizes.begin());
 }
}
