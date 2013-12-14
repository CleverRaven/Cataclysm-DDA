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
#include "coordinates.h"
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

/**
 * Manage and cache data about a part of the map.
 *
 * Despite the name, this class isn't actually responsible for managing the map as a whole. For that function,
 * see \ref mapbuffer. Instead, this class loads a part of the mapbuffer into a cache, and adds certain temporary
 * information such as lighting calculations to it.
 *
 * To understand the following descriptions better, you should also read \ref map_management
 *
 * The map coordinates always start at (0, 0) for the top-left and end at (map_width-1, map_height-1) for the bottom-right.
 *
 * The actual map data is stored in `submap` instances. These instances are managed by `mapbuffer`.
 * References to the currently active submaps are stored in `map::grid`:
 *     0 1 2
 *     3 4 5
 *     6 7 8
 * In this example, the top-right submap would be at `grid[2]`.
 *
 * When the player moves between submaps, the whole map is shifted, so that if the player moves one submap to the right,
 * (0, 0) now points to a tile one submap to the right from before
 */
class map
{
 friend class editmap;
 public:
// Constructors & Initialization
 map();
 map(std::vector<trap*> *trptr);
 ~map();

// Visual Output
 void debug();

 /** Draw a visible part of the map into `w`.
  *
  * This method uses `g->u.posx/posy` for visibility calculations, so it can
  * not be used for anything but the player's viewport. Likewise, only
  * `g->m` and maps with equivalent coordinates can be used, as other maps
  * would have coordinate systems incompatible with `g->u.posx`
  *
  * @param center The coordinate of the center of the viewport, this can
  *               be different from the player coordinate.
  */
 void draw(game *g, WINDOW* w, const point center);

 /** Draw the map tile at the given coordinate. Called by `map::draw()`.
  *
  * @param x, y The tile on this map to draw.
  * @param cx, cy The center of the viewport to be rendered, see `center` in `map::draw()`
  */
 void drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert, const bool show_items,
             const int view_center_x = -1, const int view_center_y = -1,
             const bool low_light = false, const bool bright_level = false);

// File I/O
 virtual void save(overmap *om, unsigned const int turn, const int x, const int y, const int z);
 virtual void load(game *g, const int wx, const int wy, const int wz, const bool update_vehicles = true, overmap *om = NULL);
 void shift(game *g, const int wx, const int wy, const int wz, const int x, const int y);
 void spawn_monsters(game *g);
 void clear_spawns();
 void clear_traps();

// Movement and LOS

 /**
  * Calculate the cost to move past the tile at (x, y).
  *
  * The move cost is determined by various obstacles, such
  * as terrain, vehicles and furniture.
  *
  * @note Movement costs for players and zombies both use this function.
  *
  * @return The return value is interpreted as follows:
  * Move Cost | Meaning
  * --------- | -------
  * 0         | Impassable
  * n > 0     | x*n turns to move past this
  */
 int move_cost(const int x, const int y, const vehicle *ignored_vehicle = NULL) const;


 /**
  * Similar behavior to `move_cost()`, but ignores vehicles.
  */
 int move_cost_ter_furn(const int x, const int y) const;

 /**
  * Cost to move out of one tile and into the next.
  *
  * @return The cost in turns to move out of `(x1, y1)` and into `(x2, y2)`
  */
 int combined_movecost(const int x1, const int y1, const int x2, const int y2,
                       const vehicle *ignored_vehicle = NULL, const int modifier = 0);

 /**
  * Returns whether the tile at `(x, y)` is transparent(you can look past it).
  */
 bool trans(const int x, const int y); // Transparent?

 /**
  * Returns whether `(Fx, Fy)` sees `(Tx, Ty)` with a view range of `range`.
  *
  * @param tc Indicates the Bresenham line used to connect the two points, and may
  *           subsequently be used to form a path between them
  */
 bool sees(const int Fx, const int Fy, const int Tx, const int Ty,
           const int range, int &tc);

 /**
  * Check whether there's a direct line of sight between `(Fx, Fy)` and
  * `(Tx, Ty)` with the additional movecost restraints.
  *
  * Checks two things:
  * 1. The `sees()` algorithm between `(Fx, Fy)` and `(Tx, Ty)`
  * 2. That moving over the line of sight would have a move_cost between
  *    `cost_min` and `cost_max`.
  */
 bool clear_path(const int Fx, const int Fy, const int Tx, const int Ty,
                 const int range, const int cost_min, const int cost_max, int &tc);

 /**
  * Calculate a best path using A*
  *
  * @param Fx, Fy The source location from which to path.
  * @param Tx, Ty The destination to which to path.
  *
  * @param bash Whether we should path through terrain that's impassable, but can
  *             be destroyed(closed windows, doors, etc.)
  */
 std::vector<point> route(const int Fx, const int Fy, const int Tx, const int Ty,
                          const bool bash = true);

 int coord_to_angle (const int x, const int y, const int tgtx, const int tgty);
