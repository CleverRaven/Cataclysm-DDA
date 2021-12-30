#include "simple_pathfinding.h"

#include <functional>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

#include "coordinates.h"
#include "enums.h"
#include "hash_utils.h"
#include "line.h"
#include "omdata.h"
#include "point.h"

namespace pf
{

const node_score node_score::rejected( -1, -1 );

node_score::node_score( int node_cost, int estimated_dest_cost ) : node_cost( node_cost ),
    estimated_dest_cost( estimated_dest_cost ) {}

namespace
{

struct point_node {
    point pos;
    om_direction::type dir;
    int priority;

    point_node( point pos, om_direction::type dir, int priority = 0 ): pos( pos ), dir( dir ),
        priority( priority ) {}
    // Operator overload required by priority queue interface.
    bool operator< ( const point_node &n ) const {
        return priority > n.priority;
    }
};

} // namespace

directed_path<point> greedy_path( const point &source, const point &dest, const point &max,
                                  two_node_scoring_fn<point> scorer )
{
    using Node = point_node;
    const auto inbounds = [ max ]( const point & p ) {
        return p.x >= 0 && p.x < max.x && p.y >= 0 && p.y < max.y;
    };
    const auto map_index = [ max ]( const point & p ) {
        return p.y * max.x + p.x;
    };

    directed_path<point> res;
    if( source == dest ) {
        return res;
    }
    if( !inbounds( source ) || !inbounds( dest ) ) {
        return res;
    }
    const Node first_node( source, om_direction::type::invalid, 1000 );
    if( scorer( directed_node<point>( first_node.pos, first_node.dir ),
                cata::nullopt ).node_cost < 0 ) {
        return res;
    }
    const size_t map_size = static_cast<size_t>( max.x * max.y );
    std::vector<bool> closed( map_size, false );
    std::vector<int> open( map_size, 0 );
    std::vector<short> dirs( map_size, 0 );
    std::priority_queue<Node, std::vector<Node>> nodes[2];

    int i = 0;
    nodes[i].push( first_node );
    open[map_index( source )] = std::numeric_limits<int>::max();
    while( !nodes[i].empty() ) {
        const Node mn( nodes[i].top() ); // get the best-looking node
        nodes[i].pop();
        // mark it visited
        closed[map_index( mn.pos )] = true;
        // if we've reached the end, draw the path and return
        if( mn.pos == dest ) {
            point p = mn.pos;
            res.nodes.reserve( nodes[i].size() );
            while( p != source ) {
                const int n = map_index( p );
                const om_direction::type dir = static_cast<om_direction::type>( dirs[n] );
                res.nodes.emplace_back( p, dir );
                p += om_direction::displace( dir );
            }
            res.nodes.emplace_back( p );
            return res;
        }
        for( om_direction::type dir : om_direction::all ) {
            const point p = mn.pos + om_direction::displace( dir );
            const int n = map_index( p );
            // don't allow out of bounds or already traversed tiles
            if( !inbounds( p ) || closed[n] ) {
                continue;
            }
            const node_score score = scorer( directed_node<point>( p, dir ), directed_node<point>( mn.pos,
                                             mn.dir ) );
            if( score.node_cost < 0 ) {
                continue;
            }
            const int priority = score.node_cost + score.estimated_dest_cost;
            // record direction to shortest path
            if( open[n] == 0 || open[n] > priority ) {
                dirs[n] = ( static_cast<int>( dir ) + 2 ) % 4;
                if( open[n] != 0 ) {
                    while( nodes[i].top().pos != p ) {
                        nodes[1 - i].push( nodes[i].top() );
                        nodes[i].pop();
                    }
                    nodes[i].pop();
                    if( nodes[i].size() > nodes[1 - i].size() ) {
                        i = 1 - i;
                    }
                    while( !nodes[i].empty() ) {
                        nodes[1 - i].push( nodes[i].top() );
                        nodes[i].pop();
                    }
                    i = 1 - i;
                }
                open[n] = priority;
                nodes[i].emplace( p, dir, priority );
            }
        }
    }
    return res;
}

namespace
{

const tripoint &direction_to_tripoint( direction dir )
{
    switch( dir ) {
        case direction::EAST:
            return tripoint_east;
        case direction::SOUTH:
            return tripoint_south;
        case direction::WEST:
            return tripoint_west;
        case direction::NORTH:
            return tripoint_north;
        case direction::ABOVECENTER:
            return tripoint_above;
        case direction::BELOWCENTER:
            return tripoint_below;
        default:
            debugmsg( "Unexpected direction: %d", static_cast<int>( dir ) );
            return tripoint_zero;
    }
}

bool is_horizontal( direction dir )
{
    switch( dir ) {
        case direction::EAST:
        case direction::SOUTH:
        case direction::WEST:
        case direction::NORTH:
            return true;
        default:
            return false;
    }
}

// The address of a navigation node, a compressed tripoint relative to the starting
// point, i.e. the start is always (0, 0, 0).
// NOLINTNEXTLINE(cata-xy)
struct node_address {
    int16_t x;
    int16_t y;
    int8_t z;
    explicit node_address( const tripoint &p ) : x( p.x ), y( p.y ), z( p.z ) {}
    bool operator== ( const node_address &other ) const {
        return x == other.x && y == other.y && z == other.z;
    }
    tripoint_abs_omt to_tripoint( const tripoint_abs_omt &origin ) const {
        return origin + tripoint( x, y, z );
    }
    node_address operator+ ( const tripoint &p ) const {
        node_address ret = *this;
        ret.x += p.x;
        ret.y += p.y;
        ret.z += p.z;
        return ret;
    }
    node_address displace( direction dir ) const {
        return *this + direction_to_tripoint( dir );
    }
};

struct node_address_hasher {
    std::size_t operator()( const node_address &addr ) const {
        std::uint64_t val = addr.x;
        val = ( val << 16 ) + addr.y;
        val = ( val << 16 ) + addr.z;
        return cata::hash64( val );
    }
};

/*
 * A node address annotated with its heuristic score, an approximation of how
 * much it would cost to reach the goal through this node.
 */
struct scored_address {
    struct node_address addr;
    int32_t score;
    bool operator> ( const scored_address &other ) const {
        return score >= other.score;
    }
};

/*
 * Data structure representing a navigation node that is known to be reachable. Contains
 * information about the path to get there and enough information to predict which nodes
 * may be reached from it.
 */
struct navigation_node {
    // Cost incurred to reach this node.
    int32_t cumulative_cost;
    // Cost of the node itself
    int16_t node_cost;
    // Direction towards the previous node in the path [3D].
    // Compressed encoding of "direction" enum.
    int8_t prev_dir;
    // Whether z-level transitions are permitted from this node.
    bool allow_z_change;

