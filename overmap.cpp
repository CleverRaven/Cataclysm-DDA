#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <vector>
#include <sstream>
#include "overmap.h"
#include "rng.h"
#include "line.h"
#include "settlement.h"
#include "game.h"
#include "npc.h"
#include "keypress.h"

#define STREETCHANCE 2
#define NUM_FOREST 250
#define TOP_HIWAY_DIST 140
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16
#define SETTLE_DICE 2
#define SETTLE_SIDES 2
#define HIVECHANCE 180	//Chance that any given forest will be a hive
#define SWAMPINESS 8	//Affects the size of a swamp
#define SWAMPCHANCE 850	// Chance that a swamp will spawn instead of forest


void settlement_building(settlement &set, int x, int y);

double dist(int x1, int y1, int x2, int y2)
{
 return sqrt(pow(x1-x2, 2) + pow(y1-y2, 2));
}

bool is_river(oter_id ter)
{
 if (ter == ot_null || (ter >= ot_bridge_ns && ter <= ot_river_nw))
  return true;
 return false;
}

bool is_ground(oter_id ter)
{
 if (ter <= ot_road_nesw_manhole)
  return true;
 return false;
}

bool is_wall_material(oter_id ter)
{
 if (is_ground(ter) ||
     (ter >= ot_house_north && ter <= ot_nuke_plant))
  return true;
 return false;
}
 
oter_id shop(int dir)
{
 oter_id ret = ot_s_lot;
 switch (rng(0, 11)) {
  case  0: ret = ot_s_lot;	       break;
  case  1: ret = ot_s_gas_north;       break;
  case  2: ret = ot_s_pharm_north;     break;
  case  3: ret = ot_s_grocery_north;   break;
  case  4: ret = ot_s_hardware_north;  break;
  case  5: ret = ot_s_sports_north;    break;
  case  6: ret = ot_s_liquor_north;    break;
  case  7: ret = ot_s_gun_north;       break;
  case  8: ret = ot_s_clothes_north;   break;
  case  9: ret = ot_s_library_north;   break;
  case 10: ret = ot_sub_station_north; break;
  case 11: ret = ot_police_north;      break;
 }
 if (ret == ot_s_lot)
  return ret;
 if (dir < 0) dir += 4;
 switch (dir) {
  case 0:                         break;
  case 1: ret = oter_id(ret + 1); break;
  case 2: ret = oter_id(ret + 2); break;
  case 3: ret = oter_id(ret + 3); break;
  default: debugmsg("Bad rotation of shop."); return ot_null;
 }
 return ret;
}

oter_id house(int dir)
{
 bool base = one_in(30);
 if (dir < 0) dir += 4;
 switch (dir) {
  case 0:  return base ? ot_house_base_north : ot_house_north;
  case 1:  return base ? ot_house_base_east  : ot_house_east;
  case 2:  return base ? ot_house_base_south : ot_house_south;
  case 3:  return base ? ot_house_base_west  : ot_house_west;
  default: debugmsg("Bad rotation of house."); return ot_null;
 }
}

// *** BEGIN overmap FUNCTIONS ***

overmap::overmap()
{
// debugmsg("Warning - null overmap!");
 nullret = ot_null;
 posx = 999;
 posy = 999;
 posz = 999;
 if (num_ter_types > 256)
  debugmsg("More than 256 oterid!  Saving won't work!");
}

overmap::overmap(game *g, int x, int y, int z)
{
 if (num_ter_types > 256)
  debugmsg("More than 256 oterid!  Saving won't work!");
 nullret = ot_null;
 open(g, x, y, z);
}

overmap::~overmap()
{
}

oter_id& overmap::ter(int x, int y)
{
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  nullret = ot_null;
  return nullret;
 }
 return t[x][y];
}

std::vector<mongroup> overmap::monsters_at(int x, int y)
{
 std::vector<mongroup> ret;
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY)
  return ret;
 for (int i = 0; i < zg.size(); i++) {
  if (trig_dist(x, y, zg[i].posx, zg[i].posy) <= zg[i].radius)
   ret.push_back(zg[i]);
 }
 return ret;
}

bool& overmap::seen(int x, int y)
{
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  nullbool = false;
  return nullbool;
 }
 return s[x][y];
}

bool overmap::has_note(int x, int y)
{
 for (int i = 0; i < notes.size(); i++) {
  if (notes[i].x == x && notes[i].y == y)
   return true;
 }
 return false;
}

std::string overmap::note(int x, int y)
{
 for (int i = 0; i < notes.size(); i++) {
  if (notes[i].x == x && notes[i].y == y)
   return notes[i].text;
 }
 return "";
}

void overmap::add_note(int x, int y, std::string message)
{
 for (int i = 0; i < notes.size(); i++) {
  if (notes[i].x == x && notes[i].y == y) {
   if (message == "")
    notes.erase(notes.begin() + i);
   else
    notes[i].text = message;
   return;
  }
 }
 if (message.length() > 0)
  notes.push_back(om_note(x, y, notes.size(), message));
}

point overmap::find_note(point origin, std::string text)
{
 int closest = 9999;
 point ret(-1, -1);
 for (int i = 0; i < notes.size(); i++) {
  if (notes[i].text.find(text) != std::string::npos &&
      rl_dist(origin.x, origin.y, notes[i].x, notes[i].y) < closest) {
   closest = rl_dist(origin.x, origin.y, notes[i].x, notes[i].y);
   ret = point(notes[i].x, notes[i].y);
  }
 }
 return ret;
}

void overmap::generate(game *g, overmap* north, overmap* east, overmap* south,
                       overmap* west)
{
 erase();
 clear();
 move(0, 0);
 for (int i = 0; i < OMAPY; i++) {
  for (int j = 0; j < OMAPX; j++) {
   ter(i, j) = ot_field;
   seen(i, j) = false;
  }
 }
 std::vector<city> road_points;	// cities and roads_out together
 std::vector<point> river_start;// West/North endpoints of rivers
 std::vector<point> river_end;	// East/South endpoints of rivers

// Determine points where rivers & roads should connect w/ adjacent maps
 if (north != NULL) {
  for (int i = 2; i < OMAPX - 2; i++) {
   if (is_river(north->ter(i,OMAPY-1)))
    ter(i, 0) = ot_river_center;
   if (north->ter(i,     OMAPY - 1) == ot_river_center &&
       north->ter(i - 1, OMAPY - 1) == ot_river_center &&
       north->ter(i + 1, OMAPY - 1) == ot_river_center) {
    if (river_start.size() == 0 ||
        river_start[river_start.size() - 1].x < i - 6)
     river_start.push_back(point(i, 0));
   }
  }
  for (int i = 0; i < north->roads_out.size(); i++) {
   if (north->roads_out[i].y == OMAPY - 1)
    roads_out.push_back(city(north->roads_out[i].x, 0, 0));
  }
 }
 int rivers_from_north = river_start.size();
 if (west != NULL) {
  for (int i = 2; i < OMAPY - 2; i++) {
   if (is_river(west->ter(OMAPX - 1, i)))
    ter(0, i) = ot_river_center;
   if (west->ter(OMAPX - 1, i)     == ot_river_center &&
       west->ter(OMAPX - 1, i - 1) == ot_river_center &&
       west->ter(OMAPX - 1, i + 1) == ot_river_center) {
    if (river_start.size() == rivers_from_north ||
        river_start[river_start.size() - 1].y < i - 6)
     river_start.push_back(point(0, i));
   }
  }
  for (int i = 0; i < west->roads_out.size(); i++) {
   if (west->roads_out[i].x == OMAPX - 1)
    roads_out.push_back(city(0, west->roads_out[i].y, 0));
  }
 }
 if (south != NULL) {
  for (int i = 2; i < OMAPX - 2; i++) {
   if (is_river(south->ter(i, 0)))
    ter(i, OMAPY - 1) = ot_river_center;
   if (south->ter(i,     0) == ot_river_center &&
       south->ter(i - 1, 0) == ot_river_center &&
       south->ter(i + 1, 0) == ot_river_center) {
    if (river_end.size() == 0 ||
        river_end[river_end.size() - 1].x < i - 6)
     river_end.push_back(point(i, OMAPY - 1));
   }
   if (south->ter(i, 0) == ot_road_nesw)
    roads_out.push_back(city(i, OMAPY - 1, 0));
  }
  for (int i = 0; i < south->roads_out.size(); i++) {
   if (south->roads_out[i].y == 0)
    roads_out.push_back(city(south->roads_out[i].x, OMAPY - 1, 0));
  }
 }
 int rivers_to_south = river_end.size();
 if (east != NULL) {
  for (int i = 2; i < OMAPY - 2; i++) {
   if (is_river(east->ter(0, i)))
    ter(OMAPX - 1, i) = ot_river_center;
   if (east->ter(0, i)     == ot_river_center &&
       east->ter(0, i - 1) == ot_river_center &&
       east->ter(0, i + 1) == ot_river_center) {
    if (river_end.size() == rivers_to_south ||
        river_end[river_end.size() - 1].y < i - 6)
     river_end.push_back(point(OMAPX - 1, i));
   }
   if (east->ter(0, i) == ot_road_nesw)
    roads_out.push_back(city(OMAPX - 1, i, 0));
  }
  for (int i = 0; i < east->roads_out.size(); i++) {
   if (east->roads_out[i].x == 0)
    roads_out.push_back(city(OMAPX - 1, east->roads_out[i].y, 0));
  }
 }
// Even up the start and end points of rivers. (difference of 1 is acceptable)
// Also ensure there's at least one of each.
 std::vector<point> new_rivers;
 if (north == NULL || west == NULL) {
  while (river_start.empty() || river_start.size() + 1 < river_end.size()) {
   new_rivers.clear();
   if (north == NULL)
    new_rivers.push_back( point(rng(10, OMAPX - 11), 0) );
   if (west == NULL)
    new_rivers.push_back( point(0, rng(10, OMAPY - 11)) );
   river_start.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }
 if (south == NULL || east == NULL) {
  while (river_end.empty() || river_end.size() + 1 < river_start.size()) {
   new_rivers.clear();
   if (south == NULL)
    new_rivers.push_back( point(rng(10, OMAPX - 11), OMAPY - 1) );
   if (east == NULL)
    new_rivers.push_back( point(OMAPX - 1, rng(10, OMAPY - 11)) );
   river_end.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }
// Now actually place those rivers.
 if (river_start.size() > river_end.size() && river_end.size() > 0) {
  std::vector<point> river_end_copy = river_end;
  int index;
  while (!river_start.empty()) {
   index = rng(0, river_start.size() - 1);
   if (!river_end.empty()) {
    place_river(river_start[index], river_end[0]);
    river_end.erase(river_end.begin());
   } else {
    mvprintw(0, 0, "%d   ", river_end_copy.size());
    getch();
    place_river(river_start[index],
                river_end_copy[rng(0, river_end_copy.size() - 1)]);
   }
   river_start.erase(river_start.begin() + index);
  }
 } else if (river_end.size() > river_start.size() && river_start.size() > 0) {
  std::vector<point> river_start_copy = river_start;
  int index;
  while (!river_end.empty()) {
   index = rng(0, river_end.size() - 1);
   if (!river_start.empty()) {
    place_river(river_start[0], river_end[index]);
    river_start.erase(river_start.begin());
   } else {
    mvprintw(0, 0, "%d   ", river_start_copy.size());
    getch();
    place_river(river_start_copy[rng(0, river_start_copy.size() - 1)],
                river_end[index]);
   }
   river_end.erase(river_end.begin() + index);
  }
 } else if (river_end.size() > 0) {
  if (river_start.size() != river_end.size())
   debugmsg("river_start.size() = %d; river_end.size() = %d; not equal??",
             river_start.size(),      river_end.size());
  for (int i = 0; i < river_start.size(); i++)
   place_river(river_start[i], river_end[i]);
 }
    
// Cities, forests, and settlements come next.
// These're agnostic of adjacent maps, so it's very simple.
 int mincit = 0;
 if (north == NULL && east == NULL && west == NULL && south == NULL)
  mincit = 1;	// The first map MUST have a city, for the player to start in!
 place_cities(cities, mincit);
 place_forest();

// Ideally we should have at least two exit points for roads, on different sides
 if (roads_out.size() < 2) { 
  std::vector<city> viable_roads;
  int tmp;
// Populate viable_roads with one point for each neighborless side.
// Make sure these points don't conflict with rivers. 
// TODO: In theory this is a potential infinte loop...
  if (north == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, 0)) || is_river(ter(tmp - 1, 0)) ||
          is_river(ter(tmp + 1, 0)) );
   viable_roads.push_back(city(tmp, 0, 0));
  }
  if (east == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(OMAPX - 1, tmp)) || is_river(ter(OMAPX - 1, tmp - 1))||
          is_river(ter(OMAPX - 1, tmp + 1)));
   viable_roads.push_back(city(OMAPX - 1, tmp, 0));
  }
  if (south == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, OMAPY - 1)) || is_river(ter(tmp - 1, OMAPY - 1))||
          is_river(ter(tmp + 1, OMAPY - 1)));
   viable_roads.push_back(city(tmp, OMAPY - 1, 0));
  }
  if (west == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(0, tmp)) || is_river(ter(0, tmp - 1)) ||
          is_river(ter(0, tmp + 1)));
   viable_roads.push_back(city(0, tmp, 0));
  }
  while (roads_out.size() < 2 && !viable_roads.empty()) {
   tmp = rng(0, viable_roads.size() - 1);
   roads_out.push_back(viable_roads[tmp]);
   viable_roads.erase(viable_roads.begin() + tmp);
  }
 }
