#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "flood_fill.h"
#include "hash_utils.h"
#include "map.h"
#include "map_iterator.h"
#include "map_memory.h"
#include "mapdata.h"
#include "messages.h"
#include "point.h"
#include "tileray.h"
#include "translations.h"
#include "type_id.h"
#include "veh_type.h"
#include "vpart_position.h"

/*
 * This file contains code that allows a vehicle to be driven by an in-game character (most
 * likely the player character) to some remote destination, following a path of adjacent overmap
 * tile (OMT) coordinates, while avoiding any obstacles along the way.
 *
 * The main entry point is do_autodrive(), which is intended to be called from the do_turn() of
 * a long-running activity. This will cause the driver character to perform one or more driving
 * actions (as long as they have enough moves), which could be steering and/or changing the
 * desired speed. The driver may also do nothing if the vehicle is going in the right
 * direction.
 *
 * Most of the logic is inside the private implementation class autodrive_controller, which is
 * instantiated at the beginning of an autodrive activity and cleaned up at the end. This class
 * caches information about the vehicle, driver and the local terrain in order to minimize the
 * amount of computation required on each invocation of do_autodrive().
 *
 * Upon reaching a new overmap tile, as well as once initially, the autodrive algorithm will
 * try to find a path that takes the vehicle from its current position through the current
 * and next OMT in order to reach the 2nd next OMT in the overall path to destination (which
 * is obtained from driver.omt_path). Thus we always plan ahead 2 OMTs worth of navigation,
 * but we only follow the first half of the route, then recompute a new route for the next
 * 2 OMTs. This ensures that we don't drive up to the edge of an OMT only to get stuck due
 * to a large obstacle that can't be driven around from that position (a dead end) and instead
 * we always choose to cross OMT boundaries in a position from which we are guaranteed to be
 * able to continue onward.
 *
 * The pathfinding algorithm being employed is A*. The graph being searched is made up of nodes
 * denoted by x and y coordinates as well as a direction of travel. Since steering changes the
 * vehicle's direction in increments of 15 degrees there are 360/15 possible orientations. By
 * limiting navigation to 2 OMTs at a time we have a maximum search space of 2*24*24*24 nodes.
 * In practice, A* finds a good path after visiting fewer than 50 nodes due to the heuristic
 * function that estimates which nodes are most likely to lead to the destination (which is
 * based on proximity to the "exit zone" leading to the next OMT and whether the vehicle is
 * pointing towards the middle of that zone). Although there can be many paths that we could
 * take to get through to the next OMT, we just take the first one we find since it tends to
 * be the most direct and is usually near-optimal. When the way is blocked we'll either give
 * up after exhausting the search space or after we hit a limit on the search length.
 *
 * In order to build the search graph for A*, we consider every possible map tile and orientation
 * in the search zone and we figure out whether the vehicle's pivot point could be placed there
 * without the vehicle overlapping some obstacle (a collision-free placement). The pivot point
 * is the part of the vehicle that moves the least while steering and is usually between the
 * wheels of the vehicle, closer to the non-steerable ones. The pathfinding problem reduces to
 * finding a path for the pivot point to follow through the set of possible positions and
 * orientations (nodes). In order to figure out the possible connections between nodes (the
 * edges of the graph) we consider a limited set of actions that the driver can perform at
 * each location: nothing, steer left or steer right (possibly more than once). Given a current
 * node, current speed and amount of steering we can predict where the vehicle's pivot point
 * will end up in 1 turn and connect the 2 nodes.
 *
 * In order to keep the search space small we only consider paths traveled at constant speed
 * (otherwise we'd have to add speed as an extra attribute in every node). When we need to
 * compute a new path we first try to use the maximum safe autodrive speed (which is typically
 * much less than the vehicle's max speed). If we can't find a path we cut the speed in half and
 * try again, repeating until we succeed or reach the minimum speed. When the target speed is
 * different from the current speed of the vehicle we try to estimate how long it will take to
 * reach that speed and take that into account when simulating the first few edges (they'll be
 * shorter or longer than the rest).
 *
 * Once we have generated a sequence of nodes to follow, we do not invoke A* again until we reach
 * the next OMT. We simply cache the path and on every subsequent call to do_autodrive() we
 * check where we are on that path, what node we're supposed to reach next and what action we
 * should use to get there. However, sometimes the simplified simulation used to compute the
 * search graph fails to accurately predict where the vehicle will end up. This could be because
 * there was too much error in an approximation (such as time to accelerate) or the driving
 * conditions changed (ran over a mound of dirt that interfered with steering). When such a
 * discrepancy is detected, autodrive will fall back to "safe mode", cutting the speed to the
 * minimum (1 tile per second) and recomputing the path.
 *
 * Since the navigation graph is cached and not updated in response to dynamic obstacles (such
 * as animals) autodrive will also perform collision detection at every turn, before taking
 * any action that may end the turn. If a possible collision is detected autodrive will enter
 * "safe mode", reducing speed and recomputing. If the speed is already at minimum it will
 * abort instead with a warning message.
 *
 * In most cases when autodrive aborts it will not automatically stop the vehicle. Stopping
 * may be undesirable since nearby enemies could get into the vehicle. It is up to the player
 * to either stop or continue driving manually for a while. However, when aborting due to a
 * collision (which happened despite collision detection, i.e. there was a simulation discrepancy
 * near an obstacle) the brakes will be engaged.
 *
 * In order to prevent exploiting the system to navigate in the dark, autodrive will also
 * perform visibility checks. Not being able to see anything in front of the vehicle will
 * immediately cancel (or fail to start) autodrive. If the driver has only partial visibility
 * of the front of the vehicle, safe mode will engage limiting the speed. Furthermore, the
 * driver needs to either see or have map memory for the map tiles in the current OMT; all
 * unseen tiles will be treated as obstacles during pathfinding. In practice, this means
 * that driving at night should only be possible through familiar terrain with limited
 * lights / night vision or through unfamiliar terrain but with good headlights / excellent
 * night vision.
 *
 * In order to keep the memory footprint small and constant, this implementation uses a different
 * coordinate system for the navigation caches than on the game map. The "nav map" is always
 * allocated to the size of two OMTs wide and one OMT high. Picture it like having one OMT `a` on
 * the left and one OMT `b` on the right. The coordinates of the nav map are translated and
 * rotated so that `a` is always the current OMT and `b` is the next OMT we are trying to reach.
 * So even if, for example, we are planning a route south, the nav map is still allocated as two
 * OMTs wide, but its coordinates are instead rotated when we want to deal with actual game map
 * coordinates.
 */

static constexpr int OMT_SIZE = coords::map_squares_per( coords::omt );
static constexpr int NAV_MAP_NUM_OMT = 2;
static constexpr int NAV_MAP_SIZE_X = NAV_MAP_NUM_OMT * OMT_SIZE;
static constexpr int NAV_MAP_SIZE_Y = OMT_SIZE;
static constexpr int NAV_VIEW_PADDING = OMT_SIZE;
static constexpr int NAV_VIEW_SIZE_X = NAV_MAP_SIZE_X + 2 * NAV_VIEW_PADDING;
static constexpr int NAV_VIEW_SIZE_Y = NAV_MAP_SIZE_Y + 2 * NAV_VIEW_PADDING;
static constexpr int TURNING_INCREMENT = 15;
static constexpr int NUM_ORIENTATIONS = 360 / TURNING_INCREMENT;
// min and max speed in tiles/s
static constexpr int MIN_SPEED_TPS = 1;
static constexpr int MAX_SPEED_TPS = 3;
static constexpr int VMIPH_PER_TPS = static_cast<int>( vehicles::vmiph_per_tile );

