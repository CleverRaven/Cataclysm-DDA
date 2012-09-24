#ifndef _MAP_H_
#define _MAP_H_

#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#else
	#include <curses.h>
#endif

#include <stdlib.h>
#include <vector>
#include <string>

#include "mapdata.h"
#include "mapitems.h"
#include "overmap.h"
#include "item.h"
#include "monster.h"
#include "npc.h"
#include "vehicle.h"

#define MAPSIZE 11

class player;
class item;
struct itype;

class map
{
 public:
// Constructors & Initialization
 map();
 map(std::vector<itype*> *itptr, std::vector<itype_id> (*miptr)[num_itloc],
     std::vector<trap*> *trptr);
 ~map();

// Visual Output
 void draw(game *g, WINDOW* w, point center);
 void debug();
 void drawsq(WINDOW* w, player &u, int x, int y, bool invert, bool show_items,
             int cx = -1, int cy = -1);

// File I/O
 virtual void save(overmap *om, unsigned int turn, int x, int y);
 virtual void load(game *g, int wx, int wy);
 void shift(game *g, int wx, int wy, int x, int y);
 void spawn_monsters(game *g);
 void clear_spawns();
 void clear_traps();

// Movement and LOS
 int move_cost(int x, int y); // Cost to move through; 0 = impassible
 int move_cost_ter_only(int x, int y); // same as above, but don't take vehicles into account
 bool trans(int x, int y); // Transparent?
 // (Fx, Fy) sees (Tx, Ty), within a range of (range)?
 // tc indicates the Bresenham line used to connect the two points, and may
 //  subsequently be used to form a path between them
 bool sees(int Fx, int Fy, int Tx, int Ty, int range, int &tc);
// clear_path is the same idea, but uses cost_min <= move_cost <= cost_max
 bool clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min,
                 int cost_max, int &tc);
// route() generates an A* best path; if bash is true, we can bash through doors
 std::vector<point> route(int Fx, int Fy, int Tx, int Ty, bool bash = true);

// vehicles
// checks, if tile is occupied by vehicle and by which part
 vehicle* veh_at(int x, int y, int &part_num);
 vehicle* veh_at(int x, int y);// checks, if tile is occupied by vehicle
 // put player on vehicle at x,y
 void board_vehicle(game *g, int x, int y, player *p);
 void unboard_vehicle(game *g, int x, int y);//remove player from vehicle at x,y

 void destroy_vehicle (vehicle *veh);
// Change vehicle coords and move vehicle's driver along.
// Returns true, if there was a submap change.
// If test is true, function only checks for submap change, no displacement
// WARNING: not checking collisions!
 bool displace_vehicle (game *g, int &x, int &y, int dx, int dy, bool test);
 void vehmove(game* g);          // Vehicle movement
// move water under wheels. true if moved
 bool displace_water (int x, int y);

// Terrain
 ter_id& ter(int x, int y); // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 std::string tername(int x, int y); // Name of terrain at (x, y)
 std::string features(int x, int y); // Words relevant to terrain (sharp, etc)
 bool has_flag(t_flag flag, int x, int y);  // checks terrain and vehicles
 bool has_flag_ter_only(t_flag flag, int x, int y); // only checks terrain
 bool is_destructable(int x, int y);        // checks terrain and vehicles
 bool is_destructable_ter_only(int x, int y);       // only checks terrain
 bool is_outside(int x, int y);
 bool flammable_items_at(int x, int y);
 point random_outdoor_tile();

 void translate(ter_id from, ter_id to); // Change all instances of $from->$to
 bool close_door(int x, int y);
 bool open_door(int x, int y, bool inside);
 // bash: if res pointer is supplied, res will contain absorbed impact or -1
 bool bash(int x, int y, int str, std::string &sound, int *res = 0);
 void destroy(game *g, int x, int y, bool makesound);
 void shoot(game *g, int x, int y, int &dam, bool hit_items, unsigned flags);
 bool hit_with_acid(game *g, int x, int y);
 void marlossify(int x, int y);

