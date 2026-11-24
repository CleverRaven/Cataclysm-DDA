/**
* For map class: vision, light, map memory, and scent
*/

#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdlib>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"

#include "map_scale_constants.h"
#include "mapdata.h"
#include "mdarray.h"
#include "point.h"
#include "submap.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

void map::set_seen_cache_dirty( const tripoint_bub_ms &change_location )
{
    if( inbounds( change_location ) ) {
        level_cache &cache = get_cache( change_location.z() );
        if( cache.seen_cache_dirty ) {
            return;
        }
        if( cast_reached( change_location ) ) {
            cache.seen_cache_dirty = true;
        }
    }
}

void map::set_seen_cache_dirty( const int zlevel )
{
    if( inbounds_z( zlevel ) ) {
        level_cache &cache = get_cache( zlevel );
        cache.seen_cache_dirty = true;
    }
}

bool map::cast_reached( const tripoint_bub_ms &p ) const
{
    const level_cache &ch = get_cache_ref( p.z() );
    return ch.seen_cache[p.x()][p.y()] > 0.0f || ch.camera_cache[p.x()][p.y()] > 0.0f ||
           ch.ledge_hidden[p.x()][p.y()];
}

void map::set_lightmap_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_cache( zlev ).lightmap_dirty = true;
    }
}

void map::set_lightmap_cache_dirty_below( const int zlev )
{
    for( int z = zlev; z >= -OVERMAP_DEPTH; z-- ) {
        if( inbounds_z( z ) ) {
            get_cache( z ).lightmap_dirty = true;
        }
    }
}

bool map::memory_cache_dec_is_dirty( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "memory_cache_dec_is_dirty called on out of bounds position" );
        return true;
    }
    return !get_cache( p.z() ).map_memory_cache_dec[p.x() + p.y() * MAPSIZE_Y];
}

void map::memory_cache_dec_set_dirty( const tripoint_bub_ms &p, bool value ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "memory_cache_dec_set_dirty called on out of bounds position" );
        return;
    }
    level_cache &ch = get_cache( p.z() );
    ch.map_memory_cache_dec[p.x() + p.y() * MAPSIZE_Y] = !value;
    ch.map_memory_sweep_pending |= value;
}

bool map::memory_cache_ter_is_dirty( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "memory_cache_ter_is_dirty called on out of bounds position" );
        return true;
    }
    return !get_cache( p.z() ).map_memory_cache_ter[p.x() + p.y() * MAPSIZE_Y];
}

void map::memory_cache_ter_set_dirty( const tripoint_bub_ms &p, bool value ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "memory_cache_ter_set_dirty called on out of bounds position" );
        return;
    }
    level_cache &ch = get_cache( p.z() );
    ch.map_memory_cache_ter[p.x() + p.y() * MAPSIZE_Y] = !value;
    ch.map_memory_sweep_pending |= value;
}

void map::memory_clear_vehicle_points( const vehicle &veh ) const
{
    avatar &player_character = get_avatar();
    for( const tripoint_abs_ms &p : veh.get_points() ) {
        if( !inbounds( p ) ) {
            continue;
        }
        memory_cache_dec_set_dirty( get_bub( p ), true );
        player_character.memorize_clear_decoration( p, "vp_" );
    }
}

void map::mark_turn_light_dirty()
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        if( get_cache_ref( z ).light_changes_by_turn ) {
            set_lightmap_cache_dirty( z );
        }
    }
}

bool map::level_reads_as_blank( const int zlev, const int clairvoyance ) const
{
    // apparent_light_at answers BLANK for every tile no cast reaches, unless
    // clairvoyance or a clairvoyant field lights it
    if( clairvoyance > 0 ) {
        return false;
    }
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *sm = get_submap_at_grid( tripoint_rel_sm{ smx, smy, zlev } );
            if( sm != nullptr && sm->field_count > 0 ) {
                return false;
            }
        }
    }
    return !level_reached_by_cast( zlev );
}

void map::invalidate_visibility_cache()
{
    visibility_force_generation = next_cache_generation();
}

const visibility_variables &map::get_visibility_variables_cache() const
{
    return visibility_variables_cache;
}

