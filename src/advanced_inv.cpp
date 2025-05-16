#include "advanced_inv.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "advanced_inv_area.h"
#include "advanced_inv_listitem.h"
#include "advanced_inv_pagination.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_imgui.h"
#include "cata_scope_helpers.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "imgui/imgui.h"
#include "input.h"
#include "input_context.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "localized_comparator.h"
#include "map.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "panels.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_iteminfo.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"

#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif

static const flag_id json_flag_NO_RELOAD( "NO_RELOAD" );
static const flag_id json_flag_NO_UNLOAD( "NO_UNLOAD" );

using move_all_entry = std::pair<std::pair<int, int>, drop_or_stash_item_info>;

namespace io
{

template<>
std::string enum_to_string<aim_exit>( const aim_exit v )
{
    switch( v ) {
        // *INDENT-OFF*
        case aim_exit::none: return "none";
        case aim_exit::okay: return "okay";
        case aim_exit::re_entry: return "re_entry";
        // *INDENT-ON*
        case aim_exit::last:
            break;
    }
    cata_fatal( "Invalid aim_exit" );
}

template<>
std::string enum_to_string<aim_entry>( const aim_entry v )
{
    switch( v ) {
        // *INDENT-OFF*
        case aim_entry::START: return "START";
        case aim_entry::VEHICLE: return "VEHICLE";
        case aim_entry::MAP: return "MAP";
        case aim_entry::RESET: return "RESET";
        // *INDENT-ON*
        case aim_entry::last:
            break;
    }
    cata_fatal( "Invalid aim_entry" );
}

} // namespace io

namespace
{
std::unique_ptr<advanced_inventory> advinv;
} // namespace

void create_advanced_inv()
{
    if( !advinv ) {
        advinv = std::make_unique<advanced_inventory>();
    }
    advinv->display();
    // keep the UI and its ui_adaptor running if we're returning
    if( uistate.transfer_save.exit_code != aim_exit::re_entry || get_avatar().activity.is_null() ) {
        advinv.reset();
        cancel_aim_processing();
    }
}

void temp_hide_advanced_inv()
{
    if( advinv ) {
        advinv->temp_hide();
    }
}

// *INDENT-OFF*
advanced_inventory::advanced_inventory()
    : recalc( true )
    , src( left )
    , dest( right )
      // panes don't need initialization, they are recalculated immediately
    , squares( {
    {
        //               pos in window
        { AIM_INVENTORY, point( 25, 2 ), tripoint::zero,       _( "Inventory" ),          _( "IN" ),  "I", "ITEMS_INVENTORY", AIM_INVENTORY},
        { AIM_SOUTHWEST, point( 30, 3 ), tripoint::south_west, _( "South West" ),         _( "SW" ),  "1", "ITEMS_SW",        AIM_WEST},
        { AIM_SOUTH,     point( 33, 3 ), tripoint::south,      _( "South" ),              _( "S" ),   "2", "ITEMS_S",         AIM_SOUTHWEST},
        { AIM_SOUTHEAST, point( 36, 3 ), tripoint::south_east, _( "South East" ),         _( "SE" ),  "3", "ITEMS_SE",        AIM_SOUTH},
        { AIM_WEST,      point( 30, 2 ), tripoint::west,       _( "West" ),               _( "W" ),   "4", "ITEMS_W",         AIM_NORTHWEST},
        { AIM_CENTER,    point( 33, 2 ), tripoint::zero,       _( "Directly below you" ), _( "DN" ),  "5", "ITEMS_CE",        AIM_CENTER},
        { AIM_EAST,      point( 36, 2 ), tripoint::east,       _( "East" ),               _( "E" ),   "6", "ITEMS_E",         AIM_SOUTHEAST},
        { AIM_NORTHWEST, point( 30, 1 ), tripoint::north_west, _( "North West" ),         _( "NW" ),  "7", "ITEMS_NW",        AIM_NORTH},
        { AIM_NORTH,     point( 33, 1 ), tripoint::north,      _( "North" ),              _( "N" ),   "8", "ITEMS_N",         AIM_NORTHEAST},
        { AIM_NORTHEAST, point( 36, 1 ), tripoint::north_east, _( "North East" ),         _( "NE" ),  "9", "ITEMS_NE",        AIM_EAST},
        { AIM_DRAGGED,   point( 22, 3 ), tripoint::zero,       _( "Grabbed Vehicle" ),    _( "GR" ),  "D", "ITEMS_DRAGGED_CONTAINER", AIM_DRAGGED},
        { AIM_ALL,       point( 25, 3 ), tripoint::zero,       _( "Surrounding area" ),   _( "AL" ),  "A", "ITEMS_AROUND",    AIM_ALL},
        { AIM_CONTAINER, point( 25, 1 ), tripoint::zero,       _( "Container" ),          _( "CN" ),  "C", "ITEMS_CONTAINER", AIM_CONTAINER},
        { AIM_PARENT,    point( 22, 1 ), tripoint::zero,       "",                        "",         "X", "ITEMS_PARENT",    AIM_PARENT},
        { AIM_WORN,      point( 22, 2 ), tripoint::zero,       _( "Worn Items" ),         _( "WR" ),  "W", "ITEMS_WORN",      AIM_WORN}
    }
} )
{
    save_state = &uistate.transfer_save;
}
// *INDENT-ON*

advanced_inventory::~advanced_inventory()
{
    save_settings( false );
    if( save_state->exit_code != aim_exit::re_entry ) {
        save_state->exit_code = aim_exit::okay;
    }
    // Only refresh if we exited manually, otherwise we're going to be right back
    if( exit ) {
        get_player_character().check_item_encumbrance_flag();
    }
}

void advanced_inventory::save_settings( bool only_panes )
{
    if( !only_panes ) {
        save_state->active_left = ( src == left );
    }
    for( int i = 0; i < NUM_PANES; ++i ) {
        panes[i].save_settings();
    }
}

void advanced_inventory::load_settings()
{
    aim_exit aim_code = static_cast<aim_exit>( save_state->exit_code );
    panes[left].load_settings( save_state->saved_area, squares, aim_code == aim_exit::re_entry );
    panes[right].load_settings( save_state->saved_area_right, squares, aim_code == aim_exit::re_entry );
    // In-vehicle flags are set dynamically inside advanced_inventory_pane::load_settings,
    // which means the flags may end up the same even if the areas are also the same. To
    // avoid this, we use the saved in-vehicle flags instead.
    if( panes[left].get_area() == panes[right].get_area() ) {
        panes[left].set_area( squares[panes[left].get_area()], save_state->pane.in_vehicle );
        // Use the negated in-vehicle flag of the left pane to ensure different
        // in-vehicle flags.
        panes[right].set_area( squares[panes[right].get_area()], !save_state->pane.in_vehicle );
    }
    save_state->exit_code = aim_exit::none;
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
        case SORTBY_DENSITY:
            return _( "density" );
        case SORTBY_CHARGES:
            return _( "charges" );
        case SORTBY_CATEGORY:
            return _( "category" );
        case SORTBY_DAMAGE:
            return _( "offensive power" );
        case SORTBY_AMMO:
            return _( "ammo/charge type" );
        case SORTBY_SPOILAGE:
            return _( "spoilage" );
        case SORTBY_PRICE:
            return _( "barter value" );
        case SORTBY_PRICEPERVOLUME:
            return _( "barter value / volume" );
        case SORTBY_PRICEPERWEIGHT:
            return _( "barter value / weight" );
        case SORTBY_STACKS:
            return _( "amount" );
    }
    return "!BUG!";
}

bool advanced_inventory::get_square( const std::string &action, aim_location &ret )
{
    for( advanced_inv_area &s : squares ) {
        if( s.actionname == action ) {
            ret = screen_relative_location( s.id );
            return true;
        }
    }
    return false;
}

aim_location advanced_inventory::screen_relative_location( aim_location area )
{
    if( g->is_tileset_isometric() ) {
        return squares[area].relative_location;
    } else {
        return area;
    }
}

inline std::string advanced_inventory::get_location_key( aim_location area )
{
    return squares[area].minimapname;
}

void advanced_inventory::init()
{
    for( advanced_inv_area &square : squares ) {
        square.init();
    }

    panes[left].save_state = &save_state->pane;
    panes[right].save_state = &save_state->pane_right;

    load_settings();

    src = ( save_state->active_left ) ? left : right;
    dest = ( save_state->active_left ) ? right : left;

    //sanity check, badly initialized values may cause problem in move_all_items( see cata_assert() )
    if( panes[src].get_area() == AIM_ALL && panes[dest].get_area() == AIM_ALL ) {
        panes[dest].set_area( AIM_INVENTORY );
    }
}

