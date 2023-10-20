#include "level_cache.h"

#include <algorithm>

level_cache::level_cache()
{
    const int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    transparency_cache_dirty.set();
    outside_cache_dirty = true;
    floor_cache_dirty = false;
    has_lightmap = false;
    has_sunlight_cache = false;
    constexpr four_quadrants four_zeros( 0.0f );
    std::fill_n( &lm[0][0], map_dimensions, four_zeros );
    std::fill_n( &sm[0][0], map_dimensions, 0.0f );
    std::fill_n( &light_source_buffer[0][0], map_dimensions, 0.0f );
    std::fill_n( &outside_cache[0][0], map_dimensions, false );
    std::fill_n( &floor_cache[0][0], map_dimensions, false );
    std::fill_n( &transparency_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &vision_transparency_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &seen_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &camera_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &visibility_cache[0][0], map_dimensions, lit_level::DARK );
    clear_vehicle_cache();
}

bool level_cache::get_veh_in_active_range() const
{
    return !veh_cached_parts.empty();
}

bool level_cache::get_veh_exists_at( const tripoint &pt ) const
{
    return veh_exists_at[ pt.x * MAPSIZE_X + pt.y ];
}

std::pair<vehicle *, int> level_cache::get_veh_cached_parts( const tripoint &pt ) const
{
    auto it = veh_cached_parts.find( pt );
    if( it != veh_cached_parts.end() ) {
        return it->second;
    }
    vehicle *veh = nullptr;
    return std::make_pair( veh, -1 );
}

void level_cache::set_veh_exists_at( const tripoint &pt, bool exists_at )
{
    veh_cache_cleared = false;
    veh_exists_at[ pt.x * MAPSIZE_X + pt.y ] = exists_at;
}

void level_cache::set_veh_cached_parts( const tripoint &pt, vehicle &veh, int part_num )
{
    veh_cache_cleared = false;
    veh_cached_parts[ pt ] = std::make_pair( &veh, part_num );
}

void level_cache::clear_vehicle_cache()
{
    if( veh_cache_cleared ) {
        return;
    }
    veh_exists_at.reset();
    veh_cached_parts.clear();
    veh_cache_cleared = true;
}

void level_cache::clear_veh_from_veh_cached_parts( const tripoint &pt, vehicle *veh )
{
    auto it = veh_cached_parts.find( pt );
    if( it != veh_cached_parts.end() && it->second.first == veh ) {
        veh_cached_parts.erase( it );
    }
}
