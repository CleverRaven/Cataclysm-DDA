#include "editmap.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "debug_menu.h"
#include "demangle.h"
#include "field.h"
#include "field_type.h"
#include "flexbuffer_json-inl.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "input_enums.h"
#include "input_popup.h"
#include "item.h"
#include "level_cache.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "monster.h"
#include "mtype.h"  // IWYU pragma: keep
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "scent_map.h"
#include "sdltiles.h"
#include "shadowcasting.h"
#include "string_formatter.h"
#include "submap.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "uilist.h"
#include "ui_manager.h"
#include "uistate.h"
#include "vehicle.h"
#include "vpart_position.h"

// NOLINTNEXTLINE(cata-static-int_id-constants)
static const ter_id undefined_ter_id( -1 );

static const ter_str_id ter_t_grave_new( "t_grave_new" );

static editmap_brush &get_brush()
{
    return uistate.editmap_state.brush;
}

namespace io
{
template<>
std::string enum_to_string<editmap_shapetype>( editmap_shapetype data )
{
    switch( data ) {
            // *INDENT-OFF*
        case editmap_shapetype::editmap_point: return _("Point");
        case editmap_shapetype::editmap_rect: return _("Rectangle");
        case editmap_shapetype::editmap_rect_filled: return _("Rectangle, Filled");
        case editmap_shapetype::editmap_line: return _("Line");
        case editmap_shapetype::editmap_circle: return _("Circle");
            // *INDENT-ON*
        default:
            cata_fatal( "Invalid editmap_shapetype in enum_to_string" );
    }
}
template<>
std::string enum_to_string<editmap_action>( editmap_action data )
{
    switch( data ) {
            // *INDENT-OFF*
        case editmap_action::EDIT_ITEMS: return _("EDIT_ITEMS");
        case editmap_action::EDIT_MAPGEN: return _("EDIT_MAPGEN");
        case editmap_action::SELECT_BRUSH: return _("SELECT_BRUSH");
        case editmap_action::SELECT_TERRAIN: return _("SELECT_TERRAIN");
        case editmap_action::SELECT_FURNITURE: return _("SELECT_FURNITURE");
        case editmap_action::SELECT_TRAP: return _("SELECT_TRAP");
        case editmap_action::SELECT_FIELD: return _("SELECT_FIELD");
        case editmap_action::SELECT_RADIATION: return _("SELECT_RADIATION");
            // *INDENT-ON*
        default:
            cata_fatal( "Invalid editmap_action in enum_to_string" );
    }
}
template<>
std::string enum_to_string<editmap_mode>( editmap_mode data )
{
    switch( data ) {
            // *INDENT-OFF*
        case editmap_mode::EMM_DRAWING: return _("EMM_DRAWING");
        case editmap_mode::EMM_SELECTING: return _("EMM_SELECTING");
            // *INDENT-ON*
        default:
            cata_fatal( "Invalid based_on_type in enum_to_string" );
    }
}
} // namespace io

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
static void edit_json( SAVEOBJ &it )
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
                deserialize_from_string( tmp, save1 );
                it = std::move( tmp );
            } catch( const std::exception &err ) {
                // NOLINTNEXTLINE(cata-translate-string-literal)
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
            string_input_popup_imgui popup( 50, save1 );
            std::string ret = popup.query();
            if( !popup.cancelled() ) {
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

bool editmap_ui::get_direction( tripoint_rel_ms &p, const std::string &action,
                                const input_context &ctxt ) const
{
    const editmap_uistate &editmap_state = uistate.editmap_state;
    const tripoint_bub_ms &target = get_brush().target;

    p = tripoint_rel_ms::zero;
    if( action == "CENTER" ) {
        p = get_player_character().pos_bub() - target;
    }

    else if( action == "LEVEL_DOWN" ) {
        p.z() = -1;
    } else if( action == "LEVEL_UP" ) {
        p.z() = 1;
    } else if( action == "SELECT" ) {
        const std::optional<tripoint_bub_ms> mouse_pos = ctxt.get_coordinates(
                    g->w_terrain, g->ter_view_p.raw().xy(), true );
        if( !mouse_pos ) {
            return false;
        }
        p = *mouse_pos - target;
    } else {
        input_context ctxt_iso( "EGET_DIRECTION" );
        ctxt_iso.set_iso( true );
        const std::optional<tripoint_rel_ms> vec = ctxt_iso.get_direction_rel_ms( action );
        if( !vec ) {
            return false;
        }
        p = *vec;
        if( editmap_state.fast_scroll ) {
            p = tripoint_rel_ms( p.x() * 5, p.y() * 5, p.z() );
        }
    }
    return p != tripoint_rel_ms::zero;
}

shared_ptr_fast<game::draw_callback_t> editmap_ui::game_draw_callback_t_container::create_or_get()
{
    shared_ptr_fast<game::draw_callback_t> cb = cbw.lock();
    if( !cb ) {
        cbw = cb = make_shared_fast<game::draw_callback_t>(
        [this]() {
            em->draw_main_ui_overlay();
        } );
        g->add_draw_callback( cb );
    }
    return cb;
}

editmap_ui::game_draw_callback_t_container &editmap_ui::draw_cb_container()
{
    if( !draw_cb_container_ ) {
        draw_cb_container_ = std::make_unique<game_draw_callback_t_container>( this );
    }
    return *draw_cb_container_;
}

input_context editmap_ui::setup_input_context()
{

    input_context ctxt( "EDITMAP" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "CONFIRM" );

    ctxt.register_action( "EDITMAP_SELECT_BRUSH" );
    ctxt.register_action( "EDITMAP_SELECT_TERRAIN" );
    ctxt.register_action( "EDITMAP_SELECT_FURNITURE" );
    ctxt.register_action( "EDITMAP_SELECT_TRAP" );
    ctxt.register_action( "EDITMAP_SELECT_FIELD" );
    ctxt.register_action( "EDITMAP_SELECT_RADIATION" );

    ctxt.register_action( "EDITMAP_ENABLE_TERRAIN" );
    ctxt.register_action( "EDITMAP_ENABLE_FURNITURE" );
    ctxt.register_action( "EDITMAP_ENABLE_TRAP" );
    ctxt.register_action( "EDITMAP_ENABLE_FIELD" );
    ctxt.register_action( "EDITMAP_ENABLE_RADIATION" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );

    ctxt.register_action( "EDITMAP_CHANGE_MODE" );
    ctxt.register_action( "EDITMAP_SWAP_POINTS" );
    ctxt.register_action( "EDITMAP_TOGGLE_ADVANCED_INFO" );
    ctxt.register_action( "EDITMAP_TOGGLE_FAST_SCROLL" );

    ctxt.register_action( "EDITMAP_EDIT_ITEMS" );
    ctxt.register_action( "EDITMAP_EDIT_OVERMAP" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // Needed for timeout to be useful
    ctxt.register_action( "ANY_INPUT" );
    return ctxt;
}


/*
* Shorthand for e.g:

std::optional<ter_id> selected_ter = brush.select_feature<ter_t>();
if (!!selected_ter) {
    brush.selected_terrain = *selected_ter;
    if (!brush.drawing_terrain) {
        brush.drawing_terrain = true;
    }
}

*/
template <typename T_t, typename T_id>
static void brush_set_feature( editmap_brush &brush, T_id &brush_selected, bool &brush_enabled )
{
    std::optional<T_id> selected_ter = brush.select_feature<T_t>();
    if( selected_ter.has_value() ) {
        brush_selected = *selected_ter;
        if( !brush_enabled ) {
            brush_enabled = true;
        }
    }
}

void editmap_ui::handle_action()
{
    editmap_uistate &editmap_state = uistate.editmap_state;
    editmap_brush &brush = get_brush();
    avatar &player_character = get_avatar();
    restore_on_out_of_scope view_offset_prev( player_character.view_offset );
    brush.target = player_character.pos_bub() + player_character.view_offset;

    if( brush.shape_basic_brush == editmap_point ) {
        brush.origin = brush.target;
    }

    brush.update_brush_points();

    std::string action;

    editmap_state.blink = true;

    const int blink_timeout_reset = get_option<int>( "BLINK_SPEED" ) / 4;
    int blink_timeout = blink_timeout_reset;

    shared_ptr_fast<game::draw_callback_t> editmap_cb = draw_cb_container().create_or_get();
    on_out_of_scope invalidate_current_ui( [this]() {
        do_ui_invalidation();
    } );

    bool invalidate_ui = false;
    do {
        invalidate_ui = true;

        if( brush.shape_basic_brush == editmap_point ) {
            uistate.editmap_state.mode = EMM_DRAWING;
        }

        ui_manager::redraw();

        action = ictxt.handle_input( 33 );

        std::optional<editmap_action> selected_action;
        for( int i = 0; i < SELECTABLE_ACTIONS; i++ ) {
            if( editmap_state.selected[i] ) {
                editmap_state.selected[i] = false;
                selected_action = static_cast<editmap_action>( i );
            }
        }
        if( !selected_action ) {
            if( action == "EDITMAP_EDIT_ITEMS" ) {
                selected_action = EDIT_ITEMS;
            } else if( action == "EDITMAP_EDIT_OVERMAP" ) {
                selected_action = EDIT_MAPGEN;
            } else if( action == "EDITMAP_SELECT_BRUSH" ) {
                selected_action = SELECT_BRUSH;
            } else if( action == "EDITMAP_SELECT_TERRAIN" ) {
                selected_action = SELECT_TERRAIN;
            } else if( action == "EDITMAP_SELECT_FURNITURE" ) {
                selected_action = SELECT_FURNITURE;
            } else if( action == "EDITMAP_SELECT_TRAP" ) {
                selected_action = SELECT_TRAP;
            } else if( action == "EDITMAP_SELECT_FIELD" ) {
                selected_action = SELECT_FIELD;
            } else if( action == "EDITMAP_SELECT_RADIATION" ) {
                selected_action = SELECT_RADIATION;
            }
        }
        if( move_target( action, ictxt, editmap_state.mode == EMM_DRAWING ) ) {
            brush.update_brush_points();
        } else if( action == "EDITMAP_ENABLE_TERRAIN" ) {
            brush.drawing_terrain = !brush.drawing_terrain;
        } else if( action == "EDITMAP_ENABLE_FURNITURE" ) {
            brush.drawing_furniture = !brush.drawing_furniture;
        } else if( action == "EDITMAP_ENABLE_TRAP" ) {
            brush.drawing_trap = !brush.drawing_trap;
        } else if( action == "EDITMAP_ENABLE_FIELD" ) {
            brush.drawing_field = !brush.drawing_field;
        } else if( action == "EDITMAP_ENABLE_RADIATION" ) {
            brush.drawing_radiation = !brush.drawing_radiation;
        } else if( action == "EDITMAP_CHANGE_MODE" ) {
            if( brush.shape_basic_brush != editmap_point ) {
                int new_mode = static_cast<int>( editmap_state.mode );
                new_mode++;
                if( new_mode == static_cast<int>( editmap_mode::EMM_LAST ) ) {
                    new_mode = 0;
                }
                editmap_mode new_mode_enum = static_cast<editmap_mode>( new_mode );
                editmap_state.mode = new_mode_enum;
            } else {
                uistate.editmap_state.mode = EMM_DRAWING;
            }
        } else if( action == "EDITMAP_TOGGLE_ADVANCED_INFO" ) {
            editmap_state.advanced_info_toggle = !editmap_state.advanced_info_toggle;
        } else if( action == "EDITMAP_TOGGLE_FAST_SCROLL" ) {
            editmap_state.fast_scroll = !editmap_state.fast_scroll;
        } else if( action == "EDITMAP_SWAP_POINTS" ) {
            tripoint_bub_ms tmporigin = brush.origin;
            brush.origin = brush.target;
            brush.target = tmporigin;
            brush.update_brush_points();
        } else if( action == "CONFIRM" ) {
            switch( editmap_state.mode ) {
                case EMM_SELECTING:
                    editmap_state.mode = EMM_DRAWING;
                    break;
                case EMM_DRAWING:
                    brush_apply();
                    break;
                default:
                    debugmsg( "invalid editmap mode" );
            }
        } else if( !!selected_action ) {
            switch( *selected_action ) {
                case EDIT_ITEMS:
                    edit_items();
                    break;
                case EDIT_MAPGEN:
                    edit_mapgen();
                    break;
                case SELECT_BRUSH:
                    brush.select_shape();
                    break;
                case SELECT_TERRAIN:
                    brush_set_feature<ter_t, ter_id>( brush, brush.selected_terrain, brush.drawing_terrain );
                    break;
                case SELECT_FURNITURE:
                    brush_set_feature<furn_t, furn_id>( brush, brush.selected_furniture, brush.drawing_furniture );
                    break;
                case SELECT_TRAP:
                    brush_set_feature<trap, trap_id>( brush, brush.selected_trap, brush.drawing_trap );
                    break;
                case SELECT_FIELD: {
                    std::optional<field_type_id> selected_fd = brush.select_field();
                    if( !!selected_fd ) {
                        brush.selected_field = *selected_fd;
                        if( !brush.drawing_field ) {
                            brush.drawing_field = true;
                        }
                    }
                    break;
                }
                case SELECT_RADIATION:
                    brush.selected_radiation = brush.select_radiation();
                    if( !brush.drawing_radiation ) {
                        brush.drawing_radiation = true;
                    }
                    break;
                default:
                    debugmsg( "invalid editmap selection" );
            }
        } else {
            invalidate_ui = false;
        }

        blink_timeout--;
        if( blink_timeout == 0 ) {
            blink_timeout = blink_timeout_reset;
            editmap_state.blink = !editmap_state.blink;
            invalidate_ui = true;
        }

        if( invalidate_ui ) {
            do_ui_invalidation();
        }

    } while( action != "QUIT" );
    editmap_state.blink = false;
}

void editmap_ui::do_ui_invalidation()
{
    avatar &player_character = get_avatar();
    const editmap_brush &brush = get_brush();
    player_character.view_offset = brush.target - player_character.pos_bub();
    g->invalidate_main_ui_adaptor();
}

void editmap_ui::draw_main_ui_overlay()
{
    editmap_uistate &editmap_state = uistate.editmap_state;
    const editmap_brush &brush = get_brush();
    const tripoint_bub_ms &target = brush.target;
    const bool blink = editmap_state.blink;

    const Creature *critter = get_creature_tracker().creature_at( target );
    map &here = get_map();

    // update target point
    if( critter != nullptr ) {
        critter->draw( g->w_terrain, target, true );
    } else {
        here.drawsq( g->w_terrain, target, drawsq_params().highlight( true ).center( target ) );
    }

    if( blink ) {
        for( const tripoint_bub_ms &p : brush.brush_points ) {
            if( brush.drawing_terrain ) {
                g->draw_terrain_override( p, brush.selected_terrain );
            }
            if( brush.drawing_furniture ) {
                g->draw_furniture_override( p, brush.selected_furniture );
            }
            if( brush.drawing_trap ) {
                g->draw_trap_override( p, brush.selected_trap );
            }
            if( brush.drawing_field ) {
                g->draw_field_override( p, brush.selected_field );
            }

            g->draw_cursor( p );
        }
    }
    if( editmap_state.tmpmap_ptr ) {
        tinymap &tmpmap = *editmap_state.tmpmap_ptr;
#ifdef TILES
        if( use_tiles ) {
            const tripoint_bub_ms player_location = get_player_character().pos_bub();
            const point_bub_ms origin_p = target.xy() + point( 1 - SEEX, 1 - SEEY );
            for( int x = 0; x < SEEX * 2; x++ ) {
                for( int y = 0; y < SEEY * 2; y++ ) {
                    const tripoint_omt_ms tmp_p( x, y, target.z() );
                    const tripoint_bub_ms map_p = origin_p + tmp_p.raw();
                    g->draw_radiation_override( tripoint_bub_ms( map_p ), tmpmap.get_radiation( tmp_p ) );
                    // scent is managed in `game` instead of `map`, so there's no override for it
                    // temperature is managed in `game` instead of `map`, so there's no override for it
                    // TODO: visibility could be affected by both the actual map and the preview map,
                    // which complicates calculation, so there's no override for it (yet)
                    g->draw_terrain_override( map_p, tmpmap.ter( tmp_p ) );
                    g->draw_furniture_override( map_p, tmpmap.furn( tmp_p ) );
                    g->draw_graffiti_override( tripoint_bub_ms( map_p ), tmpmap.has_graffiti_at( tmp_p ) );
                    g->draw_trap_override( map_p, tmpmap.tr_at( tmp_p ).loadid );
                    g->draw_field_override( map_p, tmpmap.field_at( tmp_p ).displayed_field_type() );
                    const maptile &tile = tmpmap.maptile_at( tmp_p );
                    if( tmpmap.sees_some_items( tmp_p, tripoint_omt_ms( ( player_location - origin_p ).raw() ) ) ) {
                        const item &itm = tile.get_uppermost_item();
                        const mtype *const mon = itm.get_mtype();
                        g->draw_item_override( map_p, itm.typeId(), mon ? mon->id : mtype_id::NULL_ID(),
                                               tile.get_item_count() > 1 );
                    } else {
                        g->draw_item_override( map_p, itype_id::NULL_ID(), mtype_id::NULL_ID(),
                                               false );
                    }
                    if( const optional_vpart_position ovp = tmpmap.veh_at( tmp_p ) ) {
                        const vpart_display vd = ovp->vehicle().get_display_of_tile( ovp->mount_pos() );
                        char part_mod = 0;
                        if( vd.is_open ) {
                            part_mod = 1;
                        } else if( vd.is_broken ) {
                            part_mod = 2;
                        }
                        const units::angle veh_dir = ovp->vehicle().face.dir();
                        g->draw_vpart_override( map_p, vpart_id( vd.id ), part_mod, veh_dir, vd.has_cargo,
                                                ovp->mount_pos() );
                    } else {
                        g->draw_vpart_override( map_p, vpart_id::NULL_ID(), 0, 0_degrees, false, point_rel_ms::zero );
                    }
                }
            }
            // int: count, bool: more than 1 spawn data
            std::map<tripoint_bub_ms, std::tuple<mtype_id, int, bool, Creature::Attitude>> spawns;
            for( int x = 0; x < 2; x++ ) {
                for( int y = 0; y < 2; y++ ) {
                    submap *sm = tmpmap.get_submap_at_grid( tripoint_rel_sm{ x, y, target.z()} );
                    if( sm ) {
                        const tripoint_bub_ms sm_origin = origin_p + tripoint( x * SEEX, y * SEEY, target.z() );
                        for( const spawn_point &sp : sm->spawns ) {
                            const tripoint_bub_ms spawn_p = sm_origin + rebase_rel( sp.pos );
                            const auto spawn_it = spawns.find( spawn_p );
                            if( spawn_it == spawns.end() ) {
                                const Creature::Attitude att = sp.friendly ? Creature::Attitude::FRIENDLY : Creature::Attitude::ANY;
                                spawns.emplace( spawn_p, std::make_tuple( sp.type, sp.count, false, att ) );
                            } else {
                                std::get<2>( spawn_it->second ) = true;
                            }
                        }
                    }
                }
            }
            for( const auto &it : spawns ) {
                g->draw_monster_override( tripoint_bub_ms( it.first ), std::get<0>( it.second ),
                                          std::get<1>( it.second ),
                                          std::get<2>( it.second ), std::get<3>( it.second ) );
            }
        } else {
#endif
            drawsq_params params = drawsq_params().center( tripoint_bub_ms( SEEX - 1, SEEY - 1, target.z() ) );
            for( const tripoint_omt_ms &p : tmpmap.points_on_zlevel() ) {
                tmpmap.drawsq( g->w_terrain, rebase_bub( p ), params );
            }
            tmpmap.rebuild_vehicle_level_caches();
#ifdef TILES
        }
#endif
    }
}

void editmap_ui::draw_current_point_info()
{
    const editmap_uistate &editmap_state = uistate.editmap_state;
    const editmap_brush &brush = get_brush();
    const tripoint_bub_ms &current_point = brush.target;

    const nc_color draw_color = c_light_gray;

    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( current_point );
    std::string veh_msg;
    if( !vp ) {
        veh_msg = pgettext( "vehicle", "none" );
    } else if( vp->is_inside() ) {
        veh_msg = pgettext( "vehicle", "in" );
    } else {
        veh_msg = pgettext( "vehicle", "out" );
    }

    const ter_id &t = here.ter( current_point );
    const ter_t &terrain_type = t.obj();
    const furn_t &furniture_type = here.furn( current_point ).obj();

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    cataimgui::TextColoredParagraphNewline( draw_color,
                                            string_format( _( "Target: bub_ms = %s; abs_ms = %s" ), current_point.to_string(),
                                                    here.get_abs( current_point ).to_string() ) );
    /* uncomment for origin
    cataimgui::TextColoredParagraphNewline( draw_color,
                                            string_format( "Origin: bub_ms = %s; abs_ms = %s", current_origin.to_string(),
                                                    here.get_abs( current_origin ).to_string() ) );
    */
    ImGui::Separator();
    draw_symbol_and_info( terrain_type.color(), terrain_type.symbol(), draw_color,
                          string_format( _( " Terrain: %s (#%d); move cost %d" ),
                                         terrain_type.id.str(), t.to_i(), terrain_type.movecost ) );

    const furn_id &f = here.furn( current_point );
    if( f.to_i() > 0 ) {
        draw_symbol_and_info( furniture_type.color(), furniture_type.symbol(), draw_color,
                              string_format( _( " Furniture: %s (#%d); move cost %d; move strength %d" ),
                                             furniture_type.id.str(), f.to_i(), furniture_type.movecost, furniture_type.move_str_req ) );
    }

    const trap &cur_trap = here.tr_at( current_point );
    if( !cur_trap.is_null() ) {
        draw_symbol_and_info( cur_trap.color, cur_trap.sym, draw_color,
                              string_format( _( "Trap: %s (#%d)" ), cur_trap.id.str(), cur_trap.loadid.to_i() ) );
    }

    for( auto &fld : here.get_field( current_point ) ) {
        const field_entry &cur = fld.second;
        draw_symbol_and_info( cur.color(), cur.symbol().front(), draw_color,
                              string_format( _( "Field: %s (#%d); Intensity: %d (%s); Age: %d" ),
                                             cur.get_field_type().id().str(), cur.get_field_type().to_i(),
                                             cur.get_field_intensity(), cur.name(), to_turns<int>( cur.get_field_age() ) ) );
    }

    if( ImGui::CollapsingHeader( "Advanced Info" ) || editmap_state.advanced_info_toggle ) {
        const level_cache &map_cache = here.get_cache( current_point.z() );

        Character &player_character = get_player_character();
        const std::string u_see_msg = player_character.sees( here, current_point ) ? _( "yes" ) : _( "no" );
        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( _( "distance: %d; u_see: %s; vehicle: %s; scent: %d" ),
                                                        rl_dist( player_character.pos_bub(), current_point ), u_see_msg, veh_msg,
                                                        get_scent().get( current_point ) ) );
        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( _( "sight_range: %d; noon_sight_range: %d;" ),
                                                        player_character.sight_range( g->light_level( player_character.posz() ) ),
                                                        player_character.sight_range( sun_moon_light_at_noon_near( calendar::turn ) ) ) );
        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( _( "cache{transp:%.4f; seen:%.4f; cam:%.4f}" ),
                                                        map_cache.transparency_cache[current_point.x()][current_point.y()],
                                                        map_cache.seen_cache[current_point.x()][current_point.y()],
                                                        map_cache.camera_cache[current_point.x()][current_point.y()]
                                                             ) );
        map::apparent_light_info al = map::apparent_light_helper( map_cache,
                                      tripoint_bub_ms( current_point ) );
        int apparent_light = static_cast<int>(
                                 here.apparent_light_at( current_point, here.get_visibility_variables_cache() ) );
        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( _( "outside: %d; obstructed: %d; floor: %d" ),
                                                        static_cast<int>( here.is_outside( current_point ) ),
                                                        static_cast<int>( al.obstructed ),
                                                        static_cast<int>( here.has_floor( current_point ) )
                                                             ) );
        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( _( "light_at: %s; apparent light: %.5f (%d)" ),
                                                        map_cache.lm[current_point.x()][current_point.y()].to_string(),
                                                        al.apparent_light, apparent_light ) );
        std::string extras;
        if( vp ) {
            extras += _( " [vehicle]" );
        }
        if( here.has_flag( ter_furn_flag::TFLAG_INDOORS, current_point ) ) {
            extras += _( " [indoors]" );
        }
        if( here.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, current_point ) ) {
            extras += _( " [roof]" );
        }

        cataimgui::TextColoredParagraphNewline( draw_color,
                                                string_format( "%s, %s", here.features( current_point ), extras ) );

        if( here.has_graffiti_at( current_point ) ) {
            cataimgui::TextColoredParagraphNewline( draw_color, string_format(
                    here.ter( current_point ) == ter_t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                    here.graffiti_at( current_point ) ) );
        }
    }
}

