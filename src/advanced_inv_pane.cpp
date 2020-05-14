#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_pane.h"
#include "avatar.h"
#include "game.h"
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
    auto square = squares[location];
    // determine the square's vehicle/map item presence
    bool has_veh_items = square.can_store_in_vehicle() ?
                         !square.veh->get_items( square.vstor ).empty() : false;
    bool has_map_items = !g->m.i_at( square.pos ).empty();
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
        const advanced_inv_area::itemstack &stacks = is_in_vehicle ?
                square.i_stacked( square.veh->get_items( square.vstor ) ) :
                square.i_stacked( m.i_at( square.pos ) );

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
    // not needed as there are no category entries here.
    if( sortby != SORTBY_CATEGORY ) {
        return;
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

void advanced_inventory_pane::skip_category_headers( int offset )
{
    // 0 would make no sense
    assert( offset != 0 );
    // valid index is required
    assert( static_cast<size_t>( index ) < items.size() );
    // only those two offsets are allowed
    assert( offset == -1 || offset == +1 );
    // index would not be valid, and this would be an endless loop
    assert( !items.empty() );
    while( !items[index].is_item_entry() ) {
        mod_index( offset );
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
    skip_category_headers( offset > 0 ? +1 : -1 );
}

void advanced_inventory_pane::scroll_category( int offset )
{
    // 0 would make no sense
    assert( offset != 0 );
    // only those two offsets are allowed
    assert( offset == -1 || offset == +1 );
    if( items.empty() ) {
        return;
    }
    // index must already be valid!
    assert( get_cur_item_ptr() != nullptr );
    auto cur_cat = items[index].cat;
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
    // Make sure we land on an item entry.
    skip_category_headers( offset > 0 ? +1 : -1 );
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
