/**
* For core map functions/definitions only:
* constructors, coordinates, basic I/O, drawing
*
* If your new function doesn't fit in the above, it doesn't belong here.
*/

#include "map.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "active_item_cache.h"
#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_type_traits.h"
#include "character.h"
#include "clzones.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "cuboid_rectangle.h"
#include "current_map.h"
#include "cursesdef.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "lightmap.h"
#include "map_iterator.h"
#include "map_memory.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "omdata.h"
#include "output.h"
#include "overmap_map_data_cache.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "rng.h"
#include "submap.h"
#include "trap.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weighted_list.h"

class weather_generator;

static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

// Map class methods.

map::map( int mapsize, bool zlev ) : my_MAPSIZE( mapsize ), my_HALF_MAPSIZE( mapsize / 2 ),
    zlevels( zlev )
{

    if( zlevels ) {
        grid.resize( static_cast<size_t>( my_MAPSIZE ) * my_MAPSIZE * OVERMAP_LAYERS, nullptr );
    } else {
        grid.resize( static_cast<size_t>( my_MAPSIZE ) * my_MAPSIZE, nullptr );
    }

    for( auto &ptr : pathfinding_caches ) {
        ptr = std::make_unique<pathfinding_cache>();
    }

    dbg( D_INFO ) << "map::map(): my_MAPSIZE: " << my_MAPSIZE << " z-levels enabled:" << zlevels;
    traplocs.resize( trap::count() );
}

map::~map()
{
    if( ( _main_requires_cleanup && !_main_cleanup_override ) ||
        ( _main_cleanup_override && *_main_cleanup_override ) ) {
        map &bubble_map = reality_bubble();

        bubble_map.reset_vehicles_sm_pos();
        bubble_map.rebuild_vehicle_level_caches();
        g->load_npcs();
    }
}
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
map &map::operator=( map && ) = default;

static submap null_submap;

const_maptile map::maptile_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return const_maptile( &null_submap, point_sm_ms::zero );
    }

    return maptile_at_internal( p );
}

maptile map::maptile_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        return maptile( &null_submap, point_sm_ms::zero );
    }

    return maptile_at_internal( p );
}

const_maptile map::maptile_at_internal( const tripoint_bub_ms &p ) const
{
    point_sm_ms l;
    const submap *const sm = get_submap_at( p, l );

    return const_maptile( sm, l );
}

maptile map::maptile_at_internal( const tripoint_bub_ms &p )
{
    point_sm_ms l;
    submap *const sm = get_submap_at( p, l );

    return maptile( sm, l );
}

uint8_t map::get_known_connections( const tripoint_bub_ms &p,
                                    const std::bitset<NUM_TERCONN> &connect_group,
                                    const std::map<tripoint_bub_ms, ter_id> &override ) const
{
    if( connect_group.none() ) {
        return 0;
    }

    const level_cache &ch = access_cache( p.z() );
    uint8_t val = 0;
    std::function<bool( const tripoint_bub_ms & )> is_memorized;
    avatar &player_character = get_avatar();
#ifdef TILES
    if( use_tiles ) {
        is_memorized =
        [&]( const tripoint_bub_ms & q ) {
            return !player_character.get_memorized_tile( get_abs( q ) ).get_ter_id().empty();
        };
    } else {
#endif
        is_memorized =
        [&]( const tripoint_bub_ms & q ) {
            return player_character.get_memorized_tile( get_abs( q ) ).symbol != 0;
        };
#ifdef TILES
    }
#endif

    const bool overridden = override.find( p ) != override.end();
    const bool is_transparent = ch.transparency_cache[p.x()][p.y()] > LIGHT_TRANSPARENCY_SOLID;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms neighbour = p + offsets[i];
        if( !inbounds( neighbour ) ) {
            continue;
        }
        const auto neighbour_override = override.find( neighbour );
        const bool neighbour_overridden = neighbour_override != override.end();
        // if there's some non-memory terrain to show at the neighboring tile
        const bool may_connect = neighbour_overridden ||
                                 get_visibility( ch.visibility_cache[neighbour.x()][neighbour.y()],
                                         get_visibility_variables_cache() ) == visibility_type::CLEAR ||
                                 // or if an actual center tile is transparent or next to a memorized tile
                                 ( !overridden && ( is_transparent || is_memorized( neighbour ) ) );
        if( may_connect ) {
            const ter_t &neighbour_terrain = neighbour_overridden ?
                                             neighbour_override->second.obj() : ter( neighbour ).obj();
            if( neighbour_terrain.in_connect_groups( connect_group ) ) {
                val += 1 << i;
            }
        }
    }

    return val;
}

uint8_t map::get_known_rotates_to( const tripoint_bub_ms &p,
                                   const std::bitset<NUM_TERCONN> &rotate_to_group,
                                   const std::map<tripoint_bub_ms, ter_id> &override ) const
{
    if( rotate_to_group.none() ) {
        return CHAR_MAX;
    }

    uint8_t val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms neighbour = p + offsets[i];
        if( !inbounds( neighbour ) ) {
            continue;
        }
        const auto neighbour_override = override.find( neighbour );
        const bool neighbour_overridden = neighbour_override != override.end();

        const ter_t &neighbour_terrain = neighbour_overridden ?
                                         neighbour_override->second.obj() : ter( neighbour ).obj();
        if( neighbour_terrain.in_connect_groups( rotate_to_group ) ) {
            val += 1 << i;
        }
    }

    return val;
}

uint8_t map::get_known_connections_f( const tripoint_bub_ms &p,
                                      const std::bitset<NUM_TERCONN> &connect_group,
                                      const std::map<tripoint_bub_ms, furn_id> &override ) const
{
    if( connect_group.none() ) {
        return 0;
    }

    const level_cache &ch = access_cache( p.z() );
    uint8_t val = 0;
    std::function<bool( const tripoint_bub_ms & )> is_memorized;
    avatar &player_character = get_avatar();
#ifdef TILES
    if( use_tiles ) {
        is_memorized = [&]( const tripoint_bub_ms & q ) {
            return !player_character.get_memorized_tile( get_abs( q ) ).get_dec_id().empty();
        };
    } else {
#endif
        is_memorized = [&]( const tripoint_bub_ms & q ) {
            return player_character.get_memorized_tile( get_abs( q ) ).symbol != 0;
        };
#ifdef TILES
    }
#endif

    const bool overridden = override.find( p ) != override.end();
    const bool is_transparent = ch.transparency_cache[p.x()][p.y()] > LIGHT_TRANSPARENCY_SOLID;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms pt = p + offsets[i];
        if( !inbounds( pt ) ) {
            continue;
        }
        const auto neighbour_override = override.find( pt );
        const bool neighbour_overridden = neighbour_override != override.end();
        // if there's some non-memory terrain to show at the neighboring tile
        const bool may_connect = neighbour_overridden ||
                                 get_visibility( ch.visibility_cache[pt.x()][pt.y()],
                                         get_visibility_variables_cache() ) ==
                                 visibility_type::CLEAR ||
                                 // or if an actual center tile is transparent or
                                 // next to a memorized tile
                                 ( !overridden && ( is_transparent || is_memorized( pt ) ) );
        if( may_connect ) {
            const furn_t &neighbour_furn = neighbour_overridden ?
                                           neighbour_override->second.obj() : furn( pt ).obj();
            if( neighbour_furn.in_connect_groups( connect_group ) ) {
                val += 1 << i;
            }
        }
    }

    return val;
}

