#ifndef MAP_H
#define MAP_H

#include "cursesdef.h"

#include <stdlib.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "mapdata.h"
#include "overmap.h"
#include "item.h"
#include "json.h"
#include "monster.h"
#include "npc.h"
#include "vehicle.h"
#include "lightmap.h"
#include "coordinates.h"
//TODO: include comments about how these variables work. Where are they used. Are they constant etc.
#define MAPSIZE 11
#define CAMPSIZE 1
#define CAMPCHECK 3

class player;
class item;
struct itype;
struct mapgendata;
// TODO: This should be const& but almost no functions are const
struct wrapped_vehicle{
 int x;
 int y;
 int i; // submap col
 int j; // submap row
 vehicle* v;
};

typedef std::vector<wrapped_vehicle> VehicleList;
typedef std::vector< std::pair< item*, int > > itemslice;
typedef std::string items_location;

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
 map(int mapsize = MAPSIZE);
 ~map();

// Visual Output
 void debug();

 /**
  * Sets a dirty flag on the transparency cache.
  *
  * If this isn't set, it's just assumed that
  * the transparency cache hasn't changed and
  * doesn't need to be updated.
  */
 void set_transparency_cache_dirty() {
     transparency_cache_dirty = true;
 }

 /**
  * Sets a dirty flag on the outside cache.
  *
  * If this isn't set, it's just assumed that
  * the outside cache hasn't changed and
  * doesn't need to be updated.
  */
 void set_outside_cache_dirty() {
     outside_cache_dirty = true;
 }

 /**
  * Callback invoked when a vehicle has moved.
  */
 void on_vehicle_moved();

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
 void draw(WINDOW* w, const point center);

 /** Draw the map tile at the given coordinate. Called by `map::draw()`.
  *
  * @param x, y The tile on this map to draw.
  * @param cx, cy The center of the viewport to be rendered, see `center` in `map::draw()`
  */
 void drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert, const bool show_items,
             const int view_center_x = -1, const int view_center_y = -1,
             const bool low_light = false, const bool bright_level = false);

    /**
     * Add currently loaded submaps (in @ref grid) to the @ref mapbuffer.
     * They will than be stored by that class and can be loaded from that class.
     * This can be called several times, the mapbuffer takes care of adding
     * the same submap several times. It should only be called after the map has
     * been loaded.
     * Submaps that have been loaded from the mapbuffer (and not generated) are
     * already stored in the mapbuffer.
     * TODO: determine if this is really needed? Submaps are already in the mapbuffer
     * if they have been loaded from disc and the are added by map::generate, too.
     * So when do they not appear in the mapbuffer?
     */
    void save();
    /**
     * Load submaps into @ref grid. This might create new submaps if
     * the @ref mapbuffer can not deliver the requested submap (as it does
     * not exist on disc).
     * This must be called before the map can be used at all!
     * @param om overmap to which the world coordinates are relative to.
     * @param wx coordinates (relative to om) of the submap at grid[0]. This
     * is in submap coordinates.
     * @param wy see wx
     * @param wz see wx, this is the z-level
     * @param update_vehicles If true, add vehicles to the vehicle cache.
     */
    void load(const int wx, const int wy, const int wz, const bool update_vehicles, overmap *om);
    /**
     * Same as @ref load, but uses only global submap coordinates and
     * has therefor no overmap pointer parameter.
     */
    void load_abs(const int wx, const int wy, const int wz, const bool update_vehicles);
    /**
     * Shift the map along the vector (sx,sy).
     * This is like loading the map with coordinates derived from the current
     * position of the map (@ref abs_sub) plus the shift vector.
     * Note: the map must have been loaded before this can be called.
     */
    void shift(const int sx, const int sy);
    /**
     * Spawn monsters from submap spawn points and from the overmap.
     * @param ignore_sight If true, monsters may spawn in the view of the player
     * character (useful when the whole map has been loaded instead, e.g.
     * when starting a new game, or after teleportation or after moving vertically).
     * If false, monsters are not spawned in view of of player character.
     */
    void spawn_monsters(bool ignore_sight);
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
  * @param bresenham_slope Indicates the Bresenham line used to connect the two points, and may
  *           subsequently be used to form a path between them
  */
 bool sees(const int Fx, const int Fy, const int Tx, const int Ty,
           const int range, int &bresenham_slope);

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
                 const int range, const int cost_min, const int cost_max, int &bresenham_slope) const;


 /**
  * Check whether items in the target square are accessable from the source square
  * `(Fx, Fy)` and `(Tx, Ty)`.
  *
  * Checks two things:
  * 1. The `sees()` algorithm between `(Fx, Fy)` and `(Tx, Ty)` OR origin and target match.
  * 2. That the target location isn't sealed.
  */
 bool accessable_items(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const;
 /**
  * Like @ref accessable_items but checks for accessable furniture.
  * It ignores the furniture flags of the target square (ignores if target is SEALED).
  */
 bool accessable_furniture(const int Fx, const int Fy, const int Tx, const int Ty, const int range) const;

 /**
  * Calculate next search points surrounding the current position.
  * Points closer to the target come first.
  * This method leads to straighter lines and prevents weird looking movements away from the target.
  */
 std::vector<point> getDirCircle(const int Fx, const int Fy, const int Tx, const int Ty);

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

 /**
  * Vehicle-relative coordinates from reality bubble coordinates, if a vehicle
  * actually exists here.
  * Returns 0,0 if no vehicle exists there (use veh_at to check if it exists first)
  */
 point veh_part_coordinates(const int x, const int y);

 // put player on vehicle at x,y
 void board_vehicle(int x, int y, player *p);
 void unboard_vehicle(const int x, const int y);//remove player from vehicle at x,y
 void update_vehicle_cache(vehicle *, const bool brand_new = false);
 void reset_vehicle_cache();
 void clear_vehicle_cache();
 void update_vehicle_list(submap * const to);

 void destroy_vehicle (vehicle *veh);
// Change vehicle coords and move vehicle's driver along.
// Returns true, if there was a submap change.
// If test is true, function only checks for submap change, no displacement
// WARNING: not checking collisions!
 bool displace_vehicle (int &x, int &y, const int dx, const int dy, bool test = false);
 void vehmove();          // Vehicle movement
 bool vehproceed();
// move water under wheels. true if moved
 bool displace_water (const int x, const int y);

// Furniture
 void set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture);
 void set(const int x, const int y, const std::string new_terrain, const std::string new_furniture);

 std::string name(const int x, const int y);
 bool has_furn(const int x, const int y);

 furn_id furn(const int x, const int y) const; // Furniture at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 std::string get_furn(const int x, const int y) const;
 furn_t & furn_at(const int x, const int y) const;

 void furn_set(const int x, const int y, const furn_id new_furniture);
 void furn_set(const int x, const int y, const std::string new_furniture);

 std::string furnname(const int x, const int y);
 bool can_move_furniture( const int x, const int y, player * p = NULL);