// Compile our master list of roads; it's less messy if road_points is first
 for (int i = 0; i < roads_out.size(); i++)
  road_points.push_back(roads_out[i]);
 for (int i = 0; i < cities.size(); i++)
  road_points.push_back(cities[i]);
// And finally connect them via "highways"
 place_hiways(road_points, ot_road_nesw);
// Make the roads out road points;
 for (int i = 0; i < roads_out.size(); i++)
  ter(roads_out[i].x, roads_out[i].y) = ot_road_nesw;
// Clean up our roads and rivers
 polish();
// Place the monsters, now that the terrain is laid out
 place_mongroups();
// We need to place settlements last, because people settle where there are no
// monsters!
 place_settlements(g);
 place_radios();
 place_NPCs(g);
}

void overmap::generate_sub(overmap* above)
{
 std::vector<city> subway_points;
 std::vector<city> sewer_points;
 std::vector<city> ant_points;
 std::vector<city> goo_points;
 std::vector<city> lab_points;
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   seen(i, j) = false;	// Start by setting all squares to unseen
   ter(i, j) = ot_rock;	// Start by setting everything to solid rock
   if (above->ter(i, j) >= ot_sub_station_north &&
       above->ter(i, j) <= ot_sub_station_west) {
    ter(i, j) = ot_subway_nesw;
    subway_points.push_back(city(i, j, 0));
   } else if (above->ter(i, j) == ot_road_nesw_manhole) {
    ter(i, j) = ot_sewer_nesw;
    sewer_points.push_back(city(i, j, 0));
   } else if (above->ter(i, j) == ot_anthill) {
    int size = rng(MIN_ANT_SIZE, MAX_ANT_SIZE);
    ant_points.push_back(city(i, j, size));
    zg.push_back(mongroup(mcat_ant, i * 2, j * 2, size * 1.5, rng(6000, 8000)));
   } else if (above->ter(i, j) == ot_slimepit_down) {
    int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
    goo_points.push_back(city(i, j, size));
   } else if (above->ter(i, j) == ot_forest_water)
    ter(i, j) = ot_cavern;
   else if (above->ter(i, j) == ot_lab_core ||
            (posz == -1 && above->ter(i, j) == ot_lab_stairs))
    lab_points.push_back(city(i, j, rng(1, 5 + posz)));
   else if (above->ter(i, j) == ot_lab_stairs)
    ter(i, j) = ot_lab;
   else if (above->ter(i, j) == ot_silo) {
    if (rng(2, 7) < abs(posz) || rng(2, 7) < abs(posz))
     ter(i, j) = ot_silo_finale;
    else
     ter(i, j) = ot_silo;
   }
  }
 }
 for (int i = 0; i < goo_points.size(); i++)
  build_slimepit(goo_points[i].x, goo_points[i].y, goo_points[i].s);
 place_hiways(sewer_points,  ot_sewer_nesw);
 polish(ot_sewer_ns, ot_sewer_nesw);
 place_hiways(subway_points, ot_subway_nesw);
 for (int i = 0; i < subway_points.size(); i++)
  ter(subway_points[i].x, subway_points[i].y) = ot_subway_station;
 for (int i = 0; i < lab_points.size(); i++)
  build_lab(lab_points[i].x, lab_points[i].y, lab_points[i].s);
 for (int i = 0; i < ant_points.size(); i++)
  build_anthill(ant_points[i].x, ant_points[i].y, ant_points[i].s);
 for (int i = 0; i < above->cities.size(); i++) {
  if (one_in(3))
   zg.push_back(
    mongroup(mcat_chud, above->cities[i].x * 2, above->cities[i].y * 2,
             above->cities[i].s, above->cities[i].s * 20));
  if (!one_in(8))
   zg.push_back(
    mongroup(mcat_sewer, above->cities[i].x * 2, above->cities[i].y * 2,
             above->cities[i].s * 3.5, above->cities[i].s * 70));
 }
 place_rifts();
 polish(ot_subway_ns, ot_subway_nesw);
 polish(ot_ants_ns, ot_ants_nesw);
// Basements done last so sewers, etc. don't overwrite them
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (above->ter(i, j) >= ot_house_base_north &&
       above->ter(i, j) <= ot_house_base_west)
    ter(i, j) = ot_basement;
  }
 }
}

void overmap::make_tutorial()
{
 if (posz == 9) {
  for (int i = 0; i < OMAPX; i++) {
   for (int j = 0; j < OMAPY; j++)
    ter(i, j) = ot_rock;
  }
 }
 ter(50, 50) = ot_tutorial;
 zg.clear();
}

point overmap::find_closest(point origin, oter_id type, int type_range,
                            int &dist, bool must_be_seen)
{
 int max = (dist == 0 ? OMAPX / 2 : dist);
 for (dist = 0; dist <= max; dist++) {
  for (int x = origin.x - dist; x <= origin.x + dist; x++) {
   for (int y = origin.y - dist; y <= origin.y + dist; y++) {
    if (ter(x, y) >= type && ter(x, y) < type + type_range &&
        (!must_be_seen || seen(x, y)))
     return point(x, y);
   }
  }
 }
 return point(-1, -1);
}

