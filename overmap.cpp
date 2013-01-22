#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#elif (defined __CYGWIN__)
      #include "ncurses/curses.h"
#else
	#include <curses.h>
#endif

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
#include <cstring>
#include <ostream>
#include "debug.h"

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
 return sqrt(double(pow(x1-x2, 2.0) + pow(y1-y2, 2.0)));
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
 int type = rng(0, 17);
 if (one_in(20))
  type = 18;
 switch (type) {
  case  0: ret = ot_s_lot;	         break;
  case  1: ret = ot_s_gas_north;         break;
  case  2: ret = ot_s_pharm_north;       break;
  case  3: ret = ot_s_grocery_north;     break;
  case  4: ret = ot_s_hardware_north;    break;
  case  5: ret = ot_s_sports_north;      break;
  case  6: ret = ot_s_liquor_north;      break;
  case  7: ret = ot_s_gun_north;         break;
  case  8: ret = ot_s_clothes_north;     break;
  case  9: ret = ot_s_library_north;     break;
  case 10: ret = ot_s_restaurant_north;  break;
  case 11: ret = ot_sub_station_north;   break;
  case 12: ret = ot_bank_north;          break;
  case 13: ret = ot_bar_north;           break;
  case 14: ret = ot_s_electronics_north; break;
  case 15: ret = ot_pawn_north;          break;
  case 16: ret = ot_mil_surplus_north;   break;
  case 17: ret = ot_s_garage_north;      break;
  case 18: ret = ot_police_north;        break;
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

overmap::overmap(const overmap & om)
{
#define CP(x) x = om.x
 CP(cities);
 CP(roads_out);
 CP(towns);
 CP(zg);
 CP(radios);
 CP(posx);
 CP(posy);
 CP(posz);
 CP(npcs);
 CP(nullret);
 CP(nullbool);
 CP(notes);
#undef CP
 memcpy(t, om.t, sizeof(t));
 memcpy(s, om.s, sizeof(s));
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

std::vector<mongroup*> overmap::monsters_at(int x, int y)
{
 std::vector<mongroup*> ret;
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY)
  return ret;
 for (int i = 0; i < zg.size(); i++) {
  if (trig_dist(x, y, zg[i].posx, zg[i].posy) <= zg[i].radius)
   ret.push_back(&(zg[i]));
 }
 return ret;
}

bool overmap::is_safe(int x, int y)
{
 std::vector<mongroup*> mons = monsters_at(x, y);
 if (monsters_at(x, y).empty())
  return true;

 bool safe = true;
 for (int n = 0; n < mons.size() && safe; n++)
  safe = mons[n]->is_safe();

 return safe;
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

void overmap::delete_note(int x, int y)
{
 std::vector<om_note>::iterator it;
 for (it = notes.begin(); it < notes.end(); it++) {
  if (it->x == x && it->y == y){
  	notes.erase(it);
  }
 }
}

point overmap::display_notes()
{
 std::string title = "Notes:";
 WINDOW* w_notes = newwin(25, 80, 0, 0);
 const int maxitems = 20;	// Number of items to show at one time.
 char ch = '.';
 int start = 0, cur_it;
 mvwprintz(w_notes, 0, 0, c_ltgray, title.c_str());
 do{
  if (ch == '<' && start > 0) {
   for (int i = 1; i < 25; i++)
    mvwprintz(w_notes, i, 0, c_black, "                                                     ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_notes, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < notes.size()) {
   start = cur_it;
   mvwprintw(w_notes, maxitems + 2, 12, "            ");
   for (int i = 1; i < 25; i++)
    mvwprintz(w_notes, i, 0, c_black, "                                                     ");
  }
  int cur_line = 2;
  int last_line = -1;
  char cur_let = 'a';
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
   if (cur_it < notes.size()) {
   mvwputch (w_notes, cur_line, 0, c_white, cur_let++);
   mvwprintz(w_notes, cur_line, 2, c_ltgray, "- %s", notes[cur_it].text.c_str());
   } else{
    last_line = cur_line - 2;
    break;
   }
   cur_line++;
  }

  if(last_line == -1)
   last_line = 23;
  if (start > 0)
   mvwprintw(w_notes, maxitems + 4, 0, "< Go Back");
  if (cur_it < notes.size())
   mvwprintw(w_notes, maxitems + 4, 12, "> More notes"); 
  if(ch >= 'a' && ch <= 't'){
   int chosen_line = (int)(ch % (int)'a');
   if(chosen_line < last_line)
    return point(notes[start + chosen_line].x, notes[start + chosen_line].y); 
  }
  mvwprintz(w_notes, 0, 40, c_white, "Press letter to center on note");
  mvwprintz(w_notes, 24, 40, c_white, "Spacebar - Return to map  ");
  wrefresh(w_notes);
  ch = getch();
 } while(ch != ' ');
 delwin(w_notes);
 return point(-1,-1);
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
   } else
    place_river(river_start[index],
                river_end_copy[rng(0, river_end_copy.size() - 1)]);
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
   } else
    place_river(river_start_copy[rng(0, river_start_copy.size() - 1)],
                river_end[index]);
   river_end.erase(river_end.begin() + index);
  }
 } else if (river_end.size() > 0) {
  if (river_start.size() != river_end.size())
   river_start.push_back( point(rng(OMAPX * .25, OMAPX * .75),
                                rng(OMAPY * .25, OMAPY * .75)));
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
// Compile our master list of roads; it's less messy if roads_out is first
 for (int i = 0; i < roads_out.size(); i++)
  road_points.push_back(roads_out[i]);
 for (int i = 0; i < cities.size(); i++)
  road_points.push_back(cities[i]);
// And finally connect them via "highways"
 place_hiways(road_points, ot_road_null);
// Place specials
 place_specials();
// Make the roads out road points;
 for (int i = 0; i < roads_out.size(); i++)
  ter(roads_out[i].x, roads_out[i].y) = ot_road_nesw;
// Clean up our roads and rivers
 polish();
// Place the monsters, now that the terrain is laid out
 place_mongroups();
 place_radios();
}

