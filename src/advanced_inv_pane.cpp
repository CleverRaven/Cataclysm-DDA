#include <cstddef>
#include <iterator>
#include <list>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_pagination.h"
#include "advanced_inv_pane.h"
#include "avatar.h"
#include "cata_assert.h"
#include "flag.h"
#include "item.h"
#include "item_pocket.h"
#include "item_search.h"
#include "make_static.h"
#include "map.h"
#include "map_selector.h"
#include "options.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"

class item_category;

#if defined(__ANDROID__)
#   include <SDL_keyboard.h>
#endif

void advanced_inventory_pane::set_area( const advanced_inv_area &square, bool in_vehicle_cargo )
{
    prev_area = area;
    prev_viewing_cargo = viewing_cargo;
    area = square.id;
    viewing_cargo = square.can_store_in_vehicle() && ( in_vehicle_cargo || area == AIM_DRAGGED );
}

void advanced_inventory_pane::restore_area()
{
    area = prev_area;
    viewing_cargo = prev_viewing_cargo;
}

void advanced_inventory_pane::save_settings() const
{
    save_state->container = container;
    save_state->in_vehicle = in_vehicle();
    save_state->area_idx = get_area();
    save_state->selected_idx = index;
    save_state->filter = filter;
    save_state->sort_idx = sortby;
}

void advanced_inventory_pane::load_settings( int saved_area_idx,
        const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares, bool is_re_enter )
{
    const int i_location = ( get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) &&
                             !is_re_enter ) ? saved_area_idx :
                           save_state->area_idx;
    const aim_location location = static_cast<aim_location>( i_location );
    const advanced_inv_area &square = squares[location];
    // determine the square's vehicle/map item presence
    bool has_veh_items = square.can_store_in_vehicle() ?
                         !square.veh->get_items( square.vstor ).empty() : false;
    bool has_map_items = !get_map().i_at( square.pos ).empty();
    // determine based on map items and settings to show cargo
    bool show_vehicle = false;
    if( is_re_enter ) {
        show_vehicle = save_state->in_vehicle;
    } else if( has_veh_items == has_map_items ) {
        show_vehicle = save_state->in_vehicle && square.can_store_in_vehicle();
    } else {
        show_vehicle = has_veh_items;
    }
    set_area( square, show_vehicle );
    sortby = static_cast<advanced_inv_sortby>( save_state->sort_idx );
    index = save_state->selected_idx;
    filter = save_state->filter;
    container = save_state->container;
}

bool advanced_inventory_pane::is_filtered( const advanced_inv_listitem &it ) const
{
    return is_filtered( *it.items.front() );
}