void advanced_inventory::print_items( side p, bool active )
{
    map &here = get_map();
    advanced_inventory_pane &pane = panes[p];
    const auto &items = pane.items;
    const catacurses::window &window = pane.window;
    const int index = pane.index;
    bool compact = TERMX <= 100;
    pane.other_cont = -1;
    std::unordered_set<const item *> other_pane_conts;
    if( panes[-p + 1].container ) {
        item_location parent_recursive = panes[-p + 1].container;
        other_pane_conts.insert( parent_recursive.get_item() );
        while( parent_recursive.has_parent() ) {
            parent_recursive = parent_recursive.parent_item();
            other_pane_conts.insert( parent_recursive.get_item() );
        }
    }

    int columns = getmaxx( window );
    std::string spaces( columns - 4, ' ' );

    nc_color norm = active ? c_white : c_dark_gray;

    Character &player_character = get_player_character();
    //print inventory's current and total weight + volume
    if( pane.get_area() == AIM_INVENTORY || pane.get_area() == AIM_WORN ||
        ( pane.get_area() == AIM_CONTAINER && pane.container ) ) {

        double weight_carried;
        double weight_capacity;
        units::volume volume_carried;
        units::volume volume_capacity;
        if( pane.get_area() == AIM_CONTAINER ) {
            weight_carried = convert_weight( pane.container->get_total_contained_weight() );
            weight_capacity = convert_weight( pane.container->get_total_weight_capacity() );
            volume_carried = pane.container->get_total_contained_volume();
            volume_capacity = pane.container->get_total_capacity();
        } else {
            weight_carried = convert_weight( player_character.weight_carried() );
            weight_capacity = convert_weight( player_character.weight_capacity() );
            volume_carried = player_character.volume_carried();
            volume_capacity = player_character.volume_capacity();
        }
        // align right, so calculate formatted head length
        const std::string formatted_head = string_format( "%.1f/%.1f %s  %s/%s %s",
                                           weight_carried, weight_capacity, weight_units(),
                                           format_volume( volume_carried ),
                                           format_volume( volume_capacity ),
                                           volume_units_abbr() );
        const int hrightcol = columns - 1 - utf8_width( formatted_head );
        nc_color color = weight_carried > weight_capacity ? c_red : c_light_green;
        mvwprintz( window, point( hrightcol, 4 ), color, "%.1f", weight_carried );
        wprintz( window, c_light_gray, "/%.1f %s  ", weight_capacity, weight_units() );
        color = volume_carried.value() > volume_capacity.value() ?
                c_red : c_light_green;
        wprintz( window, color, format_volume( volume_carried ) );
        wprintz( window, c_light_gray, "/%s %s", format_volume( volume_capacity ), volume_units_abbr() );
    } else {
        //print square's current and total weight + volume
        std::string formatted_head;
        if( pane.get_area() == AIM_ALL ) {
            formatted_head = string_format( "%3.1f %s  %s %s",
                                            convert_weight( squares[pane.get_area()].weight ),
                                            weight_units(),
                                            format_volume( squares[pane.get_area()].volume ),
                                            volume_units_abbr() );
        } else {
            units::volume maxvolume = 0_ml;
            advanced_inv_area &s = squares[pane.get_area()];
            if( pane.get_area() == AIM_CONTAINER && pane.container ) {
                maxvolume = pane.container->get_total_capacity();
            } else if( pane.in_vehicle() ) {
                maxvolume = s.get_vehicle_stack().max_volume();
            } else {
                maxvolume = here.max_volume( s.pos );
            }
            formatted_head = string_format( "%3.1f %s  %s/%s %s",
                                            convert_weight( pane.in_vehicle() ? s.weight_veh : s.weight ),
                                            weight_units(),
                                            format_volume( pane.in_vehicle() ? s.volume_veh : s.volume ),
                                            format_volume( maxvolume ),
                                            volume_units_abbr() );
        }
        mvwprintz( window, point( columns - 1 - utf8_width( formatted_head ), 4 ), norm, formatted_head );
    }

    //print header row and determine max item name length
    // Last printable column
    const int lastcol = columns - 2;
    const size_t name_startpos = compact ? 1 : 4;
    const size_t src_startpos = lastcol - 18;
    const size_t amt_startpos = lastcol - 15;
    const size_t weight_startpos = lastcol - 10;
    const size_t vol_startpos = lastcol - 4;
    // Default name length
    int max_name_length = amt_startpos - name_startpos - 1;

    //~ Items list header (length type 1). Table fields length without spaces: amt - 4, weight - 5, vol - 4.
    const int table_hdr_len1 = utf8_width( _( "amt weight vol" ) );
    //~ Items list header (length type 2). Table fields length without spaces: src - 2, amt - 4, weight - 5, vol - 4.
    const int table_hdr_len2 = utf8_width( _( "src amt weight vol" ) );

    mvwprintz( window, point( compact ? 1 : 4, 5 ), c_light_gray, _( "Name (charges)" ) );
    if( pane.get_area() == AIM_ALL && !compact ) {
        mvwprintz( window, point( lastcol - table_hdr_len2 + 1, 5 ), c_light_gray,
                   _( "src amt weight vol" ) );
        // 1 for space
        max_name_length = src_startpos - name_startpos - 1;
    } else {
        mvwprintz( window, point( lastcol - table_hdr_len1 + 1, 5 ), c_light_gray, _( "amt weight vol" ) );
    }

    int pageStart = 0; // index of first item on current page

    advanced_inventory_pagination pagination( linesPerPage, pane );
    if( !items.empty() ) {
        // paginate up to the current item (to count pages)
        for( int i = 0; i <= index; i++ ) {
            const bool pagebreak = pagination.step( i );
            if( pagebreak ) {
                pageStart = i;
            }
        }
    }

    pagination.reset_page();
    for( size_t i = pageStart; i < items.size(); i++ ) {
        const advanced_inv_listitem &sitem = items[i];
        const int line = pagination.line;
        int item_line = line;
        if( pane.sortby == SORTBY_CATEGORY && pagination.new_category( sitem.cat ) ) {
            // don't put category header at bottom of page
            if( line == linesPerPage - 1 ) {
                break;
            }
            // insert category header
            mvwprintz( window, point( ( columns - utf8_width( sitem.cat->name_header() ) - 6 ) / 2, 6 + line ),
                       c_cyan, "[%s]", sitem.cat->name_header() );
            item_line = line + 1;
        }

        const item &it = *sitem.items.front();
        const bool selected = active && index == static_cast<int>( i );

        nc_color thiscolor;
        if( !active ) {
            thiscolor = norm;
        } else if( it.is_food_container() && !it.is_craft() && it.num_item_stacks() == 1 ) {
            thiscolor = it.all_items_top().front()->color_in_inventory();
        } else {
            thiscolor = it.color_in_inventory();
        }
        nc_color thiscolordark = c_dark_gray;
        nc_color print_color;

        if( selected ) {
            if( !other_pane_conts.empty() && other_pane_conts.count( &it ) == 1 ) {
                pane.other_cont = item_line;
                thiscolor = c_white_yellow;
            } else {
                thiscolor = inCategoryMode && pane.sortby == SORTBY_CATEGORY ? c_white_red : hilite( c_white );
            }
            thiscolordark = hilite( thiscolordark );
            if( compact ) {
                mvwprintz( window, point( 1, 6 + item_line ), thiscolor, "  %s", spaces );
            } else {
                mvwprintz( window, point( 1, 6 + item_line ), thiscolor, ">>%s", spaces );
            }
        } else if( !other_pane_conts.empty() && other_pane_conts.count( &it ) == 1 ) {
            pane.other_cont = item_line;
            thiscolor = i_brown;
            mvwprintz( window, point( 1, 6 + item_line ), thiscolor, spaces );
        }

        std::string item_name;
        std::string stolen_string;
        bool stolen = false;
        if( !it.is_owned_by( player_character, true ) ) {
            stolen_string = "<color_light_red>!</color>";
            stolen = true;
        }
        if( it.is_money() ) {
            //Count charges
            // TODO: transition to the item_location system used for the normal inventory
            unsigned int charges_total = 0;
            for( const item_location &item : sitem.items ) {
                charges_total += item->ammo_remaining( );
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
        trim_and_print( window, point( compact ? 1 : 4, 6 + item_line ), max_name_length, thiscolor,
                        item_name );

        //print src column
        // TODO: specify this is coming from a vehicle!
        if( pane.get_area() == AIM_ALL && !compact ) {
            mvwprintz( window, point( src_startpos, 6 + item_line ), thiscolor, squares[sitem.area].shortname );
        }

        //print "amount" column
        int it_amt = sitem.stacks;
        if( it_amt > 1 ) {
            print_color = thiscolor;
            if( it_amt > 9999 ) {
                it_amt = 9999;
                print_color = selected ? hilite( c_red ) : c_red;
            }
            mvwprintz( window, point( amt_startpos, 6 + item_line ), print_color, "%4d", it_amt );
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
        mvwprintz( window, point( weight_startpos, 6 + item_line ), print_color, "%5.*f", w_precision,
                   it_weight );

        //print volume column
        bool it_vol_truncated = false;
        double it_vol_value = 0.0;
        std::string it_vol = format_volume( sitem.volume, 5, &it_vol_truncated, &it_vol_value );
        if( it_vol_truncated && it_vol_value > 0.0 ) {
            print_color = selected ? hilite( c_red ) : c_red;
        } else {
            print_color = sitem.volume.value() > 0 ? thiscolor : thiscolordark;
        }
        mvwprintz( window, point( vol_startpos, 6 + item_line ), print_color, it_vol );

        if( active && sitem.autopickup ) {
            mvwprintz( window, point( 1, 6 + item_line ), magenta_background( it.color_in_inventory() ),
                       compact ? it.tname().substr( 0, 1 ) : ">" );
        }

        if( pagination.step( i ) ) { // page end
            break;
        }
    }
}

struct advanced_inv_sorter {
    advanced_inv_sortby sortby;
    explicit advanced_inv_sorter( advanced_inv_sortby sort ) {
        sortby = sort;
    }
    bool operator()( const advanced_inv_listitem &d1, const advanced_inv_listitem &d2 ) const {
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
            case SORTBY_DENSITY: {
                const double density1 = static_cast<double>( d1.weight.value() ) /
                                        static_cast<double>( std::max( 1, d1.volume.value() ) );
                const double density2 = static_cast<double>( d2.weight.value() ) /
                                        static_cast<double>( std::max( 1, d2.volume.value() ) );
                if( density1 != density2 ) {
                    return density1 > density2;
                }
                break;
            }
            case SORTBY_CHARGES:
                if( d1.items.front()->charges != d2.items.front()->charges ) {
                    return d1.items.front()->charges > d2.items.front()->charges;
                }
                break;
            case SORTBY_CATEGORY:
                if( d1.cat != d2.cat ) {
                    return d1.cat < d2.cat;
                }
                break;
            case SORTBY_DAMAGE: {
                const double dam1 = d1.items.front()->average_dps( get_player_character() );
                const double dam2 = d2.items.front()->average_dps( get_player_character() );
                if( dam1 != dam2 ) {
                    return dam1 > dam2;
                }
                break;
            }
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
                    return localized_compare( a1, a2 );
                }
            }
            break;
            case SORTBY_SPOILAGE:
                if( d1.items.front()->spoilage_sort_order() != d2.items.front()->spoilage_sort_order() ) {
                    return d1.items.front()->spoilage_sort_order() < d2.items.front()->spoilage_sort_order();
                }
                break;
            case SORTBY_PRICE:
                if( d1.items.front()->price( true ) != d2.items.front()->price( true ) ) {
                    return d1.items.front()->price( true ) > d2.items.front()->price( true );
                }
                break;
            case SORTBY_PRICEPERVOLUME: {
                const double price_density1 = static_cast<double>( d1.items.front()->price( true ) ) /
                                              static_cast<double>( std::max( 1, d1.items.front()->volume().value() ) );
                const double price_density2 = static_cast<double>( d2.items.front()->price( true ) ) /
                                              static_cast<double>( std::max( 1, d2.items.front()->volume().value() ) );
                if( price_density1 != price_density2 ) {
                    return price_density1 > price_density2;
                }
                break;
            }
            case SORTBY_PRICEPERWEIGHT: {
                const double price_density1 = static_cast<double>( d1.items.front()->price( true ) ) /
                                              static_cast<double>( std::max<std::int64_t>( 1, d1.items.front()->weight().value() ) );
                const double price_density2 = static_cast<double>( d2.items.front()->price( true ) ) /
                                              static_cast<double>( std::max<std::int64_t>( 1, d2.items.front()->weight().value() ) );
                if( price_density1 != price_density2 ) {
                    return price_density1 > price_density2;
                }
                break;
            }
            case SORTBY_STACKS:
                if( d1.stacks != d2.stacks ) {
                    return d1.stacks > d2.stacks;
                }
                break;
        }
        // secondary sort by name and link length
        auto const sort_key = []( advanced_inv_listitem const & d ) {
            return std::make_tuple( d.name_without_prefix, d.contents_count, d.name,
                                    d.items.front()->link_sort_key() );
        };
        return localized_compare( sort_key( d1 ), sort_key( d2 ) );
    }
};