// Terrain
 ter_id ter(const int x, const int y) const; // Terrain integer id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 std::string get_ter(const int x, const int y) const; // Terrain string id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
 std::string get_ter_harvestable(const int x, const int y) const; // harvestable of the terrain
 ter_id get_ter_transforms_into(const int x, const int y) const; // get the terrain id to transform to
 int get_ter_harvest_season(const int x, const int y) const; // get season to harvest the terrain
 ter_t & ter_at(const int x, const int y) const; // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]

 void ter_set(const int x, const int y, const ter_id new_terrain);
 void ter_set(const int x, const int y, const std::string new_terrain);

 std::string tername(const int x, const int y) const; // Name of terrain at (x, y)

 // Check for terrain/furniture/field that provide a
 // "fire" item to be used for example when crafting or when
 // a iuse function needs fire.
 bool has_nearby_fire(int x, int y, int radius = 1);
 /**
  * Check if player can see some items at (x,y). Includes:
  * - check for items at this location (!i_at().empty())
  * - check for SEALED flag (sealed furniture/terrain makes
  * items not visible under any circumstances).
  * - check for CONTAINER flag (makes items only visible when
  * the player is at (x,y) or at an adjacent square).
  */
 bool sees_some_items(int x, int y, const player &u);
 /**
  * Check if the player could see items at (x,y) if there were
  * any items. This is similar to @ref sees_some_items, but it
  * does not check that there are actually any items.
  */
 bool could_see_items(int x, int y, const player &u);


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

 /** Returns true if there is a bashable vehicle part or the furn/terrain is bashable at x,y */
 bool is_bashable(const int x, const int y);
 /** Returns true if the terrain at x,y is bashable */
 bool is_bashable_ter(const int x, const int y);
 /** Returns true if the furniture at x,y is bashable */
 bool is_bashable_furn(const int x, const int y);
 /** Returns true if the furniture or terrain at x,y is bashable */
 bool is_bashable_ter_furn(const int x, const int y);
 /** Returns max_str of the furniture or terrain at x,y */
 int bash_strength(const int x, const int y);
 /** Returns min_str of the furniture or terrain at x,y */
 int bash_resistance(const int x, const int y);
 /** Returns a success rating from -1 to 10 for a given tile based on a set strength, used for AI movement planning
  *  Values roughly correspond to 10% increment chances of success on a given bash, rounded down. -1 means the square is not bashable */
 int bash_rating(const int str, const int x, const int y);

 /** Generates rubble at the given location, if overwrite is true it just writes on top of what currently exists
  *  floor_type is only used if there is a non-bashable wall at the location or with overwrite = true */
 void make_rubble(const int x, const int y, furn_id rubble_type = f_rubble, bool items = false,
                    ter_id floor_type = t_dirt, bool overwrite = false);

 bool is_divable(const int x, const int y);
 bool is_outside(const int x, const int y);
 bool flammable_items_at(const int x, const int y);
 bool moppable_items_at(const int x, const int y);
 point random_outdoor_tile();
