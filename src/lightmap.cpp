#include "lightmap.h" // IWYU pragma: associated
#include "shadowcasting.h" // IWYU pragma: associated

#include <cmath>
#include <cstring>

#include "fragment_cloud.h" // IWYU pragma: keep
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "submap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "vpart_reference.h"
#include "weather.h"

#define LIGHTMAP_CACHE_X MAPSIZE_X
#define LIGHTMAP_CACHE_Y MAPSIZE_Y

static constexpr point lightmap_boundary_min( point_zero );
static constexpr point lightmap_boundary_max( LIGHTMAP_CACHE_X, LIGHTMAP_CACHE_Y );
static constexpr point lightmap_clearance_min( point_zero );
static constexpr point lightmap_clearance_max( 1, 1 );

const rectangle lightmap_boundaries( lightmap_boundary_min, lightmap_boundary_max );
const rectangle lightmap_clearance( lightmap_clearance_min, lightmap_clearance_max );

const efftype_id effect_onfire( "onfire" );
const efftype_id effect_haslight( "haslight" );

constexpr double PI     = 3.14159265358979323846;
constexpr double HALFPI = 1.57079632679489661923;
constexpr double SQRT_2 = 1.41421356237309504880;

std::string four_quadrants::to_string() const
{
    return string_format( "(%.2f,%.2f,%.2f,%.2f)",
                          ( *this )[quadrant::NE], ( *this )[quadrant::SE],
                          ( *this )[quadrant::SW], ( *this )[quadrant::NW] );
}

void map::add_light_from_items( const tripoint &p, std::list<item>::iterator begin,
                                std::list<item>::iterator end )
{
    for( auto itm_it = begin; itm_it != end; ++itm_it ) {
        float ilum = 0.0; // brightness
        int iwidth = 0; // 0-360 degrees. 0 is a circular light_source
        int idir = 0;   // otherwise, it's a light_arc pointed in this direction
        if( itm_it->getlight( ilum, iwidth, idir ) ) {
            if( iwidth > 0 ) {
                apply_light_arc( p, idir, ilum, iwidth );
            } else {
                add_light_source( p, ilum );
            }
        }
    }
}

// TODO Consider making this just clear the cache and dynamically fill it in as trans() is called
void map::build_transparency_cache( const int zlev )
{
    auto &map_cache = get_cache( zlev );
    auto &transparency_cache = map_cache.transparency_cache;
    auto &outside_cache = map_cache.outside_cache;

    if( !map_cache.transparency_cache_dirty ) {
        return;
    }

    // Default to just barely not transparent.
    std::uninitialized_fill_n(
        &transparency_cache[0][0], MAPSIZE_X * MAPSIZE_Y,
        static_cast<float>( LIGHT_TRANSPARENCY_OPEN_AIR ) );

    float sight_penalty = weather_data( g->weather ).sight_penalty;

    // Traverse the submaps in order
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( {smx, smy, zlev} );

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const int x = sx + smx * SEEX;
                    const int y = sy + smy * SEEY;

                    auto &value = transparency_cache[x][y];

                    if( !( cur_submap->ter[sx][sy].obj().transparent &&
                           cur_submap->frn[sx][sy].obj().transparent ) ) {
                        value = LIGHT_TRANSPARENCY_SOLID;
                        continue;
                    }

                    if( outside_cache[x][y] ) {
                        // FIXME: Places inside vehicles haven't been marked as
                        // inside yet so this is incorrectly penalising for
                        // weather in vehicles.
                        value *= sight_penalty;
                    }

                    for( const auto &fld : cur_submap->fld[sx][sy] ) {
                        const field_entry &cur = fld.second;
                        const field_id type = cur.getFieldType();
                        const int density = cur.getFieldDensity();

                        if( fieldlist[type].transparent[density - 1] ) {
                            continue;
                        }

                        // Fields are either transparent or not, however we want some to be translucent
                        switch( type ) {
                            case fd_cigsmoke:
                            case fd_weedsmoke:
                            case fd_cracksmoke:
                            case fd_methsmoke:
                            case fd_relax_gas:
                                value *= 5;
                                break;
                            case fd_smoke:
                            case fd_incendiary:
                            case fd_toxic_gas:
                            case fd_tear_gas:
                                if( density == 3 ) {
                                    value = LIGHT_TRANSPARENCY_SOLID;
                                } else if( density == 2 ) {
                                    value *= 10;
                                }
                                break;
                            case fd_nuke_gas:
                                value *= 10;
                                break;
                            case fd_fire:
                                value *= 1.0 - ( density * 0.3 );
                                break;
                            default:
                                value = LIGHT_TRANSPARENCY_SOLID;
                                break;
                        }
                        // TODO: [lightmap] Have glass reduce light as well
                    }
                }
            }
        }
    }
    map_cache.transparency_cache_dirty = false;
}

void map::apply_character_light( player &p )
{
    if( p.has_effect( effect_onfire ) ) {
        apply_light_source( p.pos(), 8 );
    } else if( p.has_effect( effect_haslight ) ) {
        apply_light_source( p.pos(), 4 );
    }

    const float held_luminance = p.active_light();
    if( held_luminance > LIGHT_AMBIENT_LOW ) {
        apply_light_source( p.pos(), held_luminance );
    }

    if( held_luminance >= 4 && held_luminance > ambient_light_at( p.pos() ) - 0.5f ) {
        p.add_effect( effect_haslight, 1_turns );
    }
}

