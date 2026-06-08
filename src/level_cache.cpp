#include "level_cache.h"

#include <cstring>

level_cache::level_cache()
{
    clear();
}

void level_cache::clear()
{
    // Blast zeroes over the entire region. Some compilers, looking at you msvc, aren't smart enough
    // to turn this into a single fused memset. Sometimes it doesn't even optimize fill_n into memset
    // but emits scalar loops. This murders map test performance.
#pragma GCC diagnostic push
    // It's safe to disable this because we static_assert that level_cache_default_zero_members is
    // trivially copyable, and we know all zeros bit pattern is a legal representation for the type.
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    std::memset( static_cast<level_cache_default_zero_members *>( this ), 0,
                 sizeof( level_cache_default_zero_members ) );
#pragma GCC diagnostic pop

    transparency_cache_dirty.set();
    outside_cache_dirty = true;
    floor_cache_dirty = false;
    seen_cache_dirty = false;
    lightmap_dirty = true;
    has_colored_lights = false;
    no_floor_gaps = false;

    natural_light_level_cache = 0.0f;

    vehicle_list.clear();
    zone_vehicles.clear();

    clear_vehicle_cache();
}

bool level_cache::get_veh_in_active_range() const
{
    return !veh_cached_parts.empty();
}

bool level_cache::get_veh_exists_at( const tripoint_bub_ms &pt ) const
{
    return veh_exists_at[ pt.x() * MAPSIZE_X + pt.y()];
}

std::pair<vehicle *, int> level_cache::get_veh_cached_parts( const tripoint_bub_ms &pt ) const
{
    auto it = veh_cached_parts.find( pt );
    if( it != veh_cached_parts.end() ) {
        return it->second;
    }
    vehicle *veh = nullptr;
    return std::make_pair( veh, -1 );
}

void level_cache::set_veh_exists_at( const tripoint_bub_ms &pt, bool exists_at )
{
    veh_cache_cleared = false;
    veh_exists_at[ pt.x() * MAPSIZE_X + pt.y()] = exists_at;
}

void level_cache::set_veh_cached_parts( const tripoint_bub_ms &pt, vehicle &veh, int part_num )
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

void level_cache::clear_veh_from_veh_cached_parts( const tripoint_bub_ms &pt, vehicle *veh )
{
    auto it = veh_cached_parts.find( pt );
    if( it != veh_cached_parts.end() && it->second.first == veh ) {
        veh_cached_parts.erase( it );
    }
}
