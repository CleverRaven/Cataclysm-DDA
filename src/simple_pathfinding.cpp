#include "simple_pathfinding.h"

#include <functional>
#include <limits>
#include <queue>
#include <vector>

#include "enums.h"
#include "omdata.h"
#include "point.h"
#include "point_traits.h"

namespace pf
{

const node_score node_score::rejected( -1, -1 );

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
            res.nodes.emplace_back( p, om_direction::type::invalid );
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

} // namespace pf
