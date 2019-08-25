#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "field.h"
#include "input.h"
#include "item_category.h"
#include "item_search.h"
#include "item_stack.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "calendar.h"
#include "color.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "optional.h"
#include "ret_val.h"
#include "type_id.h"
#include "clzones.h"
#include "colony.h"
#include "enums.h"
#include "faction.h"
#include "item_location.h"
#include "map_selector.h"
#include "pimpl.h"
#include "bionics.h"
#include "comestible_inv.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <utility>
#include <numeric>

#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif

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
    }
}

void comestible_inventory::save_settings()
{
    bool is_reset = uistate.comestible_save.exit_code != exit_re_entry;
    pane.save_settings( is_reset );
    if( is_reset ) {
        if( uistate.comestible_save.bio != -1 ) {
            g->u.deactivate_bionic( uistate.comestible_save.bio );
        }
        uistate.comestible_save.bio = -1;
    }
}

comestible_inv_area *comestible_inventory::get_square( const std::string &action )
{
    for( size_t i = 0; i < squares.size(); i++ ) {
        if( action == squares[i].info.actionname ) {
            return &squares[i];
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

    headstart = 0; //(TERMY>w_height)?(TERMY-w_height)/2:0;
    colstart = TERMX > w_width ? ( TERMX - w_width ) / 2 : 0;

    head = catacurses::newwin( head_height, w_width - minimap_width, point( colstart, headstart ) );
    mm_border = catacurses::newwin( minimap_height + 2, minimap_width + 2,
                                    point( colstart + ( w_width - ( minimap_width + 2 ) ), headstart ) );
    minimap = catacurses::newwin( minimap_height, minimap_width,
                                  point( colstart + ( w_width - ( minimap_width + 1 ) ), headstart + 1 ) );
    window = catacurses::newwin( w_height, w_width, point( colstart, headstart + head_height ) );



    uistate.comestible_save.exit_code = exit_none;

    std::vector<comestible_inv_columns> columns;
    if( uistate.comestible_save.bio == -1 ) {
        columns = { COLUMN_CALORIES, COLUMN_QUENCH, COLUMN_JOY };
        if( g->u.can_estimate_rot() ) {
            columns.emplace_back( COLUMN_EXPIRES );
        } else {
            columns.emplace_back( COLUMN_SHELF_LIFE );
        }
    } else {
        columns = { COLUMN_ENERGY, COLUMN_SORTBY_CATEGORY };
    }
    int itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff
    pane.init( columns, itemsPerPage, window, &squares );
    set_pane_legend();
    if( uistate.comestible_save.bio == -1 ) {
        pane.special_filter = []( const item & it ) {
            const std::string n = it.get_category().name();
            if( uistate.comestible_save.show_food && n != "FOOD" ) {
                return true;
            }
            if( !uistate.comestible_save.show_food && n != "DRUGS" ) {
                return true;
            }

            if( !g->u.can_consume( it ) ) {
                return true;
            }
            return false;
        };
        pane.title = uistate.comestible_save.show_food ? "FOOD" : "DRUGS";
        pane.default_sortby = COLUMN_EXPIRES;
    } else {
        pane.special_filter = []( const item & it ) {
            return !g->u.can_consume( it ) || g->u.get_acquirable_energy( it ) <= 0;
        };
        pane.title = g->u.bionic_at_index( uistate.comestible_save.bio ).id.obj().name;
        pane.default_sortby = COLUMN_ENERGY;
    }
}

void comestible_inventory::set_pane_legend()
{
    std::vector<legend_data> data;
    if( uistate.comestible_save.bio == -1 ) {
        data.push_back( { 'z', c_light_gray, "Switch Food/Drugs" } );
        data.push_back( { 'h', c_light_red, "Warm up food" } );
        data.push_back( { '&', c_cyan, "Craft with" } );
    }
    pane.legend = data;
    pane.needs_redraw = true;
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
    pane.add_sort_enries( sm );
    sm.query();

    if( sm.ret < 0 ) {
        return false;
    }

    pane.sortby = static_cast<comestible_inv_columns>( sm.ret );
    return true;
}

void comestible_inventory::display( int bio )
{
    if( uistate.comestible_save.exit_code != exit_re_entry ) {
        uistate.comestible_save.bio = bio;
    }
    init();
    g->u.inv.restack( g->u );

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
    ctxt.register_action( "ITEMS_CONTAINER" );

    if( uistate.comestible_save.bio == -1 ) {
        ctxt.register_action( "CONSUME_FOOD" );
        ctxt.register_action( "SWITCH_FOOD" );
        ctxt.register_action( "HEAT_UP" );
        ctxt.register_action( "CRAFT_WITH" );
    }

    exit = false;
    redo( true, true );

    using ai = comestible_inv_area_info;
    while( !exit ) {
        if( g->u.moves < 0 ) {
            do_return_entry();
            return;
        }

        pane.redraw();

        if( redraw ) {
            werase( head );
            werase( minimap );
            werase( mm_border );
            draw_border( head );
            Messages::display_messages( head, 2, 1, w_width - 1, head_height - 2 );
            draw_minimap();

            const std::string msg = _( "< [?] show help >" );
            mvwprintz( head, point( w_width - ( minimap_width + 2 ) - utf8_width( msg ) - 1, 0 ),
                       c_white, msg );
            if( g->u.has_watch() ) {
                const std::string time = to_string_time_of_day( calendar::turn );
                mvwprintz( head, point( 2, 0 ), c_white, time );
            }
            wrefresh( head );
            refresh_minimap();
        }
        redo( false, false );

        // current item in source pane, might be null
        comestible_inv_listitem *sitem = pane.get_cur_item_ptr();
        //comestible_inv_area_info::aim_location changeSquare = NUM_AIM_LOCATIONS;
        comestible_inv_area *new_square;

        const std::string action = ctxt.handle_input();

        if( action == "CATEGORY_SELECTION" ) {
            pane.inCategoryMode = !pane.inCategoryMode;
            pane.needs_redraw =
                true; // We redraw to force the color change of the highlighted line and header text.
        } else if( action == "HELP_KEYBINDINGS" ) {
            redo( recalc, true );
        }   else if( ( new_square = get_square( action ) ) != nullptr ) {
            if( pane.get_area()->info.id == new_square->get_relative_location() ) {
                //DO NOTHING
            } else if( new_square->canputitems() ) {
                pane.set_area( new_square, new_square->is_vehicle_default() );
                pane.index = 0;
                redo( true, true );
            } else {
                popup( _( "You can't put items there!" ) );
                redo( recalc, true ); // to clear the popup
            }
        } else if( action == "TOGGLE_FAVORITE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            for( auto *it : sitem->items ) {
                it->set_favorite( !it->is_favorite );
            }
            // recalc = true; In case we've merged faved and unfaved items
            redo( true, true );
        } else if( action == "SORT" ) {
            if( show_sort_menu( ) ) {
                redo( true, redraw );
            }
            redo( recalc, true );
        } else if( action == "FILTER" ) {
            draw_item_filter_rules( pane.window, 1, 11, item_filter_type::FILTER );
            pane.do_filter( w_height - 1, w_width / 2 - 4 );
        } else if( action == "RESET_FILTER" ) {
            pane.set_filter( "" );
        } else if( action == "TOGGLE_AUTO_PICKUP" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            if( sitem->autopickup ) {
                get_auto_pickup().remove_rule( sitem->items.front() );
                sitem->autopickup = false;
            } else {
                get_auto_pickup().add_rule( sitem->items.front() );
                sitem->autopickup = true;
            }

            redo( true, redraw );
        } else if( action == "EXAMINE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            int ret = 0;
            const int info_width = w_width / 2;
            const int info_startx = colstart + info_width;
            if( pane.get_area()->info.id == ai::AIM_INVENTORY || pane.get_area()->info.id == ai::AIM_WORN ) {
                int idx = pane.get_area()->info.id == ai::AIM_INVENTORY ? sitem->idx :
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
                //src == comestible_inventory::side::left ? game::LEFT_OF_INFO : game::RIGHT_OF_INFO);
                if( !g->u.has_activity( activity_id( "ACT_ADV_INVENTORY" ) ) ) {
                    exit = true;
                } else {
                    g->u.cancel_activity();
                }
                // Might have changed a stack (activated an item, repaired an item, etc.)
                if( pane.get_area()->info.id == ai::AIM_INVENTORY ) {
                    g->u.inv.restack( g->u );
                }
                redo( true, redraw );
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
                pane.scroll_by( +1 );
            } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
                pane.scroll_by( -1 );
            }
            redo( recalc, true ); // item info window overwrote the other pane and the header
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "PAGE_DOWN" ) {
            pane.scroll_by( +pane.itemsPerPage );
        } else if( action == "PAGE_UP" ) {
            pane.scroll_by( -pane.itemsPerPage );
        } else if( action == "DOWN" ) {
            pane.scroll_by( +1 );
        } else if( action == "UP" ) {
            pane.scroll_by( -1 );
        } else if( action == "TOGGLE_VEH" ) {
            if( pane.get_area()->has_vehicle() ) {
                if( pane.get_area()->info.id != ai::AIM_DRAGGED ) {
                    // Toggle between vehicle and ground
                    pane.set_area( pane.get_area(), !pane.is_in_vehicle() );
                    pane.index = 0;
                    // make sure to update the minimap as well!
                    redo( recalc, true );
                    pane.needs_recalc = true;
                }
            } else {
                popup( _( "No vehicle there!" ) );
                redo( recalc, true );
            }
        } else if( action == "SWITCH_FOOD" ) {
            uistate.comestible_save.show_food = !uistate.comestible_save.show_food;
            pane.title = uistate.comestible_save.show_food ? "FOOD" : "DRUGS";
            redo( true, redraw );
        } else if( action == "HEAT_UP" ) {
            heat_up( sitem->items.front() );
        } else if( action == "CRAFT_WITH" ) {
            item *it = sitem->items.front();
            if( it->is_food_container() ) {
                it = &it->contents.front();
            }
            std::string food_name = it->nname( it->typeId() );
            g->u.craft( tripoint_zero, string_format( "c:%s", food_name ) );
            //TODO: next 2 lines seem hacky - refreshing twice?
            //without 1st line we can see craft menu after it closes
            //without 2nd line we can't see comestible menu after crafting closes
            g->refresh_all();
            redo( recalc, true );
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
                    tripoint target = p.pos() + sitem->area->offset;
                    item_location loc;
                    if( sitem->from_vehicle ) {
                        const cata::optional<vpart_reference> vp = g->m.veh_at( target ).part_with_feature( "CARGO", true );
                        if( !vp ) {
                            add_msg( _( "~~~~~~~~~ not vehicle?" ) );
                            return;
                        }
                        vehicle *const veh = &vp->vehicle();
                        const int part = vp->part_index();

                        loc = item_location( vehicle_cursor( *veh, part ), it );
                    } else {
                        if( sitem->area->info.type == ai::AREA_TYPE_PLAYER ) {
                            loc = item_location( p, it );
                        } else {
                            loc = item_location( map_cursor( target ), it );
                        }
                    }

                    loc.remove_item();
                }
            }
        }
    }
}

