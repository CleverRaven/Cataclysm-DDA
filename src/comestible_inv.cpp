#include "comestible_inv.h"

#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "field.h"
#include "game.h"
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

#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif

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

enum aim_exit {
    exit_none = 0,
    exit_okay,
    exit_re_entry
};

// *INDENT-OFF*
comestible_inventory::comestible_inventory()
	: head_height(5)
	, min_w_height(10)
	, min_w_width(FULL_SCREEN_WIDTH)
	, max_w_width(120)
	, inCategoryMode(false)
	, recalc(true)
	, redraw(true)
	, filter_edit(false)
	// panes don't need initialization, they are recalculated immediately
	, squares({
	{
			//               hx  hy  x    y   z
			{ AIM_INVENTORY, 25, 2, {0,   0,  0}, _("Inventory"),          _("IN") },
			{ AIM_SOUTHWEST, 30, 3, { -1,  1,  0}, _("South West"),         _("SW") },
			{ AIM_SOUTH,     33, 3, {0,   1,  0}, _("South"),              _("S")  },
			{ AIM_SOUTHEAST, 36, 3, {1,   1,  0}, _("South East"),         _("SE") },
			{ AIM_WEST,      30, 2, { -1,  0,  0}, _("West"),               _("W")  },
			{ AIM_CENTER,    33, 2, {0,   0,  0}, _("Directly below you"), _("DN") },
			{ AIM_EAST,      36, 2, {1,   0,  0}, _("East"),               _("E")  },
			{ AIM_NORTHWEST, 30, 1, { -1, -1,  0}, _("North West"),         _("NW") },
			{ AIM_NORTH,     33, 1, {0,  -1,  0}, _("North"),              _("N")  },
			{ AIM_NORTHEAST, 36, 1, {1,  -1,  0}, _("North East"),         _("NE") },
			{ AIM_DRAGGED,   25, 1, {0,   0,  0}, _("Grabbed Vehicle"),    _("GR") },
			{ AIM_ALL,       22, 3, {0,   0,  0}, _("Surrounding area"),   _("AL") },
			{ AIM_CONTAINER, 22, 1, {0,   0,  0}, _("Container"),          _("CN") },
			{ AIM_WORN,      25, 3, {0,   0,  0}, _("Worn Items"),         _("WR") }
		}
		})
{
	// initialize screen coordinates for small overview 3x3 grid, depending on control scheme
	if (tile_iso && use_tiles) {
		// Rotate the coordinates.
		squares[1].hscreen.x = 33;
		squares[2].hscreen.x = 36;
		squares[3].hscreen.y = 2;
		squares[4].hscreen.y = 3;
		squares[6].hscreen.y = 1;
		squares[7].hscreen.y = 2;
		squares[8].hscreen.x = 30;
		squares[9].hscreen.x = 33;
	}
}
// *INDENT-ON*

comestible_inventory::~comestible_inventory()
{
    save_settings( false );
    auto &aim_code = uistate.adv_inv_exit_code;
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

void comestible_inventory::save_settings( bool only_panes )
{
    //if (!only_panes) {
    //  uistate.adv_inv_last_coords = g->u.pos();
    //  uistate.adv_inv_src = src;
    //  uistate.adv_inv_dest = dest;
    //}
    uistate.adv_inv_in_vehicle[0] = pane.in_vehicle();
    uistate.adv_inv_area[0] = pane.get_area();
    uistate.adv_inv_index[0] = pane.index;
    uistate.adv_inv_filter[0] = pane.filter;
}

//TODO: fix settings
void comestible_inventory::load_settings()
{
    aim_exit aim_code = static_cast<aim_exit>( uistate.adv_inv_exit_code );
    aim_location location;
    if( get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) ) {
        location = static_cast<aim_location>( uistate.adv_inv_default_areas[0] );
    } else {
        location = static_cast<aim_location>( uistate.adv_inv_area[0] );
    }
    auto square = squares[location];
    // determine the square's vehicle/map item presence
    bool has_veh_items = square.can_store_in_vehicle() ?
                         !square.veh->get_items( square.vstor ).empty() : false;
    bool has_map_items = !g->m.i_at( square.pos ).empty();
    // determine based on map items and settings to show cargo
    bool show_vehicle = aim_code == exit_re_entry ?
                        uistate.adv_inv_in_vehicle[0] : has_veh_items ? true :
                        has_map_items ? false : square.can_store_in_vehicle();
    pane.set_area( square, show_vehicle );
    //pane.sortby = static_cast<advanced_inv_sortby>(uistate.adv_inv_sort[0]);
    pane.index = uistate.adv_inv_index[0];
    pane.filter = uistate.adv_inv_filter[0];
    uistate.adv_inv_exit_code = exit_none;

    pane.filter_show_food = true;
}

std::string comestible_inventory::get_sortname( comestible_inv_sortby sortby )
{
    switch( sortby ) {
        case COMESTIBLE_SORTBY_NAME:
            return _( "name" );
        case COMESTIBLE_SORTBY_WEIGHT:
            return _( "weight" );
        case COMESTIBLE_SORTBY_VOLUME:
            return _( "volume" );
        case COMESTIBLE_SORTBY_CALORIES:
            return _( "calories" );
        case COMESTIBLE_SORTBY_QUENCH:
            return _( "quench" );
        case COMESTIBLE_SORTBY_JOY:
            return _( "joy" );
        case COMESTIBLE_SORTBY_SPOILAGE:
            return _( "spoilage" );
    }
    return "!BUG!";
}

bool comestible_inventory::get_square( const std::string &action, aim_location &ret )
{
    if( action == "ITEMS_INVENTORY" ) {
        ret = AIM_INVENTORY;
    } else if( action == "ITEMS_WORN" ) {
        ret = AIM_WORN;
    } else if( action == "ITEMS_NW" ) {
        ret = screen_relative_location( AIM_NORTHWEST );
    } else if( action == "ITEMS_N" ) {
        ret = screen_relative_location( AIM_NORTH );
    } else if( action == "ITEMS_NE" ) {
        ret = screen_relative_location( AIM_NORTHEAST );
    } else if( action == "ITEMS_W" ) {
        ret = screen_relative_location( AIM_WEST );
    } else if( action == "ITEMS_CE" ) {
        ret = AIM_CENTER;
    } else if( action == "ITEMS_E" ) {
        ret = screen_relative_location( AIM_EAST );
    } else if( action == "ITEMS_SW" ) {
        ret = screen_relative_location( AIM_SOUTHWEST );
    } else if( action == "ITEMS_S" ) {
        ret = screen_relative_location( AIM_SOUTH );
    } else if( action == "ITEMS_SE" ) {
        ret = screen_relative_location( AIM_SOUTHEAST );
    } else if( action == "ITEMS_AROUND" ) {
        ret = AIM_ALL;
    } else if( action == "ITEMS_DRAGGED_CONTAINER" ) {
        ret = AIM_DRAGGED;
    } else if( action == "ITEMS_CONTAINER" ) {
        ret = AIM_CONTAINER;
    } else {
        return false;
    }
    return true;
}

