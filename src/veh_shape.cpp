#include "veh_shape.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

#include "avatar.h"
#include "cata_scope_helpers.h"
#include "color.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "input_enums.h"
#include "map.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "options.h"
#include "output.h"
#include "player_activity.h"
#include "point.h"
#include "ret_val.h"
#include "translations.h"
#include "uilist.h"
#include "ui_manager.h"
#include "units.h"
#include "veh_type.h"
#include "veh_utils.h"
#include "vehicle.h"
#include "vpart_range.h"

veh_shape::veh_shape( map &here, vehicle &vehicle ): veh( vehicle ), here( here ) { }

player_activity veh_shape::start( const tripoint_bub_ms &pos )
{
    map &temp = here; // Work around to get 'here' to be accepted into the lambda list
    avatar &you = get_avatar();
    on_out_of_scope cleanup( [&temp]() {
        temp.invalidate_map_cache( get_avatar().view_offset.z() );
    } );
    restore_on_out_of_scope view_offset_prev( you.view_offset );

    cursor_allowed.clear();
    for( const vpart_reference &part : veh.get_all_parts() ) {
        cursor_allowed.insert( part.pos_bub( here ) );
    }

    if( !set_cursor_pos( pos ) ) {
        debugmsg( "failed to set cursor at given part" );
        set_cursor_pos( veh.bub_part_pos( here, 0 ) );
    }

    const auto target_ui_cb = make_shared_fast<game::draw_callback_t>(
    [&]() {
        const avatar &you = get_avatar();
        g->draw_cursor_unobscuring( you.pos_bub( here ) + you.view_offset );
    } );
    g->add_draw_callback( target_ui_cb );
    ui_adaptor ui;
    ui.mark_resize();
    init_input();

    std::string action = "CONFIRM";

    while( true ) {
        g->invalidate_main_ui_adaptor();
        ui_manager::redraw();

        if( action.empty() ) {
            action = ctxt.handle_input( get_option<int>( "EDGE_SCROLL" ) );
        }

        if( handle_cursor_movement( action ) || ( action == "HELP_KEYBINDINGS" ) ) {
            action.clear();
            continue;
        } else if( action == "CONFIRM" ) {
            std::optional<vpart_reference> part;
            do {
                part = select_part_at_cursor( _( "Choose part to change shape" ),
                                              _( "Confirm to select or quit to select another tile." ),
                []( const vpart_reference & vp ) {
                    if( vp.info().variants.size() <= 1 ) {
                        return ret_val<void>::make_failure( _( "Only one shape" ) );
                    } else if( vp.part_displayed() && vp.part_displayed()->part_index() != vp.part_index() ) {
                        return ret_val<void>::make_success( _( "Part is obscured" ) );
                    }
                    return ret_val<void>::make_success();
                }, part );
                if( part.has_value() ) {
                    change_part_shape( part.value() );
                }
            } while( part.has_value() );
            action.clear();
        } else if( action == "QUIT" ) {
            return player_activity();
        } else {
            debugmsg( "here be dragons" );
            return player_activity();
        }
    }
}

std::vector<vpart_reference> veh_shape::parts_under_cursor() const
{
    std::vector<vpart_reference> res;
    // TODO: tons of methods getting parts from vehicle but all of them seem inadequate?
    for( int part_idx = 0; part_idx < veh.part_count_real(); part_idx++ ) {
        const vehicle_part &p = veh.part( part_idx );
        if( veh.bub_part_pos( here, p ) == get_cursor_pos() && !p.is_fake ) {
            res.emplace_back( veh, part_idx );
        }
    }
    return res;
}

std::optional<vpart_reference> veh_shape::select_part_at_cursor(
    const std::string &title, const std::string &extra_description,
    const std::function<ret_val<void>( const vpart_reference & )> &predicate,
    const std::optional<vpart_reference> &preselect ) const
{
    const std::vector<vpart_reference> parts = parts_under_cursor();
    if( parts.empty() ) {
        return std::nullopt;
    }

    uilist menu;
    menu.desc_enabled = !extra_description.empty();
    menu.hilight_disabled = true;

    for( const vpart_reference &pt : parts ) {
        ret_val<void> predicate_result = predicate( pt );
        uilist_entry entry( -1, true, MENU_AUTOASSIGN, pt.part().name(), "", predicate_result.str() );
        entry.desc = extra_description;
        entry.enabled = predicate_result.success();
        entry.retval = predicate_result.success() ? menu.entries.size() : -2;
        if( preselect && preselect->part_index() == pt.part_index() ) {
            menu.selected = menu.entries.size();
        }
        menu.entries.push_back( entry );
    }
    menu.text = title;
    menu.query();

    return menu.ret >= 0 ? std::optional<vpart_reference>( parts[menu.ret] ) : std::nullopt;
}