static ter_id get_alt_ter( bool isvert, ter_id sel_ter )
{
    std::map<std::string, std::string> alts;
    alts["_v"] = "_h";
    alts["_vertical"] = "_horizontal";
    alts["_v_alarm"] = "_h_alarm";
    const std::string tersid = sel_ter.obj().id.str();
    const int sidlen = tersid.size();
    for( const auto &it : alts ) {
        const std::string suffix = isvert ? it.first : it.second;
        const int slen = suffix.size();
        if( sidlen > slen && tersid.substr( sidlen - slen, slen ) == suffix ) {
            const std::string asuffix = isvert ? it.second : it.first;
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

template<>
int symbol( const field_type &t )
{
    return static_cast<int>( t.get_symbol().front() );
}

template<typename T_t>
static std::string name( const T_t &t );

template<>
std::string name( const ter_t &t )
{
    return t.name();
}

template<>
std::string name( const furn_t &t )
{
    return t.name();
}

template<>
std::string name( const trap &t )
{
    return t.name();
}

template<>
std::string name( const field_type &t )
{
    return t.get_name();
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

template<>
nc_color color( const field_type &t )
{
    return t.get_intensity_level().color;
}

template<typename T_t>
static std::string describe( const T_t &t );

template<>
std::string describe( const ter_t &type )
{
    return string_format( _( "Move cost: %d\nIndoors: %s\nRoof: %s" ), type.movecost,
                          type.has_flag( ter_furn_flag::TFLAG_INDOORS ) ? _( "Yes" ) : _( "No" ),
                          type.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF ) ? _( "Yes" ) : _( "No" ) );
}

template<>
std::string describe( const furn_t &type )
{
    return string_format( _( "Move cost: %d\nIndoors: %s\nRoof: %s" ), type.movecost,
                          type.has_flag( ter_furn_flag::TFLAG_INDOORS ) ? _( "Yes" ) : _( "No" ),
                          type.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF ) ? _( "Yes" ) : _( "No" ) );
}

template<>
std::string describe( const trap &type )
{
    return type.debug_describe();
}

template<>
std::string describe( const field_type & )
{
    return std::string();
}

template<typename T_t>
static void apply( const T_t &t, const editmap_brush &brush );

template<>
void apply<ter_t>( const ter_t &t, const editmap_brush &brush )
{
    bool isvert = false;
    bool doalt = false;
    ter_id teralt = undefined_ter_id;
    int alta = -1;
    int altb = -1;
    const ter_id &sel_ter = t.id.id();
    if( brush.shape_basic_brush == editmap_rect ) {
        if( t.symbol() == LINE_XOXO || t.symbol() == '|' ) {
            isvert = true;
            teralt = get_alt_ter( isvert, sel_ter );
        } else if( t.symbol() == LINE_OXOX || t.symbol() == '-' ) {
            teralt = get_alt_ter( isvert, sel_ter );
        }
        if( teralt != undefined_ter_id ) {
            if( isvert ) {
                alta = brush.target.y();
                altb = brush.origin.y();
            } else {
                alta = brush.target.x();
                altb = brush.origin.x();
            }
            doalt = true;
        }
    }

    map &here = get_map();
    for( const tripoint_bub_ms &elem : brush.brush_points ) {
        ter_id wter = sel_ter;
        if( doalt ) {
            int coord = isvert ? elem.y() : elem.x();
            if( coord == alta || coord == altb ) {
                wter = teralt;
            }
        }
        here.ter_set( elem, wter );
    }
}

template<>
void apply<furn_t>( const furn_t &t, const editmap_brush &brush )
{
    const furn_id &sel_frn = t.id.id();
    map &here = get_map();
    for( const tripoint_bub_ms &elem : brush.brush_points ) {
        here.furn_set( elem, sel_frn );
    }
}

template<>
void apply<trap>( const trap &t, const editmap_brush &brush )
{
    map &here = get_map();
    for( const tripoint_bub_ms &elem : brush.brush_points ) {
        here.trap_set( elem, t.loadid );
    }
}

template<>
void apply<field_type>( const field_type &t, const editmap_brush &brush )
{
    map &here = get_map();
    for( const tripoint_bub_ms &elem : brush.brush_points ) {
        here.set_field_intensity( elem, t.id.id(), brush.selected_field_intensity );
    }
}

// edit terrain, furnitrue, or traps
template<typename T_t>
std::optional<int_id<T_t>> editmap_brush::select_feature()
{
    using T_id = decltype( T_t().id.id() );

    if( T_t::count() == 0 ) {
        debugmsg( "Empty %s list", demangle( typeid( T_t ).name() ) );
        return std::nullopt;
    }

    uilist emenu;
    emenu.desc_enabled = true;
    emenu.input_category = "EDITMAP_FEATURE";

    for( int i = 0; i < static_cast<int>( T_t::count() ); ++i ) {
        const T_t &type = T_id( i ).obj();
        std::string feature_name;
        if( name( type ).empty() ) {
            feature_name = string_format( pgettext( "map feature id", "(%s)" ), type.id.str() );
        } else {
            feature_name = string_format( pgettext( "map feature name and id", "%s (%s)" ), name( type ),
                                          type.id.str() );
        }
        uilist_entry ent( feature_name );
        ent.retval = i;
        ent.enabled = true;
        ent.hotkey = input_event();
        ent.extratxt.sym = symbol( type );
        ent.extratxt.color = color( type );
        ent.desc = describe( type );

        emenu.entries.emplace_back( ent );
    }

    emenu.query();
    do {
        if( emenu.ret > MENU_AUTOASSIGN ) {
            return T_id( emenu.ret );
        }
    } while( emenu.ret != UILIST_CANCEL );

    return std::nullopt;
}

/*
 * edit items in target square. WIP
 */
enum editmap_imenu_ent {
    imenu_bday,
    imenu_damage,
    imenu_degradation,
    imenu_burnt,
    imenu_tags,
    imenu_sep,
    imenu_savetest,
    imenu_exit,
};

void editmap_ui::edit_items()
{
    const editmap_brush &brush = get_brush();

    uilist ilmenu;
    map_stack items = get_map().i_at( brush.target );
    int i = 0;
    for( item &an_item : items ) {
        ilmenu.addentry( i++, true, 0, "%s%s", an_item.tname(),
                         an_item.is_emissive() ? " L" : "" );
    }
    ilmenu.addentry( items.size(), true, 'a', _( "Add item" ) );
    ilmenu.input_category = "EDIT_ITEMS";
    ilmenu.additional_actions = {
        { "HELP_KEYBINDINGS", translation() } // to refresh the view after exiting from keybindings
    };
    ilmenu.allow_additional = true;

    shared_ptr_fast<uilist_impl> ilmenu_ui = ilmenu.create_or_get_ui();

    do {
        ilmenu.query();
        if( ilmenu.ret >= 0 && ilmenu.ret < static_cast<int>( items.size() ) ) {
            item &it = *items.get_iterator_from_index( ilmenu.ret );
            uilist imenu;
            imenu.addentry( imenu_bday, true, -1, pgettext( "item manipulation debug menu entry", "bday: %d" ),
                            to_turn<int>( it.birthday() ) );
            imenu.addentry( imenu_damage, true, -1, pgettext( "item manipulation debug menu entry",
                            "damage: %d" ), it.damage() );
            imenu.addentry( imenu_degradation, true, -1, pgettext( "item manipulation debug menu entry",
                            "degradation: %d" ), it.degradation() );
            imenu.addentry( imenu_burnt, true, -1, pgettext( "item manipulation debug menu entry",
                            "burnt: %d" ), static_cast<int>( it.burnt ) );
            imenu.addentry( imenu_tags, true, -1, pgettext( "item manipulation debug menu entry",
                            "tags: %s" ), debug_menu::iterable_to_string( it.get_flags(), " ",
            []( const flag_id & f ) {
                return f.str();
            } ) );
            imenu.addentry( imenu_sep, false, 0, pgettext( "item manipulation debug menu entry",
                            "-[ light emission ]-" ) );
            imenu.addentry( imenu_savetest, true, -1, pgettext( "item manipulation debug menu entry",
                            "savetest" ) );
            imenu.input_category = "EDIT_ITEMS";
            imenu.additional_actions = {
                { "HELP_KEYBINDINGS", translation() } // to refresh the view after exiting from keybindings
            };
            imenu.allow_additional = true;

            shared_ptr_fast<uilist_impl> imenu_ui = imenu.create_or_get_ui();

            do {
                imenu.query();
                if( imenu.ret >= 0 && imenu.ret < imenu_savetest ) {
                    int intval = -1;
                    std::string strval;
                    switch( imenu.ret ) {
                        case imenu_bday:
                            intval = to_turn<int>( it.birthday() );
                            break;
                        case imenu_damage:
                            intval = it.damage();
                            break;
                        case imenu_degradation:
                            intval = it.degradation();
                            break;
                        case imenu_burnt:
                            intval = static_cast<int>( it.burnt );
                            break;
                        case imenu_tags:
                            strval = debug_menu::iterable_to_string( it.get_flags(), " ",
                            []( const flag_id & f ) {
                                return f.str();
                            } );
                            break;
                    }
                    int retval = 0;
                    bool confirmed = false;
                    if( imenu.ret < imenu_tags ) {
                        retval = intval;
                        confirmed = query_int( retval, true, "set:" );
                    } else if( imenu.ret == imenu_tags ) {
                        string_input_popup_imgui popup( 50, strval, _( "Flags:" ) );
                        popup.set_description( "UPPERCASE, no quotes, separate with spaces:" );
                        strval = popup.query();
                        confirmed = !popup.cancelled();
                    }
                    if( confirmed ) {
                        switch( imenu.ret ) {
                            case imenu_bday:
                                it.set_birthday( time_point::from_turn( retval ) );
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_bday].txt = string_format( "bday: %d", to_turn<int>( it.birthday() ) );
                                break;
                            case imenu_damage:
                                it.set_damage( retval );
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_damage].txt = string_format( "damage: %d", it.damage() );
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_degradation].txt = string_format( "degradation: %d", it.degradation() );
                                break;
                            case imenu_degradation:
                                it.set_degradation( retval );
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_degradation].txt = string_format( "degradation: %d", it.degradation() );
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_damage].txt = string_format( "damage: %d", it.damage() );
                                break;
                            case imenu_burnt:
                                it.burnt = retval;
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_burnt].txt = string_format( "burnt: %d", it.burnt );
                                break;
                            case imenu_tags:
                                const auto tags = debug_menu::string_to_iterable<std::vector<std::string>>( strval, " " );
                                it.unset_flags();
                                for( const auto &t : tags ) {
                                    it.set_flag( flag_id( t ) );
                                }
                                // NOLINTNEXTLINE(cata-translate-string-literal)
                                imenu.entries[imenu_tags].txt = string_format( "tags: %s",
                                                                debug_menu::iterable_to_string( it.get_flags(), " ",
                                []( const flag_id & f ) {
                                    return f.str();
                                } ) );
                                break;
                        }
                    }
                } else if( imenu.ret == imenu_savetest ) {
                    edit_json( it );
                }
            } while( imenu.ret != UILIST_CANCEL );
        } else if( ilmenu.ret == static_cast<int>( items.size() ) ) {
            debug_menu::wishitem( nullptr, brush.target );
            ilmenu.entries.clear();
            i = 0;
            for( item &an_item : items ) {
                ilmenu.addentry( i++, true, 0, "%s%s", an_item.tname(),
                                 an_item.is_emissive() ? " L" : "" );
            }
            ilmenu.addentry( items.size(), true, 'a',
                             pgettext( "item manipulation debug menu entry for adding an item on a tile", "Add item" ) );
            ilmenu.setup();
            ilmenu.filterlist();
        }
    } while( ilmenu.ret != UILIST_CANCEL );
}