uint8_t map::get_known_rotates_to_f( const tripoint_bub_ms &p,
                                     const std::bitset<NUM_TERCONN> &rotate_to_group,
                                     const std::map<tripoint_bub_ms, ter_id> &override,
                                     const std::map<tripoint_bub_ms, furn_id> &override_f ) const
{
    if( rotate_to_group.none() ) {
        return CHAR_MAX;
    }

    uint8_t val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint_bub_ms pt = p + offsets[i];
        if( !inbounds( pt ) ) {
            continue;
        }
        const auto neighbour_override = override.find( pt );
        const bool neighbour_overridden = neighbour_override != override.end();

        const auto neighbour_override_f = override_f.find( pt );
        const bool neighbour_overridden_f = neighbour_override_f != override_f.end();

        const ter_t &neighbour = neighbour_overridden ?
                                 neighbour_override->second.obj() : ter( pt ).obj();

        const furn_t &neighbour_f = neighbour_overridden_f ?
                                    neighbour_override_f->second.obj() : furn( pt ).obj();

        if( neighbour.in_connect_groups( rotate_to_group ) ||
            neighbour_f.in_connect_groups( rotate_to_group ) ) {
            val += 1 << i;
        }
    }

    return val;
}

static std::optional<char32_t> get_memory_at( const tripoint_bub_ms &p )
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( get_map().get_abs( p ) );
    if( mt.symbol != 0 ) {
        return mt.symbol;
    }
    return std::nullopt;
}

void map::draw( const catacurses::window &w, const tripoint_bub_ms &center )
{
    // We only need to draw anything if we're not in tiles mode.
    if( is_draw_tiles_mode() ) {
        return;
    }

    g->reset_light_level();

    update_visibility_cache( center.z() );
    const visibility_variables &cache = get_visibility_variables_cache();

    const auto &visibility_cache = get_cache_ref( center.z() ).visibility_cache;

    int wnd_h = getmaxy( w );
    int wnd_w = getmaxx( w );
    const tripoint_bub_ms offs = center - tripoint_rel_ms( wnd_w / 2, wnd_h / 2, 0 );

    // Map memory should be at least the size of the view range
    // so that new tiles can be memorized, and at least the size of the terminal
    // since displayed area may be bigger than view range.
    const point_bub_ms min_mm_reg{
        std::min( 0, offs.x() ),
        std::min( 0, offs.y() )
    };
    const point_bub_ms max_mm_reg{
        std::max( MAPSIZE_X, offs.x() + wnd_w ),
        std::max( MAPSIZE_Y, offs.y() + wnd_h )
    };
    avatar &player_character = get_avatar();
    player_character.prepare_map_memory_region(
        get_abs( { min_mm_reg, center.z() } ),
        get_abs( { max_mm_reg, center.z() } ) );

    const auto draw_background = [&]( const tripoint_bub_ms & p ) {
        int sym = ' ';
        nc_color col = c_black;
        if( const std::optional<char32_t> memorized_symbol = get_memory_at( p ) ) {
            sym = *memorized_symbol;
            col = c_brown;
        }
        wputch( w, col, sym );
    };

    const auto draw_vision_effect = [&]( const visibility_type vis ) -> bool {
        int sym = '#';
        nc_color col;
        switch( vis )
        {
            case visibility_type::LIT:
                // can only tell that this square is bright
                col = c_light_gray;
                break;
            case visibility_type::BOOMER:
                col = c_pink;
                break;
            case visibility_type::BOOMER_DARK:
                col = c_magenta;
                break;
            default:
                return false;
        }
        wputch( w, col, sym );
        return true;
    };

    drawsq_params params = drawsq_params().memorize( true );
    for( int wy = 0; wy < wnd_h; wy++ ) {
        for( int wx = 0; wx < wnd_w; wx++ ) {
            wmove( w, point( wx, wy ) );
            const tripoint_bub_ms p = offs + tripoint_rel_ms( wx, wy, 0 );
            if( !inbounds( p ) ) {
                draw_background( p );
                continue;
            }

            const lit_level lighting = visibility_cache[p.x()][p.y()];
            const visibility_type vis = get_visibility( lighting, cache );

            if( draw_vision_effect( vis ) ) {
                continue;
            }

            if( vis == visibility_type::HIDDEN || vis == visibility_type::DARK ) {
                draw_background( p );
                continue;
            }

            const const_maptile curr_maptile = maptile_at_internal( p );
            params
            .low_light( lighting == lit_level::LOW )
            .bright_light( lighting == lit_level::BRIGHT );
            if( draw_maptile( w, p, curr_maptile, params ) ) {
                continue;
            }
            const maptile tile_below = maptile_at_internal( p + tripoint::below );
            draw_from_above( w, p + tripoint::below, tile_below, params );
        }
    }

    // Memorize off-screen tiles
    half_open_rectangle<point_bub_ms> display( offs.xy(), offs.xy() + point( wnd_w, wnd_h ) );
    drawsq_params mm_params = drawsq_params().memorize( true ).output( false );
    for( int y = 0; y < MAPSIZE_Y; y++ ) {
        for( int x = 0; x < MAPSIZE_X; x++ ) {
            const tripoint_bub_ms p( x, y, center.z() );
            if( display.contains( p.xy() ) ) {
                // Have been memorized during display loop
                continue;
            }

            const lit_level lighting = visibility_cache[p.x()][p.y()];
            const visibility_type vis = get_visibility( lighting, cache );

            if( vis != visibility_type::CLEAR ) {
                continue;
            }

            const maptile curr_maptile = maptile_at_internal( p );
            mm_params
            .low_light( lighting == lit_level::LOW )
            .bright_light( lighting == lit_level::BRIGHT );

            draw_maptile( w, p, curr_maptile, mm_params );
        }
    }
}

void map::drawsq( const catacurses::window &w, const tripoint_bub_ms &p,
                  const drawsq_params &params ) const
{
    // If we are in tiles mode, the only thing we want to potentially draw is a highlight
    if( is_draw_tiles_mode() ) {
        if( params.highlight() ) {
            g->draw_highlight( p );
        }
        return;
    }

    if( !inbounds( p ) ) {
        return;
    }

    const tripoint_bub_ms view_center = params.center();
    const int k = p.x() + getmaxx( w ) / 2 - view_center.x();
    const int j = p.y() + getmaxy( w ) / 2 - view_center.y();
    if( k < 0 || k >= getmaxx( w ) || j < 0 || j >= getmaxy( w ) ) {
        return;
    }
    wmove( w, point( k, j ) );

    const const_maptile tile = maptile_at( p );
    if( draw_maptile( w, p, tile, params ) ) {
        return;
    }

    tripoint_bub_ms below( p + tripoint::below );
    const const_maptile tile_below = maptile_at( below );
    draw_from_above( w, below, tile_below, params );
}

// a check to see if the lower floor needs to be rendered in tiles
bool map::dont_draw_lower_floor( const tripoint_bub_ms &p ) const
{
    if( !zlevels || p.z() <= -OVERMAP_DEPTH ) {
        return true;
    } else if( !inbounds( p ) ) {
        return false;
    } else {
        return get_cache( p.z() ).floor_cache[p.x()][p.y()];
    }
}