/**
 * Data type representing a vehicle orientation, which corresponds to an angle that is
 * a multiple of TURNING_INCREMENT degrees, measured from the x axis.
 * Range: [0, NUM_ORIENTATIONS)
 */
enum class orientation : int8_t {
    d0 = 0,
};

/*
 * Rotation amount which is a multiple of 90 degrees.
 */
enum class quad_rotation : int {
    d0 = 0, d90, d180, d270,
};

/**
 * Data type representing the convex hull of a vehicle under a particular rotation.
 */
struct vehicle_profile {
    tileray tdir;
    // Points occupied by the convex hull of the vehicle. Coordinates relative to pivot.
    std::vector<point> occupied_zone;
    // Points to check for collision when moving one step in this direction.
    // Coordinates relative to pivot.
    std::vector<point> collision_points;
};

/**
 * Data type describing how what driving actions to perform at a given location
 * in order to follow the current path to destination.
 */
struct navigation_step {
    tripoint_abs_ms pos;
    orientation steering_dir;
    int target_speed_tps;
};

/**
 * The address of a navigation node, i.e. a position and orientation on the nav map.
 */
// NOLINTNEXTLINE(cata-xy)
struct node_address {
    int16_t x;
    int16_t y;
    orientation facing_dir;
    bool operator== ( const node_address &other ) const {
        return x == other.x && y == other.y && facing_dir == other.facing_dir;
    }
    point get_point() const {
        return {x, y};
    }
};

struct node_address_hasher {
    std::size_t operator()( const node_address &addr ) const {
        std::uint64_t val = addr.x;
        val = ( val << 16 ) + addr.y;
        val = ( val << 16 ) + static_cast<int>( addr.facing_dir );
        return cata::hash64( val );
    }
};

/*
 * A node address annotated with its heuristic score, an approximation of how
 * much it would cost to reach the goal from this node.
 */
struct scored_address {
    struct node_address addr;
    int16_t score;
    bool operator> ( const scored_address &other ) const {
        return score > other.score;
    }
};

/*
 * Data structure representing a navigation node that is known to be reachable. Contains
 * information about the path to get there and enough information about the state of
 * the vehicle and driver to predict which nodes can be reached on the next turn.
 */
struct navigation_node {
    node_address prev;
    int16_t cost;
    int16_t tileray_steps;
    int16_t speed;
    int8_t target_speed_tps;
    bool is_goal;
};

/*
 * Data type describing a point transformation via translation and rotation.
 */
struct coord_transformation {
    point pre_offset;
    quad_rotation rotation;
    point post_offset;

    point transform( const point &p ) const;
    tripoint transform( const point &p, int z ) const;
    orientation transform( orientation dir ) const;
    node_address transform( const point &p, orientation dir ) const;
    coord_transformation inverse() const;
};

/*
 * Collection of points for which there is still some check left to do, and a list of points
 * that have already been visited. Used for remembering which points are up/down ramps when
 * evaluating obstacles. Ramps will lead to obstacle-detection on another z-level
 */
struct point_queue {
    std::queue<tripoint_bub_ms> to_check;
    std::unordered_set<tripoint_bub_ms> visited;
    void enqueue( const tripoint_bub_ms &p ) {
        if( visited.find( p ) == visited.end() ) {
            to_check.push( p );
        }
    }
};

/**
 * Data structure that caches all the data needed in order to navigate from one
 * OMT to the next OMT along the path to destination. Main components:
 * - the view map (precomputed obstacle positions)
 * - the nav map (valid positions and orientations where the vehicle pivot may be placed)
 * - the navigation path found by A* search
 */
struct auto_navigation_data {
    tripoint_abs_omt current_omt;
    tripoint_abs_omt next_omt;
    tripoint_abs_omt next_next_omt;

    // boundaries of the view map
    half_open_rectangle<point> view_bounds;
    // boundaries of the nav map
    half_open_rectangle<point> nav_bounds;
    // boundaries of the next omt in nav map coords
    half_open_rectangle<point> next_omt_bounds;
    // transformation from nav map to game map coords
    coord_transformation nav_to_map;
    // transformation from view map to game map coords
    coord_transformation view_to_map;
    // transformation from nav map to view map coords
    coord_transformation nav_to_view;

    bool land_ok;
    bool water_ok;
    bool air_ok;
    // the maximum speed to consider driving at, in tiles/s
    int max_speed_tps;
    // max acceleration
    std::vector<int> acceleration;
    // max amount of steering actions per turn
    int max_steer;

    std::array<vehicle_profile, NUM_ORIENTATIONS> profiles;
    // known obstacles on the view map
    cata::mdarray<bool, point, NAV_VIEW_SIZE_X, NAV_VIEW_SIZE_Y> is_obstacle;
    // z-level of where the ground is per point on the view map
    // Almost always same as the OMT's z, but might differ per mapsquare if we are driving up or down ramps
    cata::mdarray<int, point, NAV_VIEW_SIZE_X, NAV_VIEW_SIZE_Y> ground_z;
    // where on the nav map the vehicle pivot may be placed
    std::array<cata::mdarray<bool, point, NAV_VIEW_SIZE_X, NAV_VIEW_SIZE_Y>, NUM_ORIENTATIONS>
    valid_positions;
    // node addresses that are valid end positions
    std::unordered_set<node_address, node_address_hasher> goal_zone;
    // the middle of the goal zone, in nav map coords
    std::array<point, NAV_MAP_NUM_OMT> goal_points;

    // computed path to next OMT
    std::vector<navigation_step> path;

    void clear() {
        current_omt = { 0, 0, -100 };
        path.clear();
    }
    vehicle_profile &profile( orientation dir ) {
        return profiles.at( static_cast<int>( dir ) );
    }
    const vehicle_profile &profile( orientation dir ) const {
        return profiles.at( static_cast<int>( dir ) );
    }
    bool &valid_position( orientation dir, point p ) {
        return valid_positions[static_cast<int>( dir )][p.x][p.y];
    }
    bool valid_position( orientation dir, point p ) const {
        return valid_positions[static_cast<int>( dir )][p.x][p.y];
    }
    bool valid_position( const node_address &addr ) const {
        return valid_position( addr.facing_dir, point( addr.x, addr.y ) );
    }
    // transforms a point from map coords into view map coords
    point to_view( const tripoint_abs_ms &p ) const {
        return view_to_map.inverse().transform( p.raw().xy() );
    }
    // transforms a point from map bub coords into view map coords
    point to_view( const tripoint_bub_ms &p ) const {
        return to_view( get_map().getglobal( p ) );
    }
    // returns `p` adjusted so that the z-level is placed on the ground
    template<typename Tripoint>
    Tripoint adjust_z( const Tripoint &p ) const;
};

enum class collision_check_result : int {
    ok,
    no_visibility,
    close_obstacle,
    slow_down
};