void editmap_brush::update_brush_points()
{
    const int z = target.z();

    brush_points.clear();
    switch( shape_basic_brush ) {
        case editmap_point:
            origin = target;
            brush_points = { origin };
            break;
        case editmap_circle: {
            int radius = rl_dist( origin, target );
            map &here = get_map();
            for( const tripoint_bub_ms &p : here.points_in_radius( origin, radius ) ) {
                if( rl_dist( p, origin ) <= radius ) {
                    if( here.inbounds( p ) ) {
                        brush_points.push_back( p );
                    }
                }
            }
        }
        break;
        case editmap_rect_filled:
        case editmap_rect: {
            point s;
            point e;
            if( target.x() < origin.x() ) {
                s.x = target.x();
                e.x = origin.x();
            } else {
                s.x = origin.x();
                e.x = target.x();
            }
            if( target.y() < origin.y() ) {
                s.y = target.y();
                e.y = origin.y();
            } else {
                s.y = origin.y();
                e.y = target.y();
            }
            for( int x = s.x; x <= e.x; x++ ) {
                for( int y = s.y; y <= e.y; y++ ) {
                    if( shape_basic_brush == editmap_rect_filled || x == s.x || x == e.x || y == s.y || y == e.y ) {
                        const tripoint_bub_ms p( x, y, z );
                        if( get_map().inbounds( p ) ) {
                            brush_points.push_back( p );
                        }
                    }
                }
            }
        }
        break;
        case editmap_line:
            brush_points = line_to( origin, target );
            break;
        default:
            debugmsg( "invalid editmap brush shape" );
    }
}

