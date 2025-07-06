#pragma once
#ifndef CATA_SRC_MAP_H
#define CATA_SRC_MAP_H

#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "active_item_cache.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_type_traits.h"
#include "cata_utility.h"
#include "colony.h"
#include "coords_fwd.h"
#include "creature.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "item_stack.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "lru_cache.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "maptile_fwd.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vpart_position.h"

#if defined(TILES)
#include "cata_tiles.h"
#endif

struct scent_block;

namespace catacurses
{
class window;
} // namespace catacurses
class Character;
class Creature;
class basecamp;
class character_id;
class computer;
class field;
class field_entry;
class item_location;
class mapgendata;
class monster;
class relic_procgen_data;
class submap;
class vehicle;
class zone_data;
struct fragment_cloud;
struct partial_con;
struct spawn_data;
struct trap;
template<typename Tripoint>
class tripoint_range;

enum class special_item_type : int;
class npc_template;
class tileray;
class vpart_reference;
struct MonsterGroupResult;
struct mongroup;
struct projectile;
struct veh_collision;

struct wrapped_vehicle {
    tripoint_bub_ms pos;
    vehicle *v;
};

using VehicleList = std::vector<wrapped_vehicle>;
class map;

enum class ter_furn_flag : int;
struct pathfinding_cache;
struct pathfinding_settings;
struct pathfinding_target;
template<typename T>
struct weighted_int_list;
struct field_proc_data;

class PathfindingFlags;

class map_stack : public item_stack
{
    private:
        tripoint_bub_ms location;
        map *myorigin;
    public:
        map_stack( cata::colony<item> *newstack, tripoint_bub_ms newloc, map *neworigin ) :
            item_stack( newstack ), location( newloc ), myorigin( neworigin ) {}
        void insert( map &, const item &newitem ) override;
        void insert( const item &newitem );
        iterator erase( const_iterator it ) override;
        int count_limit() const override {
            return MAX_ITEM_IN_SQUARE;
        }
        units::volume max_volume() const override;
};

struct visibility_variables {
    // Is this struct initialized for current z-level
    bool variables_set = false;
    bool u_sight_impaired = false;
    bool u_is_boomered = false;
    bool visibility_cache_dirty = true;
    // Cached values for map visibility calculations
    int g_light_level = 0;
    int u_clairvoyance = 0;
    float vision_threshold = 0.0f;
    std::optional<field_type_id> clairvoyance_field;
    tripoint_bub_ms last_pos;
};

struct bash_params {
    // Initial strength
    int strength = 0;
    // Make a sound?
    bool silent = false;
    // Essentially infinite bash strength + some
    bool destroy = false;
    // Do we want to bash floor if no furn/wall exists?
    bool bash_floor = false;
    /**
     * Value from 0.0 to 1.0 that affects interpolation between str_min and str_max
     * At 0.0, the bash is against str_min of targeted objects
     * This is required for proper "piercing" bashing, so that one strong hit
     * can destroy a wall and a floor under it rather than only one at a time.
     */
    float roll = 0.0f;
    // Was anything hit?
    bool did_bash = false;
    // Was anything destroyed?
    bool success = false;
    // Did we bash furniture, terrain or vehicle
    bool bashed_solid = false;
    /*
     * Are we bashing this location from above?
     * Used in determining what sort of terrain the location will turn into,
     * since if we bashed from above and destroyed it, it probably shouldn't
     * have a roof either.
    */
    bool bashing_from_above = false;
};

/** Draw parameters used by map::drawsq() and similar methods. */
struct drawsq_params {
    private:
        tripoint_bub_ms view_center = tripoint_bub_ms::invalid;
        ter_str_id ter_override = ter_str_id::NULL_ID();
        furn_str_id furn_override = furn_str_id::NULL_ID();
        bool do_highlight = false;
        bool do_show_items = true;
        bool do_low_light = false;
        bool do_bright_light = false;
        bool do_memorize = false;
        bool do_output = true;

    public:
        drawsq_params() = default;

        /**
         * Highlight the tile. On TILES, draws an overlay; on CURSES, inverts color.
         * Default: false.
         */
        //@{
        constexpr drawsq_params &highlight( bool v ) {
            do_highlight = v;
            return *this;
        }
        constexpr bool highlight() const {
            return do_highlight;
        }
        //@}

        /**
         * Whether to draw items on the tile.
         * Default: true.
         */
        //@{
        constexpr drawsq_params &show_items( bool v ) {
            do_show_items = v;
            return *this;
        }
        constexpr bool show_items() const {
            return do_show_items;
        }
        //@}

        /**
         * Whether tile is low light, and should be drawn with muted color.
         * Default: false.
         */
        //@{
        constexpr drawsq_params &low_light( bool v ) {
            do_low_light = v;
            return *this;
        }
        constexpr bool low_light() const {
            return do_low_light;
        }
        //@}

        /**
         * Whether tile is in bright light. Affects NV overlay, and nothing else.
         * Default: false;
         */
        //@{
        constexpr drawsq_params &bright_light( bool v ) {
            do_bright_light = v;
            return *this;
        }
        constexpr bool bright_light() const {
            return do_bright_light;
        }
        //@}

        /**
         * Whether the tile should be memorized. Used only in map::draw().
         * Default: false.
         */
        //@{
        constexpr drawsq_params &memorize( bool v ) {
            do_memorize = v;
            return *this;
        }
        constexpr bool memorize() const {
            return do_memorize;
        }
        //@}

        /**
         * HACK: Whether the tile should be printed. Used only in map::draw()
         * as a hack for memorizing off-screen tiles.
         * Default: true.
         */
        //@{
        constexpr drawsq_params &output( bool v ) {
            do_output = v;
            return *this;
        }
        constexpr bool output() const {
            return do_output;
        }
        //@}

        /**
         * Set view center.
         * Default: uses avatar's current view center.
         */
        //@{
        constexpr drawsq_params &center( const tripoint_bub_ms &p ) {
            view_center = p;
            return *this;
        }
        constexpr drawsq_params &center_at_avatar() {
            view_center = tripoint_bub_ms::invalid;
            return *this;
        }
        tripoint_bub_ms center() const;
        //@}

        /**
         * Set terrain or furniture override.
         * Default: no override.
         */
        //@{
        drawsq_params &terrain_override( const ter_str_id &id ) {
            ter_override = id;
            return *this;
        }
        drawsq_params &furniture_override( const furn_str_id &id ) {
            furn_override = id;
            return *this;
        }
        const ter_str_id &terrain_override() const;
        const furn_str_id &furniture_override() const;
        //@}
};

struct tile_render_info {
    struct common {
        const tripoint_bub_ms pos;
        // accumulator for 3d tallness of sprites rendered here so far;
        int height_3d = 0;

        common( const tripoint_bub_ms &pos, const int height_3d )
            : pos( pos ), height_3d( height_3d ) {}
    };

    struct vision_effect {
        visibility_type vis;

        explicit vision_effect( const visibility_type vis )
            : vis( vis ) {}
    };

    struct sprite {
        lit_level ll;
        std::array<bool, 5> invisible;

        sprite( const lit_level ll, const std::array<bool, 5> &inv )
            : ll( ll ), invisible( inv ) {}
    };

    common com;
    std::variant<vision_effect, sprite> var;

    tile_render_info( const common &com, const vision_effect &var )
        : com( com ), var( var ) {}

    tile_render_info( const common &com, const sprite &var )
        : com( com ), var( var ) {}
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
 *
 * The map's coverage is 11 * 11 submaps, corresponding to the current submap plus 5 submaps on every side of it,
 * covering all Z levels. 5 submaps means 5 * 12 = 60 map squares, which means the map covers the current reality bubble
 * in all directions for as long as you remain within the current submap (and when you leave the map shifts).
 * The natural relative reference system for the map is the tripoint_bub_ms, as the map covers 132 * 132 tiles
 * (11 * 11 submaps).
 */
class map
{
        friend class teleport;
        friend class editmap;
        friend std::list<item> map_cursor::remove_items_with( const std::function<bool( const item & )> &,
                int );