class vehicle::autodrive_controller
{
    public:
        explicit autodrive_controller( const vehicle &driven_veh, const Character &driver );
        const Character &get_driver() {
            return driver;
        }
        const auto_navigation_data &get_data() {
            return data;
        }
        void check_safe_speed();
        std::optional<navigation_step> compute_next_step();
        collision_check_result check_collision_zone( orientation turn_dir );
        void reduce_speed();

    private:
        const vehicle &driven_veh;
        const Character &driver;
        auto_navigation_data data;

        void compute_coordinates();
        bool check_drivable( const tripoint_bub_ms &pt ) const;
        void compute_obstacles();
        void enqueue_if_ramp( point_queue &ramp_points, const map &here, const tripoint_bub_ms &p ) const;
        void compute_obstacles_from_enqueued_ramp_points( point_queue &ramp_points, const map &here );
        vehicle_profile compute_profile( orientation facing ) const;
        void compute_valid_positions();
        void compute_goal_zone();
        void precompute_data();
        scored_address compute_node_score( const node_address &addr, const navigation_node &node ) const;
        void compute_next_nodes( const node_address &addr, const navigation_node &node,
                                 int target_speed_tps,
                                 std::vector<std::pair<node_address, navigation_node>> &next_nodes ) const;
        std::optional<std::vector<navigation_step>> compute_path( int speed_tps ) const;
};

static const std::array<orientation, NUM_ORIENTATIONS> &all_orientations()
{
    static const auto orientations_array = [] {
        std::array<orientation, NUM_ORIENTATIONS> ret;
        for( int i = 0; i < NUM_ORIENTATIONS; i++ )
        {
            ret[i] = static_cast<orientation>( i );
        }
        return ret;
    }();
    return orientations_array;
}

/*
 * Normalize an orientation value to the range [0, NUM_ORIENTATIONS-1] using modular arithmetic.
 */
static orientation normalize_orientation( int steering_turns )
{
    steering_turns %= NUM_ORIENTATIONS;
    // beware that x % N < 0 when x < 0
    if( steering_turns < 0 ) {
        steering_turns += NUM_ORIENTATIONS;
    }
    return static_cast<orientation>( steering_turns );
}

static orientation to_orientation( units::angle angle )
{
    return normalize_orientation( std::lround( units::to_degrees(
                                      angle ) / TURNING_INCREMENT ) );
}

static std::string to_string( orientation dir )
{
    return std::to_string( static_cast<int>( dir ) );
}

static orientation operator- ( const orientation &dir )
{
    return static_cast<orientation>( ( NUM_ORIENTATIONS - static_cast<int>
                                       ( dir ) ) % NUM_ORIENTATIONS );
}

static orientation operator+ ( const orientation &dir1, const orientation &dir2 )
{
    return static_cast<orientation>( ( static_cast<int>( dir1 ) + static_cast<int>
                                       ( dir2 ) ) % NUM_ORIENTATIONS );
}

static orientation operator- ( const orientation &dir1, const orientation &dir2 )
{
    return dir1 + -dir2;
}

static orientation operator+ ( const orientation &dir, int steering_turns )
{
    return dir + normalize_orientation( steering_turns );
}

static quad_rotation operator- ( const quad_rotation &rot )
{
    return static_cast<quad_rotation>( ( 4 - static_cast<int>( rot ) ) % 4 );
}

static quad_rotation operator+ ( const quad_rotation &rot1, const quad_rotation &rot2 )
{
    return static_cast<quad_rotation>( ( static_cast<int>( rot1 ) + static_cast<int>( rot2 ) ) % 4 );
}

static quad_rotation operator- ( const quad_rotation &rot1, const quad_rotation &rot2 )
{
    return rot1 + -rot2;
}

static orientation operator+ ( const orientation &dir, const quad_rotation &rot )
{
    return dir + static_cast<orientation>( static_cast<int>( rot ) *  NUM_ORIENTATIONS / 4 );
}

static orientation &operator+= ( orientation &dir, const quad_rotation &rot )
{
    return dir = dir + rot;
}

static units::angle to_angle( const orientation &dir )
{
    return units::from_degrees( static_cast<int>( dir ) * TURNING_INCREMENT );
}

/*
 * Returns the difference between two orientation values, normalized to the range
 * [-NUM_ORIENTATIONS/2, NUM_ORIENTATIONS/2) via modular arithmetic.
 */
static int orientation_diff( const orientation &dir1, const orientation &dir2 )
{
    return static_cast<int>( dir1 - dir2 + quad_rotation::d180 ) - NUM_ORIENTATIONS / 2;
}

/*
 * Returns the angle of the given point, which must lie on either axis
 */
static quad_rotation to_quad_rotation( const point &pt )
{
    if( pt.x > 0 ) {
        return quad_rotation::d0;
    } else if( pt.x < 0 ) {
        return quad_rotation::d180;
    } else if( pt.y > 0 ) {
        return quad_rotation::d90;
    } else {
        return quad_rotation::d270;
    }
}

static node_address make_node_address( point pos, orientation dir )
{
    return { static_cast<int16_t>( pos.x ), static_cast<int16_t>( pos.y ), dir };
}

/*
 * Computes atan2(dy, dx) in "orientation" units. Optimized to use only integer
 * arithmetic and table lookups since it's used in the inner loop of A*.
 */
// NOLINTNEXTLINE(cata-xy)
static orientation approx_orientation( int dx, int dy )
{
    orientation ret = orientation::d0;
    if( dy < 0 ) {
        dx *= -1;
        dy *= -1;
        ret += quad_rotation::d180;
    }
    if( dx < 0 ) {
        std::swap( dx, dy );
        dy *= -1;
        ret += quad_rotation::d90;
    }
    if( dy > dx ) {
        ret = ret + quad_rotation::d90 - approx_orientation( dy, dx );
    } else if( dy > 0 ) {
        static const auto atan_table = [] {
            constexpr int table_size = 101;
            std::array<orientation, table_size> table;
            for( int i = 0; i <  table_size; i++ )
            {
                table[i] = to_orientation( units::from_radians( std::atan( 1.0 * i /
                                           ( table_size - 1 ) ) ) );
            }
            return table;
        }();
        ret = ret + atan_table[ dy * 100 / dx ];
    }
    return ret;
}

static point rotate( point p, quad_rotation rotation )
{
    switch( rotation ) {
        case quad_rotation::d0:
            return p;
        case quad_rotation::d90:
            return { -p.y - 1, p.x };
        case quad_rotation::d180:
            return { -p.x - 1, -p.y - 1 };  // NOLINT(cata-use-point-arithmetic)
        case quad_rotation::d270:
            return { p.y, -p.x - 1 };
    }
    return p;
}

point coord_transformation::transform( const point &p ) const
{
    return rotate( p - pre_offset, rotation ) + post_offset;
}

tripoint coord_transformation::transform( const point &p, int z ) const
{
    return tripoint( transform( p ), z );
}

orientation coord_transformation::transform( orientation dir ) const
{
    return dir + rotation;
}

node_address coord_transformation::transform( const point &p, orientation dir ) const
{
    return make_node_address( transform( p ), transform( dir ) );
}

coord_transformation coord_transformation::inverse() const
{
    return { post_offset, -rotation, pre_offset };
}

static int signum( int val )
{
    return ( 0 < val ) - ( val < 0 );
}

