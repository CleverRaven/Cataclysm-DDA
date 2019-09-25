#include "advanced_inv.h"

#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "ime.h"
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

enum aim_exit {
    exit_none = 0,
    exit_okay,
    exit_re_entry
};

// *INDENT-OFF*
advanced_inventory::advanced_inventory()
    : head_height( 5 )
    , min_w_height( 10 )
    , min_w_width( FULL_SCREEN_WIDTH )
    , max_w_width( 120 )
    , inCategoryMode( false )
    , recalc( true )
    , redraw( true )
    , src( left )
    , dest( right )
    , filter_edit( false )
      // panes don't need initialization, they are recalculated immediately
    , squares( {
    {
        //               hx  hy
        { AIM_INVENTORY, 25, 2, tripoint_zero,       _( "Inventory" ),          _( "IN" ) },
        { AIM_SOUTHWEST, 30, 3, tripoint_south_west, _( "South West" ),         _( "SW" ) },
        { AIM_SOUTH,     33, 3, tripoint_south,      _( "South" ),              _( "S" )  },
        { AIM_SOUTHEAST, 36, 3, tripoint_south_east, _( "South East" ),         _( "SE" ) },
        { AIM_WEST,      30, 2, tripoint_west,       _( "West" ),               _( "W" )  },
        { AIM_CENTER,    33, 2, tripoint_zero,       _( "Directly below you" ), _( "DN" ) },
        { AIM_EAST,      36, 2, tripoint_east,       _( "East" ),               _( "E" )  },
        { AIM_NORTHWEST, 30, 1, tripoint_north_west, _( "North West" ),         _( "NW" ) },
        { AIM_NORTH,     33, 1, tripoint_north,      _( "North" ),              _( "N" )  },
        { AIM_NORTHEAST, 36, 1, tripoint_north_east, _( "North East" ),         _( "NE" ) },
        { AIM_DRAGGED,   25, 1, tripoint_zero,       _( "Grabbed Vehicle" ),    _( "GR" ) },
        { AIM_ALL,       22, 3, tripoint_zero,       _( "Surrounding area" ),   _( "AL" ) },
        { AIM_CONTAINER, 22, 1, tripoint_zero,       _( "Container" ),          _( "CN" ) },
        { AIM_WORN,      25, 3, tripoint_zero,       _( "Worn Items" ),         _( "WR" ) }
    }
} )
{
    // initialize screen coordinates for small overview 3x3 grid, depending on control scheme
    if( tile_iso && use_tiles ) {
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

advanced_inventory::~advanced_inventory()
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
        werase( left_window );
        werase( right_window );
        g->refresh_all();
        g->u.check_item_encumbrance_flag();
    }
}

void advanced_inventory::save_settings( bool only_panes )
{
    if( !only_panes ) {
        uistate.adv_inv_last_coords = g->u.pos();
        uistate.adv_inv_src = src;
        uistate.adv_inv_dest = dest;
    }
    for( int i = 0; i < NUM_PANES; ++i ) {
        uistate.adv_inv_in_vehicle[i] = panes[i].in_vehicle();
        uistate.adv_inv_area[i] = panes[i].get_area();
        uistate.adv_inv_index[i] = panes[i].index;
        uistate.adv_inv_filter[i] = panes[i].filter;
    }
}

void advanced_inventory::load_settings()
{
    aim_exit aim_code = static_cast<aim_exit>( uistate.adv_inv_exit_code );
    for( int i = 0; i < NUM_PANES; ++i ) {
        aim_location location;
        if( get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) ) {
            location = static_cast<aim_location>( uistate.adv_inv_default_areas[i] );
        } else {
            location = static_cast<aim_location>( uistate.adv_inv_area[i] );
        }
        auto square = squares[location];
        // determine the square's vehicle/map item presence
        bool has_veh_items = square.can_store_in_vehicle() ?
                             !square.veh->get_items( square.vstor ).empty() : false;
        bool has_map_items = !g->m.i_at( square.pos ).empty();
        // determine based on map items and settings to show cargo
        bool show_vehicle = aim_code == exit_re_entry ?
                            uistate.adv_inv_in_vehicle[i] : has_veh_items ? true :
                            has_map_items ? false : square.can_store_in_vehicle();
        panes[i].set_area( square, show_vehicle );
        panes[i].sortby = static_cast<advanced_inv_sortby>( uistate.adv_inv_sort[i] );
        panes[i].index = uistate.adv_inv_index[i];
        panes[i].filter = uistate.adv_inv_filter[i];
    }
    uistate.adv_inv_exit_code = exit_none;
}

std::string advanced_inventory::get_sortname( advanced_inv_sortby sortby )
{
    switch( sortby ) {
        case SORTBY_NONE:
            return _( "none" );
        case SORTBY_NAME:
            return _( "name" );
        case SORTBY_WEIGHT:
            return _( "weight" );
        case SORTBY_VOLUME:
            return _( "volume" );
        case SORTBY_CHARGES:
            return _( "charges" );
        case SORTBY_CATEGORY:
            return _( "category" );
        case SORTBY_DAMAGE:
            return _( "damage" );
        case SORTBY_AMMO:
            return _( "ammo/charge type" );
        case SORTBY_SPOILAGE:
            return _( "spoilage" );
    }
    return "!BUG!";
}

bool advanced_inventory::get_square( const std::string &action, aim_location &ret )
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

