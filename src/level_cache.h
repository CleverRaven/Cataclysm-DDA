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

        cata::mdarray<four_quadrants, point_bub_ms> lm;
        cata::mdarray<float, point_bub_ms> sm;
        // To prevent redundant ray casting into neighbors: precalculate bulk light source positions.
        // This is only valid for the duration of generate_lightmap
        cata::mdarray<float, point_bub_ms> light_source_buffer;

        // Cache of natural light level is useful if it needs to be in sync with the light cache.
        float natural_light_level_cache;

        // if false, means tile is under the roof ("inside"), true means tile is "outside"
        // "inside" tiles are protected from sun, rain, etc. (see ter_furn_flag::TFLAG_INDOORS flag)
        cata::mdarray<bool, point_bub_ms> outside_cache;

        // true when vehicle below has "ROOF" or "OPAQUE" part, furniture below has ter_furn_flag::TFLAG_SUN_ROOF_ABOVE
        //      or terrain doesn't have ter_furn_flag::TFLAG_NO_FLOOR flag
        // false otherwise
        // i.e. true == has floor
        cata::mdarray<bool, point_bub_ms> floor_cache;

        // stores cached transparency of the tiles
        // units: "transparency" (see LIGHT_TRANSPARENCY_OPEN_AIR)
        cata::mdarray<float, point_bub_ms>transparency_cache;

        // materialized  (transparency_cache[i][j] > LIGHT_TRANSPARENCY_SOLID)
        // doesn't consider fields (i.e. if tile is covered in thick smoke, it's still
        // considered transparent for the purpuses of this cache)
        // true, if tile is not opaque
        std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> transparent_cache_wo_fields;

        // stores "adjusted transparency" of the tiles
        // initial values derived from transparency_cache, uses same units
        // examples of adjustment: changed transparency on player's tile and special case for crouching
        cata::mdarray<float, point_bub_ms> vision_transparency_cache;

        // stores "visibility" of the tiles to the player
        // values range from 1 (fully visible to player) to 0 (not visible)
        cata::mdarray<float, point_bub_ms> seen_cache;

        // same as `seen_cache` (same units) but contains values for cameras and mirrors
        // effective "visibility_cache" is calculated as "max(seen_cache, camera_cache)"
        cata::mdarray<float, point_bub_ms> camera_cache;

        // stores resulting apparent brightness to player, calculated by map::apparent_light_at
        cata::mdarray<lit_level, point_bub_ms> visibility_cache;
        std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_dec;
        std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_ter;
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
