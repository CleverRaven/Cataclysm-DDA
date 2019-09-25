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
#include "bionics.h"
#include "comestible_inv.h"
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

comestible_inventory::comestible_inventory()
    : head_height( 5 )
    , min_w_height( 10 )
    , min_w_width( FULL_SCREEN_WIDTH )
    , max_w_width( 120 )
    , recalc( true )
    , redraw( true )
    , squares( {
    {
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_SOUTHWEST )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_SOUTH )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_SOUTHEAST )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_WEST )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_CENTER )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_EAST )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_NORTHWEST )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_NORTH )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_NORTHEAST )},

        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_DRAGGED )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_INVENTORY )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_WORN )},

        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_ALL )},
        {comestible_inv_area::get_info( comestible_inv_area_info::AIM_ALL_I_W )}
    }
} )
{
    //TODO:XXX
    // initialize screen coordinates for small overview 3x3 grid, depending on control scheme
    //if (tile_iso && use_tiles) {
    //  // Rotate the coordinates.
    //  squares[1].info.hscreen.x = 33;
    //  squares[2].info.hscreen.x = 36;
    //  squares[3].info.hscreen.y = 2;
    //  squares[4].info.hscreen.y = 3;
    //  squares[6].info.hscreen.y = 1;
    //  squares[7].info.hscreen.y = 2;
    //  squares[8].info.hscreen.x = 30;
    //  squares[9].info.hscreen.x = 33;
    //}
}

comestible_inventory::~comestible_inventory()
{
    save_settings();
    auto &aim_code = uistate.comestible_save.exit_code;
    if( aim_code != exit_re_entry ) {
        aim_code = exit_okay;
    }
    // Only refresh if we exited manually, otherwise we're going to be right back
    if( exit ) {
        werase( head );
        werase( minimap );
        werase( mm_border );
        werase( window );
        g->refresh_all();
        g->u.check_item_encumbrance_flag();
    }
}

void comestible_inventory::save_settings()
{
    bool is_reset = uistate.comestible_save.exit_code != exit_re_entry;
    pane->save_settings( is_reset );
    if( is_reset ) {
        if( uistate.comestible_save.bio != -1 ) {
            g->u.deactivate_bionic( uistate.comestible_save.bio );
        }
        uistate.comestible_save.bio = -1;
    }
}

comestible_inv_area *comestible_inventory::get_area( const std::string &action )
{
    for( auto &i : squares ) {
        if( action == i.info.actionname ) {
            return &i;
        }
    }
    return nullptr;
}

void comestible_inventory::init()
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
    window = catacurses::newwin( w_height, w_width, point( colstart, headstart + head_height ) );

    uistate.comestible_save.exit_code = exit_none;

    //pane is set by child
    int itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff
    pane->init( itemsPerPage, window );
}

void comestible_inventory::set_additional_info( std::vector<legend_data> data )
{
    pane->additional_info = data;
    pane->needs_redraw = true;
}

// be explicit with the values
enum aim_entry {
    ENTRY_START = 0,
    ENTRY_VEHICLE = 1,
    ENTRY_MAP = 2,
    ENTRY_RESET = 3
};

bool comestible_inventory::show_sort_menu( )
{
    uilist sm;
    sm.text = _( "Sort by... " );
    pane->add_sort_entries( sm );
    sm.query();

    if( sm.ret < 0 ) {
        return false;
    }

    pane->sortby = static_cast<comestible_inv_columns>( sm.ret );
    return true;
}
input_context comestible_inventory::register_actions()
{
    input_context ctxt( "COMESTIBLE_INVENTORY" );
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
    ctxt.register_action( "ITEMS_DEFAULT" );
    ctxt.register_action( "SAVE_DEFAULT" );

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

    ctxt.register_action( "CONSUME_FOOD" );
    return ctxt;
}