bool map::draw_maptile( const catacurses::window &w, const tripoint_bub_ms &p,
                        const const_maptile &curr_maptile, const drawsq_params &params ) const
{
    drawsq_params param = params;
    nc_color tercol;
    const ter_t &curr_ter = params.terrain_override().is_null()
                            ? curr_maptile.get_ter_t() : params.terrain_override().obj();
    const furn_t &curr_furn = params.furniture_override().is_null()
                              ? curr_maptile.get_furn_t() : params.furniture_override().obj();
    const trap &curr_trap = curr_maptile.get_trap().obj();
    const field &curr_field = curr_maptile.get_field();
    int sym;
    int memory_sym;
    bool hi = false;
    bool graf = false;
    bool draw_item_sym = false;

    if( curr_ter.has_flag( ter_furn_flag::TFLAG_AUTO_WALL_SYMBOL ) ) {
        memory_sym = sym = determine_wall_corner( p );
        tercol = curr_ter.color();
    } else {
        memory_sym = sym = curr_ter.symbol();
        tercol = curr_ter.color();
    }

    avatar &player_character = get_avatar();
    if( curr_furn.id ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
        if( !( player_character.get_grab_type() == object_type::FURNITURE
               && p == player_character.pos_bub() + player_character.grab_point ) ) {
            memory_sym = sym;
        }
    }
    if( curr_ter.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) &&
        curr_ter.has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) &&
        !player_character.is_underwater() ) {
        param.show_items( false ); // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if( curr_trap.can_see( p, player_character ) ) {
        tercol = curr_trap.color;
        if( curr_trap.sym == '%' ) {
            switch( rng( 1, 5 ) ) {
                case 1:
                    memory_sym = sym = '*';
                    break;
                case 2:
                    memory_sym = sym = '0';
                    break;
                case 3:
                    memory_sym = sym = '8';
                    break;
                case 4:
                    memory_sym = sym = '&';
                    break;
                case 5:
                    memory_sym = sym = '+';
                    break;
            }
        } else {
            memory_sym = sym = curr_trap.sym;
        }
    }
    if( const_cast<map *>( this )->partial_con_at( p ) != nullptr ) {
        tercol = tr_unfinished_construction->color;
        memory_sym = sym = tr_unfinished_construction->sym;
    }
    if( curr_field.field_count() > 0 ) {
        const field_type_id &fid = curr_field.displayed_field_type();
        const field_entry *fe = curr_field.find_field( fid );
        const std::string field_symbol = fid->get_symbol();
        if( field_symbol == "&" || fe == nullptr ) {
            // Do nothing, a '&' indicates invisible fields.
        } else if( field_symbol == "*" ) {
            // A random symbol.
            switch( rng( 1, 5 ) ) {
                case 1:
                    memory_sym = sym = '*';
                    break;
                case 2:
                    memory_sym = sym = '0';
                    break;
                case 3:
                    memory_sym = sym = '8';
                    break;
                case 4:
                    memory_sym = sym = '&';
                    break;
                case 5:
                    memory_sym = sym = '+';
                    break;
            }
        } else {
            // A field symbol '%' indicates the field should not hide
            // items/terrain. When the symbol is not '%' it will
            // hide items (the color is still inverted if there are items,
            // but the tile symbol is not changed).
            // draw_item_sym indicates that the item symbol should be used
            // even if sym is not '.'.
            // As we don't know at this stage if there are any items
            // (that are visible to the player!), we always set the symbol.
            // If there are items and the field does not hide them,
            // the code handling items will override it.
            draw_item_sym = ( field_symbol == "%" );
            // If field display_priority is > 1, and the field is set to hide items,
            //draw the field as it obscures what's under it.
            if( ( field_symbol != "%" && fid.obj().priority > 1 ) || ( field_symbol != "%" &&
                    sym == '.' ) )  {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                memory_sym = sym = static_cast<uint8_t>( field_symbol[0] );
            }
            tercol = fe->color();
        }
    }

    // TODO: change the local variable sym to std::string and use it instead of this hack.
    // Currently this are different variables because terrain/... uses int as symbol type and
    // item now use string. Ideally they should all be strings.
    std::string item_sym;

    // If there are items here, draw those instead
    if( param.show_items() && curr_maptile.get_item_count() > 0 &&
        sees_some_items( p, player_character ) ) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if( sym != '.' && sym != '%' && !draw_item_sym ) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            item_sym = curr_maptile.get_uppermost_item().symbol();
            if( !draw_item_sym ) {
                tercol = curr_maptile.get_uppermost_item().color();
            }
            if( curr_maptile.get_item_count() > 1 ) {
                param.highlight( !param.highlight() );
            }
        }
    }

    int veh_part = 0;
    const vehicle *veh = veh_at_internal( p, veh_part );
    if( veh != nullptr ) {
        const vpart_display vd = veh->get_display_of_tile( veh->part( veh_part ).mount );
        sym = vd.symbol_curses;
        tercol = vd.color;
        item_sym.clear(); // clear the item symbol so `sym` is used instead.

        if( !veh->forward_velocity() && !veh->player_in_control( *this, player_character )
            && !( player_character.get_grab_type() == object_type::VEHICLE
                  && veh->get_points().count( ( player_character.pos_abs() +
                                                player_character.grab_point ) ) ) ) {
            memory_sym = sym;
        }
        if( !vd.carried_furn.empty() ) {
            furn_str_id furn( vd.carried_furn );
            sym = furn->symbol();
            tercol = furn->color();
        }
    }

    if( param.memorize() && memory_cache_ter_is_dirty( p ) ) {
        player_character.memorize_symbol( get_abs( p ), memory_sym );
        memory_cache_ter_set_dirty( p, false );
    }

    // If there's graffiti here, change background color
    if( curr_maptile.has_graffiti() ) {
        graf = true;
    }

    const auto u_vision = player_character.get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = param.bright_light() ? c_white : c_light_green;
    } else if( param.low_light() || u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( param.highlight() ) {
        tercol = invert_color( tercol );
    } else if( hi ) {
        tercol = hilite( tercol );
    } else if( graf ) {
        tercol = red_background( tercol );
    }

    if( item_sym.empty() && sym == ' ' ) {
        if( !zlevels || p.z() <= -OVERMAP_DEPTH || !curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            // Print filler symbol
            tercol = c_black;
        } else {
            // Draw tile underneath this one instead
            return false;
        }
    }

    if( params.output() ) {
        if( item_sym.empty() ) {
            wputch( w, tercol, sym );
        } else {
            wprintz( w, tercol, item_sym );
        }
    }
    return true;
}

void map::draw_from_above( const catacurses::window &w, const tripoint_bub_ms &p,
                           const const_maptile &curr_tile, const drawsq_params &params ) const
{
    static const int AUTO_WALL_PLACEHOLDER = 2; // this should never appear as a real symbol!

    nc_color tercol = c_dark_gray;
    int sym = ' ';

    const ter_t &curr_ter = curr_tile.get_ter_t();
    const furn_t &curr_furn = curr_tile.get_furn_t();
    int part_below;
    const vehicle *veh;
    if( curr_furn.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
    } else if( curr_furn.movecost < 0 ) {
        sym = '.';
        tercol = curr_furn.color();
    } else if( ( veh = veh_at_internal( p, part_below ) ) != nullptr ) {
        const vpart_position vpp( const_cast<vehicle &>( *veh ), part_below );
        const vpart_display vd = veh->get_display_of_tile( vpp.mount_pos(), true, true, true );
        const int roof = veh->roof_at_part( part_below );
        sym = vd.symbol_curses;
        tercol = roof >= 0 || vpp.obstacle_at_part() ? c_light_gray : c_light_gray_cyan;
    } else if( curr_ter.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) ) {
        if( curr_ter.has_flag( ter_furn_flag::TFLAG_AUTO_WALL_SYMBOL ) ) {
            sym = AUTO_WALL_PLACEHOLDER;
        } else if( curr_ter.has_flag( ter_furn_flag::TFLAG_RAMP ) ) {
            sym = '>';
        } else {
            sym = curr_ter.symbol();
        }
        tercol = curr_ter.color();
    } else if( curr_ter.movecost == 0 ) {
        sym = '.';
        tercol = curr_ter.color();
    } else if( !curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
        sym = '.';
        if( curr_ter.color() != c_cyan ) {
            // Need a special case here, it doesn't cyanize well
            tercol = cyan_background( curr_ter.color() );
        } else {
            tercol = c_black_cyan;
        }
    } else {
        sym = curr_ter.symbol();
        tercol = curr_ter.color();
    }

    if( sym == AUTO_WALL_PLACEHOLDER ) {
        sym = determine_wall_corner( p );
    }

    const std::bitset<NUM_VISION_MODES> &u_vision = get_player_character().get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = params.bright_light() ? c_white : c_light_green;
    } else if( params.low_light() || u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( params.highlight() ) {
        tercol = invert_color( tercol );
    }

    if( params.output() ) {
        wputch( w, tercol, sym );
    }
}

