#pragma once
#ifndef CATA_SRC_MAP_MEMORY_H
#define CATA_SRC_MAP_MEMORY_H

#include <iosfwd>

#include "lru_cache.h"
#include "point.h" // IWYU pragma: keep

class JsonIn;
class JsonObject;
class JsonOut;

struct memorized_terrain_tile {
    std::string tile;
    int subtile;
    int rotation;
};

class map_memory
{
    public:
        void store( JsonOut &jsout ) const;
        void load( JsonIn &jsin );
        void load( const JsonObject &jsin );

        /** Memorizes a given tile; finalize_tile_memory needs to be called after it */
        void memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                            int subtile, int rotation );
        /** Returns last stored map tile in given location */
        memorized_terrain_tile get_tile( const tripoint &pos ) const;

        void memorize_symbol( int limit, const tripoint &pos, int symbol );
        int get_symbol( const tripoint &pos ) const;

        void clear_memorized_tile( const tripoint &pos );
    private:
        lru_cache<tripoint, memorized_terrain_tile> tile_cache;
        lru_cache<tripoint, int> symbol_cache;
};

#endif // CATA_SRC_MAP_MEMORY_H