std::string comestible_inventory::process_actions( input_context &ctxt )
{
    using ai = comestible_inv_area_info;
    // current item in source pane, might be null
    comestible_inv_listitem *sitem = pane->get_cur_item_ptr();
    comestible_inv_area *new_square;

    const std::string action = ctxt.handle_input();

    if( action == "CATEGORY_SELECTION" ) {
        pane->inCategoryMode = !pane->inCategoryMode;
        pane->needs_redraw =
            true; // We redraw to force the color change of the highlighted line and header text.
    } else if( action == "HELP_KEYBINDINGS" ) {
        refresh( recalc, true );
    } else if( ( new_square = get_area( action ) ) != nullptr ) {
        if( pane->get_area().info.id == new_square->get_relative_location() ) {
            //DO NOTHING
        } else if( new_square->is_valid() ) {
            pane->set_area( *new_square, new_square->is_vehicle_default() );
            pane->index = 0;
            refresh( true, true );
        } else {
            popup( _( "You can't put items there!" ) );
            refresh( recalc, true ); // to clear the popup
        }
    } else if( action == "TOGGLE_FAVORITE" ) {
        if( sitem == nullptr || !sitem->is_item_entry() ) {
            return action;
        }
        for( auto *it : sitem->items ) {
            it->set_favorite( !it->is_favorite );
        }
        // recalc = true; In case we've merged faved and unfaved items
        refresh( true, true );
    } else if( action == "SORT" ) {
        if( show_sort_menu() ) {
            refresh( true, redraw );
        }
        refresh( recalc, true );
    } else if( action == "FILTER" ) {
        draw_item_filter_rules( pane->window, 1, 11, item_filter_type::FILTER );
        pane->start_user_filtering( w_height - 1, w_width / 2 - 4 );
    } else if( action == "RESET_FILTER" ) {
        pane->set_filter( "" );
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
    } else if( action == "EXAMINE" ) {
        if( sitem == nullptr || !sitem->is_item_entry() ) {
            return action;
        }
        int ret = 0;
        const int info_width = w_width / 2;
        const int info_startx = colstart + info_width;
        if( pane->get_area().info.id == ai::AIM_INVENTORY || pane->get_area().info.id == ai::AIM_WORN ) {
            int idx = pane->get_area().info.id == ai::AIM_INVENTORY ? sitem->idx :
                      player::worn_position_to_index( sitem->idx );
            // Setup a "return to AIM" activity. If examining the item creates a new activity
            // (e.g. reading, reloading, activating), the new activity will be put on top of
            // "return to AIM". Once the new activity is finished, "return to AIM" comes back
            // (automatically, see player activity handling) and it re-opens the AIM.
            // If examining the item did not create a new activity, we have to remove
            // "return to AIM".
            do_return_entry();
            assert( g->u.has_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) ) );
            ret = g->inventory_item_menu( idx, info_startx, info_width, game::RIGHT_OF_INFO );
            if( !g->u.has_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) ) ) {
                exit = true;
            } else {
                g->u.cancel_activity();
            }
            // Might have changed a stack (activated an item, repaired an item, etc.)
            if( pane->get_area().info.id == ai::AIM_INVENTORY ) {
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
            pane->scroll_by( +1 );
        } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
            pane->scroll_by( -1 );
        }
        refresh( recalc, true ); // item info window overwrote the other pane and the header
    } else if( action == "QUIT" ) {
        exit = true;
    } else if( action == "PAGE_DOWN" ) {
        pane->scroll_by( +pane->itemsPerPage );
    } else if( action == "PAGE_UP" ) {
        pane->scroll_by( -pane->itemsPerPage );
    } else if( action == "DOWN" ) {
        pane->scroll_by( +1 );
    } else if( action == "UP" ) {
        pane->scroll_by( -1 );
    } else if( action == "TOGGLE_VEH" ) {
        if( pane->get_area().has_vehicle() ) {
            if( pane->get_area().info.id != ai::AIM_DRAGGED ) {
                // Toggle between vehicle and ground
                pane->set_area( pane->get_area(), !pane->is_in_vehicle() );
                pane->index = 0;
                // make sure to update the minimap as well!
                refresh( recalc, true );
                pane->needs_recalc = true;
            }
        } else {
            popup( _( "No vehicle there!" ) );
            refresh( recalc, true );
        }
    } else if( action == "CONSUME_FOOD" ) {
        player &p = g->u;
        item *it = sitem->items.front();
        int pos = p.get_item_position( it );
        if( pos != INT_MIN ) {
            p.consume( pos );
        } else if( p.consume_item( *it ) ) {
            if( it->is_food_container() ) {
                it->contents.erase( it->contents.begin() );
                add_msg( _( "You leave the empty %s." ), it->tname() );
            } else {
                tripoint target = p.pos() + sitem->get_area().offset;
                item_location loc;
                if( sitem->from_vehicle ) {
                    const cata::optional<vpart_reference> vp = g->m.veh_at( target ).part_with_feature( "CARGO", true );
                    if( !vp ) {
                        assert( false ); //item is from vehicle, but there's no vehicle at that location
                        return action;
                    }
                    vehicle &veh = vp->vehicle();
                    const int part = vp->part_index();

                    loc = item_location( vehicle_cursor( veh, part ), it );
                } else {
                    if( sitem->get_area().info.type == ai::AREA_TYPE_PLAYER ) {
                        loc = item_location( p, it );
                    } else {
                        loc = item_location( map_cursor( target ), it );
                    }
                }
                loc.remove_item();
            }
        }
    }
    return action;
}

