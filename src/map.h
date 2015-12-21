#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include <memory>

#include "game_constants.h"
#include "item.h"
#include "lightmap.h"
#include "item_stack.h"
#include "active_item_cache.h"
#include "int_id.h"
#include "string_id.h"

//TODO: include comments about how these variables work. Where are they used. Are they constant etc.
#define CAMPSIZE 1
#define CAMPCHECK 3

class player;
class monster;
class item;
class Creature;
class tripoint_range;
enum field_id : int;
class field;
class field_entry;
class vehicle;
struct submap;
struct maptile;
class basecamp;
class computer;
struct itype;
struct mapgendata;
struct trap;
using trap_id = int_id<trap>;
struct oter_id;
struct regional_settings;
struct mongroup;
struct ter_t;
using ter_id = int_id<ter_t>;
struct furn_t;
using furn_id = int_id<furn_t>;
struct mtype;
using mtype_id = string_id<mtype>;
struct projectile;
struct veh_collision;
class tileray;

// TODO: This should be const& but almost no functions are const
struct wrapped_vehicle{
 int x;
 int y;
 int z;
 int i; // submap col
 int j; // submap row
 vehicle* v;
};

typedef std::vector<wrapped_vehicle> VehicleList;
typedef std::vector< std::pair< item*, int > > itemslice;
typedef std::string items_location;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
class VehicleGroup;
using vgroup_id = string_id<VehicleGroup>;
struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;
class map;
enum ter_bitflags : int;
template<typename T>
struct id_or_id;

class map_stack : public item_stack {
private:
    std::list<item> *mystack;
    tripoint location;
    map *myorigin;
public:
    map_stack( std::list<item> *newstack, tripoint newloc, map *neworigin ) :
    mystack(newstack), location(newloc), myorigin(neworigin) {};
    size_t size() const override;
    bool empty() const override;
    std::list<item>::iterator erase( std::list<item>::iterator it ) override;
    void push_back( const item &newitem ) override;
    void insert_at( std::list<item>::iterator index, const item &newitem ) override;
    std::list<item>::iterator begin();
    std::list<item>::iterator end();
    std::list<item>::const_iterator begin() const;
    std::list<item>::const_iterator end() const;
    std::list<item>::reverse_iterator rbegin();
    std::list<item>::reverse_iterator rend();
    std::list<item>::const_reverse_iterator rbegin() const;
    std::list<item>::const_reverse_iterator rend() const;
    item &front() override;
    item &operator[]( size_t index ) override;
};

struct visibility_variables {
    bool variables_set; // Is this struct initialized for current z-level
    // cached values for map visibility calculations
    int g_light_level;
    int u_clairvoyance;
    bool u_sight_impaired;
    bool u_is_boomered;
    float vision_threshold;
};

struct bash_params {
    int strength; // Initial strength

    bool silent; // Make a sound?
    bool destroy; // Essentially infinite bash strength + some
    bool bash_floor; // Do we want to bash floor if no furn/wall exists?
    /**
     * Value from 0.0 to 1.0 that affects interpolation between str_min and str_max
     * At 0.0, the bash is against str_min of targetted objects
     * This is required for proper "piercing" bashing, so that one strong hit
     * can destroy a wall and a floor under it rather than only one at a time.
     */
    float roll;

    bool did_bash; // Was anything hit?
    bool success; // Was anything destroyed?

    bool bashed_solid; // Did we bash furniture, terrain or vehicle
};

enum visibility_type {
  VIS_HIDDEN,
  VIS_CLEAR,
  VIS_LIT,
  VIS_BOOMER,
  VIS_DARK,
  VIS_BOOMER_DARK
};

struct level_cache {
    level_cache(); // Zeroes all relevant values
    level_cache( const level_cache &other ) = default;

    bool transparency_cache_dirty;
    bool outside_cache_dirty;

    float lm[MAPSIZE*SEEX][MAPSIZE*SEEY];
    float sm[MAPSIZE*SEEX][MAPSIZE*SEEY];
    // To prevent redundant ray casting into neighbors: precalculate bulk light source positions.
    // This is only valid for the duration of generate_lightmap
    float light_source_buffer[MAPSIZE*SEEX][MAPSIZE*SEEY];
    bool outside_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
    float transparency_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
    float seen_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];
    lit_level visibility_cache[MAPSIZE*SEEX][MAPSIZE*SEEY];

    bool veh_in_active_range;
    bool veh_exists_at[SEEX * MAPSIZE][SEEY * MAPSIZE];
    std::map< tripoint, std::pair<vehicle*,int> > veh_cached_parts;
    std::set<vehicle*> vehicle_list;
};

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
 map(int mapsize = MAPSIZE, bool zlev = false);
 map( bool zlev ) : map( MAPSIZE, zlev ) { }
 ~map();

 map &operator=( map&& ) = default;

// Visual Output
 void debug();



    /**
     * Sets a dirty flag on the transparency cache.
     *
     * If this isn't set, it's just assumed that
     * the transparency cache hasn't changed and
     * doesn't need to be updated.
     */
    void set_transparency_cache_dirty( const int zlev ) {
        if( inbounds_z( zlev ) ) {
            get_cache( zlev ).transparency_cache_dirty = true;
        }
    }

    /**
     * Sets a dirty flag on the outside cache.
     *
     * If this isn't set, it's just assumed that
     * the outside cache hasn't changed and
     * doesn't need to be updated.
     */
    void set_outside_cache_dirty( const int zlev ) {
        if( inbounds_z( zlev ) ) {
            get_cache( zlev ).outside_cache_dirty = true;
        }
    }

    /**
     * Callback invoked when a vehicle has moved.
     */
    void on_vehicle_moved( const int zlev );

    /** Determine the visible light level for a tile, based on light_at
     * for the tile, vision distance, etc
     *
     * @param x, y The tile on this map to draw.
     */
    lit_level apparent_light_at( const tripoint &p, const visibility_variables &cache ) const;
    visibility_type get_visibility( const lit_level ll,
                                    const visibility_variables &cache ) const;

    bool apply_vision_effects( WINDOW *w, lit_level ll,
                               const visibility_variables &cache ) const;

    /** Draw a visible part of the map into `w`.
     *
     * This method uses `g->u.posx()/posy()` for visibility calculations, so it can
     * not be used for anything but the player's viewport. Likewise, only
     * `g->m` and maps with equivalent coordinates can be used, as other maps
     * would have coordinate systems incompatible with `g->u.posx()`
     *
     * @param center The coordinate of the center of the viewport, this can
     *               be different from the player coordinate.
     */
    void draw( WINDOW* w, const tripoint &center );

    /** Draw the map tile at the given coordinate. Called by `map::draw()`.
    *
    * @param p The tile on this map to draw.
    * @param view_center_x, view_center_y The center of the viewport to be rendered,
    *        see `center` in `map::draw()`
    */
    void drawsq( WINDOW* w, player &u, const tripoint &p,
                 const bool invert = false, const bool show_items = true ) const;
    void drawsq( WINDOW* w, player &u, const tripoint &p,
                 const bool invert, const bool show_items,
                 const tripoint &view_center,
                 const bool low_light = false, const bool bright_level = false,
                 const bool inorder = false) const;

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
     * @param wx global coordinates of the submap at grid[0]. This
     * is in submap coordinates.
     * @param wy see wx
     * @param wz see wx, this is the z-level
     * @param update_vehicles If true, add vehicles to the vehicle cache.
     */
    void load(const int wx, const int wy, const int wz, const bool update_vehicles);
    /**
     * Shift the map along the vector (sx,sy).
     * This is like loading the map with coordinates derived from the current
     * position of the map (@ref abs_sub) plus the shift vector.
     * Note: the map must have been loaded before this can be called.
     */
    void shift( const int sx, const int sy );
    /**
     * Moves the map vertically to (not by!) newz.
     * Does not actually shift anything, only forces cache updates.
     * In the future, it will either actually shift the map or it will get removed
     *  after 3D migration is complete.
     */
    void vertical_shift( const int newz );

 void clear_spawns();
 void clear_traps();

    const maptile maptile_at( const tripoint &p ) const;
    maptile maptile_at( const tripoint &p );
