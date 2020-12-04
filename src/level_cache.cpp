#include "level_cache.h"

#include <cstdlib>

#include "vehicle.h"

level_cache::level_cache()
{
    const int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    transparency_cache_dirty.set();
    outside_cache_dirty = true;
    floor_cache_dirty = false;
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
    veh_in_active_range = false;
    std::fill_n( &veh_exists_at[0][0], map_dimensions, false );
}

bool level_cache::get_veh_in_active_range() const
{
    return veh_in_active_range;
}

bool level_cache::get_veh_exists_at( const tripoint &pt ) const
{
    return veh_exists_at[ pt.x ][ pt.y ];
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

void level_cache::set_veh_in_active_range( bool is_active )
{
    veh_in_active_range = is_active;
    veh_cache_dirty = true;
    veh_cache_active |= is_active;
}

void level_cache::set_veh_exists_at( const tripoint &pt, bool exists_at )
{
    veh_exists_at[ pt.x ][ pt.y ] = exists_at;
    veh_cache_dirty = true;
    veh_cache_active |= exists_at;
}

void level_cache::set_veh_cached_parts( const tripoint &pt, vehicle *veh, int part_num )
{
    veh_cached_parts[ pt ] = std::make_pair( veh, part_num );
    veh_cache_dirty = true;
    veh_cache_active |= veh != nullptr;
}

void level_cache::verify_vehicle_cache()
{
    if( !veh_cache_dirty ) {
        return;
    }

    const bool old_viar = veh_in_active_range;
    veh_in_active_range &= std::any_of( std::begin( veh_exists_at ),
    std::end( veh_exists_at ), []( const auto & row ) {
        return std::any_of( std::begin( row ), std::end( row ), []( bool veh_exists ) {
            return veh_exists;
        } );
    } );
    // update the cache dirty bit if anything changed
    veh_cache_dirty |= old_viar != veh_in_active_range;
    veh_cache_active |= veh_in_active_range;
}

void level_cache::clear_vehicle_cache()
{
    if( !veh_cache_active ) {
        return;
    }

    std::memset( veh_exists_at, 0, sizeof( veh_exists_at ) );
    veh_cached_parts.clear();
    veh_in_active_range = false;
    veh_cache_dirty = false;
    veh_cache_active = false;
}

void level_cache::clear_veh_from_veh_cached_parts( const tripoint &pt, vehicle *veh )
{
    auto it = veh_cached_parts.find( pt );
    if( it != veh_cached_parts.end() && it->second.first == veh ) {
        veh_cached_parts.erase( it );
        veh_cache_dirty = true;
    }
}