std::vector<tripoint_bub_ms> map::get_dir_circle( const tripoint_bub_ms &f,
        const tripoint_bub_ms &t ) const
{
    std::vector<tripoint_bub_ms> circle;
    circle.resize( 8 );

    // The line below can be crazy expensive - we only take the FIRST point of it
    const std::vector<tripoint_bub_ms> line = line_to( f, t, 0, 0 );
    const std::vector<tripoint_bub_ms> spiral = closest_points_first( f, 1 );
    const std::vector<int> pos_index{ 1, 2, 4, 6, 8, 7, 5, 3 };

    //  All possible constellations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    size_t pos_offset = 0;
    for( size_t i = 1; i < spiral.size(); i++ ) {
        if( spiral[i] == line[0] ) {
            pos_offset = i - 1;
            break;
        }
    }

    for( size_t i = 1; i < spiral.size(); i++ ) {
        if( pos_offset >= pos_index.size() ) {
            pos_offset = 0;
        }

        circle[pos_index[pos_offset++] - 1] = spiral[i];
    }

    return circle;
}

void map::save()
{
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            if( zlevels ) {
                for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                    saven( { gridx, gridy, gridz } );
                }
            } else {
                saven( { gridx, gridy, abs_sub.z() } );
            }
        }
    }
}

void map::load( const tripoint_abs_sm &w, const bool update_vehicle,
                const bool pump_events )
{
    map &main_map = get_map();
    // It used to be unsafe to load a map that overlaps with the primary map;
    // Show an info line in tests to help track new errors
    if( test_mode && this != &main_map && main_map.inbounds( project_to<coords::ms>( w ) ) ) {
        DebugLog( D_INFO, DC_ALL )
                << "loading non-main map at " << w.to_string()
                << " which overlaps with main map (abs_sub = " << main_map.abs_sub.to_string() << ")";
    }
    for( auto &traps : traplocs ) {
        traps.clear();
    }
    field_furn_locs.clear();
    field_ter_locs.clear();
    submaps_with_active_items.clear();
    submaps_with_active_items_dirty.clear();
    set_abs_sub( w );
    clear_vehicle_level_caches();
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            loadn( { gridx, gridy }, update_vehicle );
            if( pump_events ) {
                inp_mngr.pump_events();
            }
        }
    }
    rebuild_vehicle_level_caches();

    if( !explosion_handler::explosion_processing_active() ) {
        // actualize after loading all submaps to prevent errors
        // with entities at the edges
        for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
            for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
                const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    actualize( { gridx, gridy, gridz } );
                    if( pump_events ) {
                        inp_mngr.pump_events();
                    }
                }
            }
        }
    }

    // tinymap derives from map, and a remote load would otherwise wipe the bubble's locks.
    reconcile_loaded_items( this == &get_map()
                            ? reconcile_scope::full_rebuild
                            : reconcile_scope::additive );
}

template<int SIZE, int MULTIPLIER>
void shift_bitset_cache( std::bitset<SIZE *SIZE> &cache, const point_rel_sm &s )
{
    // sx shifts by MULTIPLIER rows, sy shifts by MULTIPLIER columns.
    int shift_amount = s.x() * MULTIPLIER + s.y() * SIZE * MULTIPLIER;
    if( shift_amount > 0 ) {
        cache >>= static_cast<size_t>( shift_amount );
    } else if( shift_amount < 0 ) {
        cache <<= static_cast<size_t>( -shift_amount );
    }
    // Shifting in the y direction shifted in 0 values, no no additional clearing is necessary, but
    // a shift in the x direction makes values "wrap" to the next row, and they need to be zeroed.
    if( s.x() == 0 ) {
        return;
    }
    const size_t x_offset = s.x() > 0 ? SIZE - MULTIPLIER : 0;
    for( size_t y = 0; y < SIZE; ++y ) {
        size_t y_offset = y * SIZE;
        for( size_t x = 0; x < MULTIPLIER; ++x ) {
            cache.reset( y_offset + x_offset + x );
        }
    }
}

template void
shift_bitset_cache<MAPSIZE_X, SEEX>( std::bitset<MAPSIZE_X *MAPSIZE_X> &cache,
                                     const point_rel_sm &s );
template void
shift_bitset_cache<MAPSIZE, 1>( std::bitset<MAPSIZE *MAPSIZE> &cache, const point_rel_sm &s );

void map::shift( const point_rel_sm &sp )
{
    if( !zlevels ) {
        debugmsg( "map::shift called from map that doesn't support Z levels" );
        return;
    }

    // Special case of 0-shift; refresh the map
    if( sp == point_rel_sm::zero ) {
        return; // Skip this?
    }

    if( std::abs( sp.x() ) > 1 || std::abs( sp.y() ) > 1 ) {
        debugmsg( "map::shift called with a shift of more than one submap" );
        return;
    }

    const tripoint_abs_sm abs = get_abs_sub();
    std::vector<tripoint_rel_sm> loaded_grids;

    const int zmin = -OVERMAP_DEPTH;
    const int zmax = OVERMAP_HEIGHT;

    const int x_start = sp.x() >= 0 ? 0 : my_MAPSIZE - 1;
    const int x_stop = sp.x() >= 0 ? my_MAPSIZE : -1;
    const int x_step = sp.x() >= 0 ? 1 : -1;
    const int y_start = sp.y() >= 0 ? 0 : my_MAPSIZE - 1;
    const int y_stop = sp.y() >= 0 ? my_MAPSIZE : -1;
    const int y_step = sp.y() >= 0 ? 1 : -1;

    // Have to run on_unload before changing the abs_sub for the map.
    // TODO: can probably skip a bunch of iteration here.
    for( int gridx = x_start; gridx != x_stop; gridx += x_step ) {
        for( int gridy = y_start; gridy != y_stop; gridy += y_step ) {
            // The sp.x() and sp.y() check here establishes that we are moving
            // in the x or y direction respectively. If we are not moving in the x direction,
            // we don't want to call on_unload on grid points where gridx == x_start.
            if( ( sp.x() && gridx == x_start ) || ( sp.y() && gridy == y_start ) ) {
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    const tripoint_rel_sm grid( gridx, gridy, gridz );
                    on_unload( grid );
                }
            }
        }
    }
    set_abs_sub( abs + sp );

    g->shift_destination_preview( { -sp.x() * SEEX, -sp.y() * SEEY } );

    shift_traps( sp );

    vehicle *remoteveh = g->remoteveh();

    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        level_cache *cache = get_cache_lazy( gridz );
        if( !cache ) {
            continue;
        }
        for( vehicle *veh : cache->vehicle_list ) {
            veh->zones_dirty = true;
        }
    }

    // Shift the map sx submaps to the right and sy submaps down.
    // sx and sy should never be bigger than +/-1.
    // absx and absy are our position in the world, for saving/loading purposes.
    clear_vehicle_level_caches();

    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        // Clear vehicle list and rebuild after shift
        // mlangsdorf 2020 - this is kind of insane, building the cache is not free, why are
        // we doing this?
        clear_vehicle_list( gridz );
        level_cache *cache = get_cache_lazy( gridz );
        if( cache ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( cache->map_memory_cache_dec, sp );
            shift_bitset_cache<MAPSIZE_X, SEEX>( cache->map_memory_cache_ter, sp );
            cache->map_memory_sweep_pending = true;
            shift_bitset_cache<MAPSIZE, 1>( cache->field_cache, sp );
        }
    }

    for( int gridx = x_start; gridx != x_stop; gridx += x_step ) {
        for( int gridy = y_start; gridy != y_stop; gridy += y_step ) {
            if( gridx + sp.x() != x_stop && gridy + sp.y() != y_stop ) {
                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    const tripoint_rel_sm grid( gridx, gridy, gridz );

                    copy_grid( grid, grid + sp );
                    submap *const cur_submap = get_submap_at_grid( grid );
                    if( cur_submap == nullptr ) {
                        debugmsg( "Tried to update vehicle list at %s but the submap is not loaded", grid.to_string() );
                        continue;
                    }
                    update_vehicle_list( cur_submap, gridz );
                }
            } else {
                loadn( { gridx, gridy }, true );

                for( int gridz = zmin; gridz <= zmax; gridz++ ) {
                    loaded_grids.emplace_back( gridx, gridy, gridz );
                }
            }
        }
    }

    rebuild_vehicle_level_caches();

    g->setremoteveh( remoteveh );

    if( !support_cache_dirty.empty() ) {
        std::set<tripoint_bub_ms> old_cache = std::move( support_cache_dirty );
        support_cache_dirty.clear();
        for( const tripoint_bub_ms &pt : old_cache ) {
            support_cache_dirty.insert( pt + point( -sp.x() * SEEX, -sp.y() * SEEY ) );
        }
    }
    // actualize after loading all submaps to prevent errors
    // with entities at the edges
    for( tripoint_rel_sm loaded_grid : loaded_grids ) {
        actualize( loaded_grid );
    }
    if( !loaded_grids.empty() ) {
        // A shift walks the bubble it now has, so the crafts it scrolled away from are
        // not seen and must keep the claims their leases still cover.
        reconcile_loaded_items( reconcile_scope::additive );
    }
}

