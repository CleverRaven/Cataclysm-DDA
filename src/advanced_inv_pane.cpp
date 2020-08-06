#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_pagination.h"
#include "advanced_inv_pane.h"
#include "avatar.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_search.h"
#include "map.h"
#include "options.h"
#include "player.h"
#include "uistate.h"
#include "units.h"
#include "vehicle.h"

#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif
void advanced_inventory_pane::save_settings()
{
    save_state->in_vehicle = in_vehicle();
    save_state->area_idx = get_area();
    save_state->selected_idx = index;
    save_state->filter = filter;
    save_state->sort_idx = sortby;
}

void advanced_inventory_pane::load_settings( int saved_area_idx,
        const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares, bool is_re_enter )
{
    const int i_location = ( get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) ) ? saved_area_idx :
                           save_state->area_idx;
    const aim_location location = static_cast<aim_location>( i_location );
    const advanced_inv_area square = squares[location];
    // determine the square's vehicle/map item presence
    bool has_veh_items = square.can_store_in_vehicle() ?
                         !square.veh->get_items( square.vstor ).empty() : false;
    bool has_map_items = !get_map().i_at( square.pos ).empty();
    // determine based on map items and settings to show cargo
    bool show_vehicle = is_re_enter ?
                        save_state->in_vehicle : has_veh_items ? true :
                        has_map_items ? false : square.can_store_in_vehicle();
    set_area( square, show_vehicle );
    sortby = static_cast<advanced_inv_sortby>( save_state->sort_idx );
    index = save_state->selected_idx;
    filter = save_state->filter;
}

static const std::string flag_HIDDEN_ITEM( "HIDDEN_ITEM" );