void map::generate_lightmap( const int zlev )
{
    auto &map_cache = get_cache( zlev );
    auto &lm = map_cache.lm;
    auto &sm = map_cache.sm;
    auto &outside_cache = map_cache.outside_cache;
    std::memset( lm, 0, sizeof( lm ) );
    std::memset( sm, 0, sizeof( sm ) );

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
    std::memset( light_source_buffer, 0, sizeof( light_source_buffer ) );

    constexpr std::array<int, 4> dir_x = {{  0, -1, 1, 0 }};    //    [0]
    constexpr std::array<int, 4> dir_y = {{ -1,  0, 0, 1 }};    // [1][X][2]
    constexpr std::array<int, 4> dir_d = {{ 90, 0, 180, 270 }}; //    [3]
    constexpr std::array<std::array<quadrant, 2>, 4> dir_quadrants = {{
            {{ quadrant::NE, quadrant::NW }},
            {{ quadrant::SW, quadrant::NW }},
            {{ quadrant::SE, quadrant::NE }},
            {{ quadrant::SE, quadrant::SW }},
        }
    };

    const float natural_light  = g->natural_light_level( zlev );
    const float inside_light = ( natural_light > LIGHT_SOURCE_BRIGHT ) ?
                               LIGHT_AMBIENT_LOW + 1.0 : LIGHT_AMBIENT_MINIMAL;
    // Apply sunlight, first light source so just assign
    for( int sx = 0; sx < LIGHTMAP_CACHE_X; ++sx ) {
        for( int sy = 0; sy < LIGHTMAP_CACHE_Y; ++sy ) {
            // In bright light indoor light exists to some degree
            if( !outside_cache[sx][sy] ) {
                lm[sx][sy].fill( inside_light );
            } else {
                lm[sx][sy].fill( natural_light );
            }
        }
    }

    apply_character_light( g->u );
    for( npc &guy : g->all_npcs() ) {
        apply_character_light( guy );
    }

    // Traverse the submaps in order
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, zlev } );

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const int x = sx + smx * SEEX;
                    const int y = sy + smy * SEEY;
                    const tripoint p( x, y, zlev );
                    // Project light into any openings into buildings.
                    if( !outside_cache[p.x][p.y] ) {
                        // Apply light sources for external/internal divide
                        for( int i = 0; i < 4; ++i ) {
                            if( generic_inbounds( { p.x + dir_x[i], p.y + dir_y[i] },
                                                  lightmap_boundaries, lightmap_clearance
                                                ) && outside_cache[p.x + dir_x[i]][p.y + dir_y[i]]
                              ) {
                                if( light_transparency( p ) > LIGHT_TRANSPARENCY_SOLID ) {
                                    update_light_quadrants(
                                        lm[p.x][p.y], natural_light, quadrant::default_ );
                                    apply_directional_light( p, dir_d[i], natural_light );
                                } else {
                                    update_light_quadrants(
                                        lm[p.x][p.y], natural_light, dir_quadrants[i][0] );
                                    update_light_quadrants(
                                        lm[p.x][p.y], natural_light, dir_quadrants[i][1] );
                                }
                            }
                        }
                    }

                    if( cur_submap->lum[sx][sy] && has_items( p ) ) {
                        auto items = i_at( p );
                        add_light_from_items( p, items.begin(), items.end() );
                    }

                    const ter_id terrain = cur_submap->ter[sx][sy];
                    if( terrain == t_lava ) {
                        add_light_source( p, 50 );
                    } else if( terrain == t_console ) {
                        add_light_source( p, 10 );
                    } else if( terrain == t_thconc_floor_olight ) {
                        add_light_source( p, 120 );
                    } else if( terrain == t_utility_light ) {
                        add_light_source( p, 240 );
                    }

                    for( auto &fld : cur_submap->fld[sx][sy] ) {
                        const field_entry *cur = &fld.second;
                        // TODO: [lightmap] Attach light brightness to fields
                        switch( cur->getFieldType() ) {
                            case fd_fire:
                                if( 3 == cur->getFieldDensity() ) {
                                    add_light_source( p, 160 );
                                } else if( 2 == cur->getFieldDensity() ) {
                                    add_light_source( p, 60 );
                                } else {
                                    add_light_source( p, 20 );
                                }
                                break;
                            case fd_fire_vent:
                            case fd_flame_burst:
                                add_light_source( p, 20 );
                                break;
                            case fd_electricity:
                            case fd_plasma:
                                if( 3 == cur->getFieldDensity() ) {
                                    add_light_source( p, 20 );
                                } else if( 2 == cur->getFieldDensity() ) {
                                    add_light_source( p, 4 );
                                } else {
                                    // Kinda a hack as the square will still get marked.
                                    apply_light_source( p, LIGHT_SOURCE_LOCAL );
                                }
                                break;
                            case fd_incendiary:
                                if( 3 == cur->getFieldDensity() ) {
                                    add_light_source( p, 160 );
                                } else if( 2 == cur->getFieldDensity() ) {
                                    add_light_source( p, 60 );
                                } else {
                                    add_light_source( p, 20 );
                                }
                                break;
                            case fd_laser:
                                apply_light_source( p, 4 );
                                break;
                            case fd_spotlight:
                                add_light_source( p, 80 );
                                break;
                            case fd_dazzling:
                                add_light_source( p, 5 );
                                break;
                            default:
                                //Suppress warnings
                                break;
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
        const tripoint &mp = critter.pos();
        if( inbounds( mp ) ) {
            if( critter.has_effect( effect_onfire ) ) {
                apply_light_source( mp, 8 );
            }
            // TODO: [lightmap] Attach natural light brightness to creatures
            // TODO: [lightmap] Allow creatures to have light attacks (ie: eyebot)
            // TODO: [lightmap] Allow creatures to have facing and arc lights
            if( critter.type->luminance > 0 ) {
                apply_light_source( mp, critter.type->luminance );
            }
        }
    }

    // Apply any vehicle light sources
    VehicleList vehs = get_vehicles();
    for( auto &vv : vehs ) {
        vehicle *v = vv.v;

        auto lights = v->lights( true );

        float veh_luminance = 0.0;
        float iteration = 1.0;

        for( const auto pt : lights ) {
            const auto &vp = pt->info();
            if( vp.has_flag( VPFLAG_CONE_LIGHT ) ||
                vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                veh_luminance += vp.bonus / iteration;
                iteration = iteration * 1.1;
            }
        }

        for( const auto pt : lights ) {
            const auto &vp = pt->info();
            tripoint src = v->global_part_pos3( *pt );

            if( !inbounds( src ) ) {
                continue;
            }

            if( vp.has_flag( VPFLAG_CONE_LIGHT ) ) {
                if( veh_luminance > LL_LIT ) {
                    add_light_source( src, SQRT_2 ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance, 45 );
                }

            } else if( vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                if( veh_luminance > LL_LIT ) {
                    add_light_source( src, SQRT_2 ); // Add a little surrounding light
                    apply_light_arc( src, v->face.dir() + pt->direction, veh_luminance, 90 );
                }

            } else if( vp.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ) {
                add_light_source( src, SQRT_2 ); // Add a little surrounding light
                apply_light_arc( src, v->face.dir() + pt->direction, vp.bonus, 180 );

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

        for( const vpart_reference &vp : v->get_all_parts() ) {
            const size_t p = vp.part_index();
            const tripoint pp = vp.pos();
            if( !inbounds( pp ) ) {
                continue;
            }
            if( vp.has_feature( VPFLAG_CARGO ) && !vp.has_feature( "COVERED" ) ) {
                add_light_from_items( pp, v->get_items( p ).begin(), v->get_items( p ).end() );
            }
        }
    }

    /* Now that we have position and intensity of all bulk light sources, apply_ them
      This may seem like extra work, but take a 12x12 raging inferno:
        unbuffered: (12^2)*(160*4) = apply_light_ray x 92160
        buffered:   (12*4)*(160)   = apply_light_ray x 7680
    */
    const tripoint cache_start( 0, 0, zlev );
    const tripoint cache_end( LIGHTMAP_CACHE_X, LIGHTMAP_CACHE_Y, zlev );
    for( const tripoint &p : points_in_rectangle( cache_start, cache_end ) ) {
        if( light_source_buffer[p.x][p.y] > 0.0 ) {
            apply_light_source( p, light_source_buffer[p.x][p.y] );
        }
    }

    if( g->u.has_active_bionic( bionic_id( "bio_night" ) ) ) {
        for( const tripoint &p : points_in_rectangle( cache_start, cache_end ) ) {
            if( rl_dist( p, g->u.pos() ) < 15 ) {
                lm[p.x][p.y].fill( LIGHT_AMBIENT_MINIMAL );
            }
        }
    }
}

void map::add_light_source( const tripoint &p, float luminance )
{
    auto &light_source_buffer = get_cache( p.z ).light_source_buffer;
    light_source_buffer[p.x][p.y] = std::max( luminance, light_source_buffer[p.x][p.y] );
}

// Tile light/transparency: 3D

lit_level map::light_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return LL_DARK;    // Out of bounds
    }

    const auto &map_cache = get_cache_ref( p.z );
    const auto &lm = map_cache.lm;
    const auto &sm = map_cache.sm;
    if( sm[p.x][p.y] >= LIGHT_SOURCE_BRIGHT ) {
        return LL_BRIGHT;
    }

    const float max_light = lm[p.x][p.y].max();
    if( max_light >= LIGHT_AMBIENT_LIT ) {
        return LL_LIT;
    }

    if( max_light >= LIGHT_AMBIENT_LOW ) {
        return LL_LOW;
    }

    return LL_DARK;
}