void map::vertical_shift( const int newz )
{
    if( !zlevels ) {
        debugmsg( "Called map::vertical_shift in a non-z-level world" );
        return;
    }

    if( newz < -OVERMAP_DEPTH || newz > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  newz, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return;
    }

    tripoint_abs_sm trp = get_abs_sub();
    set_abs_sub( tripoint_abs_sm( trp.xy(), newz ) );

    // TODO: Remove the function when it's safe
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::saven( const tripoint_bub_sm &grid )
{
    dbg( D_INFO ) << "map::saven(worldx[" << abs_sub.x() << "], worldy[" << abs_sub.y()
                  << "], worldz[" << abs_sub.z()
                  << "], gridx[" << grid.x() << "], gridy[" << grid.y() << "], gridz[" << grid.z() << "])";
    const int gridn = get_nonant( rebase_rel( grid ) );
    submap *submap_to_save = getsubmap( gridn );
    if( submap_to_save == nullptr ) {
        debugmsg( "Tried to save submap node (%d) but it's not loaded", gridn );
        return;
    }

    tripoint_rel_sm relative_origin = rebase_rel( grid );
    on_unload( relative_origin );
    const tripoint_abs_sm abs = abs_sub.xy() + relative_origin;

    if( !zlevels && grid.z() != abs_sub.z() ) {
        debugmsg( "Tried to save submap (%d,%d,%d) as (%d,%d,%d), which isn't supported in non-z-level builds",
                  abs.x(), abs.y(), abs_sub.z(), abs.x(), abs.y(), grid.z() );
    }

    dbg( D_INFO ) << "map::saven abs: " << abs
                  << "  gridn: " << gridn;
    submap_to_save->last_touched = calendar::turn;
    MAPBUFFER.add_submap( abs, submap_to_save );
}

// Optimized mapgen function that only works properly for very simple overmap types
// Does not create or require a temporary map and does its own saving.
// Note that it assumes the map doesn't exist: it's an error to call this when it does.
bool generate_uniform( const tripoint_abs_sm &p, const ter_str_id &ter )
{
    if( MAPBUFFER.submap_exists( p ) ) {
        return false;
    }

    std::unique_ptr<submap> sm = std::make_unique<submap>();
    sm->set_all_ter( ter, true );
    sm->last_touched = calendar::turn;
    return MAPBUFFER.add_submap( p, sm );
}

bool generate_uniform_omt( const tripoint_abs_sm &p, const oter_id &terrain_type )
{
    dbg( D_INFO ) << "generate_uniform p: " << p
                  << "  terrain_type: " << terrain_type.id().str();

    const std::optional<ter_str_id> ter = terrain_type->get_uniform_terrain();

    if( !ter ) {
        return false;
    }

    bool ret = true;
    for( int xd = 0; xd <= 1; xd++ ) {
        for( int yd = 0; yd <= 1; yd++ ) {
            ret &= generate_uniform( p + point_rel_sm( xd, yd ), *ter );
        }
    }
    return ret;
}

void map::loadn( const point_bub_sm &grid, bool update_vehicles )
{
    dbg( D_INFO ) << "map::loadn(game[" << g.get() << "], worldx[" << abs_sub.x()
                  << "], worldy[" << abs_sub.y() << "], grid " << grid << ")";

    const tripoint_abs_sm grid_abs_sub = abs_sub + rebase_rel( grid );
    const tripoint_abs_omt grid_abs_omt = project_to<coords::omt>( grid_abs_sub );
    // Get the base submap "grid" is an offset from.
    const tripoint_abs_sm grid_sm_base = project_to<coords::sm>( grid_abs_omt );
    bool map_incomplete = false;

    map &bubble_map = reality_bubble();

    bool const main_inbounds =
        this != &bubble_map && bubble_map.inbounds( project_to<coords::ms>( grid_abs_sub ) );

    // It might be possible to just check the (0, 0) submap as we should never have
    // a case where only one submap is missing from an OMT level.
    for( int gridx = 0; !map_incomplete && gridx <= 1; gridx++ ) {
        for( int gridy = 0; !map_incomplete && gridy <= 1; gridy++ ) {
            for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                const tripoint grid_pos( gridx, gridy, gridz );
                if( !MAPBUFFER.submap_exists( grid_sm_base.xy() + grid_pos ) ) {
                    map_incomplete = true;
                    break;
                }
            }
        }
    }

    if( map_incomplete ) {
        smallmap tmp_map;
        swap_map swap( *tmp_map.cast_to_map() );
        tmp_map.main_cleanup_override( false );
        tmp_map.generate( grid_abs_omt, calendar::turn, true );
        _main_requires_cleanup |= main_inbounds && tmp_map.is_main_cleanup_queued();

        for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
            const tripoint_abs_sm pos = {grid_sm_base.xy(), gridz };
            if( !MAPBUFFER.submap_exists( pos ) ) {
                dbg( D_ERROR ) << "failed to generate a submap at " << pos;
                debugmsg( "failed to generate a submap at %s", pos.to_string() );
                return;
            }
        }
    }

    const int start_z = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int stop_z = zlevels ? OVERMAP_HEIGHT : abs_sub.z();

    submap *tmpsub;

    for( int z = start_z; z <= stop_z; z++ ) {
        const tripoint_abs_sm pos = { grid_abs_sub.xy(), z };
        // New submap changes the content of the map and all caches must be recalculated
        set_transparency_cache_dirty( z );
        set_seen_cache_dirty( z );
        set_outside_cache_dirty( z );
        set_floor_cache_dirty( z );
        set_pathfinding_cache_dirty( z );
        tmpsub = MAPBUFFER.lookup_submap( pos );
        setsubmap( get_nonant( tripoint_rel_sm{ grid.x(), grid.y(), z} ), tmpsub );
        if( !tmpsub->active_items.empty() ) {
            submaps_with_active_items_dirty.emplace( pos );
        }
        if( tmpsub->field_count > 0 ) {
            get_cache( z ).field_cache.set( grid.x() + grid.y() * MAPSIZE );
        }

        // Destroy bugged no-part vehicles
        auto &veh_vec = tmpsub->vehicles;
        for( auto iter = veh_vec.begin(); iter != veh_vec.end(); ) {
            vehicle *veh = iter->get();
            if( veh->part_count() > 0 ) {
                // Always fix submap coordinates for easier Z-level-related operations
                veh->sm_pos = abs_sub.xy() + tripoint_rel_sm{ rebase_rel( grid ), z};
                iter++;
                if( main_inbounds ) {
                    _main_requires_cleanup = true;
                }
            } else {
                if( veh->tracking_on ) {
                    overmap_buffer.remove_vehicle( veh );
                }
                dirty_vehicle_list.erase( veh );
                iter = veh_vec.erase( iter );
            }
        }

        // Update vehicle data
        if( update_vehicles ) {
            level_cache &map_cache = get_cache( z );
            for( const auto &veh : tmpsub->vehicles ) {
                // Only add if not tracking already.
                if( map_cache.vehicle_list.find( veh.get() ) == map_cache.vehicle_list.end() ) {
                    map_cache.vehicle_list.insert( veh.get() );
                    if( !veh->loot_zones.empty() ) {
                        map_cache.zone_vehicles.insert( veh.get() );
                    }
                }
            }
        }

        if( zlevels ) {
            add_tree_tops( { grid.x(), grid.y(), z } );
        }
        // When a submap is loaded into the reality bubble, apply
        // temperature-based phase changes
        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const tripoint_abs_sm pos_sm = { grid_abs_sub.xy(), z };
        const tripoint_abs_ms sm_abs_ms = project_to<coords::ms>( pos_sm );
        const tripoint_bub_ms sm_origin = get_bub( sm_abs_ms );
        for( int sx = 0; sx < SEEX; ++sx ) {
            for( int sy = 0; sy < SEEX; ++sy ) {
                temp_based_phase_change_at( sm_origin + point( sx, sy ), wgen );
            }
        }
    }
}