point overmap::choose_point(game *g)
{
 timeout(BLINK_SPEED);	// Enable blinking!
 WINDOW* w_map = newwin(25, 80, 0, 0);
 bool legend = true, blink = true, note_here = false, npc_here = false;
 std::string note_text, npc_name;
 int cursx = (g->levx + 1) / 2, cursy = (g->levy + 1) / 2;
 int origx = cursx, origy = cursy;
 char ch;
 overmap hori, vert, diag;
 point ret(-1, -1);
 point target(-1, -1);
 if (g->u.active_mission >= 0 &&
     g->u.active_mission < g->u.active_missions.size())
  target = g->u.active_missions[g->u.active_mission].target;
 do {
  int omx, omy;
  bool see, target_drawn;
  oter_id cur_ter;
  nc_color ter_color;
  long ter_sym;
/* First, determine if we're close enough to the edge to need to load an
 * adjacent overmap, and load it/them. */
  if (cursx < 25) {
   hori = overmap(g, posx - 1, posy, posz);
   if (cursy < 12)
    diag = overmap(g, posx - 1, posy - 1, posz);
   if (cursy > OMAPY - 14)
    diag = overmap(g, posx - 1, posy + 1, posz);
  }
  if (cursx > OMAPX - 26) {
   hori = overmap(g, posx + 1, posy, posz);
   if (cursy < 12)
    diag = overmap(g, posx + 1, posy - 1, posz);
   if (cursy > OMAPY - 14)
    diag = overmap(g, posx + 1, posy + 1, posz);
  }
  if (cursy < 12)
   vert = overmap(g, posx, posy - 1, posz);
  if (cursy > OMAPY - 14)
   vert = overmap(g, posx, posy + 1, posz);

// Now actually draw the map
  for (int i = -25; i < 25; i++) {
   for (int j = -12; j <= (ch == 'j' ? 13 : 12); j++) {
    target_drawn = false;
    omx = cursx + i;
    omy = cursy + j;
    see = false;
    npc_here = false;
    if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) { // It's in-bounds
     cur_ter = ter(omx, omy);
     see = seen(omx, omy);
     if (note_here = has_note(omx, omy))
      note_text = note(omx, omy);
     for (int n = 0; n < npcs.size(); n++) {
      if ((npcs[n].mapx + 1) / 2 == omx && (npcs[n].mapy + 1) / 2 == omy) {
       npc_here = true;
       npc_name = npcs[n].name;
       n = npcs.size();
      } else {
       npc_here = false;
       npc_name = "";
      }
     }
// <Out of bounds placement>
    } else if (omx < 0) {
     omx += OMAPX;
     if (omy < 0 || omy >= OMAPY) {
      omy += (omy < 0 ? OMAPY : 0 - OMAPY);
      cur_ter = diag.ter(omx, omy);
      see = diag.seen(omx, omy);
      if (note_here = diag.has_note(omx, omy))
       note_text = diag.note(omx, omy);
     } else {
      cur_ter = hori.ter(omx, omy);
      see = hori.seen(omx, omy);
      if (note_here = hori.has_note(omx, omy))
       note_text = hori.note(omx, omy);
     }
    } else if (omx >= OMAPX) {
     omx -= OMAPX;
     if (omy < 0 || omy >= OMAPY) {
      omy += (omy < 0 ? OMAPY : 0 - OMAPY);
      cur_ter = diag.ter(omx, omy);
      see = diag.seen(omx, omy);
      if (note_here = diag.has_note(omx, omy))
       note_text = diag.note(omx, omy);
     } else {
      cur_ter = hori.ter(omx, omy);
      see = hori.seen(omx, omy);
      if (note_here = hori.has_note(omx, omy))
       note_text = hori.note(omx, omy);
     }
    } else if (omy < 0) {
     omy += OMAPY;
     cur_ter = vert.ter(omx, omy);
     see = vert.seen(omx, omy);
     if (note_here = vert.has_note(omx, omy))
      note_text = vert.note(omx, omy);
    } else if (omy >= OMAPY) {
     omy -= OMAPY;
     cur_ter = vert.ter(omx, omy);
     see = vert.seen(omx, omy);
     if (note_here = vert.has_note(omx, omy))
      note_text = vert.note(omx, omy);
    } else
     debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
// </Out of bounds replacement>
    if (see) {
     if (note_here && blink) {
      ter_color = c_yellow;
      ter_sym = 'N';
     } else if (omx == origx && omy == origy && blink) {
      ter_color = g->u.color();
      ter_sym = '@';
     } else if (npc_here && blink) {
      ter_color = c_pink;
      ter_sym = '@';
     } else if (omx == target.x && omy == target.y && blink) {
      ter_color = c_red;
      ter_sym = '*';
     } else {
      if (cur_ter >= num_ter_types || cur_ter < 0)
       debugmsg("Bad ter %d (%d, %d)", cur_ter, omx, omy);
      ter_color = oterlist[cur_ter].color;
      ter_sym = oterlist[cur_ter].sym;
     }
    } else { // We haven't explored this tile yet
     ter_color = c_dkgray;
     ter_sym = '#';
    }
    if (j == 0 && i == 0)
     mvwputch_hi (w_map, 12,     25,     ter_color, ter_sym);
    else
     mvwputch    (w_map, 12 + j, 25 + i, ter_color, ter_sym);
   }
  }
  if (target.x != -1 && target.y != -1 && blink &&
      (target.x < cursx - 25 || target.x > cursx + 25  ||
       target.y < cursy - 12 || target.y > cursy + 12    )) {
   switch (direction_from(cursx, cursy, target.x, target.y)) {
    case NORTH:      mvwputch(w_map,  0, 25, c_red, '^');       break;
    case NORTHEAST:  mvwputch(w_map,  0, 49, c_red, LINE_OOXX); break;
    case EAST:       mvwputch(w_map, 12, 49, c_red, '>');       break;
    case SOUTHEAST:  mvwputch(w_map, 24, 49, c_red, LINE_XOOX); break;
    case SOUTH:      mvwputch(w_map, 24, 25, c_red, 'v');       break;
    case SOUTHWEST:  mvwputch(w_map, 24,  0, c_red, LINE_XXOO); break;
    case WEST:       mvwputch(w_map, 12,  0, c_red, '<');       break;
    case NORTHWEST:  mvwputch(w_map,  0,  0, c_red, LINE_OXXO); break;
   }
  }
  if (has_note(cursx, cursy)) {
   note_text = note(cursx, cursy);
   for (int i = 0; i < note_text.length(); i++)
    mvwputch(w_map, 1, i, c_white, LINE_OXOX);
   mvwputch(w_map, 1, note_text.length(), c_white, LINE_XOOX);
   mvwputch(w_map, 0, note_text.length(), c_white, LINE_XOXO);
   mvwprintz(w_map, 0, 0, c_yellow, note_text.c_str());
  } else if (npc_here) {
   for (int i = 0; i < npc_name.length(); i++)
    mvwputch(w_map, 1, i, c_white, LINE_OXOX);
   mvwputch(w_map, 1, npc_name.length(), c_white, LINE_XOOX);
   mvwputch(w_map, 0, npc_name.length(), c_white, LINE_XOXO);
   mvwprintz(w_map, 0, 0, c_yellow, npc_name.c_str());
  }
  if (legend) {
   cur_ter = ter(cursx, cursy);
// Draw the vertical line
   for (int j = 0; j < 25; j++)
    mvwputch(w_map, j, 51, c_white, LINE_XOXO);
// Clear the legend
   for (int i = 51; i < 80; i++) {
    for (int j = 0; j < 25; j++)
     mvwputch(w_map, j, i, c_black, 'x');
   }

   if (seen(cursx, cursy)) {
    mvwputch(w_map, 1, 51, oterlist[cur_ter].color, oterlist[cur_ter].sym);
    mvwprintz(w_map, 1, 53, oterlist[cur_ter].color, "%s",
              oterlist[cur_ter].name.c_str());
   } else
    mvwprintz(w_map, 1, 51, c_dkgray, "# Unexplored");

   if (target.x != -1 && target.y != -1) {
    int distance = rl_dist(origx, origy, target.x, target.y);
    mvwprintz(w_map, 3, 51, c_white, "Distance to target: %d", distance);
   }
   mvwprintz(w_map, 19, 51, c_magenta,           "Use movement keys to pan.  ");
   mvwprintz(w_map, 20, 51, c_magenta,           "0 - Center map on character");
   mvwprintz(w_map, 21, 51, c_magenta,           "t - Toggle legend          ");
   mvwprintz(w_map, 22, 51, c_magenta,           "/ - Search                 ");
   mvwprintz(w_map, 23, 51, c_magenta,           "N - Add a note             ");
   mvwprintz(w_map, 24, 51, c_magenta,           "Esc or q - Return to game  ");
  }
// Done with all drawing!
  wrefresh(w_map);
  ch = input();

  int dirx, diry;
  if (ch != ERR)
   blink = true;	// If any input is detected, make the blinkies on
  get_direction(dirx, diry, ch);
  if (dirx != -2 && diry != -2) {
   cursx += dirx;
   cursy += diry;
  } else if (ch == '0') {
   cursx = origx;
   cursy = origy;
  } else if (ch == '\n')
   ret = point(cursx, cursy);
  else if (ch == KEY_ESCAPE || ch == 'q' || ch == 'Q')
   ret = point(-1, -1);
  else if (ch == 'N') {
   timeout(-1);
   add_note(cursx, cursy, string_input_popup("Enter note:"));
   timeout(BLINK_SPEED);
  } else if (ch == '/') {
   timeout(-1);
   std::string term = string_input_popup("Search term:");
   timeout(BLINK_SPEED);
   int range = 1;
   point found = find_note(point(cursx, cursy), term);
   if (found.x == -1) {	// Didn't find a note
    for (int i = 0; i < num_ter_types; i++) {
     if (oterlist[i].name.find(term) != std::string::npos) {
      if (i == ot_forest || i == ot_hive || i == ot_hiway_ns ||
          i == ot_bridge_ns)
       range = 2;
      else if (i >= ot_road_ns && i < ot_road_nesw_manhole)
       range = ot_road_nesw_manhole - i + 1;
      else if (i >= ot_river_center && i < ot_river_nw)
       range = ot_river_nw - i + 1;
      else if (i >= ot_house_north && i < ot_lab)
       range = 4;
      else if (i == ot_lab)
       range = 2;
      int maxdist = OMAPX;
      found = find_closest(point(cursx,cursy), oter_id(i), range, maxdist,true);
      i = num_ter_types;
     }
    }
   }
   if (found.x != -1) {
    cursx = found.x;
    cursy = found.y;
   }
  }/* else if (ch == 't')  *** Legend always on for now! ***
   legend = !legend;
*/
  else if (ch == ERR)	// Hit timeout on input, so make characters blink
   blink = !blink;
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && ch != ' ' && ch != '\n');
 timeout(-1);
 werase(w_map);
 wrefresh(w_map);
 delwin(w_map);
 erase();
 g->refresh_all();
 return ret;
}
 

