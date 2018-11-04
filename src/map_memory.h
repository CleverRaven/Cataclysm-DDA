#pragma once
#ifndef MAP_MEMORY_H
#define MAP_MEMORY_H

#include "enums.h"

#include <list>
#include <string>
#include <unordered_map>

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
        void memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                            const int subtile, const int rotation );
        /** Returns last stored map tile in given location */
        memorized_terrain_tile get_tile( const tripoint &p ) const;

        void memorize_symbol( int limit, const tripoint &pos, const long symbol );
        long get_symbol( const tripoint &p ) const;
    private:
        void trim( int limit );
        using tile_pair = std::pair<tripoint, memorized_terrain_tile>;
        std::list<tile_pair> tiles;
        std::unordered_map<tripoint, std::list<tile_pair>::iterator> tile_map;
        using symbol_pair = std::pair<tripoint, long>;
        std::list<symbol_pair> symbols;
        std::unordered_map<tripoint, std::list<symbol_pair>::iterator> symbol_map;
};

#endif
