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
#include "comestible_inv_pane.h"

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

struct col_data {
    comestible_inv_columns id;
    std::string col_name;
    int width;

    char hotkey;
    std::string sort_name;

    col_data( comestible_inv_columns id, std::string col_name, int width, char hotkey,
              std::string sort_name ) :
        id( id ),
        col_name( col_name ), width( width ), hotkey( hotkey ), sort_name( sort_name ) {

    }
};

// *INDENT-OFF*
static const col_data get_col_data(comestible_inv_columns col) {
    static std::array<col_data, COLUMN_NUM_ENTRIES> col_data = { {
        {COLUMN_NAME,       _("Name (charges)"),0, 'n', _("name")},

        {COLUMN_SRC,        _("src"),         4, ';', _("")},
        {COLUMN_AMOUNT,     _("amt"),         5, ';', _("")},

        {COLUMN_WEIGHT,     _("weight"),      7, 'w', _("weight")},
        {COLUMN_VOLUME,     _("vol"),         8, 'v', _("volume")},

        {COLUMN_CALORIES,   _("calories"),    9, 'c', _("calories")},
        {COLUMN_QUENCH,     _("quench"),      7, 'q', _("quench")},
        {COLUMN_JOY,        _("joy"),         4, 'j', _("joy")},
        {COLUMN_EXPIRES,    _("expires in"),  12,'s', _("spoilage")},
        {COLUMN_SHELF_LIFE, _("shelf life"),  12,'s', _("shelf life")},

        {COLUMN_ENERGY,     _("energy"),      12,'e', _("energy")},

        {COLUMN_SORTBY_CATEGORY,_("fake"),    0,'c', _("category")}
    } };
    return col_data[col];
}
// *INDENT-ON*

void comestible_inventory_pane::init( std::vector<comestible_inv_columns> c, int items_per_page,
                                      catacurses::window w, std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS>
                                      *s )
{
    columns = c;
    itemsPerPage = items_per_page;
    window = w;
    squares = s;
    is_compact = TERMX <= 100;

    inCategoryMode = false;

    comestible_inv_area_info::aim_location location =
        static_cast<comestible_inv_area_info::aim_location>( uistate.comestible_save.area_idx );
    set_area( &( *squares )[location], uistate.comestible_save.in_vehicle );

    index = uistate.comestible_save.selected_idx;
    filter = uistate.comestible_save.filter;

    std::vector<comestible_inv_columns> default_columns = { COLUMN_AMOUNT, COLUMN_WEIGHT, COLUMN_VOLUME };
    if( !is_compact ) {
        default_columns.emplace( default_columns.begin(), COLUMN_SRC );
    }
    columns.insert( columns.begin(), default_columns.begin(), default_columns.end() );
    if( std::find( columns.begin(), columns.end(),
                   uistate.comestible_save.sort_idx ) != columns.end() ) {
        sortby = static_cast<comestible_inv_columns>( uistate.comestible_save.sort_idx );
    } else {
        sortby = COLUMN_NAME;
    }
}

void comestible_inventory_pane::save_settings( bool reset )
{
    if( reset ) {
        uistate.comestible_save.in_vehicle = false;
        uistate.comestible_save.area_idx = comestible_inv_area_info::AIM_ALL_I_W;
        uistate.comestible_save.selected_idx = 0;
        uistate.comestible_save.selected_itm = nullptr;
        uistate.comestible_save.filter = "";
    } else {
        uistate.comestible_save.in_vehicle = viewing_cargo;
        uistate.comestible_save.area_idx = cur_area->info.id;
        uistate.comestible_save.selected_idx = index;
        if( get_cur_item_ptr() != nullptr ) {
            uistate.comestible_save.selected_itm = get_cur_item_ptr()->items.front();
        } else {
            uistate.comestible_save.selected_itm = nullptr;
        }
        uistate.comestible_save.filter = filter;
    }
    uistate.comestible_save.sort_idx = sortby;
}