void overmap::generate_sub(overmap* above)
{
 std::vector<city> subway_points;
 std::vector<city> sewer_points;
 std::vector<city> ant_points;
 std::vector<city> goo_points;
 std::vector<city> lab_points;
 std::vector<point> shaft_points;
 std::vector<city> mine_points;
 std::vector<point> bunker_points;
 std::vector<point> shelter_points;
 std::vector<point> triffid_points;
 std::vector<point> temple_points;
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   seen(i, j) = false;	// Start by setting all squares to unseen
   ter(i, j) = ot_rock;	// Start by setting everything to solid rock
  }
 }

 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (above->ter(i, j) >= ot_sub_station_north &&
       above->ter(i, j) <= ot_sub_station_west) {
    ter(i, j) = ot_subway_nesw;
    subway_points.push_back(city(i, j, 0));

   } else if (above->ter(i, j) == ot_road_nesw_manhole) {
    ter(i, j) = ot_sewer_nesw;
    sewer_points.push_back(city(i, j, 0));

   } else if (above->ter(i, j) == ot_sewage_treatment) {
    for (int x = i-1; x <= i+1; x++) {
     for (int y = j-1; y <= j+1; y++) {
      ter(x, y) = ot_sewage_treatment_under;
     }
    }
    ter(i, j) = ot_sewage_treatment_hub;
    sewer_points.push_back(city(i, j, 0));

   } else if (above->ter(i, j) == ot_spider_pit)
    ter(i, j) = ot_spider_pit_under;

   else if (above->ter(i, j) == ot_cave && posz == -1) {
    if (one_in(3))
     ter(i, j) = ot_cave_rat;
    else
     ter(i, j) = ot_cave;

   } else if (above->ter(i, j) == ot_cave_rat && posz == -2)
    ter(i, j) = ot_cave_rat;

   else if (above->ter(i, j) == ot_anthill) {
    int size = rng(MIN_ANT_SIZE, MAX_ANT_SIZE);
    ant_points.push_back(city(i, j, size));
    zg.push_back(mongroup(mcat_ant, i * 2, j * 2, size * 1.5, rng(6000, 8000)));

   } else if (above->ter(i, j) == ot_slimepit_down) {
    int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
    goo_points.push_back(city(i, j, size));

   } else if (above->ter(i, j) == ot_forest_water)
    ter(i, j) = ot_cavern;

   else if (above->ter(i, j) == ot_triffid_grove ||
            above->ter(i, j) == ot_triffid_roots)
    triffid_points.push_back( point(i, j) );

   else if (above->ter(i, j) == ot_temple_stairs)
    temple_points.push_back( point(i, j) );

   else if (above->ter(i, j) == ot_lab_core ||
            (posz == -1 && above->ter(i, j) == ot_lab_stairs))
    lab_points.push_back(city(i, j, rng(1, 5 + posz)));

   else if (above->ter(i, j) == ot_lab_stairs)
    ter(i, j) = ot_lab;

   else if (above->ter(i, j) == ot_bunker && posz == -1)
    bunker_points.push_back( point(i, j) );

   else if (above->ter(i, j) == ot_shelter)
    shelter_points.push_back( point(i, j) );

   else if (above->ter(i, j) == ot_mine_entrance)
    shaft_points.push_back( point(i, j) );

   else if (above->ter(i, j) == ot_mine_shaft ||
            above->ter(i, j) == ot_mine_down    ) {
    ter(i, j) = ot_mine;
    mine_points.push_back(city(i, j, rng(6 + posz, 10 + posz)));

   } else if (above->ter(i, j) == ot_mine_finale) {
    for (int x = i - 1; x <= i + 1; x++) {
     for (int y = j - 1; y <= j + 1; y++)
      ter(x, y) = ot_spiral;
    }
    ter(i, j) = ot_spiral_hub;
    zg.push_back(mongroup(mcat_spiral, i * 2, j * 2, 2, 200));

   } else if (above->ter(i, j) == ot_silo) {
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
 polish(ot_subway_ns, ot_subway_nesw);
 polish(ot_ants_ns, ot_ants_nesw);
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
 for (int i = 0; i < mine_points.size(); i++)
  build_mine(mine_points[i].x, mine_points[i].y, mine_points[i].s);
// Basements done last so sewers, etc. don't overwrite them
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (above->ter(i, j) >= ot_house_base_north &&
       above->ter(i, j) <= ot_house_base_west)
    ter(i, j) = ot_basement;
  }
 }

 for (int i = 0; i < shaft_points.size(); i++)
  ter(shaft_points[i].x, shaft_points[i].y) = ot_mine_shaft;

 for (int i = 0; i < bunker_points.size(); i++)
  ter(bunker_points[i].x, bunker_points[i].y) = ot_bunker;

 for (int i = 0; i < shelter_points.size(); i++)
  ter(shelter_points[i].x, shelter_points[i].y) = ot_shelter_under;

 for (int i = 0; i < triffid_points.size(); i++) {
  if (posz == -1)
   ter( triffid_points[i].x, triffid_points[i].y ) = ot_triffid_roots;
  else
   ter( triffid_points[i].x, triffid_points[i].y ) = ot_triffid_finale;
 }

 for (int i = 0; i < temple_points.size(); i++) {
  if (posz == -5)
   ter( temple_points[i].x, temple_points[i].y ) = ot_temple_finale;
  else
   ter( temple_points[i].x, temple_points[i].y ) = ot_temple_stairs;
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

std::vector<point> overmap::find_all(point origin, oter_id type, int type_range,
                            int &dist, bool must_be_seen)
{
 std::vector<point> res;
 int max = (dist == 0 ? OMAPX / 2 : dist);
 for (dist = 0; dist <= max; dist++) {
  for (int x = origin.x - dist; x <= origin.x + dist; x++) {
   for (int y = origin.y - dist; y <= origin.y + dist; y++) {
    if (ter(x, y) >= type && ter(x, y) < type + type_range &&
        (!must_be_seen || seen(x, y)))
     res.push_back(point(x, y));
   }
  }
 }
 return res;
}

std::vector<point> overmap::find_terrain(std::string term, int cursx, int cursy)
{
 std::vector<point> found;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (seen(x, y) && oterlist[ter(x, y)].name.find(term) != std::string::npos)
    found.push_back( point(x, y) );
  }
 }
 return found;
}