void comestible_inventory::display()
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
        pane->redraw();

        if( redraw ) {
            werase( head );
            draw_border( head );
            Messages::display_messages( head, 2, 1, w_width - 1, head_height - 2 );
            if( uistate.comestible_save.bio == -1 ) {
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

void comestible_inventory::redraw_minimap()
{
    werase( mm_border );

    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( pane->get_area().info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
        //point(1,0) - point_east so travis is happy
        mvwprintz( mm_border, point_east, c_light_gray, utf8_truncate( _( "Mul" ), minimap_width ) );
    }
    wrefresh( mm_border );

    werase( minimap );
    // get the center of the window
    tripoint pc = { getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0 };
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );

    if( pane->get_area().info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
        return;
    }

    char sym = minimap_get_sym();
    if( sym != '\0' ) {
        auto a = pane->get_area();
        auto pt = pc + a.offset;
        // invert the color if pointing to the player's position
        auto cl = a.info.type == comestible_inv_area_info::AREA_TYPE_PLAYER ?
                  invert_color( c_light_cyan ) : c_light_cyan.blink();
        mvwputch( minimap, pt.xy(), cl, sym );
    }

    // Invert player's tile color if exactly one pane points to player's tile
    bool player_selected = pane->get_area().info.type == comestible_inv_area_info::AREA_TYPE_PLAYER;
    g->u.draw( minimap, g->u.pos(), player_selected );
    wrefresh( minimap );
}

char comestible_inventory::minimap_get_sym() const
{
    if( pane->get_area().info.type == comestible_inv_area_info::AREA_TYPE_PLAYER ) {
        return '^';
    } else if( pane->is_in_vehicle() ) {
        return 'V';
    } else {
        return ' ';
    }
}

void comestible_inventory::do_return_entry()
{
    // only save pane settings
    g->u.assign_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) );
    g->u.activity.auto_resume = true;
    uistate.comestible_save.exit_code = exit_re_entry;
    save_settings();
}

void comestible_inventory::refresh( bool needs_recalc, bool needs_redraw )
{
    recalc = needs_recalc;
    pane->needs_recalc = needs_recalc;

    redraw = needs_redraw;
    pane->needs_redraw = needs_redraw;
}

/*******    FOOD    *******/

void comestible_inventory_food::init()
{
    pane = std::make_unique< comestible_inventory_pane_food>( squares );
    comestible_inventory::init();
}

input_context comestible_inventory_food::register_actions()
{
    input_context ctxt = comestible_inventory::register_actions();
    ctxt.register_action( "SWITCH_FOOD" );
    ctxt.register_action( "HEAT_UP" );
    ctxt.register_action( "CRAFT_WITH" );
    ctxt.register_action( "SHOW_HELP" );
    return ctxt;
}

void comestible_inventory_food::set_additional_info( std::vector<legend_data> data )
{
    std::pair<std::string, nc_color> desc;
    desc = g->u.get_hunger_description();
    data.emplace_back( string_format( "%s %s", _( "Food :" ), colorize( desc.first, desc.second ) ),
                       point_zero, c_dark_gray );
    desc = g->u.get_thirst_description();
    data.emplace_back( string_format( "%s %s", _( "Drink:" ), colorize( desc.first, desc.second ) ),
                       point_south, c_dark_gray );
    desc = g->u.get_pain_description();
    data.emplace_back( string_format( "%s %s", _( "Pain :" ), colorize( desc.first, desc.second ) ),
                       point( 0, 2 ), c_dark_gray );
    comestible_inventory::set_additional_info( data );
}

