#include "inventory_ui.h"

#include <cstdint>

#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "colony.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "inventory.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "item_search.h"
#include "item_stack.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_selector.h"
#include "memory_fast.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "ret_val.h"
#include "sdltiles.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#endif

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

/** The maximum distance from the screen edge, to snap a window to it */
static const size_t max_win_snap_distance = 4;
/** The minimal gap between two cells */
static const int min_cell_gap = 2;
/** The gap between two cells when screen space is limited*/
static const int normal_cell_gap = 4;
/** The minimal gap between the first cell and denial */
static const int min_denial_gap = 2;
/** The minimal gap between two columns */
static const int min_column_gap = 2;
/** The gap between two columns when there's enough space, but they are not centered */
static const int normal_column_gap = 8;
/**
 * The minimal occupancy ratio to align columns to the center
 * @see inventory_selector::get_columns_occupancy_ratio()
 */
static const double min_ratio_to_center = 0.85;

/** These categories should keep their original order and can't be re-sorted by inventory presets */
static const std::set<std::string> ordered_categories = {{ "ITEMS_WORN" }};

struct navigation_mode_data {
    navigation_mode next_mode;
    translation name;
    nc_color color;
};

struct inventory_input {
    std::string action;
    int ch = 0;
    inventory_entry *entry;
};

bool inventory_entry::operator==( const inventory_entry &other ) const
{
    return get_category_ptr() == other.get_category_ptr() && locations == other.locations;
}

class selection_column_preset : public inventory_selector_preset
{
    public:
        selection_column_preset() = default;
        std::string get_caption( const inventory_entry &entry ) const override {
            std::string res;
            const size_t available_count = entry.get_available_count();
            const item_location &item = entry.any_item();

            if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
                //~ %1$d: chosen count, %2$d: available count
                res += string_format( pgettext( "count", "%1$d of %2$d" ), entry.chosen_count,
                                      available_count ) + " ";
            } else if( available_count != 1 ) {
                res += string_format( "%d ", available_count );
            }
            if( item->is_money() ) {
                cata_assert( available_count == entry.get_stack_size() );
                if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
                    res += item->display_money( available_count, item->ammo_remaining(),
                                                entry.get_selected_charges() );
                } else {
                    res += item->display_money( available_count, item->ammo_remaining() );
                }
            } else {
                res += item->display_name( available_count );
            }
            return res;
        }

        nc_color get_color( const inventory_entry &entry ) const override {
            Character &player_character = get_player_character();
            if( entry.is_item() ) {
                if( &*entry.any_item() == &player_character.weapon ) {
                    return c_light_blue;
                } else if( player_character.is_worn( *entry.any_item() ) ) {
                    return c_cyan;
                }
            }
            return inventory_selector_preset::get_color( entry );
        }
};

static const selection_column_preset selection_preset{};

int inventory_entry::get_total_charges() const
{
    int result = 0;
    for( const item_location &location : locations ) {
        result += location->charges;
    }
    return result;
}

int inventory_entry::get_selected_charges() const
{
    cata_assert( chosen_count <= locations.size() );
    int result = 0;
    for( size_t i = 0; i < chosen_count; ++i ) {
        const item_location &location = locations[i];
        result += location->charges;
    }
    return result;
}

size_t inventory_entry::get_available_count() const
{
    if( locations.size() == 1 ) {
        return any_item()->count();
    } else {
        return locations.size();
    }
}

int inventory_entry::get_invlet() const
{
    if( custom_invlet != INT_MIN ) {
        return custom_invlet;
    }
    if( !is_item() ) {
        return '\0';
    }
    return any_item()->invlet;
}

nc_color inventory_entry::get_invlet_color() const
{
    if( !is_selectable() ) {
        return c_dark_gray;
    } else if( get_player_character().inv->assigned_invlet.count( get_invlet() ) ) {
        return c_yellow;
    } else {
        return c_white;
    }
}

void inventory_entry::update_cache()
{
    cached_name = any_item()->tname( 1 );
}

const item_category *inventory_entry::get_category_ptr() const
{
    if( custom_category != nullptr ) {
        return custom_category;
    }
    if( !is_item() ) {
        return nullptr;
    }
    return &any_item()->get_category_of_contents();
}

bool inventory_column::activatable() const
{
    return std::any_of( entries.begin(), entries.end(), []( const inventory_entry & e ) {
        return e.is_selectable();
    } );
}

inventory_entry *inventory_column::find_by_invlet( int invlet ) const
{
    for( const auto &elem : entries ) {
        if( elem.is_item() && elem.get_invlet() == invlet ) {
            return const_cast<inventory_entry *>( &elem );
        }
    }
    return nullptr;
}

size_t inventory_column::get_width() const
{
    return std::max( get_cells_width(), reserved_width );
}

size_t inventory_column::get_height() const
{
    return std::min( entries.size(), height );
}

inventory_selector_preset::inventory_selector_preset()
{
    append_cell(
    std::function<std::string( const inventory_entry & )>( [ this ]( const inventory_entry & entry ) {
        return get_caption( entry );
    } ) );
}

bool inventory_selector_preset::sort_compare( const inventory_entry &lhs,
        const inventory_entry &rhs ) const
{
    return lhs.cached_name.compare( rhs.cached_name ) < 0; // Simple alphabetic order
}

nc_color inventory_selector_preset::get_color( const inventory_entry &entry ) const
{
    return entry.is_item() ? entry.any_item()->color_in_inventory() : c_magenta;
}

std::function<bool( const inventory_entry & )> inventory_selector_preset::get_filter(
    const std::string &filter ) const
{
    auto item_filter = basic_item_filter( filter );

    return [item_filter]( const inventory_entry & e ) {
        return item_filter( *e.any_item() );
    };
}

std::string inventory_selector_preset::get_caption( const inventory_entry &entry ) const
{
    const size_t count = entry.get_stack_size();
    std::string disp_name;
    if( entry.any_item()->is_money() ) {
        disp_name = entry.any_item()->display_money( count, entry.any_item()->ammo_remaining() );
    } else {
        disp_name = entry.any_item()->display_name( count );
    }

    return ( count > 1 ) ? string_format( "%d %s", count, disp_name ) : disp_name;
}

std::string inventory_selector_preset::get_denial( const inventory_entry &entry ) const
{
    return entry.is_item() ? get_denial( entry.any_item() ) : std::string();
}

std::string inventory_selector_preset::get_cell_text( const inventory_entry &entry,
        size_t cell_index ) const
{
    if( cell_index >= cells.size() ) {
        debugmsg( "Invalid cell index %d.", cell_index );
        return "it's a bug!";
    }
    if( !entry ) {
        return std::string();
    } else if( entry.is_item() ) {
        return cells[cell_index].get_text( entry );
    } else if( cell_index != 0 ) {
        return replace_colors( cells[cell_index].title );
    } else {
        return entry.get_category_ptr()->name();
    }
}

bool inventory_selector_preset::is_stub_cell( const inventory_entry &entry,
        size_t cell_index ) const
{
    if( !entry.is_item() ) {
        return false;
    }
    const std::string &text = get_cell_text( entry, cell_index );
    return text.empty() || text == cells[cell_index].stub;
}

void inventory_selector_preset::append_cell( const
        std::function<std::string( const item_location & )> &func,
        const std::string &title, const std::string &stub )
{
    // Don't capture by reference here. The func should be able to die earlier than the object itself
    append_cell( std::function<std::string( const inventory_entry & )>( [ func ](
    const inventory_entry & entry ) {
        return func( entry.any_item() );
    } ), title, stub );
}

void inventory_selector_preset::append_cell( const
        std::function<std::string( const inventory_entry & )> &func,
        const std::string &title, const std::string &stub )
{
    const auto iter = std::find_if( cells.begin(), cells.end(), [ &title ]( const cell_t &cell ) {
        return cell.title == title;
    } );
    if( iter != cells.end() ) {
        debugmsg( "Tried to append a duplicate cell \"%s\": ignored.", title.c_str() );
        return;
    }
    cells.emplace_back( func, title, stub );
}

std::string inventory_selector_preset::cell_t::get_text( const inventory_entry &entry ) const
{
    return replace_colors( func( entry ) );
}

bool inventory_holster_preset::is_shown( const item_location &contained ) const
{
    if( contained.eventually_contains( holster ) ) {
        return false;
    }
    if( contained.where() != item_location::type::container
        && contained->made_of( phase_id::LIQUID ) ) {
        // spilt liquid cannot be picked up
        return false;
    }
    item item_copy( *contained );
    item_copy.charges = 1;
    if( !holster->contents.can_contain( item_copy ).success() ) {
        return false;
    }

    //only hide if it is in the toplevel of holster (to allow shuffling of items inside a bag)
    for( const item *it : holster->all_items_top() ) {
        if( it == contained.get_item() ) {
            return false;
        }
    }

    if( contained->is_bucket_nonempty() ) {
        return false;
    }
    if( !holster->all_pockets_rigid() &&
        !holster.parents_can_contain_recursive( &item_copy ) ) {
        return false;
    }
    return true;
}

void inventory_column::select( size_t new_index, scroll_direction dir )
{
    if( new_index < entries.size() ) {
        if( !entries[new_index].is_selectable() ) {
            new_index = next_selectable_index( new_index, dir );
        }

        selected_index = new_index;
        page_offset = ( new_index == static_cast<size_t>( -1 ) ) ?
                      0 : selected_index - selected_index % entries_per_page;
    }
}

size_t inventory_column::next_selectable_index( size_t index, scroll_direction dir ) const
{
    if( entries.empty() ) {
        return index;
    }
    // limit index to the space of the size of entries
    index = index % entries.size();
    size_t new_index = index;
    do {
        // 'new_index' incremented by 'dir' using division remainder (number of entries) to loop over the entries.
        // Negative step '-k' (backwards) is equivalent to '-k + N' (forward), where:
        //     N = entries.size()  - number of elements,
        //     k = |step|          - absolute step (k <= N).
        new_index = ( new_index + static_cast<int>( dir ) + entries.size() ) % entries.size();
    } while( new_index != index && !entries[new_index].is_selectable() );

    if( !entries[new_index].is_selectable() ) {
        return static_cast<size_t>( -1 );
    }

    return new_index;
}

void inventory_column::move_selection( scroll_direction dir )
{
    size_t index = selected_index;

    do {
        index = next_selectable_index( index, dir );
    } while( index != selected_index && is_selected_by_category( entries[index] ) );

    select( index, dir );
}

void inventory_column::move_selection_page( scroll_direction dir )
{
    size_t index = selected_index;

    do {
        const size_t next_index = next_selectable_index( index, dir );
        const bool flipped = next_index == selected_index ||
                             ( next_index > selected_index ) != ( static_cast<int>( dir ) > 0 );

        if( flipped && page_of( next_index ) == page_index() ) {
            break; // If flipped and still on the same page - no need to flip
        }

        index = next_index;
    } while( page_of( next_selectable_index( index, dir ) ) == page_index() );

    select( index, dir );
}