/*
 * Shift 'var' (i.e., part of a coordinate plane) by 'shift'.
 * If the result is not >= min and < 'max', constrain the result and adjust 'shift',
 * so it can adjust subsequent points of a set consistently.
 */
static int limited_shift( int var, int &shift, int min, int max )
{
    if( var + shift < min ) {
        shift = min - var;
    } else if( var + shift >= max ) {
        shift = max - 1 - var;
    }
    return var + shift;
}

bool editmap_ui::move_target( const std::string &action, const input_context &ctxt,
                              bool moveorigin ) const
{
    editmap_uistate &editmap_state = uistate.editmap_state;
    editmap_brush &brush = editmap_state.brush;
    tripoint_bub_ms &target = brush.target;
    tripoint_rel_ms mp;

    if( !get_direction( mp, action, ctxt ) ) {
        return false;
    }

    target.x() = limited_shift( target.x(), mp.x(), 0, MAPSIZE_X );
    target.y() = limited_shift( target.y(), mp.y(), 0, MAPSIZE_Y );
    // OVERMAP_HEIGHT is the limit, not size of a 0 based vector, and limited_shift restricts to <, not <=
    target.z() = limited_shift( target.z(), mp.z(), -OVERMAP_DEPTH, OVERMAP_HEIGHT + 1 );
    if( moveorigin ) {
        brush.origin += mp;
    }
    return true;
}

