#include "lightmap.h" // IWYU pragma: associated
#include "shadowcasting.h" // IWYU pragma: associated

#include <bitset>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
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

    for( const item_pocket *pkt : it.get_all_contained_pockets() ) {
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

// TODO: Consider making this just clear the cache and dynamically fill it in as is_transparent() is called
bool map::build_transparency_cache( const int zlev )
{
    level_cache &map_cache = get_cache( zlev );
    auto &transparent_cache_wo_fields = map_cache.transparent_cache_wo_fields;
    auto &transparency_cache = map_cache.transparency_cache;
    auto &outside_cache = map_cache.outside_cache;

    if( map_cache.transparency_cache_dirty.none() ) {
        return false;
    }

    // if true, all submaps are invalid (can use batch init)
    bool rebuild_all = map_cache.transparency_cache_dirty.all();

    if( rebuild_all ) {
        // Default to just barely not transparent.
        std::uninitialized_fill_n( &transparency_cache[0][0], MAPSIZE_X * MAPSIZE_Y,
                                   static_cast<float>( LIGHT_TRANSPARENCY_OPEN_AIR ) );
        for( auto &row : transparent_cache_wo_fields ) {
            row.set(); // true means transparent
        }
    }

    const float sight_penalty = get_weather().weather_id->sight_penalty;

    // Traverse the submaps in order
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{smx, smy, zlev} );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build transparency cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          zlev );
                continue;
            }

            const point_bub_ms sm_offset = coords::project_to<coords::ms>( point_bub_sm( smx, smy ) );

            if( !rebuild_all && !map_cache.transparency_cache_dirty[smx * MAPSIZE + smy] ) {
                continue;
            }

            // calculates transparency of a single tile
            // x,y - coords in map local coords
            auto calc_transp = [&]( const point_bub_ms & p ) {
                const point_sm_ms sp = rebase_sm( p - sm_offset );
                float value = LIGHT_TRANSPARENCY_OPEN_AIR;

                if( !( cur_submap->get_ter( sp ).obj().transparent &&
                       cur_submap->get_furn( sp ).obj().transparent ) ) {
                    return std::make_pair( LIGHT_TRANSPARENCY_SOLID, LIGHT_TRANSPARENCY_SOLID );
                }
                if( outside_cache[p.x()][p.y()] ) {
                    // FIXME: Places inside vehicles haven't been marked as
                    // inside yet so this is incorrectly penalising for
                    // weather in vehicles.
                    value *= sight_penalty;
                }
                float value_wo_fields = value;
                for( const auto &fld : cur_submap->get_field( sp ) ) {
                    const field_intensity_level &i_level = fld.second.get_intensity_level();
                    if( i_level.transparent ) {
                        continue;
                    }
                    // Fields are either transparent or not, however we want some to be translucent
                    value = value * i_level.translucency;
                }
                // TODO: [lightmap] Have glass reduce light as well.
                // Note, binary transluceny is implemented in build_vision_transparency_cache below
                return std::make_pair( value, value_wo_fields );
            };

            if( cur_submap->is_uniform() ) {
                float value;
                float dummy;
                std::tie( value, dummy ) = calc_transp( sm_offset );
                // if rebuild_all==true all values were already set to LIGHT_TRANSPARENCY_OPEN_AIR
                if( !rebuild_all || value != LIGHT_TRANSPARENCY_OPEN_AIR ) {
                    bool opaque = value <= LIGHT_TRANSPARENCY_SOLID;
                    for( int sx = 0; sx < SEEX; ++sx ) {
                        // init all sy indices in one go
                        std::uninitialized_fill_n( &transparency_cache[sm_offset.x() + sx][sm_offset.y()], SEEY, value );
                        if( opaque ) {
                            auto &bs = transparent_cache_wo_fields[sm_offset.x() + sx];
                            for( int i = 0; i < SEEY; i++ ) {
                                bs[sm_offset.y() + i] = false;
                            }
                        }
                    }
                }
            } else {
                for( int sx = 0; sx < SEEX; ++sx ) {
                    const int x = sx + sm_offset.x();
                    for( int sy = 0; sy < SEEY; ++sy ) {
                        const int y = sy + sm_offset.y();
                        float transp_wo_fields;
                        std::tie( transparency_cache[x][y], transp_wo_fields ) = calc_transp( {x, y } );
                        transparent_cache_wo_fields[x][y] = transp_wo_fields > LIGHT_TRANSPARENCY_SOLID;
                    }
                }
            }
        }
    }
    //build_vision_transparency_cache copies the transparency_cache so don't reset transparency_cache_dirty until it's resolved
    return true;
}

