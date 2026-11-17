/**
This file is for the most important map cache operations only.
`update_visibility_cache()` is only included because it uses the conditional tiles include.
*/

#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"

#if defined(TILES)
#include "sdltiles.h"
#endif

static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_haslight( "haslight" );
static const efftype_id effect_onfire( "onfire" );

static const field_type_str_id field_fd_clairvoyant( "fd_clairvoyant" );

static level_cache nullcache; // Dummy cache for z-levels outside bounds

std::list<std::unique_ptr<level_cache>> map::free_cache_pool;

std::unique_ptr<level_cache, map::level_cache_free> map::alloc_cache()
{
    std::unique_ptr<level_cache, level_cache_free> cache;
    if( !free_cache_pool.empty() ) {
        cache.reset( free_cache_pool.front().release() );
        free_cache_pool.pop_front();
        cache->clear();
    } else {
        cache.reset( new level_cache{} );
    }
    return cache;
}

void map::invalidate_map_cache( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        level_cache &ch = get_cache( zlev );
        ch.floor_cache_dirty = true;
        ch.seen_cache_dirty = true;
        ch.outside_cache_dirty = true;
        ch.lightmap_dirty = true;
        set_transparency_cache_dirty( zlev );
    }
}

void map::update_visibility_cache( const int zlev )
{
    Character &player_character = get_player_character();
    const avatar &u = get_avatar();
    const tripoint_bub_ms pos = player_character.pos_bub( *this );

    // what apparent_light_at reads that is the same on every level
    visibility_inputs common;
    common.pos = player_character.pos_abs();
    common.fov_generation = seen_cache_generation;
    common.forced = visibility_force_generation;
    common.vision_threshold = player_character.get_vision_threshold( ambient_light_at( pos ) );
    common.clairvoyance = player_character.clairvoyance();
    common.unimpaired_range = player_character.unimpaired_range();
    common.boomered = player_character.has_effect( effect_boomered );
    common.aiming = u.recoil < MAX_RECOIL;
    common.aim_generation = common.aiming ? u.aim_generation() : 0;

    const auto set_variables = [&]( const visibility_inputs & inputs ) {
        visibility_variables_inputs = inputs;
        visibility_variables_cache.g_light_level = inputs.g_light_level;
        visibility_variables_cache.vision_threshold = inputs.vision_threshold;
        visibility_variables_cache.u_clairvoyance = inputs.clairvoyance;
        visibility_variables_cache.u_sight_impaired = player_character.sight_impaired();
        visibility_variables_cache.u_is_boomered = inputs.boomered;
        visibility_variables_cache.clairvoyance_field.reset();
        if( field_fd_clairvoyant.is_valid() ) {
            visibility_variables_cache.clairvoyance_field = field_fd_clairvoyant;
        }
    };
    // lowest level first; the variables are set for zlev after the loop
    for( int z = lowest_vision_level( zlev ); z <= zlev; ++z ) {
        // a level between the avatar's and zlev needn't be one build_map_cache lit
        ensure_light( z );
        level_cache &ch = get_cache( z );
        visibility_inputs inputs = common;
        inputs.lightmap_generation = ch.lightmap_generation;
        inputs.sight_revision = ch.sight_revision;
        inputs.field_revision = ch.field_revision;
        inputs.g_light_level = static_cast<int>( g->light_level( z ) );
        visibility_inputs &last = visibility_keys[z + OVERMAP_DEPTH];
        if( inputs == last ) {
            continue;
        }
        last = inputs;
        set_variables( inputs );

        cata::mdarray<int, point_bub_sm> sm_squares_seen = {};

        ch.visibility_generation = next_cache_generation();
        auto &visibility_cache = ch.visibility_cache;

        if( level_reads_as_blank( z, inputs.clairvoyance ) ) {
            visibility_cache.fill( lit_level::BLANK );
        } else {
            tripoint_bub_ms p;
            p.z() = z;
            int &x = p.x();
            int &y = p.y();
            for( x = 0; x < MAPSIZE_X; x++ ) {
                for( y = 0; y < MAPSIZE_Y; y++ ) {
                    lit_level ll = apparent_light_at( p, visibility_variables_cache );
                    visibility_cache[x][y] = ll;
                    sm_squares_seen[ x / SEEX ][ y / SEEY ] += ( ll == lit_level::BRIGHT || ll == lit_level::LIT );
                }
            }
        }

        for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
            for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                if( sm_squares_seen[gridx][gridy] > 36 ) { // 25% of the submap is visible
                    const tripoint sm( gridx, gridy, 0 );
                    const tripoint_abs_sm abs_sm = map::abs_sub + sm;
                    const tripoint_abs_omt abs_omt = project_to<coords::omt>( abs_sm );
                    overmap_buffer.set_seen( abs_omt, om_vision_level::full );
                }
            }
        }