size_t inventory_column::get_entry_cell_width( size_t index, size_t cell_index ) const
{
    size_t res = utf8_width( get_entry_cell_cache( index ).text[cell_index], true );

    if( cell_index == 0 ) {
        res += get_entry_indent( entries[index] );
    }

    return res;
}

size_t inventory_column::get_entry_cell_width( const inventory_entry &entry,
        size_t cell_index ) const
{
    size_t res = utf8_width( preset.get_cell_text( entry, cell_index ), true );

    if( cell_index == 0 ) {
        res += get_entry_indent( entry );
    }

    return res;
}

size_t inventory_column::get_cells_width() const
{
    return std::accumulate( cells.begin(), cells.end(), static_cast<size_t>( 0 ), []( size_t lhs,
    const cell_t &cell ) {
        return lhs + cell.current_width;
    } );
}

void inventory_column::set_filter( const std::string &filter )
{
    entries = entries_unfiltered;
    entries_cell_cache.clear();
    paging_is_valid = false;
    prepare_paging( filter );
}

void selection_column::set_filter( const std::string & )
{
    // always show all selected items
    inventory_column::set_filter( std::string() );
}

inventory_column::entry_cell_cache_t inventory_column::make_entry_cell_cache(
    const inventory_entry &entry ) const
{
    entry_cell_cache_t result;

    result.assigned = true;
    result.color = preset.get_color( entry );
    result.denial = preset.get_denial( entry );
    result.text.resize( preset.get_cells_count() );

    for( size_t i = 0, n = preset.get_cells_count(); i < n; ++i ) {
        result.text[i] = preset.get_cell_text( entry, i );
    }

    return result;
}

const inventory_column::entry_cell_cache_t &inventory_column::get_entry_cell_cache(
    size_t index ) const
{
    cata_assert( index < entries.size() );

    if( entries_cell_cache.size() < entries.size() ) {
        entries_cell_cache.resize( entries.size() );
    }

    if( !entries_cell_cache[index].assigned ) {
        entries_cell_cache[index] = make_entry_cell_cache( entries[index] );
    }

    return entries_cell_cache[index];
}

void inventory_column::set_width( const size_t new_width,
                                  const std::vector<inventory_column *> &all_columns )
{
    reset_width( all_columns );
    int width_gap = get_width() - new_width;
    // Now adjust the width if we must
    while( width_gap != 0 ) {
        const int step = width_gap > 0 ? -1 : 1;
        // Should return true when lhs < rhs
        const auto cmp_for_expansion = []( const cell_t &lhs, const cell_t &rhs ) {
            return lhs.visible() && lhs.gap() < rhs.gap();
        };
        // Should return true when lhs < rhs
        const auto cmp_for_shrinking = []( const cell_t &lhs, const cell_t &rhs ) {
            if( !lhs.visible() ) {
                return false;
            }
            if( rhs.gap() <= min_cell_gap ) {
                return lhs.current_width < rhs.current_width;
            } else {
                return lhs.gap() < rhs.gap();
            }
        };

        const auto &cell = step > 0
                           ? std::min_element( cells.begin(), cells.end(), cmp_for_expansion )
                           : std::max_element( cells.begin(), cells.end(), cmp_for_shrinking );

        if( cell == cells.end() || !cell->visible() ) {
            break; // This is highly unlikely to happen, but just in case
        }

        cell->current_width += step;
        width_gap += step;
    }
    reserved_width = new_width;
}

void inventory_column::set_height( size_t new_height )
{
    if( height != new_height ) {
        if( new_height <= 1 ) {
            debugmsg( "Unable to assign height <= 1 (was %zd).", new_height );
            return;
        }
        height = new_height;
        entries_per_page = new_height;
        paging_is_valid = false;
    }
}

void inventory_column::expand_to_fit( const inventory_entry &entry )
{
    if( !entry ) {
        return;
    }

    // Don't use cell cache here since the entry may not yet be placed into the vector of entries.
    const std::string denial = preset.get_denial( entry );

    for( size_t i = 0, num = denial.empty() ? cells.size() : 1; i < num; ++i ) {
        auto &cell = cells[i];

        cell.real_width = std::max( cell.real_width, get_entry_cell_width( entry, i ) );

        // Don't reveal the cell for headers and stubs
        if( cell.visible() || ( entry.is_item() && !preset.is_stub_cell( entry, i ) ) ) {
            const size_t cell_gap = i > 0 ? normal_cell_gap : 0;
            cell.current_width = std::max( cell.current_width, cell_gap + cell.real_width );
        }
    }

    if( !denial.empty() ) {
        reserved_width = std::max( get_entry_cell_width( entry, 0 ) + min_denial_gap + utf8_width( denial,
                                   true ),
                                   reserved_width );
    }
}

void inventory_column::reset_width( const std::vector<inventory_column *> & )
{
    for( auto &elem : cells ) {
        elem = cell_t();
    }
    reserved_width = 0;
    for( auto &elem : entries ) {
        expand_to_fit( elem );
    }
}

size_t inventory_column::page_of( size_t index ) const
{
    cata_assert( entries_per_page ); // To appease static analysis
    // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
    return index / entries_per_page;
}

size_t inventory_column::page_of( const inventory_entry &entry ) const
{
    return page_of( std::distance( entries.begin(), std::find( entries.begin(), entries.end(),
                                   entry ) ) );
}
bool inventory_column::has_available_choices() const
{
    if( !allows_selecting() || !activatable() ) {
        return false;
    }
    for( size_t i = 0; i < entries.size(); ++i ) {
        if( entries[i].is_item() && get_entry_cell_cache( i ).denial.empty() ) {
            return true;
        }
    }
    return false;
}

bool inventory_column::is_selected( const inventory_entry &entry ) const
{
    return entry == get_selected() || ( multiselect && is_selected_by_category( entry ) );
}

bool inventory_column::is_selected_by_category( const inventory_entry &entry ) const
{
    return entry.is_item() && mode == navigation_mode::CATEGORY
           && entry.get_category_ptr() == get_selected().get_category_ptr()
           && page_of( entry ) == page_index();
}

const inventory_entry &inventory_column::get_selected() const
{
    if( selected_index >= entries.size() || !entries[selected_index].is_item() ) {
        // clang complains if we use the default constructor here
        static const inventory_entry dummy( nullptr );
        return dummy;
    }
    return entries[selected_index];
}

inventory_entry &inventory_column::get_selected()
{
    return const_cast<inventory_entry &>( const_cast<const inventory_column *>
                                          ( this )->get_selected() );
}

std::vector<inventory_entry *> inventory_column::get_all_selected() const
{
    const auto filter_to_selected = [&]( const inventory_entry & entry ) {
        return is_selected( entry );
    };
    return get_entries( filter_to_selected );
}

std::vector<inventory_entry *> inventory_column::get_entries(
    const std::function<bool( const inventory_entry &entry )> &filter_func ) const
{
    std::vector<inventory_entry *> res;

    if( allows_selecting() ) {
        for( const auto &elem : entries ) {
            if( filter_func( elem ) ) {
                res.push_back( const_cast<inventory_entry *>( &elem ) );
            }
        }
    }

    return res;
}

void inventory_column::set_stack_favorite( std::vector<item_location> &locations,
        const bool favorite )
{
    for( item_location &loc : locations ) {
        loc->set_favorite( favorite );
    }
}

void inventory_column::on_input( const inventory_input &input )
{

    if( !empty() && active ) {
        if( input.action == "DOWN" ) {
            move_selection( scroll_direction::FORWARD );
        } else if( input.action == "UP" ) {
            move_selection( scroll_direction::BACKWARD );
        } else if( input.action == "PAGE_DOWN" ) {
            move_selection_page( scroll_direction::FORWARD );
        } else if( input.action == "PAGE_UP" ) {
            move_selection_page( scroll_direction::BACKWARD );
        } else if( input.action == "HOME" ) {
            select( 0, scroll_direction::FORWARD );
        } else if( input.action == "END" ) {
            select( entries.size() - 1, scroll_direction::BACKWARD );
        } else if( input.action == "TOGGLE_FAVORITE" ) {
            inventory_entry &selected = get_selected();
            set_stack_favorite( selected.locations, !selected.any_item()->is_favorite );
        }
    }

    if( input.action == "TOGGLE_FAVORITE" ) {
        // Favoriting items in one column may change item names in another column
        // if that column contains an item that contains the favorited item. So
        // we invalidate every column on TOGGLE_FAVORITE action.
        paging_is_valid = false;
    }
}

void inventory_column::on_change( const inventory_entry &entry )
{
    if( !entry.locations.empty() ) {
        // copy changes to the unfiltered entry
        const auto unfiltered_it = std::find_if( entries_unfiltered.begin(), entries_unfiltered.end(),
        [&]( const inventory_entry & unfiltered_entry ) {
            return unfiltered_entry.locations == entry.locations;
        } );
        if( unfiltered_it != entries_unfiltered.end() ) {
            *unfiltered_it = entry;
        }
    }
}

void inventory_column::order_by_parent()
{
    std::unordered_map<std::uintptr_t, size_t> original_order;
    original_order.reserve( entries.size() );
    for( size_t idx = 0; idx < entries.size(); ++idx ) {
        if( entries[idx].is_item() ) {
            for( const item_location &loc : entries[idx].locations ) {
                original_order.emplace( reinterpret_cast<std::uintptr_t>( &*loc ), idx );
            }
        } else {
            original_order.emplace( reinterpret_cast<std::uintptr_t>( &entries[idx] ), idx );
        }
    }

    struct entry_info {
        inventory_entry entry;
        std::vector<size_t> recursive_order;

        entry_info( inventory_entry &&moved_entry,
                    const std::unordered_map<std::uintptr_t, size_t> &original_order )
            : entry( std::move( moved_entry ) ) {
            if( entry.is_item() ) {
                item_location loc = entry.any_item();
                while( true ) {
                    const std::uintptr_t uintptr = reinterpret_cast<std::uintptr_t>( &*loc );
                    const auto it = original_order.find( uintptr );
                    if( it != original_order.end() ) {
                        recursive_order.emplace_back( it->second );
                    }
                    if( loc.has_parent() ) {
                        loc = loc.parent_item();
                    } else {
                        break;
                    }
                }
                std::reverse( recursive_order.begin(), recursive_order.end() );
            } else {
                const std::uintptr_t uintptr = reinterpret_cast<std::uintptr_t>( &moved_entry );
                const auto it = original_order.find( uintptr );
                if( it != original_order.end() ) {
                    recursive_order.emplace_back( it->second );
                }
            }
        }

        // NOLINTNEXTLINE(google-explicit-constructor)
        operator inventory_entry &&() && { // *NOPAD*
            return std::move( entry );
        }

        bool operator<( const entry_info &rhs ) const {
            return recursive_order < rhs.recursive_order;
        }
    };

    std::vector<entry_info> sorted_entries;
    sorted_entries.reserve( entries.size() );
    for( inventory_entry &entry : entries ) {
        sorted_entries.emplace_back( std::move( entry ), original_order );
    }
    std::stable_sort( sorted_entries.begin(), sorted_entries.end() );

    entries.assign( std::make_move_iterator( sorted_entries.begin() ),
                    std::make_move_iterator( sorted_entries.end() ) );
}