int overmap::closest_city(point p)
{
 int distance = 999, ret = -1;
 for (int i = 0; i < cities.size(); i++) {
  int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
  if (dist < distance || (dist == distance && cities[i].s < cities[ret].s)) {
   ret = i;
   distance = dist;
  }
 }

 return ret;
}

point overmap::random_house_in_city(int city_id)
{
 if (city_id < 0 || city_id >= cities.size()) {
  debugmsg("overmap::random_house_in_city(%d) (max %d)", city_id,
           cities.size() - 1);
  return point(-1, -1);
 }
 std::vector<point> valid;
 int startx = cities[city_id].x - cities[city_id].s,
     endx   = cities[city_id].x + cities[city_id].s,
     starty = cities[city_id].y - cities[city_id].s,
     endy   = cities[city_id].y + cities[city_id].s;
 for (int x = startx; x <= endx; x++) {
  for (int y = starty; y <= endy; y++) {
   if (ter(x, y) >= ot_house_north && ter(x, y) <= ot_house_west)
    valid.push_back( point(x, y) );
  }
 }
 if (valid.empty())
  return point(-1, -1);

 return valid[ rng(0, valid.size() - 1) ];
}

int overmap::dist_from_city(point p)
{
 int distance = 999;
 for (int i = 0; i < cities.size(); i++) {
  int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
  dist -= cities[i].s;
  if (dist < distance)
   distance = dist;
 }
 return distance;
}

