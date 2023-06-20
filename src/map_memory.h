#pragma once
#ifndef CATA_SRC_MAP_MEMORY_H
#define CATA_SRC_MAP_MEMORY_H

#include <iosfwd>

#include "game_constants.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "point.h" // IWYU pragma: keep

class JsonObject;
class JsonOut;
class JsonValue;

struct ter_t;
using ter_str_id = string_id<ter_t>;

class memorized_tile
{
    public:
        char32_t symbol = 0;

        const std::string &get_ter_id() const;
        const std::string &get_dec_id() const;
        void set_ter_id( std::string_view id );
        void set_dec_id( std::string_view id );

        int get_ter_rotation() const;
        void set_ter_rotation( int rotation );
        int get_dec_rotation() const;
        void set_dec_rotation( int rotation );

        int get_ter_subtile() const;
        void set_ter_subtile( int subtile );
        int get_dec_subtile() const;
        void set_dec_subtile( int subtile );

        bool operator==( const memorized_tile &rhs ) const;
        bool operator!=( const memorized_tile &rhs ) const {
            return !( *this == rhs );
        }
    private:
        friend struct mm_submap; // serialization needs access to private members
        ter_str_id ter_id;       // terrain tile id
        std::string dec_id;      // decoration tile id (furniture, vparts ...)
        int8_t ter_rotation = 0;
        int8_t dec_rotation = 0;
        int8_t ter_subtile = 0;
        int8_t dec_subtile = 0;
};

/** Represent a submap-sized chunk of tile memory. */
struct mm_submap {
    public:
        static const memorized_tile default_tile;

        mm_submap() = default;
        explicit mm_submap( bool make_valid );

        // @returns true if mm_submap is empty; empty submaps are skipped during saving.
        bool is_empty() const;
        // @returns true if mm_submap is valid, i.e. not returned from an uninitialized region.
        bool is_valid() const;

        const memorized_tile &get_tile( const point &p ) const;
        void set_tile( const point &p, const memorized_tile &value );

        void serialize( JsonOut &jsout ) const;
        void deserialize( int version, const JsonArray &ja );

    private:
        // NOLINTNEXTLINE(cata-serialize)
        std::vector<memorized_tile> tiles; // holds either 0 or SEEX*SEEY elements
        // NOLINTNEXTLINE(cata-serialize)
        bool valid = true;
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
         * Returns memorized tile.
         * @param pos tile position, in global ms coords.
         */
        const memorized_tile &get_tile( const tripoint &pos ) const;

        /**
         * Memorizes terrain at \p pos, overwriting old terrain values.
         * @param pos tile position, in global ms coords.
         */
        void set_tile_terrain( const tripoint &pos, std::string_view id, int subtile, int rotation );

        /**
         * Memorizes decoraiton at \p pos, overwriting old decoration values.
         * @param pos tile position, in global ms coords.
         */
        void set_tile_decoration( const tripoint &pos, std::string_view id, int subtile, int rotation );

        /**
         * Memorizes symbol at \p pos, overwriting old symbol.
         * @param pos tile position, in global ms coords.
        */
        void set_tile_symbol( const tripoint &pos, char32_t symbol );

        /**
         * Clears memorized vehicles and symbol.
         * @param pos tile position, in global ms coords.
         */
        void clear_tile_vehicles( const tripoint &pos );

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