template<typename Tripoint>
Tripoint auto_navigation_data::adjust_z( const Tripoint &p ) const
{
    if( !land_ok ) {
        return p;
    }
    const point pt_view = to_view( p );
    if( !view_bounds.contains( pt_view ) ) {
        debugmsg( "Autodrive tried to adjust zlevel on out-of-bounds point p=%s", p.to_string() );
        return p; // shouldn't happen, but who knows.
    }
    return { p.xy(), ground_z[pt_view] };
}

void vehicle::autodrive_controller::compute_coordinates()
{
    data.view_bounds = { point_zero, {NAV_VIEW_SIZE_X, NAV_VIEW_SIZE_Y} };
    data.nav_bounds = { point_zero, {NAV_MAP_SIZE_X, NAV_MAP_SIZE_Y} };
    data.next_omt_bounds = { {OMT_SIZE, 0}, {OMT_SIZE * 2, OMT_SIZE} };

    const tripoint_rel_omt omt_diff = data.next_omt - data.current_omt;
    quad_rotation next_dir = to_quad_rotation( omt_diff.raw().xy() );
    point mid_omt( OMT_SIZE / 2, OMT_SIZE / 2 );
    const tripoint_abs_ms abs_mid_omt = project_to<coords::ms>( data.current_omt ) + mid_omt;

    data.nav_to_view = { point_zero, quad_rotation::d0, {NAV_VIEW_PADDING, NAV_VIEW_PADDING} } ;
    data.nav_to_map = { mid_omt, next_dir, abs_mid_omt.raw().xy() };
    data.view_to_map = data.nav_to_map;
    data.view_to_map.pre_offset += data.nav_to_view.post_offset;
}

vehicle_profile vehicle::autodrive_controller::compute_profile( orientation facing ) const
{
    vehicle_profile ret;
    tileray tdir( to_angle( facing ) );
    ret.tdir = tdir;
    std::map<int, std::pair<int, int>> extent_map;
    const point pivot = driven_veh.pivot_point();
    for( const vehicle_part &part : driven_veh.parts ) {
        if( part.removed ) {
            continue;
        }
        tripoint pos;
        driven_veh.coord_translate( tdir, pivot, part.mount, pos );
        if( extent_map.find( pos.y ) == extent_map.end() ) {
            extent_map[pos.y] = { pos.x, pos.x };
        } else {
            auto &extent = extent_map[pos.y];
            extent.first = std::min( extent.first, pos.x );
            extent.second = std::max( extent.second, pos.x );
        }
    }
    for( const auto &extent : extent_map ) {
        const int y = extent.first;
        const int x_min = extent.second.first;
        const int x_max = extent.second.second;
        for( int x = x_min; x <= x_max; x++ ) {
            ret.occupied_zone.emplace_back( x, y );
        }
    }
    for( int part_num : driven_veh.rotors ) {
        const vehicle_part &part = driven_veh.part( part_num );
        const int diameter = part.info().rotor_info->rotor_diameter;
        const int radius = ( diameter + 1 ) / 2;
        if( radius > 0 ) {
            tripoint pos;
            driven_veh.coord_translate( tdir, pivot, part.mount, pos );
            for( tripoint pt : points_in_radius( pos, radius ) ) {
                ret.occupied_zone.emplace_back( pt.xy() );
            }
        }
    }
    // figure out the maximum amount of displacement that can happen from advancing the
    // tileray a single step by advancing it a bunch of times and reducing the x and y
    // components of the displacement to at most 1
    constexpr int far_advance = 100;
    tdir.advance( far_advance );
    const point increment( signum( tdir.dx() ), signum( tdir.dy() ) );
    // compute the subset of vehicle points that would move to previously unoccupied spots
    // (a.k.a. "moving first") when the vehicle moves one step; these are the only points
    // we need to check for collision when the vehicle is moving in this direction
    const std::unordered_set<point> occupied_set( ret.occupied_zone.begin(), ret.occupied_zone.end() );
    for( const point &pt : ret.occupied_zone ) {
        if( occupied_set.find( pt + increment ) == occupied_set.end() ) {
            ret.collision_points.emplace_back( pt );
        }
    }
    return ret;
}

// Return true if the map tile at the given position (in map coordinates)
// can be driven on (not an obstacle).
// The logic should match what is in vehicle::part_collision().
bool vehicle::autodrive_controller::check_drivable( const tripoint_bub_ms &pt ) const
{
    const map &here = get_map();

    // check if another vehicle is there; tiles occupied by the current
    // vehicle are evidently drivable
    const optional_vpart_position ovp = here.veh_at( pt );
    if( ovp ) {
        // Known corner case: some furniture can be driven over, but will collide with
        // wheel parts; if the vehicle starts over such a furniture we'll mark that tile
        // as safe and may collide with it by turning; however if we mark it unsafe
        // we'll have no viable paths away from the starting point.
        return &ovp->vehicle() == &driven_veh;
    }

    const tripoint_abs_ms pt_abs = here.getglobal( pt );
    const tripoint_abs_omt pt_omt = project_to<coords::omt>( pt_abs );
    // only check visibility for the current OMT, we'll check other OMTs when
    // we reach them
    if( pt_omt == data.current_omt ) {
        // driver must see the tile or have seen it before in order to plan a route over it
        if( !driver.sees( pt ) ) {
            if( !driver.is_avatar() ) {
                return false;
            }
            const avatar &avatar = *driver.as_avatar();
            if( !avatar.is_map_memory_valid() ) {
                debugmsg( "autodrive querying uninitialized map memory at %s", pt_abs.to_string() );
                return false;
            }
            if( avatar.get_memorized_tile( pt_abs ) == mm_submap::default_tile ) {
                // apparently open air doesn't get memorized, so pretend it is or else
                // we can't fly helicopters due to the many unseen tiles behind the driver
                if( !( data.air_ok && here.ter( pt ) == t_open_air ) ) {
                    return false;
                }
            }
        }
    }

    // check for creatures
    // TODO: padding around monsters
    Creature *critter = get_creature_tracker().creature_at( pt, true );
    if( critter && driver.sees( *critter ) ) {
        return false;
    }

    // don't drive over visible traps
    if( here.can_see_trap_at( pt.raw(), driver ) ) {
        return false;
    }

    // check for furniture that hinders movement; furniture with 0 move cost
    // can be driven on
    const furn_id furniture = here.furn( pt );
    if( furniture != f_null && furniture.obj().movecost != 0 ) {
        return false;
    }

    const ter_id terrain = here.ter( pt );
    if( terrain == t_null ) {
        return false;
    }
    // open air is an obstacle to non-flying vehicles; it is drivable
    // for flying vehicles
    if( terrain == t_open_air ) {
        return data.air_ok;
    }
    const ter_t &terrain_type = terrain.obj();
    // watercraft can drive on water
    if( data.water_ok && terrain_type.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) ) {
        return true;
    }
    // remaining checks are for land-based navigation
    if( !data.land_ok ) {
        return false;
    }
    // NOLINTNEXTLINE(bugprone-branch-clone)
    if( terrain_type.movecost <= 0 ) {
        // walls and other impassable terrain
        return false;
    } else if( terrain_type.movecost == 2 || terrain_type.has_flag( ter_furn_flag::TFLAG_NOCOLLIDE ) ) {
        // terrain with neutral move cost or tagged with NOCOLLIDE will never cause
        // collisions
        return true;
    } else if( terrain_type.bash.str_max >= 0 && !terrain_type.bash.bash_below ) {
        // bashable terrain (but not bashable floors) will cause collisions
        return false;
    } else if( terrain_type.has_flag( ter_furn_flag::TFLAG_LIQUID ) ) {
        // water and lava
        return false;
    }
    return true;
}