void inventory_column::add_entry( const inventory_entry &entry )
{
    if( std::find( entries.begin(), entries.end(), entry ) != entries.end() ) {
        debugmsg( "Tried to add a duplicate entry." );
        return;
    }
    const auto iter = std::find_if( entries.rbegin(), entries.rend(),
    [ &entry ]( const inventory_entry & cur ) {
        const item_category *cur_cat = cur.get_category_ptr();
        const item_category *new_cat = entry.get_category_ptr();

        return cur_cat == new_cat || ( cur_cat != nullptr && new_cat != nullptr
                                       && ( *cur_cat == *new_cat || *cur_cat < *new_cat ) );
    } );
    bool has_loc = false;
    if( entry.is_item() && entry.locations.front().where() == item_location::type::container ) {
        item_location entry_item = entry.locations.front();

        auto entry_with_loc = std::find_if( entries.begin(),
        entries.end(), [&entry_item]( const inventory_entry & entry ) {
            if( !entry.is_item() ) {
                return false;
            }
            item_location found_entry_item = entry.locations.front();
            return found_entry_item.where() == item_location::type::container &&
                   entry_item->display_stacked_with( *found_entry_item ) &&
                   found_entry_item.has_parent() &&
                   entry_item.parent_item() == found_entry_item.parent_item();
        } );
        if( entry_with_loc != entries.end() ) {
            has_loc = true;
            std::vector<item_location> locations = entry_with_loc->locations;
            locations.insert( locations.end(), entry.locations.begin(), entry.locations.end() );
            entries.erase( entry_with_loc );
            inventory_entry nentry( locations, entry.get_category_ptr() );
            add_entry( nentry );
        }
    }
    if( !has_loc ) {
        entries.insert( iter.base(), entry );
    }
    entries_unfiltered.clear();
    entries_cell_cache.clear();
    expand_to_fit( entry );
    paging_is_valid = false;
}

void inventory_column::move_entries_to( inventory_column &dest )
{
    for( const auto &elem : entries ) {
        if( elem.is_item() &&
            // this column already has this entry, no need to try to add it again
            std::find( dest.entries.begin(), dest.entries.end(), elem ) == dest.entries.end() ) {
            dest.add_entry( elem );
        }
    }
    dest.prepare_paging();
    clear();
}

void inventory_column::prepare_paging( const std::string &filter )
{
    if( paging_is_valid ) {
        return;
    }

    const auto filter_fn = filter_from_string<inventory_entry>(
    filter, [this]( const std::string & filter ) {
        return preset.get_filter( filter );
    } );

    // First, remove all non-items
    const auto new_end = std::remove_if( entries.begin(),
    entries.end(), [&filter_fn]( const inventory_entry & entry ) {
        return !entry.is_item() || !filter_fn( entry );
    } );
    entries.erase( new_end, entries.end() );
    // Then sort them with respect to categories (sort only once each UI session)
    if( entries_unfiltered.empty() ) {
        auto from = entries.begin();
        while( from != entries.end() ) {
            from->update_cache();
            auto to = std::next( from );
            while( to != entries.end() && from->get_category_ptr() == to->get_category_ptr() ) {
                to->update_cache();
                std::advance( to, 1 );
            }
            if( ordered_categories.count( from->get_category_ptr()->get_id().c_str() ) == 0 ) {
                std::stable_sort( from, to, [ this ]( const inventory_entry & lhs, const inventory_entry & rhs ) {
                    if( lhs.is_selectable() != rhs.is_selectable() ) {
                        return lhs.is_selectable(); // Disabled items always go last
                    }
                    Character &player_character = get_player_character();
                    // Place favorite items and items with an assigned inventory letter first,
                    // since the player cared enough to assign them
                    const bool left_has_invlet = player_character.inv->assigned_invlet.count( lhs.any_item()->invlet );
                    const bool right_has_invlet = player_character.inv->assigned_invlet.count( rhs.any_item()->invlet );
                    if( left_has_invlet != right_has_invlet ) {
                        return left_has_invlet;
                    }
                    const bool left_fav = lhs.any_item()->is_favorite;
                    const bool right_fav = rhs.any_item()->is_favorite;
                    if( left_fav != right_fav ) {
                        return left_fav;
                    }
                    return preset.sort_compare( lhs, rhs );
                } );
            }
            from = to;
        }
    }
    // Recover categories
    const item_category *current_category = nullptr;
    for( auto iter = entries.begin(); iter != entries.end(); ++iter ) {
        if( iter->get_category_ptr() == current_category ) {
            continue;
        }
        current_category = iter->get_category_ptr();
        iter = entries.insert( iter, inventory_entry( current_category ) );
        expand_to_fit( *iter );
    }
    // Determine the new height.
    entries_per_page = height;
    if( entries.size() > entries_per_page ) {
        entries_per_page -= 1;  // Make room for the page number.
        for( size_t i = entries_per_page - 1; i < entries.size(); i += entries_per_page ) {
            auto iter = std::next( entries.begin(), i );
            if( iter->is_category() ) {
                // The last item on the page must not be a category.
                entries.insert( iter, inventory_entry() );
            } else {
                // The first item on the next page must be a category.
                iter = std::next( iter );
                if( iter != entries.end() && iter->is_item() ) {
                    entries.insert( iter, inventory_entry( iter->get_category_ptr() ) );
                }
            }
        }
    }
    entries_cell_cache.clear();
    paging_is_valid = true;
    if( entries_unfiltered.empty() ) {
        entries_unfiltered = entries;
    }
    // Select the uppermost possible entry
    const size_t ind = selected_index >= entries.size() ? 0 : selected_index;
    select( ind, ind ? scroll_direction::BACKWARD : scroll_direction::FORWARD );
}

void inventory_column::clear()
{
    entries.clear();
    entries_unfiltered.clear();
    entries_cell_cache.clear();
    paging_is_valid = false;
}

bool inventory_column::select( const item_location &loc )
{
    for( size_t index = 0; index < entries.size(); ++index ) {
        if( entries[index].is_selectable()
            && std::find( entries[index].locations.begin(),
                          entries[index].locations.end(),
                          loc ) != entries[index].locations.end() ) {
            select( index, scroll_direction::FORWARD );
            return true;
        }
    }
    return false;
}

size_t inventory_column::get_entry_indent( const inventory_entry &entry ) const
{
    if( !entry.is_item() ) {
        return 0;
    }

    size_t res = 2;
    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
        res += 2;
    }
    if( allows_selecting() && activatable() && multiselect ) {
        res += 2;
    }
    return res;
}

int inventory_column::reassign_custom_invlets( const Character &p, int min_invlet, int max_invlet )
{
    int cur_invlet = min_invlet;
    for( auto &elem : entries ) {
        // Only items on map/in vehicles: those that the player does not possess.
        if( elem.is_selectable() && !p.has_item( *elem.any_item() ) ) {
            elem.custom_invlet = cur_invlet <= max_invlet ? cur_invlet++ : '\0';
        }
    }
    return cur_invlet;
}

static int num_parents( const item_location &loc )
{
    if( loc.where() != item_location::type::container ) {
        return 0;
    }
    return 2 + num_parents( loc.parent_item() );
}

void inventory_column::draw( const catacurses::window &win, const point &p,
                             std::vector<std::pair<inclusive_rectangle<point>, inventory_entry *>> &rect_entry_map )
{
    if( !visible() ) {
        return;
    }
    const auto available_cell_width = [ this ]( size_t index, size_t cell_index ) {
        const size_t displayed_width = cells[cell_index].current_width;
        const size_t real_width = get_entry_cell_width( index, cell_index );

        return displayed_width > real_width ? displayed_width - real_width : 0;
    };

    // Do the actual drawing
    for( size_t index = page_offset, line = 0; index < entries.size() &&
         line < entries_per_page; ++index, ++line ) {
        inventory_entry &entry = entries[index];
        const inventory_column::entry_cell_cache_t &entry_cell_cache = get_entry_cell_cache( index );

        if( !entry ) {
            continue;
        }

        int contained_offset = 0;
        if( entry.is_item() && indent_entries() ) {
            // indent items that are contained
            contained_offset = num_parents( entry.locations.front() );
        }

        int x1 = p.x + get_entry_indent( entry ) + contained_offset;
        int x2 = p.x + std::max( static_cast<int>( reserved_width - get_cells_width() ), 0 );
        int yy = p.y + line;

        const bool selected = active && is_selected( entry );

        const int hx_max = p.x + get_width() + contained_offset;
        inclusive_rectangle<point> rect = inclusive_rectangle<point>( point( x1, yy ),
                                          point( hx_max - 1, yy ) );
        rect_entry_map.emplace_back( rect,
                                     &entry );

        if( selected && visible_cells() > 1 ) {
            for( int hx = x1; hx < hx_max; ++hx ) {
                mvwputch( win, point( hx, yy ), h_white, ' ' );
            }
        }

        const std::string &denial = entry_cell_cache.denial;

        if( !denial.empty() ) {
            const size_t max_denial_width = std::max( static_cast<int>( get_width() - ( min_denial_gap +
                                            get_entry_cell_width( index, 0 ) ) ), 0 );
            const size_t denial_width = std::min( max_denial_width, static_cast<size_t>( utf8_width( denial,
                                                  true ) ) );

            trim_and_print( win, point( p.x + get_width() - denial_width, yy ),
                            denial_width,
                            c_red, denial );
        }

        const size_t count = denial.empty() ? cells.size() : 1;

        for( size_t cell_index = 0; cell_index < count; ++cell_index ) {
            if( !cells[cell_index].visible() ) {
                continue; // Don't show empty cells
            }

            if( line != 0 && cell_index != 0 && entry.is_category() ) {
                break; // Don't show duplicated titles
            }

            x2 += cells[cell_index].current_width;

            size_t text_width = utf8_width( entry_cell_cache.text[cell_index], true );
            size_t text_gap = cell_index > 0 ? std::max( cells[cell_index].gap(), min_cell_gap ) : 0;
            size_t available_width = x2 - x1 - text_gap;

            if( text_width > available_width ) {
                // See if we can steal some of the needed width from an adjacent cell
                if( cell_index == 0 && count >= 2 ) {
                    available_width += available_cell_width( index, 1 );
                } else if( cell_index > 0 ) {
                    available_width += available_cell_width( index, cell_index - 1 );
                }
                text_width = std::min( text_width, available_width );
            }

            if( text_width > 0 ) {
                const int text_x = cell_index == 0 ? x1 : x2 -
                                   text_width; // Align either to the left or to the right
                const std::string &text = entry_cell_cache.text[cell_index];
                const std::string &hl_option = get_option<std::string>( "INVENTORY_HIGHLIGHT" );

                if( entry.is_item() && ( selected || !entry.is_selectable() ) ) {
                    trim_and_print( win, point( text_x, yy ), text_width, selected ? h_white : c_dark_gray,
                                    remove_color_tags( text ) );
                } else if( entry.is_item() && entry.highlight_as_parent ) {
                    if( hl_option == "symbol" ) {
                        trim_and_print( win, point( text_x - 1, yy ), 1, h_white, "<" );
                        trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                    } else {
                        trim_and_print( win, point( text_x, yy ), text_width, c_white_white,
                                        remove_color_tags( text ) );
                    }
                    entry.highlight_as_parent = false;
                } else if( entry.is_item() && entry.highlight_as_child ) {
                    if( hl_option == "symbol" ) {
                        trim_and_print( win, point( text_x - 1, yy ), 1, h_white, ">" );
                        trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                    } else {
                        trim_and_print( win, point( text_x, yy ), text_width, c_black_white,
                                        remove_color_tags( text ) );
                    }
                    entry.highlight_as_child = false;
                } else {
                    trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                }
            }

            x1 = x2;
        }

        if( entry.is_item() ) {
            int xx = p.x;
            if( entry.get_invlet() != '\0' ) {
                mvwputch( win, point( p.x, yy ), entry.get_invlet_color(), entry.get_invlet() );
            }
            xx += 2;
            if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
                const nc_color color = entry.any_item()->color();
                mvwputch( win, point( xx, yy ), color, entry.any_item()->symbol() );
                xx += 2;
            }
            if( allows_selecting() && activatable() && multiselect ) {
                if( entry.chosen_count == 0 ) {
                    mvwputch( win, point( xx, yy ), c_dark_gray, '-' );
                } else if( entry.chosen_count >= entry.get_available_count() ) {
                    mvwputch( win, point( xx, yy ), c_light_green, '+' );
                } else {
                    mvwputch( win, point( xx, yy ), c_light_green, '#' );
                }
            }
        }
    }

    if( pages_count() > 1 ) {
        mvwprintw( win, p + point( 0, height - 1 ), _( "Page %d/%d" ), page_index() + 1, pages_count() );
    }
}