bool map::build_vision_transparency_cache( int zlev )
{
    level_cache &map_cache = get_cache( zlev );

    // We copy the transparency_cache so we need to recalc if it's dirty
    if( map_cache.transparency_cache_dirty.none() /*&& map_cache.vision_transparency_cache_dirty.none()*/ ) {
        return false;
    }

    const cata::mdarray<float, point_bub_ms> &transparency_cache = map_cache.transparency_cache;
    cata::mdarray<float, point_bub_ms> &vision_transparency_cache = map_cache.vision_transparency_cache;

    // TODO: Should only copy if transparency_cache was dirty
    memcpy( &vision_transparency_cache, &transparency_cache, sizeof( transparency_cache ) );

    const Character &player_character = get_player_character();
    const tripoint_bub_ms p = player_character.pos_bub();
    const bool is_player_z = p.z() == zlev;

    bool dirty = false;

    if( is_player_z ) {
        // This segment handles vision when the player is crouching or prone. It only checks adjacent tiles.
        // If you change this, also consider creature::sees and map::obstacle_coverage.
        // TODO: Is fairly nonsense because it changes vision for everyone only (eg if you @ crouch behind the window W then the NPC N and monster M can't see each other bc the window is counted as opaque)
        // .N.
        // .@.
        // #W#
        // .M.
        const bool is_crouching = player_character.is_crouching();
        const bool low_profile = player_character.has_effect( effect_quadruped_full ) &&
                                 player_character.is_running();
        const bool is_prone = player_character.is_prone();
        if( is_crouching || is_prone || low_profile ) {
            for( const tripoint_bub_ms &loc : points_in_radius( p, 1 ) ) {
                if( loc != p && coverage( loc ) >= 30 ) {
                    // If we're crouching or prone behind an obstacle, we can't see past it.
                    dirty |= vision_transparency_cache[loc.x()][loc.y()] != LIGHT_TRANSPARENCY_SOLID;
                    vision_transparency_cache[loc.x()][loc.y()] = LIGHT_TRANSPARENCY_SOLID;
                }
            }
        }
    }

    // This segment handles blocking vision through TRANSLUCENT flagged terrain.
    // Traverse the submaps in order (else map::ter() calls get_submap each time)
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{smx, smy, zlev} );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build transparency cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          zlev );
                continue;
            }
            if( !map_cache.transparency_cache_dirty[smx * MAPSIZE + smy] ) {
                continue;
            }
            for( int smi = 0; smi < SEEX; smi++ ) {
                for( int smj = 0; smj < SEEY; smj++ ) {
                    if( cur_submap->get_ter( point_sm_ms{smi, smj} ).obj().has_flag(
                            ter_furn_flag::TFLAG_TRANSLUCENT ) ) {
                        const int i = smi + ( smx * SEEX );
                        const int j = smj + ( smy * SEEY );
                        dirty |= vision_transparency_cache[i][j] != LIGHT_TRANSPARENCY_SOLID;
                        vision_transparency_cache[i][j] = LIGHT_TRANSPARENCY_SOLID;
                    }
                }
            }
        }
    }

    // The tile player is standing on should always be visible
    // Shouldn't this be handled in the player's seen cache instead??
    if( is_player_z && inbounds( p ) ) {
        vision_transparency_cache[p.x()][p.y()] = LIGHT_TRANSPARENCY_OPEN_AIR;
    }

    map_cache.transparency_cache_dirty.reset();
    return dirty;
}

void map::apply_character_light( Character &p )
{
    if( p.has_effect( effect_onfire ) ) {
        apply_light_source( p.pos_bub(), 8 );
    } else if( p.has_effect( effect_haslight ) ) {
        apply_light_source( p.pos_bub(), 4 );
    }

    const float held_luminance = p.active_light();
    if( held_luminance > LIGHT_AMBIENT_LOW ) {
        apply_light_source( p.pos_bub(), held_luminance );
    }

    if( held_luminance >= 4 && held_luminance > ambient_light_at( p.pos_bub() ) - 0.5f ) {
        p.add_effect( effect_haslight, 1_turns );
    }
}

