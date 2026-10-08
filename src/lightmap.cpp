#include "lightmap.h" // IWYU pragma: associated
#include "shadowcasting.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "field.h"
#include "fragment_cloud.h" // IWYU pragma: keep
#include "game.h"
#include "item.h"
#include "item_stack.h"
#include "level_cache.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "math_defines.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "point.h"
#include "string_formatter.h"
#include "submap.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_type.h"

static const dimension_id dimension_world_default( "default" );

static const efftype_id effect_haslight( "haslight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_quadruped_full( "quadruped_full" );

static constexpr int LIGHTMAP_CACHE_X = MAPSIZE_X;
static constexpr int LIGHTMAP_CACHE_Y = MAPSIZE_Y;

static constexpr point_bub_ms lightmap_boundary_min{};
static constexpr point_bub_ms lightmap_boundary_max( LIGHTMAP_CACHE_X, LIGHTMAP_CACHE_Y );

static const half_open_rectangle<point_bub_ms> lightmap_boundaries(
    lightmap_boundary_min, lightmap_boundary_max );

std::string four_quadrants::to_string() const
{
    return string_format( "(%.2f,%.2f,%.2f,%.2f)",
                          ( *this )[quadrant::NE], ( *this )[quadrant::SE],
                          ( *this )[quadrant::SW], ( *this )[quadrant::NW] );
}

// Dawn/dusk tint: cached per turn, returns the warm color for twilight
// or an empty color outside twilight. Only depends on calendar::turn.
static light_color_rgb cached_twilight_color()
{
    static time_point cached_turn = calendar::before_time_starts;
    static light_color_rgb cached_color{};
    if( cached_turn == calendar::turn ) {
        return cached_color;
    }
    cached_turn = calendar::turn;
    if( !is_twilight( calendar::turn ) ) {
        cached_color = {};
        return cached_color;
    }
    const units::angle alt = sun_azimuth_altitude( calendar::turn ).second;
    constexpr float lo = -6.0f;
    constexpr float hi = -1.0f;
    const float progress = std::clamp(
                               static_cast<float>( to_degrees( alt ) - lo ) / ( hi - lo ), 0.0f, 1.0f );
    const float hue = 25.0f + progress * 20.0f;
    const float ease = std::sin( progress * M_PI );
    const float tint_strength = ease * 0.35f;
    cached_color = light_color_rgb::from_hsv( hue, 0.8f, 1.0f ) * tint_strength;
    return cached_color;
}

static light_color_rgb cached_weather_color()
{
    static time_point cached_turn = calendar::before_time_starts;
    static light_color_rgb cached_color{};
    if( cached_turn == calendar::turn ) {
        return cached_color;
    }
    cached_turn = calendar::turn;

    const weather_type_id &wid = get_weather().weather_id;
    if( !wid->tint_color.is_colored() || wid->tint_strength <= 0.0f ) {
        cached_color = {};
        return cached_color;
    }

    cached_color = wid->tint_color * wid->tint_strength;
    return cached_color;
}

light_color_rgb dawn_dusk_color_for_lightmap( dimension_id dimension )
{
    if( dimension != dimension_world_default ) {
        return {};
    }
    return cached_twilight_color();
}

void map::add_item_light_recursive( const tripoint_bub_ms &p, const item &it )
{
    float ilum = 0.0f; // brightness
    units::angle iwidth = 0_degrees; // 0-360 degrees. 0 is a circular light_source
    units::angle idir = 0_degrees;   // otherwise, it's a light_arc pointed in this direction
    if( it.getlight( ilum, iwidth, idir ) ) {
        if( iwidth > 0_degrees ) {
            apply_light_arc( p, idir, ilum, iwidth );
        } else {
            add_light_source( p, ilum );
        }
    }

    for( const item_pocket *pkt : it.get_container_pockets() ) {
        if( pkt->transparent() ) {
            for( const item *cont : pkt->all_items_top() ) {
                add_item_light_recursive( p, *cont );
            }
        }
    }
}

void map::add_light_from_items( const tripoint_bub_ms &p, const item_stack &items )
{
    for( const item &it : items ) {
        add_item_light_recursive( p, it );
    }
}

// TRANSLUCENT terrain or furniture passes light but blocks sight
static bool blocks_sight( const ter_t &ter, const furn_t &furn )
{
    return ter.has_flag( ter_furn_flag::TFLAG_TRANSLUCENT ) ||
           furn.has_flag( ter_furn_flag::TFLAG_TRANSLUCENT );
}

// marks dirty the submaps where an input of the transparency build differs from
// the copy it last read
static void mark_changed_submaps_dirty( level_cache &map_cache,
                                        const cata::mdarray<bool, point_bub_ms> &input,
                                        const cata::mdarray<bool, point_bub_ms> &consumed, const int map_size )
{
    if( std::memcmp( &input, &consumed, sizeof( input ) ) == 0 ) {
        return;
    }
    for( int smx = 0; smx < map_size; ++smx ) {
        for( int smy = 0; smy < map_size; ++smy ) {
            for( int sx = 0; sx < SEEX; ++sx ) {
                const int x = sx + smx * SEEX;
                if( !std::equal( &input[x][smy * SEEY], &input[x][smy * SEEY] + SEEY,
                                 &consumed[x][smy * SEEY] ) ) {
                    map_cache.transparency_cache_dirty.set( smx * MAPSIZE + smy );
                    break;
                }
            }
        }
    }
}

// TODO: Consider making this just clear the cache and dynamically fill it in as is_transparent() is called
bool map::build_transparency_cache( const int zlev )
{
    level_cache &map_cache = get_cache( zlev );
    auto &transparent_cache_wo_fields = map_cache.transparent_cache_wo_fields;
    auto &transparency_cache = map_cache.transparency_cache;
    auto &sight_cache = map_cache.sight_cache;
    auto &sight_cache_wo_fields = map_cache.sight_cache_wo_fields;
    const auto &outside_cache = map_cache.outside_cache;

    const cata::mdarray<bool, point_bub_ms> &vehicle_opaque = map_cache.vehicle_opaque_cache;

    // weather, shelter and vehicles are inputs too but nothing marks them dirty,
    // so compare with what last build read
    const float sight_penalty = get_weather().weather_id->sight_penalty;
    if( sight_penalty != map_cache.built_sight_penalty ) {
        map_cache.transparency_cache_dirty.set();
    } else {
        if( map_cache.outside_rewritten ) {
            mark_changed_submaps_dirty( map_cache, outside_cache, map_cache.transparency_outside,
                                        my_MAPSIZE );
        }
        if( map_cache.vehicle_opaque_any || map_cache.transparency_vehicle_opaque_any ) {
            mark_changed_submaps_dirty( map_cache, vehicle_opaque, map_cache.transparency_vehicle_opaque,
                                        my_MAPSIZE );
        }
    }
    map_cache.outside_rewritten = false;

    if( map_cache.transparency_cache_dirty.none() ) {
        return false;
    }

    // compared whole after the pass: a per-tile check costs every tile of a
    // weather rebuild, and weather never changes this array
    const std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X> wo_fields_before = transparent_cache_wo_fields;
    bool changed = false;
    const auto write = [&]( const point_bub_ms & p, const float light, const bool light_wo_fields,
    const float sight, const bool sight_wo_fields ) {
        const bool tile_changed = transparency_cache[p.x()][p.y()] != light ||
                                  transparent_cache_wo_fields[p.x()][p.y()] != light_wo_fields ||
                                  sight_cache[p.x()][p.y()] != sight ||
                                  sight_cache_wo_fields[p.x()][p.y()] != sight_wo_fields;
        changed |= tile_changed;
        // only a change where a cast reached alters what any cast sees
        if( tile_changed && cast_reached( tripoint_bub_ms( p, zlev ) ) ) {
            map_cache.seen_cache_dirty = true;
        }
        transparency_cache[p.x()][p.y()] = light;
        transparent_cache_wo_fields[p.x()][p.y()] = light_wo_fields;
        sight_cache[p.x()][p.y()] = sight;
        sight_cache_wo_fields[p.x()][p.y()] = sight_wo_fields;
    };

    // Traverse the submaps in order
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            if( !map_cache.transparency_cache_dirty[smx * MAPSIZE + smy] ) {
                continue;
            }
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{smx, smy, zlev} );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build transparency cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          zlev );
                continue;
            }
            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const point_sm_ms sp( sx, sy );
                    const point_bub_ms p( sx + smx * SEEX, sy + smy * SEEY );
                    const ter_t &ter = cur_submap->get_ter( sp ).obj();
                    const furn_t &furn = cur_submap->get_furn( sp ).obj();
                    if( vehicle_opaque[p.x()][p.y()] || !( ter.transparent && furn.transparent ) ) {
                        write( p, LIGHT_TRANSPARENCY_SOLID, false, LIGHT_TRANSPARENCY_SOLID, false );
                        continue;
                    }
                    float value = LIGHT_TRANSPARENCY_OPEN_AIR;
                    if( outside_cache[p.x()][p.y()] ) {
                        value *= sight_penalty;
                    }
                    const float value_wo_fields = value;
                    for( const auto &fld : cur_submap->get_field( sp ) ) {
                        const field_intensity_level &i_level = fld.second.get_intensity_level();
                        if( i_level.transparent ) {
                            continue;
                        }
                        // Fields are either transparent or not, however we want some to be translucent
                        value = value * i_level.translucency;
                    }
                    // TODO: [lightmap] Have glass reduce light as well.
                    const bool sight_blocked = blocks_sight( ter, furn );
                    write( p, value, value_wo_fields > LIGHT_TRANSPARENCY_SOLID,
                           sight_blocked ? LIGHT_TRANSPARENCY_SOLID : value,
                           !sight_blocked && value_wo_fields > LIGHT_TRANSPARENCY_SOLID );
                }
            }
        }
    }

    map_cache.transparency_outside = outside_cache;
    map_cache.transparency_vehicle_opaque = vehicle_opaque;
    map_cache.transparency_vehicle_opaque_any = map_cache.vehicle_opaque_any;
    map_cache.built_sight_penalty = sight_penalty;
    map_cache.vision_transparency_dirty |= map_cache.transparency_cache_dirty;
    map_cache.transparency_cache_dirty.reset();
    if( changed ) {
        map_cache.sight_revision = next_cache_generation();
        last_scene_change = map_cache.sight_revision;
    }
    // creature zones flood through tiles clear without fields, and vehicle
    // doors change them with no tile write
    if( transparent_cache_wo_fields != wo_fields_before && this == &reality_bubble() ) {
        get_creature_tracker().invalidate_reachability_cache();
    }
    return changed;
}

