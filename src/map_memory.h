#pragma once
#ifndef CATA_SRC_MAP_MEMORY_H
#define CATA_SRC_MAP_MEMORY_H

#include <iosfwd>

#include "game_constants.h"
#include "memory_fast.h"
#include "point.h" // IWYU pragma: keep

class JsonOut;
class JsonIn;
class JsonObject;
class JsonOut;

struct memorized_terrain_tile {
    std::string tile;
    int subtile;
    int rotation;
};

struct memorized_submap {
    memorized_terrain_tile tiles[SEEX][SEEY];
    int symbols[SEEX][SEEY];

    memorized_submap();

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
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

            coord_pair( const tripoint &p );
        };

    public:
        /** Load memorized submaps around given (global) pos. */
        void load( const tripoint &pos );

        /** Load legacy memory file. TODO: remove after 0.F (or whatever BN will have instead). */
        void load_legacy( JsonIn &jsin );

        /** Save memorized submaps to disk, drop far-away ones */
        bool save( const tripoint &pos );

        /**
         * Prepares map memory for rendering and/or memorization of given region.
         * @param p1 top-left corner of the region, in global ms coords
         * @param p2 bottom-right corner of the region, in global ms coords
         * Both coords are inclusive and should be on the same Z level.
         */
        void prepare_region( const tripoint &p1, const tripoint &p2 );

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
        memorized_terrain_tile get_tile( const tripoint &pos ) const;

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
        std::map<tripoint, shared_ptr_fast<memorized_submap>> submaps;

        std::vector<shared_ptr_fast<memorized_submap>> cached;
        tripoint cache_pos;
        point cache_size;

        /** Make sure submap exists within 'submaps' and return its pointer */
        shared_ptr_fast<memorized_submap> fetch_submap( const tripoint &sm_pos );
        /** Load submap from disk. @returns nullptr if failed. */
        shared_ptr_fast<memorized_submap> load_submap( const tripoint &sm_pos );
        /** Allocate empty submap */
        shared_ptr_fast<memorized_submap> allocate_submap();

        /** Get submap from within the cache */
        //@{
        const memorized_submap &get_submap( const tripoint &sm_pos ) const;
        memorized_submap &get_submap( const tripoint &sm_pos );
        //@}
};

#endif // CATA_SRC_MAP_MEMORY_H