size_t inventory_column::visible_cells() const
{
    return std::count_if( cells.begin(), cells.end(), []( const cell_t &elem ) {
        return elem.visible();
    } );
}

selection_column::selection_column( const std::string &id, const std::string &name ) :
    inventory_column( selection_preset ),
    selected_cat( id, no_translation( name ), 0 ) {}

selection_column::~selection_column() = default;

void selection_column::reset_width( const std::vector<inventory_column *> &all_columns )
{
    inventory_column::reset_width( all_columns );

    const auto always_yes = []( const inventory_entry & ) {
        return true;
    };

    for( const inventory_column *const col : all_columns ) {
        if( col && !dynamic_cast<const selection_column *>( col ) ) {
            for( const inventory_entry *const ent : col->get_entries( always_yes ) ) {
                if( ent ) {
                    expand_to_fit( *ent );
                }
            }
        }
    }
}

void selection_column::prepare_paging( const std::string & )
{
    // always show all selected items
    inventory_column::prepare_paging( std::string() );

    if( entries.empty() ) { // Category must always persist
        entries.emplace_back( &*selected_cat );
        expand_to_fit( entries.back() );
    }

    if( !last_changed.is_null() ) {
        const auto iter = std::find( entries.begin(), entries.end(), last_changed );
        if( iter != entries.end() ) {
            select( std::distance( entries.begin(), iter ), scroll_direction::FORWARD );
        }
        last_changed = inventory_entry();
    }
}

void selection_column::on_change( const inventory_entry &entry )
{
    inventory_entry my_entry( entry, &*selected_cat );

    auto iter = std::find( entries.begin(), entries.end(), my_entry );

    if( iter == entries.end() ) {
        if( my_entry.chosen_count == 0 ) {
            return; // Not interested.
        }
        add_entry( my_entry );
        prepare_paging();
        last_changed = my_entry;
    } else if( iter->chosen_count != my_entry.chosen_count ) {
        if( my_entry.chosen_count > 0 ) {
            iter->chosen_count = my_entry.chosen_count;
            expand_to_fit( my_entry );
        } else {
            iter = entries.erase( iter );
        }
        entries_unfiltered.clear();
        paging_is_valid = false;
        prepare_paging();
        if( iter != entries.end() ) {
            last_changed = *iter;
        }
    }
}

// TODO: Move it into some 'item_stack' class.
static std::vector<std::list<item *>> restack_items( const std::list<item>::const_iterator &from,
                                   const std::list<item>::const_iterator &to, bool check_components = false )
{
    std::vector<std::list<item *>> res;

    for( auto it = from; it != to; ++it ) {
        auto match = std::find_if( res.begin(), res.end(),
        [ &it, check_components ]( const std::list<item *> &e ) {
            return it->display_stacked_with( *const_cast<item *>( e.back() ), check_components );
        } );

        if( match != res.end() ) {
            match->push_back( const_cast<item *>( &*it ) );
        } else {
            res.emplace_back( 1, const_cast<item *>( &*it ) );
        }
    }

    return res;
}

// TODO: Move it into some 'item_stack' class.
static std::vector<std::list<item *>> restack_items( const item_stack::const_iterator &from,
                                   const item_stack::const_iterator &to, bool check_components = false )
{
    std::vector<std::list<item *>> res;

    for( auto it = from; it != to; ++it ) {
        auto match = std::find_if( res.begin(), res.end(),
        [ &it, check_components ]( const std::list<item *> &e ) {
            return it->display_stacked_with( *const_cast<item *>( e.back() ), check_components );
        } );

        if( match != res.end() ) {
            match->push_back( const_cast<item *>( &*it ) );
        } else {
            res.emplace_back( 1, const_cast<item *>( &*it ) );
        }
    }

    return res;
}

const item_category *inventory_selector::naturalize_category( const item_category &category,
        const tripoint &pos )
{
    const auto find_cat_by_id = [ this ]( const item_category_id & id ) {
        const auto iter = std::find_if( categories.begin(),
        categories.end(), [ &id ]( const item_category & cat ) {
            return cat.get_id() == id;
        } );
        return iter != categories.end() ? &*iter : nullptr;
    };

    const int dist = rl_dist( u.pos(), pos );

    if( dist != 0 ) {
        const std::string suffix = direction_suffix( u.pos(), pos );
        const item_category_id id = item_category_id( string_format( "%s_%s", category.get_id().c_str(),
                                    suffix.c_str() ) );

        const auto *existing = find_cat_by_id( id );
        if( existing != nullptr ) {
            return existing;
        }

        const std::string name = string_format( "%s %s", category.name(), suffix.c_str() );
        const int sort_rank = category.sort_rank() + dist;
        const item_category new_category( id, no_translation( name ), sort_rank );

        categories.push_back( new_category );
    } else {
        const item_category *const existing = find_cat_by_id( category.get_id() );
        if( existing != nullptr ) {
            return existing;
        }

        categories.push_back( category );
    }

    return &categories.back();
}

void inventory_selector::add_entry( inventory_column &target_column,
                                    std::vector<item_location> &&locations,
                                    const item_category *custom_category,
                                    const size_t chosen_count )
{
    if( !preset.is_shown( locations.front() ) ) {
        return;
    }

    is_empty = false;
    inventory_entry entry( locations, custom_category,
                           preset.get_denial( locations.front() ).empty(),
                           /*chosen_count=*/chosen_count );

    target_column.add_entry( entry );

    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }
}

void inventory_selector::add_item( inventory_column &target_column,
                                   item_location &&location,
                                   const item_category *custom_category )
{
    add_entry( target_column,
               std::vector<item_location>( 1, location ),
               custom_category );
    for( item *it : location->all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
        add_item( target_column, item_location( location, it ), custom_category );
    }
}

void inventory_selector::add_items( inventory_column &target_column,
                                    const std::function<item_location( item * )> &locator,
                                    const std::vector<std::list<item *>> &stacks,
                                    const item_category *custom_category )
{
    const item_category *nat_category = nullptr;

    for( const auto &elem : stacks ) {
        std::vector<item_location> locations;
        std::transform( elem.begin(), elem.end(), std::back_inserter( locations ), locator );
        item_location const &loc = locations.front();

        if( custom_category == nullptr ) {
            nat_category = &loc->get_category_of_contents();
        } else if( nat_category == nullptr && preset.is_shown( loc ) ) {
            nat_category = naturalize_category( *custom_category, loc.position() );
        }

        add_entry( target_column, std::move( locations ), nat_category );
    }
}

void inventory_selector::add_contained_items( item_location &container )
{
    add_contained_items( container, own_inv_column );
}

void inventory_selector::add_contained_items( item_location &container, inventory_column &column,
        const item_category *const custom_category )
{
    if( container->has_flag( STATIC( flag_id( "NO_UNLOAD" ) ) ) ) {
        return;
    }

    for( item *it : container->all_items_top() ) {
        item_location child( container, it );
        add_contained_items( child, column, custom_category );
        const item_category *nat_category = nullptr;
        if( custom_category == nullptr ) {
            nat_category = &child->get_category_of_contents();
        } else if( preset.is_shown( child ) ) {
            nat_category = naturalize_category( *custom_category, child.position() );
        }
        add_entry( column, std::vector<item_location>( 1, child ), nat_category );
    }
}

void inventory_selector::add_character_items( Character &character )
{
    character.visit_items( [ this, &character ]( item * it, item * ) {
        if( it == &character.weapon ) {
            add_item( own_gear_column, item_location( character, it ),
                      &item_category_id( "WEAPON_HELD" ).obj() );
        } else if( character.is_worn( *it ) ) {
            add_item( own_gear_column, item_location( character, it ),
                      &item_category_id( "ITEMS_WORN" ).obj() );
        }
        return VisitResponse::NEXT;
    } );
    // Visitable interface does not support stacks so it has to be here
    for( std::list<item> *elem : character.inv->slice() ) {
        add_items( own_inv_column, [&character]( item * it ) {
            return item_location( character, it );
        }, restack_items( ( *elem ).begin(), ( *elem ).end(), preset.get_checking_components() ),
        &item_category_id( "ITEMS_WORN" ).obj() );
        for( item &it_elem : *elem ) {
            item_location parent( character, &it_elem );
            add_contained_items( parent, own_inv_column,
                                 &item_category_id( "ITEMS_WORN" ).obj() );
        }
    }
    // this is a little trick; we want the default behavior for contained items to be in own_inv_column
    // and this function iterates over all the entries after we added them to the inventory selector
    // to put them in the right place
    toggle_categorize_contained();
}