// mapgen

void draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2);
void draw_line_ter(const std::string type, int x1, int y1, int x2, int y2);
void draw_line_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_line_furn(const std::string type, int x1, int y1, int x2, int y2);
void draw_fill_background(ter_id type);
void draw_fill_background(std::string type);
void draw_fill_background(ter_id (*f)());
void draw_fill_background(const id_or_id & f);

void draw_square_ter(ter_id type, int x1, int y1, int x2, int y2);
void draw_square_ter(std::string type, int x1, int y1, int x2, int y2);
void draw_square_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_square_furn(std::string type, int x1, int y1, int x2, int y2);
void draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2);
void draw_square_ter(const id_or_id & f, int x1, int y1, int x2, int y2);
void draw_rough_circle(ter_id type, int x, int y, int rad);
void draw_rough_circle(std::string type, int x, int y, int rad);
void draw_rough_circle_furn(furn_id type, int x, int y, int rad);
void draw_rough_circle_furn(std::string type, int x, int y, int rad);

void add_corpse(int x, int y);


//
 void translate(const std::string terfrom, const std::string terto); // Change all instances of $from->$to
 void translate_radius(const std::string terfrom, const std::string terto, const float radi, const int uX, const int uY);
 void translate(const ter_id from, const ter_id to); // Change all instances of $from->$to
 void translate_radius(const ter_id from, const ter_id to, const float radi, const int uX, const int uY);
 bool close_door(const int x, const int y, const bool inside, const bool check_only);
 bool open_door(const int x, const int y, const bool inside, const bool check_only = false);
 /** Makes spores at the respective x and y. source is used for kill counting */
 void create_spores(const int x, const int y, Creature* source = NULL);
 /** Checks if a square should collapse, returns the X for the one_in(X) collapse chance */
 int collapse_check(const int x, const int y);
 /** Causes a collapse at (x, y), such as from destroying a wall */
 void collapse_at(const int x, const int y);
 /** Returns a pair where first is whether something was smashed and second is if it was a success */
 std::pair<bool, bool> bash(const int x, const int y, const int str, bool silent = false,
                            bool destroy = false, vehicle *bashing_vehicle = nullptr);
 // spawn items from the list, see map_bash_item_drop
 void spawn_item_list(const std::vector<map_bash_item_drop> &items, int x, int y);
 /** Keeps bashing a square until it can't be bashed anymore */
 void destroy(const int x, const int y, const bool silent = false);
 /** Keeps bashing a square until there is no more furniture */
 void destroy_furn(const int x, const int y, const bool silent = false);
 void crush(const int x, const int y);
 void shoot(const int x, const int y, int &dam, const bool hit_items,
            const std::set<std::string>& ammo_effects);
 bool hit_with_acid(const int x, const int y);
 bool hit_with_fire(const int x, const int y);
 bool marlossify(const int x, const int y);
 bool has_adjacent_furniture(const int x, const int y);
 void mop_spills(const int x, const int y);

 // Signs
 const std::string get_signage(const int x, const int y) const;
 void set_signage(const int x, const int y, std::string message) const;
 void delete_signage(const int x, const int y) const;

 // Radiation
 int get_radiation(const int x, const int y) const; // Amount of radiation at (x, y);
 void set_radiation(const int x, const int y, const int value);

 /** Increment the radiation in the given tile by the given delta
  *  (decrement it if delta is negative)
  */
 void adjust_radiation(const int x, const int y, const int delta);