static const trait_id trait_GRAZER( "GRAZER" );
static const trait_id trait_RUMINANT( "RUMINANT" );
void comestible_inv( int b )
{
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
    comestible_inventory new_inv;
    new_inv.display( b );
}

void comestible_inventory::refresh_minimap()
{
    // redraw border around minimap
    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( pane.get_area()->info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
        mvwprintz( mm_border, point( 1, 0 ), c_light_gray, utf8_truncate( _( "Mul" ), minimap_width ) );
    }
    // refresh border, then minimap
    wrefresh( mm_border );
    wrefresh( minimap );
}

void comestible_inventory::draw_minimap()
{
    // get the center of the window
    tripoint pc = { getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0 };
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );

    if( pane.get_area()->info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
        return;
    }

    char sym = minimap_get_sym();
    if( sym != '\0' ) {
        auto sq = pane.get_area();
        auto pt = pc + sq->offset;
        // invert the color if pointing to the player's position
        auto cl = sq->info.type == comestible_inv_area_info::AREA_TYPE_PLAYER ?
                  invert_color( c_light_cyan ) : c_light_cyan.blink();
        mvwputch( minimap, pt.xy(), cl, sym );
    }

    // Invert player's tile color if exactly one pane points to player's tile
    bool player_selected = pane.get_area()->info.type == comestible_inv_area_info::AREA_TYPE_PLAYER;
    g->u.draw( minimap, g->u.pos(), player_selected );
}