void advanced_inventory::print_items( advanced_inventory_pane &pane, bool active )
{
    const auto &items = pane.items;
    const catacurses::window &window = pane.window;
    const auto index = pane.index;
    const int page = index / itemsPerPage;
    bool compact = TERMX <= 100;

    int columns = getmaxx( window );
    std::string spaces( columns - 4, ' ' );

    nc_color norm = active ? c_white : c_dark_gray;

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
        const int hrightcol = columns - 1 - formatted_head.length();
        nc_color color = weight_carried > weight_capacity ? c_red : c_light_green;
        mvwprintz( window, point( hrightcol, 4 ), color, "%.1f", weight_carried );
        wprintz( window, c_light_gray, "/%.1f %s  ", weight_capacity, weight_units() );
        color = g->u.volume_carried().value() > g->u.volume_capacity().value() ? c_red : c_light_green;
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
        mvwprintz( window, point( columns - 1 - formatted_head.length(), 4 ), norm, formatted_head );
    }

    //print header row and determine max item name length
    const int lastcol = columns - 2; // Last printable column
    const size_t name_startpos = compact ? 1 : 4;
    const size_t src_startpos = lastcol - 18;
    const size_t amt_startpos = lastcol - 15;
    const size_t weight_startpos = lastcol - 10;
    const size_t vol_startpos = lastcol - 4;
    int max_name_length = amt_startpos - name_startpos - 1; // Default name length

    //~ Items list header. Table fields length without spaces: amt - 4, weight - 5, vol - 4.
    const int table_hdr_len1 = utf8_width( _( "amt weight vol" ) ); // Header length type 1
    //~ Items list header. Table fields length without spaces: src - 2, amt - 4, weight - 5, vol - 4.
    const int table_hdr_len2 = utf8_width( _( "src amt weight vol" ) ); // Header length type 2

    mvwprintz( window, point( compact ? 1 : 4, 5 ), c_light_gray, _( "Name (charges)" ) );
    if( pane.get_area() == AIM_ALL && !compact ) {
        mvwprintz( window, point( lastcol - table_hdr_len2 + 1, 5 ), c_light_gray,
                   _( "src amt weight vol" ) );
        max_name_length = src_startpos - name_startpos - 1; // 1 for space
    } else {
        mvwprintz( window, point( lastcol - table_hdr_len1 + 1, 5 ), c_light_gray, _( "amt weight vol" ) );
    }

    for( int i = page * itemsPerPage, x = 0 ; i < static_cast<int>( items.size() ) &&
         x < itemsPerPage ; i++, x++ ) {
        const auto &sitem = items[i];
        if( sitem.is_category_header() ) {
            mvwprintz( window, point( ( columns - utf8_width( sitem.name ) - 6 ) / 2, 6 + x ), c_cyan, "[%s]",
                       sitem.name );
            continue;
        }
        if( !sitem.is_item_entry() ) {
            // Empty entry at the bottom of a page.
            continue;
        }
        const auto &it = *sitem.items.front();
        const bool selected = active && index == i;

        nc_color thiscolor = active ? it.color_in_inventory() : norm;
        nc_color thiscolordark = c_dark_gray;
        nc_color print_color;

        if( selected ) {
            thiscolor = inCategoryMode && pane.sortby == SORTBY_CATEGORY ? c_white_red : hilite( c_white );
            thiscolordark = hilite( thiscolordark );
            if( compact ) {
                mvwprintz( window, point( 1, 6 + x ), thiscolor, "  %s", spaces );
            } else {
                mvwprintz( window, point( 1, 6 + x ), thiscolor, ">>%s", spaces );
            }
        }

        std::string item_name;
        std::string stolen_string;
        bool stolen = false;
        if( !it.is_owned_by( g->u, true ) ) {
            stolen_string = "<color_light_red>!</color>";
            stolen = true;
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
                item_name = it.display_name();
            }
        }
        if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
            item_name = string_format( "%s %s", it.symbol(), item_name );
        }

        //print item name
        trim_and_print( window, point( compact ? 1 : 4, 6 + x ), max_name_length, thiscolor, item_name );

        //print src column
        // TODO: specify this is coming from a vehicle!
        if( pane.get_area() == AIM_ALL && !compact ) {
            mvwprintz( window, point( src_startpos, 6 + x ), thiscolor, squares[sitem.area].shortname );
        }

        //print "amount" column
        int it_amt = sitem.stacks;
        if( it_amt > 1 ) {
            print_color = thiscolor;
            if( it_amt > 9999 ) {
                it_amt = 9999;
                print_color = selected ? hilite( c_red ) : c_red;
            }
            mvwprintz( window, point( amt_startpos, 6 + x ), print_color, "%4d", it_amt );
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
        mvwprintz( window, point( weight_startpos, 6 + x ), print_color, "%5.*f", w_precision, it_weight );

        //print volume column
        bool it_vol_truncated = false;
        double it_vol_value = 0.0;
        std::string it_vol = format_volume( sitem.volume, 5, &it_vol_truncated, &it_vol_value );
        if( it_vol_truncated && it_vol_value > 0.0 ) {
            print_color = selected ? hilite( c_red ) : c_red;
        } else {
            print_color = sitem.volume.value() > 0 ? thiscolor : thiscolordark;
        }
        mvwprintz( window, point( vol_startpos, 6 + x ), print_color, it_vol );

        if( active && sitem.autopickup ) {
            mvwprintz( window, point( 1, 6 + x ), magenta_background( it.color_in_inventory() ),
                       compact ? it.tname().substr( 0, 1 ) : ">" );
        }
    }
}

struct advanced_inv_sorter {
    advanced_inv_sortby sortby;
    advanced_inv_sorter( advanced_inv_sortby sort ) {
        sortby = sort;
    }
    bool operator()( const advanced_inv_listitem &d1, const advanced_inv_listitem &d2 ) {
        // Note: the item pointer can only be null on sort by category, otherwise it is always valid.
        switch( sortby ) {
            case SORTBY_NONE:
                if( d1.idx != d2.idx ) {
                    return d1.idx < d2.idx;
                }
                break;
            case SORTBY_NAME:
                // Fall through to code below the switch
                break;
            case SORTBY_WEIGHT:
                if( d1.weight != d2.weight ) {
                    return d1.weight > d2.weight;
                }
                break;
            case SORTBY_VOLUME:
                if( d1.volume != d2.volume ) {
                    return d1.volume > d2.volume;
                }
                break;
            case SORTBY_CHARGES:
                if( d1.items.front()->charges != d2.items.front()->charges ) {
                    return d1.items.front()->charges > d2.items.front()->charges;
                }
                break;
            case SORTBY_CATEGORY:
                assert( d1.cat != nullptr );
                assert( d2.cat != nullptr );
                if( d1.cat != d2.cat ) {
                    return *d1.cat < *d2.cat;
                } else if( d1.is_category_header() ) {
                    return true;
                } else if( d2.is_category_header() ) {
                    return false;
                }
                break;
            case SORTBY_DAMAGE:
                if( d1.items.front()->damage() != d2.items.front()->damage() ) {
                    return d1.items.front()->damage() < d2.items.front()->damage();
                }
                break;
            case SORTBY_AMMO: {
                const std::string a1 = d1.items.front()->ammo_sort_name();
                const std::string a2 = d2.items.front()->ammo_sort_name();
                // There are many items with "false" ammo types (e.g.
                // scrap metal has "components") that actually is not
                // used as ammo, so we consider them as non-ammo.
                const bool ammoish1 = !a1.empty() && a1 != "components" && a1 != "none" && a1 != "NULL";
                const bool ammoish2 = !a2.empty() && a2 != "components" && a2 != "none" && a2 != "NULL";
                if( ammoish1 != ammoish2 ) {
                    return ammoish1;
                } else if( ammoish1 && ammoish2 ) {
                    if( a1 == a2 ) {
                        // For items with the same ammo type, we sort:
                        // guns > tools > magazines > ammunition
                        if( d1.items.front()->is_gun() && !d2.items.front()->is_gun() ) {
                            return true;
                        }
                        if( !d1.items.front()->is_gun() && d2.items.front()->is_gun() ) {
                            return false;
                        }
                        if( d1.items.front()->is_tool() && !d2.items.front()->is_tool() ) {
                            return true;
                        }
                        if( !d1.items.front()->is_tool() && d2.items.front()->is_tool() ) {
                            return false;
                        }
                        if( d1.items.front()->is_magazine() && d2.items.front()->is_ammo() ) {
                            return true;
                        }
                        if( d2.items.front()->is_magazine() && d1.items.front()->is_ammo() ) {
                            return false;
                        }
                    }
                    return std::lexicographical_compare( a1.begin(), a1.end(),
                                                         a2.begin(), a2.end(),
                                                         sort_case_insensitive_less() );
                }
            }
            break;
            case SORTBY_SPOILAGE:
                if( d1.items.front()->spoilage_sort_order() != d2.items.front()->spoilage_sort_order() ) {
                    return d1.items.front()->spoilage_sort_order() < d2.items.front()->spoilage_sort_order();
                }
                break;
        }
        // secondary sort by name
        const std::string *n1;
        const std::string *n2;
        if( d1.name_without_prefix == d2.name_without_prefix ) {
            //if names without prefix equal, compare full name
            n1 = &d1.name;
            n2 = &d2.name;
        } else {
            //else compare name without prefix
            n1 = &d1.name_without_prefix;
            n2 = &d2.name_without_prefix;
        }
        return std::lexicographical_compare( n1->begin(), n1->end(),
                                             n2->begin(), n2->end(), sort_case_insensitive_less() );
    }
};

inline char advanced_inventory::get_location_key( aim_location area )
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

char advanced_inventory::get_direction_key( aim_location area )
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

int advanced_inventory::print_header( advanced_inventory_pane &pane, aim_location sel )
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
        mvwprintz( window, point( x, y ), bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%c", in_vehicle && sel != AIM_DRAGGED ? 'V' : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }
    return squares[AIM_INVENTORY].hscreen.y + ofs;
}

int advanced_inv_area::get_item_count() const
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

void advanced_inv_area::init()
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

void advanced_inventory::init()
{
    for( auto &square : squares ) {
        square.init();
    }

    load_settings();

    src  = static_cast<side>( uistate.adv_inv_src );
    dest = static_cast<side>( uistate.adv_inv_dest );

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
    left_window = catacurses::newwin( w_height, w_width / 2, point( colstart,
                                      headstart + head_height ) );
    right_window = catacurses::newwin( w_height, w_width / 2, point( colstart + w_width / 2,
                                       headstart + head_height ) );

    itemsPerPage = w_height - 2 - 5; // 2 for the borders, 5 for the header stuff

    panes[left].window = left_window;
    panes[right].window = right_window;
}