void editmap_brush::select_shape()
{
    uilist smenu;
    smenu.text = _( "Brush Shape" );
    smenu.addentry( editmap_point, true, 'p', io::enum_to_string( editmap_point ) );
    smenu.addentry( editmap_rect, true, 'r', io::enum_to_string( editmap_rect ) );
    smenu.addentry( editmap_rect_filled, true, 'f', io::enum_to_string( editmap_rect_filled ) );
    smenu.addentry( editmap_line, true, 'l', io::enum_to_string( editmap_line ) );
    smenu.addentry( editmap_circle, true, 'c', io::enum_to_string( editmap_circle ) );
    smenu.selected = static_cast<int>( shape_basic_brush );
    do {
        smenu.query();
        if( smenu.ret > MENU_AUTOASSIGN ) {
            shape_basic_brush = static_cast<editmap_shapetype>( smenu.ret );
            update_brush_points();
            break;
        }
    } while( smenu.ret != UILIST_CANCEL );
}

void editmap_ui::mapgen_preview( const real_coords &tc, oter_id previewed_omt ) const
{
    editmap_uistate &editmap_state = uistate.editmap_state;
    editmap_brush &brush = get_brush();
    tripoint_bub_ms &target = brush.target;
    // Coordinates of the overmap terrain that should be generated.
    const point_abs_omt omt_pos2 = tc.abs_omt();
    const tripoint_abs_omt omt_pos( omt_pos2, target.z() );
    const oter_id &omt_ref = overmap_buffer.ter( omt_pos );
    // Copy to store the original value, to restore it upon canceling
    const oter_id orig_oters = omt_ref;
    overmap_buffer.ter_set( omt_pos, previewed_omt );
    smallmap tmpmap;
    tmpmap.generate( omt_pos, calendar::turn, false, editmap_state.run_post_process );

    uilist gpmenu;

    auto set_postprocess_text = [&gpmenu]( bool toggle_state ) {
        gpmenu.entries[static_cast<int>( OMTM_TOGGLE_POSTPROCESS )].txt =
            string_format( "Toggle post-process generators (Currently %s)",
                           toggle_state ? _( "ON" ) : _( "OFF" ) );
    };

    gpmenu.addentry( static_cast<int>( OMTM_REGENERATE ), true, std::nullopt, _( "Regenerate" ) );
    gpmenu.addentry( static_cast<int>( OMTM_ROTATE ), true, std::nullopt, _( "Rotate" ) );
    gpmenu.addentry( static_cast<int>( OMTM_APPLY ), true, std::nullopt, _( "Apply" ) );
    gpmenu.addentry( static_cast<int>( OMTM_CHANGE_OTER_ID ), true, std::nullopt,
                     _( "Change oter_id (No mapgen changes)" ) );
    gpmenu.addentry( static_cast<int>( OMTM_TOGGLE_POSTPROCESS ), true, std::nullopt, "" );

    gpmenu.input_category = "MAPGEN_PREVIEW";

    gpmenu.allow_additional = true;
    gpmenu.desired_bounds = { 0.0f, -1.0f, -1.0f, -1.0f };

    map &here = get_map();

    bool showpreview = true;
    bool changed_oter_id = false;
    do {
        set_postprocess_text( editmap_state.run_post_process );
        if( showpreview ) {
            editmap_state.tmpmap_ptr = &tmpmap;
        } else {
            editmap_state.tmpmap_ptr = nullptr;
        }
        input_context ctxt( gpmenu.input_category, keyboard_mode::keycode );

        gpmenu.query( get_option<int>( "BLINK_SPEED" ) );

        changed_oter_id = gpmenu.ret == OMTM_APPLY || gpmenu.ret == OMTM_CHANGE_OTER_ID;

        switch( gpmenu.ret ) {
            case OMTM_REGENERATE:
                cleartmpmap( tmpmap );
                tmpmap.generate( omt_pos, calendar::turn, false, editmap_state.run_post_process );
                break;
            case OMTM_ROTATE:
                tmpmap.rotate( 1 );
                break;
            case OMTM_APPLY: {
                const point_rel_sm target_sub( target.x() / SEEX, target.y() / SEEY );

                here.set_transparency_cache_dirty( target.z() );
                here.set_outside_cache_dirty( target.z() );
                here.set_floor_cache_dirty( target.z() );
                here.set_pathfinding_cache_dirty( target.z() );

                here.clear_vehicle_level_caches();
                here.clear_vehicle_list( target.z() );

                for( int x = 0; x < 2; x++ ) {
                    for( int y = 0; y < 2; y++ ) {
                        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                            // Apply previewed mapgen to map. Since this is a function for testing, we try avoid triggering
                            // functions that would alter the results
                            const tripoint_rel_sm dest_pos = target_sub + tripoint( x, y, z );
                            const tripoint_rel_sm src_pos = tripoint_rel_sm{ x, y, z };

                            submap *destsm = here.get_submap_at_grid( dest_pos );
                            submap *srcsm = tmpmap.get_submap_at_grid( src_pos );
                            if( srcsm == nullptr || destsm == nullptr ) {
                                debugmsg( "Tried to apply previewed mapgen at (%d,%d,%d) but the submap is not loaded", src_pos.x(),
                                          src_pos.y(), src_pos.z() );
                                continue;
                            }

                            std::swap( *destsm, *srcsm );

                            for( auto &veh : destsm->vehicles ) {
                                veh->sm_pos = here.get_abs_sub().xy() + dest_pos;
                            }

                            if( !destsm->spawns.empty() ) { // trigger spawnpoints
                                here.spawn_monsters( true );
                            }
                        }
                    }
                }

                // Since we cleared the vehicle cache of the whole z-level (not just the generate map), we add it back here
                for( int x = 0; x < here.getmapsize(); x++ ) {
                    for( int y = 0; y < here.getmapsize(); y++ ) {
                        const tripoint_rel_sm dest_pos = tripoint_rel_sm( x, y, target.z() );
                        const submap *destsm = here.get_submap_at_grid( dest_pos );
                        if( destsm == nullptr ) {
                            debugmsg( "Tried to update vehicle cache at (%d,%d,%d) but the submap is not loaded", dest_pos.x(),
                                      dest_pos.y(), dest_pos.z() );
                            continue;
                        }
                        here.update_vehicle_list( destsm, target.z() ); // update real map's vcaches
                    }
                }

                here.rebuild_vehicle_level_caches();
                break;
            }
            case OMTM_CHANGE_OTER_ID:
                popup( _( "Changed oter_id from '%s' (%s) to '%s' (%s)" ),
                       orig_oters->get_name( om_vision_level::full ), orig_oters.id().str(),
                       omt_ref->get_name( om_vision_level::full ), omt_ref.id().str() );
                break;
            case OMTM_TOGGLE_POSTPROCESS:
                editmap_state.run_post_process = !editmap_state.run_post_process;
                break;
        }
        showpreview = gpmenu.ret == UILIST_TIMEOUT ? !showpreview : true;
    } while( gpmenu.ret != UILIST_CANCEL && !changed_oter_id );

    editmap_state.tmpmap_ptr = nullptr;
    // original om_ter wasn't changed, restore it
    if( !changed_oter_id ) {
        overmap_buffer.ter_set( omt_pos, orig_oters );
    }
    cleartmpmap( tmpmap );
}