private:
    // Versions of the above that don't do bounds checks
    const maptile maptile_at_internal( const tripoint &p ) const;
    maptile maptile_at_internal( const tripoint &p );
public:

// Movement and LOS

// Move cost: 2D overloads
    int move_cost(const int x, const int y, const vehicle *ignored_vehicle = nullptr) const;
    bool impassable(int x, int y) const;
    bool passable(int x, int y) const;
    int move_cost_ter_furn(const int x, const int y) const;

// Move cost: 3D

    /**
    * Calculate the cost to move past the tile at p.
    *
    * The move cost is determined by various obstacles, such
    * as terrain, vehicles and furniture.
    *
    * @note Movement costs for players and zombies both use this function.
    *
    * @return The return value is interpreted as follows:
    * Move Cost | Meaning
    * --------- | -------
    * 0         | Impassable. Use `passable`/`impassable` to check for this.
    * n > 0     | x*n turns to move past this
    */
    int move_cost( const tripoint &p, const vehicle *ignored_vehicle = nullptr ) const;
    bool impassable( const tripoint &p ) const;
    bool passable( const tripoint &p ) const;

    /**
    * Similar behavior to `move_cost()`, but ignores vehicles.
    */
    int move_cost_ter_furn( const tripoint &p ) const;

    /**
    * Cost to move out of one tile and into the next.
    *
    * @return The cost in turns to move out of tripoint `from` and into `to`
    */
    int combined_movecost( const tripoint &from, const tripoint &to,
                           const vehicle *ignored_vehicle = nullptr,
                           const int modifier = 0, const bool flying = false ) const;

    /**
     * Returns true if a creature could walk from `from` to `to` in one step.
     * That is, if the tiles are adjacent and either on the same z-level or connected
     * by stairs or (in case of flying monsters) open air with no floors.
     */
    bool valid_move( const tripoint &from, const tripoint &to,
                     const bool bash = false, const bool flying = false ) const;


// 3D Sees:
    /**
    * Returns whether `F` sees `T` with a view range of `range`.
    */
    bool sees( const tripoint &F, const tripoint &T, int range ) const;
 private:
    /**
     * Don't expose the slope adjust outside map functions.
     *
     * @param bresenham_slope Indicates the start offset of Bresenham line used to connect
     * the two points, and may subsequently be used to form a path between them.
     * Set to zero if the function returns false.
    **/
    bool sees( const tripoint &F, const tripoint &T, int range, int &bresenham_slope ) const;
 public:
    /**
     * Check whether there's a direct line of sight between `F` and
     * `T` with the additional movecost restraints.
     *
     * Checks two things:
     * 1. The `sees()` algorithm between `F` and `T`
     * 2. That moving over the line of sight would have a move_cost between
     *    `cost_min` and `cost_max`.
     */
    bool clear_path( const tripoint &f, const tripoint &t, const int range,
                     const int cost_min, const int cost_max ) const;

    /**
     * Iteratively tries bresenham lines with different biases
     * until it finds a clear line or decides there isn't one.
     * returns the line found, which may be the staright line, but blocked.
     */
    std::vector<tripoint> find_clear_path( const tripoint &source, const tripoint&destination ) const;

    /**
     * Check whether items in the target square are accessible from the source square
     * `f` and `t`.
     *
     * Checks two things:
     * 1. The `sees()` algorithm between `f` and `t` OR origin and target match.
     * 2. That the target location isn't sealed.
     */
    bool accessible_items( const tripoint &f, const tripoint &t, const int range ) const;
    /**
     * Like @ref accessible_items but checks for accessible furniture.
     * It ignores the furniture flags of the target square (ignores if target is SEALED).
     */
    bool accessible_furniture( const tripoint &f, const tripoint &t, const int range ) const;

 /**
  * Calculate next search points surrounding the current position.
  * Points closer to the target come first.
  * This method leads to straighter lines and prevents weird looking movements away from the target.
  */
 std::vector<point> getDirCircle(const int Fx, const int Fy, const int Tx, const int Ty) const;
 std::vector<tripoint> get_dir_circle( const tripoint &f, const tripoint &t ) const;

 /**
  * Calculate a best path using A*
  *
  * @param f The source location from which to path.
  * @param t The destination to which to path.
  * @param bash Bashing strength of pathing creature (0 means no bashing through terrain).
  * @param maxdist Consider only paths up to this length (move cost multiplies "length" of a tile).
  */
 std::vector<tripoint> route( const tripoint &f, const tripoint &t,
                              const int bash, const int maxdist ) const;

 int coord_to_angle(const int x, const int y, const int tgtx, const int tgty) const;
// Vehicles: Common to 2D and 3D
    VehicleList get_vehicles();
    void add_vehicle_to_cache( vehicle * );
    void update_vehicle_cache( vehicle *, int old_zlevel );
    void reset_vehicle_cache( int zlev );
    void clear_vehicle_cache( int zlev );
    void clear_vehicle_list( int zlev );
    void update_vehicle_list( submap * const to, const int zlev );

    void destroy_vehicle (vehicle *veh);
    void vehmove();          // Vehicle movement
    bool vehproceed(); // Returns true if a vehicle moved, false otherwise