// Temperature
 int& temperature(const int x, const int y);    // Temperature for submap
 void set_temperature(const int x, const int y, const int temperature); // Set temperature for all four submap quadrants

// Items
 // Const item accessor for examining items on the map without modifying them.
 const std::vector<item>& i_at(int x, int y) const;
 // Non-const item accessor for rare cases where items need to be modified en masse.
 // Do not insert or remove items using this, it can break assumptions about caching.
 std::vector<item>& i_at_mutable(int x, int y);
 // Accessors to retrieve a mutable reference to an item.
 item *get_item( int x, int y, int i );
 item *get_item( const int x, const int y, std::vector<item>::const_iterator i );
 itemslice i_stacked(std::vector<item>& items);
 item water_from(const int x, const int y);
 item swater_from(const int x, const int y);
 item acid_from(const int x, const int y);
 void i_clear(const int x, const int y);
 // Both i_rem() methods that return values act like conatiner::erase(),
 // returning an iterator to the next item after removal.
 std::vector<item>::const_iterator i_rem( const int x, const int y,
                                          std::vector<item>::const_iterator it );
 int i_rem(const int x, const int y, const int index);
 void i_rem(const int x, const int y, item* it);
 void spawn_artifact( const int x, const int y );
 void spawn_natural_artifact( const int x, const int y, const artifact_natural_property prop );
 void spawn_item(const int x, const int y, const std::string &itype_id,
                 const unsigned quantity=1, const long charges=0,
                 const unsigned birthday=0, const int damlevel=0, const bool rand = true);
 int max_volume(const int x, const int y);
 int free_volume(const int x, const int y);
 int stored_volume(const int x, const int y);
 bool is_full(const int x, const int y, const int addvolume = -1, const int addnumber = -1 );
 bool add_item_or_charges(const int x, const int y, item new_item, int overflow_radius = 2);
 void add_item(const int x, const int y, item new_item, int maxitems = 64);
 void process_active_items();

 std::list<item> use_amount_square( const int x, const int y, const itype_id type,
                                    int &quantity, const bool use_container );
 std::list<item> use_amount( const point origin, const int range, const itype_id type,
                             const int amount, const bool use_container = false );
 std::list<item> use_charges( const point origin, const int range, const itype_id type,
                              const long amount );

 std::list<std::pair<tripoint, item *> > get_rc_items( int x = -1, int y = -1, int z = -1 );

 void trigger_rc_items( std::string signal );