void overmap::first_house(int &x, int &y)
{
 int startx = rng(1, OMAPX - 1);
 int starty = rng(1, OMAPY - 1);
 while (ter(startx, starty) < ot_house_base_north ||
        ter(startx, starty) > ot_house_base_west) {
  startx = rng(1, OMAPX - 1);
  starty = rng(1, OMAPY - 1);
 }

 x = startx;
 y = starty;
}
  
void overmap::place_forest()
{
 int x, y;
 int forx;
 int fory;
 int fors;
 for (int i = 0; i < NUM_FOREST; i++) {
// forx and fory determine the epicenter of the forest
  forx = rng(0, OMAPX - 1);
  fory = rng(0, OMAPY - 1);
// fors determinds its basic size
  fors = rng(15, 40);
  for (int j = 0; j < cities.size(); j++) {
   while (dist(forx,fory,cities[j].x,cities[j].y) - fors / 2 < cities[j].s ) {
// Set forx and fory far enough from cities
    forx = rng(0, OMAPX - 1);
    fory = rng(0, OMAPY - 1);
// Set fors to determine the size of the forest; usually won't overlap w/ cities
    fors = rng(15, 40);
    j = 0;
   }
  } 
  int swamps = SWAMPINESS;	// How big the swamp may be...
  x = forx;
  y = fory;
// Depending on the size on the forest...
  for (int j = 0; j < fors; j++) {
   if (one_in(HIVECHANCE)) {	// Sometimes, a hive instead of a forest
    int hivesize = 0;
    for (int k = -1; k <= 1; k++) {
     for (int l = -1; l <= 1; l++) {
      if (ter(x + k, y + l) == ot_field || ter(x + k, y + l) == ot_forest ||
          ter(x + k, y + l) == ot_forest_thick ||
          ter(x + k, y + l) == ot_forest_water)
       ter(x + k, y + l) = ot_hive;
      if (ter(x + k, y + l) == ot_hive)
       hivesize++;
     }
    }
    if (ter(x, y) == ot_hive) {
     zg.push_back(mongroup(mcat_bee, x * 2, y * 2, 2,
                  hivesize * 2 + rng(0, hivesize * 4)));
     if (hivesize == 9)
      ter(x, y) = ot_hive_center;
    }
   } else {	// If it's not a hive, calculate a chance that it's swamp...
    int swamp_chance = 0;
    for (int k = -2; k <= 2; k++) {
     for (int l = -2; l <= 2; l++) {
      if (ter(x + k, y + l) == ot_forest_water ||
          (ter(x+k, y+l) >= ot_river_center && ter(x+k, y+l) <= ot_river_nw))
       swamp_chance += 5;
     }  
    }
    bool swampy = false;
    if (swamps > 0 && swamp_chance > 0 && !one_in(swamp_chance) &&
        (ter(x, y) == ot_forest || ter(x, y) == ot_forest_thick ||
         ter(x, y) == ot_field  || one_in(SWAMPCHANCE))) {
// ...and make a swamp.
     ter(x, y) = ot_forest_water;
     swampy = true;
     swamps--;
    } else if (swamp_chance == 0)
     swamps = SWAMPINESS;
    if (ter(x, y) == ot_field)
     ter(x, y) = ot_forest;
    else if (ter(x, y) == ot_forest)
     ter(x, y) = ot_forest_thick;
 
    if (swampy && (ter(x, y-1) == ot_field || ter(x, y-1) == ot_forest))
     ter(x, y-1) = ot_forest_water;
    else if (ter(x, y-1) == ot_forest)
     ter(x, y-1) = ot_forest_thick;
    else if (ter(x, y-1) == ot_field)
     ter(x, y-1) = ot_forest;

    if (swampy && (ter(x, y+1) == ot_field || ter(x, y+1) == ot_forest))
     ter(x, y+1) = ot_forest_water;
    else if (ter(x, y+1) == ot_forest)
      ter(x, y+1) = ot_forest_thick;
    else if (ter(x, y+1) == ot_field)
      ter(x, y+1) = ot_forest;

    if (swampy && (ter(x-1, y) == ot_field || ter(x-1, y) == ot_forest))
     ter(x-1, y) = ot_forest_water;
    else if (ter(x-1, y) == ot_forest)
     ter(x-1, y) = ot_forest_thick;
    else if (ter(x-1, y) == ot_field)
     ter(x-1, y) = ot_forest;

    if (swampy && (ter(x+1, y) == ot_field || ter(x+1, y) == ot_forest))
     ter(x+1, y) = ot_forest_water;
    else if (ter(x+1, y) == ot_forest)
     ter(x+1, y) = ot_forest_thick;
    else if (ter(x+1, y) == ot_field)
     ter(x+1, y) = ot_forest;
   }
// Random walk our forest
   x += rng(-2, 2);
   if (x < 0    ) x = 0;
   if (x > OMAPX) x = OMAPX;
   y += rng(-2, 2);
   if (y < 0    ) y = 0;
   if (y > OMAPY) y = OMAPY;
  }
 }
}

void overmap::place_settlements(game *g)
{
 int num_settlements = dice(SETTLE_DICE, SETTLE_SIDES);
 for (int i = 0; i < num_settlements; i++) {
  settlement tmp(posx, posy);
  tmp.posx = rng(30, OMAPX - 30);
  tmp.posy = rng(30, OMAPY - 30);
  for (int j = 0; j < zg.size(); j++) {
// tmp.pos is doubled because monster groups on are a half-size scale.
// This is the most confusing part about the scaling of the overmap.
   int dist = trig_dist(tmp.posx*2, tmp.posy*2, zg[j].posx, zg[j].posy);
   int rad = zg[j].radius;
// Basically, the CENTER of the settlement can't be inside a group's radius,
//  but the OUTSKIRTS of town certain can be.
   while (zg[j].type != mcat_forest && dist <= rad * 1.5) {
// tmp.pos is doubled because monster groups on are a half-size scale.
    dist = trig_dist(tmp.posx*2, tmp.posy*2, zg[j].posx, zg[j].posy);
    zg[j].radius;
    tmp.posx = rng(30, OMAPX - 30);
    tmp.posy = rng(30, OMAPY - 30);
    j = 0;
   }
  }
  tmp.set_population();
  tmp.size = (tmp.pop < 24 ? 2 : tmp.pop / 8);
  towns.push_back(tmp);
 }

 for (int i = 0; i < towns.size(); i++) {
  for (int x = 0 - towns[i].size; x <= towns[i].size; x++) {
// First, build a wall around the town.  TODO: Not so square?
   if (is_wall_material(ter(x, towns[i].posy - towns[i].size)))
    ter(towns[i].posx + x, towns[i].posy - towns[i].size) = ot_wall;
   if (is_wall_material(ter(x, towns[i].posy + towns[i].size)))
    ter(towns[i].posx + x, towns[i].posy + towns[i].size) = ot_wall;
   if (is_wall_material(ter(towns[i].posx - towns[i].size, x)))
    ter(towns[i].posx - towns[i].size, towns[i].posy + x) = ot_wall;
   if (is_wall_material(ter(towns[i].posx + towns[i].size, towns[i].posy + x)))
    ter(towns[i].posx + towns[i].size, towns[i].posy + x) = ot_wall;
  }
// Pick a side and make an entrance over there
  switch (rng(0, 3)) {
   case 0: ter(towns[i].posx, towns[i].posy - towns[i].size) = ot_gate; break;
   case 1: ter(towns[i].posx + towns[i].size, towns[i].posy) = ot_gate; break;
   case 2: ter(towns[i].posx, towns[i].posy + towns[i].size) = ot_gate; break;
   case 3: ter(towns[i].posx - towns[i].size, towns[i].posy) = ot_gate; break;
  }

// Clear the terrain around the city, place buildings inside the city
  for (int x = towns[i].size * -2; x <= towns[i].size * 2; x++) {
   for (int y = towns[i].size * -2; y <= towns[i].size * 2; y++) {
// Inside the walls, build buildings...
    if (x >= 0 - towns[i].size + 1 && x <= towns[i].size - 1 &&
        y >= 0 - towns[i].size + 1 && y <= towns[i].size - 1   )
     settlement_building(towns[i], towns[i].posx + x, towns[i].posy + y);
    else if (is_ground(ter(towns[i].posx + x, towns[i].posy + y)))
     ter(towns[i].posx + x, towns[i].posy + y) = ot_field;// Clear terrain
   }
  }
// Finally, pick a place for the town center (bulletin board, etc)
  int center_tries = 0, cx, cy;
  do {
   cx = towns[i].posx + rng(towns[i].size * -1 + 1, towns[i].size - 1);
   cy = towns[i].posy + rng(towns[i].size * -1 + 1, towns[i].size - 1);
   if (ter(cx, cy) == ot_set_house || ter(cx, cy) == ot_field ||
       ter(cx, cy) == ot_forest) {
    ter(cx, cy) = ot_set_center;
    center_tries = 10;
   } else {
    center_tries++;
    if (center_tries == 10)
     ter(cx, cy) = ot_set_center;
   }
  } while (center_tries != 10);
  towns[i].populate(g);
 }
}