void editmap_ui::mapgen_retarget()
{
    editmap_brush &brush = get_brush();
    tripoint_bub_ms &target = brush.target;
    std::vector<tripoint_bub_ms> &target_list = brush.brush_points;

    input_context ctxt( "EDITMAP_RETARGET" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // Needed for timeout to be useful
    ctxt.register_action( "ANY_INPUT" );
    std::string action;
    tripoint_bub_ms origm = target;

    do {
        ui_manager::redraw();
        action = ctxt.handle_input( get_option<int>( "BLINK_SPEED" ) );
        if( const std::optional<tripoint_rel_omt> vec = ctxt.get_direction_rel_omt( action ) ) {
            point_rel_ms vec_ms = coords::project_to<coords::ms>( vec->xy() );
            tripoint_bub_ms ptarget = target + vec_ms;
            map &here = get_map();

            if( here.inbounds( ptarget ) &&
                here.inbounds( ptarget + point( SEEX, SEEY ) ) ) {
                target = ptarget;

                target_list.clear();
                for( int x = target.x() - SEEX + 1; x < target.x() + SEEX + 1; x++ ) {
                    for( int y = target.y() - SEEY + 1; y < target.y() + SEEY + 1; y++ ) {
                        if( x == target.x() - SEEX + 1 || x == target.x() + SEEX ||
                            y == target.y() - SEEY + 1 || y == target.y() + SEEY ) {
                            target_list.emplace_back( x, y, target.z() );
                        }
                    }
                }
            }
        }
    } while( action != "QUIT" && action != "CONFIRM" );
    if( action != "CONFIRM" ) {
        target = origm;
    }
}

void editmap_ui::edit_mapgen() const
{
    editmap_brush &brush = get_brush();
    tripoint_bub_ms &target = brush.target;
    std::vector<tripoint_bub_ms> &target_list = brush.brush_points;

    uilist gmenu;
    gmenu.input_category = "EDIT_MAPGEN";
    gmenu.desired_bounds = { 0.0f, 0.0f, -1.0f, -1.0f };

    for( size_t i = 0; i < overmap_terrains::get_all().size(); i++ ) {
        const oter_id id( i );

        gmenu.addentry( -1, !id.id().is_null(), 0, "[%3d] %s", static_cast<int>( id ), id.id().str() );
        gmenu.entries[i].extratxt.left = 1;
        gmenu.entries[i].extratxt.color = id->get_color( om_vision_level::full );
        gmenu.entries[i].extratxt.txt = id->get_symbol( om_vision_level::full );
    }
    real_coords tc;
    map &here = get_map();

    do {
        tc.fromabs( here.get_abs( { target.xy(), here.abs_sub.z() } ).xy().raw() );
        point_bub_ms omt_lpos = here.get_bub( point_abs_ms( tc.begin_om_pos() ) );
        tripoint_bub_ms om_ltarget = omt_lpos + tripoint( -1 + SEEX, -1 + SEEY, target.z() );

        if( target.x() != om_ltarget.x() || target.y() != om_ltarget.y() ) {
            target = om_ltarget;
            tc.fromabs( here.get_abs( { target.xy(), here.abs_sub.z() } ).xy().raw() );
        }
        target_list.clear();
        for( int x = target.x() - SEEX + 1; x < target.x() + SEEX + 1; x++ ) {
            for( int y = target.y() - SEEY + 1; y < target.y() + SEEY + 1; y++ ) {
                if( x == target.x() - SEEX + 1 || x == target.x() + SEEX ||
                    y == target.y() - SEEY + 1 || y == target.y() + SEEY ) {
                    target_list.emplace_back( x, y, target.z() );
                }
            }
        }
        input_context ctxt( gmenu.input_category, keyboard_mode::keycode );
        gmenu.query();

        if( gmenu.ret >= 0 ) {
            mapgen_preview( tc, oter_id( gmenu.ret ) );
        }

    } while( gmenu.ret != UILIST_CANCEL && gmenu.ret < 0 );

    brush.update_brush_points();
}

void editmap_ui::cleartmpmap( smallmap &tmpmap ) const
{
    tmpmap.delete_unmerged_submaps();

    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        level_cache &ch = tmpmap.get_cache( z );
        ch.clear_vehicle_cache();
        ch.vehicle_list.clear();
        ch.zone_vehicles.clear();
    }
}

