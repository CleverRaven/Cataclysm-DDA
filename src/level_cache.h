#pragma once
#ifndef CATA_SRC_LEVEL_CACHE_H
#define CATA_SRC_LEVEL_CACHE_H

#include <array>
#include <bitset>
#include <cstdint>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "lightmap.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "shadowcasting.h"

// IWYU pragma: no_forward_declare four_quadrants
class vehicle;

// process wide count: equal generations mean the same cache state, even
// across map instances. 64 bits do not wrap in practice
uint64_t next_cache_generation();

// level_cache is a huge struct and some compilers need help to zero these
// members efficiently. Break them into a separate struct which can be memset
// in a single call instead of a bunch of fill_n which don't optimize well.
struct level_cache_default_zero_members {
    cata::mdarray<four_quadrants, point_bub_ms> lm;
    // sunlight alone, which every level's lm starts from; only the sunlight
    // pass writes it
    cata::mdarray<four_quadrants, point_bub_ms> sun_lm;
    cata::mdarray<float, point_bub_ms> sm;
    // Accumulated colored light energy per tile. Populated during generate_lightmap
    // alongside lm/sm. Zero = uncolored (white) light only.
    cata::mdarray<light_color_rgb, point_bub_ms> light_color_cache;
    // Bulk light source buffer. Scalar luminance uses max() for dedup;
    // color accumulates additively. Only valid during generate_lightmap.
    struct buffered_light_source {
        buffered_light_source() = default;
        float luminance;
        light_color_rgb color;
    };
    cata::mdarray<buffered_light_source, point_bub_ms> light_source_buffer;
    // if false, means tile is under the roof ("inside"), true means tile is "outside"
    // "inside" tiles are protected from sun, rain, etc. (see ter_furn_flag::TFLAG_INDOORS flag)
    cata::mdarray<bool, point_bub_ms> outside_cache;

    // false where this level's terrain has a floor gap flag (see has_floor_gap_flag:
    //      NO_FLOOR, NO_FLOOR_WATER, GOES_DOWN, TRANSPARENT_FLOOR), unless furniture
    //      below has ter_furn_flag::TFLAG_SUN_ROOF_ABOVE
    // true otherwise, and where a boardable vehicle part on this level or a ROOF or
    //      OPAQUE part below closes the gap
    // sight and sunlight cross between this level and the one below only where false
    cata::mdarray<bool, point_bub_ms> floor_cache;

    // stores cached transparency of the tiles
    // units: "transparency" (see LIGHT_TRANSPARENCY_OPEN_AIR)
    cata::mdarray<float, point_bub_ms>transparency_cache;

    // materialized  (transparency_cache[i][j] > LIGHT_TRANSPARENCY_SOLID)
    // doesn't consider fields (i.e. if tile is covered in thick smoke, it's still
    // considered transparent for the purpuses of this cache)
    // true, if tile is not opaque
    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> transparent_cache_wo_fields;

    // sight through the tiles for every observer: transparency_cache, but solid
    // where TRANSLUCENT terrain or furniture passes light and blocks sight
    cata::mdarray<float, point_bub_ms> sight_cache;
    // sight_cache without fields as a bitset; true when sight passes
    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> sight_cache_wo_fields;
    // tiles an opaque vehicle part covers, rewritten every build
    cata::mdarray<bool, point_bub_ms> vehicle_opaque_cache;
    // outside_cache and vehicle_opaque_cache as transparency build last read
    // them; a difference marks the submaps it covers dirty
    cata::mdarray<bool, point_bub_ms> transparency_outside;
    cata::mdarray<bool, point_bub_ms> transparency_vehicle_opaque;

    // stores "adjusted transparency" of the tiles
    // initial values derived from sight_cache, uses same units
    // the cover a crouching or prone avatar hides behind
    cata::mdarray<float, point_bub_ms> vision_transparency_cache;

    // stores "visibility" of the tiles to the player
    // values range from 1 (fully visible to player) to 0 (not visible)
    cata::mdarray<float, point_bub_ms> seen_cache;

    // same units as `seen_cache`: what monster and vehicle cameras transmit, each
    // cast on its own and max-merged; mirrors the avatar sees go into seen_cache
    // final classification reads max(seen_cache, camera_cache) with light and
    // observer state
    cata::mdarray<float, point_bub_ms> camera_cache;

    // tiles a cast reached that the ledge pass then hid; with seen_cache and
    // camera_cache they are what a cast read, so changes elsewhere leave it alone
    std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> ledge_hidden;