void comestible_inventory::print_items( comestible_inventory_pane &pane )
{
    const auto &items = pane.items;
    const catacurses::window &window = pane.window;
    const auto index = pane.index;
    const int page = index / itemsPerPage;
    bool compact = TERMX <= 100;

    int max_width = getmaxx( window );
    std::string spaces( max_width - 4, ' ' );

    nc_color norm = c_white;

    //print inventory's current and total weight + volume
    if( pane.get_area() == AIM_INVENTORY || pane.get_area() == AIM_WORN ) {
        const double weight_carried = convert_weight( g->u.weight_carried() );
        const double weight_capacity = convert_weight( g->u.weight_capacity() );
        std::string volume_carried = format_volume( g->u.volume_carried() );
        std::string volume_capacity = format_volume( g->u.volume_capacity() );
        // align right, so calculate formatted head length
        const std::string formatted_head = string_format( "%.1f/%.1f %s  %s/%s %s",
                                           weight_carried, weight_capacity, weight_units(),
                                           volume_carried,
                                           volume_capacity,
                                           volume_units_abbr() );
        //const int hrightcol = columns - 1 - formatted_head.length();
        //nc_color color = weight_carried > weight_capacity ? c_red : c_light_green;
        //mvwprintz(window, 4, hrightcol, color, "%.1f", weight_carried);
        wprintz( window, c_light_gray, "/%.1f %s  ", weight_capacity, weight_units() );
        nc_color color = g->u.volume_carried().value() > g->u.volume_capacity().value() ? c_red :
                         c_light_green;
        wprintz( window, color, volume_carried );
        wprintz( window, c_light_gray, "/%s %s", volume_capacity, volume_units_abbr() );
    } else { //print square's current and total weight + volume
        std::string formatted_head;
        if( pane.get_area() == AIM_ALL ) {
            formatted_head = string_format( "%3.1f %s  %s %s",
                                            convert_weight( squares[pane.get_area()].weight ),
                                            weight_units(),
                                            format_volume( squares[pane.get_area()].volume ),
                                            volume_units_abbr() );
        } else {
            units::volume maxvolume = 0_ml;
            auto &s = squares[pane.get_area()];
            if( pane.get_area() == AIM_CONTAINER && s.get_container( pane.in_vehicle() ) != nullptr ) {
                maxvolume = s.get_container( pane.in_vehicle() )->get_container_capacity();
            } else if( pane.in_vehicle() ) {
                maxvolume = s.veh->max_volume( s.vstor );
            } else {
                maxvolume = g->m.max_volume( s.pos );
            }
            formatted_head = string_format( "%3.1f %s  %s/%s %s",
                                            convert_weight( s.weight ),
                                            weight_units(),
                                            format_volume( s.volume ),
                                            format_volume( maxvolume ),
                                            volume_units_abbr() );
        }
        mvwprintz( window, 4, max_width - 1 - formatted_head.length(), norm, formatted_head );
    }

    const size_t last_x = max_width - 2;
    const size_t name_startpos = compact ? 1 : 4;

    const int num_columns = 8 + 1; //to add columns change this
    const int col_size[num_columns] = { 0,
                                        4, 5, 7, 8,
                                        9, 7, 4, 12
                                      }; //to change column size change this
    const int total_col_width = std::accumulate( col_size, col_size + num_columns, 0 );
    int cur_col = 0;
    int cur_col_x = last_x - total_col_width;

    const size_t src_startpos = cur_col_x += col_size[cur_col++];
    const size_t amt_startpos = cur_col_x += col_size[cur_col++];
    const size_t weight_startpos = cur_col_x += col_size[cur_col++];
    const size_t vol_startpos = cur_col_x += col_size[cur_col++];

    const size_t cal_startpos = cur_col_x += col_size[cur_col++];
    const size_t quench_startpos = cur_col_x += col_size[cur_col++];
    const size_t joy_startpos = cur_col_x += col_size[cur_col++];
    const size_t expires_startpos = cur_col_x += col_size[cur_col++];

    int max_name_length = src_startpos - name_startpos - 1; // Default name length

    //~ Items list header. Table fields length without spaces: amt - 4, weight - 5, vol - 4.
    const int table_hdr_len1 = utf8_width( _( "amt weight vol" ) ); // Header length type 1
    //~ Items list header. Table fields length without spaces: src - 2, amt - 4, weight - 5, vol - 4.
    const int table_hdr_len2 = utf8_width( _( "src amt weight vol" ) ); // Header length type 2

    mvwprintz( window, 5, compact ? 1 : 4, c_light_gray, _( "Name (charges)" ) );
    //if (pane.get_area() == AIM_ALL && !compact) {
    //  mvwprintz(window, 5, last_x - table_hdr_len2 + 1, c_light_gray, _("src amt weight vol"));
    //  max_name_length = src_startpos - name_startpos - 1; // 1 for space
    //}
    //else {
    //  mvwprintz(window, 5, last_x - table_hdr_len1 + 1, c_light_gray, _("amt weight vol"));
    //}

    mvwprintz( window, 5, src_startpos, c_light_gray, _( "src" ) );
    mvwprintz( window, 5, amt_startpos + 1, c_light_gray, _( "amt" ) );
    mvwprintz( window, 5, weight_startpos, c_light_gray, _( "weight" ) );
    mvwprintz( window, 5, vol_startpos + 2, c_light_gray, _( "vol" ) );
    mvwprintz( window, 5, cal_startpos, c_light_gray, _( "calories" ) );
    mvwprintz( window, 5, quench_startpos, c_light_gray, _( "quench" ) );
    mvwprintz( window, 5, joy_startpos, c_light_gray, _( "joy" ) );
    mvwprintz( window, 5, expires_startpos, c_light_gray, _( "expires in" ) );

    player &p = g->u;

    for( int i = page * itemsPerPage, x = 0; i < static_cast<int>( items.size() ) &&
         x < itemsPerPage; i++, x++ ) {
        const auto &sitem = items[i];
        if( sitem.is_category_header() ) {
            mvwprintz( window, 6 + x, ( max_width - utf8_width( sitem.name ) - 6 ) / 2, c_cyan, "[%s]",
                       sitem.name );
            continue;
        }
        if( !sitem.is_item_entry() ) {
            // Empty entry at the bottom of a page.
            continue;
        }

        const auto &it = p.get_consumable_from( *sitem.items.front() );

        const bool selected = index == i;

        nc_color thiscolor = it.color_in_inventory();
        nc_color thiscolordark = c_dark_gray;
        nc_color print_color;

        if( selected ) {
            thiscolor = inCategoryMode && pane.sortby == SORTBY_CATEGORY ? c_white_red : hilite( c_white );
            thiscolordark = hilite( thiscolordark );
            if( compact ) {
                mvwprintz( window, 6 + x, 1, thiscolor, "  %s", spaces );
            } else {
                mvwprintz( window, 6 + x, 1, thiscolor, ">>%s", spaces );
            }
        }

        std::string item_name;
        std::string stolen_string;
        bool stolen = false;
        if( it.has_owner() ) {
            const faction *item_fac = it.get_owner();
            if( item_fac != g->faction_manager_ptr->get( faction_id( "your_followers" ) ) ) {
                stolen_string = string_format( "<color_light_red>!</color>" );
                stolen = true;
            }
        }
        if( it.is_money() ) {
            //Count charges
            // TODO: transition to the item_location system used for the normal inventory
            unsigned int charges_total = 0;
            for( const auto item : sitem.items ) {
                charges_total += item->charges;
            }
            if( stolen ) {
                item_name = string_format( "%s %s", stolen_string, it.display_money( sitem.items.size(),
                                           charges_total ) );
            } else {
                item_name = it.display_money( sitem.items.size(), charges_total );
            }
        } else {
            if( stolen ) {
                item_name = string_format( "%s %s", stolen_string, it.display_name() );
            } else {
                //item_name = it.display_name();
                item_name = sitem.items.front()->display_name();
            }
        }
        if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
            item_name = string_format( "%s %s", it.symbol(), item_name );
        }

        //print item name
        trim_and_print( window, 6 + x, compact ? 1 : 4, max_name_length, thiscolor, item_name );

        //print src column
        // TODO: specify this is coming from a vehicle!
        if( pane.get_area() == AIM_ALL && !compact ) {
            mvwprintz( window, 6 + x, src_startpos, thiscolor, squares[sitem.area].shortname );
        }

        //print "amount" column
        int it_amt = sitem.stacks;
        if( it_amt > 1 ) {
            print_color = thiscolor;
            if( it_amt > 9999 ) {
                it_amt = 9999;
                print_color = selected ? hilite( c_red ) : c_red;
            }
            mvwprintz( window, 6 + x, amt_startpos, print_color, "%4d", it_amt );
        }

        //print weight column
        double it_weight = convert_weight( sitem.weight );
        size_t w_precision;
        print_color = it_weight > 0 ? thiscolor : thiscolordark;

        if( it_weight >= 1000.0 ) {
            if( it_weight >= 10000.0 ) {
                print_color = selected ? hilite( c_red ) : c_red;
                it_weight = 9999.0;
            }
            w_precision = 0;
        } else if( it_weight >= 100.0 ) {
            w_precision = 1;
        } else {
            w_precision = 2;
        }
        mvwprintz( window, 6 + x, weight_startpos, print_color, "%5.*f", w_precision, it_weight );

        //print volume column
        bool it_vol_truncated = false;
        double it_vol_value = 0.0;
        std::string it_vol = format_volume( sitem.volume, 5, &it_vol_truncated, &it_vol_value );
        if( it_vol_truncated && it_vol_value > 0.0 ) {
            print_color = selected ? hilite( c_red ) : c_red;
        } else {
            print_color = sitem.volume.value() > 0 ? thiscolor : thiscolordark;
        }
        mvwprintz( window, 6 + x, vol_startpos, print_color, it_vol );

        if( sitem.autopickup ) {
            mvwprintz( window, 6 + x, 1, magenta_background( it.color_in_inventory() ),
                       compact ? it.tname().substr( 0, 1 ) : ">" );
        }
        //get_consumable_item
        char *string_format = "";
        islot_comestible it_c = get_edible_comestible( p, it ); // .type->comestible;

        //print "CALORIES" column
        int it_cal = p.kcal_for( it );
        string_format = set_string_params( print_color, it_cal, selected );
        mvwprintz( window, 6 + x, cal_startpos, print_color, string_format, it_cal );

        //print "QUENCH" column
        int it_q = it_c.quench;
        string_format = set_string_params( print_color, it_q, selected );
        mvwprintz( window, 6 + x, quench_startpos, print_color, string_format, it_q );

        //print "JOY" column
        int it_joy = p.fun_for( it ).first;
        string_format = set_string_params( print_color, it_joy, selected, it.has_flag( "MUSHY" ) );
        mvwprintz( window, 6 + x, joy_startpos, print_color, string_format, it_joy );

        //TODO: g->u.can_estimate_rot()
        //print "SPOILS IN" column
        std::string it_spoils_string;
        if( it_c.spoils > 0_turns ) {
            if( !it.rotten() ) {
                it_spoils_string = get_time_left_rounded( p, it );
            }
        }
        print_color = selected ? hilite( c_white ) : c_cyan;
        mvwprintz( window, 6 + x, expires_startpos, print_color, "%s", it_spoils_string );

        /*
            append_cell( [ this, &p ]( const item_location & loc ) {
                std::string cbm_name;

                switch( p.get_cbm_rechargeable_with( get_consumable_item( loc ) ) ) {
                    case rechargeable_cbm::none:
                        break;
                    case rechargeable_cbm::battery:
                        cbm_name = _( "Battery" );
                        break;
                    case rechargeable_cbm::reactor:
                        cbm_name = _( "Reactor" );
                        break;
                    case rechargeable_cbm::furnace:
                        cbm_name = _( "Furnace" );
                        break;
                }

                if( !cbm_name.empty() ) {
                    return string_format( "<color_cyan>%s</color>", cbm_name );
                }

                return std::string();
            }, _( "CBM" ) );

            append_cell( [ this, &p ]( const item_location & loc ) {
                return good_bad_none( p.get_acquirable_energy( get_consumable_item( loc ) ) );
            }, _( "ENERGY" ) );

        */
    }
}