float map::ambient_light_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return 0.0f;
    }

    return get_cache_ref( p.z ).lm[p.x][p.y].max();
}

bool map::trans( const tripoint &p ) const
{
    return light_transparency( p ) > LIGHT_TRANSPARENCY_SOLID;
}

float map::light_transparency( const tripoint &p ) const
{
    return get_cache_ref( p.z ).transparency_cache[p.x][p.y];
}

// End of tile light/transparency

map::apparent_light_info map::apparent_light_helper( const level_cache &map_cache,
        const tripoint &p )
{
    const float vis = std::max( map_cache.seen_cache[p.x][p.y], map_cache.camera_cache[p.x][p.y] );
    const bool obstructed = vis <= LIGHT_TRANSPARENCY_SOLID + 0.1;

    auto is_opaque = [&map_cache]( int x, int y ) {
        return map_cache.transparency_cache[x][y] <= LIGHT_TRANSPARENCY_SOLID;
    };

    const bool p_opaque = is_opaque( p.x, p.y );
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
                { { 0, 1 }, {{ quadrant::SE, quadrant::SW }} },
                { { 0, -1 }, {{ quadrant::NE, quadrant::NW }} },
                { { 1, 0 }, {{ quadrant::SE, quadrant::NE }} },
                { { 1, 1 }, {{ quadrant::SE, quadrant::SE }} },
                { { 1, -1 }, {{ quadrant::NE, quadrant::NE }} },
                { {-1, 0 }, {{ quadrant::SW, quadrant::NW }} },
                { {-1, 1 }, {{ quadrant::SW, quadrant::SW }} },
                { {-1, -1 }, {{ quadrant::NW, quadrant::NW }} },
            }
        };

        four_quadrants seen_from( 0 );
        for( const offset_and_quadrants &oq : adjacent_offsets ) {
            const int neighbour_x = p.x + oq.offset.x;
            const int neighbour_y = p.y + oq.offset.y;

            if( !generic_inbounds( { neighbour_x, neighbour_y }, lightmap_boundaries, lightmap_clearance ) ) {
                continue;
            }
            if( is_opaque( neighbour_x, neighbour_y ) ) {
                continue;
            }
            if( map_cache.seen_cache[neighbour_x][neighbour_y] == 0 &&
                map_cache.camera_cache[neighbour_x][neighbour_y] == 0 ) {
                continue;
            }
            // This is a non-opaque visible neighbour, so count visibility from the relevant
            // quadrants
            seen_from[oq.quadrants[0]] = vis;
            seen_from[oq.quadrants[1]] = vis;
        }
        apparent_light = ( seen_from * map_cache.lm[p.x][p.y] ).max();
    } else {
        // This is the simple case, for a non-opaque tile light from all
        // directions is equivalent
        apparent_light = vis * map_cache.lm[p.x][p.y][quadrant::default_];
    }
    return { obstructed, apparent_light };
}

