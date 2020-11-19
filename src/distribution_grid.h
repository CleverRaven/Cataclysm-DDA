#pragma once
#ifndef CATA_SRC_DISTRIBUTION_GRID_H
#define CATA_SRC_DISTRIBUTION_GRID_H

#include <vector>
#include <map>
#include "calendar.h"
#include "memory_fast.h"
#include "point.h"

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
        /**
         * Map of submap coords to points on this submap
         * that contain an active tile.
         */
        std::map<tripoint, std::vector<tile_location>> contents;

    public:
        distribution_grid() = default;
        distribution_grid( const std::vector<tripoint> &global_submap_coords );
        bool empty() const;
        void update( time_point to );
        int mod_resource( int amt );
        int get_resource() const;
};

#endif // CATA_SRC_DISTRIBUTION_GRID_H
