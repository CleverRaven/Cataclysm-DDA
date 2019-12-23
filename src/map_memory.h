#pragma once
#ifndef MAP_MEMORY_H
#define MAP_MEMORY_H

#include <string>
#include <map>

#include "lru_cache.h"
#include "point.h" // IWYU pragma: keep

class JsonOut;
class JsonObject;
class JsonIn;

struct map_memory_tile {
    std::string tile;
    int subtile;
    int rotation;
};

enum class map_memory_layer : int {
    terrain = 0,
    furniture,
    trap,
    vpart,
    num_map_memory_layer
};

template<>
struct enum_traits<map_memory_layer> {
    static constexpr auto last = map_memory_layer::num_map_memory_layer;
};

class map_memory
{
    public:
        void store( JsonOut &jsout ) const;
        void load( JsonIn &jsin );

        /** Memorizes a given tile; finalize_tile_memory needs to be called after it */
        void memorize_tile( int limit, const tripoint &pos, const std::string &tile_name,
                            int subtile, int rotation, const map_memory_layer &layer );
        /** Returns last stored map tile in given location and layer */
        map_memory_tile get_tile( const tripoint &pos, const map_memory_layer &layer ) const;
        void memorize_symbol( int limit, const tripoint &pos, int symbol );
        int get_symbol( const tripoint &pos ) const;
        void clear_memorized_tile( const tripoint &pos, const map_memory_layer &layer );
        void init( const map_memory_layer &layer ) {
            tile_cache[layer].clear();
        }
        void init() {
            for( int i = 0; i < static_cast<int>( map_memory_layer::num_map_memory_layer ); i++ ) {
                init( static_cast<map_memory_layer>( i ) );
            }
        }
    private:
        std::map<map_memory_layer, lru_cache<tripoint, map_memory_tile>> tile_cache;
        lru_cache<tripoint, int> symbol_cache;
};

#endif