visibility_type map::get_visibility( const lit_level ll,
                                     const visibility_variables &cache ) const
{
    switch( ll ) {
        case lit_level::DARK:
            // can't see this square at all
            if( cache.u_is_boomered ) {
                return visibility_type::BOOMER_DARK;
            } else {
                return visibility_type::DARK;
            }
        case lit_level::BRIGHT_ONLY:
            // can only tell that this square is bright
            if( cache.u_is_boomered ) {
                return visibility_type::BOOMER;
            } else {
                return visibility_type::LIT;
            }

        case lit_level::LOW:
        // low light, square visible in monochrome
        case lit_level::LIT:
        // normal light
        case lit_level::BRIGHT:
            // bright light
            return visibility_type::CLEAR;
        case lit_level::BLANK:
        case lit_level::MEMORIZED:
            if( cache.u_is_boomered ) {
                return visibility_type::BOOMER_DARK;
            } else {
                return visibility_type::HIDDEN;
            }
    }
    return visibility_type::HIDDEN;
}

bool map::sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, const int range,
                bool with_fields, los_trace trace ) const
{
    int dummy = 0;
    return sees( F, T, range, dummy, with_fields, true, trace );
}

map::lru_cache_t &map::skew_vision_cache_for( const los_trace trace, const bool with_fields ) const
{
    return skew_vision_caches[static_cast<int>( trace ) * 2 + ( with_fields ? 0 : 1 )];
}

