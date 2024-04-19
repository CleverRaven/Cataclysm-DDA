#pragma once
#ifndef CATA_SRC_BALLISTICS_H
#define CATA_SRC_BALLISTICS_H

#include <cstddef>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "weakpoint.h"
#include "weighted_list.h"

class Creature;
class dispersion_sources;
class vehicle;
struct dealt_projectile_attack;
struct projectile;
struct tripoint;

/** Aim result for a single projectile attack */
struct projectile_attack_aim {
    ///< Hit quality, where 0.0 is a perfect hit and 1.0 is a miss
    double missed_by = 0;
    ///< Number of tiles the attack missed by
    double missed_by_tiles = 0;
    ///< Dispersion of this particular shot in arcminutes
    double dispersion = 0;
};

/**
 * Evaluates dispersion sources, range, and target to determine attack trajectory.
 **/
projectile_attack_aim projectile_attack_roll( const dispersion_sources &dispersion, double range,
        double target_size );

/**
 *  Fires a projectile at the target point from the source point with total_dispersion
 *  dispersion.
 *  Returns the rolled dispersion of the shot and the actually hit point.
 */
dealt_projectile_attack projectile_attack( const projectile &proj_arg, const tripoint &source,
        const tripoint &target_arg, const dispersion_sources &dispersion,
        Creature *origin = nullptr, const vehicle *in_veh = nullptr,
        const weakpoint_attack &attack = weakpoint_attack(), bool first = true );

/* Used for selecting which part to target in a projectile attack
 * Primarily a template for ease of testing, but can be reused!
 * To use:
 * 1. Create a 'Wrapper' class implementing two (static) functions:
 *   T Wrapper::connection(const T &)
 * given an element, produce the element it is connected to.
 *   double Wrapper::weight(const T &)
 * given an element, produce the weight of this element relative to other elements in the graph
 *
 * 2. Create a targeting_graph<T, Wrapper>, and call generate()
 *    provide generate with a central part,
 *    and a vector of parts which all connect (eventually) to the center part
 *
 * 3. Select a part by calling select()
 *    Provide select() with a value for how accurate the hit was,
 *    and a range (min/max) of possible values for accuracy (ensure max > min)
 *    Lower values are more accurate.
 */
template<typename T, typename Wrapper>
class targeting_graph
{
        struct node {
            T val;
            double weight;
            std::vector<const node *> connections;
        };
        std::map<T, node> graph;
        T root;

        const node &add_node( const T &val ) {
            node added;
            added.val = val;
            added.weight = Wrapper::weight( val );
            return graph.emplace( val, added ).first->second;
        }

        void add( const T &from, const T &to ) {
            graph[from].connections.emplace_back( &add_node( to ) );
        }

    public:
        void generate( const T &center, const std::vector<T> &parts ) {
            add_node( center );
            root = center;

            // First, enumerate all of the connections
            std::map<T, std::set<T>> connects_to;
            for( const T &cursor : parts ) {
                // The root part connects to itself - don't add that
                if( Wrapper::connection( cursor ) == cursor ) {
                    continue;
                }
                // Add the connection to the map, if it's not there already
                auto result = connects_to.emplace( Wrapper::connection( cursor ), std::set<T>() );
                // And then add the connected to the vector stored there
                result.first->second.emplace( cursor );
            }

            std::list<T> next;
            next.push_back( center );
            while( !next.empty() ) {
                const T current = next.front();
                next.pop_front();
                // Look for what connects to me
                const auto connected_iter = connects_to.find( current );
                if( connected_iter != connects_to.end() ) {
                    for( const T &cursor : connected_iter->second ) {
                        add( current, cursor );
                        next.push_back( cursor );
                    }
                    connects_to.erase( connected_iter );
                }
                // Look for what I connect to
                for( std::pair<const T, std::set<T>> &entry : connects_to ) {
                    if( entry.second.find( current ) == entry.second.end() ) {
                        continue;
                    }
                    next.push_back( entry.first );
                    add( current, entry.first );
                    entry.second.erase( current );
                }
            }
        }

        T select( const double range_min, const double range_max, double value ) const {
            // First, find the path we will follow
            // That is, what body parts we will hit with less and less accurate shots
            std::list<const node *> path;
            // Of course, start with the center of mass
            path.push_back( &graph.at( root ) );
            // While there's still a body part to hit with less accurate shots
            while( !path.back()->connections.empty() ) {
                // Select the next body part to hit based on the relative size of the options
                weighted_float_list<size_t> next;
                const std::vector<const node *> &connections = path.back()->connections;
                for( size_t i = 0; i < connections.size(); ++i ) {
                    next.add( i, connections[i]->weight );
                }
                path.push_back( connections[*next.pick()] );
            }

            // This will make the lowest possible value of 'value' 0,
            // and the highest possible value 'range_max' - 'range_min'
            // Or, if range_min is -0.5, range_max is 1.5, this will put value into the range 0-2
            // And below, we'll have a scale factor of 2 / total_weight
            value -= range_min;

            // Now, find the total weight along our path
            double total_weight = 0.0;
            for( const node *nd : path ) {
                total_weight += nd->weight;
            }

            // And using this, and the min/max ranges of our target value,
            // find how to appropriate scale the weight of each node in our path
            double scale_factor = ( range_max - range_min ) / total_weight;
            // Then, just walk along the path
            double accumulated_weight = 0.0;
            for( const node *nd : path ) {
                accumulated_weight += nd->weight * scale_factor;
                // And quit when we've gone far enough that we can't make it to the next part
                if( accumulated_weight > value ) {
                    return nd->val;
                }
            }

            // And if we made it all the way here, we (somehow) hit with a very inaccurate shot
            return path.back()->val;
        }
};

#endif // CATA_SRC_BALLISTICS_H