        //FIXME some field processor use private methods
        friend void field_processor_fd_fire( const tripoint_bub_ms &, field_entry &, field_proc_data & );
        friend void field_processor_spread_gas( const tripoint_bub_ms &, field_entry &, field_proc_data & );
        friend void field_processor_wandering_field( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_fire_vent( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_flame_burst( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );
        friend void field_processor_fd_incendiary( const tripoint_bub_ms &, field_entry &,
                field_proc_data & );

        // for testing
        friend void clear_fields( int zlevel );

    protected:
        map( int mapsize, bool zlev );
    public:
        // Constructors & Initialization
        map() : map( MAPSIZE, true ) { }
        virtual ~map();

        map &operator=( const map & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        map &operator=( map && );

        /**
         * Sets a dirty flag on the a given cache.
         *
         * If this isn't set, it's just assumed that
         * the cache hasn't changed and
         * doesn't need to be updated.
         */
        /*@{*/
        void set_transparency_cache_dirty( int zlev );

        // more granular version of the transparency cache invalidation
        // preferred over map::set_transparency_cache_dirty( const int zlev )
        // p is in local coords ("ms")
        // @param field denotes if change comes from the field
        //      fields are not considered for some caches, such as reachability_caches
        //      so passing field=true allows to skip rebuilding of such caches
        void set_transparency_cache_dirty( const tripoint_bub_ms &p, bool field = false );
        void set_seen_cache_dirty( const tripoint_bub_ms &change_location );

        // invalidates seen cache for the whole zlevel unconditionally
        void set_seen_cache_dirty( int zlevel );
        void set_outside_cache_dirty( int zlev );
        void set_floor_cache_dirty( int zlev );
        void set_pathfinding_cache_dirty( int zlev );
        void set_pathfinding_cache_dirty( const tripoint_bub_ms &p );
        /*@}*/

        void invalidate_map_cache( int zlev );

        // @returns true if map memory decoration should be re/memorized
        bool memory_cache_dec_is_dirty( const tripoint_bub_ms &p ) const;
        // @returns true if map memory terrain should be re/memorized
        bool memory_cache_ter_is_dirty( const tripoint_bub_ms &p ) const;
        // sets whether map memory decoration should be re/memorized
        void memory_cache_dec_set_dirty( const tripoint_bub_ms &p, bool value ) const;
        // sets whether map memory terrain should be re/memorized
        void memory_cache_ter_set_dirty( const tripoint_bub_ms &p, bool value ) const;
        // clears map memory for points occupied by vehicle and marks "dirty" for re-memorizing
        void memory_clear_vehicle_points( const vehicle &veh ) const;

        /**
         * A pre-filter for bresenham LOS.
         * true, if there might be a potential bresenham path between two points.
         * false, if such path definitely not possible.
         */
        bool has_potential_los( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;

        /**
         * Callback invoked when a vehicle has moved.
         */
        void on_vehicle_moved( int smz );

        struct apparent_light_info {
            bool obstructed;
            bool abs_obstructed;
            float apparent_light;
        };
        /** Helper function for light calculation; exposed here for map editor
         */
        static apparent_light_info apparent_light_helper( const level_cache &map_cache,
                const tripoint_bub_ms &p );
        /** Determine the visible light level for a tile, based on light_at
         * for the tile, vision distance, etc
         *
         * @param p The tile on this map to draw.
         * @param cache Currently cached visibility parameters
         */
        lit_level apparent_light_at( const tripoint_bub_ms &p, const visibility_variables &cache ) const;
        visibility_type get_visibility( lit_level ll,
                                        const visibility_variables &cache ) const;

        // See field.cpp
        std::tuple<maptile, maptile, maptile> get_wind_blockers( const int &winddirection,
                const tripoint_bub_ms &pos );

        /** Draw a visible part of the map into `w`.
         *
         * This method uses `g->u.posx()/posy()` for visibility calculations, so it can
         * not be used for anything but the player's viewport. Likewise, only
         * `g->m` and maps with equivalent coordinates can be used, as other maps
         * would have coordinate systems incompatible with `g->u.posx()`
         *
         * @param w Window we are drawing in
         * @param center The coordinate of the center of the viewport, this can
         *               be different from the player coordinate.
         */
        void draw( const catacurses::window &w, const tripoint_bub_ms &center );

        /**
         * Draw the map tile at the given coordinate. Called by `map::draw()`.
         *
         * @param w The window we are drawing in
         * @param p The tile on this map to draw.
         * @param params Draw parameters.
         */
        void drawsq( const catacurses::window &w, const tripoint_bub_ms &p,
                     const drawsq_params &params ) const;

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
         * @param w global coordinates of the submap at grid[0]. This
         * is in submap coordinates.
         * @param update_vehicles If true, add vehicles to the vehicle cache.
         * @param pump_events If true, handle window events during loading. If
         * you set this to true, do ensure that the map is not accessed before
         * this function returns (for example, UIs that draw the map should be
         * disabled).
         */
        void load( const tripoint_abs_sm &w, bool update_vehicles,
                   bool pump_events = false );
        /**
         * Shift the map along the vector s.
         * This is like loading the map with coordinates derived from the current
         * position of the map (@ref abs_sub) plus the shift vector.
         * Note: the map must have been loaded before this can be called.
         */
        void shift( const point_rel_sm &s );
        /**
         * Moves the map vertically to (not by!) newz.
         * Does not actually shift anything, only forces cache updates.
         * In the future, it will either actually shift the map or it will get removed
         *  after 3D migration is complete.
         */
        void vertical_shift( int newz );

        void clear_spawns();
        void clear_traps();

        const_maptile maptile_at( const tripoint_bub_ms &p ) const;
        maptile maptile_at( const tripoint_bub_ms &p );
    private:
        // Versions of the above that don't do bounds checks
        const_maptile maptile_at_internal( const tripoint_bub_ms &p ) const;
        maptile maptile_at_internal( const tripoint_bub_ms &p );
        std::pair<tripoint_bub_ms, maptile> maptile_has_bounds( const tripoint_bub_ms &p,
                bool bounds_checked );
        std::array<std::pair<tripoint_bub_ms, maptile>, 8> get_neighbors( const tripoint_bub_ms &p );
        void spread_gas( field_entry &cur, const tripoint_bub_ms &p, int percent_spread,
                         const time_duration &outdoor_age_speedup, scent_block &sblk,
                         const oter_id &om_ter );
        void create_hot_air( const tripoint_bub_ms &p, int intensity );
        bool gas_can_spread_to( field_entry &cur, const maptile &dst );
        void gas_spread_to( field_entry &cur, maptile &dst, const tripoint_bub_ms &p );
        int burn_body_part( Character &you, field_entry &cur, const bodypart_id &bp, int scale );
    public:

        // Movement and LOS
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
        int move_cost( const tripoint_bub_ms &p, const vehicle *ignored_vehicle = nullptr,
                       bool ignore_fields = false ) const;
        int move_cost( const point_bub_ms &p, const vehicle *ignored_vehicle = nullptr,
                       bool ignore_fields = false ) const {
            return move_cost( tripoint_bub_ms( p, abs_sub.z() ), ignored_vehicle, ignore_fields );
        }
        bool impassable( const tripoint_bub_ms &p ) const;
        bool impassable( const point_bub_ms &p ) const {
            return !passable( p );
        }
        bool passable( const tripoint_bub_ms &p ) const;
        bool passable( const point_bub_ms &p ) const {
            return passable( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Doesn't only check if it's possible to move to the tile as "passable" does, but also
        // that it has a floor or other support, so it's possible to remain there without extra effort,
        // and thus is a reasonable target for normal movement. Note that effortless levitation,
        // swimming, etc. by a particular creature is not checked for.
        bool passable_through( const tripoint_bub_ms &p ) const;

        bool passable_skip_fields( const tripoint_bub_ms &p ) const;
        bool is_wall_adjacent( const tripoint_bub_ms &center ) const;

        bool is_open_air( const tripoint_bub_ms &p ) const;

        bool try_fall( const tripoint_bub_ms &p, Creature *c );

        /**
        * Similar behavior to `move_cost()`, but ignores vehicles.
        */
        int move_cost_ter_furn( const tripoint_bub_ms &p ) const;
        bool impassable_ter_furn( const tripoint_bub_ms &p ) const;
        bool passable_ter_furn( const tripoint_bub_ms &p ) const;

        /**
        * Cost to move out of one tile and into the next.
        *
        * @return The cost in turns to move out of tripoint `from` and into `to`
        */
        int combined_movecost( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                               const vehicle *ignored_vehicle = nullptr,
                               int modifier = 0, bool flying = false, bool via_ramp = false,
                               bool ignore_fields = false ) const;

        /**
         * Returns true if a creature could walk from `from` to `to` in one step.
         * That is, if the tiles are adjacent and either on the same z-level or connected
         * by stairs or (in case of flying monsters) open air with no floors.
         */
        bool valid_move( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                         bool bash = false, bool flying = false, bool via_ramp = false ) const;

        /**
         * Size of map objects at `p` for purposes of ranged combat.
         * Size is in percentage of tile: if 1.0, all attacks going through tile
         * should hit map objects on it, if 0.0 there is nothing to be hit (air/water).
         */
        double ranged_target_size( const tripoint_bub_ms &p ) const;

        // Sees:
        /**
        * Returns whether `F` sees `T` with a view range of `range`.
        */
        bool sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, int range,
                   bool with_fields = true ) const;
    private:
        /**
         * Don't expose the slope adjust outside map functions.
         *
         * @param F Thing doing the seeing
         * @param T Thing being seen
         * @param range Vision range of F
         * @param bresenham_slope Indicates the start offset of Bresenham line used to connect
         * the two points, and may subsequently be used to form a path between them.
         * Set to zero if the function returns false.
        **/
        bool sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, int range, int &bresenham_slope,
                   bool with_fields = true, bool allow_cached = true ) const;
        point sees_cache_key( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;
    public:
        /**
        * Returns coverage of target in relation to the observer. Target is loc2, observer is loc1.
        * First tile from the target is an obstacle, which has the coverage value.
        * If there's no obstacle adjacent to the target - no coverage.
        */
        int obstacle_coverage( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2 ) const;
        int ledge_coverage( const Creature &viewer, const tripoint_bub_ms &target_p ) const;
        int ledge_coverage( const tripoint_bub_ms &viewer_p, const tripoint_bub_ms &target_p,
                            const float &eye_level = 1.0f ) const;
        /**
        * Returns coverage value of the tile.
        */
        int coverage( const tripoint_bub_ms &p ) const;
        /**
         * Check whether there's a direct line of sight between `F` and
         * `T` with the additional movecost restraints.
         *
         * Checks two things:
         * 1. The `sees()` algorithm between `F` and `T`
         * 2. That moving over the line of sight would have a move_cost between
         *    `cost_min` and `cost_max`.
         */
        bool clear_path( const tripoint_bub_ms &f, const tripoint_bub_ms &t, int range,
                         int cost_min, int cost_max ) const;

        /**
         * Populates a vector of points that are reachable within a number of steps from a
         * point. It could be generalized to take advantage of z levels, but would need some
         * additional code to detect whether a valid transition was on a tile.
         *
         * Does the following:
         * 1. Checks if a point is reachable using a flood fill and if it is, adds it to a vector.
         *
         */
        std::vector<tripoint_bub_ms> reachable_flood_steps( const tripoint_bub_ms &f, int range,
                int cost_min, int cost_max ) const;

        /**
         * Iteratively tries Bresenham lines with different biases
         * until it finds a clear line or decides there isn't one.
         * returns the line found, which may be the straight line, but blocked.
         */
        std::vector<tripoint_bub_ms> find_clear_path( const tripoint_bub_ms &source,
                const tripoint_bub_ms &destination ) const;

        /**
         * Check whether the player can access the items located @p. Certain furniture/terrain
         * may prevent that (e.g. a locked safe).
         */
        bool accessible_items( const tripoint_bub_ms &t ) const;
        /**
         * Calculate next search points surrounding the current position.
         * Points closer to the target come first.
         * This method leads to straighter lines and prevents weird looking movements away from the target.
         */
        std::vector<tripoint_bub_ms> get_dir_circle( const tripoint_bub_ms &f,
                const tripoint_bub_ms &t ) const;

        /**
         * Calculate the best path using A*
         *
         * @param f The source location from which to path.
         * @param target The destination to which to path.
         * @param settings Structure describing pathfinding parameters.
         * @param pre_closed Never path through those points. They can still be the source or the destination.
         */
        std::vector<tripoint_bub_ms> route( const tripoint_bub_ms &f, const pathfinding_target &target,
                                            const pathfinding_settings &settings,
        const std::function<bool( const tripoint_bub_ms & )> &avoid = []( const tripoint_bub_ms & ) {
            return false;
        } ) const;

        /**
         * Calculate the best path using A*
         *
         * @param who The creature to find a path for.
         * @param target The destination to which to path.
         */
        std::vector<tripoint_bub_ms> route( const Creature &who, const pathfinding_target &target ) const;

        // Get a straight route from f to t, only along non-rough terrain. Returns an empty vector
        // if that is not possible.
        std::vector<tripoint_bub_ms> straight_route( const tripoint_bub_ms &f,
                const tripoint_bub_ms &t ) const;
    private:
        // Pathfinding cost helper that computes the cost of moving into |p| from |cur|.
        // Includes climbing, bashing and opening doors.
        int cost_to_pass( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                          const pathfinding_settings &settings,
                          PathfindingFlags p_special ) const;
        // Pathfinding cost helper that computes the cost of moving into |p|
        // from |cur| based on perceived danger.
        // Includes moving through traps.
        int cost_to_avoid( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                           const pathfinding_settings &settings,
                           PathfindingFlags p_special ) const;
        // Sum of cost_to_pass and cost_to_avoid.
        int extra_cost( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                        const pathfinding_settings &settings,
                        PathfindingFlags p_special ) const;
    public:

        // Vehicles: Common to 2D and 3D
        VehicleList get_vehicles();
        void add_vehicle_to_cache( vehicle * );
        void clear_vehicle_point_from_cache( vehicle *veh, const tripoint_bub_ms &pt );
        // clears all vehicle level caches
        void clear_vehicle_level_caches();
        void remove_vehicle_from_cache( vehicle *veh, int zmin = -OVERMAP_DEPTH,
                                        int zmax = OVERMAP_HEIGHT );
        void reset_vehicles_sm_pos();
        // clears and build vehicle level caches
        void rebuild_vehicle_level_caches();
        void clear_vehicle_list( int zlev );
        void update_vehicle_list( const submap *to, int zlev );
        //Returns true if vehicle zones are dirty and need to be recached
        bool check_vehicle_zones( int zlev );
        std::vector<zone_data *> get_vehicle_zones( int zlev );
        void register_vehicle_zone( vehicle *, int zlev );
        bool deregister_vehicle_zone( zone_data &zone ) const;
        // returns a list of tripoints which contain parts from moving vehicles within \p max_range
        // distance from \p z position, if any parts are CONTROLS, ENGINE or WHEELS returns a
        // list of tripoints with exclusively such parts instead. Used for monster gun actor targeting.
        std::set<tripoint_bub_ms> get_moving_vehicle_targets( const Creature &z, int max_range );

        // Removes vehicle from map and returns it in unique_ptr
        std::unique_ptr<vehicle> detach_vehicle( vehicle *veh );
        void destroy_vehicle( vehicle *veh );
        // Vehicle movement
        void vehmove();
        // Selects a vehicle to move, returns false if no moving vehicles
        bool vehproceed( VehicleList &vehicle_list );

        // Vehicles
        VehicleList get_vehicles( const tripoint_bub_ms &start, const tripoint_bub_ms &end );
        /**
        * Checks if tile is occupied by vehicle and by which part.
        *
        * @param p Tile to check for vehicle
        */
        optional_vpart_position veh_at( const tripoint_abs_ms &p ) const;
        optional_vpart_position veh_at( const tripoint_bub_ms &p ) const;
        vehicle *veh_at_internal( const tripoint_bub_ms &p, int &part_num );
        const vehicle *veh_at_internal( const tripoint_bub_ms &p, int &part_num ) const;
        // Put player on vehicle at x,y
        void board_vehicle( const tripoint_bub_ms &p, Character *pl );
        // Remove given passenger from given vehicle part.
        // If dead_passenger, then null passenger is acceptable.
        void unboard_vehicle( const vpart_reference &, Character *passenger,
                              bool dead_passenger = false );
        // Remove passenger from vehicle at p.
        void unboard_vehicle( const tripoint_bub_ms &p, bool dead_passenger = false );
        // Change vehicle coordinates and move vehicle's driver along.
        // WARNING: not checking collisions!
        // optionally: include a list of parts to displace instead of the entire vehicle
        bool displace_vehicle( vehicle &veh, const tripoint_rel_ms &dp, bool adjust_pos = true,
                               const std::set<int> &parts_to_move = {} );

        // make sure a vehicle that is split across z-levels is properly supported
        // calls displace_vehicle() and shouldn't be called from displace_vehicle
        void level_vehicle( vehicle &veh );
        // move water under wheels. true if moved
        bool displace_water( const tripoint_bub_ms &dp );

        // Returns the wheel area of the vehicle multiplied by traction of the surface
        // When ignore_movement_modifiers is set to true, it returns the area of the wheels touching the ground
        // TODO: Remove the ugly sinking vehicle hack
        float vehicle_wheel_traction( const vehicle &veh, bool ignore_movement_modifiers = false );

        // Executes vehicle-vehicle collision based on vehicle::collision results
        // Returns impulse of the executed collision
        // If vector contains collisions with vehicles other than veh2, they will be ignored
        float vehicle_vehicle_collision( vehicle &veh, vehicle &veh2,
                                         const std::vector<veh_collision> &collisions );
        // Throws vehicle passengers about the vehicle, possibly out of it
        // Returns change in vehicle orientation due to lost control
        units::angle shake_vehicle( vehicle &veh, int velocity_before, const units::angle &direction );

        // Actually moves the vehicle
        // Unlike displace_vehicle, this one handles collisions
        vehicle *move_vehicle( vehicle &veh, const tripoint_rel_ms &dp, const tileray &facing );

        // Furniture
        void set( const tripoint_bub_ms &p, const ter_id &new_terrain, const furn_id &new_furniture );
        void set( const point_bub_ms &p, const ter_id &new_terrain, const furn_id &new_furniture ) {
            furn_set( p, new_furniture );
            ter_set( p, new_terrain );
        }
        std::string name( const tripoint_bub_ms &p );
        std::string name( const point_bub_ms &p ) {
            return name( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        std::string disp_name( const tripoint_bub_ms &p );
        /**
        * Returns the name of the obstacle at p that might be blocking movement/projectiles/etc.
        * Note that this only accounts for vehicles, terrain, and furniture.
        */
        std::string obstacle_name( const tripoint_bub_ms &p );
        bool has_furn( const tripoint_bub_ms &p ) const;
        bool has_furn( const point_bub_ms &p ) const {
            return has_furn( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        furn_id furn( tripoint_bub_ms p ) const;
        furn_id furn( const point_bub_ms &p ) const {
            return furn( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        /**
        * furn_reset should be true if new_furniture is being set to f_null
        * when the player is grab-moving furniture
        */
        bool furn_set( const tripoint_bub_ms &p, const furn_id &new_furniture,
                       bool furn_reset = false, bool avoid_creatures = false, bool allow_on_open_air = false );
        bool furn_set( const point_bub_ms &p, const furn_id &new_furniture, bool avoid_creatures = false,
                       bool allow_on_open_air = false ) {
            return furn_set( tripoint_bub_ms( p, abs_sub.z() ), new_furniture, false, avoid_creatures,
                             allow_on_open_air );
        }
        void furn_clear( const tripoint_bub_ms &p ) {
            furn_set( p, furn_str_id( "f_clear" ) );
        };
        void furn_clear( const point_bub_ms &p ) {
            furn_clear( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        std::string furnname( const tripoint_bub_ms &p );
        bool can_move_furniture( const tripoint_bub_ms &pos, Character *you = nullptr ) const;

        // Terrain
        ter_id ter( const tripoint_bub_ms &p ) const;
        ter_id ter( const point_bub_ms &p ) const {
            return ter( tripoint_bub_ms( p, abs_sub.z() ) );
        }

        int get_map_damage( const tripoint_bub_ms &p ) const;
        void set_map_damage( const tripoint_bub_ms &p, int dmg );

        // Return a bitfield of the adjacent tiles which connect to the given
        // connect_group.  From least-significant bit the order is south, east,
        // west, north (because that's what cata_tiles expects).
        // Based on a combination of visibility and memory, not simply the true
        // terrain. Additional overrides can be passed in to override terrain
        // at specific positions. This is used to display terrain overview in
        // the map editor.
        uint8_t get_known_connections( const tripoint_bub_ms &p,
                                       const std::bitset<NUM_TERCONN> &connect_group,
                                       const std::map<tripoint_bub_ms, ter_id> &override = {} ) const;
        // as above, but for furniture
        uint8_t get_known_connections_f( const tripoint_bub_ms &p,
                                         const std::bitset<NUM_TERCONN> &connect_group,
                                         const std::map<tripoint_bub_ms, furn_id> &override = {} ) const;

        // Return a bitfield of the adjacent tiles which rotate towards the given
        // connect_group.  From least-significant bit the order is south, east,
        // west, north (because that's what cata_tiles expects).
        // Returns CHAR_MAX if rotate_to_group is 0 (i.e. does not rotate).
        // Based on the true terrain.
        // Additional overrides can be passed in to override terrain
        // at specific positions.
        uint8_t get_known_rotates_to( const tripoint_bub_ms &p,
                                      const std::bitset<NUM_TERCONN> &rotate_to_group,
                                      const std::map<tripoint_bub_ms, ter_id> &override = {} ) const;
        // as above, but for furniture (considers neighbouring terrain and furniture)
        uint8_t get_known_rotates_to_f( const tripoint_bub_ms &p,
                                        const std::bitset<NUM_TERCONN> &rotate_to_group,
                                        const std::map<tripoint_bub_ms, ter_id> &override = {},
                                        const std::map<tripoint_bub_ms, furn_id> &override_f = {} ) const;

        /**
         * Returns the full harvest list, for spawning.
         */
        const harvest_id &get_harvest( const tripoint_bub_ms &p ) const;
        /**
         * Returns names of the items that would be dropped.
         */
        const std::set<std::string> &get_harvest_names( const tripoint_bub_ms &p ) const;
        ter_id get_ter_transforms_into( const tripoint_bub_ms &p ) const;

        bool ter_set( const tripoint_bub_ms &, const ter_id &new_terrain, bool avoid_creatures = false );
        bool ter_set( const point_bub_ms &p, const ter_id &new_terrain, bool avoid_creatures = false ) {
            return ter_set( tripoint_bub_ms( p, abs_sub.z() ), new_terrain, avoid_creatures );
        }

        std::string tername( const tripoint_bub_ms &p ) const;

        // Check for terrain/furniture/field that provide a
        // "fire" item to be used for example when crafting or when
        // a iuse function needs fire.
        bool has_nearby_fire( const tripoint_bub_ms &p, int radius = 1 ) const;
        /**
         * Check whether a table/workbench/vehicle kitchen or other flat
         * surface is nearby that could be used for crafting or eating.
         */
        bool has_nearby_table( const tripoint_bub_ms &p, int radius = 1 ) const;
        /**
         * Check whether a chair or vehicle seat is nearby.
         */
        bool has_nearby_chair( const tripoint_bub_ms &p, int radius = 1 ) const;
        /**
         * Checks whether a specific terrain is nearby.
        */
        bool has_nearby_ter( const tripoint_bub_ms &p, const ter_id &type, int radius = 1 ) const;
        /**
         * Check if creature can see some items at p. Includes:
         * - check for items at this location (has_items(p))
         * - check for SEALED flag (sealed furniture/terrain makes
         * items not visible under any circumstances).
         * - check for CONTAINER flag (makes items only visible when
         * the creature is at p or at an adjacent square).
         */
        bool sees_some_items( const tripoint_bub_ms &p, const Creature &who ) const;
        bool sees_some_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const;
        /**
         * Check if the creature could see items at p if there were
         * any items. This is similar to @ref sees_some_items, but it
         * does not check that there are actually any items.
         */
        bool could_see_items( const tripoint_bub_ms &p, const Creature &who ) const;
        bool could_see_items( const tripoint_bub_ms &p, const tripoint_bub_ms &from ) const;
        /**
         * Checks for existence of items. Faster than i_at(p).empty
         */
        bool has_items( const tripoint_bub_ms &p ) const;

        // Check if a tile with LIQUIDCONT flag only contains liquids
        bool only_liquid_in_liquidcont( const tripoint_bub_ms &p );

        /**
         * Calls the examine function of furniture or terrain at given tile, for given character.
         * Will only examine terrain if furniture had @ref iexamine::none as the examine function.
         */
        void examine( Character &you, const tripoint_bub_ms &pos ) const;

        /**
         * Returns true if point at pos is harvestable right now, with no extra tools.
         */
        bool is_harvestable( const tripoint_bub_ms &pos ) const;

        // Flags
        // Words relevant to terrain (sharp, etc)
        std::string features( const tripoint_bub_ms &p ) const;
        // Checks terrain, furniture and vehicles
        bool has_flag( const std::string &flag, const tripoint_bub_ms &p ) const;
        // True if items can be dropped in this tile
        bool can_put_items( const tripoint_bub_ms &p ) const;
        // True if items can be placed in this tile
        bool can_put_items_ter_furn( const tripoint_bub_ms &p ) const;
        // Checks terrain
        bool has_flag_ter( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_ter( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Checks furniture
        bool has_flag_furn( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_furn( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_furn( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Checks terrain or furniture
        bool has_flag_ter_or_furn( const std::string &flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter_or_furn( const std::string &flag, const point_bub_ms &p ) const {
            return has_flag_ter_or_furn( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Fast "oh hai it's update_scent/lightmap/draw/monmove/self/etc again, what about this one" flag checking
        // Checks terrain, furniture and vehicles
        bool has_flag( ter_furn_flag flag, const tripoint_bub_ms &p ) const;
        bool has_flag( ter_furn_flag flag, const point_bub_ms &p ) const {
            return has_flag( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Checks terrain
        bool has_flag_ter( ter_furn_flag flag, const tripoint_bub_ms &p ) const;
        bool has_flag_ter( ter_furn_flag flag, const point_bub_ms &p ) const {
            return has_flag_ter( flag, tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // Checks furniture
        bool has_flag_furn( ter_furn_flag flag, const tripoint_bub_ms &p ) const;
        // Checks terrain or furniture
        bool has_flag_ter_or_furn( ter_furn_flag flag, const tripoint_bub_ms &p ) const;

        // Bashable
        /** Returns true if there is a bashable vehicle part or the furn/terrain is bashable at p */
        bool is_bashable( const tripoint_bub_ms &p, bool allow_floor = false ) const;
        /** Returns true if the terrain at p is bashable */
        bool is_bashable_ter( const tripoint_bub_ms &p, bool allow_floor = false ) const;
        /** Returns true if the furniture at p is bashable */
        bool is_bashable_furn( const tripoint_bub_ms &p ) const;
        /** Returns true if the furniture or terrain at p is bashable */
        bool is_bashable_ter_furn( const tripoint_bub_ms &p, bool allow_floor = false ) const;
        /** Returns max_str of the furniture or terrain at p */
        int bash_strength( const tripoint_bub_ms &p, bool allow_floor = false ) const;
        /** Returns min_str of the furniture or terrain at p */
        int bash_resistance( const tripoint_bub_ms &p, bool allow_floor = false ) const;
        /** Returns a success rating from -1 to 10 for a given tile based on a set strength, used for AI movement planning
        *  Values roughly correspond to 10% increment chances of success on a given bash, rounded down. -1 means the square is not bashable */
        int bash_rating( int str, const tripoint_bub_ms &p, bool allow_floor = false ) const;

        // Rubble
        /** Generates rubble at the given location, if overwrite is true it just writes on top of what currently exists
         *  floor_type is only used if there is a non-bashable wall at the location or with overwrite = true */
        void make_rubble( const tripoint_bub_ms &p, const furn_id &rubble_type, bool items,
                          const ter_id &floor_type, bool overwrite = false );
        void make_rubble( const tripoint_bub_ms &p, const furn_id &rubble_type, bool items ) {
            make_rubble( p, rubble_type, items, ter_str_id( "t_dirt" ).id(), false );
        }
        void make_rubble( const tripoint_bub_ms &p ) {
            make_rubble( p, furn_str_id( "f_rubble" ), false, ter_str_id( "t_dirt" ).id(), false );
        }

        bool is_outside( const tripoint_bub_ms &p ) const;
        /**
         * Returns whether or not the terrain at the given location can be dived into
         * (by monsters that can swim or are aquatic or non-breathing).
         * @param p The coordinate to look at.
         * @return true if the terrain can be dived into; false if not.
         */
        bool is_divable( const tripoint_bub_ms &p ) const;
        bool is_water_shallow_current( const tripoint_bub_ms &p ) const;

        /** Check if the last terrain is wall in direction NORTH, SOUTH, WEST or EAST
         *  @param no_furn if true, the function will stop and return false
         *  if it encounters a furniture
         *  @param p starting coordinates of check
         *  @param max ending coordinates of check
         *  @param dir Direction of check
         *  @return true if from x to xmax or y to ymax depending on direction
         *  all terrain is floor and the last terrain is a wall */
        bool is_last_ter_wall( bool no_furn, const point_bub_ms &p,
                               const point_bub_ms &max, direction dir ) const;

        /**
         * Checks if there are any tinder flagged items on the tile.
         * @param p tile to check
         */
        bool tinder_at( const tripoint_bub_ms &p );

        /**
         * Checks if there are any flammable items on the tile.
         * @param p tile to check
         * @param threshold Fuel threshold (lower means worse fuels are accepted).
         */
        bool flammable_items_at( const tripoint_bub_ms &p, int threshold = 0 );
        /** Returns true if there is a flammable item or field or the furn/terrain is flammable at p */
        bool is_flammable( const tripoint_bub_ms &p );
        point_bub_ms random_outdoor_tile() const;
        // mapgen

        void draw_line_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                            bool avoid_creature = false );
        void draw_line_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                             bool avoid_creatures = false );
        void draw_fill_background( const ter_id &type );
        void draw_fill_background( ter_id( *f )() );
        void draw_fill_background( const weighted_int_list<ter_id> &f );

        void draw_square_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                              bool avoid_creature = false );
        void draw_square_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                               bool avoid_creatures = false );
        void draw_square_ter( ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2,
                              bool avoid_creatures = false );
        void draw_square_ter( const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
                              const point_bub_ms &p2, bool avoid_creatures = false );
        void draw_rough_circle_ter( const ter_id &type, const point_bub_ms &p, int rad );
        void draw_rough_circle_furn( const furn_id &type, const point_bub_ms &p, int rad );
        void draw_circle_ter( const ter_id &type, const rl_vec2d &p, double rad );
        void draw_circle_ter( const ter_id &type, const point_bub_ms &p, int rad );
        void draw_circle_furn( const furn_id &type, const point_bub_ms &p, int rad );

        void add_corpse( const tripoint_bub_ms &p );

        // Terrain changing functions
        // Change all instances of $from->$to
        void translate( const ter_id &from, const ter_id &to );
        // Change all instances $from->$to within this radius, optionally limited to locations in the same submap.
        // Optionally toggles instances $from->$to & $to->$from
        void translate_radius( const ter_id &from, const ter_id &to, float radi, const tripoint_bub_ms &p,
                               bool same_submap = false, bool toggle_between = false );
        void transform_radius( const ter_furn_transform_id &transform, int radi,
                               const tripoint_abs_ms &p );
        void transform_line( const ter_furn_transform_id &transform, const tripoint_abs_ms &first,
                             const tripoint_abs_ms &second );
        bool close_door( const tripoint_bub_ms &p, bool inside, bool check_only );
        bool open_door( Creature const &u, const tripoint_bub_ms &p, bool inside, bool check_only = false );
        // Destruction
        /** bash a square for a set number of times at set power.  Does not destroy */
        void batter( const tripoint_bub_ms &p, int power, int tries = 1, bool silent = false );
        /** Keeps bashing a square until it can't be bashed anymore */
        void destroy( const tripoint_bub_ms &p, bool silent = false );
        /** Keeps bashing a square until there is no more furniture */
        void destroy_furn( const tripoint_bub_ms &, bool silent = false );
        /** Keeps bashing a square until there is no more vehicle part */
        void destroy_vehicle( const tripoint_bub_ms &, bool silent = false );
        void crush( const tripoint_bub_ms &p );
        double shoot( const tripoint_bub_ms &p, projectile &proj, bool hit_items );
        /** Checks if a square should collapse, returns the X for the one_in(X) collapse chance */
        int collapse_check( const tripoint_bub_ms &p ) const;
        /** Causes a collapse at p, such as from destroying a wall */
        void collapse_at( const tripoint_bub_ms &p, bool silent, bool was_supporting = false,
                          bool destroy_pos = true );
        /** Tries to smash the items at the given tripoint. Used by the explosion code */
        void smash_items( const tripoint_bub_ms &p, int power, const std::string &cause_message );
        /**
         * Returns a pair where first is whether anything was smashed and second is if it was destroyed.
         *
         * @param p Where to bash
         * @param str How hard to bash
         * @param silent Don't produce any sound
         * @param destroy Destroys some otherwise unbashable tiles
         * @param bash_floor Allow bashing the floor and the tile that supports it
         * @param bashing_vehicle Vehicle that should NOT be bashed (because it is doing the bashing)
         */
        bash_params bash( const tripoint_bub_ms &p, int str, bool silent = false,
                          bool destroy = false, bool bash_floor = false,
                          const vehicle *bashing_vehicle = nullptr,
                          bool repair_missing_ground = true );

        // Effects of attacks/items
        bool hit_with_acid( const tripoint_bub_ms &p );
        bool hit_with_fire( const tripoint_bub_ms &p );

        /**
         * Returns true if there is furniture for which filter returns true in a 1 tile radius of p.
         * Pass return_true<furn_t> to detect all adjacent furniture.
         * @param p the location to check at
         * @param filter what to filter the furniture by.
         */
        bool has_adjacent_furniture_with( const tripoint_bub_ms &p,
                                          const std::function<bool( const furn_t & )> &filter ) const;
        /**
         * Check for moppable fields/items at this location
         * @param p the location
         * @return true if anything is moppable here, false otherwise.
         */
        bool terrain_moppable( const tripoint_bub_ms &p );
        /**
         * Remove moppable fields/items at this location
         * @param p the location
         * @return true if anything moppable was there, false otherwise.
         */
        bool mop_spills( const tripoint_bub_ms &p );
        /**
         * Moved here from weather.cpp for speed. Decays fire, washable fields and scent.
         * Washable fields are decayed only by 1/3 of the amount fire is.
         */
        void decay_fields_and_scent( const time_duration &amount );

        // Signs
        std::string get_signage( const tripoint_bub_ms &p ) const;
        void set_signage( const tripoint_bub_ms &p, const std::string &message );
        void delete_signage( const tripoint_bub_ms &p );

        // Radiation
        int get_radiation( const tripoint_bub_ms &p ) const;
        void set_radiation( const tripoint_bub_ms &p, int value );
        void set_radiation( const point_bub_ms &p, const int value ) {
            set_radiation( tripoint_bub_ms( p, abs_sub.z() ), value );
        }

        /** Increment the radiation in the given tile by the given delta
        *  (decrement it if delta is negative)
        */
        void adjust_radiation( const tripoint_bub_ms &p, int delta );
        void adjust_radiation( const point_bub_ms &p, const int delta ) {
            adjust_radiation( tripoint_bub_ms( p, abs_sub.z() ), delta );
        }

        // Temperature modifier for submap
        units::temperature_delta get_temperature_mod( const tripoint_bub_ms &p ) const;
        // Set temperature modifier for all four submap quadrants
        void set_temperature_mod( const tripoint_bub_ms &p, units::temperature_delta temperature_mod );
        void set_temperature_mod( const point_bub_ms &p, units::temperature_delta new_temperature_mod ) {
            set_temperature_mod( tripoint_bub_ms( p, abs_sub.z() ), new_temperature_mod );
        }

        // Returns points for all submaps with inconsistent state relative to
        // the list in map.  Used in tests.
        void check_submap_active_item_consistency();
        // Accessor that returns a wrapped reference to an item stack for safe modification.
        map_stack i_at( const tripoint_bub_ms &p );
        map_stack i_at( const point_bub_ms &p ) {
            return i_at( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        item liquid_from( const tripoint_bub_ms &p ) const;
        void i_clear( const tripoint_bub_ms &p );
        void i_clear( const point_bub_ms &p ) {
            i_clear( tripoint_bub_ms( p, abs_sub.z() ) );
        }
        // i_rem() methods that return values act like container::erase(),
        // returning an iterator to the next item after removal.
        map_stack::iterator i_rem( const tripoint_bub_ms &p, const map_stack::const_iterator &it );
        void i_rem( const tripoint_bub_ms &p, item *it );
        void spawn_artifact( const tripoint_bub_ms &p, const relic_procgen_id &id, int max_attributes = 5,
                             int power_level = 1000, int max_negative_power = -2000, bool is_resonant = false );
        void spawn_item( const tripoint_bub_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" );
        void spawn_item( const point_bub_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" ) {
            spawn_item( tripoint_bub_ms( p, abs_sub.z() ), type_id, quantity, charges, birthday, damlevel,
                        flags,
                        variant, faction );
        }

        units::volume max_volume( const tripoint_bub_ms &p );
        units::volume free_volume( const tripoint_bub_ms &p );
        units::volume stored_volume( const tripoint_bub_ms &p );

        /**
         *  Adds an item to map tile or stacks charges
         *  @param pos Where to add item
         *  @param obj Item to add
         *  @param copies_remaining Number of identical copies of the item to create
         *  @param overflow if destination is full attempt to drop on adjacent tiles
         *  @return reference to dropped (and possibly stacked) item or null item on failure
         *  @warning function is relatively expensive and meant for user initiated actions, not mapgen
         */
        item_location add_item_or_charges_ret_loc( const tripoint_bub_ms &pos, item obj,
                bool overflow = true );
        item &add_item_or_charges( const tripoint_bub_ms &pos, item obj, bool overflow = true );
        item &add_item_or_charges( const tripoint_bub_ms &pos, item obj, int &copies_remaining,
                                   bool overflow = true );

        /**
         * Gets spawn_rate value for item category of 'itm'.
         * If spawn_rate is more than or equal to 1.0, it will use roll_remainder on it.
        */
        float item_category_spawn_rate( const item &itm );

        /**
         * Place an item on the map, despite the parameter name, this is not necessarily a new item.
         * WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
         * map::add_item_or_charges
         *
         * @returns The item that got added, or nulitem.
         */
        item &add_item( const tripoint_bub_ms &p, item new_item, int copies );
        item &add_item( const tripoint_bub_ms &p, item new_item );

        /**
         * Update an item's active status, for example when adding
         * hot or perishable liquid to a container.
         */
        void make_active( item_location &loc );
        void make_active( tripoint_abs_sm const &loc );

        /**
         * Update luminosity before and after item's transformation
         */
        void update_lum( item_location &loc, bool add );

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
        std::list<item> use_amount_square( const tripoint_bub_ms &p, const itype_id &type, int &quantity,
                                           const std::function<bool( const item & )> &filter = return_true<item> );
        std::list<item> use_amount( const tripoint_bub_ms &origin, int range, const itype_id &type,
                                    int &quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        std::list<item> use_amount( const std::vector<tripoint_bub_ms> &reachable_pts, const itype_id &type,
                                    int &quantity,
                                    const std::function<bool( const item & )> &filter = return_true<item>, bool select_ind = false );
        std::list<item> use_charges( const tripoint_bub_ms &origin, int range, const itype_id &type,
                                     int &quantity, const std::function<bool( const item & )> &filter = return_true<item>,
                                     basecamp *bcp = nullptr, bool in_tools = false );
        std::list<item> use_charges( const std::vector<tripoint_bub_ms> &reachable_pts,
                                     const itype_id &type,
                                     int &quantity,
                                     const std::function<bool( const item & )> &filter = return_true<item>,
                                     basecamp *bcp = nullptr, bool in_tools = false );

        /** Find items located at point p (on map or in vehicles) that pass the filter */
        std::list<item_location> items_with( const tripoint_bub_ms &p,
                                             const std::function<bool( const item & )> &filter );

        /**
        * Consume UPS from UPS sources from area centered at origin.
        * @param origin the position of player
        * @param range how far the UPS can be used from
        * @param qty amount of energy to consume. Is rounded down to kJ precision. Do not use negative values.
        * @return Actual amount of energy consumed
        */
        units::energy consume_ups( const std::vector<tripoint_bub_ms> &reachable_pts, units::energy qty );
        units::energy consume_ups( const tripoint_bub_ms &origin, int range, units::energy qty );

        /*@}*/
        std::list<std::pair<tripoint_bub_ms, item *> > get_rc_items( const tripoint_bub_ms &p = { -1, -1, -1 } );

        /**
        * Place items from item group in the rectangle f - t. Several items may be spawned
        * on different places. Several items may spawn at once (at one place) when the item group says
        * so (uses @ref item_group::items_from which may return several items at once).
        * @param gp id of item group to spawn from
        * @param chance Chance for more items. A chance of 100 creates 1 item all the time, otherwise
        * it's the chance that more items will be created (place items until the random roll with that
        * chance fails). The chance is used for the first item as well, so it may not spawn an item at
        * all. Values <= 0 or > 100 are invalid.
        * @param p1 One corner of rectangle in which to spawn items
        * @param p2 Second corner of rectangle in which to spawn items
        * @param ongrass If false the items won't spawn on flat terrain (grass, floor, ...).
        * @param turn The birthday that the created items shall have.
        * @param magazine percentage chance item will contain the default magazine
        * @param ammo percentage chance item will be filled with default ammo
        * @return vector containing all placed items
        */
        std::vector<item *> place_items(
            const item_group_id &group_id, int chance,
            const tripoint_bub_ms &p1, const tripoint_bub_ms &p2,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" );
        std::vector<item *> place_items(
            const item_group_id &group_id, int chance, const point_bub_ms &p1, const point_bub_ms &p2,
            const int z_level,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" ) {
            return place_items( group_id, chance, tripoint_bub_ms( p1, z_level ),
                                tripoint_bub_ms( p2, z_level ), ongrass, turn, magazine, ammo, faction );
        }
        /**
        * Place items from an item group at p. Places as much items as the item group says.
        * (Most item groups are distributions and will only create one item.)
        * @param gp item group to spawn
        * @param p Destination of items
        * @param turn The birthday that the created items shall have.
        * @return Vector of pointers to placed items (can be empty, but no nulls).
        */
        std::vector<item *> put_items_from_loc(
            const item_group_id &group_id, const tripoint_bub_ms &p,
            const time_point &turn = calendar::start_of_cataclysm );

        // Places a list of items, or nothing if the list is empty.
        std::vector<item *> spawn_items( const tripoint_bub_ms &p,
                                         const std::vector<item> &new_items );

        void create_anomaly( const tripoint_bub_ms &p, artifact_natural_property prop,
                             bool create_rubble = true );
        void create_anomaly( const point_bub_ms &cp, artifact_natural_property prop,
                             bool create_rubble = true ) {
            create_anomaly( tripoint_bub_ms( cp, abs_sub.z() ), prop, create_rubble );
        }

        // Partial construction functions
        void partial_con_set( const tripoint_bub_ms &p, const partial_con &con );
        void partial_con_remove( const tripoint_bub_ms &p );
        partial_con *partial_con_at( const tripoint_bub_ms &p );
        // Traps
        void trap_set( const tripoint_bub_ms &p, const trap_id &type );

        const trap &tr_at( const tripoint_abs_ms &p ) const;
        const trap &tr_at( const tripoint_bub_ms &p ) const;
        /// See @ref trap::can_see, which is called for the trap here.
        bool can_see_trap_at( const tripoint_bub_ms &p, const Character &c ) const;

        void remove_trap( const tripoint_bub_ms &p );
        const std::vector<tripoint_bub_ms> &get_furn_field_locations() const;
        const std::vector<tripoint_bub_ms> &get_ter_field_locations() const;
        const std::vector<tripoint_bub_ms> &trap_locations( const trap_id &type ) const;

        /**
         * Handles activating a trap. It includes checks for avoiding the trap
         * (which also makes it visible).
         * This functions assumes the character is either on top of the trap,
         * or adjacent to it.
         */

        void maybe_trigger_trap( const tripoint_bub_ms &pos, Creature &c, bool may_avoid ) const;
        // Handles triggering a proximity trap. Similar but subtly different.
        void maybe_trigger_prox_trap( const tripoint_bub_ms &pos, Creature &c, bool may_avoid ) const;

        // Spawns byproducts from items destroyed in fire.
        void create_burnproducts( const tripoint_bub_ms &p, const item &fuel,
                                  const units::mass &burned_mass );
        // See fields.cpp
        void process_fields();
        void process_fields_in_submap( submap *current_submap, const tripoint_bub_sm &submap_pos );
        /**
         * Apply field effects to the creature when it's on a square with fields.
         */
        void creature_in_field( Creature &critter );
        /**
         * Apply trap effects to the creature, similar to @ref creature_in_field.
         * If there is no trap at the creatures location, nothing is done.
         * If the creature can avoid the trap, nothing is done as well.
         * Otherwise the trap is triggered.
         * @param critter Creature that just got trapped
         * @param may_avoid If true, the creature tries to avoid the trap
         * (@ref Creature::avoid_trap). If false, the trap is always triggered.
         */
        void creature_on_trap( Creature &critter, bool may_avoid = true ) const;
        // Field functions
        /**
         * Get the fields that are here. This is for querying and looking at it only,
         * one can not change the fields.
         * @param p The local map coordinates, if out of bounds, returns an empty field.
         */
        const field &field_at( const tripoint_bub_ms &p ) const;
        /**
         * Gets fields that are here. Both for querying and edition.
         */
        field &field_at( const tripoint_bub_ms &p );
        /**
         * Get the age of a field entry (@ref field_entry::age), if there is no
         * field of that type, returns `-1_turns`.
         */
        time_duration get_field_age( const tripoint_bub_ms &p, const field_type_id &type ) const;
        /**
         * Get the intensity of a field entry (@ref field_entry::intensity),
         * if there is no field of that type, returns 0.
         */
        int get_field_intensity( const tripoint_bub_ms &p, const field_type_id &type ) const;
        /**
         * Increment/decrement age of field entry at point.
         * @return resulting age or `-1_turns` if not present (does *not* create a new field).
         */
        time_duration mod_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                     const time_duration &offset );
        /**
         * Increment/decrement intensity of field entry at point, creating if not present,
         * removing if intensity becomes 0.
         * @return resulting intensity, or 0 for not present (either removed or not created at all).
         */
        int mod_field_intensity( const tripoint_bub_ms &p, const field_type_id &type, int offset );
        /**
         * Set age of field entry at point.
         * @param p Location of field
         * @param type ID of field
         * @param age New age of specified field
         * @param isoffset If true, the given age value is added to the existing value,
         * if false, the existing age is ignored and overridden.
         * @return resulting age or `-1_turns` if not present (does *not* create a new field).
         */
        time_duration set_field_age( const tripoint_bub_ms &p, const field_type_id &type,
                                     const time_duration &age, bool isoffset = false );
        /**
         * Set intensity of field entry at point, creating if not present,
         * removing if intensity becomes 0.
         * @param p Location of field
         * @param type ID of field
         * @param new_intensity New intensity of field
         * @param isoffset If true, the given new_intensity value is added to the existing value,
         * if false, the existing intensity is ignored and overridden.
         * @return resulting intensity, or 0 for not present (either removed or not created at all).
         */
        int set_field_intensity( const tripoint_bub_ms &p, const field_type_id &type, int new_intensity,
                                 bool isoffset = false );

        // returns true, if there **might** be a field at `p`
        // if false, there's no fields at `p`
        bool has_field_at( const tripoint_bub_ms &p, bool check_bounds = true ) const;
        bool has_field_at( const tripoint_bub_ms &p, const field_type_id &type ) const;

        // returns the a field entry that is impassable at the given point if it exists
        std::optional<field_entry> get_impassable_field_at( const tripoint_bub_ms &p );
        std::vector<field_type_id> get_impassable_field_type_ids_at( const tripoint_bub_ms &p );

        /**
         * Get field of specific type at point.
         * @return NULL if there is no such field entry at that place.
         */
        field_entry *get_field( const tripoint_bub_ms &p, const field_type_id &type );
        const field_entry *get_field( const tripoint_bub_ms &p, const field_type_id &type ) const;
        bool dangerous_field_at( const tripoint_bub_ms &p );
        bool impassable_field_at( const tripoint_bub_ms &p );

        // Check if player can move on top of it during mopping zone activity
        bool mopsafe_field_at( const tripoint_bub_ms &p );

        /**
         * Add field entry at point, or set intensity if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */
        bool add_field(
            const tripoint_bub_ms &p, const field_type_id &type_id, int intensity = INT_MAX,
            const time_duration &age = 0_turns, bool hit_player = true );
        /**
         * Remove field entry at xy, ignored if the field entry is not present.
         */
        void remove_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove );
        /**
        * Deletes the field without regard for any possible need for cleanup. Circumvents
        * the normal method of marking the field for removal which is then taken place during
        * a regular cleanup. Intended for use with fields that have a display function only,
        * with the effect of using it for other kinds of fields being untested.
        */
        void delete_field( const tripoint_bub_ms &p, const field_type_id &field_to_remove );
        /**
         * Remove all field entries at location.
         */
        void clear_fields( const tripoint_bub_ms &p );

        /**
         * Get applicable fd_electricity field type for a given point
         */
        const field_type_str_id &get_applicable_electricity_field( const tripoint_bub_ms &p ) const;

    private:
        // Is called when field intensity is changed.
        // Invalidates relevan map caches, such as transparency cache.
        void on_field_modified( const tripoint_bub_ms &p, const field_type &fd_type );

        template<typename Map>
        static cata::copy_const<Map, field_entry> *get_field_helper(
            Map &m, const tripoint_bub_ms &p, const field_type_id &type );

        std::pair<item *, tripoint_bub_ms> _add_item_or_charges( const tripoint_bub_ms &pos, item obj,
                int &copies_remaining, bool overflow = true );
    public:

        // Splatters of various kind
        void add_splatter( const field_type_id &type, const tripoint_bub_ms &where, int intensity = 1 );
        void add_splatter_trail( const field_type_id &type, const tripoint_bub_ms &from,
                                 const tripoint_bub_ms &to );
        void add_splash( const field_type_id &type, const tripoint_bub_ms &center, int radius,
                         int intensity );

        void propagate_field( const tripoint_bub_ms &center, const field_type_id &type,
                              int amount, int max_intensity = 0 );

        /**
         * Runs one cycle of emission @ref src which **may** result in propagation of fields
         * @param pos Location of emission
         * @param src Id of object producing the emission
         * @param mul Multiplies the chance and possibly qty (if `chance*mul > 100`) of the emission
         */
        void emit_field( const tripoint_bub_ms &pos, const emit_id &src, float mul = 1.0f );

        // Scent propagation helpers
        /**
         * Build the map of scent-resistant tiles.
         * Should be way faster than if done in `game.cpp` using public map functions.
         */
        void scent_blockers( std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &blocks_scent,
                             std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &reduces_scent,
                             const point_bub_ms &min, const point_bub_ms &max );

        // Computers
        computer *computer_at( const tripoint_bub_ms &p );
        computer *add_computer( const tripoint_bub_ms &p, const std::string &name, int security );

        // Camps
        void add_camp( const tripoint_abs_omt &omt_pos, const std::string &name,
                       bool need_validate = true );
        void remove_submap_camp( const tripoint_bub_ms & );
        basecamp hoist_submap_camp( const tripoint_bub_ms &p );
        bool point_within_camp( const tripoint_abs_ms &point_check ) const;
        // Graffiti
        bool has_graffiti_at( const tripoint_bub_ms &p ) const;
        const std::string &graffiti_at( const tripoint_bub_ms &p ) const;
        void set_graffiti( const tripoint_bub_ms &p, const std::string &contents );
        void delete_graffiti( const tripoint_bub_ms &p );

        // Climbing
        /**
         * Checks 3x3 block centered on p for terrain to climb.
         * @return Difficulty of climbing check from point p.
         */
        int climb_difficulty( const tripoint_bub_ms &p ) const;

        // Support (of weight, structures etc.)
    private:
        // Tiles whose ability to support things was removed in the last turn
        std::set<tripoint_bub_ms> support_cache_dirty;
        // Checks if the tile is supported and adds it to support_cache_dirty if it isn't
        void support_dirty( const tripoint_bub_ms &p );
    public:

        // Returns true if terrain at p has NO flag ter_furn_flag::TFLAG_NO_FLOOR
        // and ter_furn_flag::TFLAG_NO_FLOOR_WATER,
        // if we're not in z-levels mode or if we're at lowest level
        bool has_floor( const tripoint_bub_ms &p ) const;
        bool has_floor_or_water( const tripoint_bub_ms &p ) const;
        /** Does this tile support vehicles and furniture above it */
        bool supports_above( const tripoint_bub_ms &p ) const;
        bool has_floor_or_support( const tripoint_bub_ms &p ) const;
        bool has_vehicle_floor( const tripoint_bub_ms &p ) const;
    private:
        /**
         * Handles map objects of given type falling down.
         */
        /*@{*/
        void drop_everything( const tripoint_bub_ms &p );
        void drop_furniture( const tripoint_bub_ms &p );
        void drop_items( const tripoint_bub_ms &p );
        void drop_vehicle( const tripoint_bub_ms &p );
        void drop_fields( const tripoint_bub_ms &p );
        void drop_creature( const tripoint_bub_ms &p );
        /*@}*/
    public:
        /**
         * Invoked @ref drop_everything on cached dirty tiles.
         */
        void process_falling();

        bool is_cornerfloor( const tripoint_bub_ms &p ) const;

        // mapgen.cpp functions
        // The code relies on the submap coordinate falling on omt boundaries, so taking a
        // tripoint_abs_omt coordinate guarantees this will be fulfilled.
        void generate( const tripoint_abs_omt &p, const time_point &when, bool save_results,
                       bool run_post_process = true );
        // Used when contents has been generated by 'generate' with save_results = false to dispose of
        // submaps that aren't present in the map buffer. This is done to avoid memory leaks.
        void delete_unmerged_submaps();
        void place_spawns( const mongroup_id &group, int chance,
                           const point_bub_ms &p1, const point_bub_ms &p2, int z_level, float density,
                           bool individual = false, bool friendly = false,
                           const std::optional<std::string> &name = std::nullopt,
                           int mission_id = -1 );
        void place_gas_pump( const tripoint_bub_ms &p, int charges, const itype_id &fuel_type );
        void place_gas_pump( const tripoint_bub_ms &p, int charges );
        // 6 liters at 250 ml per charge
        void place_toilet( const tripoint_bub_ms &p, int charges = 6 * 4 );
        void place_vending( const tripoint_bub_ms &p, const item_group_id &type, bool reinforced = false,
                            bool lootable = false, bool powered = false, bool networked = false );
        // places an NPC, if static NPCs are enabled or if force is true
        character_id place_npc( const point_bub_ms &p, const string_id<npc_template> &type );
        void apply_faction_ownership( const point_bub_ms &p1, const point_bub_ms &p2,
                                      const faction_id &id );
        void add_spawn( const mtype_id &type, int count, const tripoint_bub_ms &p,
                        bool friendly = false, int faction_id = -1, int mission_id = -1,
                        const std::optional<std::string> &name = std::nullopt );
        void add_spawn( const mtype_id &type, int count, const tripoint_bub_ms &p, bool friendly,
                        int faction_id, int mission_id, const std::optional<std::string> &name,
                        const spawn_data &data );
        void add_spawn( const MonsterGroupResult &spawn_details, const tripoint_bub_ms &p );
        void do_vehicle_caching( int z );
        // Note: in 3D mode, will actually build caches on ALL z-levels
        void build_map_cache( int zlev, bool skip_lightmap = false );
        // Unlike the other caches, this populates a supplied cache instead of an internal cache.
        void build_obstacle_cache(
            const tripoint_bub_ms &start, const tripoint_bub_ms &end,
            cata::mdarray<fragment_cloud, point_bub_ms> &obstacle_cache );

        // this function spawns a vehicle from a blueprint \p type and adds it to this map
        // @param type              vehicle prototype to use for constructing the vehicle
        // @param p                 position to spawn at, in tripoint_bub_ms coords
        // @param dir               vehicle facing to spawn at
        // @param init_veh_fuel     value of 0 spawns the vehicle with no fuel
        //                          value in range of [1..99] spawns exact percentage of fuel
        //                          value of 100 spawns full tanks
        //                          a negative value will spawns some fuel left
        //                          can be overriden by VEHICLE_FUEL_AT_SPAWN EXTERNAL_OPTION
        // @param init_veh_status   value of -1 spawns lightly damaged vehicle
        //                          value of 0 spawns fully intact vehicle
        //                          value of 1 spawns with destroyed seats / controls / tanks / tires / engines
        //                          value of 2 spawns fully intact vehicle with no faults or security
        //                          can be overriden by VEHICLE_STATUS_AT_SPAWN EXTERNAL_OPTION
        // @param merge_wrecks      if true and vehicle overlaps another then both turn into wrecks
        //                          if false and vehicle will overlap aborts and returns nullptr
        vehicle *add_vehicle( const vproto_id &type, const tripoint_bub_ms &p, const units::angle &dir,
                              int init_veh_fuel = -1, int init_veh_status = -1, bool merge_wrecks = true,
                              bool force_status = false );

        // Light/transparency
        float light_transparency( const tripoint_bub_ms &p ) const;
        // Assumes 0,0 is light map center
        lit_level light_at( const tripoint_bub_ms &p ) const;
        // Raw values for tilesets
        float ambient_light_at( const tripoint_bub_ms &p ) const;
        /**
         * Returns whether the tile at `p` is transparent(you can look past it).
         */
        bool is_transparent( const tripoint_bub_ms &p ) const;
        bool is_transparent_wo_fields( const tripoint_bub_ms &p ) const;
        // End of light/transparency

        /**
         * Whether the player character (g->u) can see the given square (local map coordinates).
         * This only checks the transparency of the path to the target, the light level is not
         * checked.
         * @param t Target point to look at
         * @param max_range All squares that are further away than this are invisible.
         * Ignored if smaller than 0.
         */
        bool pl_sees( const tripoint_bub_ms &t, int max_range ) const;

        std::set<vehicle *> dirty_vehicle_list;

        /** return @ref abs_sub */
        tripoint_abs_sm get_abs_sub() const;
        /**
         * Translates local (to this map) coordinates of a square to global absolute coordinates.
         * Coordinates is in the system that is used by the ter/furn/i_at functions.
         * Output is in the same scale, but in global system.
         */
        tripoint_abs_ms get_abs( const tripoint_bub_ms &p ) const;
        /**
         * Inverse of @ref get_abs
         */
        tripoint_bub_ms get_bub( const tripoint_abs_ms &p ) const;
        point_bub_ms get_bub( const point_abs_ms &p ) const {
            return get_bub( tripoint_abs_ms( p, abs_sub.z() ) ).xy();
        }
        bool inbounds( const tripoint_bub_ms &p ) const;
        bool inbounds( const tripoint_abs_ms &p ) const;
        bool inbounds( const tripoint_abs_sm &p ) const {
            return inbounds( project_to<coords::omt>( p ) );
        }
        bool inbounds( const tripoint_abs_omt &p ) const;
        bool inbounds( const point_bub_ms &p ) const {
            return inbounds( tripoint_bub_ms( p, 0 ) );
        }

        bool inbounds_z( const int z ) const {
            return z >= -OVERMAP_DEPTH && z <= OVERMAP_HEIGHT;
        }

        /** Clips the coordinates of p to fit the map bounds */
        void clip_to_bounds( tripoint_bub_ms &p ) const;
        void clip_to_bounds( int &x, int &y ) const;
        void clip_to_bounds( int &x, int &y, int &z ) const;

        int getmapsize() const {
            return my_MAPSIZE;
        }

        // Not protected/private for mapgen.cpp and mapgen_functions.cpp access
        // Rotates the current map 90*turns degrees clockwise
        // Useful for houses, shops, etc
        // @param turns number of 90 clockwise turns to make
        // Note that this operation actually only works on tinymap and smallmap.
        void rotate( int turns );

        // Not protected/private for mapgen.cpp access
        // Mirrors the current map horizontally and/or vertically (both is technically
        // equivalent to a 180 degree rotation, while neither is a null operation).
        // Intended to base recipe usage to allow recipes to specify the mirroring of
        // a common blueprint. Note that the operation is NOT safe to use for purposes
        // other than mirroring the map, place assets on it, and then mirroring it
        // back (so the asset modification takes place in between calls to this
        // operation). This allows us to skip the shuffling of NPCs and zones
        // that the rotate operation above has to deal with.
        // @param mirror_horizontal causes horizontal mirroring of the map
        // @param mirror_vertical causes vertical mirroring of the map
        void mirror( bool mirror_horizontal, bool mirror_vertical );

        // Monster spawning:
        /**
         * Spawn monsters from submap spawn points and from the overmap.
         * @param ignore_sight If true, monsters may spawn in the view of the player
         * character (useful when the whole map has been loaded instead, e.g.
         * when starting a new game, or after teleportation or after moving vertically).
         * If false, monsters are not spawned in view of player character.
         */
        void spawn_monsters( bool ignore_sight, bool spawn_nonlocal = false );

        /**
        * Checks to see if the corpse that is rotting away generates items when it does.
        * @param it item that is spawning creatures
        * @param pnt The point on this map where the item is and where bones/etc will be
        */
        void handle_decayed_corpse( const item &it, const tripoint_abs_ms &pnt );

        /**
        * Checks to see if the item that is rotting away generates a creature when it does.
        * @param item item that is spawning creatures
        * @param p The point on this map where the item is and creature will be
        */
        void rotten_item_spawn( const item &item, const tripoint_bub_ms &p );
    private:
        // Helper #1 - spawns monsters on one submap
        void spawn_monsters_submap( const tripoint_rel_sm &gp, bool ignore_sight,
                                    bool spawn_nonlocal = false );
        // Helper #2 - spawns monsters on one submap and from one group on this submap
        void spawn_monsters_submap_group( const tripoint_rel_sm &gp, mongroup &group,
                                          bool ignore_sight );

    protected:
        void saven( const tripoint_bub_sm &grid );
        void loadn( const point_bub_sm &grid, bool update_vehicles );
        /**
         * Fast forward a submap that has just been loading into this map.
         * This is used to rot and remove rotten items, grow plants, fill funnels etc.
         */
        void actualize( const tripoint_rel_sm &grid );
        /**
         * Hacks in missing tree tops. It CAN be done in other ways, but it would be a lot of work
         * to get regional translation code to deal with chunks instead of terrain, and then use these
         * chunks everywhere instead of tree terrain tokens. Maybe some day...
         */
        void add_tree_tops( const tripoint_rel_sm &grid );
        /**
         * Try to fill funnel based items here. Simulates rain from @p since till now.
         * @param p The location in this map where to fill funnels.
         */
        void fill_funnels( const tripoint_bub_ms &p, const time_point &since );
        /**
         * Try to grow a harvestable plant to the next stage(s).
         */
        void grow_plant( const tripoint_bub_ms &p );
        /**
         * Try to grow fruits on static plants (not planted by the player)
         * @param p Place to restock
         * @param time_since_last_actualize Time since this function has been
         * called the last time.
         */
        void restock_fruits( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );
        /**
         * Produce sap on tapped maple trees
         * @param p Location of tapped tree
         * @param time_since_last_actualize Time since this function has been
         * called the last time.
         */
        void produce_sap( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );
    public:
        /**
        * Removes the tree at 'p' and produces a trunk_yield length line of trunks in the 'dir'
        * direction from 'p', leaving a stump behind at 'p'.
        */
        void cut_down_tree( tripoint_bub_ms p, point_rel_ms dir );
    protected:
        /**
         * Radiation-related plant (and fungus?) death.
         */
        void rad_scorch( const tripoint_bub_ms &p, const time_duration &time_since_last_actualize );
        void decay_cosmetic_fields( const tripoint_bub_ms &p,
                                    const time_duration &time_since_last_actualize );

        void player_in_field( Character &you );
        void monster_in_field( monster &z );
        /**
         * As part of the map shifting, this shifts the trap locations stored in @ref traplocs.
         * @param shift The amount shifting in submap, the same as go into @ref shift.
         */
        void shift_traps( const point_rel_sm &shift );

        void copy_grid( const tripoint_rel_sm &to, const tripoint_rel_sm &from );
        void draw_map( mapgendata &dat );

        void draw_lab( mapgendata &dat );

        // Builds a transparency cache and returns true if the cache was invalidated.
        // Used to determine if seen cache should be rebuilt.
        bool build_transparency_cache( int zlev );
        bool build_vision_transparency_cache( int zlev );
        // fills lm with sunlight. pzlev is current player's zlevel
        void build_sunlight_cache( int pzlev );
    public:
        void build_outside_cache( int zlev );
        // Get a bitmap indicating which layers are potentially visible from the target layer.
        std::bitset<OVERMAP_LAYERS> get_inter_level_visibility( int origin_zlevel )const ;
        // Builds a floor cache and returns true if the cache was invalidated.
        // Used to determine if seen cache should be rebuilt.
        bool build_floor_cache( int zlev );
        // We want this visible in `game`, because we want it built earlier in the turn than the rest
        void build_floor_caches();
        void seen_cache_process_ledges( array_of_grids_of<float> &seen_caches,
                                        const array_of_grids_of<const bool> &floor_caches,
                                        const std::optional<tripoint_bub_ms> &override_p ) const;

    protected:
        void generate_lightmap( int zlev );
        void build_seen_cache( const tripoint_bub_ms &origin, int target_z,
                               int extension_range = MAX_VIEW_DISTANCE,
                               bool cumulative = false,
                               bool camera = false, int penalty = 0 );
        void apply_character_light( Character &p );

        int my_MAPSIZE;
        int my_HALF_MAPSIZE;
        bool zlevels;

        /**
         * Absolute coordinates of first submap (get_submap_at(0,0))
         * This is in submap coordinates (see overmapbuffer for explanation).
         * It is set upon:
         * - loading submap at grid[0],
         * - generating submaps (@ref generate)
         * - shifting the map with @ref shift
         */
        tripoint_abs_sm abs_sub;
        // Cached value of project_to<coords::ms>( abs_sub.xy() )
        point_abs_ms abs_ms;
        /**
         * Sets @ref abs_sub, see there. Uses the same coordinate system as @ref abs_sub.
         */
        void set_abs_sub( const tripoint_abs_sm &p );

    private:
        field &get_field( const tripoint_bub_ms &p );

        /**
         * Get the submap pointer with given index in @ref grid, the index must be valid!
         */
        submap *getsubmap( size_t grididx );
        const submap *getsubmap( size_t grididx ) const;
        /**
         * Get the submap pointer containing the specified position within the reality bubble.
         * (p) must be a valid coordinate, check with @ref inbounds.
         */
        submap *unsafe_get_submap_at( const tripoint_bub_ms &p );
        const submap *unsafe_get_submap_at( const tripoint_bub_ms &p ) const;
        submap *get_submap_at( const tripoint_bub_ms &p );
        const submap *get_submap_at( const tripoint_bub_ms &p ) const;
        /**
         * Get the submap pointer containing the specified position within the reality bubble.
         * The same as other get_submap_at, (p) must be valid (@ref inbounds).
         * Also writes the position within the submap to offset_p
         */
        submap *unsafe_get_submap_at( const tripoint_bub_ms p, point_sm_ms &offset_p ) {
            tripoint_bub_sm sm;
            std::tie( sm, offset_p ) = project_remain<coords::sm>( p );
            return unsafe_get_submap_at( p );
        }
        const submap *unsafe_get_submap_at(
            const tripoint_bub_ms p, point_sm_ms &offset_p ) const {
            tripoint_bub_sm sm;
            std::tie( sm, offset_p ) = project_remain<coords::sm>( p );
            return unsafe_get_submap_at( p );
        }
        submap *get_submap_at( const tripoint_bub_ms &p, point_sm_ms &offset_p ) {
            offset_p.x() = p.x() % SEEX;
            offset_p.y() = p.y() % SEEY;
            return get_submap_at( p );
        }
        const submap *get_submap_at( const tripoint_bub_ms &p, point_sm_ms &offset_p ) const {
            offset_p.x() = p.x() % SEEX;
            offset_p.y() = p.y() % SEEY;
            return get_submap_at( p );
        }
        /**
         * Get submap pointer in the grid at given grid coordinates. Grid coordinates must
         * be valid: 0 <= x < my_MAPSIZE, same for y.
         * z must be between -OVERMAP_DEPTH and OVERMAP_HEIGHT
         */
        submap *get_submap_at_grid( const point_rel_sm &gridp ) {
            return getsubmap( get_nonant( gridp ) );
        }
        submap *get_submap_at_grid( const tripoint_rel_sm &gridp );
        const submap *get_submap_at_grid( const tripoint_rel_sm &gridp ) const;
    protected:
        /**
         * Get the index of a submap pointer in the grid given by grid coordinates. The grid
         * coordinates must be valid: 0 <= x < my_MAPSIZE, same for y.
         * Version with z-levels checks for z between -OVERMAP_DEPTH and OVERMAP_HEIGHT
         */
        size_t get_nonant( const tripoint_rel_sm &gridp ) const;
        size_t get_nonant( const point_rel_sm &gridp ) const {
            return get_nonant( tripoint_rel_sm{ gridp.x(), gridp.y(), abs_sub.z()} );
        }
        /**
         * Set the submap pointer in @ref grid at the give index. This is the inverse of
         * @ref getsubmap, any existing pointer is overwritten. The index must be valid.
         * The given submap pointer must not be null.
         */
        void setsubmap( size_t grididx, submap *smap );
    private:
        /** Caclulate the greatest populated zlevel in the loaded submaps and save in the level cache.
         * fills the map::max_populated_zlev and returns it
         * @return max_populated_zlev value
         */
        int calc_max_populated_zlev();
        /**
         * Conditionally invalidates max_pupulated_zlev cache if the submap uniformity change occurs above current
         *  max_pupulated_zlev value
         * @param zlev zlevel where uniformity change occurred
         */
        void invalidate_max_populated_zlev( int zlev );

        /**
         * Internal versions of public functions to avoid checking same variables multiple times.
         * They lack safety checks, because their callers already do those.
         */
        int move_cost_internal( const furn_t &furniture, const ter_t &terrain, const field &field,
                                const vehicle *veh, int vpart ) const;
        int bash_rating_internal( int str, const furn_t &furniture,
                                  const ter_t &terrain, bool allow_floor,
                                  const vehicle *veh, int part ) const;

        /**
         * Internal version of the drawsq. Keeps a cached maptile for less re-getting.
         * Returns false if it has drawn all it should, true if `draw_from_above` should be called after.
         */
        bool draw_maptile( const catacurses::window &w, const tripoint_bub_ms &p,
                           const const_maptile &tile, const drawsq_params &params ) const;
        /**
         * Draws the tile as seen from above.
         */
        void draw_from_above( const catacurses::window &w, const tripoint_bub_ms &p,
                              const const_maptile &tile, const drawsq_params &params ) const;

        int determine_wall_corner( const tripoint_bub_ms &p ) const;
        // apply a circular light pattern immediately, however it's best to use...
        void apply_light_source( const tripoint_bub_ms &p, float luminance );
        // ...this, which will apply the light after at the end of generate_lightmap, and prevent redundant
        // light rays from causing massive slowdowns, if there's a huge amount of light.
        void add_light_source( const tripoint_bub_ms &p, float luminance );
        // Handle just cardinal directions and 45 deg angles.
        void apply_directional_light( const tripoint_bub_ms &p, int direction, float luminance );
        void apply_light_arc( const tripoint_bub_ms &p, const units::angle &angle, float luminance,
                              const units::angle &wideangle = 30_degrees );
        void apply_light_ray( cata::mdarray<bool, point_bub_ms, MAPSIZE_X, MAPSIZE_Y> &lit,
                              const tripoint_bub_ms &s, const tripoint_bub_ms &e, float luminance );
        void add_light_from_items( const tripoint_bub_ms &p, const item_stack &items );
        void add_item_light_recursive( const tripoint_bub_ms &p, const item &it );
        std::unique_ptr<vehicle> add_vehicle_to_map( std::unique_ptr<vehicle> veh, bool merge_wrecks );

        // Internal methods used to bash just the selected features
        // Information on what to bash/what was bashed is read from/written to the bash_params struct
        void bash_ter_furn( const tripoint_bub_ms &p, bash_params &params,
                            bool repair_missing_ground = true );
        void bash_items( const tripoint_bub_ms &p, bash_params &params );
        void bash_vehicle( const tripoint_bub_ms &p, bash_params &params );
        void bash_field( const tripoint_bub_ms &p, bash_params &params );

        // Gets the roof type of the tile at p
        // Second argument refers to whether we have to get a roof (we're over an unpassable tile)
        // or can just return air because we bashed down an entire floor tile
        ter_str_id get_roof( const tripoint_bub_ms &p, bool allow_air ) const;

    public:
        void process_items();
        // All active items connected to the power_grid with their connection points.
        std::vector<item_reference> item_network_connections( vehicle *power_grid );
    private:
        // Iterates over every item on the map, passing each item to the provided function.
        void process_items_in_submap( submap &current_submap, const tripoint_rel_sm &gridp );
        void process_items_in_vehicles( submap &current_submap );
        void process_items_in_vehicle( vehicle &cur_veh, submap &current_submap );

        /** Enum used by functors in `function_over` to control execution. */
        enum iteration_state {
            ITER_CONTINUE = 0,  // Keep iterating
            ITER_SKIP_SUBMAP,   // Skip the rest of this submap
            ITER_SKIP_ZLEVEL,   // Skip the rest of this z-level
            ITER_FINISH         // End iteration
        };
        /**
        * Runs a functor over given submaps
        * over submaps in the area, getting next submap only when the current one "runs out" rather than every time.
        * gp in the functor is Grid (like `get_submap_at_grid`) coordinate of the submap,
        * Will silently clip the area to map bounds.
        * @param start Starting point for function
        * @param end End point for function
        * @param fun Function to run
        */
        /*@{*/
        template<typename Functor>
        void function_over( const tripoint_bub_ms &start, const tripoint_bub_ms &end, Functor fun ) const;
        /*@}*/

        /**
         * The list of currently loaded submaps. The size of this should not be changed.
         * After calling @ref load or @ref generate, it should only contain non-null pointers.
         * Use @ref getsubmap or @ref setsubmap to access it.
         */
        std::vector<submap *> grid;
        /**
         * This vector contains an entry for each trap type, it has therefore the same size
         * as the traplist vector. Each entry contains a list of all point on the map that
         * contain a trap of that type. The first entry however is always empty as it denotes the
         * tr_null trap.
         */
        std::vector< std::vector<tripoint_bub_ms> > traplocs;
        /**
         * Vector of tripoints containing active field-emitting furniture
         */
        std::vector<tripoint_bub_ms> field_furn_locs;
        /**
         * Vector of tripoints containing active field-emitting terrain
         */
        std::vector<tripoint_bub_ms> field_ter_locs;
        /**
         * Holds caches for visibility, light, transparency and vehicles
         */
        mutable std::array< std::unique_ptr<level_cache>, OVERMAP_LAYERS > caches;

        mutable std::array< std::unique_ptr<pathfinding_cache>, OVERMAP_LAYERS > pathfinding_caches;
        /**
         * Set of submaps that contain active items in absolute coordinates.
         */
        std::set<tripoint_abs_sm> submaps_with_active_items;
        std::set<tripoint_abs_sm> submaps_with_active_items_dirty;

        /**
         * Cache of coordinate pairs recently checked for visibility.
         */
        using lru_cache_t = lru_cache<point, char>;
        mutable lru_cache_t skew_vision_cache;
        mutable lru_cache_t skew_vision_wo_fields_cache;

        // Note: no bounds check
        level_cache &get_cache( int zlev ) const {
            std::unique_ptr<level_cache> &cache = caches[zlev + OVERMAP_DEPTH];
            if( !cache ) {
                cache = std::make_unique<level_cache>();
            }
            return *cache;
        }

        level_cache *get_cache_lazy( int zlev ) const {
            return caches[zlev + OVERMAP_DEPTH].get();
        }

        pathfinding_cache &get_pathfinding_cache( int zlev ) const;

        visibility_variables visibility_variables_cache;

        // caches the highest zlevel above which all zlevels are uniform
        // !value || value->first != map::abs_sub means cache is invalid
        std::optional<std::pair<tripoint_abs_sm, int>> max_populated_zlev = std::nullopt;

        // this is set for maps loaded in bounds of the main map (g->m)
        bool _main_requires_cleanup = false;
        std::optional<bool> _main_cleanup_override = std::nullopt;

    public:
        int supports_zlevels() const {
            return zlevels;
        }
        void queue_main_cleanup();
        bool is_main_cleanup_queued() const;
        void main_cleanup_override( bool over );
        const level_cache &get_cache_ref( int zlev ) const {
            return get_cache( zlev );
        }

        const pathfinding_cache &get_pathfinding_cache_ref( int zlev ) const;

        void update_pathfinding_cache( const tripoint_bub_ms &p ) const;
        void update_pathfinding_cache( int zlev ) const;

        void update_visibility_cache( int zlev );
        void invalidate_visibility_cache();
        const visibility_variables &get_visibility_variables_cache() const;

        void update_submaps_with_active_items();

        // Just exposed for unit test introspection.
        const std::set<tripoint_abs_sm> &get_submaps_with_active_items() const {
            return submaps_with_active_items;
        }
        // Clips the area to map bounds
        tripoint_range<tripoint_bub_ms> points_in_rectangle(
            const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const;
        tripoint_range<tripoint_bub_ms> points_in_radius(
            const tripoint_bub_ms &center, size_t radius, size_t radiusz = 0 ) const;

        /**
         * Yields a range of all points that are contained in the map and have the z-level of
         * this map (@ref abs_sub).
         */
        tripoint_range<tripoint_bub_ms> points_on_zlevel() const;
        /// Same as above, but uses the specific z-level. If the given z-level is invalid, it
        /// returns an empty range.
        tripoint_range<tripoint_bub_ms> points_on_zlevel( int z ) const;

        std::list<item_location> get_active_items_in_radius( const tripoint_bub_ms &center, int radius );
        std::list<item_location> get_active_items_in_radius( const tripoint_bub_ms &center, int radius,
                special_item_type type );

        /**returns positions of furnitures with matching flag in the specified radius*/
        std::list<tripoint_bub_ms> find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
                size_t radius,
                const std::string &flag,
                size_t radiusz = 0 ) const;
        /**returns positions of furnitures with matching flag in the specified radius*/
        std::list<tripoint_bub_ms> find_furnitures_with_flag_in_radius( const tripoint_bub_ms &center,
                size_t radius,
                ter_furn_flag flag,
                size_t radiusz = 0 ) const;
        /**returns creatures in specified radius*/
        std::list<Creature *> get_creatures_in_radius( const tripoint_bub_ms &center, size_t radius,
                size_t radiusz = 0 ) const;

        level_cache &access_cache( int zlev );
        const level_cache &access_cache( int zlev ) const;
        bool dont_draw_lower_floor( const tripoint_bub_ms &p ) const;

        bool has_haulable_items( const tripoint_bub_ms &pos );
        std::vector<item_location> get_haulable_items( const tripoint_bub_ms &pos );

#if defined(TILES)
        bool draw_points_cache_dirty = true;
        std::map<int, std::map<int, std::vector<tile_render_info>>> draw_points_cache;
        point_rel_ms prev_top_left;
        point_rel_ms prev_bottom_right;
        point prev_o;
        std::multimap<point, formatted_text> overlay_strings_cache;
        color_block_overlay_container color_blocks_cache;
#endif
};

// The map the code is currently processing. It is currently (incorrectly) fixed at the
// reality bubble, but will will have to be constantly adjusted to match the map that's
// actually processed (so reality bubble coordinates on a mapgen map are actually referring
// to that map rather than the reality bubble).
map &get_map();

// The reality bubble map, for when you need it to e.g. determine whether the map
// you're operating on is the reality bubble. This can be to process things differently
// between mapgen and the reality bubble, determine whether to carry over map cache info,
// determine whether to produce a sound, etc.
// Note that if the game will support more than one reality bubble map for a longer period
// than a single tick, this operation will have to be split into one for the active bubble
// for sound etc. processing, and another one for accessing all bubbles for cache
// synchronization purposes.
map &reality_bubble();

template<int SIZE, int MULTIPLIER>
void shift_bitset_cache( std::bitset<SIZE *SIZE> &cache, const point_rel_sm &s );

bool ter_furn_has_flag( const ter_t &ter, const furn_t &furn, ter_furn_flag flag );
bool generate_uniform( const tripoint_abs_sm &p, const ter_str_id &ter );
bool generate_uniform_omt( const tripoint_abs_sm &p, const oter_id &terrain_type );

/**
* Tinymap is a small version of the map which covers a single overmap terrain (OMT) tile,
* which corresponds to 2 * 2 submaps, or 24 * 24 map tiles. In addition to being smaller
* than the map, it's also limited to a single Z level (defined by the tripoint_abs_omt
* parameter to the 'load' operation, so it's not tied to the Z = 0 level).
* The tinymap's natural relative reference system is the tripoint_omt_ms one.
*/
class tinymap : private map
{
        friend class editmap;
    protected:
        tinymap( int mapsize, bool zlev ) : map( mapsize, zlev ) {};

        // This operation cannot be used with tinymap due to a lack of zlevel support, but are carried through for use by smallmap.
        void cut_down_tree( tripoint_omt_ms p, point_rel_ms dir ) {
            map::cut_down_tree( rebase_bub( p ), dir );
        };

    public:
        tinymap() : map( 2, false ) {}
        bool inbounds( const tripoint_omt_ms &p ) const {
            return map::inbounds( rebase_bub( p ) );
        }

        map *cast_to_map() {
            return this;
        }

        using map::save;
        void load( const tripoint_abs_omt &w, bool update_vehicles,
                   bool pump_events = false ) {
            map::load( project_to<coords::sm>( w ), update_vehicles, pump_events );
        };

        using map::is_main_cleanup_queued;
        using map::main_cleanup_override;
        using map::generate;
        using map::delete_unmerged_submaps;
        void place_spawns( const mongroup_id &group, int chance,
                           const point_omt_ms &p1, const point_omt_ms &p2, const int z_level, float density,
                           bool individual = false, bool friendly = false,
                           const std::optional<std::string> &name = std::nullopt,
                           int mission_id = -1 ) {
            map::place_spawns( group, chance, rebase_bub( p1 ), rebase_bub( p2 ), z_level, density, individual,
                               friendly,
                               name, mission_id );
        }
        void add_spawn( const mtype_id &type, int count, const tripoint_omt_ms &p,
                        bool friendly = false, int faction_id = -1, int mission_id = -1,
                        const std::optional<std::string> &name = std::nullopt ) {
            map::add_spawn( type, count, rebase_bub( p ), friendly, faction_id, mission_id, name );
        }

        using map::translate;
        ter_id ter( const tripoint_omt_ms &p ) const {
            return map::ter( rebase_bub( p ) );
        }
        bool ter_set( const tripoint_omt_ms &p, const ter_id &new_terrain, bool avoid_creatures = false ) {
            return map::ter_set( rebase_bub( p ), new_terrain, avoid_creatures );
        }
        bool has_flag_ter( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag_ter( flag, rebase_bub( p ) );
        }
        void draw_line_ter( const ter_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                            bool avoid_creature = false ) {
            map::draw_line_ter( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creature );
        }
        bool is_last_ter_wall( bool no_furn, const point_omt_ms &p,
                               const point_omt_ms &max, direction dir ) const {
            return map::is_last_ter_wall( no_furn, rebase_bub( p ), rebase_bub( max ), dir );
        }
        furn_id furn( const point_omt_ms &p ) const {
            return map::furn( rebase_bub( p ) );
        }
        furn_id furn( const tripoint_omt_ms &p ) const {
            return map::furn( rebase_bub( p ) );
        }
        bool has_furn( const tripoint_omt_ms &p ) const {
            return map::has_furn( rebase_bub( p ) );
        }
        bool has_furn( const point_omt_ms &p ) const {
            return map::has_furn( rebase_bub( p ) );
        }
        void set( const tripoint_omt_ms &p, const ter_id &new_terrain, const furn_id &new_furniture ) {
            map::set( rebase_bub( p ), new_terrain, new_furniture );
        }
        bool furn_set( const tripoint_omt_ms &p, const furn_id &new_furniture, bool furn_reset = false,
                       bool avoid_creatures = false ) {
            return map::furn_set( rebase_bub( p ), new_furniture, furn_reset, avoid_creatures );
        }
        void draw_line_furn( const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                             bool avoid_creatures = false ) {
            map::draw_line_furn( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creatures );
        }
        void draw_square_furn( const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2,
                               bool avoid_creatures = false ) {
            map::draw_square_furn( type, rebase_bub( p1 ), rebase_bub( p2 ), avoid_creatures );
        }
        bool has_flag_furn( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag_furn( flag, rebase_bub( p ) );
        }
        computer *add_computer( const tripoint_omt_ms &p, const std::string &name, int security ) {
            return map::add_computer( rebase_bub( p ), name, security );
        }
        std::string name( const tripoint_omt_ms &p ) {
            return map::name( rebase_bub( p ) );
        }
        bool impassable( const tripoint_omt_ms &p ) const {
            return map::impassable( rebase_bub( p ) );
        }
        tripoint_range<tripoint_omt_ms> points_on_zlevel() const;
        tripoint_range<tripoint_omt_ms> points_on_zlevel( int z ) const;
        tripoint_range<tripoint_omt_ms> points_in_rectangle(
            const tripoint_omt_ms &from, const tripoint_omt_ms &to ) const;
        tripoint_range<tripoint_omt_ms> points_in_radius(
            const tripoint_omt_ms &center, size_t radius, size_t radiusz = 0 ) const;
        map_stack i_at( const tripoint_omt_ms &p ) {
            return map::i_at( rebase_bub( p ) );
        }
        void spawn_item( const tripoint_omt_ms &p, const itype_id &type_id,
                         unsigned quantity = 1, int charges = 0,
                         const time_point &birthday = calendar::start_of_cataclysm, int damlevel = 0,
                         const std::set<flag_id> &flags = {}, const std::string &variant = "",
                         const std::string &faction = "" ) {
            map::spawn_item( rebase_bub( p ), type_id, quantity, charges, birthday, damlevel, flags, variant,
                             faction );
        }
        std::vector<item *> spawn_items( const tripoint_omt_ms &p, const std::vector<item> &new_items ) {
            return map::spawn_items( rebase_bub( p ), new_items );
        }
        item &add_item( const tripoint_omt_ms &p, item new_item ) {
            return map::add_item( rebase_bub( p ), std::move( new_item ) );
        }
        item &add_item_or_charges( const point_omt_ms &p, const item &obj,
                                   bool overflow = true ) {
            return map::add_item_or_charges( tripoint_bub_ms( rebase_bub( p ), abs_sub.z() ), obj, overflow );
        }
        std::vector<item *> put_items_from_loc(
            const item_group_id &group_id, const tripoint_omt_ms &p,
            const time_point &turn = calendar::start_of_cataclysm ) {
            return map::put_items_from_loc( group_id, rebase_bub( p ), turn );
        }
        item &add_item_or_charges( const tripoint_omt_ms &pos, item obj, bool overflow = true ) {
            return map::add_item_or_charges( rebase_bub( pos ), std::move( obj ), overflow );
        }
        std::vector<item *> place_items(
            const item_group_id &group_id, int chance, const tripoint_omt_ms &p1, const tripoint_omt_ms &p2,
            bool ongrass, const time_point &turn, int magazine = 0, int ammo = 0,
            const std::string &faction = "" ) {
            return map::place_items( group_id, chance, rebase_bub( p1 ), rebase_bub( p2 ), ongrass, turn,
                                     magazine, ammo, faction );
        }
        void add_corpse( const tripoint_omt_ms &p ) {
            map::add_corpse( rebase_bub( p ) );
        }
        void i_rem( const tripoint_omt_ms &p, item *it ) {
            map::i_rem( rebase_bub( p ), it );
        }
        void i_clear( const tripoint_omt_ms &p ) {
            map::i_clear( rebase_bub( p ) );
        }
        bool add_field( const tripoint_omt_ms &p, const field_type_id &type_id, int intensity = INT_MAX,
                        const time_duration &age = 0_turns, bool hit_player = true ) {
            return map::add_field( rebase_bub( p ), type_id, intensity, age, hit_player );
        }
        void delete_field( const tripoint_omt_ms &p, const field_type_id &field_to_remove ) {
            map::delete_field( rebase_bub( p ), field_to_remove );
        }
        bool has_flag( ter_furn_flag flag, const tripoint_omt_ms &p ) const {
            return map::has_flag( flag, rebase_bub( p ) );
        }
        void destroy( const tripoint_omt_ms &p, bool silent = false ) {
            map::destroy( rebase_bub( p ), silent );
        }
        const trap &tr_at( const tripoint_omt_ms &p ) const {
            return map::tr_at( rebase_bub( p ) );
        }
        void trap_set( const tripoint_omt_ms &p, const trap_id &type ) {
            map::trap_set( rebase_bub( p ), type );
        }
        void set_signage( const tripoint_omt_ms &p, const std::string &message ) {
            map::set_signage( rebase_bub( p ), message );
        }
        void delete_signage( const tripoint_omt_ms &p ) {
            map::delete_signage( rebase_bub( p ) );
        }
        VehicleList get_vehicles() {
            return map::get_vehicles();
        }
        optional_vpart_position veh_at( const tripoint_omt_ms &p ) const {
            return map::veh_at( rebase_bub( p ) );
        }
        vehicle *add_vehicle( const vproto_id &type, const tripoint_omt_ms &p, const units::angle &dir,
                              int init_veh_fuel = -1, int init_veh_status = -1, bool merge_wrecks = true,
                              bool force_status = false ) {
            return map::add_vehicle( type, rebase_bub( p ), dir, init_veh_fuel, init_veh_status,
                                     merge_wrecks, force_status );
        }
        void add_splatter_trail( const field_type_id &type, const tripoint_omt_ms &from,
                                 const tripoint_omt_ms &to ) {
            map::add_splatter_trail( type, rebase_bub( from ), rebase_bub( to ) );
        }
        void collapse_at( const tripoint_omt_ms &p, bool silent,
                          bool was_supporting = false,
                          bool destroy_pos = true ) {
            map::collapse_at( rebase_bub( p ), silent, was_supporting, destroy_pos );
        }
        tripoint_abs_sm get_abs_sub() const {
            return map::get_abs_sub();
        }
        tripoint_abs_ms get_abs( const tripoint_omt_ms &p ) const {
            return map::get_abs( rebase_bub( p ) );
        }
        tripoint_omt_ms get_omt( const tripoint_abs_ms &p ) const {
            return rebase_omt( map::get_bub( p ) );
        };
        bool is_outside( const tripoint_omt_ms &p ) const {
            return map::is_outside( rebase_bub( p ) );
        }
        int get_radiation( const tripoint_omt_ms &p ) const {
            return map::get_radiation( rebase_bub( p ) );
        }
        bool has_graffiti_at( const tripoint_omt_ms &p ) const {
            return map::has_graffiti_at( rebase_bub( p ) );
        }
        field &field_at( const tripoint_omt_ms &p ) {
            return map::field_at( rebase_bub( p ) );
        }
        maptile maptile_at( const tripoint_omt_ms &p );
        bool sees_some_items( const tripoint_omt_ms &p, const tripoint_omt_ms &from ) const {
            return map::sees_some_items( rebase_bub( p ), rebase_bub( from ) );
        }

        using map::rotate;
        using map::mirror;

        using map::build_outside_cache;

    protected:
        using map::set_abs_sub;
        using map::setsubmap;
        using map::get_nonant;
        int get_my_MAPSIZE() {
            return my_MAPSIZE;
        }
};

class fake_map : public tinymap
{
    private:
        std::vector<std::unique_ptr<submap>> temp_submaps_;
    public:
        explicit fake_map( const ter_id &ter_type = ter_str_id( "t_dirt" ).id() );
        ~fake_map() override;
        static constexpr int fake_map_z = -OVERMAP_DEPTH;
};

/**
* Smallmap is similar to tinymap in that it covers a single overmap terrain (OMT) tile, but differs
* from it in that it covers all Z levels, not just a single one. Its intended usage is for cases
* where you need to operate on an OMT, but cannot guarantee you needs are restricted to a single
* Z level.
* The smallmap's natural relative reference system is the tripoint_omt_ms one.
*/
class smallmap : public tinymap
{
    public:
        smallmap() : tinymap( 2, true ) {}

        void cut_down_tree( tripoint_omt_ms p, point_rel_ms dir ) {
            tinymap::cut_down_tree( p, dir );
        };
};

class small_fake_map : public smallmap
{
    private:
        std::vector<std::unique_ptr<submap>> temp_submaps_;
    public:
        explicit small_fake_map( const ter_id &ter_type = ter_str_id( "t_dirt" ).id() );
        ~small_fake_map() override;
};

#endif // CATA_SRC_MAP_H