advanced_inv_listitem::advanced_inv_listitem( item *an_item, int index, int count,
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
    items.push_back( an_item );
    assert( stacks >= 1 );
}

advanced_inv_listitem::advanced_inv_listitem( const std::list<item *> &list, int index,
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
    assert( stacks >= 1 );
}

advanced_inv_listitem::advanced_inv_listitem()
    : idx()
    , area()
    , id( "null" )
    , autopickup()
    , stacks()
    , cat( nullptr )
{
}

advanced_inv_listitem::advanced_inv_listitem( const item_category *cat )
    : idx()
    , area()
    , id( "null" )
    , name( cat->name() )
    , autopickup()
    , stacks()
    , cat( cat )
{
}

bool advanced_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool advanced_inv_listitem::is_item_entry() const
{
    return !items.empty();
}

bool advanced_inventory_pane::is_filtered( const advanced_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool advanced_inventory_pane::is_filtered( const item &it ) const
{
    if( filter.empty() ) {
        return false;
    }

    const std::string str = it.tname();
    if( filtercache.find( str ) == filtercache.end() ) {
        const auto filter_fn = item_filter_from_string( filter );
        filtercache[ str ] = filter_fn;

        return !filter_fn( it );
    }

    return !filtercache[ str ]( it );
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
            stacks.push_back( {&elem} );
        }
    }
    return stacks;
}

void advanced_inventory_pane::add_items_from_area( advanced_inv_area &square,
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
            advanced_inv_listitem it( item_pointers, x, square.id, false );
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
            advanced_inv_listitem it( &*iter, i, 1, square.id, false );
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
                advanced_inv_listitem ait( it, 0, 1, square.id, in_vehicle() );
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
            advanced_inv_listitem it( stacks[x], x, square.id, is_in_vehicle );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            square.volume += it.volume;
            square.weight += it.weight;
            items.push_back( it );
        }
    }
}

void advanced_inventory_pane::paginate( size_t itemsPerPage )
{
    if( sortby != SORTBY_CATEGORY ) {
        return; // not needed as there are no category entries here.
    }
    // first, we insert all the items, then we sort the result
    for( size_t i = 0; i < items.size(); ++i ) {
        if( i % itemsPerPage == 0 ) {
            // first entry on the page, should be a category header
            if( items[i].is_item_entry() ) {
                items.insert( items.begin() + i, advanced_inv_listitem( items[i].cat ) );
            }
        }
        if( ( i + 1 ) % itemsPerPage == 0 && i + 1 < items.size() ) {
            // last entry of the page, but not the last entry at all!
            // Must *not* be a category header!
            if( items[i].is_category_header() ) {
                items.insert( items.begin() + i, advanced_inv_listitem() );
            }
        }
    }
}

void advanced_inventory::recalc_pane( side p )
{
    auto &pane = panes[p];
    pane.recalc = false;
    pane.items.clear();
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.
    if( pane.get_area() == AIM_ALL ) {
        auto &alls = squares[AIM_ALL];
        auto &there = panes[-p + 1];
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
    // Insert category headers (only expected when sorting by category)
    if( pane.sortby == SORTBY_CATEGORY ) {
        std::set<const item_category *> categories;
        for( auto &it : pane.items ) {
            categories.insert( it.cat );
        }
        for( auto &cat : categories ) {
            pane.items.push_back( advanced_inv_listitem( cat ) );
        }
    }
    // Finally sort all items (category headers will now be moved to their proper position)
    std::stable_sort( pane.items.begin(), pane.items.end(), advanced_inv_sorter( pane.sortby ) );
    pane.paginate( itemsPerPage );
}

void advanced_inventory_pane::fix_index()
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

void advanced_inventory::redraw_pane( side p )
{
    // don't update ui if processing demands
    if( is_processing() ) {
        return;
    }
    auto &pane = panes[p];
    if( recalc || pane.recalc ) {
        recalc_pane( p );
    } else if( !( redraw || pane.redraw ) ) {
        return;
    }
    pane.redraw = false;
    pane.fix_index();

    const bool active = p == src;
    const advanced_inv_area &square = squares[pane.get_area()];
    auto w = pane.window;

    werase( w );
    print_items( pane, active );

    auto itm = pane.get_cur_item_ptr();
    int width = print_header( pane, itm != nullptr ? itm->area : pane.get_area() );
    bool same_as_dragged = ( square.id >= AIM_SOUTHWEST && square.id <= AIM_NORTHEAST ) &&
                           // only cardinals
                           square.id != AIM_CENTER && panes[p].in_vehicle() && // not where you stand, and pane is in vehicle
                           square.off == squares[AIM_DRAGGED].off; // make sure the offsets are the same as the grab point
    const advanced_inv_area &sq = same_as_dragged ? squares[AIM_DRAGGED] : square;
    bool car = square.can_store_in_vehicle() && panes[p].in_vehicle() && sq.id != AIM_DRAGGED;
    auto name = utf8_truncate( car ? sq.veh->name : sq.name, width );
    auto desc = utf8_truncate( sq.desc[car], width );
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, point( 2, 1 ), active ? c_green  : c_light_gray, name );
    mvwprintz( w, point( 2, 2 ), active ? c_light_blue : c_dark_gray, desc );
    trim_and_print( w, point( 2, 3 ), width, active ? c_cyan : c_dark_gray, square.flags );

    const int max_page = ( pane.items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( active && max_page > 1 ) {
        const int page = pane.index / itemsPerPage;
        mvwprintz( w, point( 2, 4 ), c_light_blue, _( "[<] page %1$d of %2$d [>]" ), page + 1, max_page );
    }

    if( active ) {
        wattron( w, c_cyan );
    }
    // draw a darker border around the inactive pane
    draw_border( w, active ? BORDER_COLOR : c_dark_gray );
    mvwprintw( w, point( 3, 0 ), _( "< [s]ort: %s >" ), get_sortname( pane.sortby ) );
    int max = square.max_size;
    if( max > 0 ) {
        int itemcount = square.get_item_count();
        int fmtw = 7 + ( itemcount > 99 ? 3 : itemcount > 9 ? 2 : 1 ) +
                   ( max > 99 ? 3 : max > 9 ? 2 : 1 );
        mvwprintw( w, point( w_width / 2 - fmtw, 0 ), "< %d/%d >", itemcount, max );
    }

    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );
    if( !filter_edit ) {
        if( !pane.filter.empty() ) {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s: %s >", fprefix, pane.filter );
        } else {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s >", fprefix );
        }
    }
    if( active ) {
        wattroff( w, c_white );
    }
    if( !filter_edit && !pane.filter.empty() ) {
        mvwprintz( w, point( 6 + utf8_width( fprefix ), getmaxy( w ) - 1 ), c_white,
                   pane.filter );
        mvwprintz( w, point( getmaxx( w ) - utf8_width( fsuffix ) - 2, getmaxy( w ) - 1 ), c_white, "%s",
                   fsuffix );
    }
    wrefresh( w );
}

// be explicit with the values
enum aim_entry {
    ENTRY_START     = 0,
    ENTRY_VEHICLE   = 1,
    ENTRY_MAP       = 2,
    ENTRY_RESET     = 3
};