void overmap::place_NPCs(game *g)
{
 npc shopkeep;
 shopkeep.omx = posx;
 shopkeep.omy = posy;
 shopkeep.omz = posz;
 int npcx, npcy;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   switch (ter(x, y)) {
    case ot_set_food:
     switch(rng(0, 3)) {
      case 0: npcx =  5; npcy =  5; break;
      case 1: npcx = 18; npcy =  5; break;
      case 2: npcx =  5; npcy = 18; break;
      case 3: npcx = 18; npcy = 18; break;
     }
     break;
    case ot_set_weapons:	npcx = 11; npcy =  8; break;
    case ot_set_guns:		npcx = 10; npcy = 12; break;
    case ot_set_clinic:		npcx = 12; npcy =  8; break;
    case ot_set_clothing:	npcx = 10; npcy = 15; break;
    case ot_set_general:	npcx =  8; npcy = 14; break;
    case ot_set_casino:		npcx = 19; npcy = 10 + 4 * rng(0, 1); break;
    case ot_set_library:	npcx = 16; npcy = 20; break;
    case ot_set_bionics:	npcx = 12; npcy = 15; break;
    default:			npcx = -1; npcy = -1; break;
   }
   if (npcx != -1 && npcy != -1) {
    shopkeep.mapx = x * 2;
    shopkeep.mapy = y * 2;
    shopkeep.posx = npcx;
    shopkeep.posy = npcy;
    shopkeep.make_shopkeep(g, ter(x, y));
    npcs.push_back(shopkeep);
   }
  }
 }
}

void overmap::settlement_building(settlement &set, int x, int y)
{
 std::vector<oter_id> valid;
// Basically, one house for every five residents (rounded up).
 valid.push_back(ot_field);
 for (int i = 0; i + set.num(ot_set_house) < set.pop * 5 + 1 && i < 4; i++)
  valid.push_back(ot_set_house);
 if (set.num(ot_set_food) * 20 < set.pop)
  valid.push_back(ot_set_food);
 if (set.num(ot_set_weapons) * (25 - set.fact.strength) < set.pop)
  valid.push_back(ot_set_weapons);
 if (set.num(ot_set_guns) * (20 - set.fact.strength) < set.pop)
  valid.push_back(ot_set_guns);
 if (set.num(ot_set_clinic) * 20 < set.pop) {
  valid.push_back(ot_set_clinic);
  if (set.fact.has_job(FACJOB_DOCTORS)) {
   valid.push_back(ot_set_clinic);
   valid.push_back(ot_set_clinic);
   valid.push_back(ot_set_clinic);
  }
 }
 if (set.num(ot_set_clothing) * 30 < set.pop)
  valid.push_back(ot_set_clothing);
 if (set.num(ot_set_general) * 12 < set.pop)
  valid.push_back(ot_set_general);
 if (set.num(ot_set_lab) * 100 < set.pop &&
     (set.fact.goal == FACGOAL_KNOWLEDGE || set.fact.has_job(FACJOB_DRUGS)))
  valid.push_back(ot_set_lab);
 if (set.num(ot_set_bionics) * 80 < set.pop &&
     (set.fact.has_value(FACVAL_BIONICS) || rng(50, 400) < set.fact.power))
  valid.push_back(ot_set_bionics);

 if (set.fact.has_job(FACJOB_GAMBLING)) {
  valid.push_back(ot_set_casino);
  valid.push_back(ot_set_casino);
  valid.push_back(ot_set_casino);
  valid.push_back(ot_set_casino);
 }
 if (set.fact.has_value(FACVAL_BOOKS)) {
  valid.push_back(ot_set_library);
  valid.push_back(ot_set_library);
  valid.push_back(ot_set_library);
  valid.push_back(ot_set_library);
 }
 if (set.fact.goal == FACGOAL_NATURE) {
  valid.push_back(ot_forest);
  valid.push_back(ot_forest);
  valid.push_back(ot_forest);
  valid.push_back(ot_forest);
 }
 if (set.num(ot_radio_tower) == 0)
  valid.push_back(ot_radio_tower);

 oter_id pick = valid[rng(0, valid.size() - 1)];
 ter(x, y) = pick;
 set.add_building(pick);
}
 