void veh_shape::change_part_shape( vpart_reference vpr ) const
{
    vehicle_part &part = vpr.part();
    const vpart_info &vpi = part.info();
    veh_menu menu( this->veh, _( "Choose cosmetic variant:" ) );
    std::string chosen_variant = part.variant; // support for cancel

    do {
        menu.reset( false );

        for( const auto &[vvid, vv] : vpi.variants ) {
            menu.add( vv.get_label() )
            .text_color( ( part.variant == vvid ) ? c_light_green : c_light_gray )
            .keep_menu_open()
            .skip_locked_check()
            .skip_theft_check()
            .location( veh.bub_part_pos( here, part ).raw() )
            .select( part.variant == vvid )
            .desc( _( "Confirm to save or exit to revert" ) )
            .symbol( vv.get_symbol_curses( 0_degrees, false ) )
            .symbol_color( vpi.color )
            .on_select( [&part, variant_id = vvid]() {
                part.variant = variant_id;
            } )
            .on_submit( [&chosen_variant, variant_id = vvid]() {
                chosen_variant = variant_id;
            } );
        }

        // An ordering of the line drawing symbols that does not result in
        // connecting when placed adjacent to each other vertically.
        menu.sort( []( const veh_menu_item & a, const veh_menu_item & b ) {
            const static std::map<int, int> symbol_order = {
                { LINE_XOXO, 0 }, { LINE_OXOX, 1 }, { LINE_XOOX, 2 }, { LINE_XXOO, 3 },
                { LINE_XXXX, 4 }, { LINE_OXXO, 5 }, { LINE_OOXX, 6 },
            };
            const auto a_iter = symbol_order.find( a._symbol );
            const auto b_iter = symbol_order.find( b._symbol );
            if( a_iter != symbol_order.end() ) {
                return ( b_iter == symbol_order.end() ) || ( a_iter->second < b_iter->second );
            } else {
                return ( b_iter == symbol_order.end() ) && ( a._symbol < b._symbol );
            }
        } );
    } while( menu.query() );

    part.variant = chosen_variant;
}

tripoint_bub_ms veh_shape::get_cursor_pos() const
{
    return cursor_pos;
}

bool veh_shape::set_cursor_pos( const tripoint_bub_ms &new_pos )
{
    avatar &you = get_avatar();

    int z = std::max( { new_pos.z(), -fov_3d_z_range, -OVERMAP_DEPTH } );
    z = std::min( {z, fov_3d_z_range, OVERMAP_HEIGHT } );

    if( !allow_zlevel_shift ) {
        z = cursor_pos.z();
    }
    tripoint_bub_ms target_pos( new_pos.xy(), z );

    if( cursor_allowed.find( target_pos ) == cursor_allowed.cend() ) {
        return false;
    }

    if( z != cursor_pos.z() ) {
        here.invalidate_map_cache( z );
    }
    cursor_pos = target_pos;
    you.view_offset = cursor_pos - you.pos_bub();
    return true;
}

bool veh_shape::handle_cursor_movement( const std::string &action )
{
    if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
        tripoint_rel_ms edge_scroll = g->mouse_edge_scrolling_terrain( ctxt );
        set_cursor_pos( get_cursor_pos() + edge_scroll );
    } else if( const std::optional<tripoint_rel_ms> delta = ctxt.get_direction_rel_ms( action ) ) {
        set_cursor_pos( get_cursor_pos() + *delta ); // move cursor with directional keys
    } else if( action == "zoom_in" ) {
        g->zoom_in();
    } else if( action == "zoom_out" ) {
        g->zoom_out();
    } else if( action == "SELECT" ) {
        const std::optional<tripoint_bub_ms> mouse_pos = ctxt.get_coordinates( g->w_terrain );
        if( !mouse_pos ) {
            return false;
        }
        if( get_cursor_pos() != *mouse_pos ) {
            set_cursor_pos( *mouse_pos );
        }
    } else if( action == "LEVEL_UP" ) {
        set_cursor_pos( get_cursor_pos() + tripoint::above );
    } else if( action == "LEVEL_DOWN" ) {
        set_cursor_pos( get_cursor_pos() + tripoint::below );
    } else {
        return false;
    }

    return true;
}

void veh_shape::init_input()
{
    ctxt = input_context( "VEH_SHAPES", keyboard_mode::keycode );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
}