std::string comestible_inventory_food::process_actions( input_context &ctxt )
{
    const std::string action = comestible_inventory::process_actions( ctxt );

    // current item in source pane, might be null
    comestible_inv_listitem *sitem = pane->get_cur_item_ptr();
    if( action == "SWITCH_FOOD" ) {
        uistate.comestible_save.show_food = !uistate.comestible_save.show_food;
        pane->title = uistate.comestible_save.show_food ? _( "FOOD" ) : _( "DRUGS" );
        refresh( true, redraw );
    } else if( action == "HEAT_UP" ) {
        heat_up( sitem->items.front() );
    } else if( action == "CRAFT_WITH" ) {
        do_return_entry();

        assert( g->u.has_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) ) );

        item *it = sitem->items.front();
        if( it->is_food_container() ) {
            it = &it->contents.front();
        }
        std::string food_name = item::nname( it->typeId() );
        g->u.craft( tripoint_zero, string_format( "c:%s,", food_name ) );

        if( !g->u.has_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) ) ) {
            exit = true;
        } else {
            //TODO: next 2 lines seem hacky - refreshing twice?
            //without 1st line we can see craft menu after it closes
            //without 2nd line we can't see comestible menu after crafting closes

            g->u.cancel_activity();
            g->refresh_all();
            refresh( recalc, true );
        }
    } else if( action == "SHOW_HELP" ) {
        std::string desc;
        std::string status;
        status = ctxt.get_desc( "HEAT_UP" );
        desc += colorize( string_format( "%c", status[0] ), c_cyan );
        desc += colorize( string_format(
                              _( ": this food is frozen and cannot be consumed. Press %s to attempt reheating it.\n" ), status ),
                          c_light_gray );

        status = ctxt.get_desc( "CRAFT_WITH" );
        desc += colorize( string_format( "%c", status[0] ), c_cyan );
        desc += colorize( string_format(
                              _( ": this item cannot be eaten in its current form. Press %s to attempt cooking with it.\n" ),
                              status ), c_light_gray );
        desc += "\n";

        desc += colorize( "m", c_yellow );
        desc += colorize( _( ": this food is mushy and has reduced joy.\n" ), c_light_gray );

        desc += colorize( "s", c_yellow );
        desc += colorize( _( ": this is stimulant (e.g. coffe, pop).\n" ), c_light_gray );

        desc += colorize( "d", c_yellow );
        desc += colorize( _( ": this is depressant (e.g. alcohol).\n" ), c_light_gray );
        desc += "\n";

        desc += colorize( "a", c_yellow );
        desc += colorize( _( ": this item is mildly addictive.\n" ), c_light_gray );

        desc += colorize( "a", c_red );
        desc += colorize( _( ": this item is strongly addictive.\n" ), c_light_gray );

        desc += colorize( "!", c_light_red );
        desc += colorize( string_format( _( ": eating this will be considered stealing." ), status ),
                          c_light_gray );

        popup( desc );
        refresh( recalc, true );
    }
    return action;
}

void comestible_inventory_food::heat_up( item *it_to_heat )
{
    //in case we want to warm up a certail quantity later on
    int batch_size = 1;

    player &p = g->u;

    // basically we want a recipe that satisfies all the requirements for warming food up
    // water seems exactly what we need, so find it and use it.
    //might be good idea to create a dummy recipe
    item water = item( "water_clean", 0 );
    recipe_subset lr = p.get_learned_recipes();
    std::vector < const recipe * > water_recipes = lr.search_result( water.typeId() );
    requirement_data req = water_recipes[0]->requirements();

    const std::vector<std::vector<tool_comp>> &req_tools = req.get_tools();
    const std::vector<std::vector<quality_requirement>> &req_qual = req.get_qualities();

    // see if we have required tools (e.g. BOIL 1)
    bool has_qual = true;
    for( const auto &i : req_qual ) {
        for( auto &rq : i ) {
            if( !p.crafting_inventory().has_quality( rq.type, rq.level, batch_size ) ) {
                has_qual = false;
                break;
            }
        }
    }
    if( !has_qual ) {
        popup( _( "You don't have necessary tools to heat up your food." ) );
        return;
    }

    // see if we have enough charges or fire
    inventory crafting_inv;
    crafting_inv.form_from_map( p.pos(), PICKUP_RANGE, &p );
    std::vector<comp_selection<tool_comp>> tool_selections;
    for( const auto &it : req_tools ) {
        comp_selection<tool_comp> ts = p.select_tool_component(
        it, batch_size, crafting_inv, DEFAULT_HOTKEYS, true, true, []( int charges ) {
            return charges / 20 + charges % 20;
        } );
        if( ts.use_from == cancel ) {
            refresh( recalc, true );
            return; //Never mind.
        }

        //no item found
        if( ts.comp.type == itype_id( "null" ) ) {
            popup( _( "You don't have a neraby fire or tool with enough charges to heat up your food." ) );
            return;
        }

        tool_selections.push_back( ts );
    }

    for( auto &tool : tool_selections ) {
        p.consume_tools( tool, batch_size );
    }

    //actually heat up the item
    //lifted from iuse.cpp - static bool heat_item( player &p )
    item &target = it_to_heat->is_food_container() ? it_to_heat->contents.front() : *it_to_heat;
    p.mod_moves( -to_turns<int>( 3_seconds ) ); //initial preparations
    // simulates heat capacity of food, more weight = longer heating time
    // this is x2 to simulate larger delta temperature of frozen food in relation to
    // heating non-frozen food (x1); no real life physics here, only aproximations
    int move_mod = to_turns<int>( time_duration::from_seconds( to_gram( target.weight() ) ) );
    if( target.item_tags.count( "FROZEN" ) ) {
        target.apply_freezerburn();

        if( target.has_flag( "EATEN_COLD" ) ) {
            target.cold_up();
            add_msg( _( "You defrost the food, but don't heat it up, since you enjoy it cold." ) );
        } else {
            add_msg( _( "You defrost and heat up the food." ) );
            target.heat_up();
            // x2 because we have to defrost and heat
            move_mod *= 2;
        }
    } else {
        add_msg( _( "You heat up the food." ) );
        target.heat_up();
    }
    p.mod_moves( -move_mod ); // time needed to actually heat up
}