#if defined(TILES)
        if( !test_mode ) {
            // Mark cata_tiles draw caches as dirty
            tilecontext->set_draw_cache_dirty();
        }
#endif
    }
    // callers read the variables of zlev, whichever levels rebuilt; its key
    // holds what it was last built from, which matches now
    if( !( visibility_variables_inputs == visibility_keys[zlev + OVERMAP_DEPTH] ) ) {
        set_variables( visibility_keys[zlev + OVERMAP_DEPTH] );
    }
    visibility_variables_cache.variables_set = true; // Not used yet
}

void map::build_scene_caches()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int z = minz; z <= maxz; z++ ) {
        build_outside_cache( z );
        build_floor_cache( z );
        level_cache &ch = get_cache( z );
        if( ch.vehicle_opaque_any ) {
            ch.vehicle_opaque_cache.fill( false );
            ch.vehicle_opaque_any = false;
        }
    }
    // needs a separate pass as it changes the caches on neighbour z-levels (e.g. floor_cache);
    // otherwise such changes might be overwritten by main cache-building logic
    for( int z = minz; z <= maxz; z++ ) {
        do_vehicle_caching( z );
    }
    // after vehicles, which mark their own tiles inside and opaque
    for( int z = minz; z <= maxz; z++ ) {
        build_transparency_cache( z );
    }
    scene_build_pending = false;
}

void map::ensure_scene_caches() const
{
    if( scene_build_pending ) {
        // scene caches are memoized state, so a const trace may fill them
        const_cast<map *>( this )->build_scene_caches();
    }
}