void comestible_inventory_pane::add_sort_enries( uilist &sm )
{
    col_data c = get_col_data( COLUMN_NAME );
    sm.addentry( c.id, true, c.hotkey, c.sort_name );
    if( sortby == COLUMN_NAME ) {
        sm.selected = 0;
    }

    for( size_t i = 0; i < columns.size(); i++ ) {
        c = get_col_data( columns[i] );
        if( c.id == COLUMN_SRC || c.id == COLUMN_AMOUNT ) {
            continue;
        }

        sm.addentry( c.id, true, c.hotkey, c.sort_name );
        if( sortby == c.id ) {
            sm.selected = i;
        }
    }
}

void comestible_inventory_pane::do_filter( int h, int w )
{
    string_input_popup spopup;
    std::string old_filter = filter;
    filter_edit = true;
    spopup.window( window, 4, h, w )
    .max_length( 256 )
    .text( filter );

#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
        SDL_StartTextInput();
    }
#endif

    do {
        mvwprintz( window, point( 2, getmaxy( window ) - 1 ), c_cyan, "< " );
        mvwprintz( window, point( w, getmaxy( window ) - 1 ), c_cyan, " >" );
        std::string new_filter = spopup.query_string( false );
        if( spopup.context().get_raw_input().get_first_input() == KEY_ESCAPE ) {
            set_filter( old_filter );
        } else {
            set_filter( new_filter );
        }
        redraw();
    } while( spopup.context().get_raw_input().get_first_input() != '\n' &&
             spopup.context().get_raw_input().get_first_input() != KEY_ESCAPE );
    filter_edit = false;
    needs_redraw = true;
}
void comestible_inventory_pane::print_items()
{
    const int page = index / itemsPerPage;

    int max_width = getmaxx( window ) - 1;
    std::string spaces( max_width - 3, ' ' );
    nc_color norm = c_white;

    //print inventory's current and total weight + volume
    if( cur_area->info.type == comestible_inv_area_info::AREA_TYPE_PLAYER ) {
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
        const int hrightcol = max_width - formatted_head.length();
        nc_color color = weight_carried > weight_capacity ? c_red : c_light_green;
        mvwprintz( window, point( hrightcol, 4 ), color, "%.1f", weight_carried );
        wprintz( window, c_light_gray, "/%.1f %s  ", weight_capacity, weight_units() );

        color = g->u.volume_carried().value() > g->u.volume_capacity().value() ? c_red :
                c_light_green;
        wprintz( window, color, volume_carried );
        wprintz( window, c_light_gray, "/%s %s", volume_capacity, volume_units_abbr() );
    } else { //print square's current and total weight + volume
        std::string formatted_head;
        std::string volume_str;

        if( cur_area->info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
            volume_str = string_format( "%s", format_volume( volume ) );
        } else {
            units::volume maxvolume = cur_area->get_max_volume( viewing_cargo );
            volume_str = string_format( "%s/%s", format_volume( volume ), format_volume( maxvolume ) );
        }
        formatted_head = string_format( "%3.1f %s  %s %s",
                                        convert_weight( weight ),
                                        weight_units(),
                                        volume_str,
                                        volume_units_abbr() );
        mvwprintz( window, point( max_width - formatted_head.length(), 4 ), norm, formatted_head );
    }

    const size_t name_startpos = is_compact ? 1 : 4;

    mvwprintz( window, point( name_startpos, 5 ), c_light_gray, get_col_data( COLUMN_NAME ).col_name );
    int cur_x = max_width;
    for( size_t i = columns.size(); i -- > 0; ) {
        const col_data d = get_col_data( columns[i] );
        if( d.width <= 0 ) {
            continue;
        }
        cur_x -= d.width;
        mvwprintz( window, point( cur_x, 5 ), c_light_gray, "%*s", d.width, d.col_name );
    }

    for( int i = page * itemsPerPage, cur_print_y = 6; i < static_cast<int>( items.size() ) &&
         cur_print_y < itemsPerPage + 6; i++, cur_print_y++ ) {
        comestible_inv_listitem &sitem = items[i];
        if( sitem.is_category_header() ) {
            mvwprintz( window, point( ( max_width - utf8_width( sitem.name ) - 6 ) / 2, cur_print_y ), c_cyan,
                       "[%s]",
                       sitem.name );
            continue;
        }
        if( !sitem.is_item_entry() ) {
            // Empty entry at the bottom of a page.
            continue;
        }

        const bool selected = index == i;


        comestible_select_state selected_state;
        if( selected ) {
            selected_state = inCategoryMode ? SELECTSTATE_CATEGORY : SELECTSTATE_SELECTED;
        } else {
            selected_state = SELECTSTATE_NONE;
        }
        sitem.print_columns( columns, selected_state, is_compact, window, max_width, cur_print_y );
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

void comestible_inventory_pane::scroll_by( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    if( items.empty() ) {
        return;
    }
    if( inCategoryMode ) {
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
    needs_redraw = true;
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
    needs_recalc = true;
}

bool comestible_inventory_pane::is_filtered( const comestible_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool comestible_inventory_pane::is_filtered( const item &it ) const
{
    if( special_filter( it ) ) {
        return true;
    }

    if( filter.empty() ) {
        return false;
    }

    const std::string str = it.tname();
    if( filtercache.find( str ) == filtercache.end() ) {
        const auto filter_fn = item_filter_from_string( filter );
        filtercache[str] = filter_fn;

        return !filter_fn( it );
    }

    return !filtercache[str]( it );
}

void comestible_inventory_pane::paginate()
{
    if( sortby != COLUMN_SORTBY_CATEGORY ) {
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

struct sort_case_insensitive_less : public std::binary_function< char, char, bool > {
    bool operator()( char x, char y ) const {
        return toupper( static_cast<unsigned char>( x ) ) < toupper( static_cast<unsigned char>( y ) );
    }
};

struct comestible_inv_sorter {
    comestible_inv_columns sortby;
    comestible_inv_columns default_sortby;
    comestible_inv_sorter( comestible_inv_columns sort, comestible_inv_columns default_sort ) {
        sortby = sort;
        default_sortby = default_sort;
    }
    bool operator()( const comestible_inv_listitem &d1, const comestible_inv_listitem &d2 ) {
        return compare( sortby, d1, d2 );
    }

    bool compare( comestible_inv_columns compare_by, const comestible_inv_listitem &d1,
                  const comestible_inv_listitem &d2 ) {
        switch( compare_by ) {
            case COLUMN_NAME:
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
            case COLUMN_WEIGHT:
                if( d1.weight != d2.weight ) {
                    return d1.weight > d2.weight;
                }
                break;
            case COLUMN_VOLUME:
                if( d1.volume != d2.volume ) {
                    return d1.volume > d2.volume;
                }
                break;
            case COLUMN_CALORIES:
                if( d1.calories != d2.calories ) {
                    return d1.calories > d2.calories;
                }
                break;
            case COLUMN_QUENCH:
                if( d1.quench != d2.quench ) {
                    return d1.quench > d2.quench;
                }
                break;
            case COLUMN_JOY:
                if( d1.joy != d2.joy ) {
                    return d1.joy > d2.joy;
                }
                break;
            case COLUMN_ENERGY:
                if( d1.energy != d2.energy ) {
                    return d1.energy > d2.energy;
                }
                break;
            case COLUMN_EXPIRES:
            case COLUMN_SHELF_LIFE:
                if( d1.items.front()->spoilage_sort_order() != d2.items.front()->spoilage_sort_order() ) {
                    return d1.items.front()->spoilage_sort_order() < d2.items.front()->spoilage_sort_order();
                }
                break;
            case COLUMN_SORTBY_CATEGORY:
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
            case COLUMN_SRC:
            case COLUMN_AMOUNT:
            case COLUMN_NUM_ENTRIES:
                //shouldn't be here
                assert( false );
                break;
        }

        if( compare_by == default_sortby ) {
            return false;
        } else {
            return compare( default_sortby, d1, d2 );
        }
    }
};

void comestible_inventory_pane::recalc()
{
    needs_recalc = false;
    items.clear();
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.

    units::volume tmp_vol;
    units::mass tmp_weight;
    if( cur_area->info.type == comestible_inv_area_info::AREA_TYPE_MULTI ) {
        volume = 0_ml;
        weight = 0_gram;
        std::vector<comestible_inv_area_info::aim_location> loc = cur_area->info.multi_locations;
        for( size_t i = 0; i < loc.size(); i++ ) {
            comestible_inv_area *s = &( ( *squares )[loc[i]] );
            // Deal with squares with ground + vehicle storage
            // Also handle the case when the other tile covers vehicle
            // or the ground below the vehicle.
            if( s->has_vehicle() ) {
                add_items_from_area( s, true, tmp_vol, tmp_weight );
                volume += tmp_vol;
                weight += tmp_weight;
            }

            // Add map items
            add_items_from_area( s, false, tmp_vol, tmp_weight );
            volume += tmp_vol;
            weight += tmp_weight;
        }
    } else {
        add_items_from_area( cur_area, viewing_cargo, volume, weight );
    }

    // Insert category headers (only expected when sorting by category)
    if( sortby == COLUMN_SORTBY_CATEGORY ) {
        std::set<const item_category *> categories;
        for( auto &it : items ) {
            categories.insert( it.cat );
        }
        for( auto &cat : categories ) {
            items.push_back( comestible_inv_listitem( cat ) );
        }
    }


    // Finally sort all items (category headers will now be moved to their proper position)
    std::stable_sort( items.begin(), items.end(), comestible_inv_sorter( sortby, default_sortby ) );
    paginate();

    int i = 0;
    for( auto &it : items ) {
        if( it.items.front() == uistate.comestible_save.selected_itm ) {
            uistate.comestible_save.selected_idx = i;
            index = i;
            break;
        }
        i++;
    }
}

//TODO: I want to move functionality to comestible_inv_area, but I dunno how to convert AIM_INVENTORY, AIM_WORN and other to the same type.
void comestible_inventory_pane::add_items_from_area( comestible_inv_area *area, bool from_cargo,
        units::volume &ret_volume, units::mass &ret_weight )
{
    assert( area->info.type != comestible_inv_area_info::AREA_TYPE_MULTI );
    if( !area->canputitems() ) {
        return;
    }
    player &u = g->u;

    ret_volume = 0_ml;
    ret_weight = 0_gram;

    units::volume tmp_volume;
    units::mass tmp_weight;
    // Existing items are *not* cleared on purpose, this might be called
    // several times in case all surrounding squares are to be shown.
    if( area->info.id == comestible_inv_area_info::AIM_INVENTORY ) {
        const invslice &stacks = u.inv.slice();
        for( size_t x = 0; x < stacks.size(); ++x ) {
            std::list<item *> item_pointers;
            for( item &i : *stacks[x] ) {
                item_pointers.push_back( &i );
            }
            comestible_inv_listitem it( item_pointers, x, area, false );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            ret_volume += it.volume;
            ret_weight += it.weight;
            items.push_back( it );
        }
    } else if( area->info.id == comestible_inv_area_info::AIM_WORN ) {
        auto iter = u.worn.begin();
        for( size_t i = 0; i < u.worn.size(); ++i, ++iter ) {
            comestible_inv_listitem it( &*iter, i, 1, area, false );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            ret_volume += it.volume;
            ret_weight += it.weight;
            items.push_back( it );
        }
    } else {
        const comestible_inv_area::area_items stacks = area->get_items( from_cargo );

        for( size_t x = 0; x < stacks.size(); ++x ) {
            comestible_inv_listitem it( stacks[x], x, area, from_cargo );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            ret_volume += it.volume;
            ret_weight += it.weight;
            items.push_back( it );
        }
    }
}

void comestible_inventory_pane::redraw()
{
    // don't update ui if processing demands
    if( needs_recalc ) {
        recalc();
    } else if( !needs_redraw ) {
        return;
    }
    needs_redraw = false;
    fix_index();

    auto w = window;

    werase( w );

    print_items();

    auto itm = get_cur_item_ptr();
    int width = print_header( itm != nullptr ? itm->area : get_area() );
    bool same_as_dragged = ( cur_area->info.id >= comestible_inv_area_info::AIM_SOUTHWEST &&
                             cur_area->info.id <= comestible_inv_area_info::AIM_NORTHEAST ) &&
                           // only cardinals
                           cur_area->info.id != comestible_inv_area_info::AIM_CENTER && is_in_vehicle() &&
                           // not where you stand, and pane is in vehicle
                           cur_area->info.default_offset == get_square(
                               comestible_inv_area_info::AIM_DRAGGED )->info.default_offset; // make sure the offsets are the same as the grab point
    const comestible_inv_area *sq = same_as_dragged ? get_square(
                                        comestible_inv_area_info::AIM_DRAGGED ) : cur_area;


    bool car = cur_area->has_vehicle() && is_in_vehicle() &&
               sq->info.id != comestible_inv_area_info::AIM_DRAGGED;
    auto name = utf8_truncate( sq->get_name( viewing_cargo ), width );
    auto desc = utf8_truncate( sq->desc[car], width );
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, point( 2, 1 ), c_green, name );
    mvwprintz( w, point( 2, 2 ), c_light_blue, desc );
    trim_and_print( w, point( 2, 3 ), width, c_cyan, cur_area->flags );

    if( !is_compact ) {
        for( size_t i = 0; i < legend.size(); i++ ) {
            legend_data l = legend[i];
            mvwprintz( window, point( 25, i + 1 ), c_light_gray, "[%c] ", l.shortcut );
            wprintz( window, l.color, "%s", l.message );
        }
    }

    const int max_page = ( items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( max_page > 1 ) {
        const int page = index / itemsPerPage;
        mvwprintz( w, point( 2, 4 ), c_light_blue, _( "[<] page %d of %d [>]" ), page + 1, max_page );
    }

    wattron( w, c_cyan );
    // draw a darker border around the inactive pane
    draw_border( w, BORDER_COLOR );
    mvwprintw( w, point( 3, 0 ), _( "< [s]ort: %s >" ), get_col_data( sortby ).sort_name );

    if( title.size() != 0 ) {
        std::string title_string = string_format( "<< %s >>", title );
        mvwprintz( w, point( ( getmaxx( w ) - title_string.size() ) / 2, 0 ), c_light_cyan, title_string );
    }

    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );

    //TODO:XXX
    //bool filter_edit = true;
    if( !filter_edit ) {
        if( !filter.empty() ) {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s: %s >", fprefix, filter );
        } else {
            mvwprintw( w, point( 2, getmaxy( w ) - 1 ), "< %s >", fprefix );
        }
    }

    wattroff( w, c_white );

    if( !filter_edit && !filter.empty() ) {
        mvwprintz( w, point( 6 + std::strlen( fprefix ), getmaxy( w ) - 1 ), c_white,
                   filter );
        mvwprintz( w, point( getmaxx( w ) - std::strlen( fsuffix ) - 2, getmaxy( w ) - 1 ), c_white, "%s",
                   fsuffix );
    }
    wrefresh( w );
}



int comestible_inventory_pane::print_header( comestible_inv_area *sel_area )
{
    int wwidth = getmaxx( window );
    int ofs = wwidth - 25 - 2 - 14;
    for( size_t i = 0; i < squares->size(); ++i ) {
        std::string key = comestible_inv_area::get_info(
                              static_cast<comestible_inv_area_info::aim_location>( i ) ).minimapname;

        comestible_inv_area *iter_square = get_square( i );

        const char *bracket = iter_square->has_vehicle() ? "<>" : "[]";

        bool is_single = cur_area->info.type != comestible_inv_area_info::AREA_TYPE_MULTI;
        bool is_selected_by_user = cur_area->info.has_area( get_square( i )->info.id );
        bool is_selected_by_item;
        if( is_single ) {
            is_selected_by_item = is_selected_by_user;
        } else {
            is_selected_by_item = get_square( i )->info.id == sel_area->info.id;
        }

        //bool is_selected = get_square(i)->info.has_area(sel_area->info.id);

        //bool q = (sel_area->info.id == get_square(i)->info.id);



        //bool iterIsMe = (get_square(i).info.id == area);
        //bool selectIsMe = (sel == area);

        //bool in_vehicle = is_in_vehicle() && get_square(i).info.id == area && sel == area && area != comestible_inv_area_info::AIM_ALL;
        //bool all_brackets = area == comestible_inv_area_info::AIM_ALL && (i >= comestible_inv_area_info::AIM_SOUTHWEST && i <= comestible_inv_area_info::AIM_NORTHEAST);
        nc_color bcolor = c_red;
        nc_color kcolor = c_red;
        if( get_square( i )->canputitems() ) {
            if( is_single ) {
                if( is_selected_by_item ) {
                    if( viewing_cargo ) {
                        bcolor = c_light_blue;
                    } else {
                        bcolor = c_light_gray;
                    }
                    kcolor = c_white;
                } else {
                    bcolor = kcolor = c_dark_gray;
                }
            } else {
                if( cur_area->info.id == get_square( i )->info.id ) {
                    bcolor = c_light_gray;
                    kcolor = c_white;
                } else {
                    if( is_selected_by_user ) {
                        bcolor = c_light_gray;
                    } else {
                        bcolor = c_dark_gray;
                    }

                    if( is_selected_by_item ) {
                        kcolor = c_light_gray;
                    } else {
                        kcolor = c_dark_gray;
                    }
                }
            }

            /* if (is_selected) {
                 if (is_single) {
                     if (viewing_cargo) {
                         bcolor = c_light_gray;
                     }
                     kcolor = c_white;
                 }
                 else {
                     bcolor = c_light_gray;
                     kcolor = c_light_gray;
                 }
             }
             else {
                 bcolor = kcolor = c_dark_gray;
             }*/
            //bcolor = in_vehicle ? c_light_blue :
            //    area == i || all_brackets ? c_light_gray : c_dark_gray;
            //kcolor = area == i ? c_white : sel == i ? c_light_gray : c_dark_gray;
        }

        const int x = get_square( i )->info.hscreen.x + ofs;
        const int y = get_square( i )->info.hscreen.y;
        mvwprintz( window, point( x, y ), bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%s", viewing_cargo && is_selected_by_item && is_single &&
                 cur_area->info.id != comestible_inv_area_info::AIM_DRAGGED ? "V" : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }
    return get_square( comestible_inv_area_info::AIM_INVENTORY )->info.hscreen.y + ofs;
}

void comestible_inv_listitem::print_columns( std::vector<comestible_inv_columns> columns,
        comestible_select_state selected_state, bool is_compact, catacurses::window window, int right_bound,
        int cur_print_y )
{

    is_selected = selected_state;
    int w;
    for( std::vector<comestible_inv_columns>::reverse_iterator col_iter = columns.rbegin();
         col_iter != columns.rend(); ++col_iter ) {
        w = get_col_data( *col_iter ).width;
        if( w <= 0 ) {
            continue;
        }
        right_bound -= w;
        if( print_string_column( *col_iter, window, right_bound, cur_print_y ) ) {
            continue;
        }

        if( print_int_column( *col_iter, window, right_bound, cur_print_y ) ) {
            continue;
        }

        if( print_default_columns( *col_iter, window, right_bound, cur_print_y ) ) {
            continue;
        }

        //TODO: error
    }
    print_name( is_compact, window, right_bound, cur_print_y );
}

bool comestible_inv_listitem::print_string_column( comestible_inv_columns col,
        catacurses::window window, int cur_print_x, int cur_print_y )
{
    std::string print_string;
    switch( col ) {
        case COLUMN_SHELF_LIFE:
            print_string = shelf_life;
            break;
        case COLUMN_EXPIRES:
            print_string = exipres_in;
            break;
        default:
            return false;
    }
    nc_color print_color;
    set_print_color( print_color, c_cyan );
    mvwprintz( window, point( cur_print_x, cur_print_y ), print_color, "%*s", get_col_data( col ).width,
               print_string );
    return true;
}

bool comestible_inv_listitem::print_int_column( comestible_inv_columns col,
        catacurses::window window, int cur_print_x, int cur_print_y )
{
    int print_val;
    bool need_highlight = false;
    switch( col ) {
        case COLUMN_CALORIES:
            print_val = calories;
            break;
        case COLUMN_QUENCH:
            print_val = quench;
            break;
        case COLUMN_JOY:
            print_val = joy;
            need_highlight = is_mushy;
            break;
        case COLUMN_ENERGY:
            print_val = energy;
            break;
        default:
            return false;
            break;
    }
    nc_color print_color;
    char const *print_format = set_string_params( print_color, print_val, need_highlight );
    std::string s = string_format( print_format, print_val );
    mvwprintz( window, point( cur_print_x, cur_print_y ), print_color, "%*s", get_col_data( col ).width,
               s );
    return true;
}

char const *comestible_inv_listitem::set_string_params( nc_color &print_color, int value,
        bool need_highlight )
{
    char const *string_format;
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

    set_print_color( print_color, print_color );
    return string_format;
}

void comestible_inv_listitem::set_print_color( nc_color &retval, nc_color default_col )
{
    switch( is_selected ) {
        case SELECTSTATE_NONE:
            retval = default_col;
            break;
        case SELECTSTATE_SELECTED:
            retval = hilite( c_white );
            break;
        case SELECTSTATE_CATEGORY:
            retval = c_white_red;
            break;
        default:
            break;
    }
}

bool comestible_inv_listitem::print_default_columns( comestible_inv_columns col,
        catacurses::window window, int cur_print_x, int cur_print_y )
{
    nc_color print_color;
    std::string s;

    switch( col ) {
        case COLUMN_VOLUME: {
            bool it_vol_truncated = false;
            double it_vol_value = 0.0;
            s = format_volume( volume, 5, &it_vol_truncated, &it_vol_value );
            if( it_vol_truncated && it_vol_value > 0.0 ) {
                print_color = c_red;
            } else {
                print_color = volume.value() > 0 ? menu_color : menu_color_dark;
            }
            break;
        }

        case COLUMN_WEIGHT: {
            double it_weight = convert_weight( weight );
            size_t w_precision;
            print_color = it_weight > 0 ? menu_color : menu_color_dark;

            if( it_weight >= 1000.0 ) {
                if( it_weight >= 10000.0 ) {
                    print_color = c_red;
                    it_weight = 9999.0;
                }
                w_precision = 0;
            } else if( it_weight >= 100.0 ) {
                w_precision = 1;
            } else {
                w_precision = 2;
            }
            s = string_format( "%5.*f", w_precision, it_weight );
            break;
        }
        case COLUMN_AMOUNT: {
            if( stacks > 1 ) {
                print_color = menu_color;
                if( stacks > 9999 ) {
                    stacks = 9999;
                    print_color = c_red;
                }
                s = string_format( "%4d", stacks );
            } else {
                s = "";
            }
            break;
        }
        case COLUMN_SRC:
            print_color = menu_color;
            s = area->info.shortname;
            break;

        default:
            return false;
            break;
    }
    set_print_color( print_color, print_color );
    mvwprintz( window, point( cur_print_x, cur_print_y ), print_color, "%*s", get_col_data( col ).width,
               s );
    return true;
}

void comestible_inv_listitem::print_name( bool is_compact, catacurses::window window,
        int right_bound, int cur_print_y )
{
    std::string item_name;

    item *it = items.front();
    if( it->is_money() ) {
        //Count charges
        // TODO: transition to the item_location system used for the normal inventory
        unsigned int charges_total = 0;
        for( const auto item : items ) {
            charges_total += item->charges;
        }
        item_name = it->display_money( items.size(), charges_total );
    } else {
        item_name = it->display_name();
    }

    //check stolen
    if( it->has_owner() ) {
        const faction *item_fac = it->get_owner();
        if( item_fac != g->faction_manager_ptr->get( faction_id( "your_followers" ) ) ) {
            item_name = string_format( "%s %s", "<color_light_red>!</color>", item_name );
        }
    }
    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
        item_name = string_format( "%s %s", it->symbol(), item_name );
    }

    int name_startpos = is_compact ? 1 : 4;
    int max_name_length = right_bound - name_startpos - 1;
    nc_color print_color;
    set_print_color( print_color, menu_color );

    if( is_selected ) { //fill whole line with color
        mvwprintz( window, point( 0, cur_print_y ), print_color, "%*s", right_bound, "" );
    }
    trim_and_print( window, point( name_startpos, cur_print_y ), max_name_length, print_color,
                    item_name );

    if( autopickup ) {
        mvwprintz( window, point( name_startpos, cur_print_y ), magenta_background( print_color ),
                   is_compact ? item_name.substr( 0, 1 ) : ">" );
    }
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


std::string comestible_inv_listitem::get_time_left_rounded( player &p, const item &it ) const
{
    if( it.is_going_bad() ) {
        return _( "soon!" );
    }

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

std::string comestible_inv_listitem::time_to_comestible_str( const time_duration &d ) const
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

comestible_inv_listitem::comestible_inv_listitem( item *an_item, int index, int count,
        comestible_inv_area *area, bool from_vehicle )
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


    menu_color = it.color_in_inventory();

    exipres_in = "";
    shelf_life = "";
    if( it_c.spoils > 0_turns ) {
        if( !it.rotten() ) {
            exipres_in = get_time_left_rounded( p, it );
        }
        shelf_life = to_string_clipped( it_c.spoils );
    }

    calories = p.kcal_for( it );
    quench = it_c.quench;
    joy = p.fun_for( it ).first;
    is_mushy = it.has_flag( "MUSHY" );

    energy = p.get_acquirable_energy( it );

    items.push_back( an_item );

    menu_color_dark = c_dark_gray;
    assert( stacks >= 1 );
}

comestible_inv_listitem::comestible_inv_listitem( const std::list<item *> &list, int index,
        comestible_inv_area *area, bool from_vehicle ) :
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



    menu_color = it.color_in_inventory();

    exipres_in = "";
    shelf_life = "";
    if( it_c.spoils > 0_turns ) {
        if( !it.rotten() ) {
            exipres_in = get_time_left_rounded( p, it );
        }
        shelf_life = to_string_clipped( it_c.spoils );
    }

    calories = p.kcal_for( it );
    quench = it_c.quench;
    joy = p.fun_for( it ).first;
    is_mushy = it.has_flag( "MUSHY" );

    energy = p.get_acquirable_energy( it );

    menu_color_dark = c_dark_gray;
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
    menu_color_dark = c_dark_gray;
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
    menu_color_dark = c_dark_gray;
}

bool comestible_inv_listitem::is_category_header() const
{
    return items.empty() && cat != nullptr;
}

bool comestible_inv_listitem::is_item_entry() const
{
    return !items.empty();
}