editmap_ui::editmap_ui() :
    cataimgui::window( _( "MAP_EDITOR" ),
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNav )
{
    ictxt = setup_input_context();
}

void editmap_ui::draw_controls()
{
    draw_current_point_info();
    draw_select_menu();
}

void editmap_ui::draw_select_menu()
{
    editmap_uistate &editmap_state = uistate.editmap_state;
    const editmap_brush &brush = get_brush();

    auto get_drawing_symbol = []( bool is_drawing ) {
        return is_drawing ? "X" : " ";
    };

    const std::array<std::string, SELECTABLE_ACTIONS> action_names = {
        _( "Edit item stack" ),
        _( "Generate new mapgen" ),
        string_format( _( "Brush: %s" ), io::enum_to_string( brush.shape_basic_brush ) ),
        string_format( _( "[%s] Terrain: %s (%s)" ), get_drawing_symbol( brush.drawing_terrain ),
                       brush.selected_terrain->name(), brush.selected_terrain.id().c_str() ),
        string_format( _( "[%s] Furniture: %s (%s)" ), get_drawing_symbol( brush.drawing_furniture ),
                       brush.selected_furniture->name(), brush.selected_furniture.id().c_str() ),
        string_format( _( "[%s] Trap: %s (%s)" ), get_drawing_symbol( brush.drawing_trap ),
                       brush.selected_trap->name(), brush.selected_trap.id().c_str() ),
        string_format( _( "[%s] Field: %s (%s)" ), get_drawing_symbol( brush.drawing_field ),
                       brush.selected_field->get_name(), brush.selected_field.id().c_str() ),
        string_format( _( "[%s] Radiation: %d" ),
                       get_drawing_symbol( brush.drawing_radiation ),  brush.selected_radiation )
    };

    cataimgui::TextColoredParagraphNewline( c_light_gray, _( "? to view keybindings" ) );
    cataimgui::TextColoredParagraphNewline( c_light_gray, string_format( _( "Fast scroll: %s" ),
                                            editmap_state.fast_scroll ? _( "ON" ) : _( "OFF" ) ) );
    cataimgui::TextColoredParagraphNewline( c_light_green, editmap_state.mode == EMM_DRAWING ?
                                            _( "MODE: Draw" ) : _( "MODE: Select Brush Points" ) );
    ImGui::Separator();
    for( int i = 0; i < SELECTABLE_ACTIONS; i++ ) {
        if( ImGui::Selectable( action_names[i].c_str() ) ) {
            editmap_state.selected[i] = true;
        }
    }
}