void overmap::draw(WINDOW *w, game *g, int &cursx, int &cursy, 
                   int &origx, int &origy, char &ch, bool blink)
{
 bool legend = true, note_here = false, npc_here = false;
 std::string note_text, npc_name;
 
 int omx, omy;
 overmap hori, vert, diag; // Adjacent maps
 point target(-1, -1);
 if (g->u.active_mission >= 0 &&
     g->u.active_mission < g->u.active_missions.size())
  target = g->find_mission(g->u.active_missions[g->u.active_mission])->target;
  bool see;
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
  bool csee = false;
  oter_id ccur_ter;
  for (int i = -25; i < 25; i++) {
   for (int j = -12; j <= (ch == 'j' ? 13 : 12); j++) {
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
      if ((note_here = diag.has_note(omx, omy)))
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
      if ((note_here = hori.has_note(omx, omy)))
       note_text = hori.note(omx, omy);
     }
    } else if (omy < 0) {
     omy += OMAPY;
     cur_ter = vert.ter(omx, omy);
     see = vert.seen(omx, omy);
     if ((note_here = vert.has_note(omx, omy)))
      note_text = vert.note(omx, omy);
    } else if (omy >= OMAPY) {
     omy -= OMAPY;
     cur_ter = vert.ter(omx, omy);
     see = vert.seen(omx, omy);
     if ((note_here = vert.has_note(omx, omy)))
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
    if (j == 0 && i == 0) {
     mvwputch_hi (w, 12,     25,     ter_color, ter_sym);
     csee = see;
     ccur_ter = cur_ter;
    } else
     mvwputch    (w, 12 + j, 25 + i, ter_color, ter_sym);
   }
  }
  if (target.x != -1 && target.y != -1 && blink &&
      (target.x < cursx - 25 || target.x > cursx + 25  ||
       target.y < cursy - 12 || target.y > cursy + 12    )) {
   switch (direction_from(cursx, cursy, target.x, target.y)) {
    case NORTH:      mvwputch(w,  0, 25, c_red, '^');       break;
    case NORTHEAST:  mvwputch(w,  0, 49, c_red, LINE_OOXX); break;
    case EAST:       mvwputch(w, 12, 49, c_red, '>');       break;
    case SOUTHEAST:  mvwputch(w, 24, 49, c_red, LINE_XOOX); break;
    case SOUTH:      mvwputch(w, 24, 25, c_red, 'v');       break;
    case SOUTHWEST:  mvwputch(w, 24,  0, c_red, LINE_XXOO); break;
    case WEST:       mvwputch(w, 12,  0, c_red, '<');       break;
    case NORTHWEST:  mvwputch(w,  0,  0, c_red, LINE_OXXO); break;
   }
  }
  if (has_note(cursx, cursy)) {
   note_text = note(cursx, cursy);
   for (int i = 0; i < note_text.length(); i++)
    mvwputch(w, 1, i, c_white, LINE_OXOX);
   mvwputch(w, 1, note_text.length(), c_white, LINE_XOOX);
   mvwputch(w, 0, note_text.length(), c_white, LINE_XOXO);
   mvwprintz(w, 0, 0, c_yellow, note_text.c_str());
  } else if (npc_here) {
   for (int i = 0; i < npc_name.length(); i++)
    mvwputch(w, 1, i, c_white, LINE_OXOX);
   mvwputch(w, 1, npc_name.length(), c_white, LINE_XOOX);
   mvwputch(w, 0, npc_name.length(), c_white, LINE_XOXO);
   mvwprintz(w, 0, 0, c_yellow, npc_name.c_str());
  }
  if (legend) {
   cur_ter = ter(cursx, cursy);
// Draw the vertical line
   for (int j = 0; j < 25; j++)
    mvwputch(w, j, 51, c_white, LINE_XOXO);
// Clear the legend
   for (int i = 51; i < 80; i++) {
    for (int j = 0; j < 25; j++)
     mvwputch(w, j, i, c_black, 'x');
   }

   if (csee) {
    mvwputch(w, 1, 51, oterlist[ccur_ter].color, oterlist[ccur_ter].sym);
    mvwprintz(w, 1, 53, oterlist[ccur_ter].color, "%s",
              oterlist[ccur_ter].name.c_str());
   } else
    mvwprintz(w, 1, 51, c_dkgray, "# Unexplored");

   if (target.x != -1 && target.y != -1) {
    int distance = rl_dist(origx, origy, target.x, target.y);
    mvwprintz(w, 3, 51, c_white, "Distance to target: %d", distance);
   }
   mvwprintz(w, 17, 51, c_magenta,           "Use movement keys to pan.  ");
   mvwprintz(w, 18, 51, c_magenta,           "0 - Center map on character");
   mvwprintz(w, 19, 51, c_magenta,           "t - Toggle legend          ");
   mvwprintz(w, 20, 51, c_magenta,           "/ - Search                 ");
   mvwprintz(w, 21, 51, c_magenta,           "N - Add a note             ");
   mvwprintz(w, 22, 51, c_magenta,           "D - Delete a note          ");
   mvwprintz(w, 23, 51, c_magenta,           "L - List notes             ");
   mvwprintz(w, 24, 51, c_magenta,           "Esc or q - Return to game  ");
  }
// Done with all drawing!
  wrefresh(w);
}