std::vector<std::pair<point_bub_ms, float>> map::observer_vision_overrides( const int zlev ) const
{
    std::vector<std::pair<point_bub_ms, float>> overrides;
    const Character &player_character = get_player_character();
    const tripoint_bub_ms p = player_character.pos_bub( *this );
    if( p.z() != zlev || !inbounds( p ) ) {
        return overrides;
    }
    // This segment handles vision when the player is crouching or prone. It only checks adjacent tiles.
    // If you change this, also consider creature::sees and map::obstacle_coverage.
    // only the avatar's own cast and display read these cells; other observers trace
    // sight_cache, so cover the avatar crouches behind hides nothing from them
    const bool low_profile = player_character.has_effect( effect_quadruped_full ) &&
                             player_character.is_running();
    if( player_character.is_crouching() || player_character.is_prone() || low_profile ) {
        for( const tripoint_bub_ms &loc : points_in_radius( p, 1 ) ) {
            if( loc != p && coverage( loc ) >= 30 ) {
                overrides.emplace_back( loc.xy(), LIGHT_TRANSPARENCY_SOLID );
            }
        }
    }
    return overrides;
}

bool map::build_vision_transparency_cache( int zlev )
{
    level_cache &map_cache = get_cache( zlev );
    const cata::mdarray<float, point_bub_ms> &sight_cache = map_cache.sight_cache;
    cata::mdarray<float, point_bub_ms> &vision_transparency_cache = map_cache.vision_transparency_cache;
    std::vector<std::pair<point_bub_ms, float>> &applied = map_cache.vision_observer_overrides;
    std::vector<std::pair<point_bub_ms, float>> wanted = observer_vision_overrides( zlev );

    const bool scene_dirty = map_cache.vision_transparency_dirty.any();
    if( !scene_dirty && wanted == applied ) {
        return false;
    }

    // what last cast saw on cells either overlay touches
    std::vector<std::pair<point_bub_ms, float>> before;
    before.reserve( applied.size() + wanted.size() );
    const auto remember = [&]( const std::vector<std::pair<point_bub_ms, float>> &cells ) {
        for( const std::pair<point_bub_ms, float> &cell : cells ) {
            before.emplace_back( cell.first, vision_transparency_cache[cell.first.x()][cell.first.y()] );
        }
    };
    remember( applied );
    remember( wanted );

    // last overlay comes off first, so the copy below compares scene with scene
    for( const std::pair<point_bub_ms, float> &cell : applied ) {
        const point_bub_ms &p = cell.first;
        vision_transparency_cache[p.x()][p.y()] = sight_cache[p.x()][p.y()];
    }

    bool dirty = false;
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            if( !map_cache.vision_transparency_dirty[smx * MAPSIZE + smy] ) {
                continue;
            }
            for( int sx = 0; sx < SEEX; ++sx ) {
                const int x = sx + smx * SEEX;
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const int y = sy + smy * SEEY;
                    dirty |= vision_transparency_cache[x][y] != sight_cache[x][y] &&
                             cast_reached( tripoint_bub_ms( x, y, zlev ) );
                    vision_transparency_cache[x][y] = sight_cache[x][y];
                }
            }
        }
    }
    map_cache.vision_transparency_dirty.reset();

    for( const std::pair<point_bub_ms, float> &cell : wanted ) {
        vision_transparency_cache[cell.first.x()][cell.first.y()] = cell.second;
    }
    for( const std::pair<point_bub_ms, float> &cell : before ) {
        dirty |= vision_transparency_cache[cell.first.x()][cell.first.y()] != cell.second;
    }
    applied = std::move( wanted );
    return dirty;
}

void map::apply_character_light( Character &p )
{
    const tripoint_bub_ms pos = p.pos_bub( *this );
    if( !inbounds( pos ) ) {
        return;
    }
    if( p.has_effect( effect_onfire ) ) {
        apply_light_source( pos, 8 );
    } else if( p.has_effect( effect_haslight ) ) {
        apply_light_source( pos, 4 );
    }

    const float held_luminance = p.active_light();
    if( held_luminance > LIGHT_AMBIENT_LOW ) {
        apply_light_source( pos, held_luminance );
    }

    if( held_luminance >= 4 && held_luminance > ambient_light_at( pos ) - 0.5f ) {
        p.add_effect( effect_haslight, 1_turns );
    }
}

// This function raytraces starting at the upper limit of the simulated area descending
// toward the lower limit. Since it's sunlight, the rays are parallel.
// Each layer consults the next layer up to determine the intensity of the light that reaches it.
// Once this is complete, additional operations add more dynamic lighting.
void map::build_sunlight_cache()
{
    const int zlev_min = -OVERMAP_DEPTH;
    // Start at the topmost populated zlevel to avoid unnecessary raycasting
    // Plus one zlevel to prevent clipping inside structures
    const int zlev_max = std::min( calc_max_populated_zlev() + 1, OVERMAP_HEIGHT );
    // levels above that are open sky: one value each, no grid to fill
    for( int zlev = OVERMAP_HEIGHT; zlev > zlev_max; zlev-- ) {
        level_cache &map_cache = get_cache( zlev );
        map_cache.natural_light_level_cache = g->natural_light_level( zlev );
        map_cache.sun_revision = next_cache_generation();
        map_cache.sun_uniform = g->natural_light_level( 0 );
    }

    // true if all previous z-levels are fully transparent to light (no floors, transparency >= air)
    bool fully_outside = true;

    // true if no light reaches this level, i.e. there were no lit tiles on the above level (light level <= inside_light_level)
    bool fully_inside = false;

    // fully_outside and fully_inside define following states:
    // initially: fully_outside=true, fully_inside=false  (fast fill)
    //    ↓
    // when first obstacles occur: fully_outside=false, fully_inside=false  (slow quadrant logic)
    //    ↓
    // when fully below ground: fully_outside=false, fully_inside=true  (fast fill)

    // Iterate top to bottom because sunlight cache needs to construct in that order.
    for( int zlev = zlev_max; zlev >= zlev_min; zlev-- ) {
        level_cache &map_cache = get_cache( zlev );
        map_cache.natural_light_level_cache = g->natural_light_level( zlev );
        map_cache.sun_revision = next_cache_generation();
        map_cache.sun_uniform = -1.0f;
        auto &lm = map_cache.sun_lm;
        // Grab illumination at ground level.
        const float outside_light_level = g->natural_light_level( 0 );
        // TODO: if zlev < 0 is open to sunlight, this won't calculate correct light, but neither does g->natural_light_level()
        const float inside_light_level = ( zlev >= 0 && outside_light_level > LIGHT_SOURCE_BRIGHT ) ?
                                         LIGHT_AMBIENT_DIM * 0.8 : LIGHT_AMBIENT_LOW;

        // all light was blocked before
        if( fully_inside ) {
            std::fill_n( &lm[0][0], MAPSIZE_X * MAPSIZE_Y, four_quadrants( inside_light_level ) );
            continue;
        }

        // If there were no obstacles before this level, just apply weather illumination since there's no opportunity
        // for light to be blocked.
        if( fully_outside ) {
            //fill with full light
            std::fill_n( &lm[0][0], MAPSIZE_X * MAPSIZE_Y, four_quadrants( outside_light_level ) );

            const auto &this_floor_cache = map_cache.floor_cache;
            const auto &this_transparency_cache = map_cache.transparency_cache;
            fully_inside = true; // recalculate

            for( int x = 0; x < MAPSIZE_X; ++x ) {
                for( int y = 0; y < MAPSIZE_Y; ++y ) {
                    // && semantics below is important, we want to skip the evaluation if possible, do not replace with &=

                    // fully_outside stays true if tile is transparent and there is no floor
                    fully_outside = fully_outside && this_transparency_cache[x][y] >= LIGHT_TRANSPARENCY_OPEN_AIR
                                    && !this_floor_cache[x][y];
                    // fully_inside stays true if tile is opaque OR there is floor
                    fully_inside = fully_inside && ( this_transparency_cache[x][y] <= LIGHT_TRANSPARENCY_SOLID ||
                                                     this_floor_cache[x][y] );
                }
            }
            continue;
        }

        // Replace this with a calculated shift based on time of day and date.
        // At first compress the angle such that it takes no more than one tile of shift per level.
        // To exceed that, we'll have to handle casting light from the side instead of the top.
        const level_cache &prev_map_cache = get_cache_ref( zlev + 1 );
        const auto &prev_lm = prev_map_cache.sun_lm;
        const auto &prev_transparency_cache = prev_map_cache.transparency_cache;
        const auto &prev_floor_cache = prev_map_cache.floor_cache;
        const auto &outside_cache = map_cache.outside_cache;
        const float sight_penalty = get_weather().weather_id->sight_penalty;
        // TODO: Replace these with a lookup inside the four_quadrants class.
        constexpr std::array<point, 5> cardinals = {
            { point::zero, point::north, point::west, point::east, point::south }
        };
        constexpr std::array<std::array<quadrant, 2>, 5> dir_quadrants = {{
                {{quadrant::NE, quadrant::NW}},
                {{quadrant::NE, quadrant::NW}},
                {{quadrant::SW, quadrant::NW}},
                {{quadrant::SE, quadrant::NE}},
                {{quadrant::SE, quadrant::SW}},
            }
        };

        fully_inside = true; // recalculate

        // Fall back to minimal light level if we don't find anything.
        std::fill_n( &lm[0][0], MAPSIZE_X * MAPSIZE_Y, four_quadrants( inside_light_level ) );

        for( int x = 0; x < MAPSIZE_X; ++x ) {
            for( int y = 0; y < MAPSIZE_Y; ++y ) {
                // Check center, then four adjacent cardinals.
                for( int i = 0; i < 5; ++i ) {
                    point prev( cardinals[i] + point( x, y ) );
                    bool inbounds = prev.x >= 0 && prev.x < MAPSIZE_X &&
                                    prev.y >= 0 && prev.y < MAPSIZE_Y;

                    if( !inbounds ) {
                        continue;
                    }

                    float prev_light_max;
                    float prev_transparency = prev_transparency_cache[prev.x][prev.y];
                    // This is pretty gross, this cancels out the per-tile transparency effect
                    // derived from weather.
                    if( outside_cache[x][y] ) {
                        prev_transparency /= sight_penalty;
                    }

                    if( prev_transparency > LIGHT_TRANSPARENCY_SOLID &&
                        !prev_floor_cache[prev.x][prev.y] &&
                        ( prev_light_max = prev_lm[prev.x][prev.y].max() ) > 0.0 ) {
                        const float light_level = clamp( prev_light_max * LIGHT_TRANSPARENCY_OPEN_AIR / prev_transparency,
                                                         inside_light_level, prev_light_max );

                        if( i == 0 ) {
                            lm[x][y].fill( light_level );
                            fully_inside &= light_level <= inside_light_level;
                            break;
                        } else {
                            fully_inside &= light_level <= inside_light_level;
                            lm[x][y][dir_quadrants[i][0]] = light_level;
                            lm[x][y][dir_quadrants[i][1]] = light_level;
                        }
                    }
                }
            }
        }
    }
}