// vehicles
 VehicleList get_vehicles();
 VehicleList get_vehicles(const int sx, const int sy, const int ex, const int ey);

 /**
  * Checks if tile is occupied by vehicle and by which part.
  *
  * @param part_num The part number of the part at this tile will be returned in this parameter.
  * @return A pointer to the vehicle in this tile.
  */
 vehicle* veh_at(const int x, const int y, int &part_num);

 /**
  * Same as `veh_at(const int, const int, int)`, but doesn't return part number.
  */
 vehicle* veh_at(const int x, const int y);// checks, if tile is occupied by vehicle

 // put player on vehicle at x,y
 void board_vehicle(game *g, int x, int y, player *p);
 void unboard_vehicle(const int x, const int y);//remove player from vehicle at x,y
 void update_vehicle_cache(vehicle *, const bool brand_new = false);
 void reset_vehicle_cache();
 void clear_vehicle_cache();
 void update_vehicle_list(const int to);

 void destroy_vehicle (vehicle *veh);
// Change vehicle coords and move vehicle's driver along.
// Returns true, if there was a submap change.
// If test is true, function only checks for submap change, no displacement
// WARNING: not checking collisions!
 bool displace_vehicle (game *g, int &x, int &y, const int dx, const int dy, bool test = false);
 void vehmove(game* g);          // Vehicle movement
 bool vehproceed(game* g);
// move water under wheels. true if moved
 bool displace_water (const int x, const int y);

// Furniture
 void set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture);
 void set(const int x, const int y, const std::string new_terrain, const std::string new_furniture);

 std::string name(const int x, const int y);
 bool has_furn(const int x, const int y);

 furn_id furn(const int x, const int y) const; // Furniture at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 int oldfurn(const int x, const int y) const; // Furniture at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 std::string get_furn(const int x, const int y) const;
 furn_t & furn_at(const int x, const int y) const;

 void furn_set(const int x, const int y, const furn_id new_furniture);
 void furn_set(const int x, const int y, const std::string new_furniture);

 std::string furnname(const int x, const int y);
 bool can_move_furniture( const int x, const int y, player * p = NULL);
// Terrain
 ter_id ter(const int x, const int y) const; // Terrain integer id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 int oldter(const int x, const int y) const; // Temporary; the game is riddled with case statements requiring enum
 std::string get_ter(const int x, const int y) const; // Terrain string id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 ter_t & ter_at(const int x, const int y) const; // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]

 void ter_set(const int x, const int y, const ter_id new_terrain);
 void ter_set(const int x, const int y, const std::string new_terrain);

 std::string tername(const int x, const int y) const; // Name of terrain at (x, y)

 // Check for terrain/furniture/field that provide a
 // "fire" item to be used for example when crafting or when
 // a iuse function needs fire.
 bool has_nearby_fire(int x, int y, int radius = 1);

 std::string features(const int x, const int y); // Words relevant to terrain (sharp, etc)
 bool has_flag(const std::string & flag, const int x, const int y) const;  // checks terrain, furniture and vehicles
 bool can_put_items(const int x, const int y); // True if items can be placed in this tile
 bool has_flag_ter(const std::string & flag, const int x, const int y) const;  // checks terrain
 bool has_flag_furn(const std::string & flag, const int x, const int y) const;  // checks furniture
 bool has_flag_ter_or_furn(const std::string & flag, const int x, const int y) const; // checks terrain or furniture
 bool has_flag_ter_and_furn(const std::string & flag, const int x, const int y) const; // checks terrain and furniture
 // fast "oh hai it's update_scent/lightmap/draw/monmove/self/etc again, what about this one" flag checking
 bool has_flag(const ter_bitflags flag, const int x, const int y) const;  // checks terrain, furniture and vehicles
 bool has_flag_ter(const ter_bitflags flag, const int x, const int y) const;  // checks terrain
 bool has_flag_furn(const ter_bitflags flag, const int x, const int y) const;  // checks furniture
 bool has_flag_ter_or_furn(const ter_bitflags flag, const int x, const int y) const; // checks terrain or furniture
 bool has_flag_ter_and_furn(const ter_bitflags flag, const int x, const int y) const; // checks terrain and furniture

 bool is_destructable(const int x, const int y);        // checks terrain and vehicles
 bool is_destructable_ter_furn(const int x, const int y);       // only checks terrain
 bool is_divable(const int x, const int y);
 bool is_outside(const int x, const int y);
 bool flammable_items_at(const int x, const int y);
 bool moppable_items_at(const int x, const int y);
 point random_outdoor_tile();

 bool has_quality(std::string quality_id, const int x, const int y);
 bool has_quality(std::string quality_id, const int quality_value, const int x, const int y);
 bool has_quality_ter(std::string quality_id, const int x, const int y);
 bool has_quality_ter(std::string quality_id, const int quality_value, const int x, const int y);
 bool has_quality_furn(std::string quality_id, const int x, const int y);
 bool has_quality_furn(std::string quality_id, const int quality_value, const int x, const int y);
 bool has_quality_ter_or_furn(std::string quality_id, const int x, const int y);
 bool has_quality_ter_or_furn(std::string quality_id, const int quality_value, const int x, const int y);
 bool has_quality_ter_and_furn(std::string quality_id, const int x, const int y);
 bool has_quality_ter_and_furn(std::string quality_id, const int quality_value, const int x, const int y);