// 3D vehicles
    VehicleList get_vehicles( const tripoint &start, const tripoint &end );
    /**
    * Checks if tile is occupied by vehicle and by which part.
    *
    * @param part_num The part number of the part at this tile will be returned in this parameter.
    * @return A pointer to the vehicle in this tile.
    */
    vehicle* veh_at( const tripoint &p, int &part_num );
    const vehicle* veh_at( const tripoint &p, int &part_num ) const;
    vehicle* veh_at_internal( const tripoint &p, int &part_num );
    const vehicle* veh_at_internal( const tripoint &p, int &part_num ) const;
    /**
    * Same as `veh_at(const int, const int, int)`, but doesn't return part number.
    */
    vehicle* veh_at( const tripoint &p );// checks if tile is occupied by vehicle
    const vehicle* veh_at( const tripoint &p ) const;
    /**
    * Vehicle-relative coordinates from reality bubble coordinates, if a vehicle
    * actually exists here.
    * Returns 0,0 if no vehicle exists there (use veh_at to check if it exists first)
    */
    point veh_part_coordinates( const tripoint &p );
    // put player on vehicle at x,y
    void board_vehicle( const tripoint &p, player *pl );
    void unboard_vehicle( const tripoint &p );//remove player from vehicle at p
    // Change vehicle coords and move vehicle's driver along.
    // WARNING: not checking collisions!
    void displace_vehicle( tripoint &p, const tripoint &dp );
    // move water under wheels. true if moved
    bool displace_water( const tripoint &dp );

    // Returns the modifier on wheel area due to bad surface
    // 1.0 on road, 0.0 in air
    // <0.0 when the vehicle should be destroyed (sunk in water)
    // TODO: Add the values between 0.0 and 1.0 for offroading
    // TODO: Remove the ugly sinking vehicle hack
    float vehicle_traction( vehicle &veh ) const;

    // Like traction, except for water
    // TODO: Actually implement (this is a stub)
    // TODO: Test for it when the vehicle sinks rather than when it has VP_FLOATS
    float vehicle_buoyancy( vehicle &veh ) const;

    // Returns if the vehicle should fall down a z-level
    bool vehicle_falling( vehicle &veh );

    // Executes vehicle-vehicle collision based on vehicle::collision results
    // Returns impulse of the executed collision
    // If vector contains collisions with vehicles other than veh2, they will be ignored
    float vehicle_vehicle_collision( vehicle &veh, vehicle &veh2,
                                     const std::vector<veh_collision> &collisions );
    // Throws vehicle passengers about the vehicle, possibly out of it
    // Returns change in vehicle orientation due to lost control
    int shake_vehicle( vehicle &veh, int velocity_before, int direction );

    // Actually moves the vehicle
    // Unlike displace_vehicle, this one handles collisions
    void move_vehicle( vehicle &veh, const tripoint &dp, const tileray &facing );

// Furniture: 2D overloads
    void set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture);
    void set(const int x, const int y, const std::string new_terrain, const std::string new_furniture);

    std::string name(const int x, const int y);
    bool has_furn(const int x, const int y) const;

    furn_id furn(const int x, const int y) const; // Furniture at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
    std::string get_furn(const int x, const int y) const;
    const furn_t & furn_at(const int x, const int y) const;

    void furn_set(const int x, const int y, const furn_id new_furniture);
    void furn_set(const int x, const int y, const std::string new_furniture);

    std::string furnname(const int x, const int y);
// Furniture: 3D
    void set( const tripoint &p, const ter_id new_terrain, const furn_id new_furniture );
    void set( const tripoint &p, const std::string new_terrain, const std::string new_furniture );

    std::string name( const tripoint &p );
    std::string disp_name( const tripoint &p );
    bool has_furn( const tripoint &p ) const;

    furn_id furn( const tripoint &p ) const;
    std::string get_furn( const tripoint &p ) const;
    const furn_t & furn_at( const tripoint &p ) const;

    void furn_set( const tripoint &p, const furn_id new_furniture );
    void furn_set( const tripoint &p, const std::string new_furniture );

    std::string furnname( const tripoint &p);
    bool can_move_furniture( const tripoint &pos, player * p = nullptr );
// Terrain: 2D overloads
    ter_id ter(const int x, const int y) const; // Terrain integer id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
    std::string get_ter(const int x, const int y) const; // Terrain string id at coord (x, y); {x|y}=(0, SEE{X|Y}*3]
    std::string get_ter_harvestable(const int x, const int y) const; // harvestable of the terrain
    ter_id get_ter_transforms_into(const int x, const int y) const; // get the terrain id to transform to
    int get_ter_harvest_season(const int x, const int y) const; // get season to harvest the terrain
    const ter_t & ter_at(const int x, const int y) const; // Terrain at coord (x, y); {x|y}=(0, SEE{X|Y}*3]

    void ter_set(const int x, const int y, const ter_id new_terrain);
    void ter_set(const int x, const int y, const std::string new_terrain);

    std::string tername(const int x, const int y) const; // Name of terrain at (x, y)
// Terrain: 3D
    ter_id ter( const tripoint &p ) const;
    std::string get_ter( const tripoint &p ) const;
    std::string get_ter_harvestable( const tripoint &p ) const;
    ter_id get_ter_transforms_into( const tripoint &p ) const;
    int get_ter_harvest_season( const tripoint &p ) const;
    const ter_t & ter_at( const tripoint &p ) const;

    void ter_set( const tripoint &p, const ter_id new_terrain);
    void ter_set( const tripoint &p, const std::string new_terrain);

    std::string tername( const tripoint &p ) const;

    // Check for terrain/furniture/field that provide a
    // "fire" item to be used for example when crafting or when
    // a iuse function needs fire.
    bool has_nearby_fire( const tripoint &p, int radius = 1);
    /**
     * Check if creature can see some items at p. Includes:
     * - check for items at this location (has_items(p))
     * - check for SEALED flag (sealed furniture/terrain makes
     * items not visible under any circumstances).
     * - check for CONTAINER flag (makes items only visible when
     * the creature is at p or at an adjacent square).
     */
    bool sees_some_items( const tripoint &p, const Creature &who ) const;
    /**
     * Check if the creature could see items at p if there were
     * any items. This is similar to @ref sees_some_items, but it
     * does not check that there are actually any items.
     */
    bool could_see_items( const tripoint &p, const Creature &who ) const;
    /**
     * Checks for existence of items. Faster than i_at(p).empty
     */
    bool has_items( const tripoint &p ) const;

// Flags: 2D overloads
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
// Flags: 3D
    std::string features( const tripoint &p ); // Words relevant to terrain (sharp, etc)
    bool has_flag( const std::string &flag, const tripoint &p ) const;  // checks terrain, furniture and vehicles
    bool can_put_items( const tripoint &p ); // True if items can be placed in this tile
    bool has_flag_ter( const std::string &flag, const tripoint &p ) const;  // checks terrain
    bool has_flag_furn( const std::string &flag, const tripoint &p ) const;  // checks furniture
    bool has_flag_ter_or_furn( const std::string &flag, const tripoint &p ) const; // checks terrain or furniture
    bool has_flag_ter_and_furn( const std::string &flag, const tripoint &p ) const; // checks terrain and furniture
    // fast "oh hai it's update_scent/lightmap/draw/monmove/self/etc again, what about this one" flag checking
    bool has_flag( const ter_bitflags flag, const tripoint &p ) const;  // checks terrain, furniture and vehicles
    bool has_flag_ter( const ter_bitflags flag, const tripoint &p ) const;  // checks terrain
    bool has_flag_furn( const ter_bitflags flag, const tripoint &p ) const;  // checks furniture
    bool has_flag_ter_or_furn( const ter_bitflags flag, const tripoint &p ) const; // checks terrain or furniture
    bool has_flag_ter_and_furn( const ter_bitflags flag, const tripoint &p ) const; // checks terrain and furniture