void map::ensure_light( const int zlev ) const
{
    // light built from the current sunlight stays valid as of the last
    // build_map_cache, full or not, like the views
    if( get_cache_ref( zlev ).lightmap_sun_revision != get_cache_ref( zlev ).sun_revision ) {
        set_sunlight_only( zlev );
    }
}

void map::set_sunlight_only( const int zlev ) const
{
    // a level no cast reaches gets sunlight alone, as every level did before
    // light was built per level; build_map_cache adds the sources where a cast
    // reached
    level_cache &ch = get_cache( zlev );
    if( ch.sun_uniform >= 0.0f ) {
        ch.lm.fill( four_quadrants( ch.sun_uniform ) );
    } else {
        ch.lm = ch.sun_lm;
    }
    ch.sm.fill( 0 );
    ch.light_color_cache.fill( light_color_rgb{} );
    ch.has_colored_lights = false;
    ch.light_full = false;
    ch.lightmap_sun_revision = ch.sun_revision;
    ch.lightmap_generation = next_cache_generation();
}

void map::generate_lightmap( const int zlev )
{
    level_cache &map_cache = get_cache( zlev );
    // Dawn/dusk tint: color sunlit tiles during twilight.
    // Weather tint: sunlight can be tinted by active weather, respecting dawn/dusk
    const light_color_rgb twilight_tint =
        dawn_dusk_color_for_lightmap( g->get_dimension_prefix() );
    const light_color_rgb weather_tint = cached_weather_color();
    const light_color_rgb ddc = {
        std::max( twilight_tint.r, weather_tint.r ),
        std::max( twilight_tint.g, weather_tint.g ),
        std::max( twilight_tint.b, weather_tint.b )
    };
    // no writer marks a tint change dirty, so the level keeps the tint it was lit with
    const bool same_tint = ddc.r == map_cache.light_tint.r && ddc.g == map_cache.light_tint.g &&
                           ddc.b == map_cache.light_tint.b;
    if( map_cache.light_full && !map_cache.lightmap_dirty && same_tint &&
        map_cache.lightmap_sun_revision == map_cache.sun_revision ) {
        return;
    }
    map_cache.light_tint = ddc;
    map_cache.light_full = true;
    map_cache.light_changes_by_turn = false;
    map_cache.lightmap_dirty = false;
    map_cache.lightmap_sun_revision = map_cache.sun_revision;

    auto &lm = map_cache.lm;
    auto &sm = map_cache.sm;
    auto &outside_cache = map_cache.outside_cache;
    auto &prev_floor_cache = get_cache( clamp( zlev + 1, -OVERMAP_DEPTH, OVERMAP_DEPTH ) ).floor_cache;
    bool top_floor = zlev == OVERMAP_DEPTH;
    // artificial light goes on top of what the sunlight pass left
    if( map_cache.sun_uniform >= 0.0f ) {
        lm.fill( four_quadrants( map_cache.sun_uniform ) );
    } else {
        lm = map_cache.sun_lm;
    }
    sm.fill( 0 );
    map_cache.light_color_cache.fill( light_color_rgb{} );
    map_cache.has_colored_lights = false;

    /* Bulk light sources wastefully cast rays into neighbors; a burning hospital can produce
         significant slowdown, so for stuff like fire and lava:
     * Step 1: Store the position and luminance in buffer via add_light_source, for efficient
         checking of neighbors. Color rides the same buffer additively.
     * Step 2: After everything else, iterate buffer and apply_light_source only in non-redundant
         directions, propagating both scalar light and color in the same octant decisions.
     * Step 3: ????
     * Step 4: Profit!
     */
    auto &light_source_buffer = map_cache.light_source_buffer;
    light_source_buffer.fill( level_cache::buffered_light_source{} );

    constexpr std::array<int, 4> dir_x = { {  0, -1, 1, 0 } };    //    [0]
    constexpr std::array<int, 4> dir_y = { { -1,  0, 0, 1 } };    // [1][X][2]
    constexpr std::array<int, 4> dir_d = { { 90, 0, 180, 270 } }; //    [3]
    constexpr std::array<std::array<quadrant, 2>, 4> dir_quadrants = { {
            {{ quadrant::NE, quadrant::NW }},
            {{ quadrant::SW, quadrant::NW }},
            {{ quadrant::SE, quadrant::NE }},
            {{ quadrant::SE, quadrant::SW }},
        }
    };

    const float natural_light = g->natural_light_level( zlev );

    // At this point lm contains only sunlight (no artificial sources yet), so
    // any excess over the indoor baseline is sunlight that reached the tile.
    if( ddc.is_colored() ) {
        const float outside_light = g->natural_light_level( 0 );
        const float inside_light = ( zlev >= 0 && outside_light > LIGHT_SOURCE_BRIGHT )
                                   ? LIGHT_AMBIENT_DIM * 0.8f : LIGHT_AMBIENT_LOW;
        auto &lcc = map_cache.light_color_cache;
        bool wrote_any = false;
        for( int x = 0; x < MAPSIZE_X; ++x ) {
            for( int y = 0; y < MAPSIZE_Y; ++y ) {
                const float sun = lm[x][y].max() - inside_light;
                if( sun > 0.5f ) {
                    const light_color_rgb contrib = ddc * sun;
                    auto &cc = lcc[x][y];
                    cc.r = std::max( cc.r, contrib.r );
                    cc.g = std::max( cc.g, contrib.g );
                    cc.b = std::max( cc.b, contrib.b );
                    wrote_any = true;
                }
            }
        }
        if( wrote_any ) {
            map_cache.has_colored_lights = true;
        }
    }

    // each source lights only the level it stands on, when that level is built;
    // light written here into another level would land after that level's
    // overrides and outlive its publication
    Character &you = get_player_character();
    if( you.posz() == zlev ) {
        apply_character_light( you );
    }
    for( npc &guy : g->all_npcs() ) {
        if( guy.posz() == zlev ) {
            apply_character_light( guy );
        }
    }

    std::vector<std::pair<tripoint_bub_ms, float>> lm_override;
    // Traverse the submaps in order
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{ smx, smy, zlev } );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to generate lightmap at (%d,%d,%d) but the submap is not loaded", smx, smy, zlev );
                continue;
            }

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const point_bub_ms p2( sx + smx * SEEX, sy + smy * SEEY );
                    const tripoint_bub_ms p( p2, zlev );
                    // Project light into any openings into buildings.
                    if( !outside_cache[p.x()][p.y()] || ( !top_floor && prev_floor_cache[p.x()][p.y()] ) ) {
                        // Apply light sources for external/internal divide
                        for( int i = 0; i < 4; ++i ) {
                            point_bub_ms neighbour = p.xy() + point( dir_x[i], dir_y[i] );
                            if( lightmap_boundaries.contains( neighbour )
                                && outside_cache[neighbour.x()][neighbour.y()] &&
                                ( top_floor || !prev_floor_cache[neighbour.x()][neighbour.y()] )
                              ) {
                                const float source_light =
                                    std::min( natural_light, lm[neighbour.x()][neighbour.y()].max() );
                                if( light_transparency( p ) > LIGHT_TRANSPARENCY_SOLID ) {
                                    update_light_quadrants( lm[p.x()][p.y()], source_light, quadrant::default_ );
                                    apply_directional_light( p, dir_d[i], source_light, ddc );
                                } else {
                                    update_light_quadrants( lm[p.x()][p.y()], source_light, dir_quadrants[i][0] );
                                    update_light_quadrants( lm[p.x()][p.y()], source_light, dir_quadrants[i][1] );
                                    if( ddc.is_colored() ) {
                                        const light_color_rgb contrib = ddc * source_light;
                                        auto &cc = map_cache.light_color_cache[p.x()][p.y()];
                                        cc.r = std::max( cc.r, contrib.r );
                                        cc.g = std::max( cc.g, contrib.g );
                                        cc.b = std::max( cc.b, contrib.b );
                                    }
                                }
                            }
                        }
                    }

                    if( cur_submap->get_lum( { sx, sy } ) ) {
                        const map_stack items = i_at( p );
                        add_light_from_items( p, items );
                        // a lit item can burn down with no notice; the tile's
                        // light count outlives an item burnt out in place
                        if( std::any_of( items.begin(), items.end(), []( const item & it ) {
                        return it.is_emissive();
                        } ) ) {
                            map_cache.light_changes_by_turn = true;
                        }
                    }

                    const ter_id &terrain = cur_submap->get_ter( { sx, sy } );
                    if( terrain->light_emitted > 0 ) {
                        add_light_source( p, terrain->light_emitted, terrain->light_color );
                    }
                    const furn_id &furniture = cur_submap->get_furn( {sx, sy } );
                    if( furniture->light_emitted > 0 ) {
                        add_light_source( p, furniture->light_emitted, furniture->light_color );
                    }

                    for( const auto &fld : cur_submap->get_field( { sx, sy } ) ) {
                        const field_entry *cur = &fld.second;
                        const field_intensity_level &fil = cur->get_intensity_level();
                        if( fil.light_emitted > 0 ) {
                            add_light_source( p, fil.light_emitted, fil.light_color );
                        }
                        if( fil.local_light_override >= 0.0f ) {
                            lm_override.emplace_back( p, fil.local_light_override );
                        }
                    }
                }
            }
        }
    }

    for( monster &critter : g->all_monsters() ) {
        if( critter.is_hallucination() ) {
            continue;
        }
        const tripoint_bub_ms mp = critter.pos_bub( *this );
        if( inbounds( mp ) && mp.z() == zlev ) {
            if( critter.has_effect( effect_onfire ) ) {
                apply_light_source( mp, 8 );
            }
            // TODO: [lightmap] Attach natural light brightness to creatures
            // TODO: [lightmap] Allow creatures to have light attacks (i.e.: eyebot)
            // TODO: [lightmap] Allow creatures to have facing and arc lights
            const float critter_luminance = critter.luminance();
            if( critter_luminance > 0 ) {
                apply_light_source( mp, critter_luminance );
            }
        }
    }

    // Apply any vehicle light sources
    VehicleList vehs = get_vehicles();
    for( wrapped_vehicle &vv : vehs ) {
        vehicle *v = vv.v;

        auto lights = v->lights();
        const float veh_luminance = vehicle::cone_light_luminance( lights );

        for( const vehicle_part *pt : lights ) {
            const vpart_info &vp = pt->info();
            tripoint_bub_ms src = v->bub_part_pos( *this, *pt );

            if( !inbounds( src ) || src.z() != zlev ) {
                continue;
            }

            if( vp.has_flag( VPFLAG_CONE_LIGHT ) ) {
                if( veh_luminance > lit_level::LIT ) {
                    add_light_source( src, M_SQRT2, vp.light_color ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance,
                                     45_degrees, vp.light_color );
                }

            } else if( vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                if( veh_luminance > lit_level::LIT ) {
                    add_light_source( src, M_SQRT2, vp.light_color ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance,
                                     90_degrees, vp.light_color );
                }

            } else if( vp.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ) {
                if( vp.has_flag( VPFLAG_WALL_MOUNTED ) ) {
                    tileray tdir( v->face.dir() + pt->direction );
                    tdir.advance();
                    tripoint_bub_ms offset = src;
                    offset.x() = src.x() + tdir.dx();
                    offset.y() = src.y() + tdir.dy();
                    add_light_source( offset, M_SQRT2, vp.light_color ); // Add a little surrounding light
                    apply_light_arc( offset, v->face.dir() + pt->direction, vp.bonus, 180_degrees,
                                     vp.light_color );
                } else {
                    add_light_source( src, M_SQRT2, vp.light_color ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, vp.bonus, 180_degrees,
                                     vp.light_color );
                }

            } else if( vp.has_flag( VPFLAG_CIRCLE_LIGHT ) ) {
                const bool odd_turn = calendar::once_every( 2_turns );
                if( vp.has_flag( VPFLAG_ODDTURN ) || vp.has_flag( VPFLAG_EVENTURN ) ) {
                    map_cache.light_changes_by_turn = true;
                }
                if( ( odd_turn && vp.has_flag( VPFLAG_ODDTURN ) ) ||
                    ( !odd_turn && vp.has_flag( VPFLAG_EVENTURN ) ) ||
                    ( !( vp.has_flag( VPFLAG_EVENTURN ) || vp.has_flag( VPFLAG_ODDTURN ) ) ) ) {

                    add_light_source( src, vp.bonus, vp.light_color );
                }

            } else {
                add_light_source( src, vp.bonus, vp.light_color );
            }
        }

        for( const vpart_reference &vpr : v->get_any_parts( VPFLAG_CARGO ) ) {
            const tripoint_bub_ms pos = vpr.pos_bub( *this );
            if( !inbounds( pos ) || pos.z() != zlev || vpr.info().has_flag( "COVERED" ) ) {
                continue;
            }
            add_light_from_items( pos, vpr.items() );
            for( const item &it : vpr.items() ) {
                if( it.is_emissive() ) {
                    map_cache.light_changes_by_turn = true;
                    break;
                }
            }
        }
    }

    /* Now that we have position and intensity of all bulk light sources, apply_ them
      This may seem like extra work, but take a 12x12 raging inferno:
        unbuffered: (12^2)*(160*4) = apply_light_ray x 92160
        buffered:   (12*4)*(160)   = apply_light_ray x 7680
    */
    const tripoint_bub_ms cache_start( 0, 0, zlev );
    const tripoint_bub_ms cache_end( LIGHTMAP_CACHE_X, LIGHTMAP_CACHE_Y, zlev );
    for( const tripoint_bub_ms &p : points_in_rectangle( cache_start, cache_end ) ) {
        if( light_source_buffer[p.x()][p.y()].luminance > 0.0 ) {
            apply_light_source( p, light_source_buffer[p.x()][p.y()].luminance );
        }
    }

    for( const std::pair<tripoint_bub_ms, float> &elem : lm_override ) {
        lm[elem.first.x()][elem.first.y()].fill( elem.second );
    }

    // 3x3 box blur on the color cache softens residual octant boundary seams.
    // Even with per-channel max in the color write, attenuation differences
    // between adjacent octants can leave visible intensity steps.
    if( map_cache.has_colored_lights ) {
        auto &light_color_cache = map_cache.light_color_cache;
        static auto blur_buf =
            std::make_unique<cata::mdarray<light_color_rgb, point_bub_ms>>();
        blur_buf->fill( light_color_rgb{} );
        for( int x = 1; x < MAPSIZE_X - 1; ++x ) {
            for( int y = 1; y < MAPSIZE_Y - 1; ++y ) {
                if( !light_color_cache[x][y].is_colored() ) {
                    continue;
                }
                light_color_rgb sum{};
                int count = 0;
                for( int dx = -1; dx <= 1; ++dx ) {
                    for( int dy = -1; dy <= 1; ++dy ) {
                        sum += light_color_cache[x + dx][y + dy];
                        ++count;
                    }
                }
                ( *blur_buf )[x][y] = sum * ( 1.0f / count );
            }
        }
        for( int x = 1; x < MAPSIZE_X - 1; ++x ) {
            for( int y = 1; y < MAPSIZE_Y - 1; ++y ) {
                if( ( *blur_buf )[x][y].is_colored() ) {
                    light_color_cache[x][y] = ( *blur_buf )[x][y];
                }
            }
        }
    }
    // only place a level's light is complete, so the only one that publishes it
    map_cache.lightmap_generation = next_cache_generation();
}