bool advanced_inventory_pane::is_filtered( const item &it ) const
{
    if( it.has_flag( STATIC( flag_id( "HIDDEN_ITEM" ) ) ) ) {
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

/** converts a raw list of items to "stacks" - items that are not count_by_charges that otherwise stack go into one stack */
static std::vector<std::vector<item_location>> item_list_to_stack(
            const item_location &parent, std::list<item *> item_list )
{
    std::vector<std::vector<item_location>> ret;
    for( auto iter_outer = item_list.begin(); iter_outer != item_list.end(); ++iter_outer ) {
        std::vector<item_location> item_stack( { item_location( parent, *iter_outer ) } );
        for( auto iter_inner = std::next( iter_outer ); iter_inner != item_list.end(); ) {
            if( ( *iter_outer )->display_stacked_with( **iter_inner ) ) {
                item_stack.emplace_back( parent, *iter_inner );
                iter_inner = item_list.erase( iter_inner );
            } else {
                ++iter_inner;
            }
        }
        ret.push_back( item_stack );
    }
    return ret;
}

std::vector<advanced_inv_listitem> outfit::get_AIM_inventory( size_t &item_index, avatar &you,
        const advanced_inventory_pane &pane, advanced_inv_area &square )
{
    std::vector<advanced_inv_listitem> items;
    for( item &worn_item : worn ) {
        if( worn_item.empty() || worn_item.has_flag( flag_NO_UNLOAD ) ) {
            continue;
        }
        for( const std::vector<item_location> &it_stack : item_list_to_stack(
                 item_location( you, &worn_item ),
                 worn_item.all_items_top( item_pocket::pocket_type::CONTAINER ) ) ) {
            advanced_inv_listitem adv_it( it_stack, item_index++, square.id, false );
            if( !pane.is_filtered( *adv_it.items.front() ) ) {
                square.volume += adv_it.volume;
                square.weight += adv_it.weight;
                items.push_back( adv_it );
            }
        }
    }
    return items;
}

std::vector<advanced_inv_listitem> avatar::get_AIM_inventory( const advanced_inventory_pane &pane,
        advanced_inv_area &square )
{
    size_t item_index = 0;

    std::vector<advanced_inv_listitem> items = worn.get_AIM_inventory( item_index, *this, pane,
            square );

    item_location weapon = get_wielded_item();
    if( weapon && weapon->is_container() ) {
        for( const std::vector<item_location> &it_stack : item_list_to_stack( weapon,
                weapon->all_items_top( item_pocket::pocket_type::CONTAINER ) ) ) {
            advanced_inv_listitem adv_it( it_stack, item_index++, square.id, false );
            if( !pane.is_filtered( *adv_it.items.front() ) ) {
                square.volume += adv_it.volume;
                square.weight += adv_it.weight;
                items.push_back( adv_it );
            }
        }
    }
    return items;
}

void outfit::add_AIM_items_from_area( avatar &you, advanced_inv_area &square,
                                      advanced_inventory_pane &pane )
{
    auto iter = worn.begin();
    for( size_t i = 0; i < worn.size(); ++i, ++iter ) {
        advanced_inv_listitem it( item_location( you, &*iter ), i + 1, 1, square.id, false );
        if( pane.is_filtered( *it.items.front() ) ) {
            continue;
        }
        square.volume += it.volume;
        square.weight += it.weight;
        pane.items.push_back( it );
    }
}

void advanced_inventory_pane::add_items_from_area( advanced_inv_area &square,
        bool vehicle_override )
{
    cata_assert( square.id != AIM_ALL );
    if( !square.canputitems( container ) ) {
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

        item_location weapon = u.get_wielded_item();
        if( weapon ) {
            advanced_inv_listitem it( weapon, 0, 1, square.id, false );
            if( !is_filtered( *it.items.front() ) ) {
                square.volume += it.volume;
                square.weight += it.weight;
                items.push_back( it );
            }
        }

        u.worn.add_AIM_items_from_area( u, square, *this );
    } else if( square.id == AIM_CONTAINER ) {
        square.volume = 0_ml;
        square.weight = 0_gram;
        if( container ) {
            if( !container->is_container_empty() ) {
                // filtering does not make sense for liquid in container
                size_t item_index = 0;
                for( const std::vector<item_location> &it_stack : item_list_to_stack( container,
                        container->all_items_top() ) ) {
                    advanced_inv_listitem adv_it( it_stack, item_index++, square.id, false );
                    if( !is_filtered( *adv_it.items.front() ) ) {
                        square.volume += adv_it.volume;
                        square.weight += adv_it.weight;
                        items.push_back( adv_it );
                    }
                }
            }
            square.desc[0] = container->tname( 1, false );
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

        map_cursor loc_cursor( square.pos );
        for( size_t x = 0; x < stacks.size(); ++x ) {
            std::vector<item_location> locs;
            locs.reserve( stacks[x].size() );
            for( item *const it : stacks[x] ) {
                if( is_in_vehicle ) {
                    locs.emplace_back( vehicle_cursor( *square.veh, square.vstor ), it );
                } else {
                    locs.emplace_back( loc_cursor, it );
                    if( it->is_corpse() ) {
                        for( item *loot : it->all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                            if( !is_filtered( *loot ) ) {
                                advanced_inv_listitem aim_item( item_location( loc_cursor, loot ), 0, 1, square.id,
                                                                is_in_vehicle );
                                square.volume += aim_item.volume;
                                square.weight += aim_item.weight;
                                items.push_back( aim_item );
                            }
                        }
                    }
                }
            }
            advanced_inv_listitem it( locs, x, square.id, is_in_vehicle );
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
    cata_assert( offset != 0 );
    cata_assert( !items.empty() );
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
    cata_assert( offset != 0 );
    if( items.empty() ) {
        return;
    }
    mod_index( offset );
}

void advanced_inventory_pane::scroll_page( int linesPerPage, int offset )
{
    // only those two offsets are allowed
    cata_assert( offset == -1 || offset == +1 );
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
    cata_assert( offset == -1 || offset == +1 );
    if( items.empty() ) {
        return;
    }
    // index must already be valid!
    cata_assert( get_cur_item_ptr() != nullptr );
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

void advanced_inventory_pane::scroll_to_start()
{
    index = 0;
}

void advanced_inventory_pane::scroll_to_end()
{
    index = static_cast<int>( items.size() ) - 1;
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