// TODO: Change this to a hash function on the map implementation. This will also allow us to
// account for the complete lack of entropy in the top 16 bits.
point map::sees_cache_key( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const
{
    // Canonicalize the order of the tripoints so the cache is reflexive.
    const tripoint_bub_ms &min = from < to ? from : to;
    const tripoint_bub_ms &max = !( from < to ) ? from : to;

    // A little gross, just pack the values into an integer.
    return point(
               min.x() << 20 | min.y() << 10 | ( min.z() + OVERMAP_DEPTH ),
               max.x() << 20 | max.y() << 10 | ( max.z() + OVERMAP_DEPTH )
           );
}

/**
 * This one is internal-only, we don't want to expose the slope tweaking ickiness outside the map class.
 **/
bool map::sees( const tripoint_bub_ms &F, const tripoint_bub_ms &T, const int range,
                int &bresenham_slope, bool with_fields, bool allow_cached, los_trace trace ) const
{
    ensure_scene_caches();
    if( skew_vision_scene_stamp != last_scene_change ) {
        for( lru_cache_t &cache : skew_vision_caches ) {
            cache.clear();
        }
        skew_vision_scene_stamp = last_scene_change;
    }
    bool ( map:: * f_transparent )( const tripoint_bub_ms & p ) const =
        trace == los_trace::optical ?
        ( with_fields ? &map::is_sight_clear : &map::is_sight_clear_wo_fields ) :
        ( with_fields ? &map::is_transparent : &map::is_transparent_wo_fields );
    lru_cache_t &skew_cache = skew_vision_cache_for( trace, with_fields );
    if( std::abs( F.z() - T.z() ) > fov_3d_z_range ||
        ( range >= 0 && range < rl_dist( F, T ) ) ||
        !inbounds( T ) ) {
        bresenham_slope = 0;
        return false; // Out of range!
    }
    const point key = sees_cache_key( F, T );
    if( allow_cached ) {
        char cached = skew_cache.get( key, -1 );
        if( cached != -1 ) {
            return cached > 0;
        }
    }
    bool visible = true;

    // Ugly `if` for now
    if( F.z() == T.z() ) {
        bresenham( F.xy(), T.xy(), bresenham_slope,
        [this, f_transparent, &visible, &T]( const point_bub_ms & new_point ) {
            // Exit before checking the last square, it's still visible even if opaque.
            if( new_point.x() == T.x() && new_point.y() == T.y() ) {
                return false;
            }
            if( !( this->*f_transparent )( { new_point.x(), new_point.y(), T.z()} ) ) {
                visible = false;
                return false;
            }
            return true;
        } );
        if( allow_cached ) {
            skew_cache.insert( 100000, key, visible ? 1 : 0 );
        }
        return visible;
    }

    tripoint_bub_ms last_point = F;
    bresenham( F, T, bresenham_slope, 0,
    [this, f_transparent, &visible, &T, &last_point]( const tripoint_bub_ms & new_point ) {
        // Exit before checking the last square if it's not a vertical transition,
        // it's still visible even if opaque.
        if( new_point == T && last_point.z() == T.z() ) {
            return false;
        }

        // TODO: Allow transparent floors (and cache them!)
        if( new_point.z() == last_point.z() ) {
            if( !( this->*f_transparent )( new_point ) ) {
                visible = false;
                return false;
            }
        } else {
            const int max_z = std::max( new_point.z(), last_point.z() );
            if( ( has_floor_or_support( { point_bub_ms( new_point.xy() ), max_z } ) ||
                  !( this->*f_transparent )( { point_bub_ms( new_point.xy() ), last_point.z()} ) ) &&
                ( has_floor_or_support( { point_bub_ms( last_point.xy() ), max_z } ) ||
                  !( this->*f_transparent )( { point_bub_ms( last_point.xy() ), new_point.z()} ) ) ) {
                visible = false;
                return false;
            }
        }

        last_point = new_point;
        return true;
    } );
    if( allow_cached ) {
        skew_cache.insert( 100000, key, visible ? 1 : 0 );
    }
    return visible;
}

int map::obstacle_coverage( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2 ) const
{
    // Can't hide if you are standing on furniture, or non-flat slowing-down terrain tile.
    if( furn( loc2 ).obj().id || ( move_cost( loc2 ) > 2 &&
                                   !has_flag_ter( ter_furn_flag::TFLAG_FLAT, loc2 ) ) ) {
        return 0;
    }
    const point a( std::abs( loc1.x() - loc2.x() ) * 2, std::abs( loc1.y() - loc2.y() ) * 2 );
    int offset = std::min( a.x, a.y ) - ( std::max( a.x, a.y ) / 2 );
    tripoint_bub_ms obstaclepos;
    bresenham( loc2, loc1, offset, 0, [&obstaclepos]( const tripoint_bub_ms & new_point ) {
        // Only adjacent tile between you and enemy is checked for cover.
        obstaclepos = new_point;
        return false;
    } );
    if( const furn_id &obstacle_f = furn( tripoint_bub_ms( obstaclepos ) ) ) {
        return obstacle_f->coverage;
    }
    if( const optional_vpart_position vp = veh_at( tripoint_bub_ms( obstaclepos ) ) ) {
        if( vp->obstacle_at_part() ) {
            return 60;
        } else if( !vp->part_with_feature( VPFLAG_AISLE, true ) ) {
            return 45;
        }
    }
    return ter( obstaclepos )->coverage;
}

int map::ledge_coverage( const Creature &viewer, const tripoint_bub_ms &target_p ) const
{
    return ledge_coverage( viewer.pos_bub( *this ), target_p, eye_level( viewer ) );
}

float map::eye_level( const Creature &viewer ) const
{
    creature_size viewer_size = viewer.get_size();

    // Viewer eye level from ground in grids
    float eye_level = 1.0f;
    switch( viewer_size ) {
        case creature_size::medium:
            break;
        case creature_size::tiny:
            eye_level = 0.4f;
            break;
        case creature_size::small:
            eye_level = 0.7f;
            break;
        case creature_size::large:
            eye_level = 1.3f;
            break;
        case creature_size::huge:
            eye_level = 1.6f;
            break;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Creature has invalid size class." );
            break;
    }
    // Viewer eye level crouch / prone multipliers
    const Character *viewer_ch = viewer.as_character();
    if( viewer_ch ) {
        if( viewer_ch->is_crouching() ) {
            eye_level *= 0.5;
        } else if( viewer_ch->is_prone() ) {
            eye_level *= 0.275;
        }
    }
    // Viewer eye level is higher when standing on furniture
    const furn_id &viewer_furn = furn( viewer.pos_bub( *this ) );
    if( viewer_furn.obj().id ) {
        eye_level += viewer_furn->coverage * 0.01f;
    }
    return eye_level;
}

int map::ledge_coverage( const tripoint_bub_ms &viewer_p, const tripoint_bub_ms &target_p,
                         const float &eye_level ) const
{
    if( viewer_p.z() == target_p.z() ) {
        return 0;
    }

    // Find ledge between viewer and target
    // Only the first ledge found is calculated for performance reasons
    tripoint_bub_ms high_p;
    tripoint_bub_ms low_p;
    if( viewer_p.z() > target_p.z() ) {
        high_p = viewer_p;
        low_p = target_p;
    } else {
        high_p = target_p;
        low_p = viewer_p;
    }
    tripoint_bub_ms ledge_p = high_p;
    bresenham( tripoint_bub_ms( low_p.xy(), high_p.z() ), high_p, 0, 0, [this,
    &ledge_p]( const tripoint_bub_ms & new_point ) {
        if( dont_draw_lower_floor( new_point ) ) {
            ledge_p = new_point;
            return false;
        }
        return true;
    } );

    // Height of each z-level in grids
    const float zlevel_to_grid_ratio = 2.0f;
    float dist_to_ledge_base = trig_dist( viewer_p, tripoint_bub_ms( ledge_p.x(), ledge_p.y(),
                                          viewer_p.z() ) );
    // Adjustment to ledge distance because ledge is assumed to be between two grids
    dist_to_ledge_base += ( viewer_p.z() < target_p.z() ) ? -0.5f : 0.5f;
    const float flat_dist = trig_dist( viewer_p, tripoint_bub_ms( target_p.xy(), viewer_p.z() ) );
    // Absolute level of viewer's eye
    const float abs_eye_z = viewer_p.z() * zlevel_to_grid_ratio + eye_level;
    // "Opposite" of the angle between the eye level and ledge
    const float eye_ledge_z_delta = ( ledge_p.z() * zlevel_to_grid_ratio ) - abs_eye_z;
    const float tangent = eye_ledge_z_delta / dist_to_ledge_base;
    // Absolute level concealed by ledge, anything below this point is invisible
    const float covered_z = abs_eye_z + ( tangent * flat_dist );
    // Ledge coverage given by comparing covered_z and the absolute z of the target space
    float ledge_coverage = ( covered_z - target_p.z() * zlevel_to_grid_ratio ) * 100;

    // Early exit if the tile is definitely not covered
    if( ledge_coverage < 0 ) {
        return 0;
    }
    // Target has a coverage penalty when standing on furniture
    const furn_id &target_furn = furn( target_p );
    if( target_furn ) {
        ledge_coverage -= target_furn->coverage;
    }

    return std::max( ledge_coverage, 0.0f );
}

int map::coverage( const tripoint_bub_ms &p ) const
{
    if( const optional_vpart_position vp = veh_at( p ) ) {
        if( vp->obstacle_at_part() ) {
            return 60;
        }
        std::optional<vpart_reference> part = vp->part_with_feature( VPFLAG_OBSTACLE, true, true );
        if( !( part && part->has_feature( VPFLAG_OPENABLE ) && part->part().open ) &&
            !vp->part_with_feature( VPFLAG_AISLE, true ) ) {
            return 45;
        }
    }
    if( const furn_id &obstacle_f = furn( p ) ) {
        return obstacle_f->coverage;
    }
    return ter( p )->coverage;
}

std::vector<int> map::vision_parts_key( const tripoint_bub_ms &origin,
                                        const int extension_range ) const
{
    std::vector<int> key;
    const optional_vpart_position vp = veh_at( origin );
    if( !vp ) {
        return key;
    }
    vehicle &veh = vp->vehicle();
    key.push_back( veh.camera_on ? 1 : 0 );
    for( const vpart_reference &part : veh.get_all_parts_with_fakes() ) {
        if( part.part().removed || part.part().is_broken() ||
            !part.info().has_flag( VPFLAG_EXTENDS_VISION ) ) {
            continue;
        }
        const tripoint_bub_ms pos = part.pos_bub( *this );
        if( rl_dist( origin, pos ) > extension_range ) {
            continue;
        }
        key.insert( key.end(), { pos.x(), pos.y(), pos.z(), static_cast<int>( part.part_index() ),
                                 part.part().hp()
                               } );
    }
    return key;
}

int map::lowest_vision_level( const int zlev ) const
{
    // avatar's view reaches fov_3d_z_range levels down from its own
    const int avatar_z = get_avatar().posz();
    return std::max( std::min( zlev, avatar_z - fov_3d_z_range ), -OVERMAP_DEPTH );
}

bool map::level_reached_by_cast( const int zlev ) const
{
    // every cast moves seen_cache_generation, so one scan per level per cast
    if( level_reached_generation != seen_cache_generation ) {
        level_reached.fill( -1 );
        level_reached_generation = seen_cache_generation;
    }
    int8_t &reached = level_reached[zlev + OVERMAP_DEPTH];
    if( reached < 0 ) {
        reached = 0;
        const level_cache &ch = get_cache_ref( zlev );
        for( int x = 0; x < MAPSIZE_X && reached == 0; ++x ) {
            for( int y = 0; y < MAPSIZE_Y; ++y ) {
                if( ch.seen_cache[x][y] > 0.0f || ch.camera_cache[x][y] > 0.0f ) {
                    reached = 1;
                    break;
                }
            }
        }
    }
    return reached > 0;
}

void map::refresh_vision_levels()
{
    const int avatar_z = get_avatar().posz();
    vision_levels_list.assign( 1, avatar_z );
    // a level below the avatar's that no cast reaches reads no light, since
    // final visibility there is blank whatever its lightmap holds
    for( int z = avatar_z - 1; z >= lowest_vision_level( avatar_z ); --z ) {
        if( level_reached_by_cast( z ) ) {
            vision_levels_list.push_back( z );
        }
    }
}

const std::vector<int> &map::vision_levels() const
{
    return vision_levels_list;
}

void map::update_sunlight()
{
    std::vector<float> natural_light;
    natural_light.reserve( OVERMAP_HEIGHT + 1 );
    for( int z = 0; z <= OVERMAP_HEIGHT; ++z ) {
        natural_light.push_back( g->natural_light_level( z ) );
    }
    std::vector<uint64_t> revisions;
    revisions.reserve( 2 * OVERMAP_LAYERS );
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        const level_cache &ch = get_cache_ref( z );
        revisions.push_back( ch.sight_revision );
        revisions.push_back( ch.geometry_revision );
    }
    if( natural_light == sunlight_natural_light && revisions == sunlight_scene_revisions ) {
        return;
    }
    build_sunlight_cache();
    sunlight_natural_light = std::move( natural_light );
    sunlight_scene_revisions = std::move( revisions );
}

