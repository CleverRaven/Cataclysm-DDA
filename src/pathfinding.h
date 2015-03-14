#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "json.h"
#include "enums.h"
#include "map.h"
#include <set>
#include <map>
#include <unordered_map>

class path_manager;

#define GETPATHFINDER(x) (path_manager::get_manager().get_pathfinder(x))

/**
 * Result of a single pathfinding
 */
struct path_data
{
    // Not needed?
    point destination;
    // If destination can't be reached from (x,y), then parent[x][y] is (-1,-1)
    point parent[SEEX * MAPSIZE][SEEY * MAPSIZE];
};

/**
 * Pathfinding class
 *
 * Handles hordes of monsters with same target and same movement modes.
 * Implements "inverse Dijkstra" - starts at the target and finds the
 * shortest path to all points that would be start points in regular Dijkstra.
 */
class pathfinder
{
private:
    friend path_manager;

    // Map destination points to path data that allows getting there
    // Note: may not contain all possible starting points! Check for (-1,-1) parent.
    std::map< point, path_data > path_map;

    // Map of destinations to sets of seekers who want to seek particular destination
    // For example, player's position can map to positions of all 
    // zombies trying to find them this turn
    std::map< point, std::set< point > > seekers;

    // Maximum distance - no path longer than this will be found
    int max_dist;
    // Bashing strength used to rate bashable obstacles
    int bash_force;
    // Should we try to open doors? (both regular and car ones)
    bool open_doors;
    // TODO: implement this block below (needed: field name->field id converter)
    // Fields we won't enter.
    // TODO: Change from set to map and assign weights instead of binary OK/unpassable
    //std::set< field_id > field_avoid;

    /**
     * Generate path data that will allow to trace paths from points in `from` to point `to`.
     * After this function finishes, @ref path_map will contain an entry that can be used
     * to generate paths from points in `from` to point `to` using method @ref get_path.
     *
     * Note that not all paths may be possible - a point with no valid path to `to` will
     * have parent (-1, -1).
     */
    void generate_path( const map &m, const std::set< point > &from, const point &to );
public:

    /**
     * Clears all path data and pathing requests
     */
    void clear_paths()
    {
        path_map.clear();
        seekers.clear();
    }
    /**
     * Changes bash strength used to estimate the cost of getting through a bashable obstacle.
     */
    void set_bash( const int new_bash )
    {
        bash_force = new_bash;
    }

    /**
     * Register that we'll want to calculate a path from->to
     *
     * Note: path won't actually be generated until @ref generate_paths is called.
     */
    void request_path( const point &from, const point &to );

    /**
     * Actually generates all pathfinding data
     */
    void generate_paths( const map &m );

    /**
     * Gets one of the paths generated with @ref generate_paths.
     * Will debugmsg if the path hasn't been registered with @ref request_path.
     */
    std::vector< point > get_path( const map &m, const point &from, const point &to ) const;
};

/**
 * Pathfinding manager
 *
 * Singleton to store and provide pathfinding data. Also a factory for pathfinders.
 */
class path_manager
{
private:
    // TODO (maybe): Change to int ids calculated at load time
    std::unordered_map< std::string, pathfinder > finders;
public:
    static path_manager& get_manager()
    {
        static path_manager manager;
        return manager;
    }

    // Gets a pathfinder of given id
    pathfinder &get_pathfinder( const std::string &id )
    {
        static pathfinder nullfinder;
        auto iter = finders.find( id );
        if( iter != finders.end() ) {
            return iter->second;
        } else {
            debugmsg( "Tried to get pathfinder %s, which doesn't exist", id.c_str() );
            return nullfinder;
        }
    }

    // Clear all paths - should be invoked before or after all pathfinding
    void clear_paths()
    {
        for( auto &f : finders ) {
            f.second.clear_paths();
        }
    }

    // Generate all paths in all pathfinders
    void generate_paths( const map &m )
    {
        for( auto &f : finders ) {
            f.second.generate_paths( m );
        }
    }

    // Loads a new pathfinder object
    void load_pathfinder( JsonObject &jo );
};

#endif