lit_level map::apparent_light_at( const tripoint &p, const visibility_variables &cache ) const
{
    const int dist = rl_dist( g->u.pos(), p );

    // Clairvoyance overrides everything.
    if( dist <= cache.u_clairvoyance ) {
        return LL_BRIGHT;
    }
    const auto &map_cache = get_cache_ref( p.z );
    const apparent_light_info a = apparent_light_helper( map_cache, p );

    // Unimpaired range is an override to strictly limit vision range based on various conditions,
    // but the player can still see light sources.
    if( dist > g->u.unimpaired_range() ) {
        if( !a.obstructed && map_cache.sm[p.x][p.y] > 0.0 ) {
            return LL_BRIGHT_ONLY;
        } else {
            return LL_DARK;
        }
    }
    if( a.obstructed ) {
        if( a.apparent_light > LIGHT_AMBIENT_LIT ) {
            if( a.apparent_light > cache.g_light_level ) {
                // This represents too hazy to see detail,
                // but enough light getting through to illuminate.
                return LL_BRIGHT_ONLY;
            } else {
                // If it's not brighter than the surroundings, it just ends up shadowy.
                return LL_LOW;
            }
        } else {
            return LL_BLANK;
        }
    }
    // Then we just search for the light level in descending order.
    if( a.apparent_light > LIGHT_SOURCE_BRIGHT || map_cache.sm[p.x][p.y] > 0.0 ) {
        return LL_BRIGHT;
    }
    if( a.apparent_light > LIGHT_AMBIENT_LIT ) {
        return LL_LIT;
    }
    if( a.apparent_light > cache.vision_threshold ) {
        return LL_LOW;
    } else {
        return LL_BLANK;
    }
}

bool map::pl_sees( const tripoint &t, const int max_range ) const
{
    if( !inbounds( t ) ) {
        return false;
    }

    if( max_range >= 0 && square_dist( t, g->u.pos() ) > max_range ) {
        return false;    // Out of range!
    }

    const auto &map_cache = get_cache_ref( t.z );
    const apparent_light_info a = apparent_light_helper( map_cache, t );
    const float light_at_player = map_cache.lm[g->u.posx()][g->u.posy()].max();
    return !a.obstructed &&
           ( a.apparent_light > g->u.get_vision_threshold( light_at_player ) ||
             map_cache.sm[t.x][t.y] > 0.0 );
}

bool map::pl_line_of_sight( const tripoint &t, const int max_range ) const
{
    if( !inbounds( t ) ) {
        return false;
    }

    if( max_range >= 0 && square_dist( t, g->u.pos() ) > max_range ) {
        // Out of range!
        return false;
    }

    const auto &map_cache = get_cache_ref( t.z );
    // Any epsilon > 0 is fine - it means lightmap processing visited the point
    return map_cache.seen_cache[t.x][t.y] > 0.0f ||
           map_cache.camera_cache[t.x][t.y] > 0.0f;
}

// For a direction vector defined by x, y, return the quadrant that's the
// source of that direction.  Assumes x != 0 && y != 0
static constexpr quadrant quadrant_from_x_y( int x, int y )
{
    return ( x > 0 ) ?
           ( ( y > 0 ) ? quadrant::NW : quadrant::SW ) :
           ( ( y > 0 ) ? quadrant::NE : quadrant::SE );
}