void map::actualize( const tripoint_rel_sm &grid )
{
    submap *const tmpsub = get_submap_at_grid( grid );
    if( tmpsub == nullptr ) {
        debugmsg( "Actualize called on null submap (%d,%d,%d)", grid.x(), grid.y(), grid.z() );
        return;
    }

    for( const std::unique_ptr<vehicle> &veh : tmpsub->vehicles ) {
        // spill out items too large, MIGRATION pockets etc from vehicle parts
        for( const vpart_reference &vp : veh->get_all_parts() ) {
            const item &base_const = vp.part().get_base();
            const_cast<item &>( base_const ).overflow( *this, vp.pos_bub( *this ) );
        }
        veh->refresh( );
    }

    const time_duration time_since_last_actualize = calendar::turn - tmpsub->last_touched;
    const bool do_funnels = grid.z() >= 0;

    // check spoiled stuff, and fill up funnels while we're at it
    process_items_in_vehicles( *tmpsub );
    process_items_in_submap( *tmpsub, grid );
    explosion_handler::process_explosions();
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const tripoint_bub_ms pnt =  rebase_bub( coords::project_to<coords::ms>( grid ) + point( x, y ) );
            const point_sm_ms p( x, y );
            const furn_t &furn = *this->furn( pnt );
            const ter_t &terr = *this->ter( pnt );
            if( !furn.emissions.empty() ) {
                field_furn_locs.push_back( pnt );
            }
            if( !terr.emissions.empty() ) {
                field_ter_locs.push_back( pnt );
            }

            const trap_id trap_here = tmpsub->get_trap( p );
            if( trap_here != tr_null ) {
                traplocs[trap_here.to_i()].push_back( pnt );
            }
            const ter_t &ter = tmpsub->get_ter( p ).obj();
            if( ter.trap != tr_null ) {
                traplocs[ter.trap.to_i()].push_back( pnt );
            }

            if( do_funnels ) {
                fill_funnels( pnt, tmpsub->last_touched );
            }

            grow_plant( pnt );

            restock_fruits( pnt, time_since_last_actualize );

            produce_sap( pnt, time_since_last_actualize );

            rad_scorch( pnt, time_since_last_actualize );

            decay_cosmetic_fields( pnt, time_since_last_actualize );
        }
    }

    // the last time we touched the submap, is right now.
    tmpsub->last_touched = calendar::turn;
}

void map::on_unload( const tripoint_rel_sm &loc )
{
    // Look up the existing map data summary entry, if it's already dynamic we don't
    // need to check all the submaps, just update it.
    tripoint_abs_sm submap_abs_origin = get_abs_sub().xy() + loc;
    tripoint_abs_omt omt_abs_origin;
    point_omt_sm offset_within_omt;
    std::tie( omt_abs_origin, offset_within_omt ) = project_remain<coords::omt>( submap_abs_origin );
    // uuuugh this should be const?
    std::shared_ptr<map_data_summary> const_summary = overmap_buffer.get_omt_summary( omt_abs_origin );
    bool adjusted = !const_summary->placeholder;


    if( !adjusted ) {
        tripoint_rel_omt omt_origin = project_to<coords::omt>( loc );
        tripoint_rel_sm sm_origin = project_to<coords::sm>( omt_origin );
        std::vector<point_rel_sm> offsets;
        offsets.reserve( 4 );
        offsets.push_back( point_rel_sm::zero );
        offsets.push_back( point_rel_sm::south );
        offsets.push_back( point_rel_sm::east );
        offsets.push_back( point_rel_sm::south_east );
        for( const point_rel_sm &offset : offsets ) {
            tripoint_rel_sm submap_offset = sm_origin + offset;
            // Is there a better option than this manual check?
            if( submap_offset.x() < 0 || submap_offset.x() >= my_MAPSIZE ||
                submap_offset.y() < 0 || submap_offset.y() >= my_MAPSIZE ||
                submap_offset.z() < -OVERMAP_DEPTH || submap_offset.z() > OVERMAP_HEIGHT ) {
                continue;
            }
            // Handle null submap ptr, it can definitely happen and we just ignore it.
            if( submap *sm = get_submap_at_grid( submap_offset ); sm && sm->player_adjusted_map ) {
                adjusted = true;
                break;
            }

        }
    }
    // Update just the quadrant of the map_data_summary matching the current submap.
    if( adjusted ) {
        // Just the passable data for now, in future might want to copy the whole thing.
        // TODO: The interface here should be "retrieve writable summary" that handles returning a
        // new summary if the existing one is a placeholder.
        std::shared_ptr<map_data_summary> new_summary = std::make_shared<map_data_summary>
                ( const_summary->passable );
        tripoint_bub_ms submap_origin = get_bub( project_to<coords::ms>( submap_abs_origin ) );
        point summary_origin = offset_within_omt.raw();
        for( point cursor; cursor.y < 12; cursor.y++ ) {
            int y_component = summary_origin.y * 12 + cursor.y;
            for( cursor.x = 0; cursor.x < 12; cursor.x++ ) {
                int x_component = summary_origin.x * 12 + cursor.x;
                bool passable_value = passable( submap_origin + cursor );
                new_summary->passable.set( ( y_component * 24 ) + x_component, passable_value );
            }
        }
        // TODO: minimize copying?
        overmap_buffer.set_passable( omt_abs_origin, new_summary->passable );
    }
}

void map::copy_grid( const tripoint_rel_sm &to, const tripoint_rel_sm &from )
{
    submap *smap = get_submap_at_grid( from );
    if( smap == nullptr ) {
        debugmsg( "Tried to copy grid from (%d,%d,%d) but the submap is not loaded", from.x(), from.y(),
                  from.z() );
        return;
    }
    setsubmap( get_nonant( to ), smap );
    for( auto &it : smap->vehicles ) {
        it->sm_pos = abs_sub.xy() + to;
    }
}

bool map::inbounds( const tripoint_abs_ms &p ) const
{
    return inbounds( get_bub( p ) );
}

bool map::inbounds( const tripoint_bub_ms &p ) const
{
    return p.x() >= 0 && p.x() < my_MAPSIZE * SEEX &&
           p.y() >= 0 && p.y() < my_MAPSIZE * SEEY &&
           p.z() >= -OVERMAP_DEPTH && p.z() <= OVERMAP_HEIGHT;
    // && ( zlevels || p.z == get_abs_sub().z() );
    // Cannot actually let inbounds check the bounds for maps not supporting Z levels, as
    // tests explicitly expect other Z level coordinates to return t_null when read
    // and write terrain to the Z level of the map rather than fail or do nothing.
}

bool map::inbounds( const tripoint_abs_omt &p ) const
{
    const tripoint_abs_omt map_origin = project_to<coords::omt>( abs_sub );
    return  p.z() >= -OVERMAP_DEPTH && p.z() <= OVERMAP_HEIGHT &&
            ( zlevels || p.z() == get_abs_sub().z() ) &&
            p.x() >= map_origin.x() &&
            p.y() >= map_origin.y() &&
            p.x() <= map_origin.x() + my_HALF_MAPSIZE &&
            p.y() <= map_origin.y() + my_HALF_MAPSIZE;
}