    direction get_prev_dir() const {
        return static_cast<direction>( prev_dir );
    }
};

const std::vector<direction> &enumerate_directions( bool allow_z_change )
{
    static const std::vector<direction> cardinal_dirs = {direction::EAST, direction::SOUTH, direction::WEST, direction::NORTH};
    static const std::vector<direction> all_dirs = [&]() {
        std::vector<direction> ret = cardinal_dirs;
        ret.push_back( direction::ABOVECENTER );
        ret.push_back( direction::BELOWCENTER );
        return ret;
    }
    ();
    if( allow_z_change ) {
        return all_dirs;
    } else {
        return cardinal_dirs;
    }
}

direction reverse_direction( direction dir )
{
    return direction_from( -direction_to_tripoint( dir ) );
}

int adjust_omt_cost( int base_cost, direction dir_in, direction dir_out )
{
    // Adjust cost for 90-degree turns. We travel from the midpoint of one edge
    // to the midpoint of an adjacent edge in a square, which is a diagonal
    // line with length = sqrt(2) / 2 for a unit square.
    if( dir_in != dir_out && is_horizontal( dir_in ) && is_horizontal( dir_out ) ) {
        // Note: sqrt(2) is approximately equal to 99 / 70.
        return base_cost * 99 / 140;
    }
    return base_cost;
}

} // namespace

const omt_score omt_score::rejected( -1 );

omt_score::omt_score( int node_cost, bool allow_z_change ) : node_cost( node_cost ),
    allow_z_change( allow_z_change ) {}

simple_path<tripoint_abs_omt> find_overmap_path( const tripoint_abs_omt &source,
        const tripoint_abs_omt &dest, const int radius, omt_scoring_fn scorer,
        cata::optional<int> max_cost )
{
    simple_path<tripoint_abs_omt> ret;
    const omt_score start_score = scorer( source );
    const omt_score end_score = scorer( dest );
    if( start_score.node_cost < 0 || end_score.node_cost < 0 ) {
        return ret;
    }
    std::unordered_map<node_address, navigation_node, node_address_hasher> known_nodes;
    std::priority_queue<scored_address, std::vector<scored_address>, std::greater<>> open_set;
    const node_address start( tripoint_zero );
    known_nodes.emplace( start, navigation_node{0, 0, -1, start_score.allow_z_change} );
    open_set.push( scored_address{ start, 0 } );
    const point_abs_omt source_point = source.xy();
    int search_count = 0;
    constexpr int max_search_count = 100000;
    while( !open_set.empty() ) {
        const node_address cur_addr = open_set.top().addr;
        open_set.pop();
        search_count++;
        const tripoint_abs_omt cur_point = cur_addr.to_tripoint( source );
        if( cur_point == dest ) {
            node_address addr = cur_addr;
            while( !( addr == start ) ) {
                const navigation_node &node = known_nodes.at( addr );
                ret.points.emplace_back( addr.to_tripoint( source ) );
                addr = addr.displace( node.get_prev_dir() );
            }
            ret.points.emplace_back( addr.to_tripoint( source ) );
            return ret;
        }
        const navigation_node &cur_node = known_nodes.at( cur_addr );
        for( direction dir : enumerate_directions( cur_node.allow_z_change ) ) {
            if( dir == cur_node.prev_dir ) {
                continue; // don't go back the way we just came
            }
            const direction rev_dir = reverse_direction( dir );
            const node_address next_addr = cur_addr.displace( dir );
            const int cumulative_cost = cur_node.cumulative_cost + adjust_omt_cost( cur_node.node_cost, rev_dir,
                                        cur_node.get_prev_dir() );
            auto iter = known_nodes.find( next_addr );
            if( iter != known_nodes.end() ) {
                navigation_node &next_node = iter->second;
                if( next_node.cumulative_cost > cumulative_cost ) {
                    next_node.cumulative_cost = cumulative_cost;
                    next_node.prev_dir = static_cast<int8_t>( rev_dir );
                }
            } else if( known_nodes.size() < max_search_count ) {
                const tripoint_abs_omt next_point = next_addr.to_tripoint( source );
                if( octile_dist( source_point, next_point.xy() ) > radius ) {
                    continue;
                }
                const omt_score next_score = scorer( next_point );
                if( next_score.node_cost < 0 ) {
                    // TODO: add to closed set to avoid re-visiting
                    continue;
                }
                // TODO: pass in the 10 (default terrain cost)
                const int xy_score = octile_dist( next_point.xy(), dest.xy(), 10 );
                const int z_score = std::abs( next_point.z() - dest.z() ) * 10;
                const int estimated_total_cost = cumulative_cost + next_score.node_cost + xy_score + z_score;
                if( max_cost && estimated_total_cost > *max_cost ) {
                    continue;
                }
                navigation_node &next_node = known_nodes[next_addr];
                next_node.cumulative_cost = cumulative_cost;
                next_node.node_cost = next_score.node_cost;
                next_node.prev_dir = static_cast<int8_t>( rev_dir );
                next_node.allow_z_change = next_score.allow_z_change;
                open_set.push( scored_address{ next_addr, estimated_total_cost } );
            }
        }
    }
    return ret;
}

} // namespace pf