// Bashable: 2D
    bool is_bashable(const int x, const int y) const;
    bool is_bashable_ter(const int x, const int y) const;
    bool is_bashable_furn(const int x, const int y) const;
    bool is_bashable_ter_furn(const int x, const int y) const;
    int bash_strength(const int x, const int y) const;
    int bash_resistance(const int x, const int y) const;
    int bash_rating(const int str, const int x, const int y) const;
// Bashable: 3D
    /** Returns true if there is a bashable vehicle part or the furn/terrain is bashable at p */
    bool is_bashable( const tripoint &p, bool allow_floor = false ) const;
    /** Returns true if the terrain at p is bashable */
    bool is_bashable_ter( const tripoint &p, bool allow_floor = false ) const;
    /** Returns true if the furniture at p is bashable */
    bool is_bashable_furn( const tripoint &p ) const;
    /** Returns true if the furniture or terrain at p is bashable */
    bool is_bashable_ter_furn( const tripoint &p, bool allow_floor = false ) const;
    /** Returns max_str of the furniture or terrain at p */
    int bash_strength( const tripoint &p, bool allow_floor = false ) const;
    /** Returns min_str of the furniture or terrain at p */
    int bash_resistance( const tripoint &p, bool allow_floor = false ) const;
    /** Returns a success rating from -1 to 10 for a given tile based on a set strength, used for AI movement planning
    *  Values roughly correspond to 10% increment chances of success on a given bash, rounded down. -1 means the square is not bashable */
    int bash_rating( const int str, const tripoint &p, bool allow_floor = false ) const;

    /** Generates rubble at the given location, if overwrite is true it just writes on top of what currently exists
     *  floor_type is only used if there is a non-bashable wall at the location or with overwrite = true */
    void make_rubble( const tripoint &p, furn_id rubble_type, bool items,
                      ter_id floor_type, bool overwrite = false );
    void make_rubble( const tripoint &p );
    void make_rubble( const tripoint &p, furn_id rubble_type, bool items );
    void make_rubble( int, int, furn_id rubble_type, bool items,
                      ter_id floor_type, bool overwrite = false) = delete;

 bool is_divable(const int x, const int y) const;
 bool is_outside(const int x, const int y) const;
    bool is_divable( const tripoint &p ) const;
    bool is_outside( const tripoint &p ) const;
 /** Check if the last terrain is wall in direction NORTH, SOUTH, WEST or EAST
  *  @param no_furn if true, the function will stop and return false
  *  if it encounters a furniture
  *  @return true if from x to xmax or y to ymax depending on direction
  *  all terrain is floor and the last terrain is a wall */
 bool is_last_ter_wall(const bool no_furn, const int x, const int y,
                       const int xmax, const int ymax, const direction dir) const;
    bool flammable_items_at( const tripoint &p );
    bool moppable_items_at( const tripoint &p );
 point random_outdoor_tile();
// mapgen

void draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2);
void draw_line_ter(const std::string type, int x1, int y1, int x2, int y2);
void draw_line_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_line_furn(const std::string type, int x1, int y1, int x2, int y2);
void draw_fill_background(ter_id type);
void draw_fill_background(std::string type);
void draw_fill_background(ter_id (*f)());
void draw_fill_background(const id_or_id<ter_t> & f);

void draw_square_ter(ter_id type, int x1, int y1, int x2, int y2);
void draw_square_ter(std::string type, int x1, int y1, int x2, int y2);
void draw_square_furn(furn_id type, int x1, int y1, int x2, int y2);
void draw_square_furn(std::string type, int x1, int y1, int x2, int y2);
void draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2);
void draw_square_ter(const id_or_id<ter_t> & f, int x1, int y1, int x2, int y2);
void draw_rough_circle(ter_id type, int x, int y, int rad);
void draw_rough_circle(std::string type, int x, int y, int rad);
void draw_rough_circle_furn(furn_id type, int x, int y, int rad);
void draw_rough_circle_furn(std::string type, int x, int y, int rad);

void add_corpse( const tripoint &p );


// Terrain changing functions
    void translate( const ter_id from, const ter_id to ); // Change all instances of $from->$to
    void translate_radius( const ter_id from, const ter_id to, const float radi, const tripoint &p );
    bool close_door( const tripoint &p, const bool inside, const bool check_only );
    bool open_door( const tripoint &p, const bool inside, const bool check_only = false );
// Destruction
     /** Keeps bashing a square until it can't be bashed anymore */
    void destroy( const tripoint &p, const bool silent = false);
    /** Keeps bashing a square until there is no more furniture */
    void destroy_furn( const tripoint &p, const bool silent = false );
    void crush( const tripoint &p );
    void shoot( const tripoint &p, projectile &proj, const bool hit_items );
    /** Checks if a square should collapse, returns the X for the one_in(X) collapse chance */
    int collapse_check( const tripoint &p );
    /** Causes a collapse at p, such as from destroying a wall */
    void collapse_at( const tripoint &p, bool silent );
    /** Tries to smash the items at the given tripoint. Used by the explosion code */
    void smash_items( const tripoint &p, const int power );
    /**
     * Returns a pair where first is whether anything was smashed and second is if it was destroyed.
     *
     * @param silent Don't produce any sound
     * @param destroy Destroys some otherwise unbashable tiles
     * @param bash_floor Allow bashing the floor and the tile that supports it
     * @param bashing_vehicle Vehicle that should NOT be bashed (because it is doing the bashing)
     */
    bash_params bash( const tripoint &p, int str, bool silent = false,
                      bool destroy = false, bool bash_floor = false,
                      const vehicle *bashing_vehicle = nullptr );

// Effects of attacks/items
    bool hit_with_acid( const tripoint &p );
    bool hit_with_fire( const tripoint &p );
    bool marlossify( const tripoint &p );
    /** Makes spores at p. source is used for kill counting */
    void create_spores( const tripoint &p, Creature* source = nullptr );
    void fungalize( const tripoint &p, Creature *source = nullptr, double spore_chance = 0.0 );

    bool has_adjacent_furniture( const tripoint &p );
    void mop_spills( const tripoint &p );
    /**
    * Moved here from weather.cpp for speed. Decays fire, washable fields and scent.
    * Washable fields are decayed only by 1/3 of the amount fire is.
    */
    void decay_fields_and_scent( const int amount );