/*******    BIO    *******/

void comestible_inventory_bio::set_additional_info( std::vector<legend_data> data )
{
    std::pair<std::string, nc_color> desc = g->u.get_power_description();
    data.push_back( { string_format( "%s %s", _( "Power :" ), colorize( desc.first, desc.second ) ), point_zero, c_dark_gray } );
    comestible_inventory::set_additional_info( data );
}

void comestible_inventory_bio::init()
{
    pane = std::make_unique< comestible_inventory_pane_bio>( squares );
    comestible_inventory::init();
}

void comestible_inv( int b )
{
    static const trait_id trait_GRAZER( "GRAZER" );
    static const trait_id trait_RUMINANT( "RUMINANT" );

    player &p = g->u;
    map &m = g->m;

    //lifted from handle_action.cpp - void game::eat(item_location(*menu)(player & p), int pos)
    if( ( p.has_active_mutation( trait_RUMINANT ) || p.has_active_mutation( trait_GRAZER ) ) &&
        ( m.ter( p.pos() ) == t_underbrush || m.ter( p.pos() ) == t_shrub ) ) {
        if( p.get_hunger() < 20 ) {
            add_msg( _( "You're too full to eat the leaves from the %s." ), m.ter( p.pos() )->name() );
            return;
        } else {
            p.moves -= 400;
            m.ter_set( p.pos(), t_grass );
            add_msg( _( "You eat the underbrush." ) );
            item food( "underbrush", calendar::turn, 1 );
            p.eat( food );
            return;
        }
    }
    if( p.has_active_mutation( trait_GRAZER ) && ( m.ter( p.pos() ) == t_grass ||
            m.ter( p.pos() ) == t_grass_long || m.ter( p.pos() ) == t_grass_tall ) ) {
        if( p.get_hunger() < 8 ) {
            add_msg( _( "You're too full to graze." ) );
            return;
        } else {
            p.moves -= 400;
            add_msg( _( "You eat the grass." ) );
            item food( item( "grass", calendar::turn, 1 ) );
            p.eat( food );
            m.ter_set( p.pos(), t_dirt );
            if( m.ter( p.pos() ) == t_grass_tall ) {
                m.ter_set( p.pos(), t_grass_long );
            } else if( m.ter( p.pos() ) == t_grass_long ) {
                m.ter_set( p.pos(), t_grass );
            } else {
                m.ter_set( p.pos(), t_dirt );
            }
            return;
        }
    }
    if( p.has_active_mutation( trait_GRAZER ) ) {
        if( m.ter( p.pos() ) == t_grass_golf ) {
            add_msg( _( "This grass is too short to graze." ) );
            return;
        } else if( m.ter( p.pos() ) == t_grass_dead ) {
            add_msg( _( "This grass is dead and too mangled for you to graze." ) );
            return;
        } else if( m.ter( p.pos() ) == t_grass_white ) {
            add_msg( _( "This grass is tainted with paint and thus inedible." ) );
            return;
        }
    }

    if( uistate.comestible_save.exit_code != exit_re_entry ) {
        uistate.comestible_save.bio = b;
    }
    if( uistate.comestible_save.bio == -1 ) {
        comestible_inventory_food new_inv;
        new_inv.display();
    } else {
        comestible_inventory_bio new_inv;
        new_inv.display();
    }
}