void map::add_light_source( const tripoint_bub_ms &p, float luminance,
                            const light_color_rgb &color )
{
    auto &buf = get_cache( p.z() ).light_source_buffer[p.x()][p.y()];
    if( luminance > buf.luminance ) {
        buf.luminance = luminance;
    }
    // Color accumulates additively, weighted by luminance so brighter sources
    // dominate the hue. Luminance itself uses max() for the buffer dedup that
    // prevents redundant ray casting into neighbors (see apply_light_source).
    if( color.is_colored() ) {
        buf.color += color * luminance;
    }
}

light_color_rgb light_color_rgb::from_hsv( float h, float s, float v )
{
    const float c = v * s;
    const float x = c * ( 1.0f - std::abs( std::fmod( h / 60.0f, 2.0f ) - 1.0f ) );
    const float m = v - c;
    float r1 = 0.0f;
    float g1 = 0.0f;
    float b1 = 0.0f;
    if( h < 60.0f ) {
        r1 = c;
        g1 = x;
    } else if( h < 120.0f ) {
        r1 = x;
        g1 = c;
    } else if( h < 180.0f ) {
        g1 = c;
        b1 = x;
    } else if( h < 240.0f ) {
        g1 = x;
        b1 = c;
    } else if( h < 300.0f ) {
        r1 = x;
        b1 = c;
    } else {
        r1 = c;
        b1 = x;
    }
    return { r1 + m, g1 + m, b1 + m };
}

