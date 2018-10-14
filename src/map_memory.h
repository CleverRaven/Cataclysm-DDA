#pragma once
#ifndef MAP_MEMORY_H
#define MAP_MEMORY_H

#include "enums.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class JsonOut;
class JsonObject;

struct memorized_terrain_tile {
    std::string tile;
    int subtile;
    int rotation;
};

class map_memory
{
    public:
        void store( JsonOut &jsout ) const;
        void load( JsonObject &jsin );

        /** Memorizes a given tile; finalize_tile_memory needs to be called after it */
        void memorize_tile( const tripoint &pos, const std::string &ter, const int subtile,
                            const int rotation );
        /** Called after several calls of memorize_tile, processes all tiles set to memorize */
        void finalize_tile_memory( size_t max_submaps );
        /** Memorizes several tiles at once */
        void memorize_tiles( const std::map<tripoint, memorized_terrain_tile> &tiles, size_t max_submaps );
        /** Adds new submaps to the memorized submap list, and pushes out the oldest ones */
        void update_submap_memory( const std::set<tripoint> &submaps, const size_t max_submaps );
        /** Erases specific submaps from memory */
        void clear_submap_memory( const std::set<tripoint> &erase );

        /** Returns last stored map tile in given location */
        memorized_terrain_tile get_memorized_terrain( const tripoint &p ) const;

        void memorize_terrain_symbol( const tripoint &pos, const long symbol );
        void memorize_terrain_symbols( const std::map<tripoint, long> &tiles, size_t max_submaps );
        void finalize_terrain_memory_curses( const size_t max_submaps );
        long get_memorized_terrain_curses( const tripoint &p ) const;
    private:
        std::map<tripoint, memorized_terrain_tile> memorized_terrain_tmp;
        std::map<tripoint, memorized_terrain_tile> memorized_terrain;
        std::map<tripoint, long> memorized_terrain_curses_tmp;
        std::map<tripoint, long> memorized_terrain_curses;
        std::vector<tripoint> memorized_submaps;
};

#endif