void editmap_brush::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "origin", origin );
    json.member( "target", target );
    json.member( "selected_terrain", selected_terrain );
    json.member( "selected_furniture", selected_furniture );
    json.member( "selected_trap", selected_trap );
    json.member( "selected_field", selected_field );
    json.member( "selected_radiation", selected_radiation );
    json.member( "drawing_terrain", drawing_terrain );
    json.member( "drawing_furniture", drawing_furniture );
    json.member( "drawing_trap", drawing_trap );
    json.member( "drawing_field", drawing_field );
    json.member( "drawing_radiation", drawing_radiation );
    json.member( "selected_field_intensity", selected_field_intensity );
    json.member( "shape_basic_brush", shape_basic_brush );
    json.end_object();
}

void editmap_brush::deserialize( const JsonObject &jo )
{
    jo.read( "origin", origin );
    jo.read( "target", target );
    jo.read( "selected_terrain", selected_terrain );
    jo.read( "selected_furniture", selected_furniture );
    jo.read( "selected_trap", selected_trap );
    jo.read( "selected_field", selected_field );
    jo.read( "selected_radiation", selected_radiation );
    jo.read( "drawing_terrain", drawing_terrain );
    jo.read( "drawing_furniture", drawing_furniture );
    jo.read( "drawing_trap", drawing_trap );
    jo.read( "drawing_field", drawing_field );
    jo.read( "drawing_radiation", drawing_radiation );
    jo.read( "selected_field_intensity", selected_field_intensity );
    jo.read( "shape_basic_brush", shape_basic_brush );
}

void editmap_uistate::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "brush", brush );
    json.member( "mode", mode );
    json.member( "advanced_info_toggle", advanced_info_toggle );
    json.member( "blink", blink );
    json.member( "run_post_process", run_post_process );
    json.member( "fast_scroll", fast_scroll );
    json.end_object();
}

void editmap_uistate::deserialize( const JsonObject &jo )
{
    jo.read( "brush", brush );
    jo.read( "mode", mode );
    jo.read( "advanced_info_toggle", advanced_info_toggle );
    jo.read( "blink", blink );
    jo.read( "run_post_process", run_post_process );
    jo.read( "fast_scroll", fast_scroll );
}

void editmap_ui::draw_symbol_and_info( nc_color symbol_color, int symbol,
                                       nc_color draw_color, const std::string_view &info )
{
    cataimgui::TextColoredParagraph( symbol_color, std::string() + static_cast<char>( symbol ) );
    cataimgui::TextColoredParagraph( draw_color, string_format( _( " %s" ), info ) );
    ImGui::NewLine();
}

cataimgui::bounds editmap_ui::get_bounds()
{
    ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    return { 0, 0, viewport.x * 0.33f, -1.0f };
}

std::optional<field_type_id> editmap_brush::select_field()
{
    std::optional<field_type_id> selected_field_opt = select_feature<field_type>();

    if( !!selected_field_opt && selected_field_opt->id() != field_type_str_id::NULL_ID() ) {
        uilist femenu;
        field_type_id selected_field = *selected_field_opt;
        int i = 0;
        for( const field_intensity_level &intensity_level : selected_field->intensity_levels ) {
            i++;
            femenu.addentry( string_format( _( "%d: %s" ), i, intensity_level.name.translated() ) );
        }

        if( selected_field_intensity >= static_cast<int>( selected_field->intensity_levels.size() ) ) {
            selected_field_intensity = 1;
        }
        if( selected_field_intensity > 0 ) {
            femenu.entries[selected_field_intensity - 1].text_color = c_cyan;
            femenu.selected = selected_field_intensity - 1;
        }

        femenu.query();
        if( femenu.ret >= 0 ) {
            selected_field_intensity = femenu.ret + 1;
        }
        return selected_field;
    }

    return selected_field_opt;
}

int editmap_brush::select_radiation() const
{
    int new_radiation = 0;
    if( query_int( new_radiation, true, "Select radiation value:" ) ) {
        return new_radiation;
    }
    return selected_radiation;
}

void editmap_ui::brush_apply()
{
    const editmap_brush &brush = get_brush();

    if( brush.drawing_terrain ) {
        apply( brush.selected_terrain.obj(), brush );
    }
    if( brush.drawing_furniture ) {
        apply( brush.selected_furniture.obj(), brush );
    }
    if( brush.drawing_trap ) {
        apply( brush.selected_trap.obj(), brush );
    }
    if( brush.drawing_field ) {
        apply( brush.selected_field.obj(), brush );
    }
    if( brush.drawing_radiation ) {
        map &here = get_map();
        for( const tripoint_bub_ms &elem : brush.brush_points ) {
            here.set_radiation( elem, brush.selected_radiation );
        }
    }
}