bool advanced_inventory::move_all_items( bool nested_call )
{
    auto &spane = panes[src];
    auto &dpane = panes[dest];

    // AIM_ALL source area routine
    if( spane.get_area() == AIM_ALL ) {
        // move all to `AIM_WORN' doesn't make sense (see `MAX_WORN_PER_TYPE')
        if( dpane.get_area() == AIM_WORN ) {
            popup( _( "You look at the items, then your clothes, and scratch your head..." ) );
            return false;
        }
        // if the source pane (AIM_ALL) is empty, then show a message and leave
        if( !is_processing() && spane.items.empty() ) {
            popup( _( "There are no items to be moved!" ) );
            return false;
        }

        auto &sarea = squares[spane.get_area()];
        auto &darea = squares[dpane.get_area()];

        // Check first if the destination area still have enough room for moving all.
        if( !is_processing() && sarea.volume > darea.free_volume( dpane.in_vehicle() ) &&
            !query_yn( _( "There isn't enough room, do you really want to move all?" ) ) ) {
            return false;
        }

        // make sure that there are items to be moved
        bool done = false;
        // copy the current pane, to be restored after the move is queued
        auto shadow = panes[src];
        // here we recursively call this function with each area in order to
        // put all items in the proper destination area, with minimal fuss
        auto &loc = uistate.adv_inv_aim_all_location;
        // re-entry nonsense
        auto &entry = uistate.adv_inv_re_enter_move_all;
        // if we are just starting out, set entry to initial value
        switch( static_cast<aim_entry>( entry++ ) ) {
            case ENTRY_START:
                ++entry;
            /* fallthrough */
            case ENTRY_VEHICLE:
                if( squares[loc].can_store_in_vehicle() ) {
                    // either do the inverse of the pane (if it is the one we are transferring to),
                    // or just transfer the contents (if it is not the one we are transferring to)
                    spane.set_area( squares[loc], dpane.get_area() == loc ? !dpane.in_vehicle() : true );
                    // add items, calculate weights and volumes... the fun stuff
                    recalc_pane( src );
                    // then move the items to the destination area
                    move_all_items( true );
                }
                break;
            case ENTRY_MAP:
                spane.set_area( squares[loc++], false );
                recalc_pane( src );
                move_all_items( true );
                break;
            case ENTRY_RESET:
                if( loc > AIM_AROUND_END ) {
                    loc = AIM_AROUND_BEGIN;
                    entry = ENTRY_START;
                    done = true;
                } else {
                    entry = ENTRY_VEHICLE;
                }
                break;
            default:
                debugmsg( "Invalid `aim_entry' [%d] reached!", entry - 1 );
                entry = ENTRY_START;
                loc = AIM_AROUND_BEGIN;
                return false;
        }
        // restore the pane to its former glory
        panes[src] = shadow;
        // make it auto loop back, if not already doing so
        if( !done && !g->u.activity ) {
            do_return_entry();
        }
        return true;
    }

    // Check some preconditions to quickly leave the function.
    if( spane.items.empty() ) {
        return false;
    }
    bool restore_area = false;
    if( dpane.get_area() == AIM_ALL ) {
        auto loc = dpane.get_area();
        // ask where we want to store the item via the menu
        if( !query_destination( loc ) ) {
            return false;
        }
        restore_area = true;
    }
    if( !squares[dpane.get_area()].canputitems() ) {
        popup( _( "You can't put items there!" ) );
        return false;
    }
    auto &sarea = squares[spane.get_area()];
    auto &darea = squares[dpane.get_area()];

    // Make sure source and destination are different, otherwise items will disappear
    // Need to check actual position to account for dragged vehicles
    if( dpane.get_area() == AIM_DRAGGED && sarea.pos == darea.pos &&
        spane.in_vehicle() == dpane.in_vehicle() ) {
        return false;
    } else if( spane.get_area() == dpane.get_area() && spane.in_vehicle() == dpane.in_vehicle() ) {
        return false;
    }

    if( nested_call || !get_option<bool>( "CLOSE_ADV_INV" ) ) {
        // Why is this here? It's because the activity backlog can act
        // like a stack instead of a single deferred activity in order to
        // accomplish some UI shenanigans. The inventory menu activity is
        // added, then an activity to drop is pushed on the stack, then
        // the drop activity is repeatedly popped and pushed on the stack
        // until all its items are processed. When the drop activity runs out,
        // the inventory menu activity is there waiting and seamlessly returns
        // the player to the menu. If the activity is interrupted instead of
        // completing, both activities are canceled.
        // Thanks to kevingranade for the explanation.
        do_return_entry();
    }

    if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
        std::list<std::pair<int, int>> dropped;
        // keep a list of favorites separated, only drop non-fav first if they exist
        std::list<std::pair<int, int>> dropped_favorite;

        if( spane.get_area() == AIM_INVENTORY ) {
            for( size_t index = 0; index < g->u.inv.size(); ++index ) {
                const auto &stack = g->u.inv.const_stack( index );
                const auto &it = stack.front();

                if( !spane.is_filtered( it ) ) {
                    ( it.is_favorite ? dropped_favorite : dropped ).emplace_back( static_cast<int>( index ),
                            it.count_by_charges() ? static_cast<int>( it.charges ) : static_cast<int>( stack.size() ) );
                }
            }
        } else if( spane.get_area() == AIM_WORN ) {
            // do this in reverse, to account for vector item removal messing with future indices
            auto iter = g->u.worn.rbegin();
            for( size_t idx = 0; idx < g->u.worn.size(); ++idx, ++iter ) {
                const size_t index = g->u.worn.size() - idx - 1;
                const auto &it = *iter;

                if( !spane.is_filtered( it ) ) {
                    ( it.is_favorite ? dropped_favorite : dropped ).emplace_back( player::worn_position_to_index(
                                index ),
                            it.count() );
                }
            }
        }
        if( dropped.empty() ) {
            if( !query_yn( _( "Really drop all your favorite items?" ) ) ) {
                return false;
            }
            dropped = dropped_favorite;
        }

        g->u.drop( dropped, g->u.pos() + darea.off );
    } else {
        if( dpane.get_area() == AIM_INVENTORY || dpane.get_area() == AIM_WORN ) {
            g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
        } else { // Vehicle and map destinations are handled the same.

            // Check first if the destination area still have enough room for moving all.
            if( !is_processing() && sarea.volume > darea.free_volume( dpane.in_vehicle() ) &&
                !query_yn( _( "There isn't enough room, do you really want to move all?" ) ) ) {
                return false;
            }

            g->u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
            // store whether the destination is a vehicle
            g->u.activity.values.push_back( dpane.in_vehicle() );
            // Stash the destination
            g->u.activity.coords.push_back( darea.off );
        }

        item_stack::iterator stack_begin, stack_end;
        if( panes[src].in_vehicle() ) {
            vehicle_stack targets = sarea.veh->get_items( sarea.vstor );
            stack_begin = targets.begin();
            stack_end = targets.end();
        } else {
            map_stack targets = g->m.i_at( sarea.pos );
            stack_begin = targets.begin();
            stack_end = targets.end();
        }

        // If moving to inventory, worn, or vehicle, silently filter buckets
        // Moving them would cause tons of annoying prompts or spills
        bool filter_buckets = dpane.get_area() == AIM_INVENTORY ||
                              dpane.get_area() == AIM_WORN ||
                              dpane.in_vehicle();
        bool filtered_any_bucket = false;
        // Push item_locations and item counts for all items at placement
        for( item_stack::iterator it = stack_begin; it != stack_end; ++it ) {
            if( spane.is_filtered( *it ) ) {
                continue;
            }
            if( filter_buckets && it->is_bucket_nonempty() ) {
                filtered_any_bucket = true;
                continue;
            }
            if( spane.in_vehicle() ) {
                g->u.activity.targets.emplace_back( vehicle_cursor( *sarea.veh, sarea.vstor ), &*it );
            } else {
                g->u.activity.targets.emplace_back( map_cursor( sarea.pos ), &*it );
            }
            // quantity of 0 means move all
            g->u.activity.values.push_back( 0 );
        }

        if( filtered_any_bucket ) {
            add_msg( m_info, _( "Skipping filled buckets to avoid spilling their contents." ) );
        }
    }
    // if dest was AIM_ALL then we used query_destination and should undo that
    if( restore_area ) {
        dpane.restore_area();
    }
    return true;
}

