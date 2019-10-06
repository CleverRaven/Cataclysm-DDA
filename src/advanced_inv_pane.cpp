#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "input.h"
#include "item_category.h"
#include "item_search.h"
#include "item_stack.h"
#include "output.h"
#include "player.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "uistate.h"
#include "calendar.h"
#include "color.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "ret_val.h"
#include "type_id.h"
#include "clzones.h"
#include "enums.h"
#include "bionics.h"
#include "advanced_inv_pane.h"
#include "material.h"
#include "ime.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <numeric>
#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif


void advanced_inv_pane::init( int items_per_page, catacurses::window w )
{
    itemsPerPage = items_per_page;
    window = w;
    is_compact = TERMX <= 100;

    inCategoryMode = false;

    advanced_inv_area_info::aim_location location =
        static_cast<advanced_inv_area_info::aim_location>( save_state.area_idx );
    set_area( all_areas[location], save_state.in_vehicle );

    index = save_state.selected_idx;
    filter = save_state.filter;

    std::vector<advanced_inv_columns> default_columns = { COLUMN_AMOUNT, COLUMN_WEIGHT, COLUMN_VOLUME, COLUMN_SORTBY_NONE };
    if( !is_compact ) {
        default_columns.emplace( default_columns.begin(), COLUMN_SRC );
    }
    columns.insert( columns.begin(), default_columns.begin(), default_columns.end() );
    if( std::find( columns.begin(), columns.end(),
                   save_state.sort_idx ) != columns.end() ) {
        sortby = static_cast<advanced_inv_columns>( save_state.sort_idx );
    } else {
        sortby = default_sortby;
    }

    if( special_filter == nullptr ) { //if not set by child
        special_filter = []( const item & ) {
            return false;
        };
    }
}

advanced_inv_pane::~advanced_inv_pane()
{
    clear_items();
}

void advanced_inv_pane::clear_items()
{
    for( auto &i : items ) {
        delete i;
    }
    items.clear();
}

void advanced_inv_pane::save_settings( bool reset )
{
    if( reset ) {
        save_state.in_vehicle = false;
        save_state.area_idx = advanced_inv_area_info::AIM_ALL_I_W;
        save_state.selected_idx = 0;
        save_state.selected_itm = nullptr;
        save_state.filter = "";
    } else {
        save_state.in_vehicle = viewing_cargo;
        save_state.area_idx = cur_area->info.id;
        save_state.selected_idx = index;
        if( get_cur_item_ptr() != nullptr ) {
            save_state.selected_itm = get_cur_item_ptr()->items.front();
        } else {
            save_state.selected_itm = nullptr;
        }
        save_state.filter = filter;
    }
    save_state.sort_idx = sortby;
}

void advanced_inv_pane::add_sort_entries( uilist &sm )
{
    col_data c = advanced_inv_listitem::get_col_data( COLUMN_NAME );
    sm.addentry( c.id, true, c.hotkey, c.get_sort_name() );
    if( sortby == COLUMN_NAME ) {
        sm.selected = 0;
    }

    for( size_t i = 0, selected_idx = 1; i < columns.size(); i++ ) {
        c = advanced_inv_listitem::get_col_data( columns[i] );
        if( c.get_sort_name().empty() ) {
            continue;
        }

        sm.addentry( c.id, true, c.hotkey, c.get_sort_name() );
        if( sortby == c.id ) {
            sm.selected = selected_idx;
        }
        ++selected_idx;
    }
}