// Tile light/transparency: 3D

lit_level map::light_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return lit_level::DARK;    // Out of bounds
    }
    ensure_light( p.z() );

    const level_cache &map_cache = get_cache_ref( p.z() );
    const auto &lm = map_cache.lm;
    const auto &sm = map_cache.sm;
    if( sm[p.x()][p.y()] >= LIGHT_SOURCE_BRIGHT ) {
        return lit_level::BRIGHT;
    }

    const float max_light = lm[p.x()][p.y()].max();
    if( max_light >= LIGHT_AMBIENT_LIT ) {
        return lit_level::LIT;
    }

    if( max_light >= LIGHT_AMBIENT_LOW ) {
        return lit_level::LOW;
    }

    return lit_level::DARK;
}

float map::ambient_light_at( const tripoint_bub_ms &p ) const
{
    if( !this->inbounds( p ) ) {
        return 0.0f;
    }
    ensure_light( p.z() );
    return get_cache_ref( p.z() ).lm[p.x()][p.y()].max();
}

bool map::is_transparent( const tripoint_bub_ms &p ) const
{
    return light_transparency( p ) > LIGHT_TRANSPARENCY_SOLID;
}

bool map::is_transparent_wo_fields( const tripoint_bub_ms &p ) const
{
    return get_cache_ref( p.z() ).transparent_cache_wo_fields[p.x()][p.y()];
}

bool map::is_sight_clear( const tripoint_bub_ms &p ) const
{
    return get_cache_ref( p.z() ).sight_cache[p.x()][p.y()] > LIGHT_TRANSPARENCY_SOLID;
}

bool map::is_sight_clear_wo_fields( const tripoint_bub_ms &p ) const
{
    return get_cache_ref( p.z() ).sight_cache_wo_fields[p.x()][p.y()];
}

float map::light_transparency( const tripoint_bub_ms &p ) const
{
    return get_cache_ref( p.z() ).transparency_cache[p.x()][p.y()];
}

// End of tile light/transparency

map::apparent_light_info map::apparent_light_helper( const level_cache &map_cache,
        const tripoint_bub_ms &p )
{
    avatar const &u = get_avatar();
    // static, so its callers' level caches are the reality bubble's
    const tripoint_bub_ms u_pos = u.pos_bub();
    const int dist = rl_dist( u_pos, p );
    const float abs_vis =
        std::max( map_cache.seen_cache[p.x()][p.y()], map_cache.camera_cache[p.x()][p.y()] );
    const float vis = dist > u.unimpaired_range() ? map_cache.camera_cache[p.x()][p.y()] : abs_vis;
    const bool obstructed = vis <= LIGHT_TRANSPARENCY_SOLID + 0.1;
    const bool abs_obstructed = abs_vis <= LIGHT_TRANSPARENCY_SOLID + 0.1;

    // avatar always sees the tile it's standing on, whatever fills it
    const bool on_avatar_level = p.z() == u_pos.z();
    auto is_opaque = [&map_cache, &u_pos, on_avatar_level]( const point_bub_ms & p ) {
        if( on_avatar_level && p == u_pos.xy() ) {
            return false;
        }
        return map_cache.transparency_cache[p.x()][p.y()] <= LIGHT_TRANSPARENCY_SOLID &&
               map_cache.vision_transparency_cache[p.x()][p.y()] <= LIGHT_TRANSPARENCY_SOLID;
    };

    // possibly reduce view if aiming (also blocks light)
    if( get_avatar().recoil < MAX_RECOIL ) {
        if( get_avatar().cant_see( p ) ) {
            return { true, true, 0.0 };
        }
    }

    const bool p_opaque = is_opaque( p.xy() );
    float apparent_light;

    if( p_opaque && vis > 0 ) {
        // This is the complicated case.  We want to check which quadrants the
        // player can see the tile from, and only count light values from those
        // quadrants.
        struct offset_and_quadrants {
            point offset;
            std::array<quadrant, 2> quadrants;
        };
        static constexpr std::array<offset_and_quadrants, 8> adjacent_offsets = {{
                { point::south,      {{ quadrant::SE, quadrant::SW }} },
                { point::north,      {{ quadrant::NE, quadrant::NW }} },
                { point::east,       {{ quadrant::SE, quadrant::NE }} },
                { point::south_east, {{ quadrant::SE, quadrant::SE }} },
                { point::north_east, {{ quadrant::NE, quadrant::NE }} },
                { point::west,       {{ quadrant::SW, quadrant::NW }} },
                { point::south_west, {{ quadrant::SW, quadrant::SW }} },
                { point::north_west, {{ quadrant::NW, quadrant::NW }} },
            }
        };

        four_quadrants seen_from( 0 );
        for( const offset_and_quadrants &oq : adjacent_offsets ) {
            const point_bub_ms neighbour = p.xy() + oq.offset;

            if( !lightmap_boundaries.contains( neighbour ) ) {
                continue;
            }
            if( is_opaque( neighbour ) ) {
                continue;
            }
            if( ( rl_dist( u_pos.xy(), neighbour ) > u.unimpaired_range() &&
                  map_cache.camera_cache[neighbour.x()][neighbour.y()] == 0 ) ||
                ( map_cache.seen_cache[neighbour.x()][neighbour.y()] == 0 &&
                  map_cache.camera_cache[neighbour.x()][neighbour.y()] == 0 ) ) {
                continue;
            }
            // This is a non-opaque visible neighbour, so count visibility from the relevant
            // quadrants
            seen_from[oq.quadrants[0]] = vis;
            seen_from[oq.quadrants[1]] = vis;
        }
        apparent_light = ( seen_from * map_cache.lm[p.x()][p.y()] ).max();
    } else {
        // This is the simple case, for a non-opaque tile light from all
        // directions is equivalent
        apparent_light = vis * map_cache.lm[p.x()][p.y()].max();
    }
    return { obstructed, abs_obstructed, apparent_light };
}

lit_level map::apparent_light_at( const tripoint_bub_ms &p,
                                  const visibility_variables &cache ) const
{
    Character &player_character = get_player_character();
    const int dist = rl_dist( player_character.pos_bub( *this ), p );

    // Clairvoyance overrides everything.
    if( cache.u_clairvoyance > 0 && dist <= cache.u_clairvoyance ) {
        return lit_level::BRIGHT;
    }
    if( cache.clairvoyance_field && field_at( p ).find_field( *cache.clairvoyance_field ) ) {
        return lit_level::BRIGHT;
    }
    const level_cache &map_cache = get_cache_ref( p.z() );
    const apparent_light_info a = apparent_light_helper( map_cache, p );

    // Unimpaired range is an override to strictly limit vision range based on various conditions,
    // but the player can still see light sources
    if( dist > player_character.unimpaired_range() && map_cache.camera_cache[p.x()][p.y()] == 0.0 ) {
        if( !a.abs_obstructed && map_cache.sm[p.x()][p.y()] > 0.0 ) {
            return lit_level::BRIGHT_ONLY;
        }
        return lit_level::BLANK;
    }

    if( a.obstructed ) {
        if( a.apparent_light > LIGHT_AMBIENT_LIT ) {
            if( a.apparent_light > cache.g_light_level ) {
                // This represents too hazy to see detail,
                // but enough light getting through to illuminate.
                return lit_level::BRIGHT_ONLY;
            }
        }
        return lit_level::BLANK;
    }
    // Then we just search for the light level in descending order.
    if( a.apparent_light > LIGHT_SOURCE_BRIGHT || map_cache.sm[p.x()][p.y()] > 0.0 ) {
        return lit_level::BRIGHT;
    }
    if( a.apparent_light > LIGHT_AMBIENT_LIT ) {
        return lit_level::LIT;
    }
    if( a.apparent_light >= cache.vision_threshold ) {
        return lit_level::LOW;
    } else {
        return lit_level::BLANK;
    }
}

bool map::pl_sees( const tripoint_bub_ms &t, const int max_range ) const
{
    if( !inbounds( t ) ) {
        return false;
    }
    ensure_light( t.z() );

    const level_cache &map_cache = get_cache_ref( t.z() );
    Character &player_character = get_player_character();
    if( max_range >= 0 && square_dist( get_abs( t ), player_character.pos_abs() ) > max_range &&
        map_cache.camera_cache[t.x()][t.y()] == 0 ) {
        return false;    // Out of range!
    }

    const apparent_light_info a = apparent_light_helper( map_cache, t );
    // avatar might not be on *this* map
    const float light_at_player = get_map().ambient_light_at( player_character.pos_bub() );
    return !a.obstructed &&
           ( a.apparent_light >= player_character.get_vision_threshold( light_at_player ) ||
             map_cache.sm[t.x()][t.y()] > 0.0 );
}

// For a direction vector defined by x, y, return the quadrant that's the
// source of that direction.  Assumes x != 0 && y != 0
// NOLINTNEXTLINE(cata-xy)
static constexpr quadrant quadrant_from_x_y( int x, int y )
{
    return ( x > 0 ) ?
           ( ( y > 0 ) ? quadrant::NW : quadrant::SW ) :
           ( ( y > 0 ) ? quadrant::NE : quadrant::SE );
}

// Precomputed 2D Euclidean distances for castLight inner loop.
// Eliminates sqrt calls from the hottest code path.
static const auto &trig_dist_2d_lut()
{
    static const auto lut = []() {
        constexpr int N = MAX_VIEW_DISTANCE + 1;
        std::array<std::array<int, N>, N> t{};
        for( int x = 0; x < N; ++x ) {
            for( int y = 0; y < N; ++y ) {
                t[x][y] = static_cast<int>(
                              std::sqrt( static_cast<double>( x * x + y * y ) ) );
            }
        }
        return t;
    }
    ();
    return lut;
}