void overmap::place_river(point pa, point pb)
{
 int x = pa.x, y = pa.y;
 do {
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (y+i >= 0 && y+i < OMAPY && x+j >= 0 && x+j < OMAPX)
     ter(x+j, y+i) = ot_river_center;
   }
  }
  if (pb.x > x && (rng(0, int(OMAPX * 1.2) - 1) < pb.x - x ||
                   (rng(0, int(OMAPX * .2) - 1) > pb.x - x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x++;
  if (pb.x < x && (rng(0, int(OMAPX * 1.2) - 1) < x - pb.x ||
                   (rng(0, int(OMAPX * .2) - 1) > x - pb.x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x--;
  if (pb.y > y && (rng(0, int(OMAPY * 1.2) - 1) < pb.y - y ||
                   (rng(0, int(OMAPY * .2) - 1) > pb.y - y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y++;
  if (pb.y < y && (rng(0, int(OMAPY * 1.2) - 1) < y - pb.y ||
                   (rng(0, int(OMAPY * .2) - 1) > y - pb.y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y--;
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
// We don't want our riverbanks touching the edge of the map for many reasons
    if ((y+i >= 1 && y+i < OMAPY - 1 && x+j >= 1 && x+j < OMAPX - 1) ||
// UNLESS, of course, that's where the river is headed!
        (abs(pb.y - (y+i)) < 4 && abs(pb.x - (x+j)) < 4))
     ter(x+j, y+i) = ot_river_center;
   }
  }
 } while (pb.x != x || pb.y != y);
}

void overmap::place_cities(std::vector<city> &cities, int min)
{
 int NUM_CITIES = dice(2, 7) + rng(min, min + 4);
 int cx, cy, cs;
 int start_dir;

 for (int i = 0; i < NUM_CITIES; i++) {
  cx = rng(20, OMAPX - 41);
  cy = rng(20, OMAPY - 41);
  cs = rng(4, 17);
  if (ter(cx, cy) == ot_field) {
   ter(cx, cy) = ot_road_nesw;
   city tmp; tmp.x = cx; tmp.y = cy; tmp.s = cs;
   cities.push_back(tmp);
   start_dir = rng(0, 3);
   for (int j = 0; j < 4; j++)
     make_road(cx, cy, cs, (start_dir + j) % 4, tmp);
  }
 }
}

void overmap::put_buildings(int x, int y, int dir, city town)
{
 int ychange = dir % 2, xchange = (dir + 1) % 2;
 for (int i = -1; i <= 1; i += 2) {
  if ((ter(x+i*xchange, y+i*ychange) == ot_field) && !one_in(STREETCHANCE)) {
   if (rng(0, 99) > 100 * dist(x,y,town.x,town.y) / town.s)
    ter(x+i*xchange, y+i*ychange) = shop(((dir%2)-i)%4);
   else
    ter(x+i*xchange, y+i*ychange) = house(((dir%2)-i)%4);
  }
 }
}

void overmap::make_road(int cx, int cy, int cs, int dir, city town)
{
 int x = cx, y = cy;
 int c = cs, croad = cs;
 switch (dir) {
 case 0:
  while (c > 0 && y > 0 && (ter(x, y-1) == ot_field || c == cs)) {
   y--;
   c--;
   ter(x, y) = ot_road_ns;
   for (int i = -1; i <= 0; i++) {
    for (int j = -1; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad - 1 && c >= 2 && ter(x - 1, y) == ot_field &&
                                  ter(x + 1, y) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 1, town);
    make_road(x, y, cs - rng(1, 3), 3, town);
   }
  }
  if (is_road(x, y-2))
   ter(x, y-1) = ot_road_ns;
  break;
 case 1:
  while (c > 0 && x < OMAPX-1 && (ter(x+1, y) == ot_field || c == cs)) {
   x++;
   c--;
   ter(x, y) = ot_road_ew;
   for (int i = -1; i <= 1; i++) {
    for (int j = 0; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad-2 && c >= 3 && ter(x, y-1) == ot_field &&
                                ter(x, y+1) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 0, town);
    make_road(x, y, cs - rng(1, 3), 2, town);
   }
  }
  if (is_road(x-2, y))
   ter(x-1, y) = ot_road_ew;
  break;
 case 2:
  while (c > 0 && y < OMAPY-1 && (ter(x, y+1) == ot_field || c == cs)) {
   y++;
   c--;
   ter(x, y) = ot_road_ns;
   for (int i = 0; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad-2 && ter(x-1, y) == ot_field && ter(x+1, y) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 1, town);
    make_road(x, y, cs - rng(1, 3), 3, town);
   }
  }
  if (is_road(x, y+2))
   ter(x, y+1) = ot_road_ns;
  break;
 case 3:
  while (c > 0 && x > 0 && (ter(x-1, y) == ot_field || c == cs)) {
   x--;
   c--;
   ter(x, y) = ot_road_ew;
   for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 0; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad - 2 && c >= 3 && ter(x, y-1) == ot_field &&
       ter(x, y+1) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 0, town);
    make_road(x, y, cs - rng(1, 3), 2, town);
   }
  }
  if (is_road(x+2, y))
   ter(x+1, y) = ot_road_ew;
  break;
 }
 cs -= rng(1, 3);
 if (cs >= 2 && c == 0) {
  int dir2;
  if (dir % 2 == 0)
   dir2 = rng(0, 1) * 2 + 1;
  else
   dir2 = rng(0, 1) * 2;
  make_road(x, y, cs, dir2, town);
  if (one_in(5))
   make_road(x, y, cs, (dir2 + 2) % 4, town);
 }
}

void overmap::build_lab(int x, int y, int s)
{
 ter(x, y) = ot_lab;
 for (int n = 0; n <= 1; n++) {	// Do it in two passes to allow diagonals
  for (int i = 1; i <= s; i++) {
   for (int lx = x - i; lx <= x + i; lx++) {
    for (int ly = y - i; ly <= y + i; ly++) {
     if ((ter(lx - 1, ly) == ot_lab || ter(lx + 1, ly) == ot_lab ||
         ter(lx, ly - 1) == ot_lab || ter(lx, ly + 1) == ot_lab) &&
         one_in(i))
      ter(lx, ly) = ot_lab;
    }
   }
  }
 }
 ter(x, y) = ot_lab_core;
 int numstairs = 0;
 if (s > 1) {	// Build stairs going down
  while (!one_in(6)) {
   int stairx, stairy;
   int tries = 0;
   do {
    stairx = rng(x - s, x + s);
    stairy = rng(y - s, y + s);
    tries++;
   } while (ter(stairx, stairy) != ot_lab && tries < 15);
   if (tries < 15)
    ter(stairx, stairy) = ot_lab_stairs;
   numstairs++;
  }
 }
 if (numstairs == 0) {	// This is the bottom of the lab;  We need a finale
  int finalex, finaley;
  do {
   finalex = rng(x - s, x + s);
   finaley = rng(y - s, y + s);
  } while (ter(finalex, finaley) != ot_lab &&
           ter(finalex, finaley) != ot_lab_core);
  ter(finalex, finaley) = ot_lab_finale;
 }
 zg.push_back(mongroup(mcat_lab, (x * 2), (y * 2), s, 60));
}

void overmap::build_anthill(int x, int y, int s)
{
 build_tunnel(x, y, s - rng(0, 3), 0);
 build_tunnel(x, y, s - rng(0, 3), 1);
 build_tunnel(x, y, s - rng(0, 3), 2);
 build_tunnel(x, y, s - rng(0, 3), 3);
 std::vector<point> queenpoints;
 for (int i = x - s; i <= x + s; i++) {
  for (int j = y - s; j <= y + s; j++) {
   if (ter(i, j) >= ot_ants_ns && ter(i, j) <= ot_ants_nesw)
    queenpoints.push_back(point(i, j));
  }
 }
 int index = rng(0, queenpoints.size() - 1);
 ter(queenpoints[index].x, queenpoints[index].y) = ot_ants_queen;
}

void overmap::build_tunnel(int x, int y, int s, int dir)
{
 if (s <= 0)
  return;
 if (ter(x, y) < ot_ants_ns || ter(x, y) > ot_ants_queen)
  ter(x, y) = ot_ants_ns;
 point next;
 switch (dir) {
  case 0: next = point(x    , y - 1);
  case 1: next = point(x + 1, y    );
  case 2: next = point(x    , y + 1);
  case 3: next = point(x - 1, y    );
 }
 if (s == 1)
  next = point(-1, -1);
 std::vector<point> valid;
 for (int i = x - 1; i <= x + 1; i++) {
  for (int j = y - 1; j <= y + 1; j++) {
   if ((ter(i, j) < ot_ants_ns || ter(i, j) > ot_ants_queen) &&
       abs(i - x) + abs(j - y) == 1)
    valid.push_back(point(i, j));
  }
 }
 for (int i = 0; i < valid.size(); i++) {
  if (valid[i].x != next.x || valid[i].y != next.y) {
   if (one_in(s * 2)) {
    if (one_in(2))
     ter(valid[i].x, valid[i].y) = ot_ants_food;
    else
     ter(valid[i].x, valid[i].y) = ot_ants_larvae;
   } else if (one_in(5)) {
    int dir2;
    if (valid[i].y == y - 1) dir2 = 0;
    if (valid[i].x == x + 1) dir2 = 1;
    if (valid[i].y == y + 1) dir2 = 2;
    if (valid[i].x == x - 1) dir2 = 3;
    build_tunnel(valid[i].x, valid[i].y, s - rng(0, 3), dir2);
   }
  }
 }
 build_tunnel(next.x, next.y, s - 1, dir);
}

void overmap::build_slimepit(int x, int y, int s)
{
 for (int n = 1; n <= s; n++) {
  for (int i = x - n; i <= x + n; i++) {
   for (int j = y - n; j <= y + n; j++) {
    if (rng(1, s * 2) >= n)
     ter(i, j) = (one_in(8) ? ot_slimepit_down : ot_slimepit);
    }
   }
 }
}

void overmap::place_rifts()
{
 int num_rifts = rng(0, 2) * rng(0, 2);
 std::vector<point> riftline;
 if (!one_in(4))
  num_rifts++;
 for (int n = 0; n < num_rifts; n++) {
  int x = rng(MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE);
  int y = rng(MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE);
  int xdist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE),
      ydist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE);
// We use rng(0, 10) as the t-value for this Bresenham Line, because by
// repeating this twice, we can get a thick line, and a more interesting rift.
  for (int o = 0; o < 3; o++) {
   if (xdist > ydist)
    riftline = line_to(x - xdist, y - ydist+o, x + xdist, y + ydist, rng(0,10));
   else
    riftline = line_to(x - xdist+o, y - ydist, x + xdist, y + ydist, rng(0,10));
   for (int i = 0; i < riftline.size(); i++) {
    if (i == riftline.size() / 2 && !one_in(3))
     ter(riftline[i].x, riftline[i].y) = ot_hellmouth;
    else
     ter(riftline[i].x, riftline[i].y) = ot_rift;
   }
  }
 }
}

void overmap::make_hiway(int x1, int y1, int x2, int y2, oter_id base)
{
 std::vector<point> next;
 int dir = 0;
 int x = x1, y = y1;
 int xdir, ydir;
 int tmp;
 bool bridge_is_okay = false;
 do {
  next.clear(); // Clear list of valid points
  // Add valid points -- step in the right x-direction
  if (x2 > x)
   next.push_back(point(x + 1, y));
  else if (x2 < x)
   next.push_back(point(x - 1, y));
  else
   next.push_back(point(-1, -1)); // X is right--don't change it!
  // Add valid points -- step in the right y-direction
  if (y2 > y)
   next.push_back(point(x, y + 1));
  else if (y2 < y)
   next.push_back(point(x, y - 1));
  for (int i = 0; i < next.size(); i++) { // Take an existing road if we can
   if (next[i].x != -1 && is_road(base, next[i].x, next[i].y)) {
    x = next[i].x;
    y = next[i].y;
    dir = i; // We are moving... whichever way that highway is moving
// If we're closer to the destination than to the origin, this highway is done!
    if (dist(x, y, x1, y1) > dist(x, y, x2, y2))
     return;
    next.clear();
   } 
  }
  if (!next.empty()) { // Assuming we DIDN'T take an existing road...
   if (next[0].x == -1) { // X is correct, so we're taking the y-change
    dir = 1; // We are moving vertically
    x = next[1].x;
    y = next[1].y;
    if (is_river(ter(x, y)))
     ter(x, y) = ot_bridge_ns;
    else
     ter(x, y) = base;
   } else if (next.size() == 1) { // Y must be correct, take the x-change
    if (dir == 1)
     ter(x, y) = base;
    dir = 0; // We are moving horizontally
    x = next[0].x;
    y = next[0].y;
    if (is_river(ter(x, y)))
     ter(x, y) = ot_bridge_ew;
    else
     ter(x, y) = base;
   } else {	// More than one eligable route; pick one randomly
    if (one_in(12) &&
       !is_river(ter(next[(dir + 1) % 2].x, next[(dir + 1) % 2].y)))
     dir = (dir + 1) % 2; // Switch the direction (hori/vert) in which we move
    x = next[dir].x;
    y = next[dir].y;
    if (dir == 0) {	// Moving horizontally
     if (is_river(ter(x, y))) {
      xdir = -1;
      bridge_is_okay = true;
      if (x2 > x)
       xdir = 1;
      tmp = x;
      while (is_river(ter(tmp, y))) {
       if (is_road(base, tmp, y))
        bridge_is_okay = false;	// Collides with another bridge!
       tmp += xdir;
      }
      if (bridge_is_okay) {
       while(is_river(ter(x, y))) {
        ter(x, y) = ot_bridge_ew;
        x += xdir;
       }
       ter(x, y) = base;
      }
     } else
      ter(x, y) = base;
    } else {		// Moving vertically
     if (is_river(ter(x, y))) {
      ydir = -1;
      bridge_is_okay = true;
      if (y2 > y)
       ydir = 1;
      tmp = y;
      while (is_river(ter(x, tmp))) {
       if (is_road(base, x, tmp))
        bridge_is_okay = false;	// Collides with another bridge!
       tmp += ydir;
      }
      if (bridge_is_okay) {
       while (is_river(ter(x, y))) {
        ter(x, y) = ot_bridge_ns;
        y += ydir;
       }
       ter(x, y) = base;
      }
     } else
      ter(x, y) = base;
    }
   }
   if (one_in(50) && posz == 0)
    building_on_hiway(x, y, dir);
  }
 } while (x != x2 || y != y2);
}

void overmap::building_on_hiway(int x, int y, int dir)
{
 int xdif = dir * (1 - 2 * rng(0,1));
 int ydif = (1 - dir) * (1 - 2 * rng(0,1));
 int rot = 0;
      if (ydif ==  1)
  rot = 0;
 else if (xdif == -1)
  rot = 1;
 else if (ydif == -1)
  rot = 2;
 else if (xdif ==  1)
  rot = 3;

 switch (rng(1, 3)) {
 case 1:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = ot_lab_stairs;
  break;
 case 2:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = house(rot);
  break;
 case 3:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = ot_radio_tower;
  break;
/*
 case 4:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdir, y + ydif) = ot_sewage_treatment;
  break;
*/
 }
}

void overmap::place_hiways(std::vector<city> cities, oter_id base)
{
 if (cities.size() == 1)
  return;
 city best;
 int closest = -1;
 int distance;
 bool maderoad = false;
 for (int i = 0; i < cities.size(); i++) {
  maderoad = false;
  closest = -1;
  for (int j = i + 1; j < cities.size(); j++) {
   distance = dist(cities[i].x, cities[i].y, cities[j].x, cities[j].y);
   if (distance < closest || closest < 0) {
    closest = distance; 
    best = cities[j];
   }
   if (distance < TOP_HIWAY_DIST) {
    maderoad = true;
    make_hiway(cities[i].x, cities[i].y, cities[j].x, cities[j].y, base);
   }
  }
  if (!maderoad && closest > 0)
   make_hiway(cities[i].x, cities[i].y, best.x, best.y, base);
 }
}

// Polish does both good_roads and good_rivers (and any future polishing) in
// a single loop; much more efficient
void overmap::polish(oter_id min, oter_id max)
{
// Main loop--checks roads and rivers that aren't on the borders of the map
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (ter(x, y) >= min && ter(x, y) <= max) {
    if (ter(x, y) >= ot_road_null && ter(x, y) <= ot_road_nesw)
     good_road(ot_road_ns, x, y);
    else if (ter(x, y) >= ot_subway_ns && ter(x, y) <= ot_subway_nesw)
     good_road(ot_subway_ns, x, y);
    else if (ter(x, y) >= ot_sewer_ns && ter(x, y) <= ot_sewer_nesw)
     good_road(ot_sewer_ns, x, y);
    else if (ter(x, y) >= ot_ants_ns && ter(x, y) <= ot_ants_nesw)
     good_road(ot_ants_ns, x, y);
    else if (ter(x, y) >= ot_river_center && ter(x, y) < ot_river_nw)
     good_river(x, y);
   }
// Sometimes a bridge will start at the edge of a river, and this looks ugly
// So, fix it by making that square normal road; bit of a kludge but it works
   else if (ter(x, y) == ot_bridge_ns &&
            (!is_river(ter(x - 1, y)) || !is_river(ter(x + 1, y))))
    ter(x, y) = ot_road_ns;
   else if (ter(x, y) == ot_bridge_ew &&
            (!is_river(ter(x, y - 1)) || !is_river(ter(x, y + 1))))
    ter(x, y) = ot_road_ew;
  }
 }
// Fixes stretches of parallel roads--turns them into two-lane highways
// Note that this fixes 2x2 areas... a "tail" of 1x2 parallel roads may be left.
// This can actually be a good thing; it ensures nice connections
// Also, this leaves, say, 3x3 areas of road.  TODO: fix this?  courtyards etc?
 for (int y = 0; y < OMAPY - 1; y++) {
  for (int x = 0; x < OMAPX - 1; x++) {
   if (ter(x, y) >= min && ter(x, y) <= max) {
    if (ter(x, y) == ot_road_nes && ter(x+1, y) == ot_road_nsw &&
        ter(x, y+1) == ot_road_nes && ter(x+1, y+1) == ot_road_nsw) {
     ter(x, y) = ot_hiway_ns;
     ter(x+1, y) = ot_hiway_ns;
     ter(x, y+1) = ot_hiway_ns;
     ter(x+1, y+1) = ot_hiway_ns;
    } else if (ter(x, y) == ot_road_esw && ter(x+1, y) == ot_road_esw &&
               ter(x, y+1) == ot_road_new && ter(x+1, y+1) == ot_road_new) {
     ter(x, y) = ot_hiway_ew;
     ter(x+1, y) = ot_hiway_ew;
     ter(x, y+1) = ot_hiway_ew;
     ter(x+1, y+1) = ot_hiway_ew;
    }
   }
  }
 }
}

bool overmap::is_road(int x, int y)
{
 if (ter(x, y) == ot_rift || ter(x, y) == ot_hellmouth)
  return true;
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  for (int i = 0; i < roads_out.size(); i++) {
   if (abs(roads_out[i].x - x) + abs(roads_out[i].y - y) <= 1)
    return true;
  }
 }
 if ((ter(x, y) >= ot_road_null && ter(x, y) <= ot_bridge_ew) ||
     (ter(x, y) >= ot_subway_ns && ter(x, y) <= ot_subway_nesw))
  return true;
 return false;
}