    // stores resulting apparent brightness to player, calculated by map::apparent_light_at
    cata::mdarray<lit_level, point_bub_ms> visibility_cache;
    std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_dec;
    std::bitset<MAPSIZE_X *MAPSIZE_Y> map_memory_cache_ter;
    std::bitset<MAPSIZE *MAPSIZE> field_cache;
};
// The only way to memset the above without UB is if it is trivially copyable
// and that all zeros is a valid representation. We can't assert the latter.
static_assert( std::is_trivially_copyable_v<level_cache_default_zero_members> );

struct level_cache : level_cache_default_zero_members {
    public:
        // Zeros all relevant values
        level_cache();
        level_cache( const level_cache &other ) = default;

        void clear();

        std::bitset<MAPSIZE *MAPSIZE> transparency_cache_dirty;
        // submaps the transparency build rewrote since the vision build last
        // copied them
        std::bitset<MAPSIZE *MAPSIZE> vision_transparency_dirty;
        // cells of vision_transparency_cache the observer overlay last wrote,
        // with the value; they go back to scene value when overlay moves
        std::vector<std::pair<point_bub_ms, float>> vision_observer_overrides;
        // weather sight penalty transparency build last applied
        float built_sight_penalty = -1.0f;
        // outside_cache written since transparency build last read it
        bool outside_rewritten = true;
        // vehicle_opaque_cache, or the copy transparency build last read, holds
        // an opaque tile; with neither, comparing is pointless
        bool vehicle_opaque_any = false;
        bool transparency_vehicle_opaque_any = false;
        // set from next_cache_generation when a value in the four transparency
        // and sight caches changes
        uint64_t sight_revision = 0;
        // set from next_cache_generation when terrain, furniture, floors or
        // vehicle parts on this level change; ledges and vertical sight read them
        uint64_t geometry_revision = 0;
        // set from next_cache_generation whenever a field on the level changes
        uint64_t field_revision = 0;
        bool outside_cache_dirty = false;
        bool floor_cache_dirty = false;
        bool seen_cache_dirty = false;
        bool lightmap_dirty = true;
        // set from next_cache_generation each time the level's light grids are
        // replaced, by a full build or by sunlight alone; a reader compares it
        // to tell the light changed
        uint64_t lightmap_generation = 0;
        // set from next_cache_generation when the sunlight pass rewrites sun_lm
        uint64_t sun_revision = 0;
        // lm also holds the level's light sources, not just sunlight
        bool light_full = false;
        // last full build found a source that can change with no notice: a
        // blinking lamp or a lit item
        bool light_changes_by_turn = false;
        // dawn, dusk and weather tint the last full build applied
        light_color_rgb light_tint{};
        // light of every tile when the level lies above all populated ones and
        // the sunlight pass left sun_lm unwritten; negative otherwise
        float sun_uniform = -1.0f;
        // sun_revision of the sun_lm lm was last built from
        uint64_t lightmap_sun_revision = 0;
        // set from next_cache_generation each time update_visibility_cache
        // recomputes visibility_cache
        uint64_t visibility_generation = 0;
        // set by any write that dirties map_memory_cache_dec or _ter on this
        // level; a tiles memorize sweep of this level is due while set
        bool map_memory_sweep_pending = true;
        // True when at least one light source on this z-level has non-white color.
        // Used to skip the color blur pass when all lights are white.
        bool has_colored_lights = false;
        // This is a single value indicating that the entire level is floored.
        bool no_floor_gaps = false;

        // Cache of natural light level is useful if it needs to be in sync with the light cache.
        float natural_light_level_cache = 0.0f;

        std::set<vehicle *> vehicle_list;
        std::set<vehicle *> zone_vehicles;

        bool get_veh_in_active_range() const;
        bool get_veh_exists_at( const tripoint_bub_ms &pt ) const;
        std::pair<vehicle *, int> get_veh_cached_parts( const tripoint_bub_ms &pt ) const;

        void set_veh_exists_at( const tripoint_bub_ms &pt, bool exists_at );
        void set_veh_cached_parts( const tripoint_bub_ms &pt, vehicle &veh, int part_num );

        void clear_vehicle_cache();
        void clear_veh_from_veh_cached_parts( const tripoint_bub_ms &pt, vehicle *veh );

    private:
        // Whether the cache is empty or not; if true, nothing has been added to the cache
        // since the most recent call to clear_vehicle_cache()
        bool veh_cache_cleared = true;
        std::bitset<MAPSIZE_X *MAPSIZE_Y> veh_exists_at;
        std::unordered_map<tripoint_bub_ms, std::pair<vehicle *, int>> veh_cached_parts;
};
#endif // CATA_SRC_LEVEL_CACHE_H