// mapgen

void draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2);
void draw_line_ter(const std::string type, int x1, int y1, int x2, int y2);
void draw_line_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_line_furn(const std::string type, int x1, int y1, int x2, int y2);
void draw_fill_background(ter_id type);
void draw_fill_background(std::string type);
void draw_fill_background(ter_id (*f)());

void draw_square_ter(ter_id type, int x1, int y1, int x2, int y2);
void draw_square_ter(std::string type, int x1, int y1, int x2, int y2);
void draw_square_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_square_furn(std::string type, int x1, int y1, int x2, int y2);
void draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2);
void draw_rough_circle(ter_id type, int x, int y, int rad);
void draw_rough_circle(std::string type, int x, int y, int rad);

void add_corpse(int x, int y);


//
 void translate(const std::string terfrom, const std::string terto); // Change all instances of $from->$to
 void translate_radius(const std::string terfrom, const std::string terto, const float radi, const int uX, const int uY);
 void translate(const ter_id from, const ter_id to); // Change all instances of $from->$to
 void translate_radius(const ter_id from, const ter_id to, const float radi, const int uX, const int uY);
 bool close_door(const int x, const int y, const bool inside, const bool check_only);
 bool open_door(const int x, const int y, const bool inside);
 // bash: if res pointer is supplied, res will contain absorbed impact or -1
 bool bash(const int x, const int y, const int str, std::string &sound, int *res = 0);
 void destroy(game *g, const int x, const int y, const bool makesound);
 void shoot(game *g, const int x, const int y, int &dam, const bool hit_items,
            const std::set<std::string>& ammo_effects);
 bool hit_with_acid(game *g, const int x, const int y);
 bool hit_with_fire(game *g, const int x, const int y);
 bool marlossify(const int x, const int y);
 bool has_adjacent_furniture(const int x, const int y);
 void mop_spills(const int x, const int y);

// Radiation
 int& radiation(const int x, const int y); // Amount of radiation at (x, y);

// Temperature
 int& temperature(const int x, const int y);    // Temperature for submap
 void set_temperature(const int x, const int y, const int temperature); // Set temperature for all four submap quadrants

// Items
 std::vector<item>& i_at(int x, int y);
 item water_from(const int x, const int y);
 item acid_from(const int x, const int y);
 void i_clear(const int x, const int y);
 void i_rem(const int x, const int y, const int index);
 point find_item(const item *it);
 void spawn_artifact(const int x, const int y, itype* type, int bday);
    void spawn_item(const int x, const int y, const std::string &itype_id,
                    const unsigned quantity=1, const int charges=0,
                    const unsigned birthday=0, const int damlevel=0);
 int max_volume(const int x, const int y);
 int free_volume(const int x, const int y);
 int stored_volume(const int x, const int y);
 bool is_full(const int x, const int y, const int addvolume = -1, const int addnumber = -1 );
 bool add_item_or_charges(const int x, const int y, item new_item, int overflow_radius = 2);
 void process_active_items(game *g);
 void process_vehicles(game *g);

 std::list<item> use_amount(const point origin, const int range, const itype_id type, const int amount,
                            const bool use_container = false, std::string extended_flag = "", int extended_range = INT_MAX);
 std::list<item> use_charges(const point origin, const int range, const itype_id type, const int amount, 
							 std::string extended_flag = "", int extended_range = INT_MAX);