// Signs
    const std::string get_signage( const tripoint &p ) const;
    void set_signage( const tripoint &p, std::string message ) const;
    void delete_signage( const tripoint &p ) const;

// Radiation
    int get_radiation( const tripoint &p ) const; // Amount of radiation at (x, y);
    void set_radiation( const tripoint &p, const int value );
    // Overload for mapgen
    void set_radiation( const int x, const int y, const int value );

    /** Increment the radiation in the given tile by the given delta
    *  (decrement it if delta is negative)
    */
    void adjust_radiation( const tripoint &p, const int delta );
    // Overload for mapgen
    void adjust_radiation( const int x, const int y, const int delta );

// Temperature
    int& temperature( const tripoint &p );    // Temperature for submap
    void set_temperature( const tripoint &p, const int temperature ); // Set temperature for all four submap quadrants
    // 2D overload for mapgen
    void set_temperature( const int x, const int y, const int temperature );

// Items
    void process_active_items();
    void trigger_rc_items( std::string signal );

// Items: 2D
    map_stack i_at(int x, int y);
    void i_clear(const int x, const int y);
    std::list<item>::iterator i_rem( const point location, std::list<item>::iterator it );
    int i_rem(const int x, const int y, const int index);
    void i_rem(const int x, const int y, item* it);
    void spawn_item(const int x, const int y, const std::string &itype_id,
                    const unsigned quantity=1, const long charges=0,
                    const unsigned birthday=0, const int damlevel=0, const bool rand = true);
    int max_volume(const int x, const int y);
    int free_volume(const int x, const int y);
    int stored_volume(const int x, const int y);
    bool add_item_or_charges(const int x, const int y, item new_item, int overflow_radius = 2);
    void add_item(const int x, const int y, item new_item);
    void spawn_an_item( const int x, const int y, item new_item,
                        const long charges, const int damlevel );
    int place_items(items_location loc, const int chance, const int x1, const int y1,
                  const int x2, const int y2, bool ongrass, const int turn, bool rand = true);
    void spawn_items(const int x, const int y, const std::vector<item> &new_items);
    void create_anomaly(const int cx, const int cy, artifact_natural_property prop);
// Items: 3D
    // Accessor that returns a wrapped reference to an item stack for safe modification.
    map_stack i_at( const tripoint &p );
    item water_from( const tripoint &p );
    void i_clear( const tripoint &p );
    // i_rem() methods that return values act like container::erase(),
    // returning an iterator to the next item after removal.
    std::list<item>::iterator i_rem( const tripoint &p, std::list<item>::iterator it );
    int i_rem( const tripoint &p, const int index );
    void i_rem( const tripoint &p, const item* it );
    void spawn_artifact( const tripoint &p );
    void spawn_natural_artifact( const tripoint &p, const artifact_natural_property prop );
    void spawn_item( const tripoint &p, const std::string &itype_id,
                     const unsigned quantity=1, const long charges=0,
                     const unsigned birthday=0, const int damlevel=0, const bool rand = true);
    int max_volume( const tripoint &p );
    int free_volume( const tripoint &p );
    int stored_volume( const tripoint &p );
    bool is_full( const tripoint &p, const int addvolume = -1, const int addnumber = -1 );
    item &add_item_or_charges( const tripoint &p, item new_item, int overflow_radius = 2 );
    item &add_item_at( const tripoint &p, std::list<item>::iterator index, item new_item );
    item &add_item( const tripoint &p, item new_item );
    item &spawn_an_item( const tripoint &p, item new_item,
                        const long charges, const int damlevel);

    /**
     * @name Consume items on the map
     *
     * The functions here consume accessible items / item charges on the map or in vehicles
     * around the player (whose positions is given as origin).
     * They return a list of copies of the consumed items (with the actually consumed charges
     * in it).
     * The quantity / amount parameter will be reduced by the number of items/charges removed.
     * If all required items could be removed from the map, the quantity/amount will be 0,
     * otherwise it will contain a positive value and the remaining items must be gathered from
     * somewhere else.
     */
    /*@{*/
    std::list<item> use_amount_square( const tripoint &p, const itype_id type,
                                       long &quantity );
    std::list<item> use_amount( const tripoint &origin, const int range, const itype_id type,
                                long &amount );
    std::list<item> use_charges( const tripoint &origin, const int range, const itype_id type,
                                 long &amount );
    /*@}*/
    std::list<std::pair<tripoint, item *> > get_rc_items( int x = -1, int y = -1, int z = -1 );

    /**
    * Place items from item group in the rectangle f - t. Several items may be spawned
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
    int place_items( items_location loc, const int chance, const tripoint &f,
                     const tripoint &t, bool ongrass, const int turn, bool rand = true );
    /**
    * Place items from an item group at p. Places as much items as the item group says.
    * (Most item groups are distributions and will only create one item.)
    * @param turn The birthday that the created items shall have.
    * @return Vector of pointers to placed items (can be empty, but no nulls).
    */
    std::vector<item*> put_items_from_loc( items_location loc, const tripoint &p, const int turn = 0 );

    // Similar to spawn_an_item, but spawns a list of items, or nothing if the list is empty.
    std::vector<item*> spawn_items( const tripoint &p, const std::vector<item> &new_items );
    void create_anomaly( const tripoint &p, artifact_natural_property prop );

 /**
  * Fetch an item from this map location, with sanity checks to ensure it still exists.
  */
 item *item_from( const tripoint &pos, const size_t index );

 /**
  * Fetch an item from this vehicle, with sanity checks to ensure it still exists.
  */
 item *item_from( vehicle *veh, const int cargo_part, const size_t index );

// Traps: 3D
 void trap_set( const tripoint &p, const trap_id id);

    const trap & tr_at( const tripoint &p ) const;
 void add_trap( const tripoint &p, const trap_id t);
 void disarm_trap( const tripoint &p );
 void remove_trap( const tripoint &p );
 const std::vector<tripoint> &trap_locations(trap_id t) const;

 bool process_fields(); // See fields.cpp
 bool process_fields_in_submap( submap * const current_submap,
                                const int submap_x, const int submap_y, const int submap_z); // See fields.cpp
        /**
         * Apply field effects to the creature when it's on a square with fields.
         */
        void creature_in_field( Creature &critter );
        /**
         * Apply trap effects to the creature, similar to @ref creature_in_field.
         * If there is no trap at the creatures location, nothing is done.
         * If the creature can avoid the trap, nothing is done as well.
         * Otherwise the trap is triggered.
         * @param may_avoid If true, the creature tries to avoid the trap
         * (@ref Creature::avoid_trap). If false, the trap is always triggered.
         */
        void creature_on_trap( Creature &critter, bool may_avoid = true );