void inventory_selector::add_map_items( const tripoint &target )
{
    map &here = get_map();
    if( here.accessible_items( target ) ) {
        map_stack items = here.i_at( target );
        const std::string name = to_upper_case( here.name( target ) );
        const item_category map_cat( name, no_translation( name ), 100 );

        add_items( map_column, [ &target ]( item * it ) {
            return item_location( map_cursor( target ), it );
        }, restack_items( items.begin(), items.end(), preset.get_checking_components() ), &map_cat );

        for( item &it_elem : items ) {
            item_location parent( map_cursor( target ), &it_elem );
            add_contained_items( parent, map_column, &map_cat );
        }
    }
}

void inventory_selector::add_vehicle_items( const tripoint &target )
{
    const cata::optional<vpart_reference> vp =
        get_map().veh_at( target ).part_with_feature( "CARGO", true );
    if( !vp ) {
        return;
    }
    vehicle *const veh = &vp->vehicle();
    const int part = vp->part_index();
    vehicle_stack items = veh->get_items( part );
    const std::string name = to_upper_case( remove_color_tags( veh->part( part ).name() ) );
    const item_category vehicle_cat( name, no_translation( name ), 200 );

    const bool check_components = this->preset.get_checking_components();

    add_items( map_column, [ veh, part ]( item * it ) {
        return item_location( vehicle_cursor( *veh, part ), it );
    }, restack_items( items.begin(), items.end(), check_components ), &vehicle_cat );

    for( item &it_elem : items ) {
        item_location parent( vehicle_cursor( *veh, part ), &it_elem );
        add_contained_items( parent, map_column, &vehicle_cat );
    }
}

void inventory_selector::add_nearby_items( int radius )
{
    if( radius >= 0 ) {
        map &here = get_map();
        for( const tripoint &pos : closest_points_first( u.pos(), radius ) ) {
            // can not reach this -> can not access its contents
            if( u.pos() != pos && !here.clear_path( u.pos(), pos, rl_dist( u.pos(), pos ), 1, 100 ) ) {
                continue;
            }
            add_map_items( pos );
            add_vehicle_items( pos );
        }
    }
}

void inventory_selector::clear_items()
{
    is_empty = true;
    for( auto &column : columns ) {
        column->clear();
    }
    own_inv_column.clear();
    own_gear_column.clear();
    map_column.clear();
}

bool inventory_selector::select( const item_location &loc )
{
    bool res = false;

    for( size_t i = 0; i < columns.size(); ++i ) {
        inventory_column *elem = columns[i];
        if( elem->visible() && elem->select( loc ) ) {
            if( !res && elem->activatable() ) {
                set_active_column( i );
                res = true;
            }
        }
    }

    return res;
}

bool inventory_selector::select_one_of( const std::vector<item_location> &locations )
{
    prepare_layout();
    for( const item_location &loc : locations ) {
        if( select( loc ) ) {
            return true;
        }
    }
    return false;
}

inventory_entry *inventory_selector::find_entry_by_invlet( int invlet ) const
{
    for( const inventory_column *elem : columns ) {
        inventory_entry *const res = elem->find_by_invlet( invlet );
        if( res != nullptr ) {
            return res;
        }
    }
    return nullptr;
}

inventory_entry *inventory_selector::find_entry_by_coordinate( const point &coordinate ) const
{
    for( auto pair : rect_entry_map ) {
        if( pair.first.contains( coordinate ) ) {
            return pair.second;
        }
    }
    return nullptr;
}

// FIXME: if columns are merged due to low screen width, they will not be splitted
// once screen width becomes enough for the columns.
void inventory_selector::rearrange_columns( size_t client_width )
{
    const inventory_entry &prev_entry = get_selected();
    const item_location prev_selection = prev_entry.is_item() ?
                                         prev_entry.any_item() : item_location::nowhere;
    while( is_overflown( client_width ) ) {
        if( !own_gear_column.empty() ) {
            own_gear_column.move_entries_to( own_inv_column );
        } else if( !map_column.empty() ) {
            map_column.move_entries_to( own_inv_column );
        } else {
            break;  // There's nothing we can do about it.
        }
    }
    if( prev_selection ) {
        select( prev_selection );
    }
}

void inventory_selector::prepare_layout( size_t client_width, size_t client_height )
{
    // This block adds categories and should go before any width evaluations
    const bool initial = get_active_column().get_selected_index() == static_cast<size_t>( -1 );
    for( auto &elem : columns ) {
        elem->set_height( client_height );
        elem->reset_width( columns );
        elem->prepare_paging( filter );
    }
    // Handle screen overflow
    rearrange_columns( client_width );
    if( initial ) {
        get_active_column().select( 0, scroll_direction::FORWARD );
    }
    // If we have a single column and it occupies more than a half of
    // the available with -> expand it
    auto visible_columns = get_visible_columns();
    if( visible_columns.size() == 1 && are_columns_centered( client_width ) ) {
        visible_columns.front()->set_width( client_width, columns );
    }

    int custom_invlet = '0';
    for( auto &elem : columns ) {
        elem->prepare_paging();
        custom_invlet = elem->reassign_custom_invlets( u, custom_invlet, '9' );
    }

    refresh_active_column();
}

void inventory_selector::prepare_layout()
{
    const auto snap = []( size_t cur_dim, size_t max_dim ) {
        return cur_dim + 2 * max_win_snap_distance >= max_dim ? max_dim : cur_dim;
    };

    const int nc_width = 2 * ( 1 + border );
    const int nc_height = get_header_height() + 1 + 2 * border;

    prepare_layout( TERMX - nc_width, TERMY - nc_height );

    const int win_width  = snap( get_layout_width() + nc_width, TERMX );
    const int win_height = snap( std::max<int>( get_layout_height() + nc_height, FULL_SCREEN_HEIGHT ),
                                 TERMY );

    prepare_layout( win_width - nc_width, win_height - nc_height );

    resize_window( win_width, win_height );
}

shared_ptr_fast<ui_adaptor> inventory_selector::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & ) {
            prepare_layout();
        } );
        current_ui->mark_resize();

        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            refresh_window();
        } );
    }
    return current_ui;
}

size_t inventory_selector::get_layout_width() const
{
    const size_t min_hud_width = std::max( get_header_min_width(), get_footer_min_width() );
    const auto visible_columns = get_visible_columns();
    const size_t gaps = visible_columns.size() > 1 ? normal_column_gap * ( visible_columns.size() - 1 )
                        : 0;

    return std::max( get_columns_width( visible_columns ) + gaps, min_hud_width );
}

size_t inventory_selector::get_layout_height() const
{
    const auto visible_columns = get_visible_columns();
    // Find and return the highest column's height.
    const auto iter = std::max_element( visible_columns.begin(), visible_columns.end(),
    []( const inventory_column * lhs, const inventory_column * rhs ) {
        return lhs->get_height() < rhs->get_height();
    } );

    return iter != visible_columns.end() ? ( *iter )->get_height() : 1;
}

size_t inventory_selector::get_header_height() const
{
    return display_stats || !hint.empty() ? 3 : 1;
}

size_t inventory_selector::get_header_min_width() const
{
    const size_t titles_width = std::max( utf8_width( title, true ),
                                          utf8_width( hint, true ) );
    if( !display_stats ) {
        return titles_width;
    }

    size_t stats_width = 0;
    for( const std::string &elem : get_stats() ) {
        stats_width = std::max( static_cast<size_t>( utf8_width( elem, true ) ), stats_width );
    }

    return titles_width + stats_width + ( stats_width != 0 ? 3 : 0 );
}

size_t inventory_selector::get_footer_min_width() const
{
    size_t result = 0;
    navigation_mode m = mode;

    do {
        result = std::max( static_cast<size_t>( utf8_width( get_footer( m ).first, true ) ) + 2 + 4,
                           result );
        m = get_navigation_data( m ).next_mode;
    } while( m != mode );

    return result;
}

void inventory_selector::draw_header( const catacurses::window &w ) const
{
    trim_and_print( w, point( border + 1, border ), getmaxx( w ) - 2 * ( border + 1 ), c_white, title );
    fold_and_print( w, point( border + 1, border + 1 ), getmaxx( w ) - 2 * ( border + 1 ), c_dark_gray,
                    hint );

    mvwhline( w, point( border, border + get_header_height() ), LINE_OXOX, getmaxx( w ) - 2 * border );

    if( display_stats ) {
        size_t y = border;
        for( const std::string &elem : get_stats() ) {
            right_print( w, y++, border + 1, c_dark_gray, elem );
        }
    }
}

inventory_selector::stat display_stat( const std::string &caption, int cur_value, int max_value,
                                       const std::function<std::string( int )> &disp_func )
{
    const nc_color color = cur_value > max_value ? c_red : c_light_gray;
    return {{
            caption,
            colorize( disp_func( cur_value ), color ), "/",
            colorize( disp_func( max_value ), c_light_gray )
        }};
}

inventory_selector::stats inventory_selector::get_weight_and_volume_stats(
    units::mass weight_carried, units::mass weight_capacity,
    const units::volume &volume_carried, const units::volume &volume_capacity,
    const units::length &longest_length, const units::volume &largest_free_volume )
{
    // This is a bit of a hack, we're prepending two entries to the weight and length stat blocks.
    std::string length_weight_caption = string_format( _( "Longest Length (%s): %s Weight (%s):" ),
                                        length_units( longest_length ),
                                        colorize( std::to_string( convert_length( longest_length ) ), c_light_gray ), weight_units() );
    std::string volume_caption = string_format( _( "Free Volume (%s): %s Volume (%s):" ),
                                 volume_units_abbr(),
                                 colorize( format_volume( largest_free_volume ), c_light_gray ),
                                 volume_units_abbr() );
    return {
        {
            display_stat( length_weight_caption,
                          to_gram( weight_carried ),
                          to_gram( weight_capacity ), []( int w )
            {
                return string_format( "%.1f", round_up( convert_weight( units::from_gram( w ) ), 1 ) );
            } ),
            display_stat( volume_caption,
                          units::to_milliliter( volume_carried ),
                          units::to_milliliter( volume_capacity ), []( int v )
            {
                return format_volume( units::from_milliliter( v ) );
            } )
        }
    };
}

inventory_selector::stats inventory_selector::get_raw_stats() const
{
    return get_weight_and_volume_stats( u.weight_carried(), u.weight_capacity(),
                                        u.volume_carried(), u.volume_capacity(),
                                        u.max_single_item_length(), u.max_single_item_volume() );
}