void map::build_map_cache( const int zlev, bool skip_lightmap )
{
    const int minz = zlevels ? -OVERMAP_DEPTH : zlev;
    const int maxz = zlevels ? OVERMAP_HEIGHT : zlev;
    build_scene_caches();
    bool seen_cache_dirty = false;
    bool camera_cache_dirty = false;
    for( int z = minz; z <= maxz; z++ ) {
        seen_cache_dirty |= get_cache( z ).seen_cache_dirty;
        seen_cache_dirty |= build_vision_transparency_cache( z );
    }

    avatar &u = get_avatar();
    const Character::moncam_cache_t mcache = u.get_active_moncams();
    // a camera's cast reads its position, range and eye height for ledges; kept
    // per map, so building another map consumes none of this one's changes
    std::vector<camera_fov_input> moncams;
    for( const Character::cached_moncam &mon : mcache ) {
        moncams.push_back( { mon.first, mon.second, mon.first->type->vision_day,
                             static_cast<int>( mon.first->get_size() )
                           } );
    }
    camera_cache_dirty |= moncams != camera_fov_moncams;
    const tripoint_abs_ms p = get_player_character().pos_abs();
    int const sr = u.unimpaired_range();
    // avatar cast also reads eye height for ledges and the mirrors and cameras
    // of the vehicle it stands in
    const float eye = inbounds( p ) ? eye_level( u ) : 0.0f;
    std::vector<int> vision_parts = inbounds( p ) ? vision_parts_key( get_bub( p ), sr ) :
                                    std::vector<int>();
    seen_cache_dirty |= avatar_fov_pos != p || sr != avatar_fov_range || eye != avatar_fov_eye_level ||
                        vision_parts != avatar_fov_vision_parts || camera_cache_dirty;
    if( seen_cache_dirty ) {
        // avatar cast writes vehicle cameras, all cameras merge into what's
        // left, so nothing an old camera saw may survive
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            level_cache &ch = get_cache( z );
            if( camera_levels_written[z + OVERMAP_DEPTH] ) {
                ch.camera_cache.fill( LIGHT_TRANSPARENCY_SOLID );
            }
            for( std::bitset<MAPSIZE_Y> &row : ch.ledge_hidden ) {
                row.reset();
            }
        }
        camera_levels_written.reset();
        seen_cache_generation = next_cache_generation();
        if( inbounds( p ) ) {
            // avatar's own level, whichever level the caller asked for
            build_seen_cache( get_bub( p ), p.z(), sr, false, 0, eye );
        }
        avatar_fov_pos = p;
        avatar_fov_range = sr;
        avatar_fov_eye_level = eye;
        avatar_fov_vision_parts = std::move( vision_parts );
        camera_cache_dirty = true;
#if defined(TILES)
        if( !test_mode ) {
            // Mark cata_tiles draw caches as dirty
            tilecontext->set_draw_cache_dirty();
        }
#endif
    }
    if( camera_cache_dirty ) {
        camera_fov_moncams = std::move( moncams );
        for( Character::cached_moncam const &mon : mcache ) {
            if( inbounds( mon.second ) ) {
                int const range = mon.first->type->vision_day;
                build_seen_cache( get_bub( mon.second ), mon.second.z(), range, true,
                                  std::max( MAX_VIEW_DISTANCE - range, 0 ), eye_level( *mon.first ) );
            }
        }
    }
    // Light sources that change with no notice to the lightmap: characters,
    // glowing or burning monsters, and vehicle lamps that lose power or break.
    // A level relights only if its own sources changed.
    {
        auto character_light = []( const Character & ch ) -> light_source_state {
            float light = ch.active_light();
            if( ch.has_effect( effect_onfire ) )
            {
                light += 8.0f;
            } else if( ch.has_effect( effect_haslight ) )
            {
                light += 4.0f;
            }
            return { light, ch.pos_bub(), 0 };
        };

        std::vector<light_source_state> current_lights;
        current_lights.push_back( character_light( get_player_character() ) );
        for( const npc &guy : g->all_npcs() ) {
            current_lights.push_back( character_light( guy ) );
        }
        for( const monster &critter : g->all_monsters() ) {
            if( critter.is_hallucination() ) {
                continue;
            }
            const float light = critter.luminance() + ( critter.has_effect( effect_onfire ) ? 8.0f : 0.0f );
            if( light > 0 ) {
                current_lights.push_back( { light, critter.pos_bub( *this ), 0 } );
            }
        }
        // the level caches' vehicle lists, as walking every submap costs more
        // than the rest of a stationary build
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
            const level_cache *ch = get_cache_lazy( z );
            if( ch == nullptr ) {
                continue;
            }
            for( vehicle *v : ch->vehicle_list ) {
                const std::vector<vehicle_part *> lights = v->lights();
                // a cone light shines at what all of them add up to, so losing one
                // dims the rest, on any level
                const float cone = vehicle::cone_light_luminance( lights );
                for( const vehicle_part *pt : lights ) {
                    const vpart_info &info = pt->info();
                    const bool shares = info.has_flag( VPFLAG_CONE_LIGHT ) ||
                                        info.has_flag( VPFLAG_WIDE_CONE_LIGHT );
                    const int dir = std::lround( units::to_degrees( v->face.dir() + pt->direction ) );
                    current_lights.push_back( { shares ? cone : static_cast<float>( info.bonus ),
                                                v->bub_part_pos( *this, *pt ), dir } );
                }
            }
        }
        if( current_lights != cached_light_sources ) {
            const auto on_level = []( const std::vector<light_source_state> &all, const int z ) {
                std::vector<light_source_state> level;
                for( const light_source_state &s : all ) {
                    if( s.pos.z() == z ) {
                        level.push_back( s );
                    }
                }
                return level;
            };
            std::set<int> levels;
            for( const light_source_state &s : cached_light_sources ) {
                levels.insert( s.pos.z() );
            }
            for( const light_source_state &s : current_lights ) {
                levels.insert( s.pos.z() );
            }
            for( const int z : levels ) {
                if( on_level( cached_light_sources, z ) != on_level( current_lights, z ) ) {
                    set_lightmap_cache_dirty( z );
                }
            }
            cached_light_sources = std::move( current_lights );
        }
    }

    if( seen_cache_dirty ) {
        refresh_vision_levels();
    }
    if( !skip_lightmap ) {
        update_sunlight();
        generate_lightmap( zlev );
        for( const int z : vision_levels_list ) {
            generate_lightmap( z );
        }
        // a level this build didn't light drops the sources it held, which
        // nothing would rebuild while its sunlight stays the same
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
            if( get_cache_ref( z ).light_full && z != zlev &&
                std::find( vision_levels_list.begin(), vision_levels_list.end(), z ) == vision_levels_list.end() ) {
                set_sunlight_only( z );
            }
        }
    }
}

level_cache &map::access_cache( int zlev )
{
    if( zlev >= -OVERMAP_DEPTH && zlev <= OVERMAP_HEIGHT ) {
        std::unique_ptr<level_cache, level_cache_free> &cache = caches[zlev + OVERMAP_DEPTH];
        if( !cache ) {
            cache = alloc_cache();
        }
        return *cache;
    }

    debugmsg( "access_cache called with invalid z-level: %d", zlev );
    return nullcache;
}

const level_cache &map::access_cache( int zlev ) const
{
    if( zlev >= -OVERMAP_DEPTH && zlev <= OVERMAP_HEIGHT ) {
        std::unique_ptr<level_cache, level_cache_free> &cache = caches[zlev + OVERMAP_DEPTH];
        if( !cache ) {
            cache = alloc_cache();
        }
        return *cache;
    }

    debugmsg( "access_cache called with invalid z-level: %d", zlev );
    return nullcache;
}