// Traps
 std::string trap_get(const int x, const int y) const;
 void trap_set(const int x, const int y, const std::string & sid);
 void trap_set(const int x, const int y, const trap_id id);

 trap_id tr_at(const int x, const int y) const;
 void add_trap(const int x, const int y, const trap_id t);
 void disarm_trap( const int x, const int y);
 void remove_trap(const int x, const int y);
 const std::set<point> &trap_locations(trap_id t) const;

// Fields
        /**
         * Get the fields that are here. This is for querying and looking at it only,
         * one can not change the fields.
         * @param x,y The local map coordinates, if out of bounds, returns an empty field.
         */
        const field& field_at( const int x, const int y ) const;
        /**
         * Get the age of a field entry (@ref field_entry::age), if there is no
         * field of that type, returns -1.
         */
        int get_field_age( const point p, const field_id t );
        /**
         * Get the density of a field entry (@ref field_entry::density),
         * if there is no field of that type, returns 0.
         */
        int get_field_strength( const point p, const field_id t );
        /**
         * Increment/decrement age of field entry at point.
         * @return resulting age or -1 if not present (does *not* create a new field).
         */
        int adjust_field_age( const point p, const field_id t, const int offset );
        /**
         * Increment/decrement density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         */
        int adjust_field_strength( const point p, const field_id t, const int offset );
        /**
         * Set age of field entry at point.
         * @return resulting age or -1 if not present (does *not* create a new field).
         * @param isoffset If true, the given age value is added to the existing value,
         * if false, the existing age is ignored and overridden.
         */
        int set_field_age( const point p, const field_id t, const int age, bool isoffset = false );
        /**
         * Set density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         * @param isoffset If true, the given str value is added to the existing value,
         * if false, the existing density is ignored and overridden.
         */
        int set_field_strength( const point p, const field_id t, const int str, bool isoffset = false );
        /**
         * Get field of specific type at point.
         * @return NULL if there is no such field entry at that place.
         */
        field_entry * get_field( const point p, const field_id t );
        /**
         * Add field entry at point, or set density if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */
        bool add_field(const point p, const field_id t, const int density, const int age);
        /**
         * Add field entry at xy, or set density if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */
        bool add_field(const int x, const int y, const field_id t, const int density);
        /**
         * Remove field entry at xy, ignored if the field entry is not present.
         */
        void remove_field( const int x, const int y, const field_id field_to_remove );
 bool process_fields(); // See fields.cpp
 bool process_fields_in_submap(submap * const current_submap, const int submap_x, const int submap_y); // See fields.cpp
        /**
         * Apply field effects to the creature when it's on a square with fields.
         */
        void creature_in_field( Creature &critter );

// Computers
 computer* computer_at(const int x, const int y);

 // Camps
 bool allow_camp(const int x, const int y, const int radius = CAMPCHECK);
 basecamp* camp_at(const int x, const int y, const int radius = CAMPSIZE);
 void add_camp(const std::string& name, const int x, const int y);

// Graffiti
    bool has_graffiti_at(int x, int y) const;
    const std::string &graffiti_at(int x, int y) const;
    void set_graffiti(int x, int y, const std::string &contents);
    void delete_graffiti(int x, int y);