int trig_dist_2d( point delta )
{
    return trig_dist_2d_lut()[std::abs( delta.x )][std::abs( delta.y )];
}

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & ),
         bool with_color = false>
static void castLight( cata::mdarray<Out, point_bub_ms> &output_cache,
                       const cata::mdarray<T, point_bub_ms> &input_array,
                       const point_bub_ms &offset, int offsetDistance,
                       T numerator = VISIBILITY_FULL,
                       int row = 1, float start = 1.0f, float end = 0.0f,
                       T cumulative_transparency = T( LIGHT_TRANSPARENCY_OPEN_AIR ),
                       light_color_rgb source_color = {},
                       cata::mdarray<light_color_rgb, point_bub_ms> *color_cache = nullptr );

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & ),
         bool with_color>
void castLight( cata::mdarray<Out, point_bub_ms> &output_cache,
                const cata::mdarray<T, point_bub_ms> &input_array,
                const point_bub_ms &offset, const int offsetDistance, const T numerator,
                const int row, float start, const float end, T cumulative_transparency,
                const light_color_rgb source_color,
                cata::mdarray<light_color_rgb, point_bub_ms> *color_cache )
{
    constexpr quadrant quad = quadrant_from_x_y( -xx - xy, -yx - yy );
    float newStart = 0.0f;
    float radius = static_cast<float>( MAX_VIEW_DISTANCE ) - offsetDistance;
    if( start < end ) {
        return;
    }
    T last_intensity( 0.0 );
    tripoint delta;
    for( int distance = row; distance <= radius; distance++ ) {
        delta.y = -distance;
        bool started_row = false;
        T current_transparency( 0.0 );
        float away = start - ( -distance + 0.5f ) / ( -distance -
                     0.5f ); //The distance between our first leadingEdge and start

        //We initialize delta.x to -distance adjusted so that the commented start < leadingEdge condition below is never false
        delta.x = -distance + std::max( static_cast<int>( std::ceil( away * ( -distance - 0.5f ) ) ), 0 );

        for( ; delta.x <= 0; delta.x++ ) {
            point current( offset.x() + delta.x * xx + delta.y * xy, offset.y() + delta.x * yx + delta.y * yy );
            float trailingEdge = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
            float leadingEdge = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

            if( !( current.x >= 0 && current.y >= 0 && current.x < MAPSIZE_X &&
                   current.y < MAPSIZE_Y ) /* || start < leadingEdge */ ) {
                continue;
            } else if( end > trailingEdge ) {
                break;
            }
            if( !started_row ) {
                started_row = true;
                current_transparency = input_array[ current.x ][ current.y ];
            }

            const int dist = ( trigdist
                               ? trig_dist_2d_lut()[std::abs( delta.x )][std::abs( delta.y )]
                               : std::max( std::abs( delta.x ), std::abs( delta.y ) ) ) + offsetDistance;
            last_intensity = calc( numerator, cumulative_transparency, dist );

            T new_transparency = input_array[ current.x ][ current.y ];

            if( check( new_transparency, last_intensity ) ) {
                update_output( output_cache[current.x][current.y], last_intensity,
                               quadrant::default_ );
            } else {
                update_output( output_cache[current.x][current.y], last_intensity, quad );
            }

            if constexpr( with_color ) {
                const light_color_rgb contrib = source_color * last_intensity;
                auto &cc = ( *color_cache )[current.x][current.y];
                cc.r = std::max( cc.r, contrib.r );
                cc.g = std::max( cc.g, contrib.g );
                cc.b = std::max( cc.b, contrib.b );
            }

            if( new_transparency == current_transparency ) {
                newStart = leadingEdge;
                continue;
            }
            // Only cast recursively if previous span was not opaque.
            if( check( current_transparency, last_intensity ) ) {
                castLight<xx, xy, yx, yy, T, Out, calc, check, update_output, accumulate, with_color>(
                    output_cache, input_array, offset, offsetDistance,
                    numerator, distance + 1, start, trailingEdge,
                    accumulate( cumulative_transparency, current_transparency, distance ),
                    source_color, color_cache );
            }
            // The new span starts at the leading edge of the previous square if it is opaque,
            // and at the trailing edge of the current square if it is transparent.
            if( !check( current_transparency, last_intensity ) ) {
                start = newStart;
            } else {
                // Note this is the same slope as the recursive call we just made.
                start = trailingEdge;
            }
            // Trailing edge ahead of leading edge means this span is fully processed.
            if( start < end ) {
                return;
            }
            current_transparency = new_transparency;
            newStart = leadingEdge;
        }
        if( !check( current_transparency, last_intensity ) ) {
            // If we reach the end of the span with terrain being opaque, we don't iterate further.
            break;
        }
        // Cumulative average of the transparency values encountered.
        cumulative_transparency = accumulate( cumulative_transparency, current_transparency, distance );
    }
}

template<typename T, typename Out, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLightAll( cata::mdarray<Out, point_bub_ms> &output_cache,
                   const cata::mdarray<T, point_bub_ms> &input_array,
                   const point_bub_ms &offset, int offsetDistance, T numerator )
{
    castLight<0, 1, 1, 0, T, Out, calc, check, update_output, accumulate>(
        output_cache, input_array, offset, offsetDistance, numerator );
    castLight<1, 0, 0, 1, T, Out, calc, check, update_output, accumulate>(
        output_cache, input_array, offset, offsetDistance, numerator );

    castLight < 0, -1, 1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );
    castLight < -1, 0, 0, 1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );

    castLight < 0, 1, -1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );
    castLight < 1, 0, 0, -1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );

    castLight < 0, -1, -1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );
    castLight < -1, 0, 0, -1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offset, offsetDistance, numerator );
}

template void castLightAll<float, four_quadrants, sight_calc, sight_check,
                           update_light_quadrants, accumulate_transparency>(
                               cata::mdarray<four_quadrants, point_bub_ms> &output_cache,
                               const cata::mdarray<float, point_bub_ms> &input_array,
                               const point_bub_ms &offset, int offsetDistance, float numerator );

template void
castLightAll<fragment_cloud, fragment_cloud, shrapnel_calc, shrapnel_check,
             update_fragment_cloud, accumulate_fragment_cloud>
(
    cata::mdarray<fragment_cloud, point_bub_ms> &output_cache,
    const cata::mdarray<fragment_cloud, point_bub_ms> &input_array,
    const point_bub_ms &offset, int offsetDistance, fragment_cloud numerator );

/**
 * Calculates the Field Of View for the provided map from the given x, y
 * coordinates into seen_cache, or for a camera into camera_cache. Values
 * represent a percentage of fully lit.
 *
 * A value equal to or below 0 means that cell is not in the
 * field of view, whereas a value equal to or above 1 means that cell is
 * in the field of view.
 *
 * @param origin the starting location
 * @param target_z Z-level the origin is seeded on
 * @param extension_range range of vehicle mirrors and cameras around origin
 * @param camera true to cast for a camera, merged into camera_cache
 * @param penalty distance added to the cast, shortening a camera's range
 * @param eye_level height of the observer's eyes, for ledges
 */