struct comestible_inv_sorter {
    int counter;
    comestible_inv_sortby sortby;
    comestible_inv_sorter( comestible_inv_sortby sort ) {
        sortby = sort;
        counter = 0;
    }
    bool operator()( const comestible_inv_listitem &d1, const comestible_inv_listitem &d2 ) {
        if( d1.is_category_header() ) {
            return true;
        } else if( d2.is_category_header() ) {
            return false;
        }

        /*
              this code will put 'hot' items to the top of the list
              problem is heating from inventory will remove it from aiming spot... decide if this is needed
        item* it1 = d1.items.front();
        item* it2 = d2.items.front();

        bool is_it1_hot = (it1->has_flag("HOT") ||
            (it1->contents.size() > 0 && it1->contents.front().has_flag("HOT")));
        bool is_it2_hot = (it2->has_flag("HOT") ||
            (it2->contents.size() > 0 && it2->contents.front().has_flag("HOT")));
        if  (is_it1_hot != is_it2_hot) {
            return is_it1_hot;
        }
        */

        switch( sortby ) {
            case COMESTIBLE_SORTBY_NAME:
                const std::string *n1;
                const std::string *n2;
                if( d1.name_without_prefix == d2.name_without_prefix ) {
                    if( d1.name == d2.name ) {
                        //fall through
                        break;
                    } else {
                        //if names without prefix equal, compare full name
                        n1 = &d1.name;
                        n2 = &d2.name;
                    }
                } else {
                    //else compare name without prefix
                    n1 = &d1.name_without_prefix;
                    n2 = &d2.name_without_prefix;
                }

                return std::lexicographical_compare( n1->begin(), n1->end(), n2->begin(), n2->end(),
                                                     sort_case_insensitive_less() );
                break;
            case COMESTIBLE_SORTBY_WEIGHT:
                if( d1.weight != d2.weight ) {
                    return d1.weight > d2.weight;
                }
                break;
            case COMESTIBLE_SORTBY_VOLUME:
                if( d1.volume != d2.volume ) {
                    return d1.volume > d2.volume;
                }
                break;
            case COMESTIBLE_SORTBY_CALORIES:
                if( d1.calories != d2.calories ) {
                    return d1.calories > d2.calories;
                }
                break;
            case COMESTIBLE_SORTBY_QUENCH:
                if( d1.quench != d2.quench ) {
                    return d1.quench > d2.quench;
                }
                break;
            case COMESTIBLE_SORTBY_JOY:
                if( d1.joy != d2.joy ) {
                    return d1.joy > d2.joy;
                }
                break;
            case COMESTIBLE_SORTBY_SPOILAGE:

                break;
        }
        // secondary sort by SPOILAGE
        return d1.items.front()->spoilage_sort_order() < d2.items.front()->spoilage_sort_order();
    }
};

void comestible_inventory::menu_square( uilist &menu )
{
    assert( menu.entries.size() >= 9 );
    int ofs = -25 - 4;
    int sel = screen_relative_location( static_cast <aim_location>( menu.selected + 1 ) );
    for( int i = 1; i < 10; i++ ) {
        aim_location loc = screen_relative_location( static_cast <aim_location>( i ) );
        char key = get_location_key( loc );
        bool in_vehicle = squares[loc].can_store_in_vehicle();
        const char *bracket = in_vehicle ? "<>" : "[]";
        // always show storage option for vehicle storage, if applicable
        bool canputitems = menu.entries[i - 1].enabled && squares[loc].canputitems();
        nc_color bcolor = canputitems ? sel == loc ? h_white : c_light_gray : c_dark_gray;
        nc_color kcolor = canputitems ? sel == loc ? h_white : c_light_gray : c_dark_gray;
        const int x = squares[loc].hscreen.x + ofs;
        const int y = squares[loc].hscreen.y + 5;
        mvwprintz( menu.window, y, x, bcolor, "%c", bracket[0] );
        wprintz( menu.window, kcolor, "%c", key );
        wprintz( menu.window, bcolor, "%c", bracket[1] );
    }
}

inline char comestible_inventory::get_location_key( aim_location area )
{
    switch( area ) {
        case AIM_INVENTORY:
            return 'I';
        case AIM_WORN:
            return 'W';
        case AIM_CENTER:
            return '5';
        case AIM_ALL:
            return 'A';
        case AIM_DRAGGED:
            return 'D';
        case AIM_CONTAINER:
            return 'C';
        case AIM_NORTH:
        case AIM_SOUTH:
        case AIM_EAST:
        case AIM_WEST:
        case AIM_NORTHEAST:
        case AIM_NORTHWEST:
        case AIM_SOUTHEAST:
        case AIM_SOUTHWEST:
            return  get_direction_key( area );
        default:
            debugmsg( "invalid [aim_location] in get_location_key()!" );
            return ' ';
    }
}

char comestible_inventory::get_direction_key( aim_location area )
{

    if( area == screen_relative_location( AIM_SOUTHWEST ) ) {
        return '1';
    }
    if( area == screen_relative_location( AIM_SOUTH ) ) {
        return '2';
    }
    if( area == screen_relative_location( AIM_SOUTHEAST ) ) {
        return '3';
    }
    if( area == screen_relative_location( AIM_WEST ) ) {
        return '4';
    }
    if( area == screen_relative_location( AIM_EAST ) ) {
        return '6';
    }
    if( area == screen_relative_location( AIM_NORTHWEST ) ) {
        return '7';
    }
    if( area == screen_relative_location( AIM_NORTH ) ) {
        return '8';
    }
    if( area == screen_relative_location( AIM_NORTHEAST ) ) {
        return '9';
    }
    debugmsg( "invalid [aim_location] in get_direction_key()!" );
    return '0';
}

int comestible_inventory::print_header( comestible_inventory_pane &pane, aim_location sel )
{
    const catacurses::window &window = pane.window;
    int area = pane.get_area();
    int wwidth = getmaxx( window );
    int ofs = wwidth - 25 - 2 - 14;
    for( int i = 0; i < NUM_AIM_LOCATIONS; ++i ) {
        const char key = get_location_key( static_cast<aim_location>( i ) );
        const char *bracket = squares[i].can_store_in_vehicle() ? "<>" : "[]";
        bool in_vehicle = pane.in_vehicle() && squares[i].id == area && sel == area && area != AIM_ALL;
        bool all_brackets = area == AIM_ALL && ( i >= AIM_SOUTHWEST && i <= AIM_NORTHEAST );
        nc_color bcolor = c_red;
        nc_color kcolor = c_red;
        if( squares[i].canputitems( pane.get_cur_item_ptr() ) ) {
            bcolor = in_vehicle ? c_light_blue :
                     area == i || all_brackets ? c_light_gray : c_dark_gray;
            kcolor = area == i ? c_white : sel == i ? c_light_gray : c_dark_gray;
        }
        const int x = squares[i].hscreen.x + ofs;
        const int y = squares[i].hscreen.y;
        mvwprintz( window, y, x, bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%c", in_vehicle && sel != AIM_DRAGGED ? 'V' : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }
    return squares[AIM_INVENTORY].hscreen.y + ofs;
}

int comestible_inv_area::get_item_count() const
{
    if( id == AIM_INVENTORY ) {
        return g->u.inv.size();
    } else if( id == AIM_WORN ) {
        return g->u.worn.size();
    } else if( id == AIM_ALL ) {
        return 0;
    } else if( id == AIM_DRAGGED ) {
        return can_store_in_vehicle() ? veh->get_items( vstor ).size() : 0;
    } else {
        return g->m.i_at( pos ).size();
    }
}

void comestible_inv_area::init()
{
    pos = g->u.pos() + off;
    veh = nullptr;
    vstor = -1;
    volume = 0_ml;   // must update in main function
    weight = 0_gram; // must update in main function
    switch( id ) {
        case AIM_INVENTORY:
        case AIM_WORN:
            canputitemsloc = true;
            break;
        case AIM_DRAGGED:
            if( g->u.get_grab_type() != OBJECT_VEHICLE ) {
                canputitemsloc = false;
                desc[0] = _( "Not dragging any vehicle!" );
                break;
            }
            // offset for dragged vehicles is not statically initialized, so get it
            off = g->u.grab_point;
            // Reset position because offset changed
            pos = g->u.pos() + off;
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
            } else {
                veh = nullptr;
                vstor = -1;
            }
            if( vstor >= 0 ) {
                desc[0] = veh->name;
                canputitemsloc = true;
                max_size = MAX_ITEM_IN_VEHICLE_STORAGE;
            } else {
                veh = nullptr;
                canputitemsloc = false;
                desc[0] = _( "No dragged vehicle!" );
            }
            break;
        case AIM_CONTAINER:
            // set container position based on location
            set_container_position();
            // location always valid, actual check is done in canputitems()
            // and depends on selected item in pane (if it is valid container)
            canputitemsloc = true;
            if( get_container() == nullptr ) {
                desc[0] = _( "Invalid container!" );
            }
            break;
        case AIM_ALL:
            desc[0] = _( "All 9 squares" );
            canputitemsloc = true;
            break;
        case AIM_SOUTHWEST:
        case AIM_SOUTH:
        case AIM_SOUTHEAST:
        case AIM_WEST:
        case AIM_CENTER:
        case AIM_EAST:
        case AIM_NORTHWEST:
        case AIM_NORTH:
        case AIM_NORTHEAST:
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
                    false ) ) {
                veh = &vp->vehicle();
                vstor = vp->part_index();
            } else {
                veh = nullptr;
                vstor = -1;
            }
            canputitemsloc = can_store_in_vehicle() || g->m.can_put_items_ter_furn( pos );
            max_size = MAX_ITEM_IN_SQUARE;
            if( can_store_in_vehicle() ) {
                desc[1] = vpart_position( *veh, vstor ).get_label().value_or( "" );
            }
            // get graffiti or terrain name
            desc[0] = g->m.has_graffiti_at( pos ) ?
                      g->m.graffiti_at( pos ) : g->m.name( pos );
        default:
            break;
    }

    /* assemble a list of interesting traits of the target square */
    // fields? with a special case for fire
    bool danger_field = false;
    const field &tmpfld = g->m.field_at( pos );
    for( auto &fld : tmpfld ) {
        const field_entry &cur = fld.second;
        if( fld.first.obj().has_fire ) {
            flags.append( _( " <color_white_red>FIRE</color>" ) );
        } else {
            if( cur.is_dangerous() ) {
                danger_field = true;
            }
        }
    }
    if( danger_field ) {
        flags.append( _( " DANGER" ) );
    }

    // trap?
    const trap &tr = g->m.tr_at( pos );
    if( tr.can_see( pos, g->u ) && !tr.is_benign() ) {
        flags.append( _( " TRAP" ) );
    }

    // water?
    static const std::array<ter_id, 8> ter_water = {
        {t_water_dp, t_water_pool, t_swater_dp, t_water_sh, t_swater_sh, t_sewage, t_water_moving_dp, t_water_moving_sh }
    };
    auto ter_check = [this]
    ( const ter_id & id ) {
        return g->m.ter( this->pos ) == id;
    };
    if( std::any_of( ter_water.begin(), ter_water.end(), ter_check ) ) {
        flags.append( _( " WATER" ) );
    }

    // remove leading space
    if( flags.length() && flags[0] == ' ' ) {
        flags.erase( 0, 1 );
    }
}