// Traps
 std::string trap_get(const int x, const int y) const;
 void trap_set(const int x, const int y, const std::string & sid);
 void trap_set(const int x, const int y, const trap_id id);

 trap_id tr_at(const int x, const int y) const;
 void add_trap(const int x, const int y, const trap_id t);
 void disarm_trap( game *g, const int x, const int y);
 void remove_trap(const int x, const int y);
 std::set<point> trap_locations(trap_id t);

// Fields
 field& field_at(const int x, const int y);

 int get_field_age(const point p, const field_id t);
 int get_field_strength(const point p, const field_id t);
 int adjust_field_age(const point p, const field_id t, const int offset);
 int adjust_field_strength(game *g, const point p, const field_id t, const int offset);
 int set_field_age(const point p, const field_id t, const int age, bool isoffset = false);
 int set_field_strength(game *g, const point p, const field_id t, const int str, bool isoffset = false);
 field_entry * get_field( const point p, const field_id t );
 bool add_field(game *g, const point p, const field_id t, unsigned int density, const int age);
 bool add_field(game *g, const int x, const int y, const field_id t, const unsigned char density);
 void remove_field(const int x, const int y, const field_id field_to_remove);
 bool process_fields(game *g); // See fields.cpp
 bool process_fields_in_submap(game *g, const int gridn); // See fields.cpp
 void step_in_field(const int x, const int y, game *g); // See fields.cpp
 void mon_in_field(const int x, const int y, game *g, monster *z); // See fields.cpp
 void field_effect(int x, int y, game *g); //See fields.cpp

// Computers
 computer* computer_at(const int x, const int y);

 // Camps
 bool allow_camp(const int x, const int y, const int radius = CAMPCHECK);
 basecamp* camp_at(const int x, const int y, const int radius = CAMPSIZE);
 void add_camp(const std::string& name, const int x, const int y);

// Graffiti
 graffiti graffiti_at(int x, int y);
 bool add_graffiti(int x, int y, std::string contents);

// mapgen.cpp functions
 void generate(game *g, overmap *om, const int x, const int y, const int z, const int turn);
 void post_process(game *g, unsigned zones);
 void place_spawns(game *g, std::string group, const int chance,
                   const int x1, const int y1, const int x2, const int y2, const float density);
 void place_gas_pump(const int x, const int y, const int charges);
 void place_toilet(const int x, const int y, const int charges = 6 * 4); // 6 liters at 250 ml per charge
 int place_items(items_location loc, const int chance, const int x1, const int y1,
                  const int x2, const int y2, bool ongrass, const int turn);
// put_items_from puts exactly num items, based on chances
 void put_items_from(items_location loc, const int num, const int x, const int y, const int turn = 0, const int quantity = 0, const int charges = 0, const int damlevel = 0);
    void spawn_item(const int x, const int y, item new_item,
                    const int charges, const int damlevel);
 void add_spawn(std::string type, const int count, const int x, const int y, bool friendly = false,
                const int faction_id = -1, const int mission_id = -1,
                std::string name = "NONE");
 void add_spawn(monster *mon);
 void create_anomaly(const int cx, const int cy, artifact_natural_property prop);
 vehicle *add_vehicle(game *g, std::string type, const int x, const int y, const int dir,
                      const int init_veh_fuel = -1, const int init_veh_status = -1,
                      const bool merge_wrecks = true);
 computer* add_computer(const int x, const int y, std::string name, const int security);
 float light_transparency(const int x, const int y) const;
 void build_map_cache(game *g);
 lit_level light_at(int dx, int dy); // Assumes 0,0 is light map center
 float ambient_light_at(int dx, int dy); // Raw values for tilesets
 bool pl_sees(int fx, int fy, int tx, int ty, int max_range);
 std::set<vehicle*> vehicle_list;
 std::map< std::pair<int,int>, std::pair<vehicle*,int> > veh_cached_parts;
 bool veh_exists_at [SEEX * MAPSIZE][SEEY * MAPSIZE];

 point get_abs_sub() {
   return abs_sub;
 };
 point getabs(const int x=0, const int y=0 );
 point getlocal(const int x, const int y );
 point getlocal( const point p ) { return getlocal(p.x, p.y); } 
 bool inboundsabs(const int x, const int y);
 bool inbounds(const int x, const int y);

 int getmapsize() { return my_MAPSIZE; };

 // Not protected/private for mapgen_functions.cpp access
 void rotate(const int turns);// Rotates the current map 90*turns degress clockwise
                              // Useful for houses, shops, etc
 void add_road_vehicles(bool city, int facing);