// 3D field functions. Eventually all the 2D ones should be replaced with those
        /**
         * Get the fields that are here. This is for querying and looking at it only,
         * one can not change the fields.
         * @param p The local map coordinates, if out of bounds, returns an empty field.
         */
        const field &field_at( const tripoint &p ) const;
        /**
         * Gets fields that are here. Both for querying and edition.
         */
        field &field_at( const tripoint &p );
        /**
         * Get the age of a field entry (@ref field_entry::age), if there is no
         * field of that type, returns -1.
         */
        int get_field_age( const tripoint &p, const field_id t ) const;
        /**
         * Get the density of a field entry (@ref field_entry::density),
         * if there is no field of that type, returns 0.
         */
        int get_field_strength( const tripoint &p, const field_id t ) const;
        /**
         * Increment/decrement age of field entry at point.
         * @return resulting age or -1 if not present (does *not* create a new field).
         */
        int adjust_field_age( const tripoint &p, const field_id t, const int offset );
        /**
         * Increment/decrement density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         */
        int adjust_field_strength( const tripoint &p, const field_id t, const int offset );
        /**
         * Set age of field entry at point.
         * @return resulting age or -1 if not present (does *not* create a new field).
         * @param isoffset If true, the given age value is added to the existing value,
         * if false, the existing age is ignored and overridden.
         */
        int set_field_age( const tripoint &p, const field_id t, const int age, bool isoffset = false );
        /**
         * Set density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         * @param isoffset If true, the given str value is added to the existing value,
         * if false, the existing density is ignored and overridden.
         */
        int set_field_strength( const tripoint &p, const field_id t, const int str, bool isoffset = false );
        /**
         * Get field of specific type at point.
         * @return NULL if there is no such field entry at that place.
         */
        field_entry *get_field( const tripoint &p, const field_id t );
        /**
         * Add field entry at point, or set density if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */
        bool add_field( const tripoint &p, const field_id t, const int density, const int age);
        /**
         * Remove field entry at xy, ignored if the field entry is not present.
         */
        void remove_field( const tripoint &p, const field_id field_to_remove );
// End of 3D field function block

// Scent propagation helpers
    /**
     * Build the map of scent-resistant tiles.
     * Should be way faster than if done in `game.cpp` using public map functions.
     */
    void scent_blockers( bool (&blocks_scent)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                         bool (&reduces_scent)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                         int minx, int miny, int maxx, int maxy );

// Computers
    computer* computer_at( const tripoint &p );
    computer* add_computer( const tripoint &p, std::string name, const int security );

 // Camps
    bool allow_camp( const tripoint &p, const int radius = CAMPCHECK);
    basecamp* camp_at( const tripoint &p, const int radius = CAMPSIZE);
    void add_camp( const tripoint &p, const std::string& name );

// Graffiti
    bool has_graffiti_at( const tripoint &p ) const;
    const std::string &graffiti_at( const tripoint &p ) const;
    void set_graffiti( const tripoint &p, const std::string &contents);
    void delete_graffiti( const tripoint &p );

// Climbing
    /**
     * Checks 3x3 block centered on p for terrain to climb.
     * @return Difficulty of climbing check from point p.
     */
    int climb_difficulty( const tripoint &p ) const;

// Support (of weight, structures etc.)
private:
    // Tiles whose ability to support things was removed in the last turn
    std::set<tripoint> support_cache_dirty;
    // Checks if the tile is supported and adds it to support_cache_dirty if it isn't
    void support_dirty( const tripoint &p );
public:

    // Returns true if terrain at p has NO flag TFLAG_NO_FLOOR,
    // if we're not in zlevels mode or if we're at lowest level
    bool has_floor( const tripoint &p ) const;
    /** Does this tile support vehicles and furniture above it */
    bool supports_above( const tripoint &p ) const;
    bool has_floor_or_support( const tripoint &p ) const;

    /**
     * Handles map objects of given type (not creatures) falling down.
     * Returns true if anything changed.
     */
    /*@{*/
    void drop_everything( const tripoint &p );
    void drop_furniture( const tripoint &p );
    void drop_items( const tripoint &p );
    void drop_vehicle( const tripoint &p );
    /*@}*/

    /**
     * Invoked @ref drop_everything on cached dirty tiles.
     */
    void process_falling();

// mapgen.cpp functions
 void generate(const int x, const int y, const int z, const int turn);
 void post_process(unsigned zones);
 void place_spawns(const mongroup_id& group, const int chance,
                   const int x1, const int y1, const int x2, const int y2, const float density);
 void place_gas_pump(const int x, const int y, const int charges);
 void place_gas_pump(const int x, const int y, const int charges, std::string fuel_type);
 void place_toilet(const int x, const int y, const int charges = 6 * 4); // 6 liters at 250 ml per charge
 void place_vending(int x, int y, std::string type);
 int place_npc(int x, int y, std::string type);

 void add_spawn(const mtype_id& type, const int count, const int x, const int y, bool friendly = false,
                const int faction_id = -1, const int mission_id = -1,
                std::string name = "NONE");
 vehicle *add_vehicle(const vgroup_id &type, const point &p, const int dir,
                      const int init_veh_fuel = -1, const int init_veh_status = -1,
                      const bool merge_wrecks = true);
 vehicle *add_vehicle(const vproto_id &type, const int x, const int y, const int dir,
                      const int init_veh_fuel = -1, const int init_veh_status = -1,
                      const bool merge_wrecks = true);
    void build_map_cache( int zlev, bool skip_lightmap = false );

    vehicle *add_vehicle( const vgroup_id &type, const tripoint &p, const int dir,
                          const int init_veh_fuel = -1, const int init_veh_status = -1,
                          const bool merge_wrecks = true);
    vehicle *add_vehicle( const vproto_id &type, const tripoint &p, const int dir,
                          const int init_veh_fuel = -1, const int init_veh_status = -1,
                          const bool merge_wrecks = true);

// Light/transparency: 3D
    float light_transparency( const tripoint &p ) const;
    lit_level light_at( const tripoint &p ) const; // Assumes 0,0 is light map center
    float ambient_light_at( const tripoint &p ) const; // Raw values for tilesets
    /**
     * Returns whether the tile at `p` is transparent(you can look past it).
     */
    bool trans( const tripoint &p ) const; // Transparent?