std::vector<std::string> inventory_selector::get_stats() const
{
    // Stats consist of arrays of cells.
    const size_t num_stats = 2;
    const std::array<stat, num_stats> stats = get_raw_stats();
    // Streams for every stat.
    std::array<std::string, num_stats> lines;
    std::array<size_t, num_stats> widths;
    // Add first cells and spaces after them.
    for( size_t i = 0; i < stats.size(); ++i ) {
        lines[i] += string_format( "%s", stats[i][0] ) + " ";
    }
    // Now add the rest of the cells and align them to the right.
    for( size_t j = 1; j < stats.front().size(); ++j ) {
        // Calculate actual cell width for each stat.
        std::transform( stats.begin(), stats.end(), widths.begin(),
        [j]( const stat & elem ) {
            return utf8_width( elem[j], true );
        } );
        // Determine the max width.
        const size_t max_w = *std::max_element( widths.begin(), widths.end() );
        // Align all stats in this cell with spaces.
        for( size_t i = 0; i < stats.size(); ++i ) {
            if( max_w > widths[i] ) {
                lines[i] += std::string( max_w - widths[i], ' ' );
            }
            lines[i] += string_format( "%s", stats[i][j] );
        }
    }
    // Construct the final result.
    return std::vector<std::string>( lines.begin(), lines.end() );
}

void inventory_selector::resize_window( int width, int height )
{
    w_inv = catacurses::newwin( height, width,
                                point( ( TERMX - width ) / 2, ( TERMY - height ) / 2 ) );
    if( spopup ) {
        spopup->window( w_inv, point( 4, getmaxy( w_inv ) - 1 ), ( getmaxx( w_inv ) / 2 ) - 4 );
    }
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->position_from_window( w_inv );
    }
}

void inventory_selector::refresh_window()
{
    cata_assert( w_inv );

    if( get_option<std::string>( "INVENTORY_HIGHLIGHT" ) != "disable" ) {
        highlight();
    }

    werase( w_inv );

    draw_frame( w_inv );
    draw_header( w_inv );
    draw_columns( w_inv );
    draw_footer( w_inv );

    wnoutrefresh( w_inv );
}

void inventory_selector::set_filter()
{
    spopup = std::make_unique<string_input_popup>();
    spopup->max_length( 256 )
    .text( filter );

    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }

    do {
        ui_manager::redraw();
        spopup->query_string( /*loop=*/false );
    } while( !spopup->confirmed() && !spopup->canceled() );

    if( spopup->confirmed() ) {
        filter = spopup->text();
        for( inventory_column *const elem : columns ) {
            elem->set_filter( filter );
        }
        if( current_ui ) {
            current_ui->mark_resize();
        }
    }

    spopup.reset();
}

void inventory_selector::set_filter( const std::string &str )
{
    prepare_layout();
    filter = str;
    for( inventory_column *const elem : columns ) {
        elem->set_filter( filter );
    }
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }
}

std::string inventory_selector::get_filter() const
{
    return filter;
}

void inventory_selector::draw_columns( const catacurses::window &w )
{
    const auto columns = get_visible_columns();

    const int screen_width = getmaxx( w ) - 2 * ( border + 1 );
    const bool centered = are_columns_centered( screen_width );

    const int free_space = screen_width - get_columns_width( columns );
    const int max_gap = ( columns.size() > 1 ) ? free_space / ( static_cast<int>
                        ( columns.size() ) - 1 ) :
                        free_space;
    const int gap = centered ? max_gap : std::min<int>( max_gap, normal_column_gap );
    const int gap_rounding_error = centered && columns.size() > 1
                                   ? free_space % ( columns.size() - 1 ) : 0;

    size_t x = border + 1;
    size_t y = get_header_height() + border + 1;
    size_t active_x = 0;

    rect_entry_map.clear();
    for( const auto &elem : columns ) {
        if( &elem == &columns.back() ) {
            x += gap_rounding_error;
        }
        if( !is_active_column( *elem ) ) {
            elem->draw( w, point( x, y ), rect_entry_map );
        } else {
            active_x = x;
        }
        x += elem->get_width() + gap;
    }

    get_active_column().draw( w, point( active_x, y ), rect_entry_map );
    if( empty() ) {
        center_print( w, getmaxy( w ) / 2, c_dark_gray, _( "Your inventory is empty." ) );
    }
}

void inventory_selector::draw_frame( const catacurses::window &w ) const
{
    draw_border( w );

    const int y = border + get_header_height();
    mvwhline( w, point( 0, y ), LINE_XXXO, 1 );
    mvwhline( w, point( getmaxx( w ) - border, y ), LINE_XOXX, 1 );
}

std::pair<std::string, nc_color> inventory_selector::get_footer( navigation_mode m ) const
{
    if( has_available_choices() ) {
        return std::make_pair( get_navigation_data( m ).name.translated(),
                               get_navigation_data( m ).color );
    }
    return std::make_pair( _( "There are no available choices" ), i_red );
}

void inventory_selector::draw_footer( const catacurses::window &w ) const
{
    if( spopup ) {
        mvwprintz( w_inv, point( 2, getmaxy( w_inv ) - 1 ), c_cyan, "< " );
        mvwprintz( w_inv, point( ( getmaxx( w_inv ) / 2 ) - 4, getmaxy( w_inv ) - 1 ), c_cyan, " >" );

        spopup->query_string( /*loop=*/false, /*draw_only=*/true );
    } else {
        int filter_offset = 0;
        if( has_available_choices() || !filter.empty() ) {
            std::string text = string_format( filter.empty() ? _( "[%s] Filter" ) : _( "[%s] Filter: " ),
                                              ctxt.get_desc( "INVENTORY_FILTER" ) );
            filter_offset = utf8_width( text + filter ) + 6;

            mvwprintz( w, point( 2, getmaxy( w ) - border ), c_light_gray, "< " );
            wprintz( w, c_light_gray, text );
            wprintz( w, c_white, filter );
            wprintz( w, c_light_gray, " >" );
        }

        const auto footer = get_footer( mode );
        if( !footer.first.empty() ) {
            const int string_width = utf8_width( footer.first );
            const int x1 = filter_offset + std::max( getmaxx( w ) - string_width - filter_offset, 0 ) / 2;
            const int x2 = x1 + string_width - 1;
            const int y = getmaxy( w ) - border;

            mvwprintz( w, point( x1, y ), footer.second, footer.first );
            mvwputch( w, point( x1 - 1, y ), c_light_gray, ' ' );
            mvwputch( w, point( x2 + 1, y ), c_light_gray, ' ' );
            mvwputch( w, point( x1 - 2, y ), c_light_gray, LINE_XOXX );
            mvwputch( w, point( x2 + 2, y ), c_light_gray, LINE_XXXO );
        }
    }
}

inventory_selector::inventory_selector( Character &u, const inventory_selector_preset &preset )
    : u( u )
    , preset( preset )
    , ctxt( "INVENTORY", keyboard_mode::keychar )
    , active_column_index( 0 )
    , mode( navigation_mode::ITEM )
    , own_inv_column( preset )
    , own_gear_column( preset )
    , map_column( preset )
{
    ctxt.register_action( "DOWN", to_translation( "Next item" ) );
    ctxt.register_action( "UP", to_translation( "Previous item" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Page down" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Page up" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next column" ) );
    ctxt.register_action( "LEFT", to_translation( "Previous column" ) );
    ctxt.register_action( "CONFIRM", to_translation( "Confirm your selection" ) );
    ctxt.register_action( "QUIT", to_translation( "Cancel" ) );
    ctxt.register_action( "CATEGORY_SELECTION", to_translation( "Switch category selection mode" ) );
    ctxt.register_action( "TOGGLE_FAVORITE", to_translation( "Toggle favorite" ) );
    ctxt.register_action( "HOME", to_translation( "Home" ) );
    ctxt.register_action( "END", to_translation( "End" ) );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "VIEW_CATEGORY_MODE" );
    ctxt.register_action( "ANY_INPUT" ); // For invlets
    ctxt.register_action( "INVENTORY_FILTER" );
    ctxt.register_action( "EXAMINE" );

    append_column( own_inv_column );
    append_column( map_column );
    append_column( own_gear_column );
}

inventory_selector::~inventory_selector() = default;

bool inventory_selector::empty() const
{
    return is_empty;
}

bool inventory_selector::has_available_choices() const
{
    return std::any_of( columns.begin(), columns.end(), []( const inventory_column * element ) {
        return element->has_available_choices();
    } );
}

inventory_input inventory_selector::get_input()
{
    inventory_input res;

    res.action = ctxt.handle_input();
    res.ch = ctxt.get_raw_input().get_first_input();

    cata::optional<point> o_p = ctxt.get_coordinates_text( w_inv );
    if( o_p ) {
        point p = o_p.value();
        if( window_contains_point_relative( w_inv, p ) ) {
            res.entry = find_entry_by_coordinate( p );
            if( res.entry != nullptr && res.entry->is_selectable() ) {
                return res;
            }
        }
    }

    res.entry = find_entry_by_invlet( res.ch );
    if( res.entry != nullptr && !res.entry->is_selectable() ) {
        res.entry = nullptr;
    }
    return res;
}

void inventory_selector::on_input( const inventory_input &input )
{
    if( input.action == "CATEGORY_SELECTION" ) {
        toggle_navigation_mode();
    } else if( input.action == "LEFT" ) {
        toggle_active_column( scroll_direction::BACKWARD );
    } else if( input.action == "RIGHT" ) {
        toggle_active_column( scroll_direction::FORWARD );
    } else if( input.action == "VIEW_CATEGORY_MODE" ) {
        toggle_categorize_contained();
    } else {
        if( has_available_choices() ) {
            for( inventory_column *elem : columns ) {
                elem->on_input( input );
            }
        }
        refresh_active_column(); // Columns can react to actions by losing their activation capacity
        if( input.action == "TOGGLE_FAVORITE" ) {
            // Favoriting items changes item name length which may require resizing
            shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
            if( current_ui ) {
                current_ui->mark_resize();
            }
        }
    }
}

void inventory_selector::on_change( const inventory_entry &entry )
{
    for( auto &elem : columns ) {
        elem->on_change( entry );
    }
    refresh_active_column(); // Columns can react to changes by losing their activation capacity
}

std::vector<inventory_column *> inventory_selector::get_visible_columns() const
{
    std::vector<inventory_column *> res( columns.size() );
    const auto iter = std::copy_if( columns.begin(), columns.end(), res.begin(),
    []( const inventory_column * e ) {
        return e->visible();
    } );
    res.resize( std::distance( res.begin(), iter ) );
    return res;
}

inventory_column &inventory_selector::get_column( size_t index ) const
{
    if( index >= columns.size() ) {
        static inventory_column dummy( preset );
        return dummy;
    }
    return *columns[index];
}

void inventory_selector::set_active_column( size_t index )
{
    if( index < columns.size() && index != active_column_index && get_column( index ).activatable() ) {
        get_active_column().on_deactivate();
        active_column_index = index;
        get_active_column().on_activate();
    }
}

size_t inventory_selector::get_columns_width( const std::vector<inventory_column *> &columns ) const
{
    return std::accumulate( columns.begin(), columns.end(), static_cast< size_t >( 0 ),
    []( const size_t &lhs, const inventory_column * column ) {
        return lhs + column->get_width();
    } );
}

double inventory_selector::get_columns_occupancy_ratio( size_t client_width ) const
{
    const auto visible_columns = get_visible_columns();
    const int free_width = client_width - get_columns_width( visible_columns )
                           - min_column_gap * std::max( static_cast<int>( visible_columns.size() ) - 1, 0 );
    return 1.0 - static_cast<double>( free_width ) / client_width;
}

bool inventory_selector::are_columns_centered( size_t client_width ) const
{
    return get_columns_occupancy_ratio( client_width ) >= min_ratio_to_center;
}

bool inventory_selector::is_overflown( size_t client_width ) const
{
    return get_columns_occupancy_ratio( client_width ) > 1.0;
}

void inventory_selector::toggle_categorize_contained()
{
    const auto return_true = []( const inventory_entry & ) {
        return true;
    };
    const auto return_item = []( const inventory_entry & entry ) {
        return entry.is_item();
    };
    std::vector<item_location> selected;
    if( get_selected().is_item() ) {
        selected = get_selected().locations;
    }
    if( own_inv_column.empty() ) {
        inventory_column replacement_column;
        for( inventory_entry *entry : own_gear_column.get_entries( return_item ) ) {
            if( entry->any_item().where() == item_location::type::container ) {
                item_location ancestor = entry->any_item();
                while( ancestor.has_parent() ) {
                    ancestor = ancestor.parent_item();
                }
                const item_category *custom_category = nullptr;
                if( ancestor.where() != item_location::type::character ) {
                    // might have been merged from the map column
                    custom_category = entry->get_category_ptr();
                }
                add_entry( own_inv_column, std::move( entry->locations ),
                           /*custom_category=*/custom_category,
                           /*chosen_count=*/entry->chosen_count );
            } else {
                replacement_column.add_entry( *entry );
            }
        }
        own_gear_column.clear();
        for( inventory_entry *entry : replacement_column.get_entries( return_true ) ) {
            own_gear_column.add_entry( *entry );
        }
        own_inv_column.set_indent_entries_override( false );
    } else {
        for( inventory_entry *entry : own_inv_column.get_entries( return_item ) ) {
            item_location ancestor = entry->any_item();
            while( ancestor.has_parent() ) {
                ancestor = ancestor.parent_item();
            }
            const item_category *custom_category = nullptr;
            if( ancestor.where() != item_location::type::character ) {
                // might have been merged from the map column
                custom_category = entry->get_category_ptr();
            } else if( &*ancestor == &u.weapon ) {
                custom_category = &item_category_id( "WEAPON_HELD" ).obj();
            } else if( u.is_worn( *ancestor ) ) {
                custom_category = &item_category_id( "ITEMS_WORN" ).obj();
            }
            add_entry( own_gear_column, std::move( entry->locations ),
                       /*custom_category=*/custom_category,
                       /*chosen_count=*/entry->chosen_count );
        }
        own_gear_column.order_by_parent();
        own_inv_column.clear();
    }
    if( !selected.empty() ) {
        select_one_of( selected );
    }

    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }
}