void vehicle::autodrive_controller::compute_obstacles()
{
    const map &here = get_map();
    const int z = data.current_omt.z();
    point_queue ramp_points;
    for( int dx = 0; dx < NAV_VIEW_SIZE_X; dx++ ) {
        for( int dy = 0; dy < NAV_VIEW_SIZE_Y; dy++ ) {
            const tripoint abs_map_pt = data.view_to_map.transform( point( dx, dy ), z );
            const tripoint_bub_ms p = here.bub_from_abs( abs_map_pt );
            data.is_obstacle[dx][dy] = !check_drivable( p );
            data.ground_z[dx][dy] = z;
            enqueue_if_ramp( ramp_points, here, p );
        }
    }
    compute_obstacles_from_enqueued_ramp_points( ramp_points, here );
}

// Checks whether `p` is a drivable ramp up or down,
// and if so adds the ramp's destination tripoint to `ramp_points`
void vehicle::autodrive_controller::enqueue_if_ramp( point_queue &ramp_points,
        const map &here, const tripoint_bub_ms &p ) const
{
    if( !data.land_ok ) {
        return;
    }
    ramp_points.visited.emplace( p );
    if( p.z() < OVERMAP_HEIGHT && here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, p ) ) {
        ramp_points.enqueue( p + tripoint_above );
    }
    if( p.z() > -OVERMAP_DEPTH && here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, p ) ) {
        ramp_points.enqueue( p + tripoint_below );
    }
}

// Flood-fills from all enqueued points of up or down ramps. For each connected point, we
// set `is_obstacle` and `ground_z` again, based on whether they are an obstacle or not on
// this zlevel.
void vehicle::autodrive_controller::compute_obstacles_from_enqueued_ramp_points(
    point_queue &ramp_points, const map &here )
{
    auto is_drivable = [this, &here]( const tripoint_bub_ms & p ) {
        return here.inbounds( p ) && check_drivable( p );
    };
    while( !ramp_points.to_check.empty() ) {
        const tripoint_bub_ms ramp_point = ramp_points.to_check.front();
        ramp_points.to_check.pop();
        for( const tripoint_bub_ms &p : ff::point_flood_fill_4_connected( ramp_point, ramp_points.visited,
                is_drivable ) ) {
            const point pt_view = data.to_view( p );
            if( !data.view_bounds.contains( pt_view ) ) {
                continue;
            }
            // We now know that point p is drivable (and not an obstacle) on this zlevel, since
            // it passed the is_drivable check.
            if( data.is_obstacle[pt_view] ) {
                // We have examined this point previously on a different zlevel but conluded
                // that it was an obstacle at that zlevel. But on this zlevel it is apparently
                // not an obstacle, so this must be our groundlevel.
                data.ground_z[pt_view] = p.z();
            } else {
                data.ground_z[pt_view] = std::min( data.ground_z[pt_view], p.z() );
            }
            data.is_obstacle[pt_view] = false;
            enqueue_if_ramp( ramp_points, here, p );
        }
    };
}

void vehicle::autodrive_controller::compute_valid_positions()
{
    const coord_transformation veh_rot = {point_zero, -data.nav_to_map.rotation, point_zero};
    for( orientation facing : all_orientations() ) {
        const vehicle_profile &profile = data.profile( data.nav_to_map.transform( facing ) );
        for( int mx = 0; mx < NAV_MAP_SIZE_X; mx++ ) {
            for( int my = 0; my < NAV_MAP_SIZE_Y; my++ ) {
                const point nav_pt( mx, my );
                bool valid = true;
                for( const point &veh_pt : profile.occupied_zone ) {
                    const point view_pt = data.nav_to_view.transform( nav_pt ) + veh_rot.transform(
                                              veh_pt ) - veh_rot.transform( point_zero );
                    if( !data.view_bounds.contains( view_pt ) || data.is_obstacle[view_pt.x][view_pt.y] ) {
                        valid = false;
                        break;
                    }
                }
                data.valid_position( facing, nav_pt ) = valid;
            }
        }
    }
}

void vehicle::autodrive_controller::compute_goal_zone()
{
    data.goal_zone.clear();
    coord_transformation goal_transform;
    if( data.next_next_omt.xy() != data.next_omt.xy() ) {
        // set the goal at the edge of next_omt and next_next_omt (in next_omt
        // space, pointing towards next_next_omt)
        const point next_omt_middle( OMT_SIZE + OMT_SIZE / 2, OMT_SIZE / 2 );
        const tripoint_rel_omt omt_diff = data.next_next_omt - data.next_omt;
        const quad_rotation rotation = to_quad_rotation( omt_diff.raw().xy() ) - data.nav_to_map.rotation;
        goal_transform = {next_omt_middle, rotation, next_omt_middle};
    } else {
        // set the goal at the edge of cur_omt and next_omt (in next_omt space,
        // pointing away from cur_omt)
        goal_transform = { point( OMT_SIZE - 1, 0 ), quad_rotation::d0, point_zero };
    }
    constexpr int max_turns = NUM_ORIENTATIONS / 8 + 1;
    const int x = 2 * OMT_SIZE - 1;
    for( int turns = -max_turns; turns <= max_turns; turns++ ) {
        const orientation dir = orientation::d0 + turns;
        static_assert( NAV_MAP_SIZE_Y == OMT_SIZE, "Unexpected nav map size" );
        for( int y = 0; y < OMT_SIZE; y++ ) {
            const point pt( x, y );
            const node_address addr = goal_transform.transform( pt, dir );
            if( data.valid_position( addr ) ) {
                data.goal_zone.insert( addr );
            }
        }
    }
    data.goal_points[0] = point( OMT_SIZE, OMT_SIZE / 2 - 1 );
    data.goal_points[1] = goal_transform.transform( point( x, OMT_SIZE / 2 - 1 ) );
}

void vehicle::autodrive_controller::precompute_data()
{
    const tripoint_abs_omt current_omt = driven_veh.global_omt_location();
    const tripoint_abs_omt next_omt = driver.omt_path.back();
    const tripoint_abs_omt next_next_omt = driver.omt_path.size() >= 2 ?
                                           driver.omt_path[driver.omt_path.size() - 2] : next_omt;
    if( current_omt != data.current_omt || next_omt != data.next_omt ||
        next_next_omt != data.next_next_omt ) {
        data.current_omt = current_omt;
        data.next_omt = next_omt;
        data.next_next_omt = next_next_omt;

        // initialize car and driver properties
        data.land_ok = driven_veh.valid_wheel_config();
        data.water_ok = driven_veh.can_float();
        data.air_ok = driven_veh.has_sufficient_rotorlift();
        data.max_speed_tps = std::min( MAX_SPEED_TPS, driven_veh.safe_velocity() / VMIPH_PER_TPS );
        data.acceleration.resize( data.max_speed_tps );
        for( int speed_tps = 0; speed_tps < data.max_speed_tps; speed_tps++ ) {
            data.acceleration[speed_tps] = driven_veh.acceleration( true, speed_tps * VMIPH_PER_TPS );
        }
        // TODO: compute from driver's skill and speed stat
        // TODO: change it during simulation based on vehicle speed and terrain
        // or maybe just keep track of player moves?
        data.max_steer = 1;
        for( orientation dir : all_orientations() ) {
            data.profile( dir ) = compute_profile( dir );
        }

        // initialize navigation data
        compute_coordinates();
        compute_obstacles();
        compute_valid_positions();
        compute_goal_zone();
        data.path.clear();
    }
}