bool advanced_inventory::show_sort_menu( advanced_inventory_pane &pane )
{
    uilist sm;
    sm.text = _( "Sort by... " );
    sm.addentry( SORTBY_NONE,     true, 'u', _( "Unsorted (recently added first)" ) );
    sm.addentry( SORTBY_NAME,     true, 'n', get_sortname( SORTBY_NAME ) );
    sm.addentry( SORTBY_WEIGHT,   true, 'w', get_sortname( SORTBY_WEIGHT ) );
    sm.addentry( SORTBY_VOLUME,   true, 'v', get_sortname( SORTBY_VOLUME ) );
    sm.addentry( SORTBY_CHARGES,  true, 'x', get_sortname( SORTBY_CHARGES ) );
    sm.addentry( SORTBY_CATEGORY, true, 'c', get_sortname( SORTBY_CATEGORY ) );
    sm.addentry( SORTBY_DAMAGE,   true, 'd', get_sortname( SORTBY_DAMAGE ) );
    sm.addentry( SORTBY_AMMO,     true, 'a', get_sortname( SORTBY_AMMO ) );
    sm.addentry( SORTBY_SPOILAGE,   true, 's', get_sortname( SORTBY_SPOILAGE ) );
    // Pre-select current sort.
    sm.selected = pane.sortby - SORTBY_NONE;
    // Calculate key and window variables, generate window,
    // and loop until we get a valid answer.
    sm.query();
    if( sm.ret < SORTBY_NONE ) {
        return false;
    }
    pane.sortby = static_cast<advanced_inv_sortby>( sm.ret );
    return true;
}

static tripoint aim_vector( aim_location id )
{
    switch( id ) {
        case AIM_SOUTHWEST:
            return tripoint_south_west;
        case AIM_SOUTH:
            return tripoint_south;
        case AIM_SOUTHEAST:
            return tripoint_south_east;
        case AIM_WEST:
            return tripoint_west;
        case AIM_EAST:
            return tripoint_east;
        case AIM_NORTHWEST:
            return tripoint_north_west;
        case AIM_NORTH:
            return tripoint_north;
        case AIM_NORTHEAST:
            return tripoint_north_east;
        default:
            return tripoint_zero;
    }
}