int advanced_inventory::print_header( advanced_inventory_pane &pane, aim_location sel )
{
    const catacurses::window &window = pane.window;
    int area = pane.get_area();
    int wwidth = getmaxx( window );
    int ofs = wwidth - 25 - 2 - 14;
    int min_x = wwidth;
    for( int i = 0; i < NUM_AIM_LOCATIONS; ++i ) {
        int data_location = screen_relative_location( static_cast<aim_location>( i ) );
        bool can_put_items = squares[data_location].canputitems( pane.get_cur_item_ptr() != nullptr ?
                             pane.get_cur_item_ptr()->items.front() : item_location::nowhere );
        const char *bracket = pane.container_base_loc == data_location ||
                              data_location == AIM_CONTAINER ? "><" :
                              squares[data_location].can_store_in_vehicle() ||
                              data_location == AIM_PARENT ? "<>" : "[]";
        bool in_vehicle = pane.in_vehicle() && data_location == area && sel == area &&
                          area != AIM_ALL;
        bool all_brackets = area == AIM_ALL && ( data_location >= AIM_SOUTHWEST &&
                            data_location <= AIM_NORTHEAST );
        nc_color bcolor = c_red;
        nc_color kcolor = c_red;
        // Highlight location [#] if it can recieve items,
        // or highlight container [C] if container mode is active.
        if( can_put_items ) {
            bcolor = in_vehicle ? c_light_blue :
                     pane.container_base_loc == data_location ? c_brown :
                     data_location == AIM_CONTAINER ? c_dark_gray :
                     area == data_location || all_brackets ? c_light_gray : c_dark_gray;
            kcolor = data_location == AIM_CONTAINER ? c_dark_gray :
                     area == data_location ? c_white :
                     sel == data_location || pane.container_base_loc == data_location ? c_light_gray : c_dark_gray;
        } else if( data_location == AIM_PARENT && pane.container ) {
            bcolor = c_light_gray;
            kcolor = c_white;
        }
        const std::string key = get_location_key( static_cast<aim_location>( i ) );
        const point p( squares[i].hscreen + point( ofs, 0 ) );
        min_x = std::min( min_x, p.x );
        mvwprintz( window, p, bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%s", in_vehicle && sel != AIM_DRAGGED ? "V" : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }

    return min_x;
}

void advanced_inventory::recalc_pane( side p )
{
    advanced_inventory_pane &pane = panes[p];
    pane.recalc = false;
    pane.items.clear();
    advanced_inventory_pane &there = panes[-p + 1];
    advanced_inv_area &other = squares[there.get_area()];
    avatar &player_character = get_avatar();
    if( pane.container &&
        pane.container_base_loc >= AIM_SOUTHWEST && pane.container_base_loc <= AIM_ALL ) {

        const tripoint_rel_ms offset = player_character.pos_abs() - pane.container.pos_abs();

        // If container is no longer adjacent or on the player's z-level, nullify it.
        if( std::abs( offset.x() ) > 1 || std::abs( offset.y() ) > 1 ||
            player_character.posz() != pane.container.pos_abs().z() ) {

            pane.container = item_location::nowhere;
            pane.container_base_loc = NUM_AIM_LOCATIONS;
        } else if( pane.container_base_loc <= AIM_NORTHEAST ) {
            pane.container_base_loc = static_cast<aim_location>( ( offset.y() + 1 ) * 3 - offset.x() + 2 );
        }
    }
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.
    if( pane.get_area() == AIM_ALL ) {
        advanced_inv_area &alls = squares[AIM_ALL];
        alls.volume = 0_ml;
        alls.weight = 0_gram;
        for( advanced_inv_area &s : squares ) {
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
                alls.volume += s.volume_veh;
                alls.weight += s.weight_veh;
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

    // Sort all items
    std::stable_sort( pane.items.begin(), pane.items.end(), advanced_inv_sorter( pane.sortby ) );

    // Attempt to move to the target item if there is one.
    if( pane.target_item_after_recalc ) {
        for( size_t i = 0; i < pane.items.size(); i++ ) {
            if( pane.items[i].items.front() == pane.target_item_after_recalc ) {
                pane.index = i;
                pane.target_item_after_recalc = item_location::nowhere;
                break;
            }
        }
    }
}

void advanced_inventory::redraw_pane( side p )
{
    input_context ctxt( "ADVANCED_INVENTORY" );

    advanced_inventory_pane &pane = panes[p];
    if( recalc || pane.recalc ) {
        recalc_pane( p );
    }
    pane.fix_index();

    const bool active = p == src;
    const advanced_inv_area &square = squares[pane.get_area()];
    catacurses::window w = pane.window;

    werase( w );
    print_items( p, active );

    advanced_inv_listitem *itm = pane.get_cur_item_ptr();
    int width = print_header( pane, itm != nullptr ? itm->area : pane.get_area() );
    // only cardinals
    // not where you stand, and pane is in vehicle
    // make sure the offsets are the same as the grab point
    bool same_as_dragged = ( square.id >= AIM_SOUTHWEST && square.id <= AIM_NORTHEAST ) &&
                           square.id != AIM_CENTER && panes[p].in_vehicle() &&
                           square.off == squares[AIM_DRAGGED].off;
    const advanced_inv_area &sq = same_as_dragged ? squares[AIM_DRAGGED] : square;
    bool car = square.can_store_in_vehicle() && panes[p].in_vehicle() && sq.id != AIM_DRAGGED;
    std::string name = utf8_truncate( car ? sq.veh->name : sq.name, width );
    std::string desc = pane.container ? pane.container->tname( 1, false ) : sq.desc[car];
    // starts at offset 2, plus space between the header and the text
    width -= 2 + 1;
    trim_and_print( w, point( 2, 1 ), width, active ? c_green  : c_light_gray, name );
    trim_and_print( w, point( 2, 2 ), width, active ? c_light_blue : c_dark_gray, desc );
    trim_and_print( w, point( 2, 3 ), width, active ? c_cyan : c_dark_gray, square.flags );

    if( active ) {
        advanced_inventory_pagination pagination( linesPerPage, pane );
        int cur_page = 0;
        for( int i = 0; i < static_cast<int>( pane.items.size() ); i++ ) {
            pagination.step( i );
            if( i == pane.index ) {
                cur_page = pagination.page;
            }
        }
        const int max_page = pagination.page;
        mvwprintz( w, point( 2, 4 ), c_light_blue, _( "[<] page %1$d of %2$d [>]" ), cur_page + 1,
                   max_page + 1 );
    }

    if( active ) {
        wattron( w, c_cyan );
    }
    // draw a darker border around the inactive pane
    draw_border( w, active ? BORDER_COLOR : c_dark_gray );
    mvwprintw( w, point( 3, 0 ), _( "< [%s] Sort: %s >" ), ctxt.get_desc( "SORT" ),
               get_sortname( pane.sortby ) );
    int max = square.max_size;
    if( max > 0 ) {
        int itemcount = square.get_item_count();
        int fmtw = 7 + ( itemcount > 99 ? 3 : itemcount > 9 ? 2 : 1 ) +
                   ( max > 99 ? 3 : max > 9 ? 2 : 1 );
        mvwprintw( w, point( w_width / 2 - fmtw, 0 ), "< %d/%d >", itemcount, max );
    }

    std::string fprefix = string_format( _( "[%s] Filter" ), ctxt.get_desc( "FILTER" ) );
    const std::string &filter = pane.get_filter();
    if( !filter_edit ) {
        if( !filter.empty() ) {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s: %s >", fprefix, filter );
        } else {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s >", fprefix );
        }
    }
    if( active ) {
        wattroff( w, c_white );
    }
    if( !filter_edit && !filter.empty() ) {
        std::string fsuffix = string_format( _( "[%s] Reset" ), ctxt.get_desc( "RESET_FILTER" ) );
        mvwprintz( w, point( 6 + utf8_width( fprefix ), getmaxy( w ) - 1 ), c_white, filter );
        mvwprintz( w, point( getmaxx( w ) - utf8_width( fsuffix ) - 2, getmaxy( w ) - 1 ), c_white, "%s",
                   fsuffix );
    }
}

bool advanced_inventory::fill_lists_with_pane_items( Character &player_character,
        advanced_inv_sortby sort_priority,
        advanced_inventory_pane &spane, advanced_inventory_pane &dpane,
        std::vector<drop_or_stash_item_info> &item_list,
        std::vector<drop_or_stash_item_info> &fav_list, bool forbid_buckets )
{
    std::vector<move_all_entry> unsorted_item_list;
    std::vector<move_all_entry> unsorted_fav_list;
    item_location wielded = player_character.get_wielded_item();
    bool try_unwield = false;
    item_location stashed_bucket;
    for( const advanced_inv_listitem &listit : spane.items ) {
        if( listit.items.front() == dpane.container ) {
            continue;
        }
        for( const item_location &it : listit.items ) {

            // do not move liquids or gases
            if( ( it->made_of_from_type( phase_id::LIQUID ) && !it->is_frozen_liquid() ) ||
                it->made_of_from_type( phase_id::GAS ) ) {
                continue;
            }
            if( dpane.get_area() == AIM_INVENTORY ) {

                if( !player_character.can_stash_partial( *it ) ) {
                    continue;
                }
            } else if( dpane.container &&
                       !dpane.container->can_contain_directly( *it ).success() ) {
                continue;
            }
            if( it->is_corpse() && !it->empty_container() ) {
                // Only allow moving corpses if they're empty.
                continue;
            }
            if( forbid_buckets && it->is_bucket_nonempty() ) {
                // Don't allow putting nonempty buckets into pockets.
                stashed_bucket = it;
                continue;
            } else if( it == wielded ) {
                // Only allow moving wielded item if it's the only valid item left.
                try_unwield = true;
                continue;
            }

            if( sort_priority == advanced_inv_sortby::SORTBY_NONE ) {
                if( it->is_favorite ) {
                    fav_list.emplace_back( it, it->count() );
                } else {
                    item_list.emplace_back( it, it->count() );
                }
            } else {
                int weight_int = it->weight().value() > INT_MAX ? INT_MAX :
                                 static_cast<int>( it->weight().value() );

                std::pair<int, int> sort_values = sort_priority == advanced_inv_sortby::SORTBY_VOLUME ?
                                                  std::make_pair( it->volume().value(), weight_int ) :
                                                  std::make_pair( weight_int, it->volume().value() );
                if( it->is_favorite ) {
                    unsorted_fav_list.emplace_back( sort_values, drop_or_stash_item_info( it, it->count() ) );
                } else {
                    unsorted_item_list.emplace_back( sort_values, drop_or_stash_item_info( it, it->count() ) );
                }
            }
        }
    }

    if( item_list.empty() && fav_list.empty() &&
        unsorted_item_list.empty() && unsorted_fav_list.empty() ) {

        if( stashed_bucket ) {
            if( !query_yn( _( "The %s would spill if stored there.  Store its contents first?" ),
                           stashed_bucket->tname() ) ) {
                return false;
            }
            for( item *it : stashed_bucket->get_contents().all_items_top() ) {
                item_list.emplace_back( item_location( stashed_bucket, it ), it->count() );
            }
            if( stashed_bucket->is_favorite ) {
                fav_list.emplace_back( stashed_bucket, stashed_bucket->count() );
            } else {
                item_list.emplace_back( stashed_bucket, stashed_bucket->count() );
            }
            return true;

        } else if( try_unwield ) {
            if( !query_yn( _( "Unwield the %s?" ), wielded->tname() ) ) {
                return false;
            }
            if( wielded->is_favorite ) {
                fav_list.emplace_back( wielded, wielded->count() );
            } else {
                item_list.emplace_back( wielded, wielded->count() );
            }
            return true;
        }
        popup_getkey( _( "None of the items can be moved there." ) );
        return false;
    }

    if( sort_priority == advanced_inv_sortby::SORTBY_NONE ) {
        return true;
    }

    auto sort = [&dpane]( const move_all_entry & lhs, const move_all_entry & rhs ) {
        // pickup_activity_actor processes from the back, so reverse the order if moving to inventory.
        if( lhs.first.first == rhs.first.first ) {
            return dpane.get_area() == AIM_INVENTORY ?
                   lhs.first.second > rhs.first.second : lhs.first.second < rhs.first.second;
        }
        return dpane.get_area() == AIM_INVENTORY ?
               lhs.first.first > rhs.first.first : lhs.first.first < rhs.first.first;
    };
    std::sort( std::begin( unsorted_item_list ), std::end( unsorted_item_list ), sort );
    std::sort( std::begin( unsorted_fav_list ), std::end( unsorted_fav_list ), sort );

    for( const move_all_entry &entry : unsorted_item_list ) {
        item_list.push_back( entry.second );
    }
    for( const move_all_entry &entry : unsorted_fav_list ) {
        fav_list.push_back( entry.second );
    }
    return true;
}

bool advanced_inventory::move_all_items()
{
    advanced_inventory_pane &spane = panes[src];
    advanced_inventory_pane &dpane = panes[dest];

    Character &player_character = get_player_character();

    // Check some preconditions to quickly leave the function.
    if( spane.get_area() == AIM_CONTAINER && dpane.get_area() == AIM_INVENTORY ) {
        if( spane.container.held_by( player_character ) ) {
            // TODO: Implement this, distributing the contents to other inventory pockets.
            popup_getkey( _( "You already have everything in that container." ) );
            return false;
        }
    }
    if( spane.get_area() == AIM_CONTAINER &&
        spane.container.get_item()->has_flag( json_flag_NO_UNLOAD ) ) {
        popup_getkey( _( "Source container can't be unloaded." ) );
        return false;
    }
    if( dpane.get_area() == AIM_CONTAINER &&
        dpane.container.get_item()->has_flag( json_flag_NO_RELOAD ) ) {
        popup_getkey( _( "Destination container can't be reloaded." ) );
        return false;
    }
    size_t liquid_items = 0;
    for( const advanced_inv_listitem &elem : spane.items ) {
        for( const item_location &elemit : elem.items ) {
            if( ( elemit->made_of_from_type( phase_id::LIQUID ) && !elemit->is_frozen_liquid() ) ||
                elemit->made_of_from_type( phase_id::GAS ) ) {
                liquid_items++;
            }
        }
    }

    if( spane.items.empty() || liquid_items == spane.items.size() ) {
        if( !is_processing() ) {
            popup_getkey( _( "No eligible items found to be moved." ) );
        } else if( spane.get_area() != AIM_ALL ) {
            // ensure we don't get stuck if the recursive calls in the switch above were interrupted
            // by a save-load cycle before the shadowed pane was restored
            spane.set_area( AIM_ALL );
        }
        return false;
    }
    std::unique_ptr<on_out_of_scope> restore_area;
    if( dpane.get_area() == AIM_ALL ) {
        aim_location loc = dpane.get_area();
        // ask where we want to store the item via the menu
        if( !query_destination( loc ) ) {
            return false;
        }
        restore_area = std::make_unique<on_out_of_scope>( [&]() {
            dpane.restore_area();
        } );
    }
    if( !squares[dpane.get_area()].canputitems( dpane.container ) ) {
        popup_getkey( _( "You can't put items there." ) );
        return false;
    }
    advanced_inv_area &sarea = squares[spane.get_area()];
    advanced_inv_area &darea = squares[dpane.get_area()];

    // Make sure source and destination are different, otherwise items will disappear
    // Need to check actual position to account for dragged vehicles
    if( dpane.get_area() == AIM_DRAGGED && sarea.pos == darea.pos &&
        spane.in_vehicle() == dpane.in_vehicle() ) {
        return false;
    }

    if( spane.get_area() == dpane.get_area() && spane.in_vehicle() == dpane.in_vehicle() &&
        spane.container == dpane.container ) {
        return false;
    }

    if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
        if( dpane.get_area() == AIM_INVENTORY ) {
            popup_getkey( _( "You try to put your bags into themselves, but physics won't let you." ) );
            return false;
        } else if( dpane.get_area() == AIM_WORN ) {
            // TODO: implement move_all to worn from inventory.
            popup_getkey(
                _( "Putting on everything from your inventory would be tricky.  Try equipping one by one." ) );
            return false;
        }
    }

    if( dpane.get_area() == AIM_WORN ) {
        // TODO: implement move_all to worn from everywhere other than inventory.
        popup_getkey(
            _( "You look at the items, then your clothes, and scratch your head…  Try equipping one by one." ) );
        return false;
    }

    // Check first if the destination area still has enough room for moving all.
    advanced_inv_sortby sort_priority = advanced_inv_sortby::SORTBY_NONE;
    std::string limitation;
    if( !is_processing() ) {
        units::volume over_volume = 0_ml;
        units::mass over_weight = 0_gram;

        const units::volume &src_volume = spane.in_vehicle() ? sarea.volume_veh : sarea.volume;
        const units::volume dest_volume_free = dpane.free_volume( darea );
        over_volume = src_volume - dest_volume_free;

        if( dpane.get_area() == AIM_INVENTORY || dpane.get_area() == AIM_CONTAINER ) {
            const units::mass &src_weight = spane.in_vehicle() ? sarea.weight_veh : sarea.weight;
            const units::mass dest_weight_free = dpane.free_weight_capacity();
            over_weight = src_weight - dest_weight_free;
        }

        if( over_volume > 0_ml && over_weight > 0_gram ) {
            limitation = _( "room or weight capacity" );
            // Prioritize whichever one is closest to the limit
            sort_priority = units::to_milliliter<int>( over_volume ) < units::to_gram<int>( over_weight ) ?
                            SORTBY_VOLUME : SORTBY_WEIGHT;
        } else if( over_volume > 0_ml ) {
            limitation = pgettext( "As in \"not enough room in the backpack\"", "room" );
            sort_priority = SORTBY_VOLUME;
        } else if( over_weight > 0_gram ) {
            limitation = _( "weight capacity" );
            sort_priority = SORTBY_WEIGHT;
        }
    }

    std::vector<drop_or_stash_item_info> pane_items;
    // Keep a list of favorites separated, only drop non-fav first if they exist.
    std::vector<drop_or_stash_item_info> pane_favs;
    bool forbid_buckets = dpane.get_area() == AIM_INVENTORY || dpane.get_area() == AIM_WORN ||
                          dpane.get_area() == AIM_CONTAINER || dpane.in_vehicle();

    if( !fill_lists_with_pane_items( player_character, sort_priority, spane, dpane,
                                     pane_items, pane_favs, forbid_buckets ) ) {
        return false;
    }

    // Move all the favorite items only if there are no other items
    if( pane_items.empty() ) {
        // Check if the list is still empty for when all that's in the aim_worn list is a wielded weapon.
        if( pane_favs.empty() ) {
            popup( _( "None of the items can be moved there." ) );
            return false;
        }
        // Ask to move favorites if the player is holding them
        if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
            if( !query_yn( _( "Really drop all your favorite items?" ) ) ) {
                return false;
            }
        }
        pane_items = pane_favs;
    }

    if( !limitation.empty() &&
        !query_yn( _( "There isn't enough %s.  Attempt to move as much as you can?" ),
                   limitation ) ) {
        return false;
    }

    if( dpane.get_area() == AIM_CONTAINER ) {
        if( dpane.container ) {
            drop_locations items_to_insert;

            for( const drop_or_stash_item_info &drop : pane_items ) {
                items_to_insert.emplace_back( drop.loc(), drop.count() );
            }
            do_return_entry();

            const insert_item_activity_actor act( dpane.container, items_to_insert );
            player_character.assign_activity( act );
        }
    } else if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ) {
        const tripoint_rel_ms placement = darea.off;
        // in case there is vehicle cargo space at dest but the player wants to drop to ground
        const bool force_ground = !dpane.in_vehicle();

        do_return_entry();

        const drop_activity_actor act( pane_items, placement, force_ground );
        player_character.assign_activity( act );
    } else if( dpane.get_area() == AIM_INVENTORY ) {
        std::vector<item_location> target_items;
        std::vector<int> quantities;
        target_items.reserve( pane_items.size() );
        quantities.reserve( pane_items.size() );
        for( const drop_or_stash_item_info &drop : pane_items ) {
            target_items.emplace_back( drop.loc() );
            // quantity of 0 means move all
            quantities.emplace_back( 0 );
        }

        do_return_entry();

        const pickup_activity_actor act( target_items, quantities, player_character.pos_bub(), false );
        player_character.assign_activity( act );
    } else {
        // Vehicle and map destinations are handled the same.

        // Stash the destination
        const tripoint_rel_ms relative_destination = darea.off;

        std::vector<item_location> target_items;
        std::vector<int> quantities;
        target_items.reserve( pane_items.size() );
        quantities.reserve( pane_items.size() );
        for( const drop_or_stash_item_info &drop : pane_items ) {
            target_items.emplace_back( drop.loc() );
            // quantity of 0 means move all
            quantities.emplace_back( 0 );
        }

        do_return_entry();

        const move_items_activity_actor act( target_items, quantities, dpane.in_vehicle(),
                                             relative_destination );
        player_character.assign_activity( act );
    }

    return true;
}

bool advanced_inventory::show_sort_menu( advanced_inventory_pane &pane )
{
    uilist sm;
    sm.text = _( "Sort by…" );
    sm.addentry( SORTBY_NONE,           true, 'u', _( "Unsorted (recently added first)" ) );
    sm.addentry( SORTBY_NAME,           true, 'n', get_sortname( SORTBY_NAME ) );
    sm.addentry( SORTBY_WEIGHT,         true, 'w', get_sortname( SORTBY_WEIGHT ) );
    sm.addentry( SORTBY_VOLUME,         true, 'v', get_sortname( SORTBY_VOLUME ) );
    sm.addentry( SORTBY_DENSITY,        true, 'd', get_sortname( SORTBY_DENSITY ) );
    sm.addentry( SORTBY_CHARGES,        true, 'x', get_sortname( SORTBY_CHARGES ) );
    sm.addentry( SORTBY_CATEGORY,       true, 'c', get_sortname( SORTBY_CATEGORY ) );
    sm.addentry( SORTBY_DAMAGE,         true, 'o', get_sortname( SORTBY_DAMAGE ) );
    sm.addentry( SORTBY_AMMO,           true, 'a', get_sortname( SORTBY_AMMO ) );
    sm.addentry( SORTBY_SPOILAGE,       true, 's', get_sortname( SORTBY_SPOILAGE ) );
    sm.addentry( SORTBY_PRICE,          true, 'b', get_sortname( SORTBY_PRICE ) );
    sm.addentry( SORTBY_PRICEPERVOLUME, true, 'r', get_sortname( SORTBY_PRICEPERVOLUME ) );
    sm.addentry( SORTBY_PRICEPERWEIGHT, true, 'g', get_sortname( SORTBY_PRICEPERWEIGHT ) );
    sm.addentry( SORTBY_STACKS,         true, 't', get_sortname( SORTBY_STACKS ) );
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

input_context advanced_inventory::register_ctxt() const
{
    input_context ctxt( "ADVANCED_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "HOME" );
    ctxt.register_action( "END" );
    ctxt.register_action( "TOGGLE_TAB" );
    ctxt.register_action( "TOGGLE_VEH" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "EXAMINE_CONTENTS" );
    ctxt.register_action( "UNLOAD_CONTAINER" );
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
    ctxt.register_action( "ITEMS_PARENT" );

    ctxt.register_action( "ITEMS_DEFAULT" );
    ctxt.register_action( "SAVE_DEFAULT" );

    return ctxt;
}

void advanced_inventory::redraw_sidebar()
{
    input_context ctxt( "ADVANCED_INVENTORY" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    werase( head );
    werase( minimap );
    werase( mm_border );
    draw_border( head );
    Messages::display_messages( head, 2, 1, w_width - 1, head_height - 2 );
    draw_minimap();
    right_print( head, 0, +3, c_white, string_format(
                     _( "< [<color_yellow>%s</color>] keybindings >" ),
                     ctxt.get_desc( "HELP_KEYBINDINGS" ) ) );
    if( get_player_character().has_watch() ) {
        const std::string time = to_string_time_of_day( calendar::turn );
        mvwprintz( head, point( 2, 0 ), c_white, time );
    }
    wnoutrefresh( head );
    refresh_minimap();
}

void advanced_inventory::change_square( const aim_location changeSquare,
                                        advanced_inventory_pane &dpane, advanced_inventory_pane &spane )
{
    // Determine behavior if current pane is used.  AIM_CONTAINER should never swap to allow for multi-containers
    if( ( panes[left].get_area() == changeSquare || panes[right].get_area() == changeSquare ) &&
        changeSquare != AIM_CONTAINER && changeSquare != AIM_PARENT ) {
        if( squares[changeSquare].can_store_in_vehicle() && changeSquare != AIM_DRAGGED ) {
            // only deal with spane, as you can't _directly_ change dpane
            if( spane.get_area() == AIM_CONTAINER ) {
                spane.container = item_location::nowhere;
                spane.container_base_loc = NUM_AIM_LOCATIONS;
                // Update dpane to show items removed from the spane.
                dpane.recalc = true;
            }
            if( spane.get_area() == dpane.get_area() ) {
                // swap the `in_vehicle` element of each pane if "one in, one out"
                spane.set_area( squares[spane.get_area()], !spane.in_vehicle() );
                dpane.set_area( squares[dpane.get_area()], !dpane.in_vehicle() );
                recalc = true;
            } else if( dpane.get_area() == changeSquare ) {
                spane.set_area( squares[changeSquare], !dpane.in_vehicle() );
                spane.recalc = true;
            } else {
                spane.set_area( squares[spane.get_area()], !spane.in_vehicle() );
                spane.recalc = true;
            }
        } else {
            swap_panes();
        }
    } else if( changeSquare == AIM_PARENT && spane.container ) {
        spane.target_item_after_recalc = spane.container;
        if( spane.container.has_parent() ) {
            if( spane.container_base_loc == AIM_INVENTORY && !spane.container.parent_item().has_parent() ) {
                // If we're here from the inventory, skip past individual worn items straight to the full inventory view.
                change_square( AIM_INVENTORY, dpane, spane );
            } else {
                if( spane.container.parent_item() == dpane.container ) {
                    swap_panes();
                    return;
                }
                spane.container = spane.container.parent_item();
                spane.set_area( squares[AIM_CONTAINER], false );
            }
        } else {
            change_square( spane.container_base_loc, dpane, spane );
        }
        spane.recalc = true;
        spane.index = 0;
        dpane.recalc = true;
    } else if( squares[changeSquare].canputitems(
                   changeSquare == AIM_CONTAINER && spane.get_cur_item_ptr() != nullptr ?
                   spane.get_cur_item_ptr()->items.front() : item_location::nowhere ) ) {
        if( changeSquare == AIM_CONTAINER ) {
            item_location &target_container = spane.get_cur_item_ptr()->items.front();
            if( target_container == dpane.container ) {
                swap_panes();
                return;
            }
            // Set the pane's container to the selected item.
            spane.container = target_container;
            if( spane.get_area() != AIM_CONTAINER ) {
                spane.container_base_loc = spane.get_area();
            }
            // Update dpane to hide items added to the spane.
            dpane.recalc = true;
        } else {
            // Reset the pane's container whenever we're switching to something else.
            spane.container = item_location::nowhere;
            if( spane.get_area() == AIM_CONTAINER ) {
                spane.container_base_loc = NUM_AIM_LOCATIONS;
                // Update dpane to show items removed from the spane.
                dpane.recalc = true;
            }
        }
        // Check the original area if we can place items in vehicle storage.
        bool in_vehicle_cargo = false;
        if( squares[changeSquare].can_store_in_vehicle() && spane.get_area() != changeSquare ) {
            // auto select vehicle if items exist at said square, or both are empty
            if( changeSquare == AIM_DRAGGED ) {
                in_vehicle_cargo = true;
            } else {
                // check item stacks in vehicle and map at said square
                advanced_inv_area sq = squares[changeSquare];
                map_stack map_stack = get_map().i_at( sq.pos );
                vehicle_stack veh_stack = sq.get_vehicle_stack();
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
    } else {
        switch( changeSquare ) {
            case AIM_DRAGGED:
                popup_getkey( _( "You aren't dragging a vehicle." ) );
                break;
            case AIM_CONTAINER:
                popup_getkey( _( "That isn't a container." ) );
                break;
            case AIM_PARENT:
                popup_getkey( _( "You aren't in a container." ) );
                break;
            default:
                popup_getkey( _( "You can't put items there." ) );
                break;
        }
    }
}

void advanced_inventory::start_activity(
    const aim_location destarea,
    const aim_location /*srcarea*/,
    advanced_inv_listitem *sitem, int &amount_to_move,
    const bool from_vehicle, const bool to_vehicle ) const
{

    const bool by_charges = sitem->items.front()->count_by_charges();

    Character &player_character = get_player_character();

    if( destarea != AIM_CONTAINER ) {
        // Find target items and quantities thereof for the new activity
        std::vector<item_location> target_items;
        std::vector<int> quantities;
        if( by_charges ) {
            target_items.emplace_back( sitem->items.front() );
            quantities.push_back( amount_to_move );
        } else {
            for( std::vector<item_location>::iterator it = sitem->items.begin(); amount_to_move > 0 &&
                 it != sitem->items.end(); ++it ) {
                target_items.emplace_back( *it );
                quantities.push_back( 0 );
                --amount_to_move;
            }
            if( to_vehicle && sitem->items.front()->is_bucket_nonempty() ) {
                // Copy the pointers so they won't break if the list gets reordered as buckets are partly emptied.
                std::vector<item_location> target_buckets = sitem->items;
                if( target_buckets.front()->contains_no_solids() ) {
                    target_buckets.front()->handle_liquid_or_spill( player_character, &*target_buckets.front() );
                    if( !target_buckets.front()->empty_container() ) {
                        return;
                    }
                } else {
                    if( !query_yn( _( "The %s would spill if stored there.  Store its contents separately first?" ),
                                   target_buckets.front()->tname() ) ) {
                        return;
                    }
                    for( item *it : target_buckets.front()->get_contents().all_items_top() ) {
                        target_items.emplace_back( target_buckets.front(), it );
                        quantities.emplace_back( it->count() );
                    }
                }
            }
        }

        if( destarea == AIM_WORN ) {
            const wear_activity_actor act( target_items, quantities );
            player_character.assign_activity( act );
        } else if( destarea == AIM_WIELD ) {
            player_character.assign_activity(
                wield_activity_actor( target_items.front(), quantities.front() ) );
        } else if( destarea == AIM_INVENTORY ) {
            const std::optional<tripoint_bub_ms> starting_pos = from_vehicle
                    ? std::nullopt
                    : std::optional<tripoint_bub_ms>( player_character.pos_bub() );
            const pickup_activity_actor act( target_items, quantities, starting_pos, false );
            player_character.assign_activity( act );
        } else {
            // Stash the destination
            const tripoint_rel_ms relative_destination = squares[destarea].off;

            const move_items_activity_actor act( target_items, quantities, to_vehicle, relative_destination );
            player_character.assign_activity( act );
        }
    } else {
        if( !panes[dest].container.get_item() ) {
            debugmsg( "Active container is null, failed to insert!" );
            return;
        }
        if( sitem->items.front() == panes[dest].container ||
            sitem->items.front().eventually_contains( panes[dest].container ) ) {
            popup_getkey( _( "You can't put the %s inside itself." ), sitem->items.front()->type_name() );
            return;
        }
        if( panes[dest].container->will_spill_if_unsealed()
            && panes[dest].container.where() != item_location::type::map
            && !player_character.is_wielding( *panes[dest].container ) ) {

            popup_getkey( _( "The %s would spill; it must be on the ground or wielded." ),
                          panes[dest].container->type_name() );
            return;
        }

        // Create drop locations out of target items and quantities
        drop_locations target_inserts;
        if( by_charges ) {
            target_inserts.emplace_back( std::make_pair( sitem->items.front(), amount_to_move ) );
        } else {
            for( std::vector<item_location>::iterator it = sitem->items.begin(); amount_to_move > 0 &&
                 it != sitem->items.end(); ++it ) {
                target_inserts.emplace_back( std::make_pair( *it, 0 ) );
                --amount_to_move;
            }
            if( sitem->items.front()->is_bucket_nonempty() ) {
                // Copy the pointers so they won't break if the list gets reordered as buckets are partly emptied.
                std::vector<item_location> target_buckets = sitem->items;
                if( target_buckets.front()->contains_no_solids() ) {
                    target_buckets.front()->handle_liquid_or_spill( player_character, &*target_buckets.front() );
                    if( !target_buckets.front()->empty_container() ) {
                        return;
                    }
                } else {
                    if( !query_yn( _( "The %s would spill if stored there.  Store its contents separately first?" ),
                                   target_buckets.front()->tname() ) ) {
                        return;
                    }
                    // Emplace contents in front of the container because insert_item_activity_actor processes from the front.
                    for( item *it : target_buckets.front()->get_contents().all_items_top() ) {
                        target_inserts.emplace_front( item_location( target_buckets.front(), it ), it->count() );
                    }
                }
            }
        }

        const insert_item_activity_actor act( panes[dest].container, target_inserts, false, false );
        player_character.assign_activity( act );
    }
}

bool advanced_inventory::action_move_item( advanced_inv_listitem *sitem,
        advanced_inventory_pane &dpane, const advanced_inventory_pane &spane,
        const std::string &action )
{
    bool exit = false;
    if( sitem == nullptr ) {
        return false;
    }
    aim_location destarea = dpane.get_area();
    aim_location srcarea = sitem->area;
    bool restore_area = destarea == AIM_ALL;
    if( !query_destination( destarea ) ) {
        return false;
    }
    // Not necessarily equivalent to spane.in_vehicle() if using AIM_ALL
    bool from_vehicle = sitem->from_vehicle;
    bool to_vehicle = dpane.in_vehicle();

    // Same location check for AIM_CONTAINER and AIM_ALL.
    // AIM_ALL should disable same area check and handle it with proper filtering instead.
    // This is a workaround around the lack of vehicle location info in
    // either aim_location or advanced_inv_listitem.
    if( spane.container ) {
        if( spane.container == dpane.container ) {
            popup_getkey( _( "The %1$s is already in the %2$s." ),
                          sitem->items.front()->type_name(), dpane.container->type_name() );
            return false;
        }
    } else if( squares[srcarea].is_same( squares[destarea] ) &&
               spane.get_area() != AIM_ALL && spane.in_vehicle() == dpane.in_vehicle() ) {
        popup_getkey( _( "The %1$s is already there." ), sitem->items.front()->type_name() );
        return false;
    }
    cata_assert( !sitem->items.empty() );
    avatar &player_character = get_avatar();
    if( destarea == AIM_WORN ) {
        const item &itm = *sitem->items.front().get_item();
        ret_val<void> can_wear = player_character.can_wear( itm );
        ret_val<void> can_wield = player_character.can_wield( itm );
        if( can_wear.success() ) {
        } else if( can_wield.success() ) {
            destarea = AIM_WIELD;
            std::string err = can_wear.c_str();
            err += "\n";
            err += _( "Wield instead?" );
            if( !query_yn( err ) ) {
                return false;
            }
        } else {
            std::string err = can_wear.c_str();
            err += "\n";
            err += can_wield.c_str();
            popup_getkey( err.c_str(), PF_GET_KEY );
            return false;
        }
    }
    int amount_to_move = 0;
    if( !query_charges( destarea, *sitem, action, amount_to_move ) ) {
        return false;
    }
    item it_copy = *sitem->items.front();
    if( it_copy.count_by_charges() ) {
        it_copy.charges = std::min( amount_to_move, sitem->items.front()->charges );
    }
    if( spane.get_area() == AIM_CONTAINER &&
        spane.container.get_item()->has_flag( json_flag_NO_UNLOAD ) ) {
        popup_getkey( _( "Source container can't be unloaded." ) );
        return false;
    }
    if( destarea == AIM_CONTAINER ) {
        if( dpane.container.get_item()->has_flag( json_flag_NO_RELOAD ) ) {
            popup_getkey( _( "Destination container can't be reloaded." ) );
            return false;
        }
        ret_val<void> can_contain = dpane.container->can_contain( it_copy );
        if( can_contain.success() ) {
            can_contain = dpane.container.parents_can_contain_recursive( &it_copy );
        }
        if( !can_contain.success() ) {
            popup_getkey( can_contain.str().empty() ?
                          //~ %1$s: item we failed to put in the container, %2$s: container to put item in
                          string_format( _( "Could not put %1$s into %2$s." ),
                                         sitem->items.front()->tname(), dpane.container->tname() ).c_str() :
                          //~ %1$s: item we failed to put in the container, %2$s: container to put item in, %3$s: reason it failed
                          string_format( _( "Could not put %1$s into %2$s, %3$s." ),
                                         sitem->items.front()->tname(), dpane.container->tname(), can_contain.str() ).c_str() );
            return false;
        }
    }
    // This makes sure that all item references in the advanced_inventory_pane::items vector
    // are recalculated, even when they might not have changed, but they could (e.g. items
    // taken from inventory, but unable to put into the cargo trunk go back into the inventory,
    // but are potentially at a different place).
    recalc = true;
    cata_assert( amount_to_move > 0 );

    if( srcarea == AIM_CONTAINER && destarea == AIM_INVENTORY &&
        spane.container.held_by( player_character ) ) {
        popup_getkey( _( "The %s is already in your inventory.  You may want to (U)nload" ),
                      sitem->items.front()->tname() );

    } else if( srcarea == AIM_INVENTORY && destarea == AIM_WORN ) {

        // make sure advanced inventory is reopened after activity completion.
        do_return_entry();

        const wear_activity_actor act( { sitem->items.front() }, { amount_to_move } );
        player_character.assign_activity( act );
        // exit so that the activity can be carried out
        exit = true;

    } else if( srcarea == AIM_INVENTORY && destarea == AIM_WIELD ) {
        do_return_entry();
        const wield_activity_actor act( sitem->items.front(), amount_to_move );
        player_character.assign_activity( act );
        exit = true;
    } else if( srcarea == AIM_INVENTORY ||
               ( srcarea == AIM_WORN && sitem->items.front() != player_character.get_wielded_item() ) ) {
        cata_assert( destarea != AIM_WIELD );

        // make sure advanced inventory is reopened after activity completion
        do_return_entry();
        if( srcarea == AIM_WORN && destarea == AIM_INVENTORY ) {
            // if worn, we need to fix with the worn index number (starts at -2, as -1 is weapon)
            // this is ok because worn items are never stacked (can't move more than 1)
            player_character.takeoff( Character::worn_position_to_index( sitem->idx ) + 1 );
        } else {
            // important if item is worn
            if( player_character.can_drop( it_copy ).success() ) {
                if( destarea == AIM_CONTAINER ) {
                    do_return_entry();
                    start_activity( destarea, srcarea, sitem, amount_to_move, from_vehicle, to_vehicle );
                } else {
                    const tripoint_rel_ms placement = squares[destarea].off;
                    // incase there is vehicle cargo space at dest but the player wants to drop to ground
                    const bool force_ground = !to_vehicle;
                    std::vector<drop_or_stash_item_info> to_drop;

                    int remaining_amount = amount_to_move;
                    for( const item_location &itm : sitem->items ) {
                        if( remaining_amount <= 0 ) {
                            break;
                        }
                        const int move_amount = itm->count_by_charges() ?
                                                std::min( remaining_amount, itm->charges ) : 1;
                        to_drop.emplace_back( itm, move_amount );
                        remaining_amount -= move_amount;
                    }

                    const drop_activity_actor act( to_drop, placement, force_ground );
                    player_character.assign_activity( act );
                }
            }
        }

        // exit so that the activity can be carried out
        exit = true;
    } else {
        if( destarea == AIM_INVENTORY ) {
            bool can_stash = player_character.can_stash( it_copy );
            if( !can_stash ) {
                popup_getkey( _( "You have no space for the %s." ), sitem->items.front()->tname() );
                return false;
            }
        }
        // from map/vehicle: start ACT_PICKUP or ACT_MOVE_ITEMS as necessary
        // Make sure advanced inventory is reopened after activity completion.
        do_return_entry();
        start_activity( destarea, srcarea, sitem, amount_to_move, from_vehicle, to_vehicle );

        // exit so that the activity can be carried out
        exit = true;
    }

    // if dest was AIM_ALL then we used query_destination and should undo that
    if( restore_area ) {
        dpane.restore_area();
    }
    return exit;
}

void advanced_inventory::action_examine( advanced_inv_listitem *sitem,
        advanced_inventory_pane &spane )
{
    int ret = 0;
    const auto info_width = [this]() -> int {
        return w_width / 2;
    };
    const auto info_startx = [this]() -> int {
        return colstart + ( src == advanced_inventory::side::left ? w_width / 2 : 0 );
    };
    avatar &player_character = get_avatar();
    if( spane.get_area() == AIM_INVENTORY || spane.get_area() == AIM_WORN ||
        ( spane.container && spane.container.carrier() == player_character.as_character() ) ) {
        const item_location &loc = sitem->items.front();
        // Setup for a "return to AIM" in case examining the item creates a new activity
        // (e.g. reading, reloading, activating).
        activity_id last_activity = player_character.activity.id();
        // `inventory_item_menu` may call functions that move items, so we should
        // always recalculate during this period to ensure all item references are valid
        always_recalc = true;
        ret = g->inventory_item_menu( loc, info_startx, info_width,
                                      src == advanced_inventory::side::left ? game::LEFT_OF_INFO : game::RIGHT_OF_INFO );
        always_recalc = false;
        // If examining the item did create a new activity, we have to add "return to AIM".
        if( last_activity != player_character.activity.id() || !ui ) {
            exit = true;
            do_return_entry();
        } else {
            uistate.transfer_save.exit_code = aim_exit::none;
        }
        // Might have changed a stack (activated an item, repaired an item, etc.)
        if( spane.get_area() == AIM_INVENTORY ) {
            player_character.inv->restack( player_character );
        }
        recalc = true;
    } else {
        item &it = *sitem->items.front();
        std::vector<iteminfo> vThisItem;
        std::vector<iteminfo> vDummy;
        it.info( true, vThisItem );

        item_info_data data( it.tname(), it.type_name(), vThisItem, vDummy );
        data.handle_scrolling = true;
        data.arrow_scrolling = true;

        iteminfo_window info_window( data, point( info_startx(), 0 ), info_width(), TERMY );
        info_window.execute();
    }
    if( ret == KEY_NPAGE || ret == KEY_DOWN ) {
        spane.scroll_by( +1 );
    } else if( ret == KEY_PPAGE || ret == KEY_UP ) {
        spane.scroll_by( -1 );
    }
}

bool advanced_inventory::action_unload( advanced_inv_listitem *sitem,
                                        advanced_inventory_pane &spane, advanced_inventory_pane &dpane )
{
    avatar &u = get_avatar();
    item_location src = spane.container;
    item_location dest = dpane.container;

    if( !src && sitem ) {
        src = sitem->items.front();
    } else {
        add_msg( m_info, _( "Nothing to unload." ) );
        return false;
    }

    do_return_entry();
    // always exit to proc do_return_entry, even when no activity was assigned
    exit = true;
    return u.unload( src, false, dest );
}

void advanced_inventory::process_action( const std::string &input_action )
{
    recalc = false;
    // source and destination pane
    advanced_inventory_pane &spane = panes[src];
    advanced_inventory_pane &dpane = panes[dest];
    // current item in source pane, might be null
    advanced_inv_listitem *sitem = spane.get_cur_item_ptr();

    aim_location changeSquare = NUM_AIM_LOCATIONS;
    avatar &u = get_avatar();

    const std::string &action = is_processing() ? "MOVE_ALL_ITEMS" : input_action;
    if( action == "CATEGORY_SELECTION" ) {
        inCategoryMode = !inCategoryMode;
    } else if( action == "ITEMS_DEFAULT" ) {
        for( side cside : {
                 left, right
             } ) {
            advanced_inventory_pane &pane = panes[cside];
            int i_location = cside == left ? save_state->saved_area : save_state->saved_area_right;
            aim_location location = static_cast<aim_location>( i_location );
            if( pane.get_area() != location || location == AIM_ALL ) {
                pane.recalc = true;
            }
            pane.set_area( squares[location] );
        }
    } else if( action == "SAVE_DEFAULT" ) {
        save_state->saved_area = panes[left].get_area();
        save_state->saved_area_right = panes[right].get_area();
        popup( _( "Default layout was saved." ) );
    } else if( get_square( action, changeSquare ) ) {
        change_square( changeSquare, dpane, spane );
    } else if( action == "TOGGLE_FAVORITE" ) {
        if( sitem == nullptr ) {
            return;
        }
        for( item_location &item : sitem->items ) {
            item->set_favorite( !item->is_favorite );
        }
        // In case we've merged faved and unfaved items
        recalc = true;
    } else if( action == "MOVE_SINGLE_ITEM" ||
               action == "MOVE_VARIABLE_ITEM" ||
               action == "MOVE_ITEM_STACK" ) {
        exit = action_move_item( sitem, dpane, spane, action );
    } else if( action == "MOVE_ALL_ITEMS" ) {
        exit = move_all_items();
        recalc = true;
        if( exit ) {
            if( get_option<bool>( "CLOSE_ADV_INV" ) ) {
                move_all_items_and_waiting_to_quit = true;
            }
        }
    } else if( action == "SORT" ) {
        if( show_sort_menu( spane ) ) {
            recalc = true;
        }
    } else if( action == "FILTER" ) {
        const std::string &filter = spane.get_filter();
        filter_edit = true;
        if( ui ) {
            spopup = std::make_unique<string_input_popup>();
            spopup->max_length( 256 ).text( filter );
            spopup->identifier( "item_filter" ).hist_use_uilist( false );
            ui->mark_resize();
        }

        do {
            if( ui ) {
                ui_manager::redraw();
            }
            std::string new_filter = spopup->query_string( false );
            if( spopup->canceled() ) {
                // restore original filter
                spane.set_filter( filter );
            } else {
                spane.set_filter( new_filter );
            }
        } while( !spopup->canceled() && !spopup->confirmed() );
        filter_edit = false;
        spopup = nullptr;
    } else if( action == "RESET_FILTER" ) {
        spane.set_filter( "" );
    } else if( action == "TOGGLE_AUTO_PICKUP" ) {
        if( sitem == nullptr ) {
            return;
        }
        if( sitem->autopickup ) {
            get_auto_pickup().remove_rule( &*sitem->items.front() );
            sitem->autopickup = false;
        } else {
            get_auto_pickup().add_rule( &*sitem->items.front(), true );
            sitem->autopickup = true;
        }
        recalc = true;
    } else if( action == "EXAMINE" ) {
        if( sitem == nullptr ) {
            return;
        }
        action_examine( sitem, spane );
    } else if( action == "EXAMINE_CONTENTS" ) {
        if( sitem == nullptr ) {
            return;
        }
        item_location sitem_location = sitem->items.front();
        inventory_examiner examine_contents( u, sitem_location );
        examine_contents.add_contained_items( sitem_location );
        int examine_result = examine_contents.execute();
        if( examine_result == NO_CONTENTS_TO_EXAMINE ) {
            action_examine( sitem, spane );
        }
    } else if( action == "UNLOAD_CONTAINER" ) {
        recalc = action_unload( sitem, spane, dpane );
    } else if( action == "QUIT" ) {
        exit = true;
    } else if( action == "PAGE_DOWN" ) {
        spane.scroll_page( linesPerPage, +1 );
    } else if( action == "PAGE_UP" ) {
        spane.scroll_page( linesPerPage, -1 );
    } else if( action == "HOME" ) {
        spane.scroll_to_start();
    } else if( action == "END" ) {
        spane.scroll_to_end();
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
    } else if( action == "RIGHT" ) {
        src = right;
    } else if( action == "TOGGLE_TAB" ) {
        src = dest;
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
            }
        } else {
            popup_getkey( _( "There's no vehicle storage space there." ) );
        }
    }
    dest = src == advanced_inventory::side::left ? advanced_inventory::side::right :
           advanced_inventory::side::left;
}

void advanced_inventory::display()
{
    avatar &player_character = get_avatar();
    input_context ctxt{ register_ctxt() };

    exit = false;
    if( !is_processing() ) {

        player_character.inv->restack( player_character );

        recalc = true;
        g->wait_popup_reset();
    }

    if( !ui ) {
        init();
        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [&]( ui_adaptor & ui ) {
            constexpr int min_w_height = 10;
            const int min_w_width = FULL_SCREEN_WIDTH;
            const int max_w_width = get_option<bool>( "AIM_WIDTH" ) ? TERMX : std::max( 120,
                                    TERMX - 2 * ( panel_manager::get_manager().get_width_right() +
                                                  panel_manager::get_manager().get_width_left() ) );

            w_height = TERMY < min_w_height + head_height ? min_w_height : TERMY - head_height;
            w_width = TERMX < min_w_width ? min_w_width : TERMX > max_w_width ? max_w_width :
                      static_cast<int>( TERMX );

            //(TERMY>w_height)?(TERMY-w_height)/2:0;
            headstart = 0;
            colstart = TERMX > w_width ? ( TERMX - w_width ) / 2 : 0;

            head = catacurses::newwin( head_height, w_width - minimap_width, point( colstart, headstart ) );
            mm_border = catacurses::newwin( minimap_height + 2, minimap_width + 2,
                                            point( colstart + ( w_width - ( minimap_width + 2 ) ), headstart ) );
            minimap = catacurses::newwin( minimap_height, minimap_width,
                                          point( colstart + ( w_width - ( minimap_width + 1 ) ), headstart + 1 ) );
            panes[left].window = catacurses::newwin( w_height, w_width / 2, point( colstart,
                                 headstart + head_height ) );
            panes[right].window = catacurses::newwin( w_height, w_width / 2, point( colstart + w_width / 2,
                                  headstart + head_height ) );

            // 2 for the borders, 5 for the header stuff
            linesPerPage = w_height - 2 - 5;

            if( filter_edit && spopup ) {
                spopup->window( panes[src].window, point( 4, w_height - 1 ), w_width / 2 - 4 );
            }

            ui.position( point( colstart, headstart ), point( w_width, head_height + w_height ) );
        } );
        ui->mark_resize();

        ui->on_redraw( [&]( const ui_adaptor & ) {
            if( always_recalc ) {
                recalc = true;
            }

            redraw_pane( advanced_inventory::side::left );
            redraw_pane( advanced_inventory::side::right );
            if( panes[0].other_cont > -1 && panes[0].other_cont < linesPerPage ) {
                mvwprintz( panes[0].window, point( w_width / 2 - 1, panes[0].other_cont + 6 ), i_brown, " " );
                mvwprintz( panes[1].window, point( 0, panes[0].other_cont + 6 ), c_brown, "▶" );
            } else if( panes[1].other_cont > -1 && panes[1].other_cont < linesPerPage ) {
                mvwprintz( panes[0].window, point( w_width / 2 - 1, panes[1].other_cont + 6 ), c_brown, "◀" );
                mvwprintz( panes[1].window, point( 0, panes[1].other_cont + 6 ), i_brown, " " );
            }
            wnoutrefresh( panes[src].window );
            wnoutrefresh( panes[dest].window );
            redraw_sidebar();

            if( filter_edit && spopup ) {
                draw_item_filter_rules( panes[dest].window, 1, w_height - 2, item_filter_type::FILTER );
                mvwprintz( panes[src].window, point( 2, getmaxy( panes[src].window ) - 1 ), c_cyan, "< " );
                mvwprintz( panes[src].window, point( w_width / 2 - 4, getmaxy( panes[src].window ) - 1 ), c_cyan,
                           " >" );
                spopup->query_string( /*loop=*/false, /*draw_only=*/true );
            }
        } );
    }

    while( !exit ) {
        if( player_character.get_moves() < 0 ) {
            do_return_entry();
            return;
        }

        if( ui ) {
            ui->invalidate_ui();
            if( recalc ) {
                g->invalidate_main_ui_adaptor();
            }
            ui_manager::redraw_invalidated();
        }

        if( !is_processing() && move_all_items_and_waiting_to_quit ) {
            break;
        }

        process_action( ctxt.handle_input() );
    }
}

class query_destination_callback : public uilist_callback
{
    private:
        advanced_inventory &_adv_inv;
        // Render a fancy ASCII grid at the left of the menu.
        void draw_squares( const uilist *menu );
    public:
        explicit query_destination_callback( advanced_inventory &adv_inv ) : _adv_inv( adv_inv ) {}
        void refresh( uilist *menu ) override {
            draw_squares( menu );
        }
        float desired_extra_space_left( ) override {
            cataimgui::PushMonoFont();
            float rv = ImGui::CalcTextSize( "[1][2][3]" ).x + ( 3.0 * ImGui::GetStyle().ItemSpacing.x );
            ImGui::PopFont();
            return rv;
        }
};

void query_destination_callback::draw_squares( const uilist *menu )
{
    ImGui::TableSetColumnIndex( 0 );
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();
    cata_assert( menu->entries.size() >= 9 );
    int sel = 0;
    if( menu->previewing >= 0 && static_cast<size_t>( menu->previewing ) < menu->entries.size() ) {
        sel = _adv_inv.screen_relative_location(
                  static_cast <aim_location>( menu->previewing + 1 ) );
    }
    cataimgui::PushMonoFont();
    for( int i = 7; i >= 1; i -= 3 ) { // 7,4,1
        for( int ii = 0; ii <= 2; ii++ ) { // +0,1,2
            aim_location loc = _adv_inv.screen_relative_location( static_cast <aim_location>( i + ii ) );
            std::string key = _adv_inv.get_location_key( loc );
            advanced_inv_area &square = _adv_inv.get_one_square( loc );
            bool in_vehicle = square.can_store_in_vehicle();
            const char *bracket = in_vehicle ? "<>" : "[]";
            // always show storage option for vehicle storage, if applicable
            bool canputitems = menu->entries[i + ii - 1].enabled && square.canputitems();
            nc_color bcolor = canputitems ? ( sel == loc ? h_white : c_light_gray ) : c_red;
            nc_color kcolor = canputitems ? ( sel == loc ? h_white : c_dark_gray ) : c_red;
            // TODO(db48x): maybe make these clickable buttons or something
            ImGui::PushID( i + ii );
            ImGui::BeginGroup();
            ImGui::TextColored( bcolor, "%c", bracket[0] );
            ImGui::SameLine( 0.0, 0.0 );
            ImGui::TextColored( kcolor, "%s", key.c_str() );
            ImGui::SameLine( 0.0, 0.0 );
            ImGui::TextColored( bcolor, "%c", bracket[1] );
            ImGui::EndGroup();
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
    ImGui::PopFont();
}

bool advanced_inventory::query_destination( aim_location &def )
{
    if( def != AIM_ALL ) {
        if( squares[def].canputitems( panes[dest].container ) ) {
            return true;
        }
        popup_getkey( _( "You can't put items there." ) );
        return false;
    }

    uilist menu;
    menu.text = _( "Select destination" );
    query_destination_callback cb( *this );
    menu.callback = &cb;

    {
        std::vector <aim_location> ordered_locs;
        static_assert( AIM_NORTHEAST - AIM_SOUTHWEST == 8,
                       "Expected 9 contiguous directions in the aim_location enum" );
        for( int i = AIM_SOUTHWEST; i <= AIM_NORTHEAST; i++ ) {
            ordered_locs.push_back( screen_relative_location( static_cast <aim_location>( i ) ) );
        }
        for( aim_location &ordered_loc : ordered_locs ) {
            advanced_inv_area &s = squares[ordered_loc];
            const int size = s.get_item_count();
            std::string prefix = string_format( "%2d/%d", size, MAX_ITEM_IN_SQUARE );
            if( size >= MAX_ITEM_IN_SQUARE ) {
                prefix += _( " (FULL)" );
            }
            if( s.veh != nullptr ) {
                menu.addentry_col( ordered_loc,
                                   s.canputitems() && s.id != panes[src].get_area(),
                                   get_location_key( ordered_loc )[0],
                                   prefix + " " + s.name,
                                   s.veh->name );
            } else {
                menu.addentry( ordered_loc,
                               s.canputitems() && s.id != panes[src].get_area(),
                               get_location_key( ordered_loc )[0],
                               prefix + " " + s.name );
            }
        }
    }
    // Selected keyed to uilist.entries, which starts at 0.
    menu.selected = save_state->last_popup_dest - AIM_SOUTHWEST;
    menu.query();
    if( menu.ret >= AIM_SOUTHWEST && menu.ret <= AIM_NORTHEAST ) {
        cata_assert( squares[menu.ret].canputitems() );
        def = static_cast<aim_location>( menu.ret );
        // we have to set the destination pane so that move actions will target it
        // we can use restore_area later to undo this
        panes[dest].set_area( squares[def], true );
        save_state->last_popup_dest = menu.ret;
        return true;
    }
    return false;
}

bool advanced_inventory::query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                                        const std::string &action, int &amount )
{
    // should be a specific location instead
    cata_assert( destarea != AIM_ALL );
    // valid item is obviously required
    cata_assert( !sitem.items.empty() );
    const item &it = *sitem.items.front();
    const bool by_charges = it.count_by_charges();
    // default to move all, unless if being equipped
    const int input_amount = by_charges ? it.charges : action == "MOVE_SINGLE_ITEM" ? 1 : sitem.stacks;
    // there has to be something to begin with
    cata_assert( input_amount > 0 );
    amount = input_amount;

    // Includes moving from/to inventory and around on the map.
    if( it.made_of_from_type( phase_id::LIQUID ) && !it.is_frozen_liquid() ) {
        popup_getkey( _( "Spilt liquids cannot be picked back up.  Try mopping them up instead." ) );
        return false;
    }
    if( it.made_of_from_type( phase_id::GAS ) ) {
        popup_getkey( _( "Spilt gasses cannot be picked up.  They will disappear over time." ) );
        return false;
    }
    Character &player_character = get_player_character();
    // Check how many items you can stash. extra check because free_volume() doesn't account for pockets the item would not fit .
    if( destarea == AIM_INVENTORY ) {
        int copies_remaining = amount;
        player_character.can_stash_partial( it, copies_remaining, /*ignore_pkt_settings=*/false );
        amount -= copies_remaining;
        if( amount <= 0 ) {
            popup_getkey( _( "No pocket can contain the %s." ), it.tname() );
            return false;
        }
    }
    // Check volume, this should work the same map and vehicles, but not for worn
    else if( destarea != AIM_WIELD && destarea != AIM_WORN ) {
        const units::volume free_volume = panes[dest].free_volume( squares[destarea] );
        const units::mass free_mass = panes[dest].free_weight_capacity();
        const int room_for = std::min( it.charges_per_volume( free_volume ),
                                       it.charges_per_weight( free_mass ) );
        if( room_for <= 0 ) {
            popup_getkey( _( "Destination area is full.  Remove some items first." ) );
            return false;
        }
        amount = std::min( room_for, amount );
    }
    // Map and vehicles have a maximal item count, check that. Inventory does not have this.
    if( destarea != AIM_INVENTORY &&
        destarea != AIM_WORN &&
        destarea != AIM_WIELD &&
        destarea != AIM_CONTAINER
      ) {
        advanced_inv_area &p = squares[destarea];
        const int cntmax = p.max_size - p.get_item_count();
        // For items counted by charges, adding it adds 0 items if something there stacks with it.
        const bool adds0 = by_charges && std::any_of( panes[dest].items.begin(), panes[dest].items.end(),
        [&it]( const advanced_inv_listitem & li ) {
            return li.items.front()->stacks_with( it );
        } );
        if( cntmax <= 0 && !adds0 ) {
            popup_getkey( _( "Destination area has too many items.  Remove some first." ) );
            return false;
        }
        // Items by charge count as a single item, regardless of the charges. As long as the
        // destination can hold another item, one can move all charges.
        if( !by_charges ) {
            amount = std::min( cntmax, amount );
        }
    }

    // Inventory has a weight capacity, map and vehicle don't have that
    if( destarea == AIM_INVENTORY || destarea == AIM_WORN || destarea == AIM_WIELD ) {
        const units::mass unitweight = it.weight() / ( by_charges ? it.charges : 1 );
        const units::mass max_weight = player_character.max_pickup_capacity() -
                                       player_character.weight_carried();
        if( unitweight > 0_gram && unitweight * amount > max_weight ) {
            const int weightmax = max_weight / unitweight;
            if( weightmax <= 0 ) {
                popup_getkey( _( "This is too heavy!" ) );
                return false;
            }
            amount = std::min( weightmax, amount );
        }
    }
    // handle how many of armor type we can equip (max of 2 per type)
    if( destarea == AIM_WORN ) {
        const itype_id &id = sitem.items.front()->typeId();
        // how many slots are available for the item?
        const int slots_available = id->max_worn - player_character.amount_worn( id );
        // base the amount to equip on amount of slots available
        amount = std::min( slots_available, input_amount );
    }
    // Now we have the final amount. Query if requested or limited room left.
    if( action == "MOVE_VARIABLE_ITEM" || amount < input_amount ) {
        const int count = by_charges ? it.charges : sitem.stacks;
        const char *msg = nullptr;
        std::string popupmsg;
        if( amount >= input_amount ) {
            msg = _( "How many do you want to move?  [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, count );
        } else {
            msg = _( "Destination can only hold %d!  Move how many?  [Have %d] (0 to cancel)" );
            popupmsg = string_format( msg, amount, count );
        }
        // At this point amount contains the maximal amount that the destination can hold.
        const int possible_max = std::min( input_amount, amount );
        if( amount <= 0 ) {
            popup_getkey( _( "The destination is already full." ) );
        } else {
            // In test_mode always use max possible
            // TODO: maybe a way to provide a custom amount?
            test_mode ? amount = possible_max : query_int( amount, false, popupmsg );
        }
        if( amount <= 0 ) {
            return false;
        }
        if( amount > possible_max ) {
            amount = possible_max;
        }
    }
    return true;
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
    wnoutrefresh( mm_border );
    wnoutrefresh( minimap );
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
    Character &player_character = get_player_character();
    // draw the 3x3 tiles centered around player
    get_map().draw( minimap, player_character.pos_bub() );
    for( const side s : sides ) {
        char sym = get_minimap_sym( s );
        if( sym == '\0' ) {
            continue;
        }
        advanced_inv_area sq = squares[panes[s].get_area()];
        tripoint pt = pc + sq.off.raw();
        // invert the color if pointing to the player's position
        nc_color cl = sq.id == AIM_INVENTORY || sq.id == AIM_WORN ?
                      invert_color( c_light_cyan ) : c_light_cyan.blink();
        mvwputch( minimap, pt.xy(), cl, sym );
    }

    // Invert player's tile color if exactly one pane points to player's tile
    bool invert_left = false;
    bool invert_right = false;
    const auto is_selected = [ this ]( const aim_location & where, size_t side ) {
        return where == this->panes[ side ].get_area();
    };
    for( const aim_location &loc : player_locations ) {
        invert_left |= is_selected( loc, 0 );
        invert_right |= is_selected( loc, 1 );
    }

    if( !invert_left || !invert_right ) {
        player_character.draw( minimap, player_character.pos_bub(), invert_left || invert_right );
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
        case '@':
            // '^' or 'v'
            ch = d_side[panes[-p + 1].get_area() == AIM_CENTER];
            break;
        case '#':
            // 'L' or 'R'
            ch = panes[p].in_vehicle() ? 'V' : c_side[p];
            break;
        case '^':
            // do not show anything
            ch ^= ch;
            break;
    }
    return ch;
}

void advanced_inventory::swap_panes()
{
    // Switch left and right pane.
    std::swap( panes[left], panes[right] );
    // Switch save states
    std::swap( panes[left].save_state, panes[right].save_state );
    // Switch currently selected item
    std::swap( panes[left].target_item_after_recalc, panes[right].target_item_after_recalc );
    // Window pointer must be unchanged!
    std::swap( panes[left].window, panes[right].window );
    // Recalculation required for weight & volume
    recalc = true;
}

void advanced_inventory::do_return_entry()
{
    // only save pane settings
    save_settings( true );
    uistate.open_menu = create_advanced_inv;
    save_state->exit_code = aim_exit::re_entry;
}

void advanced_inventory::temp_hide()
{
    ui.reset();
    do_return_entry();
    cancel_aim_processing();
}

bool advanced_inventory::is_processing() const
{
    return save_state->re_enter_move_all != aim_entry::START;
}

void cancel_aim_processing()
{
    uistate.transfer_save.re_enter_move_all = aim_entry::START;
    uistate.transfer_save.aim_all_location = AIM_AROUND_BEGIN;
    uistate.transfer_save.exit_code = aim_exit::none;
}
