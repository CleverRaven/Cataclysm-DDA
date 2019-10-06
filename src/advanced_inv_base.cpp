#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "input.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "uistate.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "color.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "enums.h"
#include "pimpl.h"
#include "advanced_inv_base.h"
#include "recipe_dictionary.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>

enum aim_exit {
    exit_none = 0,
    exit_okay,
    exit_re_entry
};

advanced_inv_base::advanced_inv_base()
    : head_height( 5 )
    , min_w_height( 10 )
    , min_w_width( FULL_SCREEN_WIDTH )
    , max_w_width( 120 )
    , recalc( true )
    , redraw( true )
    , squares( {
    {
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_SOUTHWEST )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_SOUTH )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_SOUTHEAST )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_WEST )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_CENTER )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_EAST )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_NORTHWEST )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_NORTH )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_NORTHEAST )},

        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_INVENTORY )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_WORN )},

        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_ALL )},
        {advanced_inv_area::get_info( advanced_inv_area_info::AIM_ALL_I_W )}
    }
} )
{
}

advanced_inv_base::~advanced_inv_base()
{
    save_settings_internal();
    // Only refresh if we exited manually, otherwise we're going to be right back
    if( is_full_exit() ) {
        werase( head );
        werase( minimap );
        werase( mm_border );
        werase( pane.get()->window );
        g->refresh_all();
        g->u.check_item_encumbrance_flag();
    }
    if( is_full_exit() ) {
        save_state->exit_code = exit_okay;
    }
}

bool advanced_inv_base::is_full_exit()
{
    return save_state->exit_code != exit_re_entry;
}

void advanced_inv_base::save_settings()
{
    save_settings_internal();
}

void advanced_inv_base::save_settings_internal()
{
    pane.get()->save_settings( is_full_exit() && reset_on_exit );
}

void advanced_inv_base::init()
{
    for( auto &square : squares ) {
        square.init();
    }
    w_height = TERMY < min_w_height + head_height ? min_w_height : TERMY - head_height;
    w_width = TERMX < min_w_width ? min_w_width : TERMX > max_w_width ? max_w_width :
              static_cast<int>( TERMX );

    headstart = 0;
    colstart = TERMX > w_width ? ( TERMX - w_width ) / 2 : 0;

    head = catacurses::newwin( head_height, w_width - minimap_width, point( colstart, headstart ) );
    mm_border = catacurses::newwin( minimap_height + 2, minimap_width + 2,
                                    point( colstart + ( w_width - ( minimap_width + 2 ) ), headstart ) );
    minimap = catacurses::newwin( minimap_height, minimap_width,
                                  point( colstart + ( w_width - ( minimap_width + 1 ) ), headstart + 1 ) );

    save_state->exit_code = exit_none;

    init_pane();
}

void advanced_inv_base::init_pane()
{
    //pane is set by child
    int itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff
    catacurses::window window = catacurses::newwin( w_height, w_width, point( colstart,
                                headstart + head_height ) );
    get_pane()->init( itemsPerPage, window );
}

void advanced_inv_base::set_additional_info( std::vector<legend_data> data )
{
    get_pane()->additional_info = data;
    get_pane()->needs_redraw = true;
}