// End of light/transparency
        /**
         * Whether the player character (g->u) can see the given square (local map coordinates).
         * This only checks the transparency of the path to the target, the light level is not
         * checked.
         * @param max_range All squares that are further away than this are invisible.
         * Ignored if smaller than 0.
         */
        bool pl_sees( const tripoint &t, int max_range ) const;
    std::set<vehicle*> dirty_vehicle_list;

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
     * Translates tripoint in local coords (near player) to global,
     * just as the 2D variant of the function.
     * z-coord remains unchanged (it is always global).
     */
    tripoint getabs( const tripoint &p ) const;
    /**
     * Inverse of @ref getabs
     */
    point getlocal(const int x, const int y ) const;
    point getlocal(const point p) const { return getlocal(p.x, p.y); }
    tripoint getlocal( const tripoint &p ) const;
 bool inbounds(const int x, const int y) const;
 bool inbounds(const int x, const int y, const int z) const;
 bool inbounds( const tripoint &p ) const;

    bool inbounds_z( const int z ) const {
        return z >= -OVERMAP_DEPTH && z <= OVERMAP_HEIGHT;
    }

    /** Clips the coords of p to fit the map bounds */
    void clip_to_bounds( tripoint &p ) const;
    void clip_to_bounds( int &x, int &y ) const;
    void clip_to_bounds( int &x, int &y, int &z ) const;

 int getmapsize() const { return my_MAPSIZE; };
 bool has_zlevels() const { return zlevels; }

 // Not protected/private for mapgen_functions.cpp access
 void rotate(const int turns);// Rotates the current map 90*turns degress clockwise
                              // Useful for houses, shops, etc

// Monster spawning:
public:
    /**
     * Spawn monsters from submap spawn points and from the overmap.
     * @param ignore_sight If true, monsters may spawn in the view of the player
     * character (useful when the whole map has been loaded instead, e.g.
     * when starting a new game, or after teleportation or after moving vertically).
     * If false, monsters are not spawned in view of of player character.
     */
    void spawn_monsters( bool ignore_sight );
private:
    // Helper #1 - spawns monsters on one submap
    void spawn_monsters_submap( const tripoint &gp, bool ignore_sight );
    // Helper #2 - spawns monsters on one submap and from one group on this submap
    void spawn_monsters_submap_group( const tripoint &gp, mongroup &group, bool ignore_sight );

protected:
        void saven( int gridx, int gridy, int gridz );
        void loadn( int gridx, int gridy, bool update_vehicles );
        void loadn( int gridx, int gridy, int gridz, bool update_vehicles );
        /**
         * Fast forward a submap that has just been loading into this map.
         * This is used to rot and remove rotten items, grow plants, fill funnels etc.
         */
        void actualize( const int gridx, const int gridy, const int gridz );
        /**
         * Whether the item has to be removed as it has rotten away completely.
         * @param pnt The *absolute* position of the item in the world (not just on this map!),
         * used for rot calculation.
         * @return true if the item has rotten away and should be removed, false otherwise.
         */
        bool has_rotten_away( item &itm, const tripoint &pnt ) const;
        /**
         * Go through the list of items, update their rotten status and remove items
         * that have rotten away completely.
         * @param pnt The point on this map where the items are, used for rot calculation.
         */
        template <typename Container>
        void remove_rotten_items( Container &items, const tripoint &p );
        /**
         * Try to fill funnel based items here.
         * @param pnt The location in this map where to fill funnels.
         */
        void fill_funnels( const tripoint &p );
        /**
         * Try to grow a harvestable plant to the next stage(s).
         */
        void grow_plant( const tripoint &p );
        /**
         * Try to grow fruits on static plants (not planted by the player)
         * @param time_since_last_actualize Time (in turns) since this function has been
         * called the last time.
         */
        void restock_fruits( const tripoint &p, int time_since_last_actualize );
        void player_in_field( player &u );
        void monster_in_field( monster &z );
        /**
         * As part of the map shifting, this shifts the trap locations stored in @ref traplocs.
         * @param shift The amount shifting in submap, the same as go into @ref shift.
         */
        void shift_traps( const tripoint &shift );

        void copy_grid( const tripoint &to, const tripoint &from );
 void draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
                const oter_id t_south, const oter_id t_west, const oter_id t_neast,
                const oter_id t_seast, const oter_id t_nwest, const oter_id t_swest,
                const oter_id t_above, const int turn, const float density,
                const int zlevel, const regional_settings * rsettings);

 void build_transparency_cache( int zlev );
public:
 void build_outside_cache( int zlev );