static navigation_node make_start_node( const node_address &start, const vehicle &driven_veh )
{
    navigation_node ret;
    ret.prev = start;
    ret.cost = 0;
    ret.tileray_steps = driven_veh.face.get_steps();
    ret.speed = driven_veh.velocity;
    ret.target_speed_tps = 0;
    ret.is_goal = false;
    return ret;
}

scored_address vehicle::autodrive_controller::compute_node_score( const node_address &addr,
        const navigation_node &node ) const
{
    // TODO: tweak this
    constexpr int cost_mult = 1;
    constexpr int forward_dist_mult = 10;
    constexpr int side_dist_mult = 8;
    constexpr int angle_mult = 2;
    constexpr int nearness_penalty = 15;
    scored_address ret{ addr, 0 };
    ret.score += cost_mult * node.cost;
    if( node.is_goal ) {
        return ret;
    }
    static constexpr std::array<point, 4> neighbor_deltas = {
        point_east, point_south, point_west, point_north
    };
    for( const point &neighbor_delta : neighbor_deltas ) {
        const point p = addr.get_point() + neighbor_delta;
        if( !data.nav_bounds.contains( p ) || !data.valid_position( addr.facing_dir, p ) ) {
            ret.score += nearness_penalty;
        }
    }
    const int omt_index = std::min( std::max( addr.x / OMT_SIZE, 0 ),
                                    static_cast<int>( data.goal_points.size() ) - 1 );
    const point waypoint_delta = data.goal_points[omt_index] - addr.get_point();
    const point goal_delta = data.goal_points.back() - addr.get_point();
    const orientation waypoint_dir = approx_orientation( waypoint_delta.x, waypoint_delta.y );
    ret.score += forward_dist_mult * std::abs( goal_delta.x );
    ret.score += side_dist_mult * std::abs( goal_delta.y );
    if( waypoint_delta.x > OMT_SIZE / 4 ) {
        ret.score += angle_mult * std::abs( orientation_diff( addr.facing_dir, waypoint_dir ) );
    }
    return ret;
}

void vehicle::autodrive_controller::compute_next_nodes( const node_address &addr,
        const navigation_node &node, int target_speed_tps,
        std::vector<std::pair<node_address, navigation_node>> &next_nodes )
const
{
    constexpr int move_cost = 10;
    constexpr int steering_cost = 1;
    const int sign = target_speed_tps > 0 ? 1 : -1;
    const int target_speed = target_speed_tps * VMIPH_PER_TPS;
    const int cur_omt = addr.x / OMT_SIZE;
    int next_speed = target_speed;
    int num_tiles_to_move = std::abs( target_speed_tps );
    if( target_speed_tps > 1 && node.speed < target_speed ) {
        const int cur_tps = std::min( std::max( node.speed / VMIPH_PER_TPS, 0 ), data.max_speed_tps - 1 );
        next_speed = std::min( std::max<int>( node.speed, 0 ) + data.acceleration[cur_tps], target_speed );
        num_tiles_to_move = next_speed / VMIPH_PER_TPS;
    }
    for( int steer = -data.max_steer; steer <= data.max_steer; steer++ ) {
        node_address next_addr = addr;
        next_addr.facing_dir = addr.facing_dir + steer;
        tileray tdir = data.profile( next_addr.facing_dir ).tdir;
        if( steer == 0 ) {
            tdir.advance( node.tileray_steps );
        }
        bool ok = true;
        bool goal_found = false;
        for( int i = 0; i < num_tiles_to_move && ok; i++ ) {
            tdir.advance( sign );
            next_addr.x += tdir.dx();
            next_addr.y += tdir.dy();
            if( !data.nav_bounds.contains( next_addr.get_point() ) ) {
                // it's ok to go out of bounds only from the "next" omt and only
                // after crossing the goal zone
                if( cur_omt == 0 || !goal_found ) {
                    ok = false;
                }
            } else if( !data.valid_position( next_addr ) ) {
                ok = false;
            } else if( !goal_found && data.goal_zone.find( next_addr ) != data.goal_zone.end() ) {
                goal_found = true;
            }
        }
        if( !ok ) {
            continue;
        }
        const int next_omt = next_addr.x / OMT_SIZE;
        if( next_omt < cur_omt ) {
            // no going back to a previous omt
            continue;
        }
        navigation_node next_node;
        next_node.prev = addr;
        next_node.tileray_steps = num_tiles_to_move + ( !steer ? node.tileray_steps : 0 );
        // TODO: improve this
        next_node.cost = node.cost + move_cost * num_tiles_to_move + ( steer ? steering_cost : 0 );
        next_node.speed = next_speed;
        next_node.target_speed_tps = target_speed_tps;
        next_node.is_goal = goal_found;
        next_nodes.emplace_back( next_addr, next_node );
    }
}

std::optional<std::vector<navigation_step>> vehicle::autodrive_controller::compute_path(
            int speed_tps ) const
{
    if( speed_tps == 0 || speed_tps < -1 ) {
        return std::nullopt;
    }
    // TODO: tweak this
    constexpr int max_search_count = 10000;
    std::vector<navigation_step> ret;
    // TODO: check simple reachability first and bail out or set upper bound on node score
    std::unordered_map<node_address, navigation_node, node_address_hasher> known_nodes;
    std::priority_queue<scored_address, std::vector<scored_address>, std::greater<>>
            open_set;
    const tripoint_abs_ms veh_pos = driven_veh.global_square_location();
    const node_address start = data.nav_to_map.inverse().transform(
                                   veh_pos.raw().xy(), to_orientation( driven_veh.face.dir() ) );
    known_nodes.emplace( start, make_start_node( start, driven_veh ) );
    open_set.push( scored_address{ start, 0 } );
    std::vector<std::pair<node_address, navigation_node>> next_nodes;
    while( !open_set.empty() ) {
        const node_address cur_addr = open_set.top().addr;
        open_set.pop();
        const navigation_node &cur_node = known_nodes[cur_addr];
        if( cur_node.is_goal ) {
            node_address addr = cur_addr;
            while( !( addr == start ) ) {
                const navigation_node &node = known_nodes.at( addr );
                const node_address &prev = node.prev;
                const tripoint_abs_ms prev_loc( data.nav_to_map.transform( prev.get_point(),
                                                data.current_omt.z() ) );
                ret.emplace_back( navigation_step{
                    data.adjust_z( prev_loc ),
                    data.nav_to_map.transform( addr.facing_dir ),
                    node.target_speed_tps
                } );
                addr = prev;
            }
            return ret;
        }
        next_nodes.clear();
        compute_next_nodes( cur_addr, cur_node, speed_tps, next_nodes );
        for( const auto &next : next_nodes ) {
            const node_address &next_addr = next.first;
            const navigation_node &next_node = next.second;
            auto iter = known_nodes.find( next_addr );
            if( iter != known_nodes.end() ) {
                const navigation_node &other_node = iter->second;
                if( next_node.cost < other_node.cost ) {
                    const bool same_dir = cur_addr.facing_dir == next_addr.facing_dir;
                    const bool dir_multiple_45 = static_cast<int>( next_addr.facing_dir ) % 3 == 0;
                    if( ( !same_dir && next_node.tileray_steps == 1 ) || dir_multiple_45 ) {
                        iter->second = next_node;
                    }
                }
            } else if( known_nodes.size() < max_search_count ) {
                known_nodes[next_addr] = next_node;
                open_set.push( compute_node_score( next_addr, next_node ) );
            }
        }
    }
    return std::nullopt;
}