char comestible_inventory::minimap_get_sym() const
{
    if( pane.get_area()->info.type == comestible_inv_area_info::AREA_TYPE_PLAYER ) {
        return '^';
    } else if( pane.is_in_vehicle() ) {
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

void comestible_inventory::redo( bool needs_recalc, bool needs_redraw )
{
    recalc = needs_recalc;
    pane.needs_recalc = needs_recalc;

    redraw = needs_redraw;
    pane.needs_redraw = needs_redraw;
}

void comestible_inventory::heat_up( item *it_to_heat )
{
    player &p = g->u;
    inventory map_inv = inventory();
    map_inv.form_from_map( p.pos(), 1 );

    bool has_heat_item = false;
    bool is_near_fire = false;
    bool can_use_bio = false;
    bool can_use_hotplate = false;

    //TODO: I think can use crafting.cpp -
    //  comp_selection<item_comp> player::select_item_component(){}
    //or
    //  comp_selection<tool_comp> player::select_tool_component(){}
    //to search and select items

    //first find tools we can use
    const std::function<bool( const item & )> &heat_filter =
    []( const item & it ) {

        bool can_heat = ( it.get_use( "HEAT_FOOD" ) != nullptr );
        return can_heat;
    };

    has_heat_item = p.has_item_with( heat_filter );
    if( !has_heat_item ) {
        has_heat_item = map_inv.has_item_with( heat_filter );
    }

    if( has_heat_item ) {
        if( g->m.has_nearby_fire( p.pos() ) ) {
            is_near_fire = true;
        } else if( p.has_active_bionic( bionic_id( "bio_tools" ) ) && p.power_level > 10 ) {
            can_use_bio = true;
        }
    }

    const std::function<bool( const item & )> &filter =
    []( const item & it ) {

        if( !it.is_tool() ) {
            return false;
        }
        bool is_hotplate = ( it.get_use( "HOTPLATE" ) != nullptr );
        if( !is_hotplate ) {
            return false;
        }

        return it.units_remaining( g->u ) >= it.type->charges_to_use();
    };

    std::vector<item *> hotplates = p.items_with( filter );
    std::vector<item *> hotplates_map = map_inv.items_with( filter );
    //hotplates.insert(hotplates.end(), hotplates_map.begin(), hotplates_map.end());

    can_use_hotplate = ( hotplates.size() > 0 ) || ( hotplates_map.size() > 0 );

    //check we can actually heat up
    if( !has_heat_item && !can_use_hotplate ) {
        popup( _( "You don't have a suitable container or hotplate with enough charges." ) );
        redo( recalc, true );
        return;
    }

    if( has_heat_item && !can_use_hotplate ) {
        if( !is_near_fire && !can_use_bio ) {
            popup( _( "You need to be near fire to heat an item in a container." ) );
            redo( recalc, true );
            return;
        }
    }

    //give user options to chose from
    int choice = -1;
    if( !( has_heat_item && is_near_fire ) ) {
        uilist sm;
        sm.text = _( "Choose a way to heat an item..." );
        int counter = 0;
        if( can_use_bio ) {
            sm.addentry( counter, true, 'a' + counter, _( "use bio tools" ) );
            counter++;
        }

        for( size_t i = 0; i < hotplates.size(); i++ ) {
            sm.addentry( counter, true, 'a' + counter, string_format( _( "%s in inventory" ),
                         hotplates.at( i )->display_name() ) );
            counter++;
        }

        for( size_t i = 0; i < hotplates_map.size(); i++ ) {
            sm.addentry( counter, true, 'a' + counter, string_format( _( "%s nearby" ),
                         hotplates_map.at( i )->display_name() ) );
            counter++;
        }
        //sm.selected = pane.sortby - SORTBY_NONE;
        sm.query();

        if( sm.ret < 0 ) {
            //'Never mind'
            redo( recalc, true );
            return;
        }
        choice = sm.ret;
    }

    //use charges as needed; choice == -1 if we can use fire
    if( choice != -1 ) {
        if( can_use_bio && choice == 0 ) {
            p.charge_power( -10 );
        } else {
            if( can_use_bio ) {
                choice--;
            }
            item *it_choice;
            if( static_cast<size_t>( choice ) < hotplates.size() ) {
                it_choice = hotplates[choice];
            } else {
                choice -= hotplates.size();
                it_choice = hotplates_map[choice];
            }
            int charges_to_use = it_choice->type->charges_to_use();
            std::list<item> used = {};
            tripoint pos = tripoint();
            it_choice->use_charges( it_choice->typeId(), charges_to_use, used, pos );
        }
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