// This function raytraces starting at the upper limit of the simulated area descending
// toward the lower limit. Since it's sunlight, the rays are parallel.
// Each layer consults the next layer up to determine the intensity of the light that reaches it.
// Once this is complete, additional operations add more dynamic lighting.
void map::build_sunlight_cache( int pzlev )
{
    const int zlev_min = -OVERMAP_DEPTH;
    // Start at the topmost populated zlevel to avoid unnecessary raycasting
    // Plus one zlevel to prevent clipping inside structures
    const int zlev_max = clamp( calc_max_populated_zlev() + 1, std::min( pzlev + 1, OVERMAP_HEIGHT ),
                                OVERMAP_HEIGHT );

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
        auto &lm = map_cache.lm;
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
        const auto &prev_lm = prev_map_cache.lm;
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

void map::generate_lightmap( const int zlev )
{
    level_cache &map_cache = get_cache( zlev );
    auto &lm = map_cache.lm;
    auto &sm = map_cache.sm;
    auto &outside_cache = map_cache.outside_cache;
    auto &prev_floor_cache = get_cache( clamp( zlev + 1, -OVERMAP_DEPTH, OVERMAP_DEPTH ) ).floor_cache;
    bool top_floor = zlev == OVERMAP_DEPTH;
    lm.fill( four_quadrants{} );
    sm.fill( 0 );

    /* Bulk light sources wastefully cast rays into neighbors; a burning hospital can produce
         significant slowdown, so for stuff like fire and lava:
     * Step 1: Store the position and luminance in buffer via add_light_source, for efficient
         checking of neighbors.
     * Step 2: After everything else, iterate buffer and apply_light_source only in non-redundant
         directions
     * Step 3: ????
     * Step 4: Profit!
     */
    auto &light_source_buffer = map_cache.light_source_buffer;
    light_source_buffer.fill( 0 );

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

    build_sunlight_cache( zlev );

    apply_character_light( get_player_character() );
    for( npc &guy : g->all_npcs() ) {
        apply_character_light( guy );
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
                                    apply_directional_light( p, dir_d[i], source_light );
                                } else {
                                    update_light_quadrants( lm[p.x()][p.y()], source_light, dir_quadrants[i][0] );
                                    update_light_quadrants( lm[p.x()][p.y()], source_light, dir_quadrants[i][1] );
                                }
                            }
                        }
                    }

                    if( cur_submap->get_lum( { sx, sy } ) ) {
                        add_light_from_items( p, i_at( p ) );
                    }

                    const ter_id &terrain = cur_submap->get_ter( { sx, sy } );
                    if( terrain->light_emitted > 0 ) {
                        add_light_source( p, terrain->light_emitted );
                    }
                    const furn_id &furniture = cur_submap->get_furn( {sx, sy } );
                    if( furniture->light_emitted > 0 ) {
                        add_light_source( p, furniture->light_emitted );
                    }

                    for( const auto &fld : cur_submap->get_field( { sx, sy } ) ) {
                        const field_entry *cur = &fld.second;
                        const int light_emitted = cur->get_intensity_level().light_emitted;
                        if( light_emitted > 0 ) {
                            add_light_source( p, light_emitted );
                        }
                        const float light_override = cur->get_intensity_level().local_light_override;
                        if( light_override >= 0.0f ) {
                            lm_override.emplace_back( p, light_override );
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
        const tripoint_bub_ms mp = critter.pos_bub();
        if( inbounds( mp ) ) {
            if( critter.has_effect( effect_onfire ) ) {
                apply_light_source( mp, 8 );
            }
            // TODO: [lightmap] Attach natural light brightness to creatures
            // TODO: [lightmap] Allow creatures to have light attacks (i.e.: eyebot)
            // TODO: [lightmap] Allow creatures to have facing and arc lights
            float critter_luminance = critter.calculate_by_enchantment( critter.type->luminance,
                                      enchant_vals::mod::LUMINATION, true );
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

        float veh_luminance = 0.0f;
        float iteration = 1.0f;

        for( const vehicle_part *pt : lights ) {
            const vpart_info &vp = pt->info();
            if( vp.has_flag( VPFLAG_CONE_LIGHT ) ||
                vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                veh_luminance += vp.bonus / iteration;
                iteration = iteration * 1.1f;
            }
        }

        for( const vehicle_part *pt : lights ) {
            const vpart_info &vp = pt->info();
            tripoint_bub_ms src = v->bub_part_pos( *this, *pt );

            if( !inbounds( src ) ) {
                continue;
            }

            if( vp.has_flag( VPFLAG_CONE_LIGHT ) ) {
                if( veh_luminance > lit_level::LIT ) {
                    add_light_source( src, M_SQRT2 ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance,
                                     45_degrees );
                }

            } else if( vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                if( veh_luminance > lit_level::LIT ) {
                    add_light_source( src, M_SQRT2 ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance,
                                     90_degrees );
                }

            } else if( vp.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ) {
                if( vp.has_flag( VPFLAG_WALL_MOUNTED ) ) {
                    tileray tdir( v->face.dir() + pt->direction );
                    tdir.advance();
                    tripoint_bub_ms offset = src;
                    offset.x() = src.x() + tdir.dx();
                    offset.y() = src.y() + tdir.dy();
                    add_light_source( offset, M_SQRT2 ); // Add a little surrounding light
                    apply_light_arc( offset, v->face.dir() + pt->direction, vp.bonus, 180_degrees );
                } else {
                    add_light_source( src, M_SQRT2 ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, vp.bonus, 180_degrees );
                }

            } else if( vp.has_flag( VPFLAG_CIRCLE_LIGHT ) ) {
                const bool odd_turn = calendar::once_every( 2_turns );
                if( ( odd_turn && vp.has_flag( VPFLAG_ODDTURN ) ) ||
                    ( !odd_turn && vp.has_flag( VPFLAG_EVENTURN ) ) ||
                    ( !( vp.has_flag( VPFLAG_EVENTURN ) || vp.has_flag( VPFLAG_ODDTURN ) ) ) ) {

                    add_light_source( src, vp.bonus );
                }

            } else {
                add_light_source( src, vp.bonus );
            }
        }

        for( const vpart_reference &vpr : v->get_any_parts( VPFLAG_CARGO ) ) {
            const tripoint_bub_ms pos = vpr.pos_bub( *this );
            if( !inbounds( pos ) || vpr.info().has_flag( "COVERED" ) ) {
                continue;
            }
            add_light_from_items( pos, vpr.items() );
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
        if( light_source_buffer[p.x()][p.y()] > 0.0 ) {
            apply_light_source( p, light_source_buffer[p.x()][p.y()] );
        }
    }
    for( const std::pair<tripoint_bub_ms, float> &elem : lm_override ) {
        lm[elem.first.x()][elem.first.y()].fill( elem.second );
    }
}

void map::add_light_source( const tripoint_bub_ms &p, float luminance )
{
    auto &light_source_buffer = get_cache( p.z() ).light_source_buffer;
    light_source_buffer[p.x()][p.y()] = std::max( luminance, light_source_buffer[p.x()][p.y()] );
}

// Tile light/transparency: 3D

lit_level map::light_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return lit_level::DARK;    // Out of bounds
    }

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

float map::light_transparency( const tripoint_bub_ms &p ) const
{
    return get_cache_ref( p.z() ).transparency_cache[p.x()][p.y()];
}

// End of tile light/transparency

map::apparent_light_info map::apparent_light_helper( const level_cache &map_cache,
        const tripoint_bub_ms &p )
{
    avatar const &u = get_avatar();
    const int dist = rl_dist( u.pos_bub(), p );
    const float abs_vis =
        std::max( map_cache.seen_cache[p.x()][p.y()], map_cache.camera_cache[p.x()][p.y()] );
    const float vis = dist > u.unimpaired_range() ? map_cache.camera_cache[p.x()][p.y()] : abs_vis;
    const bool obstructed = vis <= LIGHT_TRANSPARENCY_SOLID + 0.1;
    const bool abs_obstructed = abs_vis <= LIGHT_TRANSPARENCY_SOLID + 0.1;

    auto is_opaque = [&map_cache]( const point_bub_ms & p ) {
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
            if( ( rl_dist( u.pos_bub().xy(), neighbour ) > u.unimpaired_range() &&
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
    const int dist = rl_dist( player_character.pos_bub(), p );

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

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLight( cata::mdarray<Out, point_bub_ms> &output_cache,
                const cata::mdarray<T, point_bub_ms> &input_array,
                const point_bub_ms &offset, int offsetDistance,
                T numerator = VISIBILITY_FULL,
                int row = 1, float start = 1.0f, float end = 0.0f,
                T cumulative_transparency = T( LIGHT_TRANSPARENCY_OPEN_AIR ) );

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLight( cata::mdarray<Out, point_bub_ms> &output_cache,
                const cata::mdarray<T, point_bub_ms> &input_array,
                const point_bub_ms &offset, const int offsetDistance, const T numerator,
                const int row, float start, const float end, T cumulative_transparency )
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

            const int dist = rl_dist( tripoint::zero, delta ) + offsetDistance;
            last_intensity = calc( numerator, cumulative_transparency, dist );

            T new_transparency = input_array[ current.x ][ current.y ];

            if( check( new_transparency, last_intensity ) ) {
                update_output( output_cache[current.x][current.y], last_intensity,
                               quadrant::default_ );
            } else {
                update_output( output_cache[current.x][current.y], last_intensity, quad );
            }

            if( new_transparency == current_transparency ) {
                newStart = leadingEdge;
                continue;
            }
            // Only cast recursively if previous span was not opaque.
            if( check( current_transparency, last_intensity ) ) {
                castLight<xx, xy, yx, yy, T, Out, calc, check, update_output, accumulate>(
                    output_cache, input_array, offset, offsetDistance,
                    numerator, distance + 1, start, trailingEdge,
                    accumulate( cumulative_transparency, current_transparency, distance ) );
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
 * coordinates. Returns a lightmap for a result where the values represent a
 * percentage of fully lit.
 *
 * A value equal to or below 0 means that cell is not in the
 * field of view, whereas a value equal to or above 1 means that cell is
 * in the field of view.
 *
 * @param origin the starting location
 * @param target_z Z-level to draw light map on
 */
void map::build_seen_cache( const tripoint_bub_ms &origin, const int target_z, int extension_range,
                            bool cumulative, bool camera, int penalty )
{
    level_cache &map_cache = get_cache( target_z );
    using mdarray = cata::mdarray<float, point_bub_ms>;
    mdarray &transparency_cache = map_cache.vision_transparency_cache;
    mdarray &seen_cache = map_cache.seen_cache;
    mdarray &camera_cache = map_cache.camera_cache;
    mdarray &out_cache = camera ? camera_cache : seen_cache;

    constexpr float light_transparency_solid = LIGHT_TRANSPARENCY_SOLID;
    constexpr int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    if( !cumulative ) {
        std::uninitialized_fill_n(
            &camera_cache[0][0], map_dimensions, light_transparency_solid );
    }

    // Cache the caches (pointers to them)
    array_of_grids_of<const float> transparency_caches;
    array_of_grids_of<float> seen_caches;
    array_of_grids_of<const bool> floor_caches;
    vertical_direction directions_to_cast = vertical_direction::BOTH;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        level_cache &cur_cache = get_cache( z );
        transparency_caches[z + OVERMAP_DEPTH] = &cur_cache.vision_transparency_cache;
        seen_caches[z + OVERMAP_DEPTH] = camera ? &cur_cache.camera_cache : &cur_cache.seen_cache;
        floor_caches[z + OVERMAP_DEPTH] = &cur_cache.floor_cache;
        if( !cumulative ) {
            std::uninitialized_fill_n(
                &( *seen_caches[z + OVERMAP_DEPTH] )[0][0], map_dimensions, light_transparency_solid );
        }
        cur_cache.seen_cache_dirty = false;
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
    seen_cache_process_ledges( seen_caches, floor_caches, std::nullopt );

    const optional_vpart_position vp = veh_at( origin );
    if( !vp ) {
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
            ( *mocache )[mirror_pos.x()][mirror_pos.y()] = LIGHT_TRANSPARENCY_OPEN_AIR;
        }

        // TODO: Factor in the mirror facing and only cast in the
        // directions the player's line of sight reflects to.
        //
        // The naive solution of making the mirrors act like a second player
        // at an offset appears to give reasonable results though.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            *mocache, transparency_cache, mirror_pos.xy(), offsetDistance );
    }
}

void map::seen_cache_process_ledges( array_of_grids_of<float> &seen_caches,
                                     const array_of_grids_of<const bool> &floor_caches,
                                     const std::optional<tripoint_bub_ms> &override_p ) const
{
    Character &player_character = get_player_character();
    // If override is not given, use player character for calculations
    const tripoint_bub_ms origin = override_p.value_or( player_character.pos_bub() );
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
                            if( override_p ? ledge_coverage( origin, p ) > 100 : ledge_coverage( player_character,
                                    p ) > 100 ) {
                                ( *seen_caches[cache_z] )[p.x()][p.y()] = 0.0f;
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
    cata::mdarray<four_quadrants, point_bub_ms> &lm = cache.lm;
    cata::mdarray<float, point_bub_ms> &sm = cache.sm;
    cata::mdarray<float, point_bub_ms> &transparency_cache =
        cache.transparency_cache;
    cata::mdarray<float, point_bub_ms> &light_source_buffer =
        cache.light_source_buffer;

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

    /* If we're a 5 luminance fire , we skip casting rays into ey && sx if we have
         neighboring fires to the north and west that were applied via light_source_buffer
       If there's a 1 luminance candle east in buffer, we still cast rays into ex since it's smaller
       If there's a 100 luminance magnesium flare south added via apply_light_source instead od
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
    bool north = p2.y() != 0 && light_source_buffer[p2.x()][p2.y() - 1] < luminance;
    bool south = p2.y() != peer_inbounds && light_source_buffer[p2.x()][p2.y() + 1] < luminance;
    bool east = p2.x() != peer_inbounds && light_source_buffer[p2.x() + 1][p2.y()] < luminance;
    bool west = p2.x() != 0 && light_source_buffer[p2.x() - 1][p2.y()] < luminance;

    if( north ) {
        castLight < 1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
        castLight < -1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    }

    if( east ) {
        castLight < 0, -1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
        castLight < 0, -1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    }

    if( south ) {
        castLight<1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, p2, 0, luminance );
        castLight < -1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    }

    if( west ) {
        castLight<0, 1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, p2, 0, luminance );
        castLight < 0, 1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    }
}

void map::apply_directional_light( const tripoint_bub_ms &p, int direction, float luminance )
{
    const point_bub_ms p2( p.xy() );

    level_cache &cache = get_cache( p.z() );
    cata::mdarray<four_quadrants, point_bub_ms> &lm = cache.lm;
    cata::mdarray<float, point_bub_ms> &transparency_cache =
        cache.transparency_cache;

    if( direction == 90 ) {
        castLight < 1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
        castLight < -1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    } else if( direction == 0 ) {
        castLight < 0, -1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
        castLight < 0, -1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    } else if( direction == 270 ) {
        castLight<1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, p2, 0, luminance );
        castLight < -1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    } else if( direction == 180 ) {
        castLight<0, 1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, p2, 0, luminance );
        castLight < 0, 1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, p2, 0, luminance );
    }
}

void map::apply_light_arc( const tripoint_bub_ms &p, const units::angle &angle, float luminance,
                           const units::angle &wideangle )
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

        // i is positive
        switch( i % 8 ) {
            case 0:
                castLight < 0, -1, -1, 0, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, tan( end_angle ), tan( start_angle ) );
                break;
            case 1:
                castLight < -1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, cot( start_angle ), cot( end_angle ) );
                break;
            case 2:
                castLight < 1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, -cot( end_angle ), -cot( start_angle ) );
                break;
            case 3:
                castLight < 0, 1, -1, 0, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, -tan( start_angle ), -tan( end_angle ) );
                break;
            case 4:
                castLight < 0, 1, 1, 0, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency >(
                              lm, transparency_cache, p2, 0, luminance, 1, tan( end_angle ), tan( start_angle ) );
                break;
            case 5:
                castLight < 1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency >(
                              lm, transparency_cache, p2, 0, luminance, 1, cot( start_angle ), cot( end_angle ) );
                break;
            case 6:
                castLight < -1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, -cot( end_angle ), -cot( start_angle ) );
                break;
            case 7:
                castLight < 0, -1, 1, 0, float, four_quadrants, light_calc, light_check,
                          update_light_quadrants, accumulate_transparency > (
                              lm, transparency_cache, p2, 0, luminance, 1, -tan( start_angle ), -tan( end_angle ) );
                break;
        }
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
