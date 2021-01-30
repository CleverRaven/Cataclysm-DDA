#pragma once
#ifndef CATA_SRC_DISTRIBUTION_GRID_H
#define CATA_SRC_DISTRIBUTION_GRID_H

#include <vector>
#include <map>
#include "active_tile_data.h"
#include "calendar.h"
#include "memory_fast.h"
#include "point.h"

class map;
class mapbuffer;

struct tile_location {
    point on_submap;
    tripoint absolute;

    tile_location( point on_submap, tripoint absolute )
        : on_submap( on_submap )
        , absolute( absolute )
    {}
};

/**
 * A cache that organizes producers, storage and consumers
 * of some resource, like electricity.
 */
class distribution_grid
{
    private:
        friend class distribution_grid_tracker;
        /**
         * Map of submap coords to points on this submap
         * that contain an active tile.
         */
        std::map<tripoint, std::vector<tile_location>> contents;
        std::vector<tripoint> submap_coords;

        mapbuffer &mb;

    public:
        distribution_grid( const std::vector<tripoint> &global_submap_coords, mapbuffer &buffer );
        bool empty() const;
        explicit operator bool() const;
        void update( time_point to );
        int mod_resource( int amt );
        int get_resource() const;
};

/**
 * Contains and manages all the active distribution grids.
 */
class distribution_grid_tracker
{
    private:
        /**
         * Mapping of sm position to grid it belongs to.
         */
        std::map<tripoint, shared_ptr_fast<distribution_grid>> parent_distribution_grids;

        /**
         * @param omt_pos Absolute submap position of one of the tiles of the grid.
         */
        void make_distribution_grid_at( const tripoint &sm_pos );

        /**
         * In submap coords, to mirror @ref map
         */
        rectangle bounds;

        mapbuffer &mb;

    public:
        distribution_grid_tracker();
        distribution_grid_tracker( mapbuffer &buffer );
        distribution_grid_tracker( distribution_grid_tracker && ) = default;
        /**
         * Gets grid at given global map square coordinate. @ref map::getabs
         */
        /**@{*/
        distribution_grid &grid_at( const tripoint &p );
        const distribution_grid &grid_at( const tripoint &p ) const;
        /*@}*/

        void update( time_point to );
        /**
         * Loads grids in an area given by submap coords.
         */
        void load( rectangle area );
        /**
         * Loads grids in the same area as a given map.
         */
        void load( const map &m );

        /**
         * Updates grid at given global map square coordinate.
         */
        void on_changed( const tripoint &p );
        void on_saved();

        /**
         * Traverses the graph of connected vehicles and grids.
         */
        template <typename VehFunc, typename GridFunc, typename StartPoint>
        int traverse_graph( StartPoint *start, int amount,
                            VehFunc veh_action, GridFunc grid_action );
};



/**
 * Returns distribution grid tracker that is a part of the global game *g. @ref game
 * TODO: This wouldn't be required in an ideal world
 */
distribution_grid_tracker &get_distribution_grid_tracker();

#endif // CATA_SRC_DISTRIBUTION_GRID_H