point overmap::choose_point(game *g)
{
 WINDOW* w_map = newwin(25, 80, 0, 0);
 WINDOW* w_search = newwin(13, 27, 3, 51);
 timeout(BLINK_SPEED);	// Enable blinking!
 bool blink = true;
 int cursx = (g->levx + int(MAPSIZE / 2)) / 2,
     cursy = (g->levy + int(MAPSIZE / 2)) / 2;
 int origx = cursx, origy = cursy;
 char ch = 0;
 point ret(-1, -1);
 
 do {  
  draw(w_map, g, cursx, cursy, origx, origy, ch, blink);
  ch = input();
  int dirx, diry;
  if (ch != ERR)
   blink = true;	// If any input is detected, make the blinkies on
  get_direction(g, dirx, diry, ch);
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
   add_note(cursx, cursy, string_input_popup(49, "Enter note")); // 49 char max
   timeout(BLINK_SPEED);
  } else if(ch == 'D'){
   timeout(-1);
   if (has_note(cursx, cursy)){
    bool res = query_yn("Really delete note?");
    if (res == true)
     delete_note(cursx, cursy);
   }
   timeout(BLINK_SPEED);
  } else if (ch == 'L'){
   timeout(-1);
   point p = display_notes();
   if (p.x != -1){
    cursx = p.x;
    cursy = p.y;
   }
   timeout(BLINK_SPEED);
   wrefresh(w_map);
  } else if (ch == '/') {
   int tmpx = cursx, tmpy = cursy;
   timeout(-1);
   std::string term = string_input_popup("Search term:");
   timeout(BLINK_SPEED);
   draw(w_map, g, cursx, cursy, origx, origy, ch, blink);
   point found = find_note(point(cursx, cursy), term);
   if (found.x == -1) {	// Didn't find a note
    std::vector<point> terlist;
    terlist = find_terrain(term, origx, origy);
    if (terlist.size() != 0){
     int i = 0;
     //Navigate through results
     do {
      //Draw search box
      wborder(w_search, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
              LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
      mvwprintz(w_search, 1, 1, c_red, "Find place:");
      mvwprintz(w_search, 2, 1, c_ltblue, "                         ");
      mvwprintz(w_search, 2, 1, c_ltblue, "%s", term.c_str());
      mvwprintz(w_search, 4, 1, c_white,
       "'<' '>' Cycle targets.");
      mvwprintz(w_search, 10, 1, c_white, "Enter/Spacebar to select.");
      mvwprintz(w_search, 11, 1, c_white, "q to return.");
      ch = input();
      if (ch == ERR)
       blink = !blink;
      else if (ch == '<') {
       i++;
       if(i > terlist.size() - 1)
        i = 0;
      } else if(ch == '>'){
       i--;
       if(i < 0)
        i = terlist.size() - 1;
      }
      cursx = terlist[i].x;
      cursy = terlist[i].y;       
      draw(w_map, g, cursx, cursy, origx, origy, ch, blink);
      wrefresh(w_search);
      timeout(BLINK_SPEED);
     } while(ch != '\n' && ch != ' ' && ch != 'q'); 
     //If q is hit, return to the last position
     if(ch == 'q'){
      cursx = tmpx;
      cursy = tmpy;
     }
     ch = '.';
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
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && ch != ' ' &&
          ch != '\n');
 timeout(-1);
 werase(w_map);
 wrefresh(w_map);
 delwin(w_map);
 werase(w_search);
 wrefresh(w_search);
 delwin(w_search);
 erase();
 g->refresh_all();
 return ret;
}

void overmap::first_house(int &x, int &y)
{
 std::vector<point> valid;
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j) == ot_shelter)
    valid.push_back( point(i, j) );
  }
 }
 if (valid.size() == 0) {
  debugmsg("Couldn't find a shelter!");
  x = 1;
  y = 1;
  return;
 }
 int index = rng(0, valid.size() - 1);
 x = valid[index].x;
 y = valid[index].y;
}

void overmap::process_mongroups()
{
 for (int i = 0; i < zg.size(); i++) {
  if (zg[i].dying) {
   zg[i].population *= .8;
   zg[i].radius *= .9;
  }
 }
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
   if (rng(0, 99) > 80 * dist(x,y,town.x,town.y) / town.s)
    ter(x+i*xchange, y+i*ychange) = shop(((dir%2)-i)%4);
   else {
    if (rng(0, 99) > 130 * dist(x, y, town.x, town.y) / town.s)
     ter(x+i*xchange, y+i*ychange) = ot_park;
    else
     ter(x+i*xchange, y+i*ychange) = house(((dir%2)-i)%4);
   }
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
  int tries = 0;
  do {
   finalex = rng(x - s, x + s);
   finaley = rng(y - s, y + s);
   tries++;
  } while (tries < 15 && ter(finalex, finaley) != ot_lab &&
                         ter(finalex, finaley) != ot_lab_core);
  ter(finalex, finaley) = ot_lab_finale;
 }
 zg.push_back(mongroup(mcat_lab, (x * 2), (y * 2), s, 400));
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

void overmap::build_mine(int x, int y, int s)
{
 bool finale = (s <= rng(1, 3));
 int built = 0;
 if (s < 2)
  s = 2;
 while (built < s) {
  ter(x, y) = ot_mine;
  std::vector<point> next;
  for (int i = -1; i <= 1; i += 2) {
   if (ter(x, y + i) == ot_rock)
    next.push_back( point(x, y + i) );
   if (ter(x + i, y) == ot_rock)
    next.push_back( point(x + i, y) );
  }
  if (next.empty()) { // Dead end!  Go down!
   ter(x, y) = (finale ? ot_mine_finale : ot_mine_down);
   return;
  }
  point p = next[ rng(0, next.size() - 1) ];
  x = p.x;
  y = p.y;
  built++;
 }
 ter(x, y) = (finale ? ot_mine_finale : ot_mine_down);
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
 int tmp = 0;
 bool bridge_is_okay = false;
 bool found_road = false;
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
    else if (!is_road(base, x, y))
     ter(x, y) = base;
   } else if (next.size() == 1) { // Y must be correct, take the x-change
    if (dir == 1)
     ter(x, y) = base;
    dir = 0; // We are moving horizontally
    x = next[0].x;
    y = next[0].y;
    if (is_river(ter(x, y)))
     ter(x, y) = ot_bridge_ew;
    else if (!is_road(base, x, y))
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
     } else if (!is_road(base, x, y))
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
     } else if (!is_road(base, x, y))
      ter(x, y) = base;
    }
   }