void advanced_inv_pane::start_user_filtering( int h, int w )
{
    string_input_popup spopup;
    std::string old_filter = filter;
    filter_edit = true;
    spopup.window( window, 4, h, w )
    .max_length( 256 )
    .text( filter );


    ime_sentry sentry;

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
void advanced_inv_pane::print_items()
{
    const int page = index / itemsPerPage;

    int max_width = getmaxx( window ) - 1;
    std::string spaces( max_width - 3, ' ' );
    nc_color norm = c_white;

    //print inventory's current and total weight + volume
    if( cur_area->info.type == advanced_inv_area_info::AREA_TYPE_PLAYER ) {
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

        if( cur_area->info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
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

    const size_t name_startpos = 3;//is_compact ? 1 : 4;

    mvwprintz( window, point( name_startpos, 5 ), c_light_gray,
               advanced_inv_listitem::get_col_data( COLUMN_NAME ).get_col_name() );
    int cur_x = max_width;
    for( size_t i = columns.size(); i -- > 0; ) {
        const col_data d = advanced_inv_listitem::get_col_data( columns[i] );
        if( d.width <= 0 ) {
            continue;
        }
        cur_x -= d.width;
        mvwprintz( window, point( cur_x, 5 ), c_light_gray, "%*s", d.width, d.get_col_name() );
    }

    for( int i = page * itemsPerPage, cur_print_y = 6; i < static_cast<int>( items.size() ) &&
         cur_print_y < itemsPerPage + 6; i++, cur_print_y++ ) {
        advanced_inv_listitem &sitem = *items[i];
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

        sitem.print_columns( columns, item_selected_state( index == i ), window, max_width, cur_print_y );
    }
}

advanced_inv_select_state advanced_inv_pane::item_selected_state( bool selected )
{
    if( selected ) {
        return inCategoryMode ? SELECTSTATE_CATEGORY : SELECTSTATE_SELECTED;
    } else {
        return SELECTSTATE_NONE;
    }
}

void advanced_inv_pane::skip_category_headers( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    assert( static_cast<size_t>( index ) < items.size() ); // valid index is required
    assert( offset == -1 || offset == +1 ); // only those two offsets are allowed
    assert( !items.empty() ); // index would not be valid, and this would be an endless loop
    while( !items[index]->is_item_entry() ) {
        mod_index( offset );
    }
}

void advanced_inv_pane::mod_index( int offset )
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

void advanced_inv_pane::scroll_by( int offset )
{
    assert( offset != 0 ); // 0 would make no sense
    if( items.empty() ) {
        return;
    }
    if( inCategoryMode ) {
        assert( get_cur_item_ptr() != nullptr ); // index must already be valid!
        auto cur_cat = items[index]->cat;
        if( offset > 0 ) {
            while( items[index]->cat == cur_cat ) {
                index++;
                if( static_cast<size_t>( index ) >= items.size() ) {
                    index = 0; // wrap to begin, stop there.
                    break;
                }
            }
        } else {
            while( items[index]->cat == cur_cat ) {
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

advanced_inv_listitem *advanced_inv_pane::get_cur_item_ptr()
{
    if( static_cast<size_t>( index ) >= items.size() ) {
        return nullptr;
    }
    return items[index];
}

void advanced_inv_pane::set_filter( const std::string &new_filter )
{
    if( filter == new_filter ) {
        return;
    }
    filter = new_filter;
    filtercache.clear();
    needs_recalc = true;
}

bool advanced_inv_pane::is_filtered( const advanced_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool advanced_inv_pane::is_filtered( const item &it ) const
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

void advanced_inv_pane::paginate()
{
    if( sortby != COLUMN_SORTBY_CATEGORY ) {
        return; // not needed as there are no category entries here.
    }
    // first, we insert all the items, then we sort the result
    for( size_t i = 0; i < items.size(); ++i ) {
        if( i % itemsPerPage == 0 ) {
            // first entry on the page, should be a category header
            if( items[i]->is_item_entry() ) {
                items.insert( items.begin() + i, create_listitem( items[i]->cat ) );
            }
        }
        if( ( i + 1 ) % itemsPerPage == 0 && i + 1 < items.size() ) {
            // last entry of the page, but not the last entry at all!
            // Must *not* be a category header!
            if( items[i]->is_category_header() ) {
                items.insert( items.begin() + i, create_listitem() );
            }
        }
    }
}

void advanced_inv_pane::fix_index()
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

void advanced_inv_pane::recalc()
{
    needs_recalc = false;
    clear_items();
    // Add items from the source location or in case of all 9 surrounding squares,
    // add items from several locations.

    units::volume tmp_vol;
    units::mass tmp_weight;
    if( cur_area->info.type == advanced_inv_area_info::AREA_TYPE_MULTI ) {
        volume = 0_ml;
        weight = 0_gram;
        std::vector<advanced_inv_area_info::aim_location> loc = cur_area->info.multi_locations;
        for( auto &i : loc ) {
            advanced_inv_area &s = get_square( i );
            // Deal with squares with ground + vehicle storage
            // Also handle the case when the other tile covers vehicle
            // or the ground below the vehicle.
            if( s.has_vehicle() ) {
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
        add_items_from_area( *cur_area, viewing_cargo, volume, weight );
    }

    // Insert category headers (only expected when sorting by category)
    if( sortby == COLUMN_SORTBY_CATEGORY ) {
        std::set<const item_category *> categories;
        for( auto &it : items ) {
            categories.insert( it->cat );
        }
        for( auto &cat : categories ) {
            items.push_back( create_listitem( cat ) );
        }
    }
    // Finally sort all items (category headers will now be moved to their proper position)
    if( !items.empty() ) {
        std::stable_sort( items.begin(), items.end(), items.front()->get_sort_function( sortby,
                          default_sortby ) );
    }
    paginate();

    int i = 0;
    for( auto &it : items ) {
        if( it->items.front() == save_state.selected_itm ) {
            save_state.selected_idx = i;
            index = i;
            break;
        }
        i++;
    }
}

void advanced_inv_pane::add_items_from_area( advanced_inv_area &area, bool from_cargo,
        units::volume &ret_volume, units::mass &ret_weight )
{
    assert( area.info.type != advanced_inv_area_info::AREA_TYPE_MULTI );
    if( !area.is_valid() ) {
        return;
    }

    ret_volume = 0_ml;
    ret_weight = 0_gram;
    advanced_inv_listitem *it;

    // Existing items are *not* cleared on purpose, this might be called
    // several times in case all surrounding squares are to be shown.
    const advanced_inv_area::area_items stacks = area.get_items( from_cargo );

    for( size_t x = 0; x < stacks.size(); ++x ) {
        it = create_listitem( stacks[x], x, area, from_cargo );
        if( is_filtered( *it->items.front() ) ) {
            continue;
        }
        ret_volume += it->volume;
        ret_weight += it->weight;
        items.push_back( it );
    }
}

void advanced_inv_pane::redraw()
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
    int width = print_header( itm != nullptr ? itm->get_area() : get_area() );

    bool car = cur_area->has_vehicle() && viewing_cargo;
    auto name = utf8_truncate( cur_area->get_name( viewing_cargo ), width );
    auto desc = utf8_truncate( cur_area->desc[car], width );
    width -= 2 + 1; // starts at offset 2, plus space between the header and the text
    mvwprintz( w, point( 2, 1 ), c_green, name );
    mvwprintz( w, point( 2, 2 ), c_light_blue, desc );
    trim_and_print( w, point( 2, 3 ), width, c_cyan, cur_area->flags );

    for( auto &i : additional_info ) {
        print_colored_text( window, i.position, i.color, i.color, i.message );
    }

    const int max_page = ( items.size() + itemsPerPage - 1 ) / itemsPerPage;
    if( max_page > 1 ) {
        const int page = index / itemsPerPage;
        mvwprintz( w, point( 2, 4 ), c_light_blue, _( "[<] page %d of %d [>]" ), page + 1, max_page );
    }

    wattron( w, c_cyan );
    draw_border( w, BORDER_COLOR );
    mvwprintw( w, point( 3, 0 ), _( "< [s]ort: %s >" ),
               advanced_inv_listitem::get_col_data( sortby ).get_sort_name() );

    if( !title.empty() ) {
        std::string title_string = string_format( "<< %s >>", title );
        mvwprintz( w, point( ( getmaxx( w ) - title_string.size() ) / 2, 0 ), c_light_cyan, title_string );
    }

    const char *fprefix = _( "[F]ilter" );
    const char *fsuffix = _( "[R]eset" );

    if( !filter_edit ) {
        std::string s = filter.empty() ? "< %s >" : "< %s: %s >";
        mvwprintw( w, point( 2, getmaxy( w ) - 1 ), string_format( s, fprefix, filter ) );
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

int advanced_inv_pane::print_header( advanced_inv_area &sel_area )
{
    int ofs = getmaxx( window ) - 25 - 2 - 14;

    // print icon for dragged 'area'
    const int drag_x = 25 + ofs;
    const int drag_y = 1;
    if( g->u.get_grab_type() == OBJECT_VEHICLE ) {
        mvwprintz( window, point( drag_x, drag_y ), c_dark_gray, _( "<D>" ) );
    } else {
        mvwprintz( window, point( drag_x, drag_y ), c_red, _( "[D]" ) );
    }

    // print icons for all advanced_inv_area
    for( size_t i = 0; i < all_areas.size(); ++i ) {
        std::string key = advanced_inv_area::get_info(
                              static_cast<advanced_inv_area_info::aim_location>( i ) ).minimapname;

        advanced_inv_area &iter_square = get_square( i );

        const char *bracket = iter_square.has_vehicle() ? "<>" : "[]";

        bool is_single = cur_area->info.type != advanced_inv_area_info::AREA_TYPE_MULTI;
        bool is_selected_by_user = cur_area->info.has_area( iter_square.info.id );
        bool is_selected_by_item;
        if( is_single ) {
            is_selected_by_item = is_selected_by_user;
        } else {
            is_selected_by_item = iter_square.info.id == sel_area.info.id;
        }
        nc_color bcolor = c_red;
        nc_color kcolor = c_red;
        if( iter_square.is_valid() ) {
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
                if( cur_area->info.id == iter_square.info.id ) {
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
        }

        const int x = iter_square.info.hscreen.x + ofs;
        const int y = iter_square.info.hscreen.y;
        mvwprintz( window, point( x, y ), bcolor, "%c", bracket[0] );
        wprintz( window, kcolor, "%s", viewing_cargo && is_selected_by_item && is_single
                 ? "V" : key );
        wprintz( window, bcolor, "%c", bracket[1] );
    }
    return get_square( advanced_inv_area_info::AIM_INVENTORY ).info.hscreen.y + ofs;
}
