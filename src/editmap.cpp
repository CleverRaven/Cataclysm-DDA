#include "editmap.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <array>
#include <exception>
#include <memory>
#include <set>
#include <utility>
#include <typeinfo>

#include "avatar.h"
#include "calendar.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "debug_menu.h"
#include "field.h"
#include "game.h"
#include "input.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "npc.h"
#include "output.h"
#include "overmapbuffer.h"
#include "scent_map.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "cata_utility.h"
#include "creature.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "omdata.h"
#include "shadowcasting.h"
#include "string_id.h"
#include "colony.h"

static constexpr tripoint editmap_boundary_min( 0, 0, -OVERMAP_DEPTH );
static constexpr tripoint editmap_boundary_max( MAPSIZE_X, MAPSIZE_Y, OVERMAP_HEIGHT + 1 );

static constexpr box editmap_boundaries( editmap_boundary_min, editmap_boundary_max );

static const ter_id undefined_ter_id( -1 );

static std::vector<std::string> fld_string( const std::string &str, int width )
{
    std::vector<std::string> lines;
    if( width < 1 ) {
        lines.push_back( str );
        return lines;
    }

    int linepos = width;
    int linestart = 0;
    int crpos = -2;
    while( linepos < static_cast<int>( str.length() ) || crpos != -1 ) {
        crpos = str.find( '\n', linestart );
        if( crpos != -1 && crpos <= linepos ) {
            lines.push_back( str.substr( linestart, crpos - linestart ) );
            linepos = crpos + width + 1;
            linestart = crpos + 1;
        } else {
            int spacepos = str.rfind( ',', linepos );
            if( spacepos == -1 ) {
                spacepos = str.find( ',', linepos );
            }
            if( spacepos < linestart ) {
                spacepos = linestart + width;
                if( spacepos < static_cast<int>( str.length() ) ) {
                    lines.push_back( str.substr( linestart, width ) );
                    linepos = spacepos + width;
                    linestart = spacepos;
                }
            } else {
                lines.push_back( str.substr( linestart, spacepos - linestart ) );
                linepos = spacepos + width;
                linestart = spacepos;
            }
        }
    }
    lines.push_back( str.substr( linestart ) );
    return lines;
}

template<class SAVEOBJ>
void edit_json( SAVEOBJ &it )
{
    int tmret = -1;
    std::string save1 = serialize( it );
    std::string osave1 = save1;
    std::vector<std::string> fs1 = fld_string( save1, TERMX - 10 );
    do {
        uilist tm;

        for( auto &elem : fs1 ) {
            tm.addentry( -1, false, -2, elem );
        }
        if( tmret == 0 ) {
            try {
                SAVEOBJ tmp;
                deserialize( tmp, save1 );
                it = std::move( tmp );
            } catch( const std::exception &err ) {
                popup( "Error on deserialization: %s", err.what() );
            }
            std::string save2 = serialize( it );
            std::vector<std::string> fs2 = fld_string( save2, TERMX - 10 );

            tm.addentry( -1, false, -2, "== Reloaded: =====================" );
            for( size_t s = 0; s < fs2.size(); ++s ) {
                tm.addentry( -1, false, -2, fs2[s] );
                if( s < fs1.size() && fs2[s] != fs1[s] ) {
                    tm.entries.back().force_color = true;
                    tm.entries.back().text_color = c_light_green;
                    tm.entries[s].force_color = true;
                    tm.entries[s].text_color = c_light_red;
                }
            }
            fs2.clear();
        } else if( tmret == 1 ) {
            string_input_popup popup;
            std::string ret = popup
                              .text( save1 )
                              .query_string();
            if( popup.confirmed() ) {
                fs1 = fld_string( ret, TERMX - 10 );
                save1 = ret;
                tmret = -2;
            }
        } else if( tmret == 2 ) {
            write_to_file( "save/jtest-1j.txt", [&]( std::ostream & fout ) {
                fout << osave1;
            }, nullptr );
            write_to_file( "save/jtest-2j.txt", [&]( std::ostream & fout ) {
                fout << serialize( it );
            }, nullptr );
        }
        tm.addentry( 0, true, 'r', pgettext( "item manipulation debug menu entry", "rehash" ) );
        tm.addentry( 1, true, 'e', pgettext( "item manipulation debug menu entry", "edit" ) );
        tm.addentry( 2, true, 'd', pgettext( "item manipulation debug menu entry",
                                             "dump to save/jtest-*.txt" ) );
        if( tmret != -2 ) {
            tm.query();
            if( tm.ret < 0 ) {
                tmret = 3;
            } else {
                tmret = tm.ret;
            }
        } else {
            tmret = 0;
        }

    } while( tmret != 3 );

}

editmap::editmap()
{
    width = 45;
    height = TERMY;
    offsetX = VIEW_OFFSET_X;
    infoHeight = 0;
    sel_field = -1;
    sel_field_intensity = -1;

    w_info = catacurses::window();
    blink = false;
    altblink = false;
    moveall = false;
    editshape = editmap_rect;
    refresh_mplans = true;

    tmax = point( getmaxx( g->w_terrain ), getmaxy( g->w_terrain ) );
    target_list.clear();
    hilights.clear();
    hilights["mplan"].blink_interval.push_back( true );
    hilights["mplan"].blink_interval.push_back( false );
    hilights["mplan"].cur_blink = 0;
    hilights["mplan"].color = c_red;
    hilights["mplan"].setup();

    hilights["mapgentgt"].blink_interval.push_back( true );
    hilights["mapgentgt"].blink_interval.push_back( false );
    hilights["mapgentgt"].blink_interval.push_back( false );
    hilights["mapgentgt"].cur_blink = 0;
    hilights["mapgentgt"].color = c_cyan;
    hilights["mapgentgt"].setup();

    uberdraw = false;
}

editmap::~editmap() = default;

void editmap_hilight::draw( editmap &em, bool update )
{
    cur_blink++;
    if( cur_blink >= static_cast<int>( blink_interval.size() ) ) {
        cur_blink = 0;
    }
    if( blink_interval[ cur_blink ] || update ) {
        for( auto &elem : points ) {
            const tripoint &p = elem.first;
            // but only if there's no vehicles/mobs/npcs on a point
            if( !g->m.veh_at( p ) && !g->critter_at( p ) ) {
                const ter_t &terrain = g->m.ter( p ).obj();
                char t_sym = terrain.symbol();
                nc_color t_col = terrain.color();

                if( g->m.furn( p ) > 0 ) {
                    const furn_t &furniture_type = g->m.furn( p ).obj();
                    t_sym = furniture_type.symbol();
                    t_col = furniture_type.color();
                }
                const field &t_field = g->m.field_at( p );
                if( t_field.field_count() > 0 ) {
                    field_type_id t_ftype = t_field.displayed_field_type();
                    const field_entry *t_fld = t_field.find_field( t_ftype );
                    if( t_fld != nullptr ) {
                        t_col = t_fld->color();
                        t_sym = t_fld->symbol()[0];
                    }
                }
                if( blink_interval[ cur_blink ] ) {
                    t_col = getbg( t_col );
                }
                tripoint scrpos = em.pos2screen( p );
                mvwputch( g->w_terrain, scrpos.xy(), t_col, t_sym );
            }
        }
    }
}
/*
 * map position to screen position
 */
tripoint editmap::pos2screen( const tripoint &p )
{
    return p + tmax / 2 - target.xy();
}

/*
 * get_direction with extended moving via HJKL keys
 */
bool editmap::eget_direction( tripoint &p, const std::string &action ) const
{
    p = tripoint_zero;
    if( action == "CENTER" ) {
        p = g->u.pos() - target;
    } else if( action == "LEFT_WIDE" ) {
        p.x = -tmax.x / 2;
    } else if( action == "DOWN_WIDE" ) {
        p.y = tmax.y / 2;
    } else if( action == "UP_WIDE" ) {
        p.y = -tmax.y / 2;
    } else if( action == "RIGHT_WIDE" ) {
        p.x = tmax.x / 2;
    } else if( action == "LEVEL_DOWN" ) {
        p.z = -1;
    } else if( action == "LEVEL_UP" ) {
        p.z = 1;
    } else {
        input_context ctxt( "EGET_DIRECTION" );
        ctxt.set_iso( true );
        const cata::optional<tripoint> vec = ctxt.get_direction( action );
        if( !vec ) {
            return false;
        }
        p = *vec;
    }
    return true;
}