tripoint_range<tripoint_omt_ms> tinymap::points_on_zlevel() const
{

    const tripoint_range <tripoint_bub_ms> temp = map::points_on_zlevel();
    return tripoint_range <tripoint_omt_ms>( rebase_omt( temp.min() ), rebase_omt( temp.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_on_zlevel( int z ) const
{
    const tripoint_range<tripoint_bub_ms> temp = map::points_on_zlevel( z );
    return tripoint_range <tripoint_omt_ms>( rebase_omt( temp.min() ), rebase_omt( temp.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_in_rectangle(
    const tripoint_omt_ms &from, const tripoint_omt_ms &to ) const
{
    const tripoint_range<tripoint_bub_ms> preliminary_result = map::points_in_rectangle( rebase_bub(
                from ),
            rebase_bub( to ) );
    return tripoint_range<tripoint_omt_ms>( rebase_omt( preliminary_result.min() ),
                                            rebase_omt( preliminary_result.max() ) );
}

tripoint_range<tripoint_omt_ms> tinymap::points_in_radius(
    const tripoint_omt_ms &center, size_t radius, size_t radiusz ) const
{
    const tripoint_range<tripoint_bub_ms> preliminary_result = map::points_in_radius( rebase_bub(
                center ), radius,
            radiusz );
    return tripoint_range<tripoint_omt_ms>( rebase_omt( preliminary_result.min() ),
                                            rebase_omt( preliminary_result.max() ) );
}

maptile tinymap::maptile_at( const tripoint_omt_ms &p )
{
    return map::maptile_at( rebase_bub( p ) );
}


// set up a map just long enough scribble on it
// this tinymap should never, ever get saved
fake_map::fake_map( const ter_id &ter_type )
{
    const tripoint_abs_sm tripoint_below_zero( 0, 0, fake_map_z );

    set_abs_sub( tripoint_below_zero );
    for( int gridx = 0; gridx < get_my_MAPSIZE(); gridx++ ) {
        for( int gridy = 0; gridy < get_my_MAPSIZE(); gridy++ ) {
            std::unique_ptr<submap> sm = std::make_unique<submap>();

            sm->set_all_ter( ter_type );
            sm->set_all_furn( furn_str_id::NULL_ID() );
            sm->set_all_traps( tr_null );

            setsubmap( get_nonant( tripoint_rel_sm{ gridx, gridy, fake_map_z } ), sm.get() );

            temp_submaps_.emplace_back( std::move( sm ) );
        }
    }
}

fake_map::~fake_map() = default;

small_fake_map::small_fake_map( const ter_id &ter_type )
{
    set_abs_sub( tripoint_abs_sm::zero );

    for( int gridx = 0; gridx < get_my_MAPSIZE(); gridx++ ) {
        for( int gridy = 0; gridy < get_my_MAPSIZE(); gridy++ ) {
            for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                std::unique_ptr<submap> sm = std::make_unique<submap>();

                sm->set_all_ter( ter_type );
                sm->set_all_furn( furn_str_id::NULL_ID() );
                sm->set_all_traps( tr_null );

                setsubmap( get_nonant( tripoint_rel_sm{ gridx, gridy, gridz } ), sm.get() );

                temp_submaps_.emplace_back( std::move( sm ) );
            }
        }
    }
}

small_fake_map::~small_fake_map() = default;

//////////
///// coordinate helpers

tripoint_abs_ms map::get_abs( const tripoint_bub_ms &p ) const
{
    return tripoint_abs_ms{ p.x() + abs_ms.x(), p.y() + abs_ms.y(), p.z() };
}

tripoint_bub_ms map::get_bub( const tripoint_abs_ms &p ) const
{
    return tripoint_bub_ms { p.x() - abs_ms.x(), p.y() - abs_ms.y(), p.z()};
}

void map::set_abs_sub( const tripoint_abs_sm &p )
{
    abs_sub = p;
    abs_ms = point_abs_ms{ project_to<coords::ms>( abs_sub.xy() ) };
}

tripoint_abs_sm map::get_abs_sub() const
{
    return abs_sub;
}

template<typename Array>
static cata::copy_const<Array, submap> *getsubmap_helper(
    const Array &grid, const size_t grididx )
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d. Grid size: %d", grididx, grid.size() );
        return nullptr;
    }
    return grid[grididx];
}

submap *map::getsubmap( const size_t grididx )
{
    return getsubmap_helper( grid, grididx );
}

const submap *map::getsubmap( const size_t grididx ) const
{
    return getsubmap_helper( grid, grididx );
}

submap *map::unsafe_get_submap_at( const tripoint_bub_ms &p )
{
    cata_assert( inbounds( p ) );
    return get_submap_at_grid( tripoint_rel_sm{ p.x() / SEEX, p.y() / SEEY, p.z() } );
}

const submap *map::unsafe_get_submap_at( const tripoint_bub_ms &p ) const
{
    cata_assert( inbounds( p ) );
    return get_submap_at_grid( { p.x() / SEEX, p.y() / SEEY, p.z() } );
}

void map::setsubmap( const size_t grididx, submap *const smap )
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d", grididx );
        return;
    } else if( smap == nullptr ) {
        debugmsg( "Tried to set NULL submap pointer at index %d", grididx );
        return;
    }
    grid[grididx] = smap;
}

submap *map::get_submap_at( const tripoint_bub_ms &p )
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position %s", p.to_string() );
        return nullptr;
    }
    return unsafe_get_submap_at( p );
}

const submap *map::get_submap_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position %s", p.to_string() );
        return nullptr;
    }
    return unsafe_get_submap_at( p );
}

submap *map::get_submap_at_grid( const tripoint_rel_sm &gridp )
{
    return getsubmap( get_nonant( gridp ) );
}

const submap *map::get_submap_at_grid( const tripoint_rel_sm &gridp ) const
{
    return getsubmap( get_nonant( gridp ) );
}

size_t map::get_nonant( const tripoint_rel_sm &gridp ) const
{
    if( gridp.x() < 0 || gridp.x() >= my_MAPSIZE ||
        gridp.y() < 0 || gridp.y() >= my_MAPSIZE ||
        gridp.z() < -OVERMAP_DEPTH || gridp.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to access invalid map position at grid (%d,%d,%d)", gridp.x(), gridp.y(),
                  gridp.z() );
        return 0;
    }

    if( zlevels ) {
        const int indexz = gridp.z() + OVERMAP_DEPTH; // Can't be lower than 0
        return indexz + ( gridp.x() + gridp.y() * my_MAPSIZE ) * OVERMAP_LAYERS;
    } else {
        return gridp.x() + gridp.y() * my_MAPSIZE;
    }
}