void map::build_seen_cache( const tripoint_bub_ms &origin, const int target_z, int extension_range,
                            bool camera, int penalty, const float eye_level )
{
    level_cache &map_cache = get_cache( target_z );
    using mdarray = cata::mdarray<float, point_bub_ms>;
    mdarray &transparency_cache = map_cache.vision_transparency_cache;
    mdarray &seen_cache = map_cache.seen_cache;
    mdarray &camera_cache = map_cache.camera_cache;

    constexpr float light_transparency_solid = LIGHT_TRANSPARENCY_SOLID;
    constexpr int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    // a camera casts into its own grids and merges after its own ledge pass, so
    // one camera's ledges never hide what another sees
    static const std::unique_ptr<std::array<mdarray, OVERMAP_LAYERS>> camera_scratch_storage =
                std::make_unique<std::array<mdarray, OVERMAP_LAYERS>>();
    std::array<mdarray, OVERMAP_LAYERS> &camera_scratch = *camera_scratch_storage;

    // Cache the caches (pointers to them)
    array_of_grids_of<const float> transparency_caches;
    array_of_grids_of<float> seen_caches;
    array_of_grids_of<const bool> floor_caches;
    vertical_direction directions_to_cast = vertical_direction::BOTH;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        level_cache &cur_cache = get_cache( z );
        // a camera sees past the avatar's own cover
        transparency_caches[z + OVERMAP_DEPTH] = camera ? &cur_cache.sight_cache :
                &cur_cache.vision_transparency_cache;
        seen_caches[z + OVERMAP_DEPTH] = camera ? &camera_scratch[z + OVERMAP_DEPTH] :
                                         &cur_cache.seen_cache;
        floor_caches[z + OVERMAP_DEPTH] = &cur_cache.floor_cache;
        std::uninitialized_fill_n(
            &( *seen_caches[z + OVERMAP_DEPTH] )[0][0], map_dimensions, light_transparency_solid );
        if( !camera ) {
            cur_cache.seen_cache_dirty = false;
        }
        if( origin.z() == z && cur_cache.no_floor_gaps ) {
            directions_to_cast = vertical_direction::UP;
        }
    }
    if( origin.z() == target_z ) {
        ( *seen_caches[ target_z + OVERMAP_DEPTH ] )[origin.x()][origin.y()] = VISIBILITY_FULL;
    }

    cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
        seen_caches, transparency_caches, floor_caches, origin, penalty, 1.0,
        directions_to_cast );
    seen_cache_process_ledges( seen_caches, floor_caches, origin, eye_level );
    // set here too: the early returns below skip the final set after the
    // mirror pass
    seen_cache_generation = next_cache_generation();

    // mirrors too
    mdarray &out_cache = camera ? camera_scratch[target_z + OVERMAP_DEPTH] : seen_cache;
    const mdarray &mirror_transparency = camera ? map_cache.sight_cache : transparency_cache;
    const auto merge_camera = [&]() {
        if( !camera ) {
            return;
        }
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            mdarray &merged = get_cache( z ).camera_cache;
            const mdarray &cast = camera_scratch[z + OVERMAP_DEPTH];
            bool wrote = false;
            for( int x = 0; x < MAPSIZE_X; ++x ) {
                for( int y = 0; y < MAPSIZE_Y; ++y ) {
                    wrote |= cast[x][y] > LIGHT_TRANSPARENCY_SOLID;
                    merged[x][y] = std::max( merged[x][y], cast[x][y] );
                }
            }
            if( wrote ) {
                camera_levels_written.set( z + OVERMAP_DEPTH );
            }
        }
    };

    const optional_vpart_position vp = veh_at( origin );
    if( !vp ) {
        merge_camera();
        return;
    }
    vehicle *const veh = &vp->vehicle();

    // We're inside a vehicle. Do mirror calculations.
    std::vector<int> mirrors;
    // Do all the sight checks first to prevent fake multiple reflection
    // from happening due to mirrors becoming visible due to processing order.
    // Cameras are also handled here, so that we only need to get through all vehicle parts once
    int cam_control = -1;
    for( const vpart_reference &vp : veh->get_all_parts_with_fakes() ) {
        if( vp.part().removed || vp.part().is_broken() || !vp.info().has_flag( VPFLAG_EXTENDS_VISION ) ) {
            continue;
        }
        const tripoint_bub_ms mirror_pos = vp.pos_bub( *this );
        if( rl_dist( origin, vp.pos_bub( *this ) ) > extension_range ) {
            continue;
        }
        // We can utilize the current state of the seen cache to determine
        // if the player can see the mirror from their position.
        if( !vp.info().has_flag( "CAMERA" ) &&
            out_cache[mirror_pos.x()][mirror_pos.y()] < LIGHT_TRANSPARENCY_SOLID + 0.1 ) {
            continue;
        } else if( !vp.info().has_flag( "CAMERA_CONTROL" ) ) {
            mirrors.emplace_back( static_cast<int>( vp.part_index() ) );
        } else {
            if( square_dist( origin, mirror_pos ) <= 1 && veh->camera_on ) {
                cam_control = static_cast<int>( vp.part_index() );
            }
        }
    }

    for( const int mirror : mirrors ) {
        const vehicle_part &vp_mirror = veh->part( mirror );
        const vpart_info &vpi_mirror = vp_mirror.info();
        const bool is_camera = vpi_mirror.has_flag( "CAMERA" );
        if( is_camera && cam_control < 0 ) {
            continue; // Player not at camera control, so cameras don't work
        }

        const tripoint_bub_ms mirror_pos = veh->bub_part_pos( *this, vp_mirror );

        // Determine how far the light has already traveled so mirrors
        // don't cheat the light distance falloff.
        int offsetDistance;
        mdarray *mocache = &out_cache;
        if( !is_camera ) {
            offsetDistance = penalty + rl_dist( origin, mirror_pos );
        } else {
            offsetDistance = MAX_VIEW_DISTANCE - vpi_mirror.bonus * vp_mirror.hp() / vpi_mirror.durability;
            mocache = &camera_cache;
            camera_levels_written.set( target_z + OVERMAP_DEPTH );
            ( *mocache )[mirror_pos.x()][mirror_pos.y()] = LIGHT_TRANSPARENCY_OPEN_AIR;
            castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
                *mocache, map_cache.sight_cache, mirror_pos.xy(), offsetDistance );
            continue;
        }

        // TODO: Factor in the mirror facing and only cast in the
        // directions the player's line of sight reflects to.
        //
        // The naive solution of making the mirrors act like a second player
        // at an offset appears to give reasonable results though.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            *mocache, mirror_transparency, mirror_pos.xy(), offsetDistance );
    }
    merge_camera();
    seen_cache_generation = next_cache_generation();
}

