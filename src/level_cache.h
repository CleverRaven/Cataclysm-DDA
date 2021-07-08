#pragma once
#ifndef CATA_SRC_LEVEL_CACHE_H
#define CATA_SRC_LEVEL_CACHE_H

#include <array>
#include <bitset>
#include <set>
#include <unordered_map>
#include <utility>

#include "game_constants.h"
#include "lightmap.h"
#include "point.h"
#include "reachability_cache.h"
#include "shadowcasting.h"
#include "value_ptr.h"

class vehicle;

struct level_cache {
    public:
        // Zeros all relevant values
        level_cache();
        level_cache( const level_cache &other ) = default;

        std::bitset<MAPSIZE *MAPSIZE> transparency_cache_dirty;
        bool outside_cache_dirty = false;
        bool floor_cache_dirty = false;
        bool seen_cache_dirty = false;
        // This is a single value indicating that the entire level is floored.
        bool no_floor_gaps = false;

        four_quadrants lm[MAPSIZE_X][MAPSIZE_Y];
        float sm[MAPSIZE_X][MAPSIZE_Y];
        // To prevent redundant ray casting into neighbors: precalculate bulk light source positions.
        // This is only valid for the duration of generate_lightmap
        float light_source_buffer[MAPSIZE_X][MAPSIZE_Y];

        // if false, means tile is under the roof ("inside"), true means tile is "outside"
        // "inside" tiles are protected from sun, rain, etc. (see "INDOORS" flag)
        bool outside_cache[MAPSIZE_X][MAPSIZE_Y];

        // true when vehicle below has "ROOF" or "OPAQUE" part, furniture below has "SUN_ROOF_ABOVE"
        //      or terrain doesn't have "NO_FLOOR" flag
        // false otherwise
        // i.e. true == has floor
        bool floor_cache[MAPSIZE_X][MAPSIZE_Y];

        // stores cached transparency of the tiles
        // units: "transparency" (see LIGHT_TRANSPARENCY_OPEN_AIR)
        float transparency_cache[MAPSIZE_X][MAPSIZE_Y];

        // materialized  (transparency_cache[i][j] > LIGHT_TRANSPARENCY_SOLID)
        // doesn't consider fields (i.e. if tile is covered in thick smoke, it's still
        // considered transparent for the purpuses of this cache)
        // true, if tile is not opaque
        std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> transparent_cache_wo_fields;

        // stores "adjusted transparency" of the tiles
        // initial values derived from transparency_cache, uses same units
        // examples of adjustment: changed transparency on player's tile and special case for crouching
        float vision_transparency_cache[MAPSIZE_X][MAPSIZE_Y];

        // stores "visibility" of the tiles to the player
        // values range from 1 (fully visible to player) to 0 (not visible)
        float seen_cache[MAPSIZE_X][MAPSIZE_Y];

        // same as `seen_cache` (same units) but contains values for cameras and mirrors
        // effective "visibility_cache" is calculated as "max(seen_cache, camera_cache)"
        float camera_cache[MAPSIZE_X][MAPSIZE_Y];

        // reachability caches
        // Note: indirection here is introduced, because caches are quite large:
        // at least (MAPSIZE_X * MAPSIZE_Y) * 4 bytes (≈69,696 bytes) each
        // so having them directly as part of the level_cache interferes with
        // CPU cache coherency of level_cache
        cata::value_ptr<reachability_cache_horizontal>r_hor_cache =
            cata::make_value<reachability_cache_horizontal>();
        cata::value_ptr<reachability_cache_vertical> r_up_cache =
            cata::make_value<reachability_cache_vertical>();

        // stores resulting apparent brightness to player, calculated by map::apparent_light_at
        lit_level visibility_cache[MAPSIZE_X][MAPSIZE_Y];
        std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_seen_cache;
        std::bitset<MAPSIZE *MAPSIZE> field_cache;

        std::set<vehicle *> vehicle_list;
        std::set<vehicle *> zone_vehicles;

        bool get_veh_in_active_range() const;
        bool get_veh_exists_at( const tripoint &pt ) const;
        std::pair<vehicle *, int> get_veh_cached_parts( const tripoint &pt ) const;

        void set_veh_exists_at( const tripoint &pt, bool exists_at );
        void set_veh_cached_parts( const tripoint &pt, vehicle &veh, int part_num );

        void clear_vehicle_cache();
        void clear_veh_from_veh_cached_parts( const tripoint &pt, vehicle *veh );

    private:
        // Whether the cache is empty or not; if true, nothing has been added to the cache
        // since the most recent call to clear_vehicle_cache()
        bool veh_cache_cleared = true;
        std::bitset<MAPSIZE_X *MAPSIZE_Y> veh_exists_at;
        std::unordered_map<tripoint, std::pair<vehicle *, int>> veh_cached_parts;
};
#endif // CATA_SRC_LEVEL_CACHE_H