bool advanced_inv_base::show_sort_menu( )
{
    uilist sm;
    sm.text = _( "Sort by... " );
    get_pane()->add_sort_entries( sm );
    sm.query();

    if( sm.ret < 0 ) {
        return false;
    }

    get_pane()->sortby = static_cast<advanced_inv_columns>( sm.ret );
    return true;
}
input_context advanced_inv_base::register_actions()
{
    input_context ctxt( "ADVANCED_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "CATEGORY_SELECTION" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );

    ctxt.register_action( "TOGGLE_VEH" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "TOGGLE_AUTO_PICKUP" );
    ctxt.register_action( "TOGGLE_FAVORITE" );

    ctxt.register_action( "ITEMS_NW" );
    ctxt.register_action( "ITEMS_N" );
    ctxt.register_action( "ITEMS_NE" );
    ctxt.register_action( "ITEMS_W" );
    ctxt.register_action( "ITEMS_CE" );
    ctxt.register_action( "ITEMS_E" );
    ctxt.register_action( "ITEMS_SW" );
    ctxt.register_action( "ITEMS_S" );
    ctxt.register_action( "ITEMS_SE" );
    ctxt.register_action( "ITEMS_INVENTORY" );
    ctxt.register_action( "ITEMS_WORN" );
    ctxt.register_action( "ITEMS_AROUND" );
    ctxt.register_action( "ITEMS_AROUND_I_W" );
    ctxt.register_action( "ITEMS_DRAGGED_CONTAINER" );

    return ctxt;
}

std::string advanced_inv_base::process_actions( input_context &ctxt )
{
    using ai = advanced_inv_area_info;
    // current item in source pane, might be null
    advanced_inv_listitem *sitem = get_pane()->get_cur_item_ptr();

    const std::string action = ctxt.handle_input();

    if( action == "CATEGORY_SELECTION" ) {
        get_pane()->inCategoryMode = !get_pane()->inCategoryMode;
        get_pane()->needs_redraw =
            true; // We redraw to force the color change of the highlighted line and header text.
    } else if( action == "HELP_KEYBINDINGS" ) {
        refresh( recalc, true );
    } else if( action == "TOGGLE_FAVORITE" ) {
        if( sitem == nullptr || !sitem->is_item_entry() ) {
            return action;
        }
        for( auto *it : sitem->items ) {
            it->set_favorite( !it->is_favorite );
        }
        // recalc = true; In case we've merged faved and unfaved items
        refresh( true, true );
    } else if( action == "TOGGLE_AUTO_PICKUP" ) {
        if( sitem == nullptr || !sitem->is_item_entry() ) {
            return action;
        }
        if( sitem->autopickup ) {
            get_auto_pickup().remove_rule( sitem->items.front() );
            sitem->autopickup = false;
        } else {
            get_auto_pickup().add_rule( sitem->items.front() );
            sitem->autopickup = true;
        }

        refresh( true, redraw );
    } else if( action == "SORT" ) {
        bool needs_recalc = recalc || show_sort_menu();
        refresh( needs_recalc, true );
    } else if( action == "FILTER" ) {
        draw_item_filter_rules( get_pane()->window, 1, 11, item_filter_type::FILTER );
        get_pane()->start_user_filtering( w_height - 1, w_width / 2 - 4 );
    } else if( action == "RESET_FILTER" ) {
        get_pane()->set_filter( "" );
    } else if( action == "EXAMINE" ) {
        if( sitem == nullptr || !sitem->is_item_entry() ) {
            return action;
        }
        int ret = 0;
        const int info_width = w_width / 2;
        const int info_startx = colstart + info_width;
        if( get_pane()->get_area().info.id == ai::AIM_INVENTORY ||
            get_pane()->get_area().info.id == ai::AIM_WORN ) {
            int idx = get_pane()->get_area().info.id == ai::AIM_INVENTORY ? sitem->idx :
                      player::worn_position_to_index( sitem->idx );
            // Setup a "return to AIM" activity. If examining the item creates a new activity
            // (e.g. reading, reloading, activating), the new activity will be put on top of
            // "return to AIM". Once the new activity is finished, "return to AIM" comes back
            // (automatically, see player activity handling) and it re-opens the AIM.
            // If examining the item did not create a new activity, we have to remove
            // "return to AIM".
            do_return_entry();
            assert( g->u.has_activity( activity_id( "ACT_ADV_INVENTORY" ) ) );
            ret = g->inventory_item_menu( idx, info_startx, info_width, game::RIGHT_OF_INFO );
            if( !g->u.has_activity( activity_id( "ACT_ADV_INVENTORY" ) ) ) {
                exit = true;
            } else {
                g->u.cancel_activity();
            }
            // Might have changed a stack (activated an item, repaired an item, etc.)
            if( get_pane()->get_area().info.id == ai::AIM_INVENTORY ) {
                g->u.inv.restack( g->u );
            }
            refresh( true, redraw );
        } else {
            item &it = *sitem->items.front();
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            it.info( true, vThisItem );
            int iDummySelect = 0;
            ret = draw_item_info( info_startx,
                                  info_width, 0, 0, it.tname(), it.type_name(), vThisItem, vDummy, iDummySelect,
                                  false, false, true ).get_first_input();
        }
        if( ret == KEY_NPAGE || ret == KEY_DOWN ) {
            get_pane()->scroll_by( +1 );
        } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
            get_pane()->scroll_by( -1 );
        }
        refresh( recalc, true ); // item info window overwrote the other pane and the header
    } else if( action == "QUIT" ) {
        exit = true;
    } else if( action == "PAGE_DOWN" ) {
        get_pane()->scroll_by( +get_pane()->itemsPerPage );
    } else if( action == "PAGE_UP" ) {
        get_pane()->scroll_by( -get_pane()->itemsPerPage );
    } else if( action == "DOWN" ) {
        get_pane()->scroll_by( +1 );
    } else if( action == "UP" ) {
        get_pane()->scroll_by( -1 );
    } else if( action == "TOGGLE_VEH" ) {
        if( get_pane()->get_area().has_vehicle() ) {
            get_pane()->set_area( get_pane()->get_area(), !get_pane()->is_in_vehicle() );
            refresh( true, true );
        } else {
            popup( _( "No vehicle there!" ) );
            refresh( recalc, true );
        }
    } else { //point to a new location
        std::string err;
        advanced_inv_area *new_square = get_area( action, err );
        bool force_vehicle = action == "ITEMS_DRAGGED_CONTAINER";
        if( !err.empty() ) {
            popup( err );
            refresh( recalc, true );
        } else if( new_square == nullptr ) {
            //DO NOTHING - here when processing action from child class
        } else if( get_pane()->get_area().info.id == new_square->get_relative_location() ) {
            //here when user tried to switch to the same location
            if( !get_pane()->is_in_vehicle() && force_vehicle ) {
                get_pane()->set_area( get_pane()->get_area(), true );
                refresh( true, true );
            }
        } else {
            get_pane()->set_area( *new_square, new_square->is_vehicle_default() || force_vehicle );
            refresh( true, true );
        }
    }
    return action;
}