bool overmap::is_road(oter_id base, int x, int y)
{
 oter_id min, max;
        if (base >= ot_road_null && base <= ot_bridge_ew) {
  min = ot_road_null;
  max = ot_bridge_ew;
 } else if (base >= ot_subway_ns && base <= ot_subway_nesw) {
  min = ot_subway_station;
  max = ot_subway_nesw;
 } else if (base >= ot_sewer_ns && base <= ot_sewer_nesw) {
  min = ot_sewer_ns;
  max = ot_sewer_nesw;
 } else if (base >= ot_ants_ns && base <= ot_ants_queen) {
  min = ot_ants_ns;
  max = ot_ants_queen;
 } else	{ // Didn't plan for this!
  debugmsg("Bad call to is_road, %s", oterlist[base].name.c_str());
  return false;
 }
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  for (int i = 0; i < roads_out.size(); i++) {
   if (abs(roads_out[i].x - x) + abs(roads_out[i].y - y) <= 1)
    return true;
  }
 }
 if (ter(x, y) >= min && ter(x, y) <= max)
  return true;
 return false;
}

void overmap::good_road(oter_id base, int x, int y)
{
 int d = ot_road_ns;
 if (is_road(base, x, y-1)) {
  if (is_road(base, x+1, y)) { 
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_nesw - d);
    else
     ter(x, y) = oter_id(base + ot_road_nes - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_new - d);
    else
     ter(x, y) = oter_id(base + ot_road_ne - d);
   } 
  } else {
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_nsw - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_wn - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } 
  }
 } else {
  if (is_road(base, x+1, y)) { 
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_esw - d);
    else
     ter(x, y) = oter_id(base + ot_road_es - d);
   } else
    ter(x, y) = oter_id(base + ot_road_ew - d);
  } else {
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_sw - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_ew - d);
    else {// No adjoining roads/etc. Happens occasionally, esp. with sewers.
     ter(x, y) = oter_id(base + ot_road_nesw - d);
    }
   } 
  }
 }
 if (ter(x, y) == ot_road_nesw && one_in(4))
  ter(x, y) = ot_road_nesw_manhole;
}

void overmap::good_river(int x, int y)
{
 if (is_river(ter(x - 1, y))) {
  if (is_river(ter(x, y - 1))) {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y))) {
// River on N, S, E, W; but we might need to take a "bite" out of the corner
     if (!is_river(ter(x - 1, y - 1)))
      ter(x, y) = ot_river_c_not_nw;
     else if (!is_river(ter(x + 1, y - 1)))
      ter(x, y) = ot_river_c_not_ne;
     else if (!is_river(ter(x - 1, y + 1)))
      ter(x, y) = ot_river_c_not_sw;
     else if (!is_river(ter(x + 1, y + 1)))
      ter(x, y) = ot_river_c_not_se;
     else
      ter(x, y) = ot_river_center;
    } else
     ter(x, y) = ot_river_east;
   } else {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_south;
    else
     ter(x, y) = ot_river_se;
   }
  } else {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_north;
    else
     ter(x, y) = ot_river_ne;
   } else {
    if (is_river(ter(x + 1, y))) // Means it's swampy
     ter(x, y) = ot_forest_water;
   }
  }
 } else {
  if (is_river(ter(x, y - 1))) {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_west;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   } else {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_sw;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   }
  } else {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_nw;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   } else // Should never happen
    ter(x, y) = ot_forest_water;
  }
 }
}
    