// mapgen.cpp functions
 void generate(const int x, const int y, const int z, const int turn);
 void post_process(unsigned zones);
 void place_spawns(std::string group, const int chance,
                   const int x1, const int y1, const int x2, const int y2, const float density);
 void place_gas_pump(const int x, const int y, const int charges);
 void place_toilet(const int x, const int y, const int charges = 6 * 4); // 6 liters at 250 ml per charge
 void place_vending(int x, int y, std::string type);
 int place_npc(int x, int y, std::string type);
 /**
  * Place items from item group in the rectangle (x1,y1) - (x2,y2). Several items may be spawned
  * on different places. Several items may spawn at once (at one place) when the item group says
  * so (uses @ref item_group::items_from which may return several items at once).
  * @param chance Chance for more items. A chance of 100 creates 1 item all the time, otherwise
  * it's the chance that more items will be created (place items until the random roll with that
  * chance fails). The chance is used for the first item as well, so it may not spawn an item at
  * all. Values <= 0 or > 100 are invalid.
  * @param ongrass If false the items won't spawn on flat terrain (grass, floor, ...).
  * @param turn The birthday that the created items shall have.
  * @return The number of placed items.
  */
 int place_items(items_location loc, const int chance, const int x1, const int y1,
                  const int x2, const int y2, bool ongrass, const int turn, bool rand = true);
 /**
  * Place items from an item group at (x,y). Places as much items as the item group says.
  * (Most item groups are distributions and will only create one item.)
  * @param turn The birthday that the created items shall have.
  * @return The number of placed items.
  */
 int put_items_from_loc(items_location loc, const int x, const int y, const int turn = 0);
 void spawn_an_item(const int x, const int y, item new_item,
                    const long charges, const int damlevel);
 // Similar to spawn_an_item, but spawns a list of items, or nothing if the list is empty.
 void spawn_items(const int x, const int y, const std::vector<item> &new_items);
 void add_spawn(std::string type, const int count, const int x, const int y, bool friendly = false,
                const int faction_id = -1, const int mission_id = -1,
                std::string name = "NONE");
 void create_anomaly(const int cx, const int cy, artifact_natural_property prop);
 vehicle *add_vehicle(std::string type, const int x, const int y, const int dir,
                      const int init_veh_fuel = -1, const int init_veh_status = -1,
                      const bool merge_wrecks = true);
 computer* add_computer(const int x, const int y, std::string name, const int security);
 float light_transparency(const int x, const int y) const;
 void build_map_cache();
 lit_level light_at(int dx, int dy); // Assumes 0,0 is light map center
 float ambient_light_at(int dx, int dy); // Raw values for tilesets
 bool pl_sees(int fx, int fy, int tx, int ty, int max_range);
 std::set<vehicle*> vehicle_list;
 std::set<vehicle*> dirty_vehicle_list;

 std::map< std::pair<int,int>, std::pair<vehicle*,int> > veh_cached_parts;
 bool veh_exists_at [SEEX * MAPSIZE][SEEY * MAPSIZE];

    /** return @ref abs_sub */
    tripoint get_abs_sub() const;
    /**
     * Translates local (to this map) coordinates of a square to
     * global absolute coordinates. (x,y) is in the system that
     * is used by the ter_at/furn_at/i_at functions.
     * Output is in the same scale, but in global system.
     */
    point getabs(const int x, const int y) const;
    point getabs(const point p) const { return getabs(p.x, p.y); }
    /**
     * Inverse of @ref getabs
     */
    point getlocal(const int x, const int y ) const;
    point getlocal(const point p) const { return getlocal(p.x, p.y); }
 bool inboundsabs(const int x, const int y);
 bool inbounds(const int x, const int y) const;

 int getmapsize() { return my_MAPSIZE; };

 // Not protected/private for mapgen_functions.cpp access
 void rotate(const int turns);// Rotates the current map 90*turns degress clockwise
                              // Useful for houses, shops, etc
 void add_road_vehicles(bool city, int facing);