void comestible_inventory::init()
{
    for( auto &square : squares ) {
        square.init();
    }

    load_settings();

    w_height = TERMY < min_w_height + head_height ? min_w_height : TERMY - head_height;
    w_width = TERMX < min_w_width ? min_w_width : TERMX > max_w_width ? max_w_width :
              static_cast<int>( TERMX );

    headstart = 0; //(TERMY>w_height)?(TERMY-w_height)/2:0;
    colstart = TERMX > w_width ? ( TERMX - w_width ) / 2 : 0;

    head = catacurses::newwin( head_height, w_width - minimap_width, headstart, colstart );
    mm_border = catacurses::newwin( minimap_height + 2, minimap_width + 2, headstart,
                                    colstart + ( w_width - ( minimap_width + 2 ) ) );
    minimap = catacurses::newwin( minimap_height, minimap_width, headstart + 1,
                                  colstart + ( w_width - ( minimap_width + 1 ) ) );
    window = catacurses::newwin( w_height, w_width, headstart + head_height, colstart );

    itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff

    pane.window = window;
}

comestible_inv_listitem::comestible_inv_listitem( item *an_item, int index, int count,
        aim_location area, bool from_vehicle )
    : idx( index )
    , area( area )
    , id( an_item->typeId() )
    , name( an_item->tname( count ) )
    , name_without_prefix( an_item->tname( 1, false ) )
    , autopickup( get_auto_pickup().has_rule( an_item ) )
    , stacks( count )
    , volume( an_item->volume() * stacks )
    , weight( an_item->weight() * stacks )
    , cat( &an_item->get_category() )
    , from_vehicle( from_vehicle )
{
    player &p = g->u;
    item it = p.get_consumable_from( *an_item );
    islot_comestible it_c = get_edible_comestible( p, it );
    calories = p.kcal_for( it );
    quench = it_c.quench;
    joy = p.fun_for( it ).first;

    items.push_back( an_item );
    assert( stacks >= 1 );
}

comestible_inv_listitem::comestible_inv_listitem( const std::list<item *> &list, int index,
        aim_location area, bool from_vehicle ) :
    idx( index ),
    area( area ),
    id( list.front()->typeId() ),
    items( list ),
    name( list.front()->tname( list.size() ) ),
    name_without_prefix( list.front()->tname( 1, false ) ),
    autopickup( get_auto_pickup().has_rule( list.front() ) ),
    stacks( list.size() ),
    volume( list.front()->volume() * stacks ),
    weight( list.front()->weight() * stacks ),
    cat( &list.front()->get_category() ),
    from_vehicle( from_vehicle )
{
    player &p = g->u;
    item it = p.get_consumable_from( *list.front() );
    islot_comestible it_c = get_edible_comestible( p, it );
    calories = p.kcal_for( it );
    quench = it_c.quench;
    joy = p.fun_for( it ).first;
    assert( stacks >= 1 );
}

comestible_inv_listitem::comestible_inv_listitem()
    : idx()
    , area()
    , id( "null" )
    , autopickup()
    , stacks()
    , cat( nullptr )
{
}

comestible_inv_listitem::comestible_inv_listitem( const item_category *cat )
    : idx()
    , area()
    , id( "null" )
    , name( cat->name() )
    , autopickup()
    , stacks()
    , cat( cat )
{
}

comestible_inv_listitem::comestible_inv_listitem( const std::string cat_name )
    : idx()
    , area()
    , id( "null" )
    , name( cat_name )
    , autopickup()
    , stacks()
    , cat( cat )
{
}

bool comestible_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool comestible_inv_listitem::is_item_entry() const
{
    return !items.empty();
}

const islot_comestible &comestible_inv_listitem::get_edible_comestible( player &p,
        const item &it ) const
{
    if( it.is_comestible() && p.can_eat( it ).success() ) {
        // Ok since can_eat() returns false if is_craft() is true
        return *it.type->comestible;
    }
    static const islot_comestible dummy{};
    return dummy;
}