void overmap::place_mongroups()
{
// Cities are full of zombies
 for (int i = 0; i < cities.size(); i++) {
  if (!one_in(16) || cities[i].s > 5)
   zg.push_back(
	mongroup(mcat_zombie, (cities[i].x * 2), (cities[i].y * 2),
	         int(cities[i].s * 2.5), cities[i].s * 80));
 }

// Lots of random anthills
 for (int n = 0; n < 30; n++) {
  int x = rng(MAX_ANT_SIZE * 2, OMAPX - MAX_ANT_SIZE * 2);
  int y = rng(MAX_ANT_SIZE * 2, OMAPY - MAX_ANT_SIZE * 2);
  bool is_okay = true;
  for (int i = -2; i <= 2 && is_okay; i++) {
   for (int j = -2; j <= 2 && is_okay; j++) {
    if (ter(x + i, y + j) != ot_field)
     is_okay = false;
   }
  }
  if (is_okay) {
   ter(x, y) = ot_anthill;
   zg.push_back(
	mongroup(mcat_ant, x * 2, y * 2, dice(6, 6), rng(1000, 2000)));
  }
 }

// Figure out where swamps are, and place swamp monsters
 for (int x = 3; x < OMAPX - 3; x += 7) {
  for (int y = 3; y < OMAPY - 3; y += 7) {
   int swamp_count = 0;
   for (int sx = x - 3; sx <= x + 3; sx++) {
    for (int sy = y - 3; sy <= y + 3; sy++) {
     if (ter(sx, sy) == ot_forest_water || is_river(ter(sx, sy)))
      swamp_count++;
    }
   }
   if (swamp_count >= 15) // 30% swamp!
    zg.push_back(mongroup(mcat_swamp, x * 2, y * 2, 3, rng(500, 2000)));
  }
 }

// Place slimepits
 for (int i = 0; i < 15; i++) {
  int x = rng(MAX_GOO_SIZE * 2, OMAPX - MAX_GOO_SIZE * 2);
  int y = rng(MAX_GOO_SIZE * 2, OMAPY - MAX_GOO_SIZE * 2);
  if (ter(x, y) == ot_field)
   ter(x, y) = ot_slimepit_down;
 }

// Place a silo or six
 for (int n = 0; n < 6; n++) {
  int x = rng(3, OMAPX - 4);
  int y = rng(3, OMAPY - 4);
  bool okay = true;
  for (int i = x - 3; i <= x + 3 && okay; i++) {
   for (int j = y - 3; j <= y + 3 && okay; j++) {
    if (ter(i, j) != ot_field && ter(i, j) != ot_forest &&
        ter(i, j) != ot_forest_thick && ter(i, j) != ot_forest_water)
     okay = false;
   }
  }
  if (okay)
   ter(x, y) = ot_silo;
 }
 
// Place the "put me anywhere" grounds
 int numgroups = rng(0, 3);
 for (int i = 0; i < numgroups; i++) {
  zg.push_back(
	mongroup(mcat_worm, rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1),
	         rng(20, 40), rng(500, 1000)));
 }

 numgroups = rng(0, 6);
 for (int i = 0; i < numgroups; i++) {
  zg.push_back(
	mongroup(mcat_plants, rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1),
	         rng(30, 50), rng(800, 1300)));
 }
 numgroups = rng(0, 7);
 for (int i = 0; i < numgroups; i++) {
  zg.push_back(
	mongroup(mcat_fungi, rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1),
	         rng(20, 30), rng(400, 800)));
 }
// Forest groups cover the entire map
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY, OMAPY,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY * 2 - 1, OMAPY,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, OMAPX, 0, OMAPX,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, OMAPX * 2 - 1, 0, OMAPX,
                 rng(2000, 12000)));
}

void overmap::place_radios()
{
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j) == ot_radio_tower)
    radios.push_back(radio_tower(i*2, j*2, rng(80, 200),
                     "This is the debug Radio Tower Msg"));
  }
 }
}

void overmap::save(std::string name)
{
 save(name, posx, posy, posz);
}

void overmap::save(std::string name, int x, int y, int z)
{
 std::stringstream plrfilename, terfilename;
 std::ofstream fout;
 plrfilename << "save/" << name << ".seen." << x << "." << y << "." << z;
 terfilename << "save/o." << x << "." << y << "." << z;
 fout.open(plrfilename.str().c_str());
 for (int j = 0; j < OMAPY; j++) {
  for (int i = 0; i < OMAPX; i++) {
   if (seen(i, j))
    fout << "1";
   else
    fout << "0";
  }
  fout << std::endl;
 }
 for (int i = 0; i < notes.size(); i++)
  fout << "N " << notes[i].x << " " << notes[i].y << " " << notes[i].num <<
          std::endl << notes[i].text << std::endl;
 fout.close();
 fout.open(terfilename.str().c_str(), std::ios_base::trunc);
 for (int j = 0; j < OMAPY; j++) {
  for (int i = 0; i < OMAPX; i++)
   fout << char(int(ter(i, j)) + 32);
 }
 fout << std::endl;
 for (int i = 0; i < zg.size(); i++)
  fout << "Z " << zg[i].type << " " << zg[i].posx << " " << zg[i].posy << " " <<
          int(zg[i].radius) << " " << zg[i].population << std::endl;
 for (int i = 0; i < cities.size(); i++)
  fout << "t " << cities[i].x << " " << cities[i].y << " " << cities[i].s <<
          std::endl;
 for (int i = 0; i < roads_out.size(); i++)
  fout << "R " << roads_out[i].x << " " << roads_out[i].y << std::endl;
 for (int i = 0; i < radios.size(); i++)
  fout << "T " << radios[i].x << " " << radios[i].y << " " <<
          radios[i].strength << " " << std::endl << radios[i].message <<
          std::endl;
/*	BUGGY - omit for now
 for (int i = 0; i < npcs.size(); i++)
  fout << "n " << npcs[i].save_info() << std::endl;
*/
 fout.close();
}

void overmap::open(game *g, int x, int y, int z)
{
 std::stringstream plrfilename, terfilename;
 std::ifstream fin;
 char line[OMAPX];
 char datatype;
 int ct, cx, cy, cs, cp;
 city tmp;

 plrfilename << "save/" << g->u.name << ".seen." << x << "." << y << "." << z;
 terfilename << "save/o." << x << "." << y << "." << z;
// Set position IDs
 posx = x;
 posy = y;
 posz = z;
 fin.open(terfilename.str().c_str());
// DEBUG VARS
 int nummg = 0;
 if (fin.is_open()) {
  for (int j = 0; j < OMAPY; j++) {
   for (int i = 0; i < OMAPX; i++) {
    ter(i, j) = oter_id(fin.get() - 32);
    if (ter(i, j) < 0 || ter(i, j) > num_ter_types)
     debugmsg("Loaded bad ter!  %s; ter %d",
              terfilename.str().c_str(), ter(i, j));
   }
  }
  while (fin >> datatype) {
          if (datatype == 'Z') {	// Monster group
    fin >> ct >> cx >> cy >> cs >> cp;
    zg.push_back(mongroup(moncat_id(ct), cx, cy, cs, cp));
    nummg++;
   } else if (datatype == 't') {	// City
    fin >> cx >> cy >> cs;
    tmp.x = cx; tmp.y = cy; tmp.s = cs;
    cities.push_back(tmp);
   } else if (datatype == 'R') {	// Road leading out
    fin >> cx >> cy;
    tmp.x = cx; tmp.y = cy; tmp.s = 0;
    roads_out.push_back(tmp);
   } else if (datatype == 'T') {	// Radio tower
    radio_tower tmp;
    fin >> tmp.x >> tmp.y >> tmp.strength;
    getline(fin, tmp.message);	// Chomp endl
    getline(fin, tmp.message);
    radios.push_back(tmp);
   } else if (datatype == 'n') {	// NPC
    std::string npcdata;
    getline(fin, npcdata);
    npc tmp;
    tmp.load_info(npcdata);
    npcs.push_back(tmp);
   } else if (datatype == 'I' || datatype == 'C' || datatype == 'W' ||
              datatype == 'w' || datatype == 'c') {
    std::string itemdata;
    getline(fin, itemdata);
    if (npcs.empty()) {
     debugmsg("Overmap %d:%d:%d tried to load object data, without an NPC!",
              posx, posy, posz);
     debugmsg(itemdata.c_str());
    } else {
     item tmp(itemdata, g);
     npc* last = &(npcs.back());
     switch (datatype) {
      case 'I': last->inv.push_back(tmp);			break;
      case 'C': last->inv.back().contents.push_back(tmp);	break;
      case 'W': last->worn.push_back(tmp);			break;
      case 'w': last->weapon = tmp;				break;
      case 'c': last->weapon.contents.push_back(tmp);		break;
     }
    }
   }
  }
  fin.close();
  fin.open(plrfilename.str().c_str());
  if (fin.is_open()) {	// Load private seen data
   for (int j = 0; j < OMAPY; j++) {
    fin.getline(line, OMAPX + 1);
    for (int i = 0; i < OMAPX; i++) {
     if (line[i] == '1')
      seen(i, j) = true;
     else
      seen(i, j) = false;
    }
   }
   while (fin >> datatype) {	// Load private notes
    if (datatype == 'N') {
     om_note tmp;
     fin >> tmp.x >> tmp.y >> tmp.num;
     getline(fin, tmp.text);	// Chomp endl
     getline(fin, tmp.text);
     notes.push_back(tmp);
    }
   }
   fin.close();
  } else {
   for (int j = 0; j < OMAPY; j++) {
    for (int i = 0; i < OMAPX; i++)
    seen(i, j) = false;
   }
  }
 } else if (z <= -1) {	// No map exists, and we are underground!
// Fetch the terrain above
  overmap* above = new overmap(g, x, y, z + 1);
  generate_sub(above);
  save(g->u.name, x, y, z);
  delete above;
 } else {	// No map exists!  Prepare neighbors, and generate one.
  std::vector<overmap*> pointers;
// Fetch north and south
  for (int i = -1; i <= 1; i+=2) {
   std::stringstream tmpfilename;
   tmpfilename << "save/o." << x << "." << y + i << "." << z;
   fin.open(tmpfilename.str().c_str());
   if (fin.is_open()) {
    fin.close();
    pointers.push_back(new overmap(g, x, y+i, z));
   } else
    pointers.push_back(NULL);
  }
// Fetch east and west
  for (int i = -1; i <= 1; i+=2) {
   std::stringstream tmpfilename;
   tmpfilename << "save/o." << x + i << "." << y << "." << z;
   fin.open(tmpfilename.str().c_str());
   if (fin.is_open()) {
    fin.close();
    pointers.push_back(new overmap(g, x+i, y, z));
   } else
    pointers.push_back(NULL);
  }
// pointers looks like (north, south, west, east)
  generate(g, pointers[0], pointers[3], pointers[1], pointers[2]);
  for (int i = 0; i < 4; i++)
   delete pointers[i];
  save(g->u.name, x, y, z);
 }
}

