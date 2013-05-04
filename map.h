#ifndef _MAP_H_
#define _MAP_H_

#include "cursesdef.h"

#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <map>

#include "mapdata.h"
#include "mapitems.h"
#include "overmap.h"
#include "item.h"
#include "monster.h"
#include "npc.h"
#include "vehicle.h"
#include "graffiti.h"
#include "lightmap.h"

//TODO: include comments about how these variables work. Where are they used. Are they constant etc.
#define MAPSIZE 11
#define CAMPSIZE 1
#define CAMPCHECK 3

class player;
class item;
struct itype;

// TODO: This should be const& but almost no functions are const
struct wrapped_vehicle{
 int x;
 int y;
 int i; // submap col
 int j; // submap row
 vehicle* v;
};

typedef std::vector<wrapped_vehicle> VehicleList;

class map
{
 public:

// Constructors & Initialization
 map();
 map(std::map<std::string, itype*>* itptr, std::vector<itype_id> (*miptr)[num_itloc],
     std::vector<trap*> *trptr);
 ~map();

// Visual Output
 void draw(game *g, WINDOW* w, const point center);
 void debug();
 void drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert, const bool show_items,
             const int view_center_x = -1, const int view_center_y = -1,
             const bool low_light = false, const bool bright_level = false);

// File I/O
 virtual void save(overmap *om, unsigned const int turn, const int x, const int y, const int z);
 virtual void load(game *g, const int wx, const int wy, const int wz, const bool update_vehicles = true);
 void shift(game *g, const int wx, const int wy, const int wz, const int x, const int y);
 void spawn_monsters(game *g);
 void clear_spawns();
 void clear_traps();

// Movement and LOS
 int move_cost(const int x, const int y); // Cost to move through; 0 = impassible
 int move_cost_ter_only(const int x, const int y); // same as above, but don't take vehicles into account
 bool trans(const int x, const int y); // Transparent?
 // (Fx, Fy) sees (Tx, Ty), within a range of (range)?
 // tc indicates the Bresenham line used to connect the two points, and may
 //  subsequently be used to form a path between them
 bool sees(const int Fx, const int Fy, const int Tx, const int Ty,
           const int range, int &tc);
// clear_path is the same idea, but uses cost_min <= move_cost <= cost_max
 bool clear_path(const int Fx, const int Fy, const int Tx, const int Ty,
                 const int range, const int cost_min, const int cost_max, int &tc);
// route() generates an A* best path; if bash is true, we can bash through doors
 std::vector<point> route(const int Fx, const int Fy, const int Tx, const int Ty,
                          const bool bash = true);

// vehicles
 VehicleList get_vehicles();
 VehicleList get_vehicles(const int sx, const int sy, const int ex, const int ey);

// checks, if tile is occupied by vehicle and by which part
 vehicle* veh_at(const int x, const int y, int &part_num);
 vehicle* veh_at(const int x, const int y);// checks, if tile is occupied by vehicle
 // put player on vehicle at x,y
 void board_vehicle(game *g, int x, int y, player *p);
 void unboard_vehicle(game *g, const int x, const int y);//remove player from vehicle at x,y
 void update_vehicle_cache(vehicle *, const bool brand_new = false);
 void reset_vehicle_cache();
 void clear_vehicle_cache();
 void update_vehicle_list(const int to);

 void destroy_vehicle (vehicle *veh);
// Change vehicle coords and move vehicle's driver along.
// Returns true, if there was a submap change.
// If test is true, function only checks for submap change, no displacement
// WARNING: not checking collisions!
 bool displace_vehicle (game *g, int &x, int &y, const int dx, const int dy, bool test);
 void vehmove(game* g);          // Vehicle movement
 bool vehproceed(game* g);
// move water under wheels. true if moved
 bool displace_water (const int x, const int y);

// Terrain
 ter_id ter(const int x, const int y) const; // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 void ter_set(const int x, const int y, const ter_id new_terrain);
 std::string tername(const int x, const int y) const; // Name of terrain at (x, y)
 std::string features(const int x, const int y); // Words relevant to terrain (sharp, etc)
 bool has_flag(const t_flag flag, const int x, const int y);  // checks terrain and vehicles
 bool has_flag_ter_only(const t_flag flag, const int x, const int y) const; // only checks terrain
 bool is_destructable(const int x, const int y);        // checks terrain and vehicles
 bool is_destructable_ter_only(const int x, const int y);       // only checks terrain
 bool is_outside(const int x, const int y);
 bool flammable_items_at(const int x, const int y);
 bool moppable_items_at(const int x, const int y);
 point random_outdoor_tile();

 void translate(const ter_id from, const ter_id to); // Change all instances of $from->$to
 void translate_radius(const ter_id from, const ter_id to, const float radi, const int uX, const int uY); 
 bool close_door(const int x, const int y, const bool inside);
 bool open_door(const int x, const int y, const bool inside);
 // bash: if res pointer is supplied, res will contain absorbed impact or -1
 bool bash(const int x, const int y, const int str, std::string &sound, int *res = 0);
 void destroy(game *g, const int x, const int y, const bool makesound);
 void shoot(game *g, const int x, const int y, int &dam, const bool hit_items, const unsigned flags);
 bool hit_with_acid(game *g, const int x, const int y);
 bool hit_with_fire(game *g, const int x, const int y); 
 void marlossify(const int x, const int y);
 bool has_adjacent_furniture(const int x, const int y);
 void mop_spills(const int x, const int y);