/*
   if (one_in(50) && posz == 0)
    building_on_hiway(x, y, dir);
*/
  }
  found_road = (
        ((ter(x, y - 1) > ot_road_null && ter(x, y - 1) < ot_river_center) ||
         (ter(x, y + 1) > ot_road_null && ter(x, y + 1) < ot_river_center) ||
         (ter(x - 1, y) > ot_road_null && ter(x - 1, y) < ot_river_center) ||
         (ter(x + 1, y) > ot_road_null && ter(x + 1, y) < ot_river_center)  ) &&
        rl_dist(x, y, x1, y2) > rl_dist(x, y, x2, y2));
 } while ((x != x2 || y != y2) && !found_road);
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
    else if (ter(x, y) >= ot_bridge_ns && ter(x, y) <= ot_bridge_ew &&
             ter(x - 1, y) >= ot_bridge_ns && ter(x - 1, y) <= ot_bridge_ew &&
             ter(x + 1, y) >= ot_bridge_ns && ter(x + 1, y) <= ot_bridge_ew &&
             ter(x, y - 1) >= ot_bridge_ns && ter(x, y - 1) <= ot_bridge_ew &&
             ter(x, y + 1) >= ot_bridge_ns && ter(x, y + 1) <= ot_bridge_ew)
     ter(x, y) = ot_road_nesw;
    else if (ter(x, y) >= ot_subway_ns && ter(x, y) <= ot_subway_nesw)
     good_road(ot_subway_ns, x, y);
    else if (ter(x, y) >= ot_sewer_ns && ter(x, y) <= ot_sewer_nesw)
     good_road(ot_sewer_ns, x, y);
    else if (ter(x, y) >= ot_ants_ns && ter(x, y) <= ot_ants_nesw)
     good_road(ot_ants_ns, x, y);
    else if (ter(x, y) >= ot_river_center && ter(x, y) < ot_river_nw)
     good_river(x, y);
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
     (ter(x, y) >= ot_subway_ns && ter(x, y) <= ot_subway_nesw) ||
     (ter(x, y) >= ot_sewer_ns  && ter(x, y) <= ot_sewer_nesw) ||
     ter(x, y) == ot_sewage_treatment_hub ||
     ter(x, y) == ot_sewage_treatment_under)
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
  if (ter(x, y) == ot_sewage_treatment_hub ||
      ter(x, y) == ot_sewage_treatment_under )
   return true;
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

void overmap::place_specials()
{
 int placed[NUM_OMSPECS];
 for (int i = 0; i < NUM_OMSPECS; i++)
  placed[i] = 0;

 std::vector<point> sectors;
 for (int x = 0; x < OMAPX; x += OMSPEC_FREQ) {
  for (int y = 0; y < OMAPY; y += OMSPEC_FREQ)
   sectors.push_back(point(x, y));
 }

 while (!sectors.empty()) {
  int sector_pick = rng(0, sectors.size() - 1);
  int x = sectors[sector_pick].x, y = sectors[sector_pick].y;
  sectors.erase(sectors.begin() + sector_pick);
  std::vector<omspec_id> valid;
  int tries = 0;
  point p;
  do {
   p = point(rng(x, x + OMSPEC_FREQ - 1), rng(y, y + OMSPEC_FREQ - 1));
   if (p.x >= OMAPX - 1)
    p.x = OMAPX - 2;
   if (p.y >= OMAPY - 1)
    p.y = OMAPY - 2;
   if (p.x == 0)
    p.x = 1;
   if (p.y == 0)
    p.y = 1;
   for (int i = 0; i < NUM_OMSPECS; i++) {
    omspec_place place;
    overmap_special special = overmap_specials[i];
    int min = special.min_dist_from_city, max = special.max_dist_from_city;
    if ((placed[i] < special.max_appearances || special.max_appearances <= 0) &&
        (min == -1 || dist_from_city(p) >= min) &&
        (max == -1 || dist_from_city(p) <= max) &&
        (place.*special.able)(this, p))
     valid.push_back( omspec_id(i) );
   }
   tries++;
  } while (valid.empty() && tries < 15); // Done looking for valid spot

  if (tries < 15) { // We found a valid spot!
// Place the MUST HAVE ones first, to try and guarantee that they appear
   std::vector<omspec_id> must_place;
   for (int i = 0; i < valid.size(); i++) {
    if (placed[i] < overmap_specials[ valid[i] ].min_appearances)
     must_place.push_back(valid[i]);
   }
   if (must_place.empty()) {
    int selection = rng(0, valid.size() - 1);
    overmap_special special = overmap_specials[ valid[selection] ];
    placed[ valid[selection] ]++;
    place_special(special, p);
   } else {
    int selection = rng(0, must_place.size() - 1);
    overmap_special special = overmap_specials[ must_place[selection] ];
    placed[ must_place[selection] ]++;
    place_special(special, p);
   }
  } // Done with <Found a valid spot>

 } // Done picking sectors...
}