vehicle::autodrive_controller::autodrive_controller( const vehicle &driven_veh,
        const Character &driver ) : driven_veh( driven_veh ), driver( driver )
{
    data.clear();
}

void vehicle::autodrive_controller::check_safe_speed()
{
    // We normally drive at or below the vehicle's "safe speed" (beyond which the engine starts
    // taking damage). We normally determine this at the beginning of path planning and cache it.
    // However, sometimes the vehicle's safe speed may drop (e.g. amphibious vehicle entering
    // water), so this extra check is needed to adjust our max speed.
    int safe_speed_tps = driven_veh.safe_velocity() / VMIPH_PER_TPS;
    if( data.max_speed_tps > safe_speed_tps ) {
        data.max_speed_tps = safe_speed_tps;
    }
}

collision_check_result vehicle::autodrive_controller::check_collision_zone( orientation turn_dir )
{
    const tripoint_bub_ms veh_pos = driven_veh.pos_bub();

    // first check if we have any visibility in front, to prevent blind driving
    tileray face_dir = driven_veh.face;
    face_dir.advance();
    const point forward_offset( face_dir.dx(), face_dir.dy() );
    bool changed_zlevel = false;
    bool blind = true;
    for( const point &p : data.profile( to_orientation( face_dir.dir() ) ).collision_points ) {
        const tripoint_bub_ms next = data.adjust_z( veh_pos + forward_offset + p );
        if( driver.sees( next ) ) {
            blind = false;
        }
        // Known quirk: the player does not always see points above or below when driving
        // up or down ramps, which makes this check think that we should stop pathfinding.
        // So here we allow having no visibility in case the path changes zlevel.
        // Checks further down below in this method will make sure that we instead slow down.
        changed_zlevel |= ( next.z() != veh_pos.z() );
    }
    if( blind && !changed_zlevel ) {
        return collision_check_result::no_visibility;
    }

    // now check the area we're about to move into in 1 step, anything there
    // should force us to stop
    const vehicle_profile &profile = data.profile( turn_dir );
    tileray tdir = driven_veh.face;
    if( turn_dir != to_orientation( tdir.dir() ) ) {
        tdir = profile.tdir;
    }
    const int speed = std::min( driven_veh.velocity + driven_veh.current_acceleration(),
                                driven_veh.cruise_velocity );
    const int speed_tps = speed / VMIPH_PER_TPS;
    std::unordered_set<point> collision_zone;
    tdir.advance();
    point offset( tdir.dx(), tdir.dy() );
    for( const point &p : profile.occupied_zone ) {
        collision_zone.insert( p + offset );
    }
    for( const point &p : collision_zone ) {
        if( !check_drivable( data.adjust_z( veh_pos + p ) ) ) {
            return collision_check_result::close_obstacle;
        }
    }

    // finally check the area further ahead; we can still avoid those collisions by reducing speed
    collision_zone.clear();
    for( int i = 1; i < speed_tps; i++ ) {
        tdir.advance();
        offset += point( tdir.dx(), tdir.dy() );
        for( const point &p : profile.collision_points ) {
            collision_zone.insert( p + offset );
        }
    }
    for( const point &p : collision_zone ) {
        const tripoint_bub_ms next = data.adjust_z( veh_pos + p );
        if( !driver.sees( next ) ) {
            return collision_check_result::slow_down;
        }
        if( !check_drivable( next ) ) {
            return collision_check_result::slow_down;
        }
    }
    return collision_check_result::ok;
}

void vehicle::autodrive_controller::reduce_speed()
{
    data.max_speed_tps = MIN_SPEED_TPS;
}

std::optional<navigation_step> vehicle::autodrive_controller::compute_next_step()
{
    precompute_data();
    const tripoint_abs_ms veh_pos = driven_veh.global_square_location();
    const bool had_cached_path = !data.path.empty();
    const bool two_steps = data.path.size() > 2;
    const navigation_step first_step = two_steps ? data.path.back() : navigation_step();
    const navigation_step second_step = two_steps ? data.path.at( data.path.size() - 2 ) :
                                        navigation_step();
    bool maintain_speed = false;
    // If vehicle did not move as far as planned and direction is the same
    // then it is still accelerating. If the vehicle moved more than expected
    // then we likely underestimated the acceleration when planning the path.
    // If either of these happen, we should maintain speed but compute a new path.
    if( two_steps && square_dist( first_step.pos.xy().raw(), second_step.pos.xy().raw() ) !=
        square_dist( first_step.pos.xy().raw(), veh_pos.xy().raw() ) &&
        first_step.steering_dir == second_step.steering_dir ) {
        data.path.pop_back();
        maintain_speed = true;
        data.path.clear();
    } else {
        while( !data.path.empty() && data.path.back().pos != veh_pos ) {
            data.path.pop_back();
        }
    }
    if( !data.path.empty() && data.path.back().target_speed_tps > data.max_speed_tps ) {
        data.path.clear();
        maintain_speed = false;
    }
    if( data.path.empty() ) {
        // if we're just starting out or we've gone off-course use the lowest speed
        if( ( had_cached_path && !maintain_speed ) || driven_veh.velocity == 0 ) {
            data.max_speed_tps = MIN_SPEED_TPS;
        }
        auto new_path = compute_path( data.max_speed_tps );
        while( !new_path && data.max_speed_tps > MIN_SPEED_TPS ) {
            // high speed didn't work, try a lower speed
            data.max_speed_tps /= 2;
            new_path = compute_path( data.max_speed_tps );
        }
        if( !new_path ) {
            return std::nullopt;
        }
        data.path.swap( *new_path );
    }
    return data.path.back();
}