// Radiation
 int& radiation(const int x, const int y);	// Amount of radiation at (x, y);

// Items
 std::vector<item>& i_at(int x, int y);
 item water_from(const int x, const int y);
 void i_clear(const int x, const int y);
 void i_rem(const int x, const int y, const int index);
 point find_item(const item *it);
 void spawn_item(const int x, const int y, itype* type, int birthday, int quantity = 0, int charges = 0);
 void spawn_item(const int x, const int y, std::string itype_id, int birthday, int quantity = 0, int charges = 0);
 void add_item(const int x, const int y, item new_item);
 void process_active_items(game *g);
 void process_active_items_in_submap(game *g, const int nonant);
 void process_vehicles(game *g);

 void use_amount(const point origin, const int range, const itype_id type, const int amount,
                 const bool use_container = false);
 void use_charges(const point origin, const int range, const itype_id type, const int amount);

// Traps
 trap_id& tr_at(const int x, const int y);
 void add_trap(const int x, const int y, const trap_id t);
 void disarm_trap( game *g, const int x, const int y);

// Fields
 field& field_at(const int x, const int y);
 bool add_field(game *g, const int x, const int y, const field_id t, const unsigned char density);
 void remove_field(const int x, const int y);
 bool process_fields(game *g);				// See fields.cpp
 bool process_fields_in_submap(game *g, const int gridn);	// See fields.cpp
 void step_in_field(const int x, const int y, game *g);		// See fields.cpp
 void mon_in_field(const int x, const int y, game *g, monster *z);	// See fields.cpp

// Computers
 computer* computer_at(const int x, const int y);

 // Camps
 bool allow_camp(const int x, const int y, const int radius = CAMPCHECK);
 basecamp* camp_at(const int x, const int y, const int radius = CAMPSIZE);
 void add_camp(const std::string& name, const int x, const int y);

// Graffiti
 graffiti graffiti_at(int x, int y);
 bool add_graffiti(game *g, int x, int y, std::string contents);

// mapgen.cpp functions
 void generate(game *g, overmap *om, const int x, const int y, const int z, const int turn);
 void post_process(game *g, unsigned zones);
 void place_spawns(game *g, std::string group, const int chance,
                   const int x1, const int y1, const int x2, const int y2, const float density);
 void place_items(items_location loc, const int chance, const int x1, const int y1,
                  const int x2, const int y2, bool ongrass, const int turn);
// put_items_from puts exactly num items, based on chances
 void put_items_from(items_location loc, const int num, const int x, const int y, const int turn = 0);
 void add_spawn(mon_id type, const int count, const int x, const int y, bool friendly = false,
                const int faction_id = -1, const int mission_id = -1,
                std::string name = "NONE");
 void add_spawn(monster *mon);
 void create_anomaly(const int cx, const int cy, artifact_natural_property prop);
 vehicle *add_vehicle(game *g, vhtype_id type, const int x, const int y, const int dir);
 computer* add_computer(const int x, const int y, std::string name, const int security);
 float light_transparency(const int x, const int y) const;
 void build_map_cache(game *g);
 lit_level light_at(int dx, int dy); // Assumes 0,0 is light map center
 float ambient_light_at(int dx, int dy); // Raw values for tilesets
 bool pl_sees(int fx, int fy, int tx, int ty, int max_range);

 std::map<std::string, itype*>* itypes;
 std::set<vehicle*> vehicle_list;
 std::map< std::pair<int,int>, std::pair<vehicle*,int> > veh_cached_parts;
 bool veh_exists_at [SEEX * MAPSIZE][SEEY * MAPSIZE];

protected:
 void saven(overmap *om, unsigned const int turn, const int x, const int y, const int z,
            const int gridx, const int gridy);
 bool loadn(game *g, const int x, const int y, const int z, const int gridx, const int gridy,
            const  bool update_vehicles = true);
 void copy_grid(const int to, const int from);
 void draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
               const oter_id t_south, const oter_id t_west, const oter_id t_above, const int turn,
               game *g, const float density);
 void add_extra(map_extra type, game *g);
 void rotate(const int turns);// Rotates the current map 90*turns degress clockwise
			// Useful for houses, shops, etc
 void build_transparency_cache();
 void build_outside_cache(const game *g);
 void generate_lightmap(game *g);
 void build_seen_cache(game *g);

 bool inbounds(const int x, const int y);
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

 bool veh_in_active_range;

private:
 long determine_wall_corner(const int x, const int y, const long orig_sym);
 void cache_seen(const int fx, const int fy, const int tx, const int ty, const int max_range);
 void apply_light_source(int x, int y, float luminance);
 void apply_light_arc(int x, int y, int angle, float luminance);
 void apply_light_ray(bool lit[MAPSIZE*SEEX][MAPSIZE*SEEY],
                      int sx, int sy, int ex, int ey, float luminance);

 float lm[MAPSIZE*SEEX][MAPSIZE*SEEY];
 float sm[MAPSIZE*SEEX][MAPSIZE*SEEY];
 bool outside_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 float transparency_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 bool seen_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 submap* grid[MAPSIZE * MAPSIZE];
};

class tinymap : public map
{
public:
 tinymap();
 tinymap(std::map<std::string, itype*> *itptr, std::vector<itype_id> (*miptr)[num_itloc],
     std::vector<trap*> *trptr);
 ~tinymap();

protected:
 virtual bool is_tiny() { return true; };

private:
 submap* grid[4];
};

#endif
