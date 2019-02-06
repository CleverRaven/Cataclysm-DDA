#pragma once
#ifndef MAP_MEMORY_H
#define MAP_MEMORY_H

#include <list>
#include <string>
#include <unordered_map>

#include "enums.h" // IWYU pragma: keep

class JsonOut;
class JsonObject;

struct memorized_terrain_tile {
    std::string tile;
    int subtile;
    int rotation;
};

template<typename T>
class lru_cache
{
    public:
        using Pair = std::pair<tripoint, T>;

        void insert( int limit, const tripoint &, const T & );
        T get( const tripoint &, const T &default_ ) const;
        void remove( const tripoint & );

        void clear();
        const std::list<Pair> &list() const;
    private:
        void trim( int limit );
        std::list<Pair> ordered_list;
        std::unordered_map<tripoint, typename std::list<Pair>::iterator> map;
};

extern template class lru_cache<memorized_terrain_tile>;
extern template class lru_cache<long>;

class map_memory
{
    public:
        void store( JsonOut &jsout ) const;
        void load( JsonIn &jsin );
        void load( JsonObject &jsin );

        /** Memorizes a given tile; finalize_tile_memory needs to be called after it */
        void memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                            const int subtile, const int rotation );
        /** Returns last stored map tile in given location */
        memorized_terrain_tile get_tile( const tripoint &pos ) const;

        void memorize_symbol( int limit, const tripoint &pos, const long symbol );
        long get_symbol( const tripoint &pos ) const;

        void clear_memorized_tile( const tripoint &pos );
    private:
        lru_cache<memorized_terrain_tile> tile_cache;
        lru_cache<long> symbol_cache;
};

#endif