void map::seen_cache_process_ledges( array_of_grids_of<float> &seen_caches,
                                     const array_of_grids_of<const bool> &floor_caches,
                                     const tripoint_bub_ms &origin, const float eye_level ) const
{
    const int min_z = std::max( origin.z() - fov_3d_z_range, -OVERMAP_DEPTH );
    // For each tile
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    // Iterate down z-levels starting from 1 level below origin
                    for( int sz = origin.z() - 1; sz >= min_z; --sz ) {
                        const tripoint_bub_ms p( sx + smx * SEEX, sy + smy * SEEY, sz );
                        const int cache_z = sz + OVERMAP_DEPTH;
                        // Until invisible tile reached
                        if( ( *seen_caches[cache_z] )[p.x()][p.y()] == 0.0f ) {
                            break;
                        }
                        // Or floor reached
                        if( ( *floor_caches[cache_z] ) [p.x()][p.y()] ) {
                            // In which case check if it should be obscured by a ledge
                            if( ledge_coverage( origin, p, eye_level ) > 100 ) {
                                ( *seen_caches[cache_z] )[p.x()][p.y()] = 0.0f;
                                get_cache( sz ).ledge_hidden[p.x()][p.y()] = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

//Schraudolph's algorithm with John's constants
static float fastexp( float x )
{
    union {
        float f;
        int i;
    } u, v;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
    u.i = static_cast<long long>( 6051102 * x + 1056478197 );
    v.i = static_cast<long long>( 1056478197 - 6051102 * x );
#pragma GCC diagnostic pop
    return u.f / v.f;
}

static float light_calc( const float &numerator, const float &transparency,
                         const int &distance )
{
    // Light needs inverse square falloff in addition to attenuation.
    return numerator  / ( fastexp( transparency * distance ) * distance );
}

static bool light_check( const float &transparency, const float &intensity )
{
    return transparency > LIGHT_TRANSPARENCY_SOLID && intensity > LIGHT_AMBIENT_LOW;
}

void map::apply_light_source( const tripoint_bub_ms &p, float luminance )
{
    level_cache &cache = get_cache( p.z() );
    auto &lm = cache.lm;
    auto &sm = cache.sm;
    auto &transparency_cache = cache.transparency_cache;
    auto &light_source_buffer = cache.light_source_buffer;
    auto &light_color_cache = cache.light_color_cache;

    const point_bub_ms p2( p.xy() );

    if( inbounds( p ) ) {
        const float min_light = std::max( static_cast<float>( lit_level::LOW ), luminance );
        lm[p2.x()][p2.y()] = elementwise_max( lm[p2.x()][p2.y()], min_light );
        sm[p2.x()][p2.y()] = std::max( sm[p2.x()][p2.y()], luminance );
    }
    if( luminance <= lit_level::LOW ) {
        return;
    } else if( luminance <= lit_level::BRIGHT_ONLY ) {
        luminance = 1.49f;
    }

    // Color propagation: the buffer stores accumulated (color * luminance).
    // Dividing by luminance recovers the average color, which castLight then
    // re-scales by the per-tile attenuated intensity -- same falloff as scalar.
    const auto &buf = light_source_buffer[p2.x()][p2.y()];
    const bool has_color = buf.color.is_colored();
    light_color_rgb source_color;
    if( has_color ) {
        source_color = buf.color * ( 1.0f / buf.luminance );
        // Set source tile color directly
        light_color_cache[p2.x()][p2.y()] += source_color * luminance;
        cache.has_colored_lights = true;
    }

    /* If we're a 5 luminance fire , we skip casting rays into ey && sx if we have
         neighboring fires to the north and west that were applied via light_source_buffer
       If there's a 1 luminance candle east in buffer, we still cast rays into ex since it's smaller
       If there's a 100 luminance magnesium flare south added via apply_light_source instead of
         add_light_source, it's unbuffered so we'll still cast rays into sy.

          ey
        nnnNnnn
        w     e
        w  5 +e
     sx W 5*1+E ex
        w ++++e
        w+++++e
        sssSsss
           sy
    */
    const int peer_inbounds = LIGHTMAP_CACHE_X - 1;
    bool north = p2.y() != 0 && light_source_buffer[p2.x()][p2.y() - 1].luminance < luminance;
    bool south = p2.y() != peer_inbounds &&
                 light_source_buffer[p2.x()][p2.y() + 1].luminance < luminance;
    bool east = p2.x() != peer_inbounds &&
                light_source_buffer[p2.x() + 1][p2.y()].luminance < luminance;
    bool west = p2.x() != 0 && light_source_buffer[p2.x() - 1][p2.y()].luminance < luminance;

    // Helper macro: cast scalar light through one octant, fusing the color
    // write into the same traversal when the source has color.
#define CAST_LIGHT_OCTANT( _xx, _xy, _yx, _yy ) \
    if( has_color ) { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency, true>( \
                lm, transparency_cache, p2, 0, luminance, 1, 1.0f, 0.0f, \
                LIGHT_TRANSPARENCY_OPEN_AIR, source_color, &light_color_cache ); \
    } else { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency>( \
                lm, transparency_cache, p2, 0, luminance ); \
    }

    if( north ) {
        CAST_LIGHT_OCTANT( 1, 0, 0, -1 )
        CAST_LIGHT_OCTANT( -1, 0, 0, -1 )
    }

    if( east ) {
        CAST_LIGHT_OCTANT( 0, -1, 1, 0 )
        CAST_LIGHT_OCTANT( 0, -1, -1, 0 )
    }

    if( south ) {
        CAST_LIGHT_OCTANT( 1, 0, 0, 1 )
        CAST_LIGHT_OCTANT( -1, 0, 0, 1 )
    }

    if( west ) {
        CAST_LIGHT_OCTANT( 0, 1, 1, 0 )
        CAST_LIGHT_OCTANT( 0, 1, -1, 0 )
    }
#undef CAST_LIGHT_OCTANT
}

void map::apply_directional_light( const tripoint_bub_ms &p, int direction,
                                   float luminance, const light_color_rgb &color )
{
    const point_bub_ms p2( p.xy() );

    level_cache &cache = get_cache( p.z() );
    cata::mdarray<four_quadrants, point_bub_ms> &lm = cache.lm;
    cata::mdarray<float, point_bub_ms> &transparency_cache =
        cache.transparency_cache;
    cata::mdarray<light_color_rgb, point_bub_ms> &light_color_cache =
        cache.light_color_cache;

    const bool has_color = color.is_colored();
    if( has_color ) {
        const light_color_rgb contrib = color * luminance;
        auto &cc = light_color_cache[p2.x()][p2.y()];
        cc.r = std::max( cc.r, contrib.r );
        cc.g = std::max( cc.g, contrib.g );
        cc.b = std::max( cc.b, contrib.b );
        cache.has_colored_lights = true;
    }

#define CAST_DIR_OCTANT( _xx, _xy, _yx, _yy ) \
    if( has_color ) { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency, true>( \
                lm, transparency_cache, p2, 0, luminance, 1, 1.0f, 0.0f, \
                LIGHT_TRANSPARENCY_OPEN_AIR, color, &light_color_cache ); \
    } else { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency>( \
                lm, transparency_cache, p2, 0, luminance ); \
    }

    if( direction == 90 ) {
        CAST_DIR_OCTANT( 1, 0, 0, -1 )
        CAST_DIR_OCTANT( -1, 0, 0, -1 )
    } else if( direction == 0 ) {
        CAST_DIR_OCTANT( 0, -1, 1, 0 )
        CAST_DIR_OCTANT( 0, -1, -1, 0 )
    } else if( direction == 270 ) {
        CAST_DIR_OCTANT( 1, 0, 0, 1 )
        CAST_DIR_OCTANT( -1, 0, 0, 1 )
    } else if( direction == 180 ) {
        CAST_DIR_OCTANT( 0, 1, 1, 0 )
        CAST_DIR_OCTANT( 0, 1, -1, 0 )
    }
#undef CAST_DIR_OCTANT
}

void map::apply_light_arc( const tripoint_bub_ms &p, const units::angle &angle, float luminance,
                           const units::angle &wideangle, const light_color_rgb &color )
{
    if( luminance <= LIGHT_SOURCE_LOCAL ) {
        return;
    }

    apply_light_source( p, LIGHT_SOURCE_LOCAL );

    const point_bub_ms p2( p.xy() );

    level_cache &cache = get_cache( p.z() );
    cata::mdarray<four_quadrants, point_bub_ms> &lm = cache.lm;
    cata::mdarray<float, point_bub_ms> &transparency_cache =
        cache.transparency_cache;
    cata::mdarray<light_color_rgb, point_bub_ms> &light_color_cache =
        cache.light_color_cache;

    const bool has_color = color.is_colored();
    if( has_color ) {
        // Source tile gets only the local halo intensity, matching scalar path
        light_color_cache[p2.x()][p2.y()] += color * LIGHT_SOURCE_LOCAL;
        cache.has_colored_lights = true;
    }

    const units::angle wangle = wideangle / 2.0;
    // Normalize so oangle is between 0 and 360 degrees
    const units::angle oangle = fmod( fmod( angle - wangle, 360_degrees ) + 360_degrees, 360_degrees );
    const units::angle cangle = oangle + wideangle;

    // Sweep over every octant
    int i = 0;
    while( true ) {
        int start = i;
        int end = i + 1;
        units::angle start_angle;
        units::angle end_angle;
        // This octant doesn't overlap with illuminated area
        if( 45_degrees * end < oangle ) {
            ++i;
            continue;
        }
        // Finish processing
        if( 45_degrees * start > cangle ) {
            break;
        }
        // Unified way to cast light in one octant
        start_angle = std::max( 45_degrees * start, oangle );
        end_angle = std::min( 45_degrees * end, cangle );

        // Helper macro: cast scalar light through one octant, fusing the
        // color write into the same traversal when the source has color.
#define CAST_ARC_OCTANT( _xx, _xy, _yx, _yy, s1, s2 ) \
    if( has_color ) { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency, true>( \
                lm, transparency_cache, p2, 0, luminance, 1, s1, s2, \
                LIGHT_TRANSPARENCY_OPEN_AIR, color, &light_color_cache ); \
    } else { \
        castLight<_xx, _xy, _yx, _yy, float, four_quadrants, light_calc, light_check, \
        update_light_quadrants, accumulate_transparency>( \
                lm, transparency_cache, p2, 0, luminance, 1, s1, s2 ); \
    }

        // i is positive
        switch( i % 8 ) {
            case 0:
                CAST_ARC_OCTANT( 0, -1, -1, 0, tan( end_angle ), tan( start_angle ) );
                break;
            case 1:
                CAST_ARC_OCTANT( -1, 0, 0, -1, cot( start_angle ), cot( end_angle ) );
                break;
            case 2:
                CAST_ARC_OCTANT( 1, 0, 0, -1, -cot( end_angle ), -cot( start_angle ) );
                break;
            case 3:
                CAST_ARC_OCTANT( 0, 1, -1, 0, -tan( start_angle ), -tan( end_angle ) );
                break;
            case 4:
                CAST_ARC_OCTANT( 0, 1, 1, 0, tan( end_angle ), tan( start_angle ) );
                break;
            case 5:
                CAST_ARC_OCTANT( 1, 0, 0, 1, cot( start_angle ), cot( end_angle ) );
                break;
            case 6:
                CAST_ARC_OCTANT( -1, 0, 0, 1, -cot( end_angle ), -cot( start_angle ) );
                break;
            case 7:
                CAST_ARC_OCTANT( 0, -1, 1, 0, -tan( start_angle ), -tan( end_angle ) );
                break;
        }
#undef CAST_ARC_OCTANT
        i++;
    }
}

void map::apply_light_ray(
    cata::mdarray<bool, point_bub_ms, LIGHTMAP_CACHE_X, LIGHTMAP_CACHE_Y> &lit,
    const tripoint_bub_ms &s, const tripoint_bub_ms &e, float luminance )
{
    point a( std::abs( e.x() - s.x() ) * 2, std::abs( e.y() - s.y() ) * 2 );
    point d( ( s.x() < e.x() ) ? 1 : -1, ( s.y() < e.y() ) ? 1 : -1 );
    point_bub_ms p( s.xy() );

    quadrant quad = quadrant_from_x_y( d.x, d.y );

    // TODO: Invert that z comparison when it's sane
    if( s.z() != e.z() || ( s.x() == e.x() && s.y() == e.y() ) ) {
        return;
    }

    auto &lm = get_cache( s.z() ).lm;
    auto &transparency_cache = get_cache( s.z() ).transparency_cache;

    float distance = 1.0f;
    float transparency = LIGHT_TRANSPARENCY_OPEN_AIR;
    const float scaling_factor = static_cast<float>( rl_dist( s, e ) ) /
                                 static_cast<float>( square_dist( s, e ) );
    // TODO: [lightmap] Pull out the common code here rather than duplication
    if( a.x > a.y ) {
        int t = a.y - ( a.x / 2 );
        do {
            if( t >= 0 ) {
                p.y() += d.y;
                t -= a.x;
            }

            p.x() += d.x;
            t += a.y;

            // TODO: clamp coordinates to map bounds before this method is called.
            if( lightmap_boundaries.contains( p ) ) {
                float current_transparency = transparency_cache[p.x()][p.y()];
                bool is_opaque = current_transparency == LIGHT_TRANSPARENCY_SOLID;
                if( !lit[p.x()][p.y()] ) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[p.x()][p.y()] = true;
                    float lm_val = luminance / ( fastexp( transparency * distance ) * distance );
                    quadrant q = is_opaque ? quad : quadrant::default_;
                    lm[p.x()][p.y()][q] = std::max( lm[p.x()][p.y()][q], lm_val );
                }
                if( is_opaque ) {
                    break;
                }
                // Cumulative average of the transparency values encountered.
                transparency = ( ( distance - 1.0 ) * transparency + current_transparency ) / distance;
            } else {
                break;
            }

            distance += scaling_factor;
        } while( !( p.x() == e.x() && p.y() == e.y() ) );
    } else {
        int t = a.x - ( a.y / 2 );
        do {
            if( t >= 0 ) {
                p.x() += d.x;
                t -= a.y;
            }

            p.y() += d.y;
            t += a.x;

            if( lightmap_boundaries.contains( p ) ) {
                float current_transparency = transparency_cache[p.x()][p.y()];
                bool is_opaque = current_transparency == LIGHT_TRANSPARENCY_SOLID;
                if( !lit[p.x()][p.y()] ) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[p.x()][p.y()] = true;
                    float lm_val = luminance / ( fastexp( transparency * distance ) * distance );
                    quadrant q = is_opaque ? quad : quadrant::default_;
                    lm[p.x()][p.y()][q] = std::max( lm[p.x()][p.y()][q], lm_val );
                }
                if( is_opaque ) {
                    break;
                }
                // Cumulative average of the transparency values encountered.
                transparency = ( ( distance - 1.0 ) * transparency + current_transparency ) / distance;
            } else {
                break;
            }

            distance += scaling_factor;
        } while( !( p.x() == e.x() && p.y() == e.y() ) );
    }
}