// Add defaults for when method is invoked for the first time.
template<int xx, int xy, int xz, int yx, int yy, int yz, int zz, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight_segment(
    const std::array<T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const T numerator = 1.0f, const int row = 1,
    float start_major = 0.0f, const float end_major = 1.0f,
    float start_minor = 0.0f, const float end_minor = 1.0f,
    T cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

template<int xx, int xy, int xz, int yx, int yy, int yz, int zz, typename T,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight_segment(
    const std::array<T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &offset, const int offset_distance,
    const T numerator, const int row,
    float start_major, const float end_major,
    float start_minor, const float end_minor,
    T cumulative_transparency )
{
    if( start_major >= end_major || start_minor >= end_minor ) {
        return;
    }

    float radius = 60.0f - offset_distance;

    constexpr int min_z = -OVERMAP_DEPTH;
    constexpr int max_z = OVERMAP_HEIGHT;

    float new_start_minor = 1.0f;

    T last_intensity = 0.0;
    static constexpr tripoint origin( 0, 0, 0 );
    tripoint delta( 0, 0, 0 );
    tripoint current( 0, 0, 0 );
    for( int distance = row; distance <= radius; distance++ ) {
        delta.y = distance;
        bool started_block = false;
        T current_transparency = 0.0f;

        // TODO: Precalculate min/max delta.z based on start/end and distance
        for( delta.z = 0; delta.z <= distance; delta.z++ ) {
            float trailing_edge_major = ( delta.z - 0.5f ) / ( delta.y + 0.5f );
            float leading_edge_major = ( delta.z + 0.5f ) / ( delta.y - 0.5f );
            current.z = offset.z + delta.x * 00 + delta.y * 00 + delta.z * zz;
            if( current.z > max_z || current.z < min_z ) {
                continue;
            } else if( start_major > leading_edge_major ) {
                continue;
            } else if( end_major < trailing_edge_major ) {
                break;
            }

            bool started_span = false;
            const int z_index = current.z + OVERMAP_DEPTH;
            for( delta.x = 0; delta.x <= distance; delta.x++ ) {
                current.x = offset.x + delta.x * xx + delta.y * xy + delta.z * xz;
                current.y = offset.y + delta.x * yx + delta.y * yy + delta.z * yz;
                float trailing_edge_minor = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
                float leading_edge_minor = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

                if( !( current.x >= 0 && current.y >= 0 &&
                       current.x < MAPSIZE_X &&
                       current.y < MAPSIZE_Y ) || start_minor > leading_edge_minor ) {
                    continue;
                } else if( end_minor < trailing_edge_minor ) {
                    break;
                }

                T new_transparency = ( *input_arrays[z_index] )[current.x][current.y];
                // If we're looking at a tile with floor or roof from the floor/roof side,
                //  that tile is actually invisible to us.
                bool floor_block = false;
                if( current.z < offset.z ) {
                    if( z_index < ( OVERMAP_LAYERS - 1 ) &&
                        ( *floor_caches[z_index + 1] )[current.x][current.y] ) {
                        floor_block = true;
                        new_transparency = LIGHT_TRANSPARENCY_SOLID;
                    }
                } else if( current.z > offset.z ) {
                    if( ( *floor_caches[z_index] )[current.x][current.y] ) {
                        floor_block = true;
                        new_transparency = LIGHT_TRANSPARENCY_SOLID;
                    }
                }

                if( !started_block ) {
                    started_block = true;
                    current_transparency = new_transparency;
                }

                const int dist = rl_dist( origin, delta ) + offset_distance;
                last_intensity = calc( numerator, cumulative_transparency, dist );

                if( !floor_block ) {
                    ( *output_caches[z_index] )[current.x][current.y] =
                        std::max( ( *output_caches[z_index] )[current.x][current.y], last_intensity );
                }

                if( !started_span ) {
                    // Need to reset minor slope, because we're starting a new line
                    new_start_minor = leading_edge_minor;
                    // Need more precision or artifacts happen
                    leading_edge_minor = start_minor;
                    started_span = true;
                }

                if( new_transparency == current_transparency ) {
                    // All in order, no need to recurse
                    new_start_minor = leading_edge_minor;
                    continue;
                }

                // We split the block into 4 sub-blocks (sub-frustums actually, this is the view from the origin looking out):
                // +-------+ <- end major
                // |   D   |
                // +---+---+ <- ???
                // | B | C |
                // +---+---+ <- major mid
                // |   A   |
                // +-------+ <- start major
                // ^       ^
                // |       end minor
                // start minor
                // A is previously processed row(s).
                // B is already-processed tiles from current row.
                // C is remainder of current row.
                // D is not yet processed row(s).
                // One we processed fully in 2D and only need to extend in last D
                // Only cast recursively horizontally if previous span was not opaque.
                if( check( current_transparency, last_intensity ) ) {
                    T next_cumulative_transparency = accumulate( cumulative_transparency, current_transparency,
                                                     distance );
                    // Blocks can be merged if they are actually a single rectangle
                    // rather than rectangle + line shorter than rectangle's width
                    const bool merge_blocks = end_minor <= trailing_edge_minor;
                    // trailing_edge_major can be less than start_major
                    const float trailing_clipped = std::max( trailing_edge_major, start_major );
                    const float major_mid = merge_blocks ? leading_edge_major : trailing_clipped;
                    cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                        output_caches, input_arrays, floor_caches,
                        offset, offset_distance, numerator, distance + 1,
                        start_major, major_mid, start_minor, end_minor,
                        next_cumulative_transparency );
                    if( !merge_blocks ) {
                        // One line that is too short to be part of the rectangle above
                        cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                            output_caches, input_arrays, floor_caches,
                            offset, offset_distance, numerator, distance + 1,
                            major_mid, leading_edge_major, start_minor, trailing_edge_minor,
                            next_cumulative_transparency );
                    }
                }

                // One from which we shaved one line ("processed in 1D")
                const float old_start_minor = start_minor;
                // The new span starts at the leading edge of the previous square if it is opaque,
                // and at the trailing edge of the current square if it is transparent.
                if( !check( current_transparency, last_intensity ) ) {
                    start_minor = new_start_minor;
                } else {
                    // Note this is the same slope as one of the recursive calls we just made.
                    start_minor = std::max( start_minor, trailing_edge_minor );
                    start_major = std::max( start_major, trailing_edge_major );
                }

                // leading_edge_major plus some epsilon
                float after_leading_edge_major = ( delta.z + 0.50001f ) / ( delta.y - 0.5f );
                cast_zlight_segment<xx, xy, xz, yx, yy, yz, zz, T, calc, check, accumulate>(
                    output_caches, input_arrays, floor_caches,
                    offset, offset_distance, numerator, distance,
                    after_leading_edge_major, end_major, old_start_minor, start_minor,
                    cumulative_transparency );

                // One we just entered ("processed in 0D" - the first point)
                // No need to recurse, we're processing it right now

                current_transparency = new_transparency;
                new_start_minor = leading_edge_minor;
            }

            if( !check( current_transparency, last_intensity ) ) {
                start_major = leading_edge_major;
            }
        }

        if( !started_block ) {
            // If we didn't scan at least 1 z-level, don't iterate further
            // Otherwise we may "phase" through tiles without checking them
            break;
        }

        if( !check( current_transparency, last_intensity ) ) {
            // If we reach the end of the span with terrain being opaque, we don't iterate further.
            break;
        }
        // Cumulative average of the values encountered.
        cumulative_transparency = accumulate( cumulative_transparency, current_transparency, distance );
    }
}

