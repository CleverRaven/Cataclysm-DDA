#pragma once
#ifndef CATA_SRC_MAP_MEMORY_H
#define CATA_SRC_MAP_MEMORY_H

#include <iosfwd>

#include "game_constants.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "point.h" // IWYU pragma: keep

class JsonOut;
class JsonObject;
class JsonOut;
class JsonValue;

struct memorized_terrain_tile {
    std::string tile;
    int subtile;
    int rotation;

    inline bool operator==( const memorized_terrain_tile &rhs ) const {
        return ( rotation == rhs.rotation ) && ( subtile == rhs.subtile ) && ( tile == rhs.tile );
    }

    inline bool operator!=( const memorized_terrain_tile &rhs ) const {
        return !( *this == rhs );
    }
};

/** Represent a submap-sized chunk of tile memory. */
struct mm_submap {
    public:
        static const memorized_terrain_tile default_tile;
        static const int default_symbol;

        mm_submap();
        explicit mm_submap( bool make_valid );

        /** Whether this mm_submap is empty. Empty submaps are skipped during saving. */
        bool is_empty() const {
            return tiles.empty() && symbols.empty();
        }

        // Whether this mm_submap is invalid, i.e. returned from an uninitialized region.
        bool is_valid() const {
            return valid;
        }

        inline const memorized_terrain_tile &tile( const point &p ) const {
            if( tiles.empty() ) {
                return default_tile;
            } else {
                return tiles[p.y * SEEX + p.x];
            }
        }

        inline void set_tile( const point &p, const memorized_terrain_tile &value ) {
            if( tiles.empty() ) {
                // call 'reserve' first to force allocation of exact size
                tiles.reserve( SEEX * SEEY );
                tiles.resize( SEEX * SEEY, default_tile );
            }
            tiles[p.y * SEEX + p.x] = value;
        }

        inline int symbol( const point &p ) const {
            if( symbols.empty() ) {
                return default_symbol;
            } else {
                return symbols[p.y * SEEX + p.x];
            }
        }

        inline void set_symbol( const point &p, int value ) {
            if( symbols.empty() ) {
                // call 'reserve' first to force allocation of exact size
                symbols.reserve( SEEX * SEEY );
                symbols.resize( SEEX * SEEY, default_symbol );
            }
            symbols[p.y * SEEX + p.x] = value;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonValue &ja );

    private:
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<memorized_terrain_tile> tiles; // holds either 0 or SEEX*SEEY elements
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<int> symbols; // holds either 0 or SEEX*SEEY elements
        bool valid = true; // NOLINT(cata-serialize)
};

/**
 * Represents a square of mm_submaps.
 * For faster save/load, submaps are collected into regions
 * and each region is saved in its own file.
 */
struct mm_region {
    cata::mdarray<shared_ptr_fast<mm_submap>, point, MM_REG_SIZE, MM_REG_SIZE> submaps;

    mm_region();

    bool is_empty() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonValue &ja );
};

/**
 * Manages map tiles memorized by the avatar.
 * Note that there are 2 separate memories in here:
 *   1. memorized graphic tiles (for TILES with a tileset)
 *   2. memorized symbols (for CURSES or TILES in ascii mode)
 * TODO: combine tiles and curses. Also, split map memory into layers (terrain/furn/vpart/...)?
 */
class map_memory
{
    private:
        /**
         * Helper class for converting global ms coord into
         * global sm coord + ms coord within the submap.
         */
        struct coord_pair {
            tripoint sm;
            point loc;

            explicit coord_pair( const tripoint &p );
        };

    public:
        map_memory();

        /** Load memorized submaps around given global map square pos. */
        void load( const tripoint &pos );

        /** Load legacy memory file. TODO: remove after 0.F (or whatever BN will have instead). */
        void load_legacy( const JsonValue &jv );

        /** Save memorized submaps to disk, drop ones far from given global map square pos. */
        bool save( const tripoint &pos );

        /**
         * Prepares map memory for rendering and/or memorization of given region.
         * @param p1 top-left corner of the region, in global ms coords
         * @param p2 bottom-right corner of the region, in global ms coords
         * Both coords are inclusive and should be on the same Z level.
         * @return whether the region was re-cached
         */
        bool prepare_region( const tripoint &p1, const tripoint &p2 );

        /**
         * Memorizes given tile, overwriting old value.
         * @param pos tile position, in global ms coords.
         */
        void memorize_tile( const tripoint &pos, const std::string &ter,
                            int subtile, int rotation );
        /**
         * Returns memorized tile.
         * @param pos tile position, in global ms coords.
         */
        const memorized_terrain_tile &get_tile( const tripoint &pos ) const;

        /**
         * Memorizes given symbol, overwriting old value.
         * @param pos tile position, in global ms coords.
        */
        void memorize_symbol( const tripoint &pos, int symbol );

        /**
         * Returns memorized symbol.
         * @param pos tile position, in global ms coords.
         */
        int get_symbol( const tripoint &pos ) const;

        /**
         * Clears memorized tile and symbol.
         * @param pos tile position, in global ms coords.
         */
        void clear_memorized_tile( const tripoint &pos );

    private:
        std::map<tripoint, shared_ptr_fast<mm_submap>> submaps;

        std::vector<shared_ptr_fast<mm_submap>> cached;
        tripoint cache_pos;
        point cache_size;

        /** Find, load or allocate a submap. @returns the submap. */
        shared_ptr_fast<mm_submap> fetch_submap( const tripoint &sm_pos );
        /** Find submap amongst the loaded submaps. @returns nullptr if failed. */
        shared_ptr_fast<mm_submap> find_submap( const tripoint &sm_pos );
        /** Load submap from disk. @returns nullptr if failed. */
        shared_ptr_fast<mm_submap> load_submap( const tripoint &sm_pos );
        /** Allocate empty submap. @returns the submap. */
        shared_ptr_fast<mm_submap> allocate_submap( const tripoint &sm_pos );

        /** Get submap from within the cache */
        //@{
        const mm_submap &get_submap( const tripoint &sm_pos ) const;
        mm_submap &get_submap( const tripoint &sm_pos );
        //@}

        void clear_cache();
};

#endif // CATA_SRC_MAP_MEMORY_H