void advanced_inventory::display()
{
    init();

    g->u.inv.restack( g->u );

    input_context ctxt( "ADVANCED_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "TOGGLE_TAB" );
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
    ctxt.register_action( "MOVE_ALL_ITEMS" );
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

    exit = false;
    recalc = true;
    redraw = true;

    while( !exit ) {
        if( g->u.moves < 0 ) {
            do_return_entry();
            return;
        }
        dest = src == advanced_inventory::side::left ? advanced_inventory::side::right :
               advanced_inventory::side::left;

        redraw_pane( advanced_inventory::side::left );
        redraw_pane( advanced_inventory::side::right );

        if( redraw && !is_processing() ) {
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
        redraw = false;
        recalc = false;
        // source and destination pane
        advanced_inventory_pane &spane = panes[src];
        advanced_inventory_pane &dpane = panes[dest];
        // current item in source pane, might be null
        advanced_inv_listitem *sitem = spane.get_cur_item_ptr();
        aim_location changeSquare = NUM_AIM_LOCATIONS;

        const std::string action = is_processing() ? "MOVE_ALL_ITEMS" : ctxt.handle_input();
        if( action == "CATEGORY_SELECTION" ) {
            inCategoryMode = !inCategoryMode;
            spane.redraw = true; // We redraw to force the color change of the highlighted line and header text.
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( action == "ITEMS_DEFAULT" ) {
            for( side cside : {
                     left, right
                 } ) {
                auto &pane = panes[cside];
                aim_location location = static_cast<aim_location>( uistate.adv_inv_default_areas[cside] );
                if( pane.get_area() != location || location == AIM_ALL ) {
                    pane.recalc = true;
                }
                pane.set_area( squares[location] );
            }
            redraw = true;
        } else if( action == "SAVE_DEFAULT" ) {
            uistate.adv_inv_default_areas[left] = panes[left].get_area();
            uistate.adv_inv_default_areas[right] = panes[right].get_area();
            popup( _( "Default layout was saved." ) );
            redraw = true;
        } else if( get_square( action, changeSquare ) ) {
            if( panes[left].get_area() == changeSquare || panes[right].get_area() == changeSquare ) {
                if( squares[changeSquare].can_store_in_vehicle() && changeSquare != AIM_DRAGGED ) {
                    // only deal with spane, as you can't _directly_ change dpane
                    if( dpane.get_area() == changeSquare ) {
                        spane.set_area( squares[changeSquare], !dpane.in_vehicle() );
                        spane.recalc = true;
                    } else if( spane.get_area() == dpane.get_area() ) {
                        // swap the `in_vehicle` element of each pane if "one in, one out"
                        spane.set_area( squares[spane.get_area()], !spane.in_vehicle() );
                        dpane.set_area( squares[dpane.get_area()], !dpane.in_vehicle() );
                        recalc = true;
                    }
                } else {
                    swap_panes();
                }
                redraw = true;
                // we need to check the original area if we can place items in vehicle storage
            } else if( squares[changeSquare].canputitems( spane.get_cur_item_ptr() ) ) {
                bool in_vehicle_cargo = false;
                if( changeSquare == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( spane.get_cur_item_ptr() );
                } else if( spane.get_area() == AIM_CONTAINER ) {
                    squares[changeSquare].set_container( nullptr );
                    // auto select vehicle if items exist at said square, or both are empty
                } else if( squares[changeSquare].can_store_in_vehicle() && spane.get_area() != changeSquare ) {
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
                spane.set_area( squares[changeSquare], in_vehicle_cargo );
                spane.index = 0;
                spane.recalc = true;
                if( dpane.get_area() == AIM_ALL ) {
                    dpane.recalc = true;
                }
                redraw = true;
            } else {
                popup( _( "You can't put items there!" ) );
                redraw = true; // to clear the popup
            }
        } else if( action == "TOGGLE_FAVORITE" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            for( auto *item : sitem->items ) {
                item->set_favorite( !item->is_favorite );
            }
            recalc = true; // In case we've merged faved and unfaved items
            redraw = true;
        } else if( action == "MOVE_SINGLE_ITEM" ||
                   action == "MOVE_VARIABLE_ITEM" ||
                   action == "MOVE_ITEM_STACK" ) {
            if( sitem == nullptr || !sitem->is_item_entry() ) {
                continue;
            }
            aim_location destarea = dpane.get_area();
            aim_location srcarea = sitem->area;
            bool restore_area = destarea == AIM_ALL;
            if( !query_destination( destarea ) ) {
                continue;
            }
            // Not necessarily equivalent to spane.in_vehicle() if using AIM_ALL
            bool from_vehicle = sitem->from_vehicle;
            bool to_vehicle = dpane.in_vehicle();

            // AIM_ALL should disable same area check and handle it with proper filtering instead.
            // This is a workaround around the lack of vehicle location info in
            // either aim_location or advanced_inv_listitem.
            if( squares[srcarea].is_same( squares[destarea] ) &&
                spane.get_area() != AIM_ALL &&
                spane.in_vehicle() == dpane.in_vehicle() ) {
                popup( _( "Source area is the same as destination (%s)." ), squares[destarea].name );
                redraw = true; // popup has messed up the screen
                continue;
            }
            assert( !sitem->items.empty() );
            const bool by_charges = sitem->items.front()->count_by_charges();
            int amount_to_move = 0;
            if( !query_charges( destarea, *sitem, action, amount_to_move ) ) {
                continue;
            }
            // This makes sure that all item references in the advanced_inventory_pane::items vector
            // are recalculated, even when they might not have changed, but they could (e.g. items
            // taken from inventory, but unable to put into the cargo trunk go back into the inventory,
            // but are potentially at a different place).
            recalc = true;
            assert( amount_to_move > 0 );
            if( destarea == AIM_CONTAINER ) {
                if( !move_content( *sitem->items.front(),
                                   *squares[destarea].get_container( to_vehicle ) ) ) {
                    redraw = true;
                    continue;
                }
            } else if( srcarea == AIM_INVENTORY && destarea == AIM_WORN ) {

                // make sure advanced inventory is reopened after activity completion.
                do_return_entry();

                g->u.assign_activity( activity_id( "ACT_WEAR" ) );

                g->u.activity.targets.emplace_back( g->u, sitem->items.front() );
                g->u.activity.values.push_back( amount_to_move );

                // exit so that the activity can be carried out
                exit = true;

            } else if( srcarea == AIM_INVENTORY || srcarea == AIM_WORN ) {

                // make sure advanced inventory is reopened after activity completion.
                do_return_entry();

                // if worn, we need to fix with the worn index number (starts at -2, as -1 is weapon)
                int idx = srcarea == AIM_INVENTORY ? sitem->idx : player::worn_position_to_index( sitem->idx );

                if( srcarea == AIM_WORN && destarea == AIM_INVENTORY ) {
                    // this is ok because worn items are never stacked (can't move more than 1).
                    g->u.takeoff( idx );

                    // exit so that the action can be carried out
                    exit = true;
                } else {
                    // important if item is worn
                    if( g->u.can_unwield( g->u.i_at( idx ) ).success() ) {
                        g->u.assign_activity( activity_id( "ACT_DROP" ) );
                        g->u.activity.placement = squares[destarea].off;

                        // incase there is vehicle cargo space at dest but the player wants to drop to ground
                        if( !to_vehicle ) {
                            g->u.activity.str_values.push_back( "force_ground" );
                        }

                        g->u.activity.values.push_back( idx );
                        g->u.activity.values.push_back( amount_to_move );

                        // exit so that the activity can be carried out
                        exit = true;
                    }
                }
            } else { // from map/vehicle: start ACT_PICKUP or ACT_MOVE_ITEMS as necessary

                // Make sure advanced inventory is reopened after activity completion.
                do_return_entry();

                if( destarea == AIM_INVENTORY ) {
                    g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
                } else if( destarea == AIM_WORN ) {
                    g->u.assign_activity( activity_id( "ACT_WEAR" ) );
                } else { // Vehicle and map destinations are handled similarly.

                    g->u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
                    // store whether the destination is a vehicle
                    g->u.activity.values.push_back( to_vehicle );
                    // Stash the destination
                    g->u.activity.coords.push_back( squares[destarea].off );
                }

                if( by_charges ) {
                    if( from_vehicle ) {
                        g->u.activity.targets.emplace_back( vehicle_cursor( *squares[srcarea].veh, squares[srcarea].vstor ),
                                                            sitem->items.front() );
                    } else {
                        g->u.activity.targets.emplace_back( map_cursor( squares[srcarea].pos ), sitem->items.front() );
                    }
                    g->u.activity.values.push_back( amount_to_move );
                } else {
                    for( std::list<item *>::iterator it = sitem->items.begin(); amount_to_move > 0 &&
                         it != sitem->items.end(); ++it ) {
                        if( from_vehicle ) {
                            g->u.activity.targets.emplace_back( vehicle_cursor( *squares[srcarea].veh, squares[srcarea].vstor ),
                                                                *it );
                        } else {
                            g->u.activity.targets.emplace_back( map_cursor( squares[srcarea].pos ), *it );
                        }
                        g->u.activity.values.push_back( 0 );
                        --amount_to_move;
                    }
                }

                // exit so that the activity can be carried out
                exit = true;
            }

            // if dest was AIM_ALL then we used query_destination and should undo that
            if( restore_area ) {
                dpane.restore_area();
            }
        } else if( action == "MOVE_ALL_ITEMS" ) {
            exit = move_all_items();
            recalc = true;
        } else if( action == "SORT" ) {
            if( show_sort_menu( spane ) ) {
                recalc = true;
                uistate.adv_inv_sort[src] = spane.sortby;
            }
            redraw = true;
        } else if( action == "FILTER" ) {
            string_input_popup spopup;
            std::string filter = spane.filter;
            filter_edit = true;
            spopup.window( spane.window, 4, w_height - 1, w_width / 2 - 4 )
            .max_length( 256 )
            .text( filter );

            draw_item_filter_rules( dpane.window, 1, 11, item_filter_type::FILTER );

            ime_sentry sentry;

            do {
                mvwprintz( spane.window, point( 2, getmaxy( spane.window ) - 1 ), c_cyan, "< " );
                mvwprintz( spane.window, point( w_width / 2 - 4, getmaxy( spane.window ) - 1 ), c_cyan, " >" );
                std::string new_filter = spopup.query_string( false );
                if( spopup.context().get_raw_input().get_first_input() == KEY_ESCAPE ) {
                    // restore original filter
                    spane.set_filter( filter );
                } else {
                    spane.set_filter( new_filter );
                }
                redraw_pane( src );
            } while( spopup.context().get_raw_input().get_first_input() != '\n' &&
                     spopup.context().get_raw_input().get_first_input() != KEY_ESCAPE );
            filter_edit = false;
            spane.redraw = true;
            dpane.redraw = true;
        } else if( action == "RESET_FILTER" ) {
            spane.set_filter( "" );
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
            const int info_startx = colstart + ( src == advanced_inventory::side::left ? info_width : 0 );
            if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
                int idx = spane.get_area() == AIM_INVENTORY ? sitem->idx :
                          player::worn_position_to_index( sitem->idx );
                // Setup a "return to AIM" activity. If examining the item creates a new activity
                // (e.g. reading, reloading, activating), the new activity will be put on top of
                // "return to AIM". Once the new activity is finished, "return to AIM" comes back
                // (automatically, see player activity handling) and it re-opens the AIM.
                // If examining the item did not create a new activity, we have to remove
                // "return to AIM".
                do_return_entry();
                assert( g->u.has_activity( activity_id( "ACT_ADV_INVENTORY" ) ) );
                ret = g->inventory_item_menu( idx, info_startx, info_width,
                                              src == advanced_inventory::side::left ? game::LEFT_OF_INFO : game::RIGHT_OF_INFO );
                if( !g->u.has_activity( activity_id( "ACT_ADV_INVENTORY" ) ) ) {
                    exit = true;
                } else {
                    g->u.cancel_activity();
                }
                // Might have changed a stack (activated an item, repaired an item, etc.)
                if( spane.get_area() == AIM_INVENTORY ) {
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
                spane.scroll_by( +1 );
            } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
                spane.scroll_by( -1 );
            }
            redraw = true; // item info window overwrote the other pane and the header
        } else if( action == "QUIT" ) {
            exit = true;
        } else if( action == "PAGE_DOWN" ) {
            spane.scroll_by( +itemsPerPage );
        } else if( action == "PAGE_UP" ) {
            spane.scroll_by( -itemsPerPage );
        } else if( action == "DOWN" ) {
            if( inCategoryMode ) {
                spane.scroll_category( +1 );
            } else {
                spane.scroll_by( +1 );
            }
        } else if( action == "UP" ) {
            if( inCategoryMode ) {
                spane.scroll_category( -1 );
            } else {
                spane.scroll_by( -1 );
            }
        } else if( action == "LEFT" ) {
            src = left;
            redraw = true;
        } else if( action == "RIGHT" ) {
            src = right;
            redraw = true;
        } else if( action == "TOGGLE_TAB" ) {
            src = dest;
            redraw = true;
        } else if( action == "TOGGLE_VEH" ) {
            if( squares[spane.get_area()].can_store_in_vehicle() ) {
                // swap the panes if going vehicle will show the same tile
                if( spane.get_area() == dpane.get_area() && spane.in_vehicle() != dpane.in_vehicle() ) {
                    swap_panes();
                    // disallow for dragged vehicles
                } else if( spane.get_area() != AIM_DRAGGED ) {
                    // Toggle between vehicle and ground
                    spane.set_area( squares[spane.get_area()], !spane.in_vehicle() );
                    spane.index = 0;
                    spane.recalc = true;
                    if( dpane.get_area() == AIM_ALL ) {
                        dpane.recalc = true;
                    }
                    // make sure to update the minimap as well!
                    redraw = true;
                }
            } else {
                popup( _( "No vehicle there!" ) );
                redraw = true;
            }
        }
    }
}

void advanced_inventory_pane::skip_category_headers( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( static_cast<size_t>( index ) < items.size() ); // valid index is required
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    assert( !items.empty() ); // index would not be valid, and this would be an endless loop
    while( !items[index].is_item_entry() ) {
        mod_index( offset );
    }
}

void advanced_inventory_pane::mod_index( int offset )
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

void advanced_inventory_pane::scroll_by( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    if( items.empty() ) {
        return;
    }
    mod_index( offset );
    skip_category_headers( offset > 0 ? +1 : -1 );
    redraw = true;
}

void advanced_inventory_pane::scroll_category( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    if( items.empty() ) {
        return;
    }
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
    // Make sure we land on an item entry.
    skip_category_headers( offset > 0 ? +1 : -1 );
    redraw = true;
}

advanced_inv_listitem *advanced_inventory_pane::get_cur_item_ptr()
{
    if( static_cast<size_t>( index ) >= items.size() ) {
        return nullptr;
    }
    return &items[index];
}

void advanced_inventory_pane::set_filter( const std::string &new_filter )
{
    if( filter == new_filter ) {
        return;
    }
    filter = new_filter;
    filtercache.clear();
    recalc = true;
}

class query_destination_callback : public uilist_callback
{
    private:
        advanced_inventory &_adv_inv;
        void draw_squares( const uilist *menu ); // Render a fancy ASCII grid at the left of the menu.
    public:
        query_destination_callback( advanced_inventory &adv_inv ) : _adv_inv( adv_inv ) {}
        void select( int /*entnum*/, uilist *menu ) override {
            draw_squares( menu );
        }
};

void query_destination_callback::draw_squares( const uilist *menu )
{
    assert( menu->entries.size() >= 9 );
    int ofs = -25 - 4;
    int sel = advanced_inventory::screen_relative_location(
                  static_cast <aim_location>( menu->selected + 1 ) );
    for( int i = 1; i < 10; i++ ) {
        aim_location loc = advanced_inventory::screen_relative_location( static_cast <aim_location>( i ) );
        char key = advanced_inventory::get_location_key( loc );
        advanced_inv_area &square = _adv_inv.get_one_square( loc );
        bool in_vehicle = square.can_store_in_vehicle();
        const char *bracket = in_vehicle ? "<>" : "[]";
        // always show storage option for vehicle storage, if applicable
        bool canputitems = menu->entries[i - 1].enabled && square.canputitems();
        nc_color bcolor = canputitems ? sel == loc ? h_white : c_light_gray : c_dark_gray;
        nc_color kcolor = canputitems ? sel == loc ? h_white : c_light_gray : c_dark_gray;
        const int x = square.hscreen.x + ofs;
        const int y = square.hscreen.y + 5;
        mvwprintz( menu->window, point( x, y ), bcolor, "%c", bracket[0] );
        wprintz( menu->window, kcolor, "%c", key );
        wprintz( menu->window, bcolor, "%c", bracket[1] );
    }
}

bool advanced_inventory::query_destination( aim_location &def )
{
    if( def != AIM_ALL ) {
        if( squares[def].canputitems() ) {
            return true;
        }
        popup( _( "You can't put items there!" ) );
        redraw = true; // the popup has messed the screen up.
        return false;
    }

    uilist menu;
    menu.text = _( "Select destination" );
    menu.pad_left = 9; /* free space for the squares */
    query_destination_callback cb( *this );
    menu.callback = &cb;

    {
        std::vector <aim_location> ordered_locs;
        static_assert( AIM_NORTHEAST - AIM_SOUTHWEST == 8,
                       "Expected 9 contiguous directions in the aim_location enum" );
        for( int i = AIM_SOUTHWEST; i <= AIM_NORTHEAST; i++ ) {
            ordered_locs.push_back( screen_relative_location( static_cast <aim_location>( i ) ) );
        }
        for( auto &ordered_loc : ordered_locs ) {
            auto &s = squares[ordered_loc];
            const int size = s.get_item_count();
            std::string prefix = string_format( "%2d/%d", size, MAX_ITEM_IN_SQUARE );
            if( size >= MAX_ITEM_IN_SQUARE ) {
                prefix += _( " (FULL)" );
            }
            menu.addentry( ordered_loc,
                           s.canputitems() && s.id != panes[src].get_area(),
                           get_location_key( ordered_loc ),
                           prefix + " " + s.name + " " + ( s.veh != nullptr ? s.veh->name : "" ) );
        }
    }
    // Selected keyed to uilist.entries, which starts at 0.
    menu.selected = uistate.adv_inv_last_popup_dest - AIM_SOUTHWEST;
    menu.show(); // generate and show window.
    while( menu.ret == UILIST_WAIT_INPUT ) {
        menu.query( false ); // query, but don't loop
    }
    redraw = true; // the menu has messed the screen up.
    if( menu.ret >= AIM_SOUTHWEST && menu.ret <= AIM_NORTHEAST ) {
        assert( squares[menu.ret].canputitems() );
        def = static_cast<aim_location>( menu.ret );
        // we have to set the destination pane so that move actions will target it
        // we can use restore_area later to undo this
        panes[dest].set_area( squares[def], true );
        uistate.adv_inv_last_popup_dest = menu.ret;
        return true;
    }
    return false;
}

bool advanced_inventory::move_content( item &src_container, item &dest_container )
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

units::volume advanced_inv_area::free_volume( bool in_vehicle ) const
{
    assert( id != AIM_ALL ); // should be a specific location instead
    if( id == AIM_INVENTORY || id == AIM_WORN ) {
        return g->u.volume_capacity() - g->u.volume_carried();
    }
    return in_vehicle ? veh->free_volume( vstor ) : g->m.free_volume( pos );
}

bool advanced_inventory::query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                                        const std::string &action, int &amount )
{
    assert( destarea != AIM_ALL ); // should be a specific location instead
    assert( !sitem.items.empty() ); // valid item is obviously required
    const item &it = *sitem.items.front();
    advanced_inv_area &p = squares[destarea];
    const bool by_charges = it.count_by_charges();
    const units::volume free_volume = p.free_volume( panes[dest].in_vehicle() );
    // default to move all, unless if being equipped
    const int input_amount = by_charges ? it.charges : action == "MOVE_SINGLE_ITEM" ? 1 : sitem.stacks;
    assert( input_amount > 0 ); // there has to be something to begin with
    amount = input_amount;

    // Includes moving from/to inventory and around on the map.
    if( it.made_of_from_type( LIQUID ) && !it.is_frozen_liquid() ) {
        popup( _( "You can't pick up a liquid." ) );
        redraw = true;
        return false;
    }

    // Check volume, this should work the same for inventory, map and vehicles, but not for worn
    const int room_for = it.charges_per_volume( free_volume );
    if( amount > room_for && squares[destarea].id != AIM_WORN ) {
        if( room_for <= 0 ) {
            popup( _( "Destination area is full.  Remove some items first." ) );
            redraw = true;
            return false;
        }
        amount = std::min( room_for, amount );
    }
    // Map and vehicles have a maximal item count, check that. Inventory does not have this.
    if( destarea != AIM_INVENTORY &&
        destarea != AIM_WORN &&
        destarea != AIM_CONTAINER ) {
        const int cntmax = p.max_size - p.get_item_count();
        // For items counted by charges, adding it adds 0 items if something there stacks with it.
        const bool adds0 = by_charges && std::any_of( panes[dest].items.begin(), panes[dest].items.end(),
        [&it]( const advanced_inv_listitem & li ) {
            return li.is_item_entry() && li.items.front()->stacks_with( it );
        } );
        if( cntmax <= 0 && !adds0 ) {
            popup( _( "Destination area has too many items.  Remove some first." ) );
            redraw = true;
            return false;
        }
        // Items by charge count as a single item, regardless of the charges. As long as the
        // destination can hold another item, one can move all charges.
        if( !by_charges ) {
            amount = std::min( cntmax, amount );
        }
    }
    // Inventory has a weight capacity, map and vehicle don't have that
    if( destarea == AIM_INVENTORY  || destarea == AIM_WORN ) {
        const units::mass unitweight = it.weight() / ( by_charges ? it.charges : 1 );
        const units::mass max_weight = g->u.has_trait( trait_id( "DEBUG_STORAGE" ) ) ?
                                       units::mass_max : g->u.weight_capacity() * 4 - g->u.weight_carried();
        if( unitweight > 0_gram && unitweight * amount > max_weight ) {
            const int weightmax = max_weight / unitweight;
            if( weightmax <= 0 ) {
                popup( _( "This is too heavy!" ) );
                redraw = true;
                return false;
            }
            amount = std::min( weightmax, amount );
        }
    }
    // handle how many of armor type we can equip (max of 2 per type)
    if( destarea == AIM_WORN ) {
        const auto &id = sitem.items.front()->typeId();
        // how many slots are available for the item?
        const int slots_available = MAX_WORN_PER_TYPE - g->u.amount_worn( id );
        // base the amount to equip on amount of slots available
        amount = std::min( slots_available, input_amount );
    }
    // Now we have the final amount. Query if requested or limited room left.
    if( action == "MOVE_VARIABLE_ITEM" || amount < input_amount ) {
        const int count = by_charges ? it.charges : sitem.stacks;
        const char *msg = nullptr;
        std::string popupmsg;
        if( amount >= input_amount ) {
            msg = _( "How many do you want to move? [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, count );
        } else {
            msg = _( "Destination can only hold %d! Move how many? [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, amount, count );
        }
        // At this point amount contains the maximal amount that the destination can hold.
        const int possible_max = std::min( input_amount, amount );
        if( amount <= 0 ) {
            popup( _( "The destination is already full!" ) );
        } else {
            amount = string_input_popup()
                     .title( popupmsg )
                     .width( 20 )
                     .only_digits( true )
                     .query_int();
        }
        if( amount <= 0 ) {
            redraw = true;
            return false;
        }
        if( amount > possible_max ) {
            amount = possible_max;
        }
    }
    return true;
}

bool advanced_inv_area::is_same( const advanced_inv_area &other ) const
{
    // All locations (sans the below) are compared by the coordinates,
    // e.g. dragged vehicle (to the south) and AIM_SOUTH are the same.
    if( id != AIM_INVENTORY && other.id != AIM_INVENTORY &&
        id != AIM_WORN      && other.id != AIM_WORN      &&
        id != AIM_CONTAINER && other.id != AIM_CONTAINER ) {
        //     have a vehicle?...     ...do the cargo index and pos match?...    ...at least pos?
        return veh == other.veh ? pos == other.pos && vstor == other.vstor : pos == other.pos;
    }
    //      ...is the id?
    return id == other.id;
}

bool advanced_inv_area::canputitems( const advanced_inv_listitem *advitem )
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

item *advanced_inv_area::get_container( bool in_vehicle )
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

void advanced_inv_area::set_container( const advanced_inv_listitem *advitem )
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

bool advanced_inv_area::is_container_valid( const item *it ) const
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

void advanced_inv_area::set_container_position()
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

void advanced_inv()
{
    advanced_inventory advinv;
    advinv.display();
}

void advanced_inventory::refresh_minimap()
{
    // don't update ui if processing demands
    if( is_processing() ) {
        return;
    }
    // redraw border around minimap
    draw_border( mm_border );
    // minor addition to border for AIM_ALL, sorta hacky
    if( panes[src].get_area() == AIM_ALL || panes[dest].get_area() == AIM_ALL ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( mm_border, point( 1, 0 ), c_light_gray, utf8_truncate( _( "All" ), minimap_width ) );
    }
    // refresh border, then minimap
    wrefresh( mm_border );
    wrefresh( minimap );
}

void advanced_inventory::draw_minimap()
{
    // if player is in one of the below, invert the player cell
    static const std::array<aim_location, 3> player_locations = {
        {AIM_CENTER, AIM_INVENTORY, AIM_WORN}
    };
    static const std::array<side, NUM_PANES> sides = {{left, right}};
    // get the center of the window
    tripoint pc = {getmaxx( minimap ) / 2, getmaxy( minimap ) / 2, 0};
    // draw the 3x3 tiles centered around player
    g->m.draw( minimap, g->u.pos() );
    for( auto s : sides ) {
        char sym = get_minimap_sym( s );
        if( sym == '\0' ) {
            continue;
        }
        auto sq = squares[panes[s].get_area()];
        auto pt = pc + sq.off;
        // invert the color if pointing to the player's position
        auto cl = sq.id == AIM_INVENTORY || sq.id == AIM_WORN ?
                  invert_color( c_light_cyan ) : c_light_cyan.blink();
        mvwputch( minimap, pt.xy(), cl, sym );
    }

    // Invert player's tile color if exactly one pane points to player's tile
    bool invert_left = false;
    bool invert_right = false;
    const auto is_selected = [ this ]( const aim_location & where, size_t side ) {
        return where == this->panes[ side ].get_area();
    };
    for( auto &loc : player_locations ) {
        invert_left |= is_selected( loc, 0 );
        invert_right |= is_selected( loc, 1 );
    }

    if( !invert_left || !invert_right ) {
        g->u.draw( minimap, g->u.pos(), invert_left || invert_right );
    }
}

char advanced_inventory::get_minimap_sym( side p ) const
{
    static const std::array<char, NUM_PANES> c_side = {{'L', 'R'}};
    static const std::array<char, NUM_PANES> d_side = {{'^', 'v'}};
    static const std::array<char, NUM_AIM_LOCATIONS> g_nome = {{
            '@', '#', '#', '#', '#', '@', '#',
            '#', '#', '#', 'D', '^', 'C', '@'
        }
    };
    char ch = g_nome[panes[p].get_area()];
    switch( ch ) {
        case '@': // '^' or 'v'
            ch = d_side[panes[-p + 1].get_area() == AIM_CENTER];
            break;
        case '#': // 'L' or 'R'
            ch = panes[p].in_vehicle() ? 'V' : c_side[p];
            break;
        case '^': // do not show anything
            ch ^= ch;
            break;
    }
    return ch;
}

aim_location advanced_inv_area::offset_to_location() const
{
    static aim_location loc_array[3][3] = {
        {AIM_NORTHWEST,     AIM_NORTH,      AIM_NORTHEAST},
        {AIM_WEST,          AIM_CENTER,     AIM_EAST},
        {AIM_SOUTHWEST,     AIM_SOUTH,      AIM_SOUTHEAST}
    };
    return loc_array[off.y + 1][off.x + 1];
}

void advanced_inventory::swap_panes()
{
    // Switch left and right pane.
    std::swap( panes[left], panes[right] );
    // Window pointer must be unchanged!
    std::swap( panes[left].window, panes[right].window );
    // No recalculation needed, data has not changed
    redraw = true;
}

void advanced_inventory::do_return_entry()
{
    // only save pane settings
    save_settings( true );
    g->u.assign_activity( activity_id( "ACT_ADV_INVENTORY" ) );
    g->u.activity.auto_resume = true;
    uistate.adv_inv_exit_code = exit_re_entry;
}

bool advanced_inventory::is_processing() const
{
    return uistate.adv_inv_re_enter_move_all != ENTRY_START;
}

aim_location advanced_inventory::screen_relative_location( aim_location area )
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

        default :
            return area;
    }
}

void cancel_aim_processing()
{
    uistate.adv_inv_re_enter_move_all = ENTRY_START;
}