std::vector<std::tuple<point, int, std::string>> vehicle::get_debug_overlay_data() const
{
    static const std::vector<std::string> debug_what = { "valid_position", "omt" };
    std::vector<std::tuple<point, int, std::string>> ret;

    const tripoint_abs_ms veh_pos = global_square_location();
    if( autodrive_local_target != tripoint_zero ) {
        ret.emplace_back( ( autodrive_local_target - veh_pos.raw() ).xy(), catacurses::red, "T" );
    }
    for( const point &pt_elem : collision_check_points ) {
        ret.emplace_back( pt_elem - veh_pos.raw().xy(), catacurses::yellow, "C" );
    }

    if( !active_autodrive_controller ) {
        return ret;
    }
    const auto_navigation_data &data = active_autodrive_controller->get_data();

    const orientation dir = to_orientation( face.dir() );
    for( const std::string &debug_str : debug_what ) {
        if( debug_str == "profiles" ) {
            const vehicle_profile &profile = data.profile( dir );
            for( const point &p : profile.occupied_zone ) {
                if( p.x == 0 && p.y == 0 ) {
                    ret.emplace_back( p, catacurses::cyan, to_string( dir ) );
                } else {
                    ret.emplace_back( p, catacurses::green, "x" );
                }
            }
            for( const point &p : profile.collision_points ) {
                ret.emplace_back( p, catacurses::red, "o" );
            }
        } else if( debug_str == "is_obstacle" ) {
            for( int dx = 0; dx < NAV_VIEW_SIZE_X; dx++ ) {
                for( int dy = 0; dy < NAV_VIEW_SIZE_Y; dy++ ) {
                    const bool obstacle = data.is_obstacle[dx][dy];
                    const int color = obstacle ? catacurses::red : catacurses::green;
                    const point pt = data.view_to_map.transform( point( dx, dy ) ) - veh_pos.raw().xy();
                    ret.emplace_back( pt, color, obstacle ? "o" : "x" );
                }
            }
        } else if( debug_str == "valid_position" ) {
            const orientation tdir = data.nav_to_map.inverse().transform( dir );
            for( int dx = 0; dx < NAV_MAP_SIZE_X; dx++ ) {
                for( int dy = 0; dy < NAV_MAP_SIZE_Y; dy++ ) {
                    const point nav_pt( dx, dy );
                    const bool valid = data.valid_position( tdir, nav_pt );
                    const int color = valid ? catacurses::green : catacurses::red;
                    const point pt = data.nav_to_map.transform( nav_pt ) - veh_pos.raw().xy();
                    ret.emplace_back( pt, color, to_string( dir ) );
                }
            }
        } else if( debug_str == "goal_zone" ) {
            std::unordered_map<point, bool> goal_map;
            for( const node_address &addr : data.goal_zone ) {
                goal_map[addr.get_point()] |= data.nav_to_map.transform( addr.facing_dir ) == dir;
            }
            for( const auto &entry : goal_map ) {
                const point pt = data.nav_to_map.transform( entry.first ) - veh_pos.raw().xy();
                const int color = entry.second ? catacurses::green : catacurses::yellow + 8;
                ret.emplace_back( pt, color, "g" );
            }
        } else if( debug_str == "goal_points" ) {
            for( point p : data.goal_points ) {
                const point pt = data.nav_to_map.transform( p ) - veh_pos.raw().xy();
                ret.emplace_back( pt, catacurses::white, "G" );
            }
        } else if( debug_str == "path" ) {
            for( const navigation_step &step : data.path ) {
                ret.emplace_back( ( step.pos - veh_pos ).raw().xy(), 8 + catacurses::yellow,
                                  to_string( step.steering_dir ) );
            }
        } else if( debug_str == "omt" ) {
            const point offset = ( project_to<coords::ms>( data.current_omt ) - veh_pos ).raw().xy();
            static const std::vector<point> corners = {point_zero, {0, OMT_SIZE - 1}, {OMT_SIZE - 1, 0}, {OMT_SIZE - 1, OMT_SIZE - 1}};
            for( point corner : corners ) {
                ret.emplace_back( corner + offset, catacurses::cyan, "+" );
            }
        }
    }
    return ret;
}

autodrive_result vehicle::do_autodrive( Character &driver )
{
    if( !is_autodriving ) {
        return autodrive_result::abort;
    }
    if( !player_in_control( driver ) || skidding ) {
        driver.add_msg_if_player( m_warning, _( "You lose control as the vehicle starts skidding." ) );
        stop_autodriving( false );
        return autodrive_result::abort;
    }
    const tripoint_abs_ms veh_pos = global_square_location();
    const tripoint_abs_omt veh_omt = project_to<coords::omt>( veh_pos );
    std::vector<tripoint_abs_omt> &omt_path = driver.omt_path;
    while( !omt_path.empty() && veh_omt.xy() == omt_path.back().xy() ) {
        omt_path.pop_back();
    }
    if( omt_path.empty() ) {
        stop_autodriving( false );
        return autodrive_result::finished;
    }
    if( !active_autodrive_controller ) {
        active_autodrive_controller = std::make_shared<autodrive_controller>( *this, driver );
    }
    if( &active_autodrive_controller->get_driver() != &driver ) {
        debugmsg( "Driver changed while auto-driving" );
        stop_autodriving();
        return autodrive_result::abort;
    }
    active_autodrive_controller->check_safe_speed();
    std::optional<navigation_step> next_step = active_autodrive_controller->compute_next_step();
    if( !next_step ) {
        // message handles pathfinding failure either due to obstacles or inability to see
        driver.add_msg_if_player( _( "Can't see a path forward." ) );
        stop_autodriving( false );
        return autodrive_result::abort;
    }
    if( next_step->pos.xy() != veh_pos.xy() ) {
        debugmsg( "compute_next_step returned an invalid result" );
        stop_autodriving();
        return autodrive_result::abort;
    }
    if( next_step->target_speed_tps == 0 && velocity == 0 ) {
        stop_autodriving( false );
        return autodrive_result::finished;
    }
    cruise_velocity = next_step->target_speed_tps * VMIPH_PER_TPS;

    // check for collisions before we steer, since steering may end our turn
    // which would cause the vehicle to move and maybe crash
    switch( active_autodrive_controller->check_collision_zone( next_step->steering_dir ) ) {
        case collision_check_result::no_visibility:
            driver.add_msg_if_player( m_warning, _( "You can't see anything in front of your vehicle!" ) );
            stop_autodriving();
            return autodrive_result::abort;
        case collision_check_result::close_obstacle:
            driver.add_msg_if_player( m_warning, _( "You're about to crash into something!" ) );
            stop_autodriving( false );
            return autodrive_result::abort;
        case collision_check_result::slow_down:
            active_autodrive_controller->reduce_speed();
            if( cruise_velocity > VMIPH_PER_TPS ) {
                cruise_velocity = VMIPH_PER_TPS;
            }
            break;
        case collision_check_result::ok:
            break;
    }

    int turn_delta = orientation_diff( next_step->steering_dir, to_orientation( turn_dir ) );
    // pldrive() does not handle steering multiple times in one call correctly
    // call it multiple times, matching how a player controls the vehicle
    for( int i = 0; i < std::abs( turn_delta ); i++ ) {
        if( driver.moves <= 0 ) {
            // we couldn't steer as many times as we wanted to but there's
            // nothing we can do about it now, hope we don't crash!
            break;
        }
        pldrive( driver, { signum( turn_delta ), 0 } );
    }
    // Don't do anything else below; the driver's turn may be over (moves <= 0) so
    // any extra actions would be "cheating".

    return autodrive_result::ok;
}

void vehicle::stop_autodriving( bool apply_brakes )
{
    if( !is_autodriving && !is_patrolling && !is_following ) {
        return;
    }
    if( apply_brakes ) {
        cruise_velocity = 0;
    }
    is_autodriving = false;
    is_patrolling = false;
    is_following = false;
    autopilot_on = false;
    autodrive_local_target = tripoint_zero;
    collision_check_points.clear();
    active_autodrive_controller.reset();
}