bool comestible_inventory_pane::is_filtered( const comestible_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool comestible_inventory_pane::is_filtered( const item &it ) const
{
    const std::string n = it.get_category().name();
    if( filter_show_food && n != "FOOD" ) {
        return true;
    }
    if( !filter_show_food && n != "DRUGS" ) {
        return true;
    }

    player &p = g->u;
    if( !p.can_consume( it ) ) {
        return true;
    }

    const std::string str = it.tname();
    if( filtercache.find( str ) == filtercache.end() ) {
        const auto filter_fn = item_filter_from_string( filter );
        filtercache[str] = filter_fn;

        return !filter_fn( it );
    }

    return !filtercache[str]( it );
}

// roll our own, to handle moving stacks better
using itemstack = std::vector<std::list<item *> >;

template <typename T>
static itemstack i_stacked( T items )
{
    //create a new container for our stacked items
    itemstack stacks;
    //    // make a list of the items first, so we can add non stacked items back on
    //    std::list<item> items(things.begin(), things.end());
    // used to recall indices we stored `itype_id' item at in itemstack
    std::unordered_map<itype_id, std::set<int>> cache;
    // iterate through and create stacks
    for( auto &elem : items ) {
        const auto id = elem.typeId();
        auto iter = cache.find( id );
        bool got_stacked = false;
        // cache entry exists
        if( iter != cache.end() ) {
            // check to see if it stacks with each item in a stack, not just front()
            for( auto &idx : iter->second ) {
                for( auto &it : stacks[idx] ) {
                    if( ( got_stacked = it->display_stacked_with( elem ) ) ) {
                        stacks[idx].push_back( &elem );
                        break;
                    }
                }
                if( got_stacked ) {
                    break;
                }
            }
        }
        if( !got_stacked ) {
            cache[id].insert( stacks.size() );
            stacks.push_back( { &elem } );
        }
    }
    return stacks;
}

void comestible_inventory_pane::add_items_from_area( comestible_inv_area &square,
        bool vehicle_override )
{
    assert( square.id != AIM_ALL );
    square.volume = 0_ml;
    square.weight = 0_gram;
    if( !square.canputitems() ) {
        return;
    }
    map &m = g->m;
    player &u = g->u;
    // Existing items are *not* cleared on purpose, this might be called
    // several times in case all surrounding squares are to be shown.
    if( square.id == AIM_INVENTORY ) {
        const invslice &stacks = u.inv.slice();
        for( size_t x = 0; x < stacks.size(); ++x ) {
            std::list<item *> item_pointers;
            for( item &i : *stacks[x] ) {
                item_pointers.push_back( &i );
            }
            comestible_inv_listitem it( item_pointers, x, square.id, false );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    } else if( square.id == AIM_WORN ) {
        auto iter = u.worn.begin();
        for( size_t i = 0; i < u.worn.size(); ++i, ++iter ) {
            comestible_inv_listitem it( &*iter, i, 1, square.id, false );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    } else if( square.id == AIM_CONTAINER ) {
        item *cont = square.get_container( in_vehicle() );
        if( cont != nullptr ) {
            if( !cont->is_container_empty() ) {
                // filtering does not make sense for liquid in container
                item *it = &square.get_container( in_vehicle() )->contents.front();
                comestible_inv_listitem ait( it, 0, 1, square.id, in_vehicle() );
                square.volume += ait.volume;
                square.weight += ait.weight;
                items.push_back( ait );
            }
            square.desc[0] = cont->tname( 1, false );
        }
    } else {
        bool is_in_vehicle = square.can_store_in_vehicle() && ( in_vehicle() || vehicle_override );
        const itemstack &stacks = is_in_vehicle ?
                                  i_stacked( square.veh->get_items( square.vstor ) ) :
                                  i_stacked( m.i_at( square.pos ) );

        for( size_t x = 0; x < stacks.size(); ++x ) {
            comestible_inv_listitem it( stacks[x], x, square.id, is_in_vehicle );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    }
}

void comestible_inventory_pane::paginate( size_t itemsPerPage )
{
    if( sortby != SORTBY_CATEGORY ) {
        return; // not needed as there are no category entries here.
    }
    // first, we insert all the items, then we sort the result
    for( size_t i = 0; i < items.size(); ++i ) {
        if( i % itemsPerPage == 0 ) {
            // first entry on the page, should be a category header
            if( items[i].is_item_entry() ) {
                items.insert( items.begin() + i, comestible_inv_listitem( items[i].cat ) );
            }
        }
        if( ( i + 1 ) % itemsPerPage == 0 && i + 1 < items.size() ) {
            // last entry of the page, but not the last entry at all!
            // Must *not* be a category header!
            if( items[i].is_category_header() ) {
                items.insert( items.begin() + i, comestible_inv_listitem() );
            }
        }
    }
}

void comestible_inventory::recalc_pane()
{
    pane.recalc = false;
    pane.items.clear();
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.
    if( pane.get_area() == AIM_ALL ) {
        auto &alls = squares[AIM_ALL];
        auto &there = pane;// s[-p + 1];
        auto &other = squares[there.get_area()];
        alls.volume = 0_ml;
        alls.weight = 0_gram;
        for( auto &s : squares ) {
            // All the surrounding squares, nothing else
            if( s.id < AIM_SOUTHWEST || s.id > AIM_NORTHEAST ) {
                continue;
            }

            // To allow the user to transfer all items from all surrounding squares to
            // a specific square, filter out items that are already on that square.
            // e.g. left pane AIM_ALL, right pane AIM_NORTH. The user holds the
            // enter key down in the left square and moves all items to the other side.
            const bool same = other.is_same( s );

            // Deal with squares with ground + vehicle storage
            // Also handle the case when the other tile covers vehicle
            // or the ground below the vehicle.
            if( s.can_store_in_vehicle() && !( same && there.in_vehicle() ) ) {
                bool do_vehicle = there.get_area() == s.id ? !there.in_vehicle() : true;
                pane.add_items_from_area( s, do_vehicle );
                alls.volume += s.volume;
                alls.weight += s.weight;
            }

            // Add map items
            if( !same || there.in_vehicle() ) {
                pane.add_items_from_area( s );
                alls.volume += s.volume;
                alls.weight += s.weight;
            }
        }
    } else {
        pane.add_items_from_area( squares[pane.get_area()] );
    }

    //TODO: just put it as a part of the header instead of one of the items
    // Insert category headers
    if( pane.filter_show_food ) {
        pane.items.push_back( comestible_inv_listitem( "FOOD" ) );
    } else {
        pane.items.push_back( comestible_inv_listitem( "CHEMICALS" ) );
    }

    // Insert category headers (only expected when sorting by category)
    //if (pane.sortby == COMESTIBLE_SORTBY_CATEGORY) {
    //
    //  std::set<const item_category*> categories;
    //  for (auto& it : pane.items) {
    //      categories.insert(it.cat);
    //  }
    //  for (auto& cat : categories) {
    //      pane.items.push_back(comestible_inv_listitem(cat));
    //  }
    //}

    // Finally sort all items (category headers will now be moved to their proper position)
    std::stable_sort( pane.items.begin(), pane.items.end(), comestible_inv_sorter( pane.sortby ) );
    pane.paginate( itemsPerPage );
}

void comestible_inventory_pane::fix_index()
{
    if( items.empty() ) {
        index = 0;
        return;
    }
    if( index < 0 ) {
        index = 0;
    } else if( static_cast<size_t>( index ) >= items.size() ) {
        index = static_cast<int>( items.size() ) - 1;
    }
    skip_category_headers( +1 );
}

void comestible_inventory::redraw_pane()
{
    // don't update ui if processing demands
    if( is_processing() ) {
        return;
    }
    if( recalc || pane.recalc ) {
        recalc_pane();
    } else if( !( redraw || pane.redraw ) ) {
        return;
    }
    pane.redraw = false;
    pane.fix_index();

    const comestible_inv_area &square = squares[pane.get_area()];
    auto w = pane.window;

    werase( w );
    print_items( pane );

    auto itm = pane.get_cur_item_ptr();
    int width = print_header( pane, itm != nullptr ? itm->area : pane.get_area() );
    bool same_as_dragged = ( square.id >= AIM_SOUTHWEST && square.id <= AIM_NORTHEAST ) &&
                           // only cardinals
                           square.id != AIM_CENTER && pane.in_vehicle() && // not where you stand, and pane is in vehicle
                           square.off == squares[AIM_DRAGGED].off; // make sure the offsets are the same as the grab point
    const comestible_inv_area &sq = same_as_dragged ? squares[AIM_DRAGGED] : square;
    bool car = square.can_store_in_vehicle() && pane.in_vehicle() && sq.id != AIM_DRAGGED;
    auto name = utf8_truncate( car ? sq.veh->name : sq.name, width );
    auto desc = utf8_truncate( sq.desc[car], width );
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, 1, 2, c_green, name );
    mvwprintz( w, 2, 2, c_light_blue, desc );
    trim_and_print( w, 3, 2, width, c_cyan, square.flags );

    const int max_page = ( pane.items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( max_page > 1 ) {
        const int page = pane.index / itemsPerPage;
        mvwprintz( w, 4, 2, c_light_blue, _( "[<] page %d of %d [>]" ), page + 1, max_page );
    }

    wattron( w, c_cyan );
    // draw a darker border around the inactive pane
    draw_border( w, BORDER_COLOR );
    mvwprintw( w, 0, 3, _( "< [s]ort: %s >" ), get_sortname( pane.sortby ) );
    int max = square.max_size;
    if( max > 0 ) {
        int itemcount = square.get_item_count();
        int fmtw = 7 + ( itemcount > 99 ? 3 : itemcount > 9 ? 2 : 1 ) +
                   ( max > 99 ? 3 : max > 9 ? 2 : 1 );
        mvwprintw( w, 0, w_width / 2 - fmtw, "< %d/%d >", itemcount, max );
    }

    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );
    if( !filter_edit ) {
        if( !pane.filter.empty() ) {
            mvwprintw( w, getmaxy( w ) - 1, 2, "< %s: %s >", fprefix, pane.filter );
        } else {
            mvwprintw( w, getmaxy( w ) - 1, 2, "< %s >", fprefix );
        }
    }

    wattroff( w, c_white );

    if( !filter_edit && !pane.filter.empty() ) {
        mvwprintz( w, getmaxy( w ) - 1, 6 + std::strlen( fprefix ), c_white,
                   pane.filter );
        mvwprintz( w, getmaxy( w ) - 1,
                   getmaxx( w ) - std::strlen( fsuffix ) - 2, c_white, "%s", fsuffix );
    }
    wrefresh( w );
}

// be explicit with the values
enum aim_entry {
    ENTRY_START = 0,
    ENTRY_VEHICLE = 1,
    ENTRY_MAP = 2,
    ENTRY_RESET = 3
};

bool comestible_inventory::show_sort_menu( comestible_inventory_pane &pane )
{
    uilist sm;
    sm.text = _( "Sort by... " );
    sm.addentry( COMESTIBLE_SORTBY_NAME, true, 'n', get_sortname( COMESTIBLE_SORTBY_NAME ) );
    sm.addentry( COMESTIBLE_SORTBY_WEIGHT, true, 'w', get_sortname( COMESTIBLE_SORTBY_WEIGHT ) );
    sm.addentry( COMESTIBLE_SORTBY_VOLUME, true, 'v', get_sortname( COMESTIBLE_SORTBY_VOLUME ) );
    sm.addentry( COMESTIBLE_SORTBY_CALORIES, true, 'c', get_sortname( COMESTIBLE_SORTBY_CALORIES ) );
    sm.addentry( COMESTIBLE_SORTBY_QUENCH, true, 'q', get_sortname( COMESTIBLE_SORTBY_QUENCH ) );
    sm.addentry( COMESTIBLE_SORTBY_JOY, true, 'j', get_sortname( COMESTIBLE_SORTBY_JOY ) );
    sm.addentry( COMESTIBLE_SORTBY_SPOILAGE, true, 's', get_sortname( COMESTIBLE_SORTBY_SPOILAGE ) );
    // Pre-select current sort.
    sm.selected = pane.sortby;
    // Calculate key and window variables, generate window,
    // and loop until we get a valid answer.
    sm.query();

    if( sm.ret < 0 ) {
        return false;
    }
    pane.sortby = static_cast<comestible_inv_sortby>( sm.ret );
    return true;
}

static tripoint aim_vector( aim_location id )
{
    switch( id ) {
        case AIM_SOUTHWEST:
            return tripoint( -1, 1, 0 );
        case AIM_SOUTH:
            return tripoint( 0, 1, 0 );
        case AIM_SOUTHEAST:
            return tripoint( 1, 1, 0 );
        case AIM_WEST:
            return tripoint( -1, 0, 0 );
        case AIM_EAST:
            return tripoint( 1, 0, 0 );
        case AIM_NORTHWEST:
            return tripoint( -1, -1, 0 );
        case AIM_NORTH:
            return tripoint( 0, -1, 0 );
        case AIM_NORTHEAST:
            return tripoint( 1, -1, 0 );
        default:
            return tripoint_zero;
    }
}

void comestible_inventory::display()
{
    init();

    g->u.inv.restack( g->u );

    input_context ctxt( "COMESTIBLE_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "TOGGLE_VEH" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TOGGLE_AUTO_PICKUP" );
    ctxt.register_action( "TOGGLE_FAVORITE" );
    ctxt.register_action( "MOVE_SINGLE_ITEM" );
    ctxt.register_action( "MOVE_VARIABLE_ITEM" );
    ctxt.register_action( "MOVE_ITEM_STACK" );
    ctxt.register_action( "CATEGORY_SELECTION" );
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
    ctxt.register_action( "ITEMS_DRAGGED_CONTAINER" );
    ctxt.register_action( "ITEMS_CONTAINER" );

    ctxt.register_action( "ITEMS_DEFAULT" );
    ctxt.register_action( "SAVE_DEFAULT" );

    ctxt.register_action( "SWITCH_FOOD" );
    ctxt.register_action( "HEAT_UP" );

    exit = false;
    recalc = true;
    redraw = true;

    while( !exit ) {
        if( g->u.moves < 0 ) {
            do_return_entry();
            return;
        }

        redraw_pane();

        if( redraw && !is_processing() ) {
            werase( head );
            werase( minimap );
            werase( mm_border );
            draw_border( head );
            Messages::display_messages( head, 2, 1, w_width - 1, head_height - 2 );
            draw_minimap();
            const std::string msg = _( "< [?] show help >" );
            mvwprintz( head, 0,
                       w_width - ( minimap_width + 2 ) - utf8_width( msg ) - 1,
                       c_white, msg );
            if( g->u.has_watch() ) {
                const std::string time = to_string_time_of_day( calendar::turn );
                mvwprintz( head, 0, 2, c_white, time );
            }
            wrefresh( head );
            refresh_minimap();
        }
        redraw = false;
        recalc = false;

        // current item in source pane, might be null
        comestible_inv_listitem *sitem = pane.get_cur_item_ptr();
        aim_location changeSquare = NUM_AIM_LOCATIONS;

        const std::string action = ctxt.handle_input();
        if( action == "CATEGORY_SELECTION" ) {
            inCategoryMode = !inCategoryMode;
            pane.redraw = true; // We redraw to force the color change of the highlighted line and header text.
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( action == "ITEMS_DEFAULT" ) {
            aim_location location = static_cast<aim_location>( uistate.adv_inv_default_areas[0] );
            if( pane.get_area() != location || location == AIM_ALL ) {
                pane.recalc = true;
            }
            pane.set_area( squares[location] );
            redraw = true;
        } else if( action == "SAVE_DEFAULT" ) {
            uistate.adv_inv_default_areas[0] = pane.get_area();
            popup( _( "Default layout was saved." ) );
            redraw = true;
        } else if( get_square( action, changeSquare ) ) {
            if( pane.get_area() == changeSquare ) {
                //DO NOTHING
            } else if( squares[changeSquare].canputitems( pane.get_cur_item_ptr() ) ) {
                bool in_vehicle_cargo = false;
                if( changeSquare == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( pane.get_cur_item_ptr() );
                } else if( pane.get_area() == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( nullptr );
                    // auto select vehicle if items exist at said square, or both are empty
                } else if( squares[changeSquare].can_store_in_vehicle() ) {
                    if( changeSquare == AIM_DRAGGED ) {
                        in_vehicle_cargo = true;
                    } else {
                        // check item stacks in vehicle and map at said square
                        auto sq = squares[changeSquare];
                        auto map_stack = g->m.i_at( sq.pos );
                        auto veh_stack = sq.veh->get_items( sq.vstor );
                        // auto switch to vehicle storage if vehicle items are there, or neither are there
                        if( !veh_stack.empty() || map_stack.empty() ) {
                            in_vehicle_cargo = true;
                        }
                    }
                }
                pane.set_area( squares[changeSquare], in_vehicle_cargo );
                pane.index = 0;
                pane.recalc = true;
                redraw = true;
            } else {
                popup( _( "You can't put items there!" ) );
                redraw = true; // to clear the popup
            }
        } else if( action == "TOGGLE_FAVORITE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            for( auto *it : sitem->items ) {
                it->set_favorite( !it->is_favorite );
            }
            recalc = true; // In case we've merged faved and unfaved items
            redraw = true;
        } else if( action == "MOVE_SINGLE_ITEM" ||
                   action == "MOVE_VARIABLE_ITEM" ||
                   action == "MOVE_ITEM_STACK" ) {

        } else if( action == "SORT" ) {
            if( show_sort_menu( pane ) ) {
                recalc = true;

                //TODO: find all uistate, no settings?
                uistate.adv_inv_sort[0] = pane.sortby;
            }
            redraw = true;
        } else if( action == "FILTER" ) {
            string_input_popup spopup;
            std::string filter = pane.filter;
            filter_edit = true;
            spopup.window( pane.window, 4, w_height - 1, w_width / 2 - 4 )
            .max_length( 256 )
            .text( filter );

            draw_item_filter_rules( pane.window, 1, 11, item_filter_type::FILTER );

#if defined(__ANDROID__)
            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                SDL_StartTextInput();
            }
#endif

            do {
                mvwprintz( pane.window, getmaxy( pane.window ) - 1, 2, c_cyan, "< " );
                mvwprintz( pane.window, getmaxy( pane.window ) - 1, w_width / 2 - 4, c_cyan, " >" );
                std::string new_filter = spopup.query_string( false );
                if( spopup.context().get_raw_input().get_first_input() == KEY_ESCAPE ) {
                    // restore original filter
                    pane.set_filter( filter );
                } else {
                    pane.set_filter( new_filter );
                }
                redraw_pane();
            } while( spopup.context().get_raw_input().get_first_input() != '\n' &&
                     spopup.context().get_raw_input().get_first_input() != KEY_ESCAPE );
            filter_edit = false;
            pane.redraw = true;
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
            recalc = true;
        } else if( action == "EXAMINE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            int ret = 0;
            const int info_width = w_width / 2;
            const int info_startx = colstart + info_width;
            if( pane.get_area() == AIM_INVENTORY || pane.get_area() == AIM_WORN ) {
                int idx = pane.get_area() == AIM_INVENTORY ? sitem->idx :
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
                if( pane.get_area() == AIM_INVENTORY ) {
                    g->u.inv.restack( g->u );
                }
                recalc = true;
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
            redraw = true; // item info window overwrote the other pane and the header
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "PAGE_DOWN" ) {
            pane.scroll_by( +itemsPerPage );
        } else if( action == "PAGE_UP" ) {
            pane.scroll_by( -itemsPerPage );
        } else if( action == "DOWN" ) {
            pane.scroll_by( +1, inCategoryMode );
        } else if( action == "UP" ) {
            pane.scroll_by( -1, inCategoryMode );
        } else if( action == "TOGGLE_VEH" ) {
            if( squares[pane.get_area()].can_store_in_vehicle() ) {
                // swap the panes if going vehicle will show the same tile
                //if (spane.get_area() == dpane.get_area() && spane.in_vehicle() != dpane.in_vehicle()) {
                //  swap_panes();
                //  // disallow for dragged vehicles
                //}
                //else
                if( pane.get_area() != AIM_DRAGGED ) {
                    // Toggle between vehicle and ground
                    pane.set_area( squares[pane.get_area()], !pane.in_vehicle() );
                    pane.index = 0;
                    pane.recalc = true;
                    // make sure to update the minimap as well!
                    redraw = true;
                }
            } else {
                popup( _( "No vehicle there!" ) );
                redraw = true;
            }
        } else if( action == "SWITCH_FOOD" ) {
            pane.filter_show_food = !pane.filter_show_food;
            recalc = true;
        } else if( action == "HEAT_UP" ) {
            heat_up( sitem->items.front() );
        }
    }
}

void comestible_inventory_pane::skip_category_headers( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( static_cast<size_t>( index ) < items.size() ); // valid index is required
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    assert( !items.empty() ); // index would not be valid, and this would be an endless loop
    while( !items[index].is_item_entry() ) {
        mod_index( offset );
    }
}

void comestible_inventory_pane::mod_index( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( !items.empty() );
    index += offset;
    if( index < 0 ) {
        index = static_cast<int>( items.size() ) - 1;
    } else if( static_cast<size_t>( index ) >= items.size() ) {
        index = 0;
    }
}

void comestible_inventory_pane::scroll_by( int offset, bool scroll_by_category )
{
    assert( offset != 0 ); // 0 would make no sense
    if( items.empty() ) {
        return;
    }
    if( scroll_by_category ) {
        assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
        assert( get_cur_item_ptr() != nullptr ); // index must already be valid!
        auto cur_cat = items[index].cat;
        if( offset > 0 ) {
            while( items[index].cat == cur_cat ) {
                index++;
                if( static_cast<size_t>( index ) >= items.size() ) {
                    index = 0; // wrap to begin, stop there.
                    break;
                }
            }
        } else {
            while( items[index].cat == cur_cat ) {
                index--;
                if( index < 0 ) {
                    index = static_cast<int>( items.size() ) - 1; // wrap to end, stop there.
                    break;
                }
            }
        }
    } else {
        mod_index( offset );
    }
    // Make sure we land on an item entry.
    skip_category_headers( offset > 0 ? +1 : -1 );
    redraw = true;
}

comestible_inv_listitem *comestible_inventory_pane::get_cur_item_ptr()
{
    if( static_cast<size_t>( index ) >= items.size() ) {
        return nullptr;
    }
    return &items[index];
}

void comestible_inventory_pane::set_filter( const std::string &new_filter )
{
    if( filter == new_filter ) {
        return;
    }
    filter = new_filter;
    filtercache.clear();
    recalc = true;
}

bool comestible_inventory::move_content( item &src_container, item &dest_container )
{
    if( !src_container.is_container() ) {
        popup( _( "Source must be container." ) );
        return false;
    }
    if( src_container.is_container_empty() ) {
        popup( _( "Source container is empty." ) );
        return false;
    }

    item &src_contents = src_container.contents.front();

    if( !src_contents.made_of( LIQUID ) ) {
        popup( _( "You can unload only liquids into target container." ) );
        return false;
    }

    std::string err;
    // TODO: Allow buckets here, but require them to be on the ground or wielded
    const int amount = dest_container.get_remaining_capacity_for_liquid( src_contents, false, &err );
    if( !err.empty() ) {
        popup( err );
        return false;
    }
    if( src_container.is_non_resealable_container() ) {
        if( src_contents.charges > amount ) {
            popup( _( "You can't partially unload liquids from unsealable container." ) );
            return false;
        }
        src_container.on_contents_changed();
    }
    dest_container.fill_with( src_contents, amount );

    uistate.adv_inv_container_content_type = dest_container.contents.front().typeId();
    if( src_contents.charges <= 0 ) {
        src_container.contents.clear();
    }

    return true;
}

units::volume comestible_inv_area::free_volume( bool in_vehicle ) const
{
    assert( id != AIM_ALL ); // should be a specific location instead
    if( id == AIM_INVENTORY || id == AIM_WORN ) {
        return g->u.volume_capacity() - g->u.volume_carried();
    }
    return in_vehicle ? veh->free_volume( vstor ) : g->m.free_volume( pos );
}
//
//bool comestible_inventory::query_charges(aim_location destarea, const comestible_inv_listitem& sitem,
//  const std::string& action, int& amount)
//{
//  assert(destarea != AIM_ALL); // should be a specific location instead
//  assert(!sitem.items.empty()); // valid item is obviously required
//  const item& it = *sitem.items.front();
//  comestible_inv_area& p = squares[destarea];
//  const bool by_charges = it.count_by_charges();
//  const units::volume free_volume = p.free_volume(panes[dest].in_vehicle());
//  // default to move all, unless if being equipped
//  const int input_amount = by_charges ? it.charges : action == "MOVE_SINGLE_ITEM" ? 1 : sitem.stacks;
//  assert(input_amount > 0); // there has to be something to begin with
//  amount = input_amount;
//
//  // Includes moving from/to inventory and around on the map.
//  if (it.made_of_from_type(LIQUID)) {
//      popup(_("You can't pick up a liquid."));
//      redraw = true;
//      return false;
//  }
//
//  // Check volume, this should work the same for inventory, map and vehicles, but not for worn
//  const int room_for = it.charges_per_volume(free_volume);
//  if (amount > room_for && squares[destarea].id != AIM_WORN) {
//      if (room_for <= 0) {
//          popup(_("Destination area is full.  Remove some items first."));
//          redraw = true;
//          return false;
//      }
//      amount = std::min(room_for, amount);
//  }
//  // Map and vehicles have a maximal item count, check that. Inventory does not have this.
//  if (destarea != AIM_INVENTORY &&
//      destarea != AIM_WORN &&
//      destarea != AIM_CONTAINER) {
//      const int cntmax = p.max_size - p.get_item_count();
//      // For items counted by charges, adding it adds 0 items if something there stacks with it.
//      const bool adds0 = by_charges && std::any_of(pane.items.begin(), pane.items.end(),
//          [&it](const comestible_inv_listitem& li) {
//              return li.is_item_entry() && li.items.front()->stacks_with(it);
//          });
//      if (cntmax <= 0 && !adds0) {
//          popup(_("Destination area has too many items.  Remove some first."));
//          redraw = true;
//          return false;
//      }
//      // Items by charge count as a single item, regardless of the charges. As long as the
//      // destination can hold another item, one can move all charges.
//      if (!by_charges) {
//          amount = std::min(cntmax, amount);
//      }
//  }
//  // Inventory has a weight capacity, map and vehicle don't have that
//  if (destarea == AIM_INVENTORY || destarea == AIM_WORN) {
//      const units::mass unitweight = it.weight() / (by_charges ? it.charges : 1);
//      const units::mass max_weight = g->u.has_trait(trait_id("DEBUG_STORAGE")) ?
//          units::mass_max : g->u.weight_capacity() * 4 - g->u.weight_carried();
//      if (unitweight > 0_gram && unitweight * amount > max_weight) {
//          const int weightmax = max_weight / unitweight;
//          if (weightmax <= 0) {
//              popup(_("This is too heavy!"));
//              redraw = true;
//              return false;
//          }
//          amount = std::min(weightmax, amount);
//      }
//  }
//  // handle how many of armor type we can equip (max of 2 per type)
//  if (destarea == AIM_WORN) {
//      const auto& id = sitem.items.front()->typeId();
//      // how many slots are available for the item?
//      const int slots_available = MAX_WORN_PER_TYPE - g->u.amount_worn(id);
//      // base the amount to equip on amount of slots available
//      amount = std::min(slots_available, input_amount);
//  }
//  // Now we have the final amount. Query if requested or limited room left.
//  if (action == "MOVE_VARIABLE_ITEM" || amount < input_amount) {
//      const int count = by_charges ? it.charges : sitem.stacks;
//      const char* msg = nullptr;
//      std::string popupmsg;
//      if (amount >= input_amount) {
//          msg = _("How many do you want to move? [Have %d] (0 to cancel)");
//          popupmsg = string_format(msg, count);
//      }
//      else {
//          msg = _("Destination can only hold %d! Move how many? [Have %d] (0 to cancel)");
//          popupmsg = string_format(msg, amount, count);
//      }
//      // At this point amount contains the maximal amount that the destination can hold.
//      const int possible_max = std::min(input_amount, amount);
//      if (amount <= 0) {
//          popup(_("The destination is already full!"));
//      }
//      else {
//          amount = string_input_popup()
//              .title(popupmsg)
//              .width(20)
//              .only_digits(true)
//              .query_int();
//      }
//      if (amount <= 0) {
//          redraw = true;
//          return false;
//      }
//      if (amount > possible_max) {
//          amount = possible_max;
//      }
//  }
//  return true;
//}

bool comestible_inv_area::is_same( const comestible_inv_area &other ) const
{
    // All locations (sans the below) are compared by the coordinates,
    // e.g. dragged vehicle (to the south) and AIM_SOUTH are the same.
    if( id != AIM_INVENTORY && other.id != AIM_INVENTORY &&
        id != AIM_WORN && other.id != AIM_WORN &&
        id != AIM_CONTAINER && other.id != AIM_CONTAINER ) {
        //     have a vehicle?...     ...do the cargo index and pos match?...    ...at least pos?
        return veh == other.veh ? pos == other.pos && vstor == other.vstor : pos == other.pos;
    }
    //      ...is the id?
    return id == other.id;
}

bool comestible_inv_area::canputitems( const comestible_inv_listitem *advitem )
{
    bool canputitems = false;
    bool from_vehicle = false;
    item *it = nullptr;
    switch( id ) {
        case AIM_CONTAINER:
            if( advitem != nullptr && advitem->is_item_entry() ) {
                it = advitem->items.front();
                from_vehicle = advitem->from_vehicle;
            }
            if( get_container( from_vehicle ) != nullptr ) {
                it = get_container( from_vehicle );
            }
            if( it != nullptr ) {
                canputitems = it->is_watertight_container();
            }
            break;
        default:
            canputitems = canputitemsloc;
            break;
    }
    return canputitems;
}

item *comestible_inv_area::get_container( bool in_vehicle )
{
    item *container = nullptr;

    if( uistate.adv_inv_container_location != -1 ) {
        // try to find valid container in the area
        if( uistate.adv_inv_container_location == AIM_INVENTORY ) {
            const invslice &stacks = g->u.inv.slice();

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                auto &it = stacks[uistate.adv_inv_container_index]->front();
                if( is_container_valid( &it ) ) {
                    container = &it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto &it = stacks[x]->front();
                    if( is_container_valid( &it ) ) {
                        container = &it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        } else if( uistate.adv_inv_container_location == AIM_WORN ) {
            auto &worn = g->u.worn;
            size_t idx = static_cast<size_t>( uistate.adv_inv_container_index );
            if( worn.size() > idx ) {
                auto iter = worn.begin();
                std::advance( iter, idx );
                if( is_container_valid( &*iter ) ) {
                    container = &*iter;
                }
            }

            // no need to reinvent the wheel
            if( container == nullptr ) {
                auto iter = worn.begin();
                for( size_t i = 0; i < worn.size(); ++i, ++iter ) {
                    if( is_container_valid( &*iter ) ) {
                        container = &*iter;
                        uistate.adv_inv_container_index = i;
                        break;
                    }
                }
            }
        } else {
            map &m = g->m;
            bool is_in_vehicle = veh &&
                                 ( uistate.adv_inv_container_in_vehicle || ( can_store_in_vehicle() && in_vehicle ) );

            const itemstack &stacks = is_in_vehicle ?
                                      i_stacked( veh->get_items( vstor ) ) :
                                      i_stacked( m.i_at( pos ) );

            // check index first
            if( stacks.size() > static_cast<size_t>( uistate.adv_inv_container_index ) ) {
                auto it = stacks[uistate.adv_inv_container_index].front();
                if( is_container_valid( it ) ) {
                    container = it;
                }
            }

            // try entire area
            if( container == nullptr ) {
                for( size_t x = 0; x < stacks.size(); ++x ) {
                    auto it = stacks[x].front();
                    if( is_container_valid( it ) ) {
                        container = it;
                        uistate.adv_inv_container_index = x;
                        break;
                    }
                }
            }
        }

        // no valid container in the area, resetting container
        if( container == nullptr ) {
            set_container( nullptr );
            desc[0] = _( "Invalid container" );
        }
    }

    return container;
}

void comestible_inv_area::set_container( const comestible_inv_listitem *advitem )
{
    if( advitem != nullptr ) {
        item *it( advitem->items.front() );
        uistate.adv_inv_container_location = advitem->area;
        uistate.adv_inv_container_in_vehicle = advitem->from_vehicle;
        uistate.adv_inv_container_index = advitem->idx;
        uistate.adv_inv_container_type = it->typeId();
        uistate.adv_inv_container_content_type = !it->is_container_empty() ?
                it->contents.front().typeId() : "null";
        set_container_position();
    } else {
        uistate.adv_inv_container_location = -1;
        uistate.adv_inv_container_index = 0;
        uistate.adv_inv_container_in_vehicle = false;
        uistate.adv_inv_container_type = "null";
        uistate.adv_inv_container_content_type = "null";
    }
}

bool comestible_inv_area::is_container_valid( const item *it ) const
{
    if( it != nullptr ) {
        if( it->typeId() == uistate.adv_inv_container_type ) {
            if( it->is_container_empty() ) {
                if( uistate.adv_inv_container_content_type == "null" ) {
                    return true;
                }
            } else {
                if( it->contents.front().typeId() == uistate.adv_inv_container_content_type ) {
                    return true;
                }
            }
        }
    }

    return false;
}

void comestible_inv_area::set_container_position()
{
    // update the offset of the container based on location
    if( uistate.adv_inv_container_location == AIM_DRAGGED ) {
        off = g->u.grab_point;
    } else {
        off = aim_vector( static_cast<aim_location>( uistate.adv_inv_container_location ) );
    }
    // update the absolute position
    pos = g->u.pos() + off;
    // update vehicle information
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO",
            false ) ) {
        veh = &vp->vehicle();
        vstor = vp->part_index();
    } else {
        veh = nullptr;
        vstor = -1;
    }
}

void comestible_inv()
{
    comestible_inventory new_inv;
    new_inv.display();
}

void comestible_inventory::refresh_minimap()
{
    // don't update ui if processing demands
    if( is_processing() ) {
        return;
    }
    // redraw border around minimap
    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( pane.get_area() == AIM_ALL ) {
        mvwprintz( mm_border, 0, 1, c_light_gray, utf8_truncate( _( "All" ), minimap_width ) );
    }
    // refresh border, then minimap
    wrefresh( mm_border );
    wrefresh( minimap );
}

void comestible_inventory::draw_minimap()
{
    // if player is in one of the below, invert the player cell
    static const std::array<aim_location, 3> player_locations = {
        {AIM_CENTER, AIM_INVENTORY, AIM_WORN}
    };
    // get the center of the window
    tripoint pc = { getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0 };
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );
    auto sq = squares[pane.get_area()];
    auto pt = pc + sq.off;
    // invert the color if pointing to the player's position
    auto cl = sq.id == AIM_INVENTORY || sq.id == AIM_WORN ?
              invert_color( c_light_cyan ) : c_light_cyan.blink();

    // Invert player's tile color if exactly one pane points to player's tile
    //bool invert_left = false;
    //bool invert_right = false;
    //const auto is_selected = [this](const aim_location& where, size_t side) {
    //  return where == this->panes[side].get_area();
    //};
    //for (auto& loc : player_locations) {
    //  invert_left |= is_selected(loc, 0);
    //  invert_right |= is_selected(loc, 1);
    //}

    //if (!invert_left || !invert_right) {
    //  g->u.draw(minimap, g->u.pos(), invert_left || invert_right);
    //}

    g->u.draw( minimap, g->u.pos(), false );
}

//char comestible_inventory::get_minimap_sym(side p) const
//{
//  static const std::array<char, 2> c_side = { {'L', 'R'} };
//  static const std::array<char, 2> d_side = { {'^', 'v'} };
//  static const std::array<char, NUM_AIM_LOCATIONS> g_nome = { {
//          '@', '#', '#', '#', '#', '@', '#',
//          '#', '#', '#', 'D', '^', 'C', '@'
//      }
//  };
//  char ch = g_nome[pane.get_area()];
//
//
//  //TODO: don't get this code exactly, just simplify for now
//  ch = pane.in_vehicle() ? 'V' : c_side[p];
//
//  switch (ch) {
//  case '@': // '^' or 'v'
//      ch = d_side[panes[-p + 1].get_area() == AIM_CENTER];
//      break;
//  case '#': // 'L' or 'R'
//      ch = panes[p].in_vehicle() ? 'V' : c_side[p];
//      break;
//  case '^': // do not show anything
//      ch ^= ch;
//      break;
//  }
//  return ch;
//}

aim_location comestible_inv_area::offset_to_location() const
{
    static aim_location loc_array[3][3] = {
        {AIM_NORTHWEST,     AIM_NORTH,      AIM_NORTHEAST},
        {AIM_WEST,          AIM_CENTER,     AIM_EAST},
        {AIM_SOUTHWEST,     AIM_SOUTH,      AIM_SOUTHEAST}
    };
    return loc_array[off.y + 1][off.x + 1];
}

void comestible_inventory::do_return_entry()
{
    // only save pane settings
    //save_settings(true);
    g->u.assign_activity( activity_id( "ACT_COMESTIBLE_INVENTORY" ) );
    g->u.activity.auto_resume = true;
    uistate.adv_inv_exit_code = exit_re_entry;
}

bool comestible_inventory::is_processing() const
{
    return uistate.adv_inv_re_enter_move_all != ENTRY_START;
}

aim_location comestible_inventory::screen_relative_location( aim_location area )
{

    if( !( tile_iso && use_tiles ) ) {
        return area;
    }
    switch( area ) {

        case AIM_SOUTHWEST:
            return AIM_WEST;

        case AIM_SOUTH:
            return AIM_SOUTHWEST;

        case AIM_SOUTHEAST:
            return AIM_SOUTH;

        case AIM_WEST:
            return AIM_NORTHWEST;

        case AIM_EAST:
            return AIM_SOUTHEAST;

        case AIM_NORTHWEST:
            return AIM_NORTH;

        case AIM_NORTH:
            return AIM_NORTHEAST;

        case AIM_NORTHEAST:
            return AIM_EAST;

        default:
            return area;
    }
}

char *comestible_inventory::set_string_params( nc_color &print_color, int value, bool selected,
        bool need_highlight )
{
    char *string_format;
    if( value > 0 ) {
        print_color = need_highlight ? c_yellow_green : c_green;
        string_format = "+%d";
    } else if( value < 0 ) {
        print_color = need_highlight ? c_yellow_red : c_red;
        string_format = "%d";
    } else {
        print_color = need_highlight ? c_yellow : c_dark_gray;
        string_format = "";
    }

    if( selected ) {
        print_color = hilite( c_white );
    }
    return string_format;
}

const islot_comestible &comestible_inventory::get_edible_comestible( player &p,
        const item &it ) const
{
    if( it.is_comestible() && p.can_eat( it ).success() ) {
        // Ok since can_eat() returns false if is_craft() is true
        return *it.type->comestible;
    }
    static const islot_comestible dummy{};
    return dummy;
}

time_duration comestible_inventory::get_time_left( player &p, const item &it ) const
{
    time_duration time_left = 0_turns;
    const time_duration shelf_life = get_edible_comestible( p, it ).spoils;
    if( shelf_life > 0_turns ) {
        const double relative_rot = it.get_relative_rot();
        time_left = shelf_life - shelf_life * relative_rot;

        // Correct for an estimate that exceeds shelf life -- this happens especially with
        // fresh items.
        if( time_left > shelf_life ) {
            time_left = shelf_life;
        }
    }

    return time_left;
}

std::string comestible_inventory::get_time_left_rounded( player &p, const item &it ) const
{
    if( it.is_going_bad() ) {
        return _( "soon!" );
    }

    time_duration time_left = get_time_left( p, it );

    //const auto make_result = [this](const time_duration& d, const char* verbose_str,
    //  const char* short_str) {
    //      return string_format(false ? verbose_str : short_str, time_to_comestible_str(d));
    //};

    time_duration divider = 0_turns;
    time_duration vicinity = 0_turns;

    if( time_left > 1_days ) {
        divider = 1_days;
        vicinity = 2_hours;
    } else if( time_left > 1_hours ) {
        divider = 1_hours;
        vicinity = 5_minutes;
    } // Minutes and seconds can be estimated precisely.

    if( divider != 0_turns ) {
        const time_duration remainder = time_left % divider;

        if( remainder >= divider - vicinity ) {
            time_left += divider;
        } else if( remainder > vicinity ) {
            if( remainder < divider / 2 ) {
                //~ %s - time (e.g. 2 hours).
                return string_format( "%s", time_to_comestible_str( time_left ) );
                //return string_format("> %s", time_to_comestible_str(time_left));
            } else {
                //~ %s - time (e.g. 2 hours).
                return string_format( "%s", time_to_comestible_str( time_left ) );
                //return string_format("< %s", time_to_comestible_str(time_left));
            }
        }
    }
    //~ %s - time (e.g. 2 hours).
    return string_format( "%s", time_to_comestible_str( time_left ) );
    //return string_format("~ %s", time_to_comestible_str(time_left));
}

std::string comestible_inventory::time_to_comestible_str( const time_duration &d ) const
{
    if( d >= calendar::INDEFINITELY_LONG_DURATION ) {
        return _( "forever" );
    }

    const float day = to_days<float>( d );
    std::string format;
    if( day < 1.00 ) {
        format = _( "%.2f days" );
    } else if( day > 1.00 ) {
        format = _( "%.0f days" );
    } else {
        format = _( "1 day" );
    }

    return string_format( format, day );
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
        redraw = true;
        //p.add_msg_if_player(m_info, _("You don't have a suitable container or hotplate with enough charges."));
        return;
    }

    if( has_heat_item && !can_use_hotplate ) {
        if( !is_near_fire && !can_use_bio ) {
            popup( _( "You need to be near fire to heat an item in a container." ) );
            redraw = true;
            //p.add_msg_if_player(m_info, _("You need to be near fire to heat an item in a container."));
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
            sm.addentry( counter, true, counter, _( "use bio tools" ) );
            counter++;
        }

        for each( item * var in hotplates ) {
            sm.addentry( counter, true, counter, string_format( _( "%s in inventory" ), var->display_name() ) );
            counter++;
        }
        for each( item * var in hotplates_map ) {
            sm.addentry( counter, true, counter, string_format( _( "%s nearby" ), var->display_name() ) );
            counter++;
        }
        //sm.selected = pane.sortby - SORTBY_NONE;
        sm.query();

        if( sm.ret < 0 ) {
            //'Never mind'
            redraw = true;
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
            if( choice < hotplates.size() ) {
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

//void cancel_aim_processing()
//{
//  uistate.adv_inv_re_enter_move_all = ENTRY_START;
//}