protected:
 void generate_lightmap( int zlev );
 void build_seen_cache( const tripoint &origin, int target_z );
 void apply_character_light( const player &p );

 int my_MAPSIZE;
 bool zlevels;

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
    field& get_field( const tripoint &p );

        /**
         * Get the submap pointer with given index in @ref grid, the index must be valid!
         */
        submap *getsubmap( size_t grididx ) const;
        /**
         * Get the submap pointer containing the specified position within the reality bubble.
         * (x,y) must be a valid coordinate, check with @ref inbounds.
         */
        submap *get_submap_at( int x, int y ) const;
        submap *get_submap_at( int x, int y, int z ) const;
        submap *get_submap_at( const tripoint &p ) const;
        /**
         * Get the submap pointer containing the specified position within the reality bubble.
         * The same as other get_submap_at, (x,y,z) must be valid (@ref inbounds).
         * Also writes the position within the submap to offset_x, offset_y
         * offset_z would always be 0, so it is not used here
         */
        submap *get_submap_at( const int x, const int y, int& offset_x, int& offset_y ) const;
        submap *get_submap_at( const int x, const int y, const int z,
                               int &offset_x, int &offset_y ) const;
        submap *get_submap_at( const tripoint &p, int &offset_x, int &offset_y ) const;
        /**
         * Get submap pointer in the grid at given grid coordinates. Grid coordinates must
         * be valid: 0 <= x < my_MAPSIZE, same for y.
         * z must be between -OVERMAP_DEPTH and OVERMAP_HEIGHT
         */
        submap *get_submap_at_grid( int gridx, int gridy ) const;
        submap *get_submap_at_grid( int gridx, int gridy, int gridz ) const;
        submap *get_submap_at_grid( const tripoint &gridp ) const;
        /**
         * Get the index of a submap pointer in the grid given by grid coordinates. The grid
         * coordinates must be valid: 0 <= x < my_MAPSIZE, same for y.
         * Version with z-levels checks for z between -OVERMAP_DEPTH and OVERMAP_HEIGHT
         */
        size_t get_nonant( int gridx, int gridy ) const;
        size_t get_nonant( const int gridx, const int gridy, const int gridz ) const;
        size_t get_nonant( const tripoint &gridp ) const;
        /**
         * Set the submap pointer in @ref grid at the give index. This is the inverse of
         * @ref getsubmap, any existing pointer is overwritten. The index must be valid.
         * The given submap pointer must not be null.
         */
        void setsubmap( size_t grididx, submap *smap );

    /**
     * Internal versions of public functions to avoid checking same variables multiple times.
     * They lack safety checks, because their callers already do those.
     */
    int move_cost_internal(const furn_t &furniture, const ter_t &terrain,
                           const vehicle *veh, const int vpart) const;
    int bash_rating_internal( const int str, const furn_t &furniture,
                              const ter_t &terrain, bool allow_floor,
                              const vehicle *veh, const int part ) const;

    /**
     * Internal version of the drawsq. Keeps a cached maptile for less re-getting.
     * Returns true if it has drawn all it should, false if `draw_from_above` should be called after.
     */
    bool draw_maptile( WINDOW* w, player &u, const tripoint &p,
                       const maptile &tile,
                       bool invert, bool show_items,
                       const tripoint &view_center,
                       bool low_light, bool bright_light, bool inorder ) const;
    /**
     * Draws the tile as seen from above.
     */
    void draw_from_above( WINDOW* w, player &u, const tripoint &p,
                          const maptile &tile, bool invert,
                          const tripoint &view_center,
                          bool low_light, bool bright_light, bool inorder ) const;

 long determine_wall_corner( const tripoint &p ) const;
 void cache_seen(const int fx, const int fy, const int tx, const int ty, const int max_range);
 // apply a circular light pattern immediately, however it's best to use...
 void apply_light_source( const tripoint &p, float luminance);
 // ...this, which will apply the light after at the end of generate_lightmap, and prevent redundant
 // light rays from causing massive slowdowns, if there's a huge amount of light.
 void add_light_source( const tripoint &p, float luminance);
 // Handle just cardinal directions and 45 deg angles.
 void apply_directional_light( const tripoint &p, int direction, float luminance );
 void apply_light_arc( const tripoint &p, int angle, float luminance, int wideangle = 30 );
 void apply_light_ray(bool lit[MAPSIZE*SEEX][MAPSIZE*SEEY],
                      const tripoint &s, const tripoint &e, float luminance);
 void add_light_from_items( const tripoint &p, std::list<item>::iterator begin,
                            std::list<item>::iterator end );
 void calc_ray_end(int angle, int range, const tripoint &p, tripoint &out ) const;
 vehicle *add_vehicle_to_map(vehicle *veh, bool merge_wrecks);

    // Internal methods used to bash just the selected features
    // Information on what to bash/what was bashed is read from/written to the bash_params struct
    void bash_ter_furn( const tripoint &p, bash_params &params );
    void bash_items( const tripoint &p, bash_params &params );
    void bash_vehicle( const tripoint &p, bash_params &params );
    void bash_field( const tripoint &p, bash_params &params );

    // Gets the roof type of the tile at p
    // Second argument refers to whether we have to get a roof (we're over an unpassable tile)
    // or can just return air because we bashed down an entire floor tile
    ter_id get_roof( const tripoint &p, bool allow_air );

 // Iterates over every item on the map, passing each item to the provided function.
 template<typename T>
     void process_items( bool active, T processor, std::string const &signal );
 template<typename T>
     void process_items_in_submap( submap * current_submap, const tripoint &gridp,
                                   T processor, std::string const &signal );
 template<typename T>
     void process_items_in_vehicles( submap *current_submap, T processor, std::string const &signal);
 template<typename T>
     void process_items_in_vehicle( vehicle *cur_veh, submap *current_submap,
                                    T processor, std::string const &signal );

    /** Enum used by functors in `function_over` to control execution. */
    enum iteration_state {
        ITER_CONTINUE = 0,  // Keep iterating
        ITER_SKIP_SUBMAP,   // Skip the rest of this submap
        ITER_SKIP_ZLEVEL,   // Skip the rest of this z-level
        ITER_FINISH         // End iteration
    };
    /**
    * Runs a `(tripoint &gp, submap* sm, point &lp) -> void` functor
    * over submaps in the area, getting next submap only when the current one "runs out" rather than every time.
    * @param gp Grid (like `get_submap_at_grid`) coordinate of the submap,
    * @param lp Local (submap) coordinate of currently accessed point.
    * Will silently clip the area to map bounds.
    */
    /*@{*/
    template<typename Functor>
        void function_over( const tripoint &start, const tripoint &end, Functor fun ) const;
    template<typename Functor>
        void function_over( int stx, int sty, int stz, int enx, int eny, int enz, Functor fun ) const;
    /*@}*/

    /**
     * The list of currently loaded submaps. The size of this should not be changed.
     * After calling @ref load or @ref generate, it should only contain non-null pointers.
     * Use @ref getsubmap or @ref setsubmap to access it.
     */
    std::vector<submap*> grid;
    /**
     * This vector contains an entry for each trap type, it has therefor the same size
     * as the @ref traplist vector. Each entry contains a list of all point on the map that
     * contain a trap of that type. The first entry however is always empty as it denotes the
     * tr_null trap.
     */
    std::vector< std::vector<tripoint> > traplocs;
    /**
     * Holds caches for visibility, light, transparency and vehicles
     */
    std::array< std::unique_ptr<level_cache>, OVERMAP_LAYERS > caches;

    // Note: no bounds check
    level_cache &get_cache( const int zlev ) {
        return *caches[zlev + OVERMAP_DEPTH];
    }

  public:
    const level_cache &get_cache_ref( const int zlev ) const {
        return *caches[zlev + OVERMAP_DEPTH];
    }

    void update_visibility_cache( visibility_variables &cache, int zlev );

    // Clips the area to map bounds
    tripoint_range points_in_rectangle( const tripoint &from, const tripoint &to ) const;
    tripoint_range points_in_radius( const tripoint &center, size_t radius, size_t radiusz = 0 ) const;
    level_cache &access_cache( int zlev );
    const level_cache &access_cache( int zlev ) const;
};

std::vector<point> closest_points_first(int radius, point p);
std::vector<point> closest_points_first(int radius,int x,int y);
// Does not build "piles" - does the same as above functions, except in tripoints
std::vector<tripoint> closest_tripoints_first(int radius, const tripoint &p);
bool ter_furn_has_flag( const ter_t &ter, const furn_t &furn, const ter_bitflags flag );
class tinymap : public map
{
friend class editmap;
public:
 tinymap(int mapsize = 2, bool zlevels = false);
};

// Hoisted to header and inlined so the test in tests/shadowcasting_test.cpp can use it.
// Beer–Lambert law says attenuation is going to be equal to
// 1 / (e^al) where a = coefficient of absorption and l = length.
// Factoring out length, we get 1 / (e^((a1*a2*a3*...*an)*l))
// We merge all of the absorption values by taking their cumulative average.
inline float sight_calc( const float &numerator, const float &transparency, const int &distance ) {
    return numerator / (float)exp( transparency * distance );
}
inline bool sight_check( const float &transparency, const float &/*intensity*/ ) {
    return transparency > LIGHT_TRANSPARENCY_SOLID;
}

template<int xx, int xy, int yx, int yy, float(*calc)(const float &, const float &, const int &),
    bool(*check)(const float &, const float &)>
    void castLight( float (&output_cache)[MAPSIZE*SEEX][MAPSIZE*SEEY],
                    const float (&input_array)[MAPSIZE*SEEX][MAPSIZE*SEEY],
                    const int offsetX, const int offsetY, const int offsetDistance,
                    const float numerator = 1.0, const int row = 1,
                    float start = 1.0f, const float end = 0.0f,
                    double cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

#endif