void overmap::place_special(overmap_special special, point p)
{
 bool rotated = false;
// First, place terrain...
 ter(p.x, p.y) = special.ter;
// Next, obey any special effects the flags might have
 if (special.flags & mfb(OMS_FLAG_ROTATE_ROAD)) {
  if (is_road(p.x, p.y - 1))
   rotated = true;
  else if (is_road(p.x + 1, p.y)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 1);
   rotated = true;
  } else if (is_road(p.x, p.y + 1)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 2);
   rotated = true;
  } else if (is_road(p.x - 1, p.y)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 3);
   rotated = true;
  }
 }

 if (!rotated && special.flags & mfb(OMS_FLAG_ROTATE_RANDOM))
  ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + rng(0, 3) );
  
 if (special.flags & mfb(OMS_FLAG_3X3)) {
  for (int x = -1; x <= 1; x++) {
   for (int y = -1; y <= 1; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    point np(p.x + x, p.y + y);
    ter(np.x, np.y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_3X3_SECOND)) {
  int startx = p.x - 1, starty = p.y;
  if (is_road(p.x, p.y - 1)) { // Road to north
   startx = p.x - 1;
   starty = p.y;
  } else if (is_road(p.x + 1, p.y)) { // Road to east
   startx = p.x - 2;
   starty = p.y - 1;
  } else if (is_road(p.x, p.y + 1)) { // Road to south
   startx = p.x - 1;
   starty = p.y - 2;
  } else if (is_road(p.x - 1, p.y)) { // Road to west
   startx = p.x;
   starty = p.y - 1;
  }
  if (startx != -1) {
   for (int x = startx; x < startx + 3; x++) {
    for (int y = starty; y < starty + 3; y++)
     ter(x, y) = oter_id(special.ter + 1);
   }
   ter(p.x, p.y) = special.ter;
  }
 }

 if (special.flags & mfb(OMS_FLAG_BLOB)) {
  for (int x = -2; x <= 2; x++) {
   for (int y = -2; y <= 2; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    omspec_place place;
    point np(p.x + x, p.y + y);
    if (one_in(1 + abs(x) + abs(y)) && (place.*special.able)(this, np))
     ter(p.x + x, p.y + y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_BIG)) {
  for (int x = -3; x <= 3; x++) {
   for (int y = -3; y <= 3; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    omspec_place place;
    point np(p.x + x, p.y + y);
    if ((place.*special.able)(this, np))
     ter(p.x + x, p.y + y) = special.ter;
     ter(p.x + x, p.y + y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_ROAD)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  make_hiway(p.x, p.y, cities[closest].x, cities[closest].y, ot_road_null);
 }

 if (special.flags & mfb(OMS_FLAG_PARKING_LOT)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  ter(p.x, p.y - 1) = ot_s_lot;
  make_hiway(p.x, p.y - 1, cities[closest].x, cities[closest].y, ot_road_null);
 }

// Finally, place monsters if applicable
 if (special.monsters != mcat_null) {
  if (special.monster_pop_min == 0 || special.monster_pop_max == 0 ||
      special.monster_rad_min == 0 || special.monster_rad_max == 0   ) {
   debugmsg("Overmap special %s has bad spawn: pop(%d, %d) rad(%d, %d)",
            oterlist[special.ter].name.c_str(), special.monster_pop_min,
            special.monster_pop_max, special.monster_rad_min,
            special.monster_rad_max);
   return;
  }
       
  int population = rng(special.monster_pop_min, special.monster_pop_max);
  int radius     = rng(special.monster_rad_min, special.monster_rad_max);
  zg.push_back(
     mongroup(special.monsters, p.x * 2, p.y * 2, radius, population));
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

// Figure out where swamps are, and place swamp monsters
 for (int x = 3; x < OMAPX - 3; x += 7) {
  for (int y = 3; y < OMAPY - 3; y += 7) {
   int swamp_count = 0;
   for (int sx = x - 3; sx <= x + 3; sx++) {
    for (int sy = y - 3; sy <= y + 3; sy++) {
     if (ter(sx, sy) == ot_forest_water)
      swamp_count += 2;
     else if (is_river(ter(sx, sy)))
      swamp_count++;
    }
   }
   if (swamp_count >= 25) // ~25% swamp or ~50% river
    zg.push_back(mongroup(mcat_swamp, x * 2, y * 2, 3,
                          rng(swamp_count * 8, swamp_count * 25)));
  }
 }

// Place the "put me anywhere" groups
 int numgroups = rng(0, 3);
 for (int i = 0; i < numgroups; i++) {
  zg.push_back(
	mongroup(mcat_worm, rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1),
	         rng(20, 40), rng(500, 1000)));
 }

// Forest groups cover the entire map
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY, OMAPY,
                 rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY * 2 - 1, OMAPY,
                 rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back(
	mongroup(mcat_forest, OMAPX, 0, OMAPX,
                 rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back(
	mongroup(mcat_forest, OMAPX * 2 - 1, 0, OMAPX,
                 rng(2000, 12000)));
 zg.back().diffuse = true;
}

void overmap::place_radios()
{
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j) == ot_radio_tower)
    if(one_in(2))
     radios.push_back(radio_tower(i*2, j*2, rng(80, 200),
    "This is the emergency broadcast system.  Please proceed quickly and calmly \
to your designated evacuation point."));
    else
     radios.push_back(radio_tower(i*2, j*2, rng(80, 200),
				  "Head West.  All survivors, head West.  Help is waiting."));
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
    int(zg[i].radius) << " " << zg[i].population << " " << zg[i].diffuse <<
    std::endl;
 for (int i = 0; i < cities.size(); i++)
  fout << "t " << cities[i].x << " " << cities[i].y << " " << cities[i].s <<
          std::endl;
 for (int i = 0; i < roads_out.size(); i++)
  fout << "R " << roads_out[i].x << " " << roads_out[i].y << std::endl;
 for (int i = 0; i < radios.size(); i++)
  fout << "T " << radios[i].x << " " << radios[i].y << " " <<
          radios[i].strength << " " << std::endl << radios[i].message <<
          std::endl;

 for (int i = 0; i < npcs.size(); i++)
  fout << "n " << npcs[i].save_info() << std::endl;

 fout.close();
}

void overmap::open(game *g, int x, int y, int z)
{
 std::stringstream plrfilename, terfilename;
 std::ifstream fin;
 char datatype;
 int ct, cx, cy, cs, cp, cd;
 city tmp;
 std::vector<item> npc_inventory;

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
    fin >> ct >> cx >> cy >> cs >> cp >> cd;
    zg.push_back(mongroup(moncat_id(ct), cx, cy, cs, cp));
    zg.back().diffuse = cd;
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
/* When we start loading a new NPC, check to see if we've accumulated items for
   assignment to an NPC.
 */
    if (!npc_inventory.empty() && !npcs.empty()) {
     npcs.back().inv.add_stack(npc_inventory);
     npc_inventory.clear();
    }
    std::string npcdata;
    getline(fin, npcdata);
    npc tmp;
    tmp.load_info(g, npcdata);
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
      case 'I': npc_inventory.push_back(tmp);                 break;
      case 'C': npc_inventory.back().contents.push_back(tmp); break;
      case 'W': last->worn.push_back(tmp);                    break;
      case 'w': last->weapon = tmp;                           break;
      case 'c': last->weapon.contents.push_back(tmp);         break;
     }
    }
   }
  }
// If we accrued an npc_inventory, assign it now
  if (!npc_inventory.empty() && !npcs.empty())
   npcs.back().inv.add_stack(npc_inventory);

// Private/per-character data
  fin.close();
  fin.open(plrfilename.str().c_str());
  if (fin.is_open()) {	// Load private seen data
   for (int j = 0; j < OMAPY; j++) {
    std::string dataline;
    getline(fin, dataline);
    for (int i = 0; i < OMAPX; i++) {
     if (dataline[i] == '1')
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


// Overmap special placement functions

bool omspec_place::water(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter >= ot_river_center && ter <= ot_river_nw);
}

bool omspec_place::land(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter < ot_river_center || ter > ot_river_nw);
}

bool omspec_place::forest(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter == ot_forest || ter == ot_forest_thick || ter == ot_forest_water);
}

bool omspec_place::wilderness(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter == ot_forest || ter == ot_forest_thick || ter == ot_forest_water ||
         ter == ot_field);
}

bool omspec_place::by_highway(overmap *om, point p)
{
 oter_id north = om->ter(p.x, p.y - 1), east = om->ter(p.x + 1, p.y),
         south = om->ter(p.x, p.y + 1), west = om->ter(p.x - 1, p.y);

 return ((north == ot_hiway_ew || north == ot_road_ew) ||
         (east  == ot_hiway_ns || east  == ot_road_ns) ||
         (south == ot_hiway_ew || south == ot_road_ew) ||
         (west  == ot_hiway_ns || west  == ot_road_ns)   );
}

std::ostream & operator<<(std::ostream & out, const overmap * om)
{
 out << "overmap(";
 if( !om )
 {
  out << "NULL)";
  return out;
 }

 out << "\n\t cities: " << om->cities;
 out << "\n\t roads_out: " << om->roads_out;
 out << "\n\t towns: " << om->towns;
 out << "\n\t zg: skipped";
 out << "\n\t radios: skipped";

 out << "\n\t posx: "<<om->posx << ", ";
 out << "posy: "<<om->posy << ", ";
 out << "posz: "<<om->posz << ", ";

 out << "\n\t npcs: " << om->npcs.size();

 out << "\n\t t: ";
 for(int x=0; x<OMAPX; ++x)
 {
  out << "\n\t  " << x << ": ";
  for(int y=0; y<OMAPY; ++y)
   out << om->t[x][y] << ", ";
 }

 out << "\n\t nullret: " << om->nullret;

 out << "\n\t s: ";
 for(int x=0; x<OMAPX; ++x)
 {
  out << "\n\t  " << x << ": ";
  for(int y=0; y<OMAPY; ++y)
   out << om->s[x][y] << ", ";
 }

 out << "\n\t nullbool: " << om->nullbool;

 out << "\n\t notes: " << om->notes.size();

 out << "\n\t)";
 return out;
}

std::ostream & operator<<(std::ostream & out, const overmap & om)
{
 out << (&om);
 return out;
}

std::ostream & operator<<(std::ostream & out, const city & c)
{
 out << "city("<<c.x<<","<<c.y<<","<<c.s<<")";
 return out;
}