advanced_inv_area *advanced_inv_base::get_area( const std::string &action, std::string &error )
{
    advanced_inv_area *retval = nullptr;
    if( action == "ITEMS_DRAGGED_CONTAINER" ) {
        if( g->u.get_grab_type() == OBJECT_VEHICLE ) {
            for( auto &i : squares ) {
                if( g->u.grab_point == i.offset ) {
                    retval = &i;
                    break;
                }
            }
        } else {
            error = _( "Not dragging any vehicle!" );
        }
    } else {
        for( auto &i : squares ) {
            if( action == i.info.actionname ) {
                retval = &i;
                break;
            }
        }
    }

    if( retval != nullptr ) {
        if( !retval->is_valid() ) {
            error = _( "You can't put items there!" );
        }
    }

    return retval;
}

void advanced_inv_base::display()
{
    init();
    g->u.inv.restack( g->u );

    input_context ctxt = register_actions();

    exit = false;
    refresh( true, true );

    while( !exit ) {
        if( g->u.moves < 0 ) {
            do_return_entry();
            return;
        }

        if( recalc ) {
            set_additional_info();
        }
        redraw_pane();
        if( redraw ) {
            werase( head );
            draw_border( head );
            Messages::display_messages( head, 2, 1, w_width - 1, head_height - 2 );
            if( show_help ) {
                const std::string msg = string_format( _( "< [%s] Statuses info >" ),
                                                       ctxt.get_desc( "SHOW_HELP" ) );
                mvwprintz( head, point( w_width - ( minimap_width + 2 ) - utf8_width( msg ) - 1, 0 ),
                           c_white, msg );
            }
            if( g->u.has_watch() ) {
                const std::string time = to_string_time_of_day( calendar::turn );
                mvwprintz( head, point( 2, 0 ), c_white, time );
            }
            wrefresh( head );

            redraw_minimap();
        }
        refresh( false, false );

        process_actions( ctxt );
    }
}
void advanced_inv_base::redraw_pane()
{
    get_pane()->redraw();
}

void advanced_inv_base::redraw_minimap()
{
    werase( mm_border );

    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( get_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        mvwprintz( mm_border, point_east, c_light_gray, utf8_truncate( _( "Mul" ), minimap_width ) );
    }
    wrefresh( mm_border );

    werase( minimap );
    // get the center of the window
    tripoint pc = { getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0 };
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );

    if( get_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        return;
    }

    char sym = minimap_get_sym( get_pane() );
    if( sym != '\0' ) {
        auto a = get_pane()->get_area();
        auto pt = pc + a.offset;
        // invert the color if pointing to the player's position
        auto cl = a.info.type == advanced_inv_area_info::AREA_TYPE_PLAYER ?
                  invert_color( c_light_cyan ) : c_light_cyan.blink();
        mvwputch( minimap, pt.xy(), cl, sym );
    }

    bool player_selected = get_pane()->get_area().info.type == advanced_inv_area_info::AREA_TYPE_PLAYER;
    g->u.draw( minimap, g->u.pos(), player_selected );
    wrefresh( minimap );
}

char advanced_inv_base::minimap_get_sym( advanced_inv_pane *pane ) const
{
    if( pane->get_area().info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        return '\0';
    } else if( pane->get_area().info.type == advanced_inv_area_info::AREA_TYPE_PLAYER ) {
        return '@';
    } else if( pane->is_in_vehicle() ) {
        return 'V';
    } else {
        return ' ';
    }
}

void advanced_inv_base::do_return_entry()
{
    // only save pane settings
    g->u.assign_activity( activity_id( "ACT_ADV_INVENTORY" ) );
    g->u.activity.auto_resume = true;
    save_state->exit_code = exit_re_entry;
    save_settings();
}

void advanced_inv_base::refresh( bool needs_recalc, bool needs_redraw )
{
    recalc = needs_recalc;
    pane->needs_recalc = needs_recalc;

    redraw = needs_redraw;
    pane->needs_redraw = needs_redraw;
}