protected:
 void saven(overmap *om, unsigned const int turn, const int x, const int y, const int z,
            const int gridx, const int gridy);
 bool loadn(game *g, const int x, const int y, const int z, const int gridx, const int gridy,
            const  bool update_vehicles = true, overmap *om = NULL);
 void copy_grid(const int to, const int from);
 void draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
                const oter_id t_south, const oter_id t_west, const oter_id t_neast,
                const oter_id t_seast, const oter_id t_nwest, const oter_id t_swest,
                const oter_id t_above, const int turn, game *g, const float density,
                const int zlevel);
 void add_extra(map_extra type, game *g);
 void build_transparency_cache();
 void build_outside_cache(const game *g);
 void generate_lightmap(game *g);
 void build_seen_cache(game *g);
 void castLight( game *g, int row, float start, float end, int xx, int xy, int yx, int yy );

 int my_MAPSIZE;
 virtual bool is_tiny() { return false; };

 std::vector<item> nulitems; // Returned when &i_at() is asked for an OOB value
 ter_id nulter;  // Returned when &ter() is asked for an OOB value
 field nulfield; // Returned when &field_at() is asked for an OOB value
 vehicle nulveh; // Returned when &veh_at() is asked for an OOB value
 int nulrad;     // OOB &radiation()
 int null_temperature;  // Because radiation does it too

 std::vector <trap*> *traps;

 bool veh_in_active_range;

 point abs_sub; // same as x y in maps.txt, for 0,0 / grid[0]
 point abs_min; // same as above in absolute coordinates (submap(x,y) * 12)
 point abs_max; // same as abs_min + ( my_MAPSIZE * 12 )
 int world_z;   // same as
 void set_abs_sub(const int x, const int y, const int z); // set the above vars on map load/shift/etc

private:
submap * getsubmap( const int grididx );
 long determine_wall_corner(const int x, const int y, const long orig_sym);
 void cache_seen(const int fx, const int fy, const int tx, const int ty, const int max_range);
 // apply a circular light pattern immediately, however it's best to use...
 void apply_light_source(int x, int y, float luminance, bool trig_brightcalc);
 // ...this, which will apply the light after at the end of generate_lightmap, and prevent redundant
 // light rays from causing massive slowdowns, if there's a huge amount of light.
 void add_light_source(int x, int y, float luminance);
 void apply_light_arc(int x, int y, int angle, float luminance, int wideangle = 30 );
 void apply_light_ray(bool lit[MAPSIZE*SEEX][MAPSIZE*SEEY],
                      int sx, int sy, int ex, int ey, float luminance, bool trig_brightcalc = true);
 void calc_ray_end(int angle, int range, int x, int y, int* outx, int* outy);
 void forget_traps(int gridx, int gridy);
 vehicle *add_vehicle_to_map(vehicle *veh, const int x, const int y, const bool merge_wrecks = true);
 void add_item(const int x, const int y, item new_item, int maxitems = 64);
 
 void process_active_items_in_submap(game *g, const int nonant);
 void process_active_items_in_vehicles(game *g, const int nonant);
 bool process_active_item(game *g, item *it, const int nonant, const int i, const int j);

 float lm[MAPSIZE*SEEX][MAPSIZE*SEEY];
 float sm[MAPSIZE*SEEX][MAPSIZE*SEEY];
 // to prevent redundant ray casting into neighbors: precalculate bulk light source positions. This is
 // only valid for the duration of generate_lightmap
 float light_source_buffer[MAPSIZE*SEEX][MAPSIZE*SEEY];
 bool outside_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 float transparency_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 bool seen_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
 submap* grid[MAPSIZE * MAPSIZE];
 std::map<trap_id, std::set<point> > traplocs;
};

std::vector<point> closest_points_first(int radius, point p);
std::vector<point> closest_points_first(int radius,int x,int y);
class tinymap : public map
{
friend class editmap;
public:
 tinymap();
 tinymap(std::vector<trap*> *trptr);
 ~tinymap();

protected:
 virtual bool is_tiny() { return true; };

private:
 submap* grid[4];
};

#endif