cata::optional<tripoint> editmap::edit()
{
    target = g->u.pos() + g->u.view_offset;
    input_context ctxt( "EDITMAP" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "LEFT_WIDE" );
    ctxt.register_action( "RIGHT_WIDE" );
    ctxt.register_action( "UP_WIDE" );
    ctxt.register_action( "DOWN_WIDE" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    ctxt.register_action( "EDIT_TRAPS" );
    ctxt.register_action( "EDIT_FIELDS" );
    ctxt.register_action( "EDIT_TERRAIN" );
    ctxt.register_action( "EDIT_FURNITURE" );
    ctxt.register_action( "EDIT_OVERMAP" );
    ctxt.register_action( "EDIT_ITEMS" );
    ctxt.register_action( "EDIT_MONSTER" );
    ctxt.register_action( "EDITMAP_SHOW_ALL" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // Needed for timeout to be useful
    ctxt.register_action( "ANY_INPUT" );
    std::string action;

    uberdraw = uistate.editmap_nsa_viewmode;
    infoHeight = 20;

    w_info = catacurses::newwin( infoHeight, width, point( offsetX, TERMY - infoHeight ) );
    do {
        if( target_list.empty() ) {
            target_list.push_back( target ); // 'editmap.target_list' always has point 'editmap.target' at least
        }
        if( target_list.size() == 1 ) {
            origin = target;               // 'editmap.origin' only makes sense if we have a list of target points.
        }
        // \u00A0 is the non-breaking space
        update_view_with_help( string_format( pgettext( "keybinding descriptions",
                                              "%s, %s, [%s,%s,%s,%s]\u00A0fast scroll, %s, %s, %s, %s, %s, %s" ),
                                              ctxt.describe_key_and_name( "EDIT_TRAPS" ),
                                              ctxt.describe_key_and_name( "EDIT_FIELDS" ),
                                              ctxt.get_desc( "LEFT_WIDE", 1 ), ctxt.get_desc( "RIGHT_WIDE", 1 ),
                                              ctxt.get_desc( "UP_WIDE", 1 ), ctxt.get_desc( "DOWN_WIDE", 1 ),
                                              ctxt.describe_key_and_name( "EDITMAP_SHOW_ALL" ),
                                              ctxt.describe_key_and_name( "EDIT_TERRAIN" ),
                                              ctxt.describe_key_and_name( "EDIT_FURNITURE" ),
                                              ctxt.describe_key_and_name( "EDIT_OVERMAP" ),
                                              ctxt.describe_key_and_name( "EDIT_ITEMS" ),
                                              ctxt.describe_key_and_name( "QUIT" ) ), pgettext( "map editor state", "Looking around" ) );

        action = ctxt.handle_input( BLINK_SPEED );

        if( action == "EDIT_TERRAIN" ) {
            edit_feature<ter_t>();
        } else if( action == "EDIT_FURNITURE" ) {
            edit_feature<furn_t>();
        } else if( action == "EDIT_FIELDS" ) {
            edit_fld();
        } else if( action == "EDIT_ITEMS" ) {
            edit_itm();
        } else if( action == "EDIT_TRAPS" ) {
            edit_feature<trap>();
        } else if( action == "EDITMAP_SHOW_ALL" ) {
            uberdraw = !uberdraw;
        } else if( action == "EDIT_MONSTER" ) {
            if( Creature *const critter = g->critter_at( target ) ) {
                edit_critter( *critter );
            } else if( g->m.veh_at( target ) ) {
                edit_veh();
            }
        } else if( action == "EDIT_OVERMAP" ) {
            edit_mapgen();
            target_list.clear();
            origin = target;
            target_list.push_back( target );
        } else if( move_target( action, 1 ) ) {
            recalc_target( editshape );         // target_list must follow movement
            if( target_list.size() > 1 ) {
                blink = true;                       // display entire list if it's more than just target point
            }
        } else {
            blink = !blink;
        }
    } while( action != "QUIT" );

    uistate.editmap_nsa_viewmode = uberdraw;

    if( action == "CONFIRM" ) {
        return target;
    }
    return cata::nullopt;
}

/*
 * This is like game::draw_ter except it completely ignores line of sight, lighting, boomered, etc.
 * This is a map editor after all.
 */

void editmap::uber_draw_ter( const catacurses::window &w, map *m )
{
    tripoint center = target;
    tripoint start = center.xy() + tripoint( -getmaxx( w ) / 2, -getmaxy( w ) / 2, target.z );
    tripoint end = center.xy() + tripoint( getmaxx( w ) / 2, getmaxy( w ) / 2, target.z );
    /*
        // pending filter options
        bool draw_furn=true;
        bool draw_ter=true;
        bool draw_trp=true;
        bool draw_fld=true;
        bool draw_veh=true;
    */
    bool draw_itm = true;
    bool game_map = m == &g->m || w == g->w_terrain;
    const int msize = MAPSIZE_X;
    if( refresh_mplans ) {
        hilights["mplan"].points.clear();
    }
    for( int x = start.x, sx = 0; x <= end.x; x++, sx++ ) {
        for( int y = start.y, sy = 0; y <= end.y; y++, sy++ ) {
            tripoint p{ x, y, target.z };
            int sym = game_map ? '%' : ' ';
            if( x >= 0 && x < msize && y >= 0 && y < msize ) {
                if( game_map ) {
                    Creature *critter = g->critter_at( p );
                    if( critter != nullptr ) {
                        critter->draw( w, center.xy(), false );
                    } else {
                        m->drawsq( w, g->u, p, false, draw_itm, center, false, true );
                    }
                    if( refresh_mplans ) {
                        monster *mon = dynamic_cast<monster *>( critter );
                        if( mon != nullptr && mon->pos() != mon->move_target() ) {
                            for( auto &location : line_to( mon->pos(), mon->move_target() ) ) {
                                hilights["mplan"].points[location] = 1;
                            }
                        }
                    }
                } else {
                    m->drawsq( w, g->u, p, false, draw_itm, center, false, true );
                }
            } else {
                mvwputch( w, point( sx, sy ), c_dark_gray, sym );
            }
        }
    }
    if( refresh_mplans ) {
        refresh_mplans = false;
    }
}

void editmap::update_view_with_help( const std::string &txt, const std::string &title )
{
    // updating map
    const Creature *critter = g->critter_at( target );

    werase( g->w_terrain );

    if( uberdraw ) {
        uber_draw_ter( g->w_terrain, &g->m ); // Bypassing the usual draw methods; not versatile enough
    } else {
        g->draw_ter( target ); // But it's optional
    }

    // update target point
    if( critter != nullptr ) {
        critter->draw( g->w_terrain, target, true );
    } else {
        g->m.drawsq( g->w_terrain, g->u, target, true, true, target );
    }
    g->draw_cursor( target );

    // hilight target_list points if blink=true (and if it's more than a point )
    if( blink && target_list.size() > 1 ) {
        for( auto &elem : target_list ) {
            const tripoint &p = elem;
            // but only if there's no vehicles/mobs/npcs on a point
            if( !g->m.veh_at( p ) && !g->critter_at( p ) ) {
                const ter_t &terrain = g->m.ter( p ).obj();
                char t_sym = terrain.symbol();
                nc_color t_col = terrain.color();

                if( g->m.has_furn( p ) ) {
                    const furn_t &furniture_type = g->m.furn( p ).obj();
                    t_sym = furniture_type.symbol();
                    t_col = furniture_type.color();
                }
                const field &t_field = g->m.field_at( p );
                if( t_field.field_count() > 0 ) {
                    field_type_id t_ftype = t_field.displayed_field_type();
                    const field_entry *t_fld = t_field.find_field( t_ftype );
                    if( t_fld != nullptr ) {
                        t_col = t_fld->color();
                        t_sym = t_fld->symbol()[0];
                    }
                }
                t_col = altblink ? green_background( t_col ) : cyan_background( t_col );
                tripoint scrpos = pos2screen( p );
                mvwputch( g->w_terrain, scrpos.xy(), t_col, t_sym );
            }
        }
    }

    // custom hilight.
    // TODO: optimize
    for( auto &elem : hilights ) {
        if( !elem.second.points.empty() ) {
            elem.second.draw( *this );
        }
    }

    // draw arrows if altblink is set (ie, [m]oving a large selection
    if( blink && altblink ) {
        const point mp = tmax / 2 + point_south_east;
        mvwputch( g->w_terrain, point( 1, mp.y ), c_yellow, '<' );
        mvwputch( g->w_terrain, point( tmax.x - 1, mp.y ), c_yellow, '>' );
        mvwputch( g->w_terrain, point( mp.x, 1 ), c_yellow, '^' );
        mvwputch( g->w_terrain, point( mp.x, tmax.y - 1 ), c_yellow, 'v' );
    }

    wrefresh( g->w_terrain );
    g->draw_panels();

    // updating info
    werase( w_info );

    const optional_vpart_position vp = g->m.veh_at( target );
    std::string veh_msg;
    if( !vp ) {
        veh_msg = pgettext( "vehicle", "no" );
    } else if( vp->is_inside() ) {
        veh_msg = pgettext( "vehicle", "in" );
    } else {
        veh_msg = pgettext( "vehicle", "out" );
    }

    const ter_t &terrain_type = g->m.ter( target ).obj();
    const furn_t &furniture_type = g->m.furn( target ).obj();

    int off = 1;
    draw_border( w_info );

    mvwprintz( w_info, point( 2, 0 ), c_light_gray, "< %d,%d >", target.x, target.y );

    mvwputch( w_info, point( 2, off ), terrain_type.color(), terrain_type.symbol() );
    mvwprintw( w_info, point( 4, off ), _( "%d: %s; movecost %d" ), g->m.ter( target ).to_i(),
               terrain_type.name(),
               terrain_type.movecost
             );
    off++; // 2
    if( g->m.furn( target ) > 0 ) {
        mvwputch( w_info, point( 2, off ), furniture_type.color(), furniture_type.symbol() );
        mvwprintw( w_info, point( 4, off ), _( "%d: %s; movecost %d movestr %d" ),
                   g->m.furn( target ).to_i(),
                   furniture_type.name(),
                   furniture_type.movecost,
                   furniture_type.move_str_req
                 );
        off++; // 3
    }
    const auto &map_cache = g->m.get_cache( target.z );

    const std::string u_see_msg = g->u.sees( target ) ? _( "yes" ) : _( "no" );
    mvwprintw( w_info, point( 1, off++ ), _( "dist: %d u_see: %s veh: %s scent: %d" ),
               rl_dist( g->u.pos(), target ), u_see_msg, veh_msg, g->scent.get( target ) );
    mvwprintw( w_info, point( 1, off++ ), _( "sight_range: %d, daylight_sight_range: %d," ),
               g->u.sight_range( g->light_level( g->u.posz() ) ),
               g->u.sight_range( current_daylight_level( calendar::turn ) ) );
    mvwprintw( w_info, point( 1, off++ ), _( "transparency: %.5f, visibility: %.5f," ),
               map_cache.transparency_cache[target.x][target.y],
               map_cache.seen_cache[target.x][target.y] );
    map::apparent_light_info al = map::apparent_light_helper( map_cache, target );
    int apparent_light = static_cast<int>(
                             g->m.apparent_light_at( target, g->m.get_visibility_variables_cache() ) );
    mvwprintw( w_info, point( 1, off++ ), _( "outside: %d obstructed: %d" ),
               static_cast<int>( g->m.is_outside( target ) ),
               static_cast<int>( al.obstructed ) );
    mvwprintw( w_info, point( 1, off++ ), _( "light_at: %s" ),
               map_cache.lm[target.x][target.y].to_string() );
    mvwprintw( w_info, point( 1, off++ ), _( "apparent light: %.5f (%d)" ),
               al.apparent_light, apparent_light );
    std::string extras;
    if( vp ) {
        extras += _( " [vehicle]" );
    }
    if( g->m.has_flag( TFLAG_INDOORS, target ) ) {
        extras += _( " [indoors]" );
    }
    if( g->m.has_flag( TFLAG_SUPPORTS_ROOF, target ) ) {
        extras += _( " [roof]" );
    }

    mvwprintw( w_info, point( 1, off ), "%s %s", g->m.features( target ), extras );
    // 9
    off++;
    for( auto &fld : g->m.get_field( target ) ) {
        const field_entry &cur = fld.second;
        mvwprintz( w_info, point( 1, off ), cur.color(),
                   _( "field: %s L:%d[%s] A:%d" ),
                   cur.get_field_type().id().str(),
                   cur.get_field_intensity(),
                   cur.name(),
                   to_turns<int>( cur.get_field_age() )
                 );
        off++; // 10ish
    }

    const trap &cur_trap = g->m.tr_at( target );
    if( cur_trap.loadid != tr_null ) {
        mvwprintz( w_info, point( 1, off ), cur_trap.color, _( "trap: %s (%d)" ), cur_trap.name(),
                   cur_trap.loadid.to_i() );
        off++; // 11
    }

    if( critter != nullptr ) {
        off = critter->print_info( w_info, off, 5, 1 );
    } else if( vp ) {
        mvwprintw( w_info, point( 1, off ), _( "There is a %s there. Parts:" ), vp->vehicle().name );
        off++;
        vp->vehicle().print_part_list( w_info, off, getmaxy( w_info ) - 1, width, vp->part_index() );
        off += 6;
    }
    map_stack target_stack = g->m.i_at( target );
    const int target_stack_size = target_stack.size();
    if( !g->m.has_flag( "CONTAINER", target ) && target_stack_size > 0 ) {
        trim_and_print( w_info, point( 1, off ), getmaxx( w_info ), c_light_gray,
                        _( "There is a %s there." ),
                        target_stack.begin()->tname() );
        off++;
        if( target_stack_size > 1 ) {
            mvwprintw( w_info, point( 1, off ), ngettext( "There is %d other item there as well.",
                       "There are %d other items there as well.",
                       target_stack_size - 1 ),
                       target_stack_size - 1 );
            off++;
        }
    }

    if( g->m.has_graffiti_at( target ) ) {
        mvwprintw( w_info, point( 1, off ),
                   g->m.ter( target ) == t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                   g->m.graffiti_at( target ) );
    }

    // updating help
    int line = getmaxy( w_info ) - 1;
    if( !title.empty() ) {
        const std::string str = string_format( "< <color_cyan>%s</color> >", title );
        center_print( w_info, line, BORDER_COLOR, str );
    }
    --line;
    std::vector<std::string> folded = foldstring( txt, width - 2 );
    for( auto it = folded.rbegin(); it != folded.rend(); ++it, --line ) {
        nc_color dummy = c_light_gray;
        print_colored_text( w_info, point( 1, line ), dummy, c_light_gray, *it );
    }
    wrefresh( w_info );
}

static ter_id get_alt_ter( bool isvert, ter_id sel_ter )
{
    std::map<std::string, std::string> alts;
    alts["_v"] = "_h";
    alts["_vertical"] = "_horizontal";
    alts["_v_alarm"] = "_h_alarm";
    const std::string tersid = sel_ter.obj().id.str();
    const int sidlen = tersid.size();
    for( std::map<std::string, std::string>::const_iterator it = alts.begin(); it != alts.end();
         ++it ) {
        const std::string suffix = isvert ? it->first : it->second;
        const int slen = suffix.size();
        if( sidlen > slen && tersid.substr( sidlen - slen, slen ) == suffix ) {
            const std::string asuffix = isvert ? it->second : it->first;
            const std::string terasid = tersid.substr( 0, sidlen - slen ) + asuffix;
            const ter_str_id tid( terasid );

            if( tid.is_valid() ) {
                return tid.id();
            }
        }
    }
    return undefined_ter_id;
}

template<typename T_t>
static std::string info_title();

template<>
std::string info_title<ter_t>()
{
    return pgettext( "map editor state", "Terrain" );
}

template<>
std::string info_title<furn_t>()
{
    return pgettext( "map editor state", "Furniture" );
}

template<>
std::string info_title<trap>()
{
    return pgettext( "map editor: traps editing", "Traps" );
}

template<typename T_id>
static T_id feature( const tripoint &p );

template<>
ter_id feature<ter_id>( const tripoint &p )
{
    return g->m.ter( p );
}

template<>
furn_id feature<furn_id>( const tripoint &p )
{
    return g->m.furn( p );
}

template<>
trap_id feature<trap_id>( const tripoint &p )
{
    return g->m.tr_at( p ).loadid;
}

template<typename T_t>
static int symbol( const T_t &t );

template<>
int symbol( const ter_t &t )
{
    return t.symbol();
}

template<>
int symbol( const furn_t &t )
{
    return t.symbol();
}

template<>
int symbol( const trap &t )
{
    return t.sym;
}

template<typename T_t>
static nc_color color( const T_t &t );

template<>
nc_color color( const ter_t &t )
{
    return t.color();
}

template<>
nc_color color( const furn_t &t )
{
    return t.color();
}

template<>
nc_color color( const trap &t )
{
    return t.color;
}

template<typename T_t>
static std::string describe( const T_t &t );

template<>
std::string describe( const ter_t &type )
{
    return string_format( _( "Move cost: %d\nIndoors: %s\nRoof: %s" ), type.movecost,
                          type.has_flag( TFLAG_INDOORS ) ? _( "Yes" ) : _( "No" ),
                          type.has_flag( TFLAG_SUPPORTS_ROOF ) ? _( "Yes" ) : _( "No" ) );
}

template<>
std::string describe( const furn_t &type )
{
    return string_format( _( "Move cost: %d\nIndoors: %s\nRoof: %s" ), type.movecost,
                          type.has_flag( TFLAG_INDOORS ) ? _( "Yes" ) : _( "No" ),
                          type.has_flag( TFLAG_SUPPORTS_ROOF ) ? _( "Yes" ) : _( "No" ) );
}

template<>
std::string describe( const trap &type )
{
    return string_format( _( "Visible: %d\nAvoidance: %d\nDifficulty: %d\nBenign: %s" ),
                          type.get_visibility(), type.get_avoidance(), type.get_difficulty(),
                          type.is_benign() ? _( "Yes" ) : _( "No" ) );
}

template<typename T_t>
static void apply( const T_t &t, const shapetype editshape, const tripoint &target,
                   const tripoint &origin, const std::vector<tripoint> &target_list );

template<>
void apply<ter_t>( const ter_t &t, const shapetype editshape, const tripoint &target,
                   const tripoint &origin, const std::vector<tripoint> &target_list )
{
    bool isvert = false;
    bool ishori = false;
    bool doalt = false;
    ter_id teralt = undefined_ter_id;
    int alta = -1;
    int altb = -1;
    const ter_id sel_ter = t.id.id();
    if( editshape == editmap_rect ) {
        if( t.symbol() == LINE_XOXO || t.symbol() == '|' ) {
            isvert = true;
            teralt = get_alt_ter( isvert, sel_ter );
        } else if( t.symbol() == LINE_OXOX || t.symbol() == '-' ) {
            ishori = true;
            teralt = get_alt_ter( isvert, sel_ter );
        }
        if( teralt != undefined_ter_id ) {
            if( isvert ) {
                alta = target.y;
                altb = origin.y;
            } else {
                alta = target.x;
                altb = origin.x;
            }
            doalt = true;
        }
    }

    for( auto &elem : target_list ) {
        ter_id wter = sel_ter;
        if( doalt ) {
            if( isvert && ( elem.y == alta || elem.y == altb ) ) {
                wter = teralt;
            } else if( ishori && ( elem.x == alta || elem.x == altb ) ) {
                wter = teralt;
            }
        }
        g->m.ter_set( elem, wter );
    }
}

template<>
void apply<furn_t>( const furn_t &t, const shapetype, const tripoint &,
                    const tripoint &, const std::vector<tripoint> &target_list )
{
    const furn_id sel_frn = t.id.id();
    for( auto &elem : target_list ) {
        g->m.furn_set( elem, sel_frn );
    }
}

template<>
void apply<trap>( const trap &t, const shapetype, const tripoint &,
                  const tripoint &, const std::vector<tripoint> &target_list )
{
    for( auto &elem : target_list ) {
        g->m.trap_set( elem, t.loadid );
    }
}

// edit terrain, furnitrue, or traps
template<typename T_t>
void editmap::edit_feature()
{
    if( T_t::count() == 0 ) {
        debugmsg( "Empty %s list", typeid( T_t ).name() );
        return;
    }

    typedef decltype( T_t().id.id() ) T_id;

    uilist emenu;
    emenu.w_width = width;
    emenu.w_height = TERMY - infoHeight;
    emenu.w_y = 0;
    emenu.w_x = offsetX;
    emenu.desc_enabled = true;
    emenu.input_category = "EDITMAP_FEATURE";
    emenu.additional_actions = {
        { "CONFIRM_QUIT", "" },
        { "EDITMAP_SHOW_ALL", "" },
        { "EDITMAP_TAB", "" },
        { "EDITMAP_MOVE", "" },
        { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
    };
    emenu.allow_additional = true;

    for( int i = 0; i < static_cast<int>( T_t::count() ); ++i ) {
        const T_t &type = T_id( i ).obj();
        std::string name;
        if( type.name().empty() ) {
            name = string_format( pgettext( "map feature id", "(%s)" ), type.id.str() );
        } else {
            name = string_format( pgettext( "map feature name and id", "%s (%s)" ), type.name(),
                                  type.id.str() );
        }
        uilist_entry ent( name );
        ent.retval = i;
        ent.enabled = true;
        ent.hotkey = 0;
        ent.extratxt.sym = symbol( type );
        ent.extratxt.color = color( type );
        ent.desc = describe( type );

        emenu.entries.emplace_back( ent );
    }
    int current_feature = emenu.selected = feature<T_id>( target ).to_i();
    emenu.entries[current_feature].text_color = c_green;

    bool quit = false;
    do {
        input_context ctxt( emenu.input_category );
        update_view_with_help( string_format( pgettext( "keybinding descriptions", "%s, %s, %s, %s, %s" ),
                                              ctxt.describe_key_and_name( "CONFIRM" ),
                                              ctxt.describe_key_and_name( "CONFIRM_QUIT" ),
                                              ctxt.describe_key_and_name( "EDITMAP_SHOW_ALL" ),
                                              ctxt.describe_key_and_name( "EDITMAP_TAB" ),
                                              ctxt.describe_key_and_name( "EDITMAP_MOVE" ) ),
                               info_title<T_t>() );
        emenu.query();
        if( emenu.ret == UILIST_CANCEL ) {
            quit = true;
        } else if( ( emenu.ret >= 0 && static_cast<size_t>( emenu.ret ) < T_t::count() ) ||
                   ( emenu.ret == UILIST_ADDITIONAL && emenu.ret_act == "CONFIRM_QUIT" ) ) {
            apply( T_id( emenu.selected ).obj(), editshape, target, origin, target_list );
            emenu.entries[current_feature].text_color = emenu.text_color;
            current_feature = emenu.selected;
            emenu.entries[current_feature].text_color = c_green;
            quit = emenu.ret == UILIST_ADDITIONAL;
        } else if( emenu.ret == UILIST_ADDITIONAL ) {
            if( emenu.ret_act == "EDITMAP_TAB" ) {
                select_shape( editshape, 0 );
                emenu.entries[current_feature].text_color = emenu.text_color;
                current_feature = feature<T_id>( target ).to_i();
                emenu.entries[current_feature].text_color = c_green;
            } else if( emenu.ret_act == "EDITMAP_MOVE" ) {
                select_shape( editshape, 1 );
                emenu.entries[current_feature].text_color = emenu.text_color;
                current_feature = feature<T_id>( target ).to_i();
                emenu.entries[current_feature].text_color = c_green;
            } else if( emenu.ret_act == "EDITMAP_SHOW_ALL" ) {
                uberdraw = !uberdraw;
            }
        }
    } while( !quit );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// field edit

void editmap::update_fmenu_entry( uilist &fmenu, field &field, const field_type_id idx )
{
    int field_intensity = 1;
    const field_type &ftype = idx.obj();
    field_entry *fld = field.find_field( idx );
    if( fld != nullptr ) {
        field_intensity = fld->get_field_intensity();
    }
    fmenu.entries[idx].txt = ftype.get_name( field_intensity - 1 );
    if( fld != nullptr ) {
        fmenu.entries[idx].txt += " " + std::string( field_intensity, '*' );
    }
    fmenu.entries[idx].text_color = fld != nullptr ? c_cyan : fmenu.text_color;
    fmenu.entries[idx].extratxt.color = ftype.get_color( field_intensity - 1 );
    fmenu.entries[idx].extratxt.txt = ftype.get_symbol( field_intensity - 1 );
}

void editmap::setup_fmenu( uilist &fmenu )
{
    fmenu.entries.clear();
    for( int i = 0; i < static_cast<int>( field_type::count() ); i++ ) {
        const field_type_id fid = static_cast<field_type_id>( i );
        fmenu.addentry( fid, true, -2, "" );
        fmenu.entries[fid].extratxt.left = 1;
        update_fmenu_entry( fmenu, g->m.get_field( target ), fid );
    }
    if( sel_field >= 0 ) {
        fmenu.selected = sel_field;
    }
}

void editmap::edit_fld()
{
    uilist fmenu;
    fmenu.w_width = width;
    fmenu.w_height = TERMY - infoHeight;
    fmenu.w_y = 0;
    fmenu.w_x = offsetX;
    setup_fmenu( fmenu );
    fmenu.input_category = "EDIT_FIELDS";
    fmenu.additional_actions = {
        { "EDITMAP_TAB", "" },
        { "EDITMAP_MOVE", "" },
        { "LEFT", "" },
        { "RIGHT", "" },
        { "EDITMAP_SHOW_ALL", "" },
        { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
    };
    fmenu.allow_additional = true;

    do {
        input_context ctxt( fmenu.input_category );
        // \u00A0 is the non-breaking space
        update_view_with_help( string_format( pgettext( "keybinding descriptions",
                                              "%s, %s, [%s,%s]\u00A0intensity, %s, %s, %s" ),
                                              ctxt.describe_key_and_name( "EDITMAP_TAB" ),
                                              ctxt.describe_key_and_name( "EDITMAP_MOVE" ),
                                              ctxt.get_desc( "LEFT", 1 ), ctxt.get_desc( "RIGHT", 1 ),
                                              ctxt.describe_key_and_name( "CONFIRM" ),
                                              ctxt.describe_key_and_name( "QUIT" ),
                                              ctxt.describe_key_and_name( "EDITMAP_SHOW_ALL" ) ),
                               pgettext( "Map editor: Editing field effects", "Field effects" ) );
        fmenu.query();
        if( ( fmenu.ret > 0 && static_cast<size_t>( fmenu.ret ) < field_type::count() ) ||
            ( fmenu.ret == UILIST_ADDITIONAL && ( fmenu.ret_act == "LEFT" || fmenu.ret_act == "RIGHT" ) ) ) {

            int field_intensity = 0;
            const field_type_id idx = static_cast<field_type_id>( fmenu.selected );
            field_entry *fld = g->m.get_field( target ).find_field( idx );
            if( fld != nullptr ) {
                field_intensity = fld->get_field_intensity();
            }
            const field_type &ftype = idx.obj();
            int fsel_intensity = field_intensity;
            if( fmenu.ret > 0 ) {
                uilist femenu;
                femenu.w_width = width;
                femenu.w_height = infoHeight;
                femenu.w_y = fmenu.w_height;
                femenu.w_x = offsetX;

                femenu.text = field_intensity < 1 ? "" : ftype.get_name( field_intensity - 1 );
                femenu.addentry( pgettext( "map editor: used to describe a clean field (e.g. without blood)",
                                           "-clear-" ) );

                int i = 0;
                for( const auto &intensity_level : ftype.intensity_levels ) {
                    i++;
                    femenu.addentry( string_format( "%d: %s", i, _( intensity_level.name ) ) );
                }
                femenu.entries[field_intensity].text_color = c_cyan;
                femenu.selected = sel_field_intensity > 0 ? sel_field_intensity : field_intensity;

                femenu.query();
                if( femenu.ret >= 0 ) {
                    fsel_intensity = femenu.ret;
                }
            } else if( fmenu.ret_act == "RIGHT" &&
                       field_intensity < static_cast<int>( ftype.get_max_intensity() ) ) {
                fsel_intensity++;
            } else if( fmenu.ret_act == "LEFT" && field_intensity > 0 ) {
                fsel_intensity--;
            }
            if( field_intensity != fsel_intensity || target_list.size() > 1 ) {
                for( auto &elem : target_list ) {
                    const auto fid = static_cast<field_type_id>( idx );
                    field &t_field = g->m.get_field( elem );
                    field_entry *t_fld = t_field.find_field( fid );
                    int t_intensity = 0;
                    if( t_fld != nullptr ) {
                        t_intensity = t_fld->get_field_intensity();
                    }
                    if( fsel_intensity != 0 ) {
                        if( t_intensity != 0 ) {
                            g->m.set_field_intensity( elem, fid, fsel_intensity );
                        } else {
                            g->m.add_field( elem, fid, fsel_intensity );
                        }
                    } else {
                        if( t_intensity != 0 ) {
                            g->m.remove_field( elem, fid );
                        }
                    }
                }
                update_fmenu_entry( fmenu, g->m.get_field( target ), idx );
                sel_field = fmenu.selected;
                sel_field_intensity = fsel_intensity;
            }
        } else if( fmenu.ret == 0 ) {
            for( auto &elem : target_list ) {
                field &t_field = g->m.get_field( elem );
                while( t_field.field_count() > 0 ) {
                    const auto rmid = t_field.begin()->first;
                    g->m.remove_field( elem, rmid );
                    if( elem == target ) {
                        update_fmenu_entry( fmenu, t_field, rmid );
                    }
                }
            }
            sel_field = fmenu.selected;
            sel_field_intensity = 0;
        } else if( fmenu.ret == UILIST_ADDITIONAL ) {
            if( fmenu.ret_act == "EDITMAP_TAB" ) {
                int sel_tmp = fmenu.selected;
                int ret = select_shape( editshape, 0 );
                if( ret > 0 ) {
                    setup_fmenu( fmenu );
                }
                fmenu.selected = sel_tmp;
            } else if( fmenu.ret_act == "EDITMAP_MOVE" ) {
                int sel_tmp = fmenu.selected;
                int ret = select_shape( editshape, 1 );
                if( ret > 0 ) {
                    setup_fmenu( fmenu );
                }
                fmenu.selected = sel_tmp;
            } else if( fmenu.ret_act == "EDITMAP_SHOW_ALL" ) {
                uberdraw = !uberdraw;
            }
        }
    } while( fmenu.ret != UILIST_CANCEL );
}

/*
 * edit items in target square. WIP
 */
enum editmap_imenu_ent {
    imenu_bday, imenu_damage, imenu_burnt,
    imenu_sep,
    imenu_savetest,
    imenu_exit,
};

void editmap::edit_itm()
{
    uilist ilmenu;
    ilmenu.w_x = offsetX;
    ilmenu.w_y = 0;
    ilmenu.w_width = width;
    ilmenu.w_height = TERMY - infoHeight - 1;
    auto items = g->m.i_at( target );
    int i = 0;
    for( auto &an_item : items ) {
        ilmenu.addentry( i++, true, 0, "%s%s", an_item.tname(),
                         an_item.is_emissive() ? " L" : "" );
    }
    ilmenu.addentry( items.size(), true, 'a', _( "Add item" ) );
    ilmenu.input_category = "EDIT_ITEMS";
    ilmenu.additional_actions = {
        { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
    };
    ilmenu.allow_additional = true;

    do {
        update_view_with_help( "", "" );
        ilmenu.query();
        if( ilmenu.ret >= 0 && ilmenu.ret < static_cast<int>( items.size() ) ) {
            item &it = *items.get_iterator_from_index( ilmenu.ret );
            uilist imenu;
            imenu.w_x = ilmenu.w_x;
            imenu.w_y = ilmenu.w_height;
            imenu.w_height = TERMX - ilmenu.w_height;
            imenu.w_width = ilmenu.w_width;
            imenu.addentry( imenu_bday, true, -1, pgettext( "item manipulation debug menu entry", "bday: %d" ),
                            to_turn<int>( it.birthday() ) );
            imenu.addentry( imenu_damage, true, -1, pgettext( "item manipulation debug menu entry",
                            "damage: %d" ), it.damage() );
            imenu.addentry( imenu_burnt, true, -1, pgettext( "item manipulation debug menu entry",
                            "burnt: %d" ), static_cast<int>( it.burnt ) );
            imenu.addentry( imenu_sep, false, 0, pgettext( "item manipulation debug menu entry",
                            "-[ light emission ]-" ) );
            imenu.addentry( imenu_savetest, true, -1, pgettext( "item manipulation debug menu entry",
                            "savetest" ) );
            imenu.input_category = "EDIT_ITEMS";
            imenu.additional_actions = {
                { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
            };
            imenu.allow_additional = true;

            do {
                ilmenu.show();
                imenu.query();
                if( imenu.ret >= 0 && imenu.ret < imenu_savetest ) {
                    int intval = -1;
                    switch( imenu.ret ) {
                        case imenu_bday:
                            intval = to_turn<int>( it.birthday() );
                            break;
                        case imenu_damage:
                            intval = it.damage();
                            break;
                        case imenu_burnt:
                            intval = static_cast<int>( it.burnt );
                            break;
                    }
                    string_input_popup popup;
                    int retval = popup
                                 .title( "set: " )
                                 .width( 20 )
                                 .text( to_string( intval ) )
                                 .query_int();
                    if( popup.confirmed() ) {
                        switch( imenu.ret ) {
                            case imenu_bday:
                                it.set_birthday( time_point::from_turn( retval ) );
                                imenu.entries[imenu_bday].txt = string_format( "bday: %d", to_turn<int>( it.birthday() ) );
                                break;
                            case imenu_damage:
                                it.set_damage( retval );
                                imenu.entries[imenu_damage].txt = string_format( "damage: %d", it.damage() );
                                break;
                            case imenu_burnt:
                                it.burnt = retval;
                                imenu.entries[imenu_burnt].txt = string_format( "burnt: %d", it.burnt );
                                break;
                        }
                    }
                } else if( imenu.ret == imenu_savetest ) {
                    edit_json( it );
                }
                g->draw_ter( target );
                wrefresh( g->w_terrain );
                g->draw_panels();
            } while( imenu.ret != UILIST_CANCEL );
        } else if( ilmenu.ret == static_cast<int>( items.size() ) ) {
            debug_menu::wishitem( nullptr, target.x, target.y, target.z );
            ilmenu.entries.clear();
            i = 0;
            for( auto &an_item : items ) {
                ilmenu.addentry( i++, true, 0, "%s%s", an_item.tname(),
                                 an_item.is_emissive() ? " L" : "" );
            }
            ilmenu.addentry( items.size(), true, 'a',
                             pgettext( "item manipulation debug menu entry for adding an item on a tile", "Add item" ) );
            ilmenu.setup();
            ilmenu.filterlist();
            ilmenu.refresh();
        }
    } while( ilmenu.ret != UILIST_CANCEL );
}

/*
 *  Todo
 */
void editmap::edit_critter( Creature &critter )
{
    if( monster *const mon_ptr = dynamic_cast<monster *>( &critter ) ) {
        edit_json( *mon_ptr );
    } else if( npc *const npc_ptr = dynamic_cast<npc *>( &critter ) ) {
        edit_json( *npc_ptr );
    }
}

void editmap::edit_veh()
{
    edit_json( g->m.veh_at( target )->vehicle() );
}

/*
 *  Calculate target_list based on origin and target class variables, and shapetype.
 */
void editmap::recalc_target( shapetype shape )
{
    const int z = target.z;
    target_list.clear();
    switch( shape ) {
        case editmap_circle: {
            int radius = rl_dist( origin, target );
            for( int x = origin.x - radius; x <= origin.x + radius; x++ ) {
                for( int y = origin.y - radius; y <= origin.y + radius; y++ ) {
                    const tripoint p( x, y, z );
                    if( rl_dist( p, origin ) <= radius ) {
                        if( editmap_boundaries.contains_half_open( p ) ) {
                            target_list.push_back( p );
                        }
                    }
                }
            }
        }
        break;
        case editmap_rect_filled:
        case editmap_rect:
            int sx;
            int sy;
            int ex;
            int ey;
            if( target.x < origin.x ) {
                sx = target.x;
                ex = origin.x;
            } else {
                sx = origin.x;
                ex = target.x;
            }
            if( target.y < origin.y ) {
                sy = target.y;
                ey = origin.y;
            } else {
                sy = origin.y;
                ey = target.y;
            }
            for( int x = sx; x <= ex; x++ ) {
                for( int y = sy; y <= ey; y++ ) {
                    if( shape == editmap_rect_filled || x == sx || x == ex || y == sy || y == ey ) {
                        const tripoint p( x, y, z );
                        if( editmap_boundaries.contains_half_open( p ) ) {
                            target_list.push_back( p );
                        }
                    }
                }
            }
            break;
        case editmap_line:
            target_list = line_to( origin, target, 0, 0 );
            break;
    }
}

/*
 * Shift 'var' (ie, part of a coordinate plane) by 'shift'.
 * If the result is not >= min and < 'max', constrain the result and adjust 'shift',
 * so it can adjust subsequent points of a set consistently.
 */
static int limited_shift( int var, int &shift, int min, int max )
{
    if( var + shift < min ) {
        shift = min - var;
    } else if( var + shift >= max ) {
        shift = shift + ( max - 1 - ( var + shift ) );
    }
    return var += shift;
}

/*
 * Move point 'editmap.target' via keystroke. 'moveorigin' determines if point 'editmap.origin' is moved as well:
 * 0: no, 1: yes, -1 (or none): as per bool 'editmap.moveall'.
 * if input or ch are not valid movement keys, do nothing and return false
 */
bool editmap::move_target( const std::string &action, int moveorigin )
{
    tripoint mp;
    bool move_origin = moveorigin == 1 ? true :
                       moveorigin == 0 ? false : moveall;
    if( eget_direction( mp, action ) ) {
        target.x = limited_shift( target.x, mp.x, 0, MAPSIZE_X );
        target.y = limited_shift( target.y, mp.y, 0, MAPSIZE_Y );
        target.z = limited_shift( target.z, mp.z, -OVERMAP_DEPTH, OVERMAP_HEIGHT + 1 );
        if( move_origin ) {
            origin += mp;
        }
        return true;
    }
    return false;
}

/*
 * Interactively select, resize, and move the list of target coordinates
 */
int editmap::select_shape( shapetype shape, int mode )
{
    tripoint orig = target;
    tripoint origor = origin;
    input_context ctxt( "EDITMAP_SHAPE" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "LEFT_WIDE" );
    ctxt.register_action( "RIGHT_WIDE" );
    ctxt.register_action( "UP_WIDE" );
    ctxt.register_action( "DOWN_WIDE" );
    ctxt.register_action( "MOVE_DOWN" );
    ctxt.register_action( "MOVE_UP" );
    ctxt.register_action( "RESIZE" );
    ctxt.register_action( "SWAP" );
    ctxt.register_action( "EDITMAP_MOVE" );
    ctxt.register_action( "START" );
    ctxt.register_action( "EDITMAP_SHOW_ALL" );
    ctxt.register_action( "EDITMAP_TAB" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );
    std::string action;
    bool update = false;
    blink = true;
    if( mode >= 0 ) {
        moveall = mode != 0;
    }
    altblink = moveall;

    do {
        if( moveall ) {
            update_view_with_help( string_format( pgettext( "keybinding descriptions", "%s, %s, %s, %s, %s" ),
                                                  ctxt.describe_key_and_name( "RESIZE" ),
                                                  ctxt.describe_key_and_name( "SWAP" ),
                                                  ctxt.describe_key_and_name( "CONFIRM" ),
                                                  ctxt.describe_key_and_name( "QUIT" ),
                                                  ctxt.describe_key_and_name( "EDITMAP_SHOW_ALL" ) ),
                                   _( "Moving selection" ) );
        } else {
            update_view_with_help( string_format( pgettext( "keybinding descriptions",
                                                  "%s, %s, %s, %s, %s, %s, %s" ),
                                                  ctxt.describe_key_and_name( "EDITMAP_MOVE" ),
                                                  ctxt.describe_key_and_name( "RESIZE" ),
                                                  ctxt.describe_key_and_name( "SWAP" ),
                                                  ctxt.describe_key_and_name( "START" ),
                                                  ctxt.describe_key_and_name( "CONFIRM" ),
                                                  ctxt.describe_key_and_name( "QUIT" ),
                                                  ctxt.describe_key_and_name( "EDITMAP_SHOW_ALL" ) ),
                                   _( "Resizing selection" ) );
        }
        action = ctxt.handle_input( BLINK_SPEED );
        if( action == "RESIZE" ) {
            if( !moveall ) {
                const int offset = 16;
                uilist smenu;
                smenu.text = _( "Selection type" );
                smenu.w_x = ( offsetX + offset ) / 2;
                smenu.addentry( editmap_rect, true, 'r', pgettext( "shape", "Rectangle" ) );
                smenu.addentry( editmap_rect_filled, true, 'f', pgettext( "shape", "Filled Rectangle" ) );
                smenu.addentry( editmap_line, true, 'l', pgettext( "shape", "Line" ) );
                smenu.addentry( editmap_circle, true, 'c', pgettext( "shape", "Filled Circle" ) );
                smenu.addentry( -2, true, 'p', pgettext( "shape", "Point" ) );
                smenu.selected = static_cast<int>( shape );
                smenu.additional_actions = {
                    { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
                };
                smenu.allow_additional = true;
                do {
                    update_view_with_help( "", pgettext( "map editor state", "Select a shape" ) );
                    smenu.query();
                    if( smenu.ret == UILIST_CANCEL ) {
                        // canceled
                    } else if( smenu.ret != -2 ) {
                        shape = static_cast<shapetype>( smenu.ret );
                        update = true;
                    } else if( smenu.ret != UILIST_ADDITIONAL ) {
                        target_list.clear();
                        origin = target;
                        target_list.push_back( target );
                        moveall = true;
                    }
                } while( smenu.ret == UILIST_ADDITIONAL );
            } else {
                moveall = false;
            }
        } else if( !moveall && action == "START" ) {
            target = origin;
            update = true;
        } else if( action == "SWAP" ) {
            tripoint tmporigin = origin;
            origin = target;
            target = tmporigin;
            update = true;
        } else if( action == "EDITMAP_MOVE" ) {
            moveall = true;
        } else if( action == "EDITMAP_SHOW_ALL" ) {
            uberdraw = !uberdraw;
        } else if( action == "EDITMAP_TAB" ) {
            if( moveall ) {
                moveall = false;
                altblink = moveall;
                action = "CONFIRM";
            } else {
                moveall = true;
            }
        } else if( move_target( action ) ) {
            update = true;
        } else {
            blink = !blink;
        }
        if( update ) {
            blink = true;
            update = false;
            recalc_target( shape );
        }
        altblink = moveall;
    } while( action != "CONFIRM" && action != "QUIT" );
    blink = true;
    altblink = false;
    if( action == "CONFIRM" ) {
        editshape = shape;
        return target_list.size();
    } else {
        target_list.clear();
        target = orig;
        origin = origor;
        target_list.push_back( target );
        blink = false;
        return -1;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Display mapgen results over selected target position, and optionally regenerate / apply / abort
 */
void editmap::mapgen_preview( const real_coords &tc, uilist &gmenu )
{
    hilights["mapgentgt"].points.clear();
    hilights["mapgentgt"].points[target + point( -SEEX, -SEEY )] = 1;
    hilights["mapgentgt"].points[target + point( 1 + SEEX, 1 + SEEY )] = 1;
    hilights["mapgentgt"].points[target + point( -SEEX, 1 + SEEY )] = 1;
    hilights["mapgentgt"].points[target + point( 1 + SEEX, -SEEY )] = 1;

    // Coordinates of the overmap terrain that should be generated.
    const point omt_pos = ms_to_omt_copy( tc.abs_pos );
    oter_id &omt_ref = overmap_buffer.ter( tripoint( omt_pos, target.z ) );
    // Copy to store the original value, to restore it upon canceling
    const oter_id orig_oters = omt_ref;
    omt_ref = oter_id( gmenu.ret );
    tinymap tmpmap;
    // TODO: add a do-not-save-generated-submaps parameter
    // TODO: keep track of generated submaps to delete them properly and to avoid memory leaks
    tmpmap.generate( tripoint( omt_pos.x * 2, omt_pos.y * 2, target.z ), calendar::turn );

    tripoint pofs = pos2screen( target + point( 1 - SEEX, 1 - SEEY ) );
    catacurses::window w_preview = catacurses::newwin( SEEX * 2, SEEY * 2, pofs.xy() );

    gmenu.border_color = c_light_gray;
    gmenu.hilight_color = c_black_white;
    gmenu.redraw();
    gmenu.show();

    uilist gpmenu;
    gpmenu.w_width = width;
    gpmenu.w_height = infoHeight - 4;
    gpmenu.w_y = gmenu.w_height;
    gpmenu.w_x = offsetX;
    gpmenu.addentry( pgettext( "map generator", "Regenerate" ) );
    gpmenu.addentry( pgettext( "map generator", "Rotate" ) );
    gpmenu.addentry( pgettext( "map generator", "Apply" ) );
    gpmenu.addentry( pgettext( "map generator", "Change Overmap (Doesn't Apply)" ) );

    gpmenu.input_category = "MAPGEN_PREVIEW";
    gpmenu.additional_actions = {
        { "LEFT", "" },
        { "RIGHT", "" },
        { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
    };
    gpmenu.allow_additional = true;

    int lastsel = gmenu.selected;
    bool showpreview = true;
    do {
        if( gmenu.selected != lastsel ) {
            lastsel = gmenu.selected;
            omt_ref = oter_id( gmenu.selected );
            cleartmpmap( tmpmap );
            tmpmap.generate( tripoint( omt_pos.x * 2, omt_pos.y * 2, target.z ), calendar::turn );
            showpreview = true;
        }
        input_context ctxt( gpmenu.input_category );
        if( use_tiles && showpreview ) {
            const point origin_p = target.xy() + point( 1 - SEEX, 1 - SEEY );
            for( int x = 0; x < SEEX * 2; x++ ) {
                for( int y = 0; y < SEEY * 2; y++ ) {
                    const tripoint tmp_p( x, y, target.z );
                    const tripoint map_p = origin_p + tmp_p;
                    g->draw_terrain_override( map_p, tmpmap.ter( tmp_p ) );
                    g->draw_furniture_override( map_p, tmpmap.furn( tmp_p ) );
                    g->draw_graffiti_override( map_p, tmpmap.has_graffiti_at( tmp_p ) );
                    g->draw_trap_override( map_p, tmpmap.tr_at( tmp_p ).loadid );
                    g->draw_field_override( map_p, tmpmap.field_at( tmp_p ).displayed_field_type() );
                    const maptile &tile = tmpmap.maptile_at( tmp_p );
                    if( tmpmap.sees_some_items( tmp_p, g->u.pos() - origin_p ) ) {
                        const item &itm = tile.get_uppermost_item();
                        const mtype *const mon = itm.get_mtype();
                        g->draw_item_override( map_p, itm.typeId(), mon ? mon->id : mtype_id::NULL_ID(),
                                               tile.get_item_count() > 1 );
                    } else {
                        g->draw_item_override( map_p, "null", mtype_id::NULL_ID(), false );
                    }
                }
            }
        }
        // \u00A0 is the non-breaking space
        update_view_with_help( string_format( pgettext( "keybinding descriptions",
                                              "[%s,%s]\u00A0prev/next oter type, [%s,%s]\u00A0select, %s, %s" ),
                                              ctxt.get_desc( "LEFT", 1 ), ctxt.get_desc( "RIGHT", 1 ),
                                              ctxt.get_desc( "UP", 1 ), ctxt.get_desc( "DOWN", 1 ),
                                              ctxt.describe_key_and_name( "CONFIRM" ),
                                              ctxt.describe_key_and_name( "QUIT" ) ),
                               string_format( pgettext( "map editor state", "Mapgen: %s" ),
                                              oter_id( gmenu.selected ).id().str() ) );
        if( !use_tiles && showpreview ) {
            hilights["mapgentgt"].draw( *this, true );
            g->draw_panels();
            tmpmap.reset_vehicle_cache( target.z );
            for( int x = 0; x < SEEX * 2; x++ ) {
                for( int y = 0; y < SEEY * 2; y++ ) {
                    tmpmap.drawsq( w_preview, g->u, tripoint( x, y, target.z ),
                                   false, true, tripoint( SEEX, SEEY, target.z ), false, true );
                }
            }
            wrefresh( w_preview );
        }
        gmenu.show();
        gpmenu.query( true, BLINK_SPEED * 3 );

        if( gpmenu.ret == UILIST_TIMEOUT ) {
            showpreview = !showpreview;
        } else if( gpmenu.ret != UILIST_ADDITIONAL ) {
            if( gpmenu.ret == 0 ) {
                cleartmpmap( tmpmap );
                tmpmap.generate( tripoint( omt_pos.x * 2, omt_pos.y * 2, target.z ), calendar::turn );
                showpreview = true;
            } else if( gpmenu.ret == 1 ) {
                tmpmap.rotate( 1 );
                showpreview = true;
            } else if( gpmenu.ret == 2 ) {

                point target_sub( target.x / SEEX, target.y / SEEY );

                g->m.set_transparency_cache_dirty( target.z );
                g->m.set_outside_cache_dirty( target.z );
                g->m.set_floor_cache_dirty( target.z );
                g->m.set_pathfinding_cache_dirty( target.z );

                g->m.clear_vehicle_cache( target.z );
                g->m.clear_vehicle_list( target.z );

                for( int x = 0; x < 2; x++ ) {
                    for( int y = 0; y < 2; y++ ) {
                        // Apply previewed mapgen to map. Since this is a function for testing, we try avoid triggering
                        // functions that would alter the results
                        const auto dest_pos = target_sub + tripoint( x, y, target.z );
                        const auto src_pos = tripoint{ x, y, target.z };

                        submap *destsm = g->m.get_submap_at_grid( dest_pos );
                        submap *srcsm = tmpmap.get_submap_at_grid( src_pos );

                        std::swap( *destsm, *srcsm );

                        for( auto &veh : destsm->vehicles ) {
                            veh->sm_pos = dest_pos;
                        }

                        g->m.update_vehicle_list( destsm, target.z ); // update real map's vcaches

                        if( !destsm->spawns.empty() ) {                              // trigger spawnpoints
                            g->m.spawn_monsters( true );
                        }
                    }
                }
                g->m.reset_vehicle_cache( target.z );

            } else if( gpmenu.ret == 3 ) {
                popup( _( "Changed oter_id from '%s' (%s) to '%s' (%s)" ),
                       orig_oters->get_name(), orig_oters.id().str(),
                       omt_ref->get_name(), omt_ref.id().str() );
            }
        } else if( gpmenu.ret == UILIST_ADDITIONAL ) {
            if( gpmenu.ret_act == "LEFT" ) {
                gmenu.scrollby( -1 );
                gmenu.show();
                gmenu.refresh();
            } else if( gpmenu.ret_act == "RIGHT" ) {
                gmenu.scrollby( 1 );
                gmenu.show();
                gmenu.refresh();
            }
        }
    } while( gpmenu.ret != 2 && gpmenu.ret != 3 && gpmenu.ret != UILIST_CANCEL );

    if( gpmenu.ret != 2 &&  // we didn't apply, so restore the original om_ter
        gpmenu.ret != 3 ) { // chose to change oter_id but not apply mapgen
        omt_ref = orig_oters;
    }
    gmenu.border_color = c_magenta;
    gmenu.hilight_color = h_white;
    gmenu.redraw();
    hilights["mapgentgt"].points.clear();
    cleartmpmap( tmpmap );
}

vehicle *editmap::mapgen_veh_query( const tripoint &omt_tgt )
{
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );

    std::vector<vehicle *> possible_vehicles;
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {
            submap *destsm = target_bay.get_submap_at_grid( { x, y, target.z } );
            for( const auto &vehicle : destsm->vehicles ) {
                possible_vehicles.push_back( vehicle.get() );
            }
        }
    }
    if( possible_vehicles.empty() ) {
        popup( _( "Your mechanic could not find a vehicle at the garage." ) );
        return nullptr;
    }

    std::vector<std::string> car_titles;
    car_titles.reserve( possible_vehicles.size() );
    for( auto &elem : possible_vehicles ) {
        car_titles.push_back( elem->name );
    }
    if( car_titles.size() == 1 ) {
        return possible_vehicles[0];
    }
    const int choice = uilist( _( "Select the Vehicle" ), car_titles );
    if( choice >= 0 && static_cast<size_t>( choice ) < possible_vehicles.size() ) {
        return possible_vehicles[choice];
    }
    return nullptr;
}

bool editmap::mapgen_veh_destroy( const tripoint &omt_tgt, vehicle *car_target )
{
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {
            submap *destsm = target_bay.get_submap_at_grid( { x, y, target.z } );
            for( auto &z : destsm->vehicles ) {
                if( z.get() == car_target ) {
                    std::unique_ptr<vehicle> old_veh = target_bay.detach_vehicle( z.get() );
                    g->m.clear_vehicle_cache( omt_tgt.z );
                    g->m.reset_vehicle_cache( omt_tgt.z );
                    g->m.clear_vehicle_list( omt_tgt.z );
                    //Rebuild vehicle_list?
                    return true;
                }
            }
        }
    }
    return false;
}

/*
 * Move mapgen's target, which is different enough from the standard tile edit to warrant it's own function.
 */
void editmap::mapgen_retarget()
{
    input_context ctxt( "EDITMAP_RETARGET" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // Needed for timeout to be useful
    ctxt.register_action( "ANY_INPUT" );
    std::string action;
    tripoint origm = target;

    do {
        update_view_with_help( string_format( pgettext( "keybinding descriptions", "%s, %s" ),
                                              ctxt.describe_key_and_name( "CONFIRM" ),
                                              ctxt.describe_key_and_name( "QUIT" ) ),
                               pgettext( "map generator", "Mapgen: Moving target" ) );
        action = ctxt.handle_input( BLINK_SPEED );
        blink = !blink;
        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            point vec_ms = omt_to_ms_copy( vec->xy() );
            tripoint ptarget = target + vec_ms;
            if( editmap_boundaries.contains_half_open( ptarget ) &&
                editmap_boundaries.contains_half_open( ptarget + point( SEEX, SEEY ) ) ) {
                target = ptarget;

                target_list.clear();
                for( int x = target.x - SEEX - 1; x < target.x + SEEX + 1; x++ ) {
                    for( int y = target.y - SEEY - 1; y < target.y + SEEY + 1; y++ ) {
                        target_list.push_back( tripoint( x, y, target.z ) );
                    }
                }
                blink = true;
            }
        } else {
            blink = !blink;
        }
    } while( action != "QUIT" && action != "CONFIRM" );
    if( action != "CONFIRM" ) {
        target = origm;
    }
    blink = true;
}

/*
 * apply mapgen to a temporary map and overlay over terrain window, optionally regenerating, rotating, and applying to the real in-game map
 */
void editmap::edit_mapgen()
{
    uilist gmenu;
    gmenu.w_width = width;
    gmenu.w_height = TERMY - infoHeight;
    gmenu.w_y = 0;
    gmenu.w_x = offsetX;
    gmenu.input_category = "EDIT_MAPGEN";
    gmenu.additional_actions = {
        { "EDITMAP_MOVE", "" },
        { "HELP_KEYBINDINGS", "" } // to refresh the view after exiting from keybindings
    };
    gmenu.allow_additional = true;

    for( size_t i = 0; i < overmap_terrains::get_all().size(); i++ ) {
        const oter_id id( i );

        gmenu.addentry( -1, !id.id().is_null(), 0, "[%3d] %s", static_cast<int>( id ), id.id().str() );
        gmenu.entries[i].extratxt.left = 1;
        gmenu.entries[i].extratxt.color = id->get_color();
        gmenu.entries[i].extratxt.txt = id->get_symbol();
    }
    real_coords tc;

    do {
        tc.fromabs( g->m.getabs( target.xy() ) );
        point omt_lpos = g->m.getlocal( tc.begin_om_pos() );
        tripoint om_ltarget = omt_lpos + tripoint( -1 + SEEX, -1 + SEEY, target.z );

        if( target.x != om_ltarget.x || target.y != om_ltarget.y ) {
            target = om_ltarget;
            tc.fromabs( g->m.getabs( target.xy() ) );
        }
        target_list.clear();
        for( int x = target.x - SEEX + 1; x < target.x + SEEX + 1; x++ ) {
            for( int y = target.y - SEEY + 1; y < target.y + SEEY + 1; y++ ) {
                if( x == target.x - SEEX + 1 || x == target.x + SEEX ||
                    y == target.y - SEEY + 1 || y == target.y + SEEY ) {
                    target_list.push_back( tripoint( x, y, target.z ) );
                }
            }
        }

        blink = true;
        input_context ctxt( gmenu.input_category );
        update_view_with_help( string_format( pgettext( "keybinding descriptions", "%s, %s, %s" ),
                                              ctxt.describe_key_and_name( "EDITMAP_MOVE" ),
                                              ctxt.describe_key_and_name( "CONFIRM" ),
                                              ctxt.describe_key_and_name( "QUIT" ) ),
                               pgettext( "map generator", "Mapgen stamp" ) );
        gmenu.query();

        if( gmenu.ret > 0 ) {
            mapgen_preview( tc, gmenu );
        } else if( gmenu.ret == UILIST_ADDITIONAL ) {
            if( gmenu.ret_act == "EDITMAP_MOVE" ) {
                mapgen_retarget();
            }
        }
    } while( gmenu.ret != UILIST_CANCEL );
}

/*
 * Special voodoo sauce required to cleanse vehicles and caches to prevent debugmsg loops when re-applying mapgen.
 */
void editmap::cleartmpmap( tinymap &tmpmap )
{
    for( auto &smap : tmpmap.grid ) {
        delete smap;
    }

    auto &ch = tmpmap.get_cache( target.z );
    std::memset( ch.veh_exists_at, 0, sizeof( ch.veh_exists_at ) );
    ch.veh_cached_parts.clear();
    ch.vehicle_list.clear();
    ch.zone_vehicles.clear();
}