void map::draw_line_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                         const int z,
                         bool avoid_creatures )
{
    draw_line( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->ter_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_line_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                          const int z,
                          bool avoid_creatures )
{
    draw_line( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->furn_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_fill_background( const ter_id &type )
{
    // Need to explicitly set caches dirty - set_ter would do it before
    set_transparency_cache_dirty( abs_sub.z() );
    set_seen_cache_dirty( abs_sub.z() );
    set_outside_cache_dirty( abs_sub.z() );
    set_pathfinding_cache_dirty( abs_sub.z() );

    // Fill each submap rather than each tile
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            submap *sm = get_submap_at_grid( point_rel_sm{gridx, gridy} );
            if( sm == nullptr ) {
                debugmsg( "Tried to fill background at (%d,%d) but the submap is not loaded", gridx, gridy );
                continue;
            }
            sm->set_all_ter( type, true );
        }
    }
}

void map::draw_fill_background( ter_id( *f )() )
{
    draw_square_ter( f, point_bub_ms::zero, point_bub_ms( SEEX * my_MAPSIZE - 1,
                     SEEY * my_MAPSIZE - 1 ) );
}
void map::draw_fill_background( const weighted_int_list<ter_id> &f )
{
    draw_square_ter( f, point_bub_ms::zero, point_bub_ms( SEEX * my_MAPSIZE - 1,
                     SEEY * my_MAPSIZE - 1 ) );
}

void map::draw_square_ter( const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                           const int z,
                           bool avoid_creatures )
{
    draw_square( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->ter_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_furn( const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2,
                            const int z,
                            bool avoid_creatures )
{
    draw_square( [this, type, avoid_creatures, z]( const point_bub_ms & p ) {
        this->furn_set( tripoint_bub_ms( p, z ), type, avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_ter( ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2,
                           bool avoid_creatures )
{
    draw_square( [this, f, avoid_creatures]( const point_bub_ms & p ) {
        this->ter_set( p, f(), avoid_creatures );
    }, p1, p2 );
}

void map::draw_square_ter( const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
                           const point_bub_ms &p2, bool avoid_creatures )
{
    draw_square( [this, f, avoid_creatures]( const point_bub_ms & p ) {
        const ter_id *tid = f.pick();
        this->ter_set( p, tid != nullptr ? *tid : ter_str_id::NULL_ID(), avoid_creatures );
    }, p1, p2 );
}

void map::draw_rough_circle_ter( const ter_id &type, const point_bub_ms &p, int rad )
{
    draw_rough_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_rough_circle_furn( const furn_id &type, const point_bub_ms &p, int rad )
{
    draw_rough_circle( [this, type]( const point_bub_ms & q ) {
        if( !is_open_air( tripoint_bub_ms( q, abs_sub.z() ) ) ) {
            this->furn_set( q, type );
        }
    }, p, rad );
}

void map::draw_circle_ter( const ter_id &type, const rl_vec2d &p, double rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_circle_ter( const ter_id &type, const point_bub_ms &p, int rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->ter_set( q, type );
    }, p, rad );
}

void map::draw_circle_furn( const furn_id &type, const point_bub_ms &p, int rad )
{
    draw_circle( [this, type]( const point_bub_ms & q ) {
        this->furn_set( q, type );
    }, p, rad );
}

tripoint_range<tripoint_bub_ms> map::points_in_rectangle( const tripoint_bub_ms &from,
        const tripoint_bub_ms &to ) const
{
    const tripoint_bub_ms min( std::max( 0, std::min( from.x(), to.x() ) ), std::max( 0,
                               std::min( from.y(),
                                         to.y() ) ), std::max( -OVERMAP_DEPTH, std::min( from.z(), to.z() ) ) );
    const tripoint_bub_ms max( std::min( SEEX * my_MAPSIZE - 1, std::max( from.x(), to.x() ) ),
                               std::min( SEEX * my_MAPSIZE - 1, std::max( from.y(), to.y() ) ), std::min( OVERMAP_HEIGHT,
                                       std::max( from.z(), to.z() ) ) );
    return tripoint_range<tripoint_bub_ms>( min, max );
}

tripoint_range<tripoint_bub_ms> map::points_in_radius(
    const tripoint_bub_ms &center, size_t radius, size_t radiusz ) const
{
    const tripoint_bub_ms min(
        std::max<int>( 0, center.x() - radius ), std::max<int>( 0, center.y() - radius ),
        clamp<int>( center.z() - radiusz, -OVERMAP_DEPTH, OVERMAP_HEIGHT ) );
    const tripoint_bub_ms max(
        std::min<int>( SEEX * my_MAPSIZE - 1, center.x() + radius ),
        std::min<int>( SEEX * my_MAPSIZE - 1, center.y() + radius ),
        clamp<int>( center.z() + radiusz, -OVERMAP_DEPTH, OVERMAP_HEIGHT ) );
    return tripoint_range<tripoint_bub_ms>( min, max );
}

tripoint_range<tripoint_bub_ms> map::points_on_zlevel( const int z ) const
{
    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        // TODO: need a default constructor that creates an empty range.
        return tripoint_range<tripoint_bub_ms>( tripoint_bub_ms::zero,
                                                tripoint_bub_ms::zero - tripoint_rel_ms::above );
    }
    return tripoint_range<tripoint_bub_ms>(
    { 0, 0, z }, { SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1, z } );
}

tripoint_range<tripoint_bub_ms> map::points_on_zlevel() const
{
    return points_on_zlevel( abs_sub.z() );

}

void map::queue_main_cleanup()
{
    if( this != &reality_bubble() ) {
        _main_requires_cleanup = true;
    }
}

bool map::is_main_cleanup_queued() const
{
    return _main_requires_cleanup;
}

void map::main_cleanup_override( bool over )
{
    _main_cleanup_override = over;
}

void map::clip_to_bounds( tripoint_bub_ms &p ) const
{
    clip_to_bounds( p.x(), p.y(), p.z() );
}

void map::clip_to_bounds( int &x, int &y ) const
{
    if( x < 0 ) {
        x = 0;
    } else if( x >= SEEX * my_MAPSIZE ) {
        x = SEEX * my_MAPSIZE - 1;
    }

    if( y < 0 ) {
        y = 0;
    } else if( y >= SEEY * my_MAPSIZE ) {
        y = SEEY * my_MAPSIZE - 1;
    }
}

void map::clip_to_bounds( int &x, int &y, int &z ) const
{
    clip_to_bounds( x, y );
    if( z < -OVERMAP_DEPTH ) {
        z = -OVERMAP_DEPTH;
    } else if( z > OVERMAP_HEIGHT ) {
        z = OVERMAP_HEIGHT;
    }
}

int map::calc_max_populated_zlev()
{
    // cache is filled and valid, skip recalculation
    if( max_populated_zlev && max_populated_zlev->first == get_abs_sub() ) {
        return max_populated_zlev->second;
    }

    // We'll assume ground level is populated
    int max_z = 0;

    for( int sz = 1; sz <= OVERMAP_HEIGHT; sz++ ) {
        bool level_done = false;
        for( int sx = 0; sx < my_MAPSIZE; sx++ ) {
            for( int sy = 0; sy < my_MAPSIZE; sy++ ) {
                const submap *sm = get_submap_at_grid( tripoint_rel_sm{ sx, sy, sz } );
                if( sm == nullptr ) {
                    debugmsg( "Tried to calc max populated zlev at (%d,%d,%d) but the submap is not loaded", sx, sy,
                              sz );
                    continue;
                }
                if( !sm->is_uniform() ) {
                    max_z = sz;
                    level_done = true;
                    break;
                }
            }
            if( level_done ) {
                break;
            }
        }
    }

    max_populated_zlev = std::make_pair( get_abs_sub(), max_z );
    return max_z;
}

void map::invalidate_max_populated_zlev( int zlev )
{
    if( max_populated_zlev && max_populated_zlev->second < zlev ) {
        max_populated_zlev->second = zlev;
    }
}

tripoint_bub_ms drawsq_params::center() const
{
    if( view_center.raw() == tripoint::min ) {
        avatar &player_character = get_avatar();
        return player_character.pos_bub() + player_character.view_offset;
    } else {
        return view_center;
    }
}

const ter_str_id &drawsq_params::terrain_override() const
{
    return ter_override;
}

const furn_str_id &drawsq_params::furniture_override() const
{
    return furn_override;
}

void map::for_noisy_vehicle( const std::function<void( const vehicle & )> &fn )
{
    const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int z = zmin; z <= zmax; ++z ) {
        const level_cache *ch = get_cache_lazy( z );
        if( ch == nullptr ) {
            continue;
        }
        for( vehicle *v : ch->vehicle_list ) {
            if( v != nullptr && v->vehicle_noise > 0 ) {
                fn( *v );
            }
        }
    }
}

bool map::has_moving_vehicles()
{
    const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int z = zmin; z <= zmax; ++z ) {
        const level_cache *ch = get_cache_lazy( z );
        if( ch == nullptr ) {
            continue;
        }
        for( const vehicle *v : ch->vehicle_list ) {
            if( v != nullptr && v->is_moving() ) {
                return true;
            }
        }
    }
    return false;
}