void inventory_selector::toggle_active_column( scroll_direction dir )
{
    if( columns.empty() ) {
        return;
    }

    size_t index = active_column_index;

    do {
        switch( dir ) {
            case scroll_direction::FORWARD:
                index = index + 1 < columns.size() ? index + 1 : 0;
                break;
            case scroll_direction::BACKWARD:
                index = index > 0 ? index - 1 : columns.size() - 1;
                break;
        }
    } while( index != active_column_index && !get_column( index ).activatable() );

    set_active_column( index );
}

void inventory_selector::toggle_navigation_mode()
{
    mode = get_navigation_data( mode ).next_mode;
    for( auto &elem : columns ) {
        elem->on_mode_change( mode );
    }
}

void inventory_selector::append_column( inventory_column &column )
{
    column.on_mode_change( mode );

    if( columns.empty() ) {
        column.on_activate();
    }

    columns.push_back( &column );
}

const navigation_mode_data &inventory_selector::get_navigation_data( navigation_mode m ) const
{
    static const std::map<navigation_mode, navigation_mode_data> mode_data = {
        { navigation_mode::ITEM,     { navigation_mode::CATEGORY, translation(),                               c_light_gray } },
        { navigation_mode::CATEGORY, { navigation_mode::ITEM,     to_translation( "Category selection mode" ), h_white  } }
    };

    return mode_data.at( m );
}

std::string inventory_selector::action_bound_to_key( char key ) const
{
    return ctxt.input_to_action( input_event( key, input_event_t::keyboard_char ) );
}

item_location inventory_pick_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    while( true ) {
        ui_manager::redraw();
        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            if( select( input.entry->any_item() ) ) {
                ui_manager::redraw();
            }
            return input.entry->any_item();
        } else if( input.action == "QUIT" ) {
            return item_location();
        } else if( input.action == "CONFIRM" ) {
            const inventory_entry &selected = get_active_column().get_selected();
            if( selected ) {
                return selected.any_item();
            }
        } else if( input.action == "INVENTORY_FILTER" ) {
            set_filter();
        } else if( input.action == "EXAMINE" ) {
            const inventory_entry &selected = get_active_column().get_selected();
            if( selected ) {
                const item *sitem = selected.any_item().get_item();
                action_examine( sitem );
            }
        } else {
            on_input( input );
        }
    }
}

void inventory_selector::action_examine( const item *sitem )
{
    // Code below pulled from the action_examine function in advanced_inv.cpp
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;
    sitem->info( true, vThisItem );
    item_info_data data( sitem->tname(), sitem->type_name(), vThisItem, vDummy );
    data.handle_scrolling = true;

    draw_item_info( [&]() -> catacurses::window {
        int maxwidth = std::max( FULL_SCREEN_WIDTH, TERMX );
        int width = std::min( 80, maxwidth );
        return catacurses::newwin( 0, width, point( maxwidth / 2 - width / 2, 0 ) ); },
    data ).get_first_input();
}

void inventory_selector::highlight()
{
    const auto return_item = []( const inventory_entry & entry ) {
        return entry.is_item();
    };
    const inventory_entry &selected = get_active_column().get_selected();
    if( !selected.is_item() ) {
        return;
    }
    item_location parent = item_location::nowhere;
    bool selected_has_parent = false;
    if( selected.is_item() && selected.any_item().has_parent() ) {
        parent = selected.any_item().parent_item();
        selected_has_parent = true;
    }
    for( const inventory_column *column : get_all_columns() ) {
        for( inventory_entry *entry : column->get_entries( return_item ) ) {
            // Find parent of selected.
            if( selected_has_parent ) {
                // Check if parent is in a stack.
                for( const item_location &test_loc : entry->locations ) {
                    if( test_loc == parent ) {
                        entry->highlight_as_parent = true;
                        break;
                    }
                }
            }
            // Find contents of selected.
            if( !entry->any_item().has_parent() ) {
                continue;
            }
            // More than one item can be highlighted when selected container is stacked.
            for( const item_location &location : selected.locations ) {
                if( entry->any_item().parent_item() == location ) {
                    entry->highlight_as_child = true;
                }
            }
        }
    }
}

inventory_multiselector::inventory_multiselector( Character &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title ) :
    inventory_selector( p, preset ),
    selection_col( new selection_column( "SELECTION_COLUMN", selection_column_title ) )
{
    ctxt.register_action( "RIGHT", to_translation( "Mark/unmark selected item" ) );
    ctxt.register_action( "DROP_NON_FAVORITE", to_translation( "Mark/unmark non-favorite items" ) );

    for( inventory_column * const &elem : get_all_columns() ) {
        elem->set_multiselect( true );
    }
    append_column( *selection_col );
}

void inventory_multiselector::rearrange_columns( size_t client_width )
{
    selection_col->set_visibility( true );
    inventory_selector::rearrange_columns( client_width );
    selection_col->set_visibility( !is_overflown( client_width ) );
}

inventory_compare_selector::inventory_compare_selector( Character &p ) :
    inventory_multiselector( p, default_preset, _( "ITEMS TO COMPARE" ) ) {}

std::pair<const item *, const item *> inventory_compare_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        inventory_entry *just_selected = nullptr;

        if( input.entry != nullptr ) {
            select( input.entry->any_item() );
            toggle_entry( input.entry );
            just_selected = input.entry;
        } else if( input.action == "RIGHT" ) {
            const auto selection( get_active_column().get_all_selected() );

            for( inventory_entry * const &elem : selection ) {
                if( elem->chosen_count == 0 || selection.size() == 1 ) {
                    toggle_entry( elem );
                    just_selected = elem;
                    if( compared.size() == 2 ) {
                        break;
                    }
                }
            }
        } else if( input.action == "CONFIRM" ) {
            popup_getkey( _( "You need two items for comparison.  Use %s to select them." ),
                          ctxt.get_desc( "RIGHT" ) );
        } else if( input.action == "QUIT" ) {
            return std::make_pair( nullptr, nullptr );
        } else if( input.action == "INVENTORY_FILTER" ) {
            set_filter();
        } else if( input.action == "TOGGLE_FAVORITE" ) {
            // TODO: implement favoriting in multi selection menus while maintaining selection
        } else {
            on_input( input );
        }

        if( compared.size() == 2 ) {
            const auto res = std::make_pair( compared[0], compared[1] );
            // Clear second selected entry to prevent comparison reopening too
            // soon
            if( just_selected ) {
                toggle_entry( just_selected );
            }
            return res;
        }
    }
}

void inventory_compare_selector::toggle_entry( inventory_entry *entry )
{
    const item *it = &*entry->any_item();
    const auto iter = std::find( compared.begin(), compared.end(), it );

    entry->chosen_count = iter == compared.end() ? 1 : 0;

    if( entry->chosen_count != 0 ) {
        compared.push_back( it );
    } else {
        compared.erase( iter );
    }

    on_change( *entry );
}