protected:
 void saven(const int x, const int y, const int z,
            const int gridx, const int gridy);
 void loadn(const int x, const int y, const int z, const int gridx, const int gridy,
            const  bool update_vehicles = true);
        /**
         * Fast forward a submap that has just been loading into this map.
         * This is used to rot and remove rotten items, grow plants, fill funnels etc.
         */
        void actualize( const int gridx, const int gridy );
        /**
         * Whether the item has to be removed as it has rotten away completely.
         * @param pnt The point on this map where the items are, used for rot calculation.
         * @return true if the item has rotten away and should be removed, false otherwise.
         */
        bool has_rotten_away( item &itm, const point &pnt ) const;
        /**
         * Go through the list of items, update their rotten status and remove items
         * that have rotten away completely.
         * @param pnt The point on this map where the items are, used for rot calculation.
         */
        void remove_rotten_items( std::vector<item> &items, const point &pnt ) const;
        /**
         * Try to fill funnel based items here.
         * @param pnt The location in this map where to fill funnels.
         */
        void fill_funnels( const point pnt );
        /**
         * Try to grow a harvestable plant to the next stage(s).
         */
        void grow_plant( const point pnt );
        /**
         * Try to grow fruits on static plants (not planted by the player)
         * @param time_since_last_actualize Time (in turns) since this function has been
         * called the last time.
         */
        void restock_fruits( const point pnt, int time_since_last_actualize );
        void player_in_field( player &u );
        void monster_in_field( monster &z );

 void copy_grid(const int to, const int from);
 void draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
                const oter_id t_south, const oter_id t_west, const oter_id t_neast,
                const oter_id t_seast, const oter_id t_nwest, const oter_id t_swest,
                const oter_id t_above, const int turn, const float density,
                const int zlevel, const regional_settings * rsettings);
 void add_extra(map_extra type);
 void build_transparency_cache();
 void build_outside_cache();
 void generate_lightmap();
 void build_seen_cache();
 void castLight( int row, float start, float end, int xx, int xy, int yx, int yy,
                 const int offsetX, const int offsetY, const int offsetDistance );

 int my_MAPSIZE;

 mutable std::vector<item> nulitems; // Returned when &i_at() is asked for an OOB value
 mutable ter_id nulter;  // Returned when &ter() is asked for an OOB value
 mutable field nulfield; // Returned when &field_at() is asked for an OOB value
 mutable vehicle nulveh; // Returned when &veh_at() is asked for an OOB value
 mutable int null_temperature;  // Because radiation does it too

 bool veh_in_active_range;

    /**
     * Absolute coordinates of first submap (get_submap_at(0,0))
     * This is in submap coordinates (see overmapbuffer for explanation).
     * It is set upon:
     * - loading submap at grid[0],
     * - generating submaps (@ref generate)
     * - shifting the map with @ref shift
     */
    tripoint abs_sub;
    /**
     * Sets @ref abs_sub, see there. Uses the same coordinate system as @ref abs_sub.
     */
    void set_abs_sub(const int x, const int y, const int z);

private:
    field& get_field(const int x, const int y);
    void spread_gas( field_entry *cur, int x, int y, field_id curtype,
                        int percent_spread, int outdoor_age_speedup );
    void create_hot_air( int x, int y, int density );

 bool transparency_cache_dirty;
 bool outside_cache_dirty;

 submap * getsubmap( const int grididx );

 /** Get the submap containing the specified position within the reality bubble. */
 submap *get_submap_at(int x, int y) const;

 /** Get the submap containing the specified position within the reality bubble.
  *  Also writes the position within the submap to offset_x, offset_y
  */
 submap *get_submap_at(int x, int y, int& offset_x, int& offset_y) const;
 submap *get_submap_at_grid(int gridx, int gridy) const;

    void spawn_monsters( int gx, int gy, mongroup &group, bool ignore_sight );

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
 void add_light_from_items( const int x, const int y, const std::vector<item> &items );
 void calc_ray_end(int angle, int range, int x, int y, int* outx, int* outy);
 void forget_traps(int gridx, int gridy);
 vehicle *add_vehicle_to_map(vehicle *veh, const int x, const int y, const bool merge_wrecks = true);

 // Iterates over every item on the map, passing each item to the provided function.
 template<typename T>
 void process_items( bool active, T processor );
 template<typename T>
 void process_items_in_submap( submap *const current_submap, int gridx, int gridy, T processor );
 template<typename T>
 void process_items_in_vehicles( submap *const current_submap, T processor);
 template<typename T>
 void process_items_in_vehicle( vehicle *cur_veh, submap *const current_submap, T processor );

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
 tinymap(int mapsize = 2);
};

#endif