template<typename T, T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         T( *accumulate )( const T &, const T &, const int & )>
void cast_zlight(
    const std::array<T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const T( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &origin, const int offset_distance, const T numerator )
{
    // Down
    cast_zlight_segment < 0, 1, 0, 1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, 1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, 1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, 1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, 1, 0, -1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, -1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, -1, 0, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, -1, 0, -1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    // Up
    cast_zlight_segment<0, 1, 0, 1, 0, 0, 1, T, calc, check, accumulate>(
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment<1, 0, 0, 0, 1, 0, 1, T, calc, check, accumulate>(
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, 1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, 1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, 1, 0, -1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < 1, 0, 0, 0, -1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );

    cast_zlight_segment < 0, -1, 0, -1, 0, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
    cast_zlight_segment < -1, 0, 0, 0, -1, 0, 1, T, calc, check, accumulate > (
        output_caches, input_arrays, floor_caches, origin, offset_distance, numerator );
}

// I can't figure out how to make implicit instantiation work when the parameters of
// the template-supplied function pointers are involved, so I'm explicitly instantiating instead.
template void cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
    const std::array<float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &origin, const int offset_distance, const float numerator );

template void cast_zlight<fragment_cloud, shrapnel_calc, shrapnel_check, accumulate_fragment_cloud>(
    const std::array<fragment_cloud( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &output_caches,
    const std::array<const fragment_cloud( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS>
    &input_arrays,
    const std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> &floor_caches,
    const tripoint &origin, const int offset_distance, const fragment_cloud numerator );

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLight( Out( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                const T( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                const int offsetX, const int offsetY, const int offsetDistance,
                const T numerator = 1.0,
                const int row = 1, float start = 1.0f, const float end = 0.0f,
                T cumulative_transparency = LIGHT_TRANSPARENCY_OPEN_AIR );

template<int xx, int xy, int yx, int yy, typename T, typename Out,
         T( *calc )( const T &, const T &, const int & ),
         bool( *check )( const T &, const T & ),
         void( *update_output )( Out &, const T &, quadrant ),
         T( *accumulate )( const T &, const T &, const int & )>
void castLight( Out( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                const T( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                const int offsetX, const int offsetY, const int offsetDistance, const T numerator,
                const int row, float start, const float end, T cumulative_transparency )
{
    constexpr quadrant quad = quadrant_from_x_y( -xx - xy, -yx - yy );
    float newStart = 0.0f;
    float radius = 60.0f - offsetDistance;
    if( start < end ) {
        return;
    }
    T last_intensity = 0.0;
    static constexpr tripoint origin( 0, 0, 0 );
    tripoint delta( 0, 0, 0 );
    for( int distance = row; distance <= radius; distance++ ) {
        delta.y = -distance;
        bool started_row = false;
        T current_transparency = 0.0;
        float away = start - ( -distance + 0.5f ) / ( -distance -
                     0.5f ); //The distance between our first leadingEdge and start

        //We initialise delta.x to -distance adjusted so that the commented start < leadingEdge condition below is never false
        delta.x = -distance + std::max( static_cast<int>( ceil( away * ( -distance - 0.5f ) ) ),
                                        0 );

        for( ; delta.x <= 0; delta.x++ ) {
            int currentX = offsetX + delta.x * xx + delta.y * xy;
            int currentY = offsetY + delta.x * yx + delta.y * yy;
            float trailingEdge = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
            float leadingEdge = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

            if( !( currentX >= 0 && currentY >= 0 && currentX < MAPSIZE_X &&
                   currentY < MAPSIZE_Y ) /* || start < leadingEdge */ ) {
                continue;
            } else if( end > trailingEdge ) {
                break;
            }
            if( !started_row ) {
                started_row = true;
                current_transparency = input_array[ currentX ][ currentY ];
            }

            const int dist = rl_dist( origin, delta ) + offsetDistance;
            last_intensity = calc( numerator, cumulative_transparency, dist );

            T new_transparency = input_array[ currentX ][ currentY ];

            if( check( new_transparency, last_intensity ) ) {
                update_output( output_cache[currentX][currentY], last_intensity,
                               quadrant::default_ );
            } else {
                update_output( output_cache[currentX][currentY], last_intensity, quad );
            }

            if( new_transparency == current_transparency ) {
                newStart = leadingEdge;
                continue;
            }
            // Only cast recursively if previous span was not opaque.
            if( check( current_transparency, last_intensity ) ) {
                castLight<xx, xy, yx, yy, T, Out, calc, check, update_output, accumulate>(
                    output_cache, input_array, offsetX, offsetY, offsetDistance,
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
void castLightAll( Out( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                   const T( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                   const int offsetX, const int offsetY, int offsetDistance, T numerator )
{
    castLight<0, 1, 1, 0, T, Out, calc, check, update_output, accumulate>(
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );
    castLight<1, 0, 0, 1, T, Out, calc, check, update_output, accumulate>(
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );

    castLight < 0, -1, 1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );
    castLight < -1, 0, 0, 1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );

    castLight < 0, 1, -1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );
    castLight < 1, 0, 0, -1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );

    castLight < 0, -1, -1, 0, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );
    castLight < -1, 0, 0, -1, T, Out, calc, check, update_output, accumulate > (
        output_cache, input_array, offsetX, offsetY, offsetDistance, numerator );
}

template void castLightAll<float, four_quadrants, sight_calc, sight_check,
                           update_light_quadrants, accumulate_transparency>(
                               four_quadrants( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
                               const float ( &input_array )[MAPSIZE_X][MAPSIZE_Y],
                               const int offsetX, const int offsetY, int offsetDistance, float numerator );

template void
castLightAll<fragment_cloud, fragment_cloud, shrapnel_calc, shrapnel_check,
             update_fragment_cloud, accumulate_fragment_cloud>
(
    fragment_cloud( &output_cache )[MAPSIZE_X][MAPSIZE_Y],
    const fragment_cloud( &input_array )[MAPSIZE_X][MAPSIZE_Y],
    const int offsetX, const int offsetY, int offsetDistance, const fragment_cloud numerator );

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
void map::build_seen_cache( const tripoint &origin, const int target_z )
{
    auto &map_cache = get_cache( target_z );
    float ( &transparency_cache )[MAPSIZE_X][MAPSIZE_Y] = map_cache.transparency_cache;
    float ( &seen_cache )[MAPSIZE_X][MAPSIZE_Y] = map_cache.seen_cache;
    float ( &camera_cache )[MAPSIZE_X][MAPSIZE_Y] = map_cache.camera_cache;

    constexpr float light_transparency_solid = LIGHT_TRANSPARENCY_SOLID;
    constexpr int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    std::uninitialized_fill_n(
        &seen_cache[0][0], map_dimensions, light_transparency_solid );
    std::uninitialized_fill_n(
        &camera_cache[0][0], map_dimensions, light_transparency_solid );

    if( !fov_3d ) {
        seen_cache[origin.x][origin.y] = LIGHT_TRANSPARENCY_CLEAR;

        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            seen_cache, transparency_cache, origin.x, origin.y, 0 );
    } else {
        if( origin.z == target_z ) {
            seen_cache[origin.x][origin.y] = LIGHT_TRANSPARENCY_CLEAR;
        }

        // Cache the caches (pointers to them)
        std::array<const float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> transparency_caches;
        std::array<float ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> seen_caches;
        std::array<const bool ( * )[MAPSIZE_X][MAPSIZE_Y], OVERMAP_LAYERS> floor_caches;
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            auto &cur_cache = get_cache( z );
            transparency_caches[z + OVERMAP_DEPTH] = &cur_cache.transparency_cache;
            seen_caches[z + OVERMAP_DEPTH] = &cur_cache.seen_cache;
            floor_caches[z + OVERMAP_DEPTH] = &cur_cache.floor_cache;
        }
        cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
            seen_caches, transparency_caches, floor_caches, origin, 0, 1.0 );
    }

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
    for( const vpart_reference &vp : veh->get_avail_parts( VPFLAG_EXTENDS_VISION ) ) {
        const tripoint mirror_pos = vp.pos();
        // We can utilize the current state of the seen cache to determine
        // if the player can see the mirror from their position.
        if( !vp.info().has_flag( "CAMERA" ) &&
            seen_cache[mirror_pos.x][mirror_pos.y] < LIGHT_TRANSPARENCY_SOLID + 0.1 ) {
            continue;
        } else if( !vp.info().has_flag( "CAMERA_CONTROL" ) ) {
            mirrors.emplace_back( vp.part_index() );
        } else {
            if( square_dist( origin, mirror_pos ) <= 1 && veh->camera_on ) {
                cam_control = vp.part_index();
            }
        }
    }

    for( int mirror : mirrors ) {
        bool is_camera = veh->part_info( mirror ).has_flag( "CAMERA" );
        if( is_camera && cam_control < 0 ) {
            continue; // Player not at camera control, so cameras don't work
        }

        const tripoint mirror_pos = veh->global_part_pos3( mirror );

        // Determine how far the light has already traveled so mirrors
        // don't cheat the light distance falloff.
        int offsetDistance;
        if( !is_camera ) {
            offsetDistance = rl_dist( origin, mirror_pos );
        } else {
            offsetDistance = 60 - veh->part_info( mirror ).bonus *
                             veh->parts[ mirror ].hp() / veh->part_info( mirror ).durability;
            camera_cache[mirror_pos.x][mirror_pos.y] = LIGHT_TRANSPARENCY_OPEN_AIR;
        }

        // @todo: Factor in the mirror facing and only cast in the
        // directions the player's line of sight reflects to.
        //
        // The naive solution of making the mirrors act like a second player
        // at an offset appears to give reasonable results though.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            camera_cache, transparency_cache, mirror_pos.x, mirror_pos.y, offsetDistance );
    }
}

//Schraudolph's algorithm with John's constants
static inline
float fastexp( float x )
{
    union {
        float f;
        int i;
    } u, v;
    u.i = ( long long )( 6051102 * x + 1056478197 );
    v.i = ( long long )( 1056478197 - 6051102 * x );
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

void map::apply_light_source( const tripoint &p, float luminance )
{
    auto &cache = get_cache( p.z );
    four_quadrants( &lm )[MAPSIZE_X][MAPSIZE_Y] = cache.lm;
    float ( &sm )[MAPSIZE_X][MAPSIZE_Y] = cache.sm;
    float ( &transparency_cache )[MAPSIZE_X][MAPSIZE_Y] = cache.transparency_cache;
    float ( &light_source_buffer )[MAPSIZE_X][MAPSIZE_Y] = cache.light_source_buffer;

    const int x = p.x;
    const int y = p.y;

    if( inbounds( p ) ) {
        const float min_light = std::max( static_cast<float>( LL_LOW ), luminance );
        lm[x][y] = elementwise_max( lm[x][y], min_light );
        sm[x][y] = std::max( sm[x][y], luminance );
    }
    if( luminance <= LL_LOW ) {
        return;
    } else if( luminance <= LL_BRIGHT_ONLY ) {
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
    bool north = ( y != 0 && light_source_buffer[x][y - 1] < luminance );
    bool south = ( y != peer_inbounds && light_source_buffer[x][y + 1] < luminance );
    bool east = ( x != peer_inbounds && light_source_buffer[x + 1][y] < luminance );
    bool west = ( x != 0 && light_source_buffer[x - 1][y] < luminance );

    if( north ) {
        castLight < 1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < -1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    }

    if( east ) {
        castLight < 0, -1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < 0, -1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    }

    if( south ) {
        castLight<1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < -1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    }

    if( west ) {
        castLight<0, 1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < 0, 1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    }
}

void map::apply_directional_light( const tripoint &p, int direction, float luminance )
{
    const int x = p.x;
    const int y = p.y;

    auto &cache = get_cache( p.z );
    four_quadrants( &lm )[MAPSIZE_X][MAPSIZE_Y] = cache.lm;
    float ( &transparency_cache )[MAPSIZE_X][MAPSIZE_Y] = cache.transparency_cache;

    if( direction == 90 ) {
        castLight < 1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < -1, 0, 0, -1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    } else if( direction == 0 ) {
        castLight < 0, -1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < 0, -1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    } else if( direction == 270 ) {
        castLight<1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < -1, 0, 0, 1, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    } else if( direction == 180 ) {
        castLight<0, 1, 1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency>(
                      lm, transparency_cache, x, y, 0, luminance );
        castLight < 0, 1, -1, 0, float, four_quadrants, light_calc, light_check,
                  update_light_quadrants, accumulate_transparency > (
                      lm, transparency_cache, x, y, 0, luminance );
    }
}

void map::apply_light_arc( const tripoint &p, int angle, float luminance, int wideangle )
{
    if( luminance <= LIGHT_SOURCE_LOCAL ) {
        return;
    }

    bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y] {};

    apply_light_source( p, LIGHT_SOURCE_LOCAL );

    // Normalize (should work with negative values too)
    const double wangle = wideangle / 2.0;

    int nangle = angle % 360;

    tripoint end;
    double rad = PI * static_cast<double>( nangle ) / 180;
    int range = LIGHT_RANGE( luminance );
    calc_ray_end( nangle, range, p, end );
    apply_light_ray( lit, p, end, luminance );

    tripoint test;
    calc_ray_end( wangle + nangle, range, p, test );

    const float wdist = hypot( end.x - test.x, end.y - test.y );
    if( wdist <= 0.5 ) {
        return;
    }

    // attempt to determine beam density required to cover all squares
    const double wstep = ( wangle / ( wdist * SQRT_2 ) );

    for( double ao = wstep; ao <= wangle; ao += wstep ) {
        if( trigdist ) {
            double fdist = ( ao * HALFPI ) / wangle;
            double orad = ( PI * ao / 180.0 );
            end.x = int( p.x + ( static_cast<double>( range ) - fdist * 2.0 ) * cos( rad + orad ) );
            end.y = int( p.y + ( static_cast<double>( range ) - fdist * 2.0 ) * sin( rad + orad ) );
            apply_light_ray( lit, p, end, luminance );

            end.x = int( p.x + ( static_cast<double>( range ) - fdist * 2.0 ) * cos( rad - orad ) );
            end.y = int( p.y + ( static_cast<double>( range ) - fdist * 2.0 ) * sin( rad - orad ) );
            apply_light_ray( lit, p, end, luminance );
        } else {
            calc_ray_end( nangle + ao, range, p, end );
            apply_light_ray( lit, p, end, luminance );
            calc_ray_end( nangle - ao, range, p, end );
            apply_light_ray( lit, p, end, luminance );
        }
    }
}

void map::apply_light_ray( bool lit[LIGHTMAP_CACHE_X][LIGHTMAP_CACHE_Y],
                           const tripoint &s, const tripoint &e, float luminance )
{
    int ax = abs( e.x - s.x ) * 2;
    int ay = abs( e.y - s.y ) * 2;
    int dx = ( s.x < e.x ) ? 1 : -1;
    int dy = ( s.y < e.y ) ? 1 : -1;
    int x = s.x;
    int y = s.y;

    quadrant quad = quadrant_from_x_y( dx, dy );

    // TODO: Invert that z comparison when it's sane
    if( s.z != e.z || ( s.x == e.x && s.y == e.y ) ) {
        return;
    }

    auto &lm = get_cache( s.z ).lm;
    auto &transparency_cache = get_cache( s.z ).transparency_cache;

    float distance = 1.0;
    float transparency = LIGHT_TRANSPARENCY_OPEN_AIR;
    const float scaling_factor = static_cast<float>( rl_dist( s, e ) ) /
                                 static_cast<float>( square_dist( s, e ) );
    // TODO: [lightmap] Pull out the common code here rather than duplication
    if( ax > ay ) {
        int t = ay - ( ax / 2 );
        do {
            if( t >= 0 ) {
                y += dy;
                t -= ax;
            }

            x += dx;
            t += ay;

            // TODO: clamp coordinates to map bounds before this method is called.
            if( generic_inbounds( { x, y }, lightmap_boundaries, lightmap_clearance ) ) {
                float current_transparency = transparency_cache[x][y];
                bool is_opaque = ( current_transparency == LIGHT_TRANSPARENCY_SOLID );
                if( !lit[x][y] ) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[x][y] = true;
                    float lm_val = luminance / ( fastexp( transparency * distance ) * distance );
                    quadrant q = is_opaque ? quad : quadrant::default_;
                    lm[x][y][q] = std::max( lm[x][y][q], lm_val );
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
        } while( !( x == e.x && y == e.y ) );
    } else {
        int t = ax - ( ay / 2 );
        do {
            if( t >= 0 ) {
                x += dx;
                t -= ay;
            }

            y += dy;
            t += ax;

            if( generic_inbounds( { x, y }, lightmap_boundaries, lightmap_clearance ) ) {
                float current_transparency = transparency_cache[x][y];
                bool is_opaque = ( current_transparency == LIGHT_TRANSPARENCY_SOLID );
                if( !lit[x][y] ) {
                    // Multiple rays will pass through the same squares so we need to record that
                    lit[x][y] = true;
                    float lm_val = luminance / ( fastexp( transparency * distance ) * distance );
                    quadrant q = is_opaque ? quad : quadrant::default_;
                    lm[x][y][q] = std::max( lm[x][y][q], lm_val );
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
        } while( !( x == e.x && y == e.y ) );
    }
}