inventory_iuse_selector::inventory_iuse_selector(
    Character &p,
    const std::string &selector_title,
    const inventory_selector_preset &preset,
    const GetStats &get_st
) :
    inventory_multiselector( p, preset, selector_title ),
    get_stats( get_st ),
    max_chosen_count( std::numeric_limits<decltype( max_chosen_count )>::max() )
{}
drop_locations inventory_iuse_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();

    auto is_entry = []( const inventory_entry & elem ) {
        return elem.is_selectable();
    };
    for( inventory_column *col : get_all_columns() ) {
        if( col->allows_selecting() ) {
            for( inventory_entry *ie : col->get_entries( is_entry ) ) {
                for( item_location const &x : ie->locations ) {
                    usable_locs.push_back( x );
                }
            }
        }
    }
    int count = 0;
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        if( input.ch >= '0' && input.ch <= '9' ) {
            count = std::min( count, INT_MAX / 10 - 10 );
            count *= 10;
            count += input.ch - '0';
        } else if( input.entry != nullptr ) {
            select( input.entry->any_item() );
            if( count == 0 && input.entry->chosen_count == 0 ) {
                count = max_chosen_count;
            }
            set_chosen_count( *input.entry, count );
            count = 0;
        } else if( input.action == "RIGHT" ) {
            const auto selected( get_active_column().get_all_selected() );

            if( count == 0 ) {
                const bool clear = std::none_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return elem->chosen_count > 0;
                } );

                if( clear ) {
                    count = max_chosen_count;
                }
            }

            for( const auto &elem : selected ) {
                set_chosen_count( *elem, count );
            }
            count = 0;
        } else if( input.action == "CONFIRM" ) {
            if( to_use.empty() ) {
                popup_getkey( _( "No items were selected.  Use %s to select them." ),
                              ctxt.get_desc( "RIGHT" ) );
                continue;
            }
            break;
        } else if( input.action == "QUIT" ) {
            return drop_locations();
        } else if( input.action == "INVENTORY_FILTER" ) {
            set_filter();
        } else {
            on_input( input );
            count = 0;
        }
    }
    drop_locations dropped_pos_and_qty;

    for( const std::pair<const item_location *const, int> &use_pair : to_use ) {
        dropped_pos_and_qty.push_back( std::make_pair( *use_pair.first, use_pair.second ) );
    }

    return dropped_pos_and_qty;
}

void inventory_iuse_selector::set_chosen_count( inventory_entry &entry, size_t count )
{
    const item_location &it = entry.any_item();
    std::map<const item_location *, int> temp_use;

    if( count == 0 ) {
        entry.chosen_count = 0;
        for( const item_location &loc : entry.locations ) {
            temp_use[&loc] = 0;
        }
    } else {
        entry.chosen_count = std::min( std::min( count, max_chosen_count ), entry.get_available_count() );
        if( it->count_by_charges() ) {
            temp_use[&it] = entry.chosen_count;
        } else {
            for( const item_location &loc : entry.locations ) {
                if( count == 0 ) {
                    break;
                }
                temp_use[&loc] = 1;
                count--;
            }
        }
    }
    // Optimisation to reduce the scale of looping if otherwise done in the preceeding code.
    for( auto iter : temp_use ) {
        for( const item_location &x : usable_locs ) {
            if( x == *iter.first ) {
                if( iter.second > 0 ) {
                    to_use[&x] = iter.second;
                } else {
                    to_use.erase( &x );
                }
            }
        }
    }

    on_change( entry );
}

inventory_selector::stats inventory_iuse_selector::get_raw_stats() const
{
    if( get_stats ) {
        return get_stats( to_use );
    }
    return stats{{ stat{{ "", "", "", "" }}, stat{{ "", "", "", "" }} }};
}

inventory_drop_selector::inventory_drop_selector( Character &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title,
        const bool warn_liquid ) :
    inventory_multiselector( p, preset, selection_column_title ),
    max_chosen_count( std::numeric_limits<decltype( max_chosen_count )>::max() ),
    warn_liquid( warn_liquid )
{
#if defined(__ANDROID__)
    // allow user to type a drop number without dismissing virtual keyboard after each keypress
    ctxt.allow_text_entry = true;
#endif
}

void inventory_drop_selector::process_selected( int &count,
        const std::vector<inventory_entry *> &selected )
{
    if( count == 0 ) {
        const bool clear = std::none_of( selected.begin(), selected.end(),
        []( const inventory_entry * elem ) {
            return elem->chosen_count > 0;
        } );

        if( clear ) {
            count = max_chosen_count;
        }
    }

    for( const auto &elem : selected ) {
        set_chosen_count( *elem, count );
    }
    count = 0;
}

void inventory_drop_selector::deselect_contained_items()
{
    std::vector<item_location> inventory_items;
    for( std::pair<item_location, int> &drop : dropping ) {
        item_location loc_front = drop.first;
        inventory_items.push_back( loc_front );
    }
    for( item_location loc_container : inventory_items ) {
        if( !loc_container->empty() ) {
            for( inventory_column *col : get_all_columns() ) {
                for( inventory_entry *selected : col->get_entries( []( const inventory_entry &
                entry ) {
                return entry.chosen_count > 0;
            } ) ) {
                    if( !selected->is_item() ) {
                        continue;
                    }
                    for( item *item_contained : loc_container->all_items_top() ) {
                        for( const item_location &selected_loc : selected->locations ) {
                            if( selected_loc.get_item() == item_contained ) {
                                set_chosen_count( *selected, 0 );
                            }
                        }
                    }
                }
            }
        }
    }
    for( inventory_column *col : get_all_columns() ) {
        for( inventory_entry *selected : col->get_entries(
        []( const inventory_entry & entry ) {
        return entry.is_item() && entry.chosen_count > 0 && entry.locations.front()->is_frozen_liquid();
        } ) ) {
            set_chosen_count( *selected, 0 );
        }
    }
}

drop_locations inventory_drop_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();

    int count = 0;
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        if( input.ch >= '0' && input.ch <= '9' ) {
            count = std::min( count, INT_MAX / 10 - 10 );
            count *= 10;
            count += input.ch - '0';
        } else if( input.entry != nullptr ) {
            select( input.entry->any_item() );
            if( count == 0 && input.entry->chosen_count == 0 ) {
                count = INT_MAX;
            }
            set_chosen_count( *input.entry, count );
            count = 0;
        } else if( input.action == "DROP_NON_FAVORITE" ) {
            const auto filter_to_nonfavorite_and_nonworn = []( const inventory_entry & entry ) {
                return entry.is_item() &&
                       !entry.any_item()->is_favorite &&
                       !get_player_character().is_worn( *entry.any_item() );
            };

            const auto selected( get_active_column().get_entries( filter_to_nonfavorite_and_nonworn ) );
            process_selected( count, selected );
            deselect_contained_items();
        } else if( input.action == "RIGHT" ) {
            const auto selected( get_active_column().get_all_selected() );

            // No amount entered, select all
            if( count == 0 ) {
                count = INT_MAX;

                // Any non favorite item to select?
                const bool select_nonfav = std::any_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return ( !elem->any_item()->is_favorite ) && elem->chosen_count == 0;
                } );

                // Otherwise, any favorite item to select?
                const bool select_fav = !select_nonfav && std::any_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return elem->any_item()->is_favorite && elem->chosen_count == 0;
                } );

                for( const auto &elem : selected ) {
                    const bool is_favorite = elem->any_item()->is_favorite;
                    if( ( select_nonfav && !is_favorite ) || ( select_fav && is_favorite ) ) {
                        set_chosen_count( *elem, count );
                    } else if( !select_nonfav && !select_fav ) {
                        // Every element is selected, unselect all
                        set_chosen_count( *elem, 0 );
                    }
                }
                deselect_contained_items();
                // Select the entered amount
            } else {
                for( const auto &elem : selected ) {
                    set_chosen_count( *elem, count );
                }
                deselect_contained_items();
            }

            count = 0;
        } else if( input.action == "CONFIRM" ) {
            if( dropping.empty() ) {
                popup_getkey( _( "No items were selected.  Use %s to select them." ),
                              ctxt.get_desc( "RIGHT" ) );
                continue;
            }
            break;
        } else if( input.action == "QUIT" ) {
            return drop_locations();
        } else if( input.action == "INVENTORY_FILTER" ) {
            set_filter();
        } else if( input.action == "TOGGLE_FAVORITE" ) {
            // TODO: implement favoriting in multi selection menus while maintaining selection
        } else {
            on_input( input );
            count = 0;
        }
    }

    drop_locations dropped_pos_and_qty;

    enum class drop_liquid {
        ask, no, yes
    } should_drop_liquid = drop_liquid::ask;

    for( const std::pair<item_location, int> &drop_pair : dropping ) {
        bool should_drop = true;
        if( drop_pair.first->made_of_from_type( phase_id::LIQUID ) ) {
            if( should_drop_liquid == drop_liquid::ask ) {
                if( !warn_liquid || query_yn(
                        _( "You are dropping liquid from its container.  You might not be able to pick it back up.  Really do so?" ) ) ) {
                    should_drop_liquid = drop_liquid::yes;
                } else {
                    should_drop_liquid = drop_liquid::no;
                }
            }
            if( should_drop_liquid == drop_liquid::no ) {
                should_drop = false;
            }
        }
        if( should_drop ) {
            dropped_pos_and_qty.push_back( drop_pair );
        }
    }

    return dropped_pos_and_qty;
}

void inventory_drop_selector::set_chosen_count( inventory_entry &entry, size_t count )
{
    const item_location &it = entry.any_item();

    if( count == 0 ) {
        entry.chosen_count = 0;
        for( const item_location &loc : entry.locations ) {
            for( auto iter = dropping.begin(); iter != dropping.end(); ) {
                if( iter->first == loc ) {
                    dropping.erase( iter );
                } else {
                    ++iter;
                }
            }
        }
    } else {
        entry.chosen_count = std::min( std::min( count, max_chosen_count ), entry.get_available_count() );
        if( it->count_by_charges() ) {
            auto iter = find_if( dropping.begin(), dropping.end(), [&it]( drop_location drop ) {
                return drop.first == it;
            } );
            if( iter == dropping.end() ) {
                dropping.emplace_back( it, static_cast<int>( entry.chosen_count ) );
            }
        } else {
            for( const item_location &loc : entry.locations ) {
                if( count == 0 ) {
                    break;
                }
                auto iter = find_if( dropping.begin(), dropping.end(), [&loc]( drop_location drop ) {
                    return drop.first == loc;
                } );
                if( iter == dropping.end() ) {
                    dropping.emplace_back( loc, 1 );
                }
                count--;
            }
        }
    }

    on_change( entry );
}

inventory_selector::stats inventory_drop_selector::get_raw_stats() const
{
    return get_weight_and_volume_stats(
               u.weight_carried_with_tweaks( dropping ),
               u.weight_capacity(),
               u.volume_carried_with_tweaks( dropping ),
               u.volume_capacity_with_tweaks( dropping ),
               u.max_single_item_length(), u.max_single_item_volume() );
}