void map::rebuild_vision_caches_from_scratch( const int zlev )
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        invalidate_map_cache( z );
        level_cache &ch = get_cache( z );
        ch.vision_observer_overrides.clear();
        // what a cast doesn't reach must read as unseen, not a stale answer
        ch.seen_cache.fill( LIGHT_TRANSPARENCY_SOLID );
        ch.camera_cache.fill( LIGHT_TRANSPARENCY_SOLID );
    }
    for( lru_cache_t &skew_cache : skew_vision_caches ) {
        skew_cache.clear();
    }
    avatar_fov_range = -1;
    avatar_fov_vision_parts.clear();
    camera_fov_moncams.clear();
    sunlight_scene_revisions.clear();
    cached_light_sources.clear();
    g->reset_light_level();
    build_map_cache( zlev );
    // light for every level final visibility reads, whatever build_map_cache
    // chose to refresh
    refresh_vision_levels();
    update_sunlight();
    for( const int z : vision_levels_list ) {
        get_cache( z ).lightmap_dirty = true;
        generate_lightmap( z );
    }
    invalidate_visibility_cache();
    update_visibility_cache( zlev );
}

template<typename Functor>
void map::function_over( const tripoint_bub_ms &start, const tripoint_bub_ms &end,
                         Functor fun ) const
{
    // start and end are just two points, end can be "before" start
    // Also clip the area to map area
    const tripoint_bub_ms min( std::max( std::min( start.x(), end.x() ), 0 ),
                               std::max( std::min( start.y(), end.y() ),
                                         0 ), std::max( std::min( start.z(), end.z() ), -OVERMAP_DEPTH ) );
    const tripoint_bub_ms max( std::min( std::max( start.x(), end.x() ), SEEX * my_MAPSIZE - 1 ),
                               std::min( std::max( start.y(), end.y() ), SEEY * my_MAPSIZE - 1 ), std::min( std::max( start.z(),
                                       end.z() ),
                                       OVERMAP_HEIGHT ) );

    // Submaps that contain the bounding points
    const point_bub_sm min_sm( min.x() / SEEX, min.y() / SEEY );
    const point_bub_sm max_sm( max.x() / SEEX, max.y() / SEEY );
    // Z outermost, because submaps are flat
    tripoint_rel_sm gp;
    int &z = gp.z();
    int &smx = gp.x();
    int &smy = gp.y();
    for( z = min.z(); z <= max.z(); z++ ) {
        for( smx = min_sm.x(); smx <= max_sm.x(); smx++ ) {
            for( smy = min_sm.y(); smy <= max_sm.y(); smy++ ) {
                submap const *cur_submap = get_submap_at_grid( { smx, smy, z } );
                if( cur_submap == nullptr ) {
                    debugmsg( "Tried to function over (%d,%d,%d) but the submap is not loaded", smx, smy, z );
                    continue;
                }
                // Bounds on the submap coordinates
                const point_bub_sm sm_min( smx > min_sm.x() ? 0 : min.x() % SEEX,
                                           smy > min_sm.y() ? 0 : min.y() % SEEY );
                const point_bub_sm sm_max( smx < max_sm.x() ? SEEX - 1 : max.x() % SEEX,
                                           smy < max_sm.y() ? SEEY - 1 : max.y() % SEEY );

                point_sm_ms lp;
                int &sx = lp.x();
                int &sy = lp.y();
                for( sx = sm_min.x(); sx <= sm_max.x(); ++sx ) {
                    for( sy = sm_min.y(); sy <= sm_max.y(); ++sy ) {
                        const iteration_state rval = fun( gp, cur_submap, lp );
                        if( rval != ITER_CONTINUE ) {
                            switch( rval ) {
                                case ITER_SKIP_ZLEVEL:
                                    smx = my_MAPSIZE + 1;
                                    smy = my_MAPSIZE + 1;
                                    [[fallthrough]];
                                case ITER_SKIP_SUBMAP:
                                    sx = SEEX;
                                    sy = SEEY;
                                    break;
                                default:
                                    return;
                            }
                        }
                    }
                }
            }
        }
    }
}