// Radiation
 int& radiation(int x, int y);	// Amount of radiation at (x, y);

// Items
 std::vector<item>& i_at(int x, int y);
 item water_from(int x, int y);
 void i_clear(int x, int y);
 void i_rem(int x, int y, int index);
 point find_item(item *it);
 void add_item(int x, int y, itype* type, int birthday);
 void add_item(int x, int y, item new_item);
 void process_active_items(game *g);
 void process_active_items_in_submap(game *g, int nonant);
 void process_vehicles(game *g);

 void use_amount(point origin, int range, itype_id type, int quantity,
                 bool use_container = false);
 void use_charges(point origin, int range, itype_id type, int quantity);

// Traps
 trap_id& tr_at(int x, int y);
 void add_trap(int x, int y, trap_id t);
 void disarm_trap( game *g, int x, int y);

// Fields
 field& field_at(int x, int y);
 bool add_field(game *g, int x, int y, field_id t, unsigned char density);
 void remove_field(int x, int y);
 bool process_fields(game *g);				// See fields.cpp
 bool process_fields_in_submap(game *g, int gridn);	// See fields.cpp
 void step_in_field(int x, int y, game *g);		// See fields.cpp
 void mon_in_field(int x, int y, game *g, monster *z);	// See fields.cpp

// Computers
 computer* computer_at(int x, int y);

// mapgen.cpp functions
 void generate(game *g, overmap *om, int x, int y, int turn);
 void post_process(game *g, unsigned zones);
 void place_items(items_location loc, int chance, int x1, int y1,
                  int x2, int y2, bool ongrass, int turn);
// put_items_from puts exactly num items, based on chances
 void put_items_from(items_location loc, int num, int x, int y, int turn = 0);
 void make_all_items_owned();
 void add_spawn(mon_id type, int count, int x, int y, bool friendly = false,
                int faction_id = -1, int mission_id = -1,
                std::string name = "NONE");
 void add_spawn(monster *mon);
 void create_anomaly(int cx, int cy, artifact_natural_property prop);
 vehicle *add_vehicle(game *g, vhtype_id type, int x, int y, int dir);
 computer* add_computer(int x, int y, std::string name, int security);
 
 std::vector <itype*> *itypes;

protected:
 void saven(overmap *om, unsigned int turn, int x, int y, int gridx, int gridy);
 bool loadn(game *g, int x, int y, int gridx, int gridy);
 void copy_grid(int to, int from);
 void draw_map(oter_id terrain_type, oter_id t_north, oter_id t_east,
               oter_id t_south, oter_id t_west, oter_id t_above, int turn,
               game *g);
 void add_extra(map_extra type, game *g);
 void rotate(int turns);// Rotates the current map 90*turns degress clockwise
			// Useful for houses, shops, etc

 bool inbounds(int x, int y);
 int my_MAPSIZE;
 virtual bool is_tiny() { return false; };

 std::vector<item> nulitems; // Returned when &i_at() is asked for an OOB value
 ter_id nulter;	// Returned when &ter() is asked for an OOB value
 trap_id nultrap; // Returned when &tr_at() is asked for an OOB value
 field nulfield; // Returned when &field_at() is asked for an OOB value
 vehicle nulveh; // Returned when &veh_at() is asked for an OOB value
 int nulrad;	// OOB &radiation()

 std::vector <trap*> *traps;
 std::vector <itype_id> (*mapitems)[num_itloc];

private:
 submap* grid[MAPSIZE * MAPSIZE];
};

class tinymap : public map
{
public:
 tinymap();
 tinymap(std::vector<itype*> *itptr, std::vector<itype_id> (*miptr)[num_itloc],
     std::vector<trap*> *trptr);
 ~tinymap();

protected:
 virtual bool is_tiny() { return true; };

private:
 submap* grid[4];
};

#endif