bool advanced_inventory_pane::is_filtered( const advanced_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool advanced_inventory_pane::is_filtered( const item &it ) const
{
    if( it.has_flag( flag_HIDDEN_ITEM ) ) {
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

/** converts a raw list of items to "stacks" - itms that are not count_by_charges that otherwise stack go into one stack */
static std::vector<std::vector<item *>> item_list_to_stack( std::list<item *> item_list )
{
    std::vector<std::vector<item *>> ret;
    for( auto iter_outer = item_list.begin(); iter_outer != item_list.end(); ++iter_outer ) {
        std::vector<item *> item_stack( { *iter_outer } );
        for( auto iter_inner = std::next( iter_outer ); iter_inner != item_list.end(); ) {
            if( ( *iter_outer )->display_stacked_with( **iter_inner ) ) {
                item_stack.push_back( *iter_inner );
                iter_inner = item_list.erase( iter_inner );
            } else {
                ++iter_inner;
            }
        }
        ret.push_back( item_stack );
    }
    return ret;
}

std::vector<advanced_inv_listitem> avatar::get_AIM_inventory( const advanced_inventory_pane &pane,
        advanced_inv_area &square )
{
    std::vector<advanced_inv_listitem> items;
    size_t item_index = 0;

    int worn_index = -2;
    for( item &worn_item : worn ) {
        if( worn_item.contents.empty() ) {
            continue;
        }
        for( const std::vector<item *> &it_stack : item_list_to_stack(
                 worn_item.contents.all_items_top() ) ) {
            advanced_inv_listitem adv_it( it_stack, item_index++, square.id, false );
            if( !pane.is_filtered( *adv_it.items.front() ) ) {
                square.volume += adv_it.volume;
                square.weight += adv_it.weight;
                items.push_back( adv_it );
            }
        }
        worn_index--;
    }

    return items;
}

void advanced_inventory_pane::add_items_from_area( advanced_inv_area &square,
        bool vehicle_override )
{
    assert( square.id != AIM_ALL );
    if( !square.canputitems() ) {
        return;
    }
    map &m = get_map();
    avatar &u = get_avatar();
    // Existing items are *not* cleared on purpose, this might be called
    // several times in case all surrounding squares are to be shown.
    if( square.id == AIM_INVENTORY ) {
        square.volume = 0_ml;
        square.weight = 0_gram;
        items = u.get_AIM_inventory( *this, square );
    } else if( square.id == AIM_WORN ) {
        square.volume = 0_ml;
        square.weight = 0_gram;
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
        square.volume = 0_ml;
        square.weight = 0_gram;
        item *cont = square.get_container( in_vehicle() );
        if( cont != nullptr ) {
            if( !cont->is_container_empty() ) {
                // filtering does not make sense for liquid in container
                item *it = &square.get_container( in_vehicle() )->contents.legacy_front();
                advanced_inv_listitem ait( it, 0, 1, square.id, in_vehicle() );
                square.volume += ait.volume;
                square.weight += ait.weight;
                items.push_back( ait );
            }
            square.desc[0] = cont->tname( 1, false );
        }
    } else {
        bool is_in_vehicle = square.can_store_in_vehicle() && ( in_vehicle() || vehicle_override );
        if( is_in_vehicle ) {
            square.volume_veh = 0_ml;
            square.weight_veh = 0_gram;
        } else {
            square.volume = 0_ml;
            square.weight = 0_gram;
        }
        const advanced_inv_area::itemstack &stacks = is_in_vehicle ?
                square.i_stacked( square.veh->get_items( square.vstor ) ) :
                square.i_stacked( m.i_at( square.pos ) );

        for( size_t x = 0; x < stacks.size(); ++x ) {
            advanced_inv_listitem it( stacks[x], x, square.id, is_in_vehicle );
            if( is_filtered( *it.items.front() ) ) {
                continue;
            }
            if( is_in_vehicle ) {
                square.volume_veh += it.volume;
                square.weight_veh += it.weight;
            } else {
                square.volume += it.volume;
                square.weight += it.weight;
            }
            items.push_back( it );
        }
    }
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
}

void advanced_inventory_pane::mod_index( int offset )
{
    // 0 would make no sense
    assert( offset != 0 );
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
    // 0 would make no sense
    assert( offset != 0 );
    if( items.empty() ) {
        return;
    }
    mod_index( offset );
}

void advanced_inventory_pane::scroll_page( int linesPerPage, int offset )
{
    // only those two offsets are allowed
    assert( offset == -1 || offset == +1 );
    if( items.empty() ) {
        return;
    }
    const int size = static_cast<int>( items.size() );

    advanced_inventory_pagination old_pagination( linesPerPage, *this );
    for( int i = 0; i <= index; i++ ) {
        old_pagination.step( i );
    }

    // underflow
    if( old_pagination.page + offset < 0 ) {
        if( index > 0 ) {
            // scroll to top of first page
            index = 0;
        } else {
            // scroll wrap
            index = size - 1;
        }
        return;
    }

    int previous_line = -1; // matching line one up from our line
    advanced_inventory_pagination new_pagination( linesPerPage, *this );
    for( int i = 0; i < size; i++ ) {
        new_pagination.step( i );
        // right page
        if( new_pagination.page == old_pagination.page + offset ) {
            // right line
            if( new_pagination.line == old_pagination.line ) {
                index = i;
                return;
            }
            // one up from right line
            if( new_pagination.line == old_pagination.line - 1 ) {
                previous_line = i;
            }
        }
    }
    // second-best matching line
    if( previous_line != -1 ) {
        index = previous_line;
        return;
    }

    // overflow
    if( index < size - 1 ) {
        // scroll to end of last page
        index = size - 1;
    } else {
        // scroll wrap
        index = 0;
    }
}

void advanced_inventory_pane::scroll_category( int offset )
{
    // only those two offsets are allowed
    assert( offset == -1 || offset == +1 );
    if( items.empty() ) {
        return;
    }
    // index must already be valid!
    assert( get_cur_item_ptr() != nullptr );
    const item_category *cur_cat = items[index].cat;
    if( offset > 0 ) {
        while( items[index].cat == cur_cat ) {
            index++;
            // wrap to begin, stop there.
            if( static_cast<size_t>( index ) >= items.size() ) {
                index = 0;
                break;
            }
        }
    } else {
        while( items[index].cat == cur_cat ) {
            index--;
            // wrap to end, stop there.
            if( index < 0 ) {
                index = static_cast<int>( items.size() ) - 1;
                break;
            }
        }
    }
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