void map::scent_blockers( std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &blocks_scent,
                          std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &reduces_scent,
                          const point_bub_ms &min, const point_bub_ms &max )
{
    ter_furn_flag reduce = ter_furn_flag::TFLAG_REDUCE_SCENT;
    ter_furn_flag block = ter_furn_flag::TFLAG_NO_SCENT;
    auto fill_values = [&]( const tripoint_rel_sm & gp, const submap * sm, const point_sm_ms & lp ) {
        // We need to generate the x/y coordinates, because we can't get them "for free"
        const point_sm_ms p = lp + coords::project_to<coords::ms>( gp.xy() );
        if( sm->get_ter( lp ).obj().has_flag( block ) ) {
            blocks_scent[p.x()][p.y()] = true;
            reduces_scent[p.x()][p.y()] = false;
        } else if( sm->get_ter( lp ).obj().has_flag( reduce ) ||
                   sm->get_furn( lp ).obj().has_flag( reduce ) ) {
            blocks_scent[p.x()][p.y()] = false;
            reduces_scent[p.x()][p.y()] = true;
        } else {
            blocks_scent[p.x()][p.y()] = false;
            reduces_scent[p.x()][p.y()] = false;
        }

        return ITER_CONTINUE;
    };

    function_over( tripoint_bub_ms( min, abs_sub.z() ), tripoint_bub_ms( max, abs_sub.z() ),
                   fill_values );

    const inclusive_rectangle<point_bub_ms> local_bounds( min, max );

    // Now vehicles

    VehicleList vehs = get_vehicles();
    for( wrapped_vehicle &wrapped_veh : vehs ) {
        vehicle &veh = *( wrapped_veh.v );
        for( const vpart_reference &vp : veh.get_all_parts_with_fakes() ) {
            if( !vp.has_feature( VPFLAG_OBSTACLE ) &&
                ( !vp.has_feature( VPFLAG_OPENABLE ) || !vp.part().open ) ) {
                continue;
            }
            const tripoint_bub_ms part_pos = vp.pos_bub( *this );
            if( local_bounds.contains( part_pos.xy() ) ) {
                reduces_scent[part_pos.x()][part_pos.y()] = true;
            }
        }
    }
}

bool map::has_potential_los( const tripoint_bub_ms &from, const tripoint_bub_ms &to ) const
{
    ensure_scene_caches();
    if( skew_vision_scene_stamp != last_scene_change ) {
        return true;
    }
    const point key = sees_cache_key( from, to );
    char cached = skew_vision_cache_for( los_trace::optical, true ).get( key, -1 );
    if( cached != -1 ) {
        return cached > 0;
    }
    return true;
}
