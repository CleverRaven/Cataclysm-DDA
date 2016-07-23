#include "inventory_ui.h"

#include "game.h"
#include "player.h"
#include "action.h"
#include "map.h"
#include "output.h"
#include "translations.h"
#include "options.h"
#include "messages.h"
#include "catacharset.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "cata_utility.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

/** The minimal gap between two cells */
static const int min_cell_gap = 1;
/** The minimal gap between two columns */
static const int min_column_gap = 2;
/** The minimal occupancy ratio (see @refer get_columns_occupancy_ratio()) to align columns to the center */
static const double min_ratio_to_center = 0.65;

static const item_category weapon_held_cat( "WEAPON HELD", _( "WEAPON HELD" ), -200 );
static const item_category items_worn_cat( "ITEMS WORN", _( "ITEMS WORN" ), -100 );

const std::string inventory_selector_preset::bug_text = _( "it's a bug!" );

struct inventory_input {
    std::string action;
    long ch;
    inventory_entry *entry;
};

class selection_column_preset: public inventory_selector_preset
{
    public:
        selection_column_preset() {}

        virtual std::string get_caption( const inventory_entry &entry ) const override {
            std::ostringstream res;
            const size_t available_count = entry.get_available_count();
            if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
                res << string_format( _( "%d of %d" ), entry.chosen_count, available_count ) << ' ';
            } else if( available_count != 1 ) {
                res << available_count << ' ';
            }
            res << entry.get_item().display_name( available_count );
            return res.str();
        }

        virtual nc_color get_color( const item &it ) const override {
            if( &it == &g->u.weapon ) {
                return c_ltblue;
            } else if( g->u.is_worn( it ) ) {
                return c_cyan;
            } else {
                return inventory_selector_preset::get_color( it );
            }
        }
};

static const selection_column_preset selection_preset;

class selection_column : public inventory_column
{
    public:
        selection_column( const std::string &id, const std::string &name ) :
            inventory_column( selection_preset ),
            selected_cat( new item_category( id, name, 0 ) ) {}

        virtual bool activatable() const override {
            return inventory_column::activatable() && pages_count() > 1;
        }

        virtual bool allows_selecting() const override {
            return false;
        }

        virtual void prepare_paging() override {
            inventory_column::prepare_paging();
            if( entries.empty() ) { // Category must always persist
                entries.emplace_back( selected_cat.get() );
                expand_to_fit( entries.back() );
            }
        }

        virtual void on_change( const inventory_entry &entry ) override;

    private:
        const std::unique_ptr<item_category> selected_cat;
};

bool inventory_entry::operator == ( const inventory_entry &other ) const
{
    return get_category_ptr() == other.get_category_ptr() && location == other.location;
}

bool inventory_entry::is_item() const
{
    return location != nullptr && *location;
}

bool inventory_entry::is_null() const
{
    return get_category_ptr() == nullptr;
}

size_t inventory_entry::get_available_count() const
{
    if( is_item() && get_stack_size() == 1 ) {
        return get_item().count_by_charges() ? get_item().charges : 1;
    } else {
        return get_stack_size();
    }
}

const item &inventory_entry::get_item() const
{
    if( !is_item() ) {
        static const item nullitem;
        debugmsg( "Tried to access an empty item." );
        return nullitem;
    }
    return *( *location );
}

long inventory_entry::get_invlet() const
{
    return ( custom_invlet != LONG_MIN ) ? custom_invlet : is_item() ? get_item().invlet : '\0';
}

const item_category *inventory_entry::get_category_ptr() const
{
    return ( custom_category != nullptr )
           ? custom_category : is_item() ? &get_item().get_category() : nullptr;
}

const item_location &inventory_entry::get_location() const
{
    if( location == nullptr ) {
        static const item_location null_location;
        return null_location;
    }
    return *location;
}

inventory_selector_preset::inventory_selector_preset()
{
    append_cell( [ this ]( const inventory_entry & entry ) -> std::string {
        return get_caption( entry );
    } );
}

bool inventory_selector_preset::is_shown( const item & ) const
{
    return true;
}

bool inventory_selector_preset::is_shown( const item_location &location ) const
{
    return is_shown( *location );
}

bool inventory_selector_preset::is_enabled( const item & ) const
{
    return true;
}

bool inventory_selector_preset::is_enabled( const item_location &location ) const
{
    return is_enabled( *location );
}

int inventory_selector_preset::get_rank( const item & ) const
{
    return 0;
}

int inventory_selector_preset::get_rank( const item_location &location ) const
{
    return get_rank( *location );
}

nc_color inventory_selector_preset::get_color( const item &it ) const
{
    return it.color_in_inventory();
}

nc_color inventory_selector_preset::get_color( const item_location &location ) const
{
    return get_color( *location );
}

std::string inventory_selector_preset::get_caption( const inventory_entry &entry ) const
{
    const size_t count = entry.get_stack_size();
    const std::string disp_name = entry.get_item().display_name( count );

    return ( count > 1 ) ? string_format( "%d %s", count, disp_name.c_str() ) : disp_name;
}

std::string inventory_selector_preset::get_cell_text( const inventory_entry &entry,
        size_t cell_index ) const
{
    if( cell_index >= cells.size() ) {
        debugmsg( "Invalid cell index %d.", cell_index );
        return bug_text;
    }
    if( entry.is_null() ) {
        return std::string();
    } else if( entry.is_item() ) {
        return cells[cell_index].second( entry );
    } else if( cell_index != 0 ) {
        return cells[cell_index].first;
    } else {
        return entry.get_category_ptr()->name;
    }
}

nc_color inventory_selector_preset::get_cell_color( const inventory_entry &entry, size_t ) const
{
    return entry.is_item() ? get_color( entry.get_location() ) : c_magenta;
}

size_t inventory_selector_preset::get_cell_width( const inventory_entry &entry,
        size_t cell_index ) const
{
    return utf8_width( get_cell_text( entry, cell_index ), true );
}

void inventory_selector_preset::append_cell( const std::function<std::string( const item & )> &func,
                                             const std::string &title )
{
    // Don't capture by reference here. The func should be able to die earlier than the object itself
    append_cell( [ func ]( const inventory_entry & entry ) {
        return func( entry.get_item() );
    }, title );
}

void inventory_selector_preset::append_cell( const std::function<std::string( const item_location & )> &func,
                                             const std::string &title )
{
    // Don't capture by reference here. The func should be able to die earlier than the object itself
    append_cell( [ func ]( const inventory_entry & entry ) {
        return func( entry.get_location() );
    }, title );
}

void inventory_selector_preset::append_cell( const std::function<std::string( const inventory_entry & )> &func,
                                             const std::string &title )
{
    const auto iter = std::find_if( cells.begin(), cells.end(), [ &title ]( const cell_pair & cell ) {
        return cell.first == title;
    } );
    if( iter != cells.end() ) {
        debugmsg( "Tried to append a duplicate cell \"%s\": ignored.", title.c_str() );
        return;
    }
    cells.emplace_back( title, func );
}

inventory_column::inventory_column( const inventory_selector_preset &preset ) :
    preset( preset )
{
    cell_widths.resize( preset.get_cells_count(), 0 );
}

bool inventory_column::activatable() const
{
    return std::any_of( entries.begin(), entries.end(), []( const inventory_entry & entry ) {
        return entry.is_selectable();
    } );
}

inventory_entry *inventory_column::find_by_invlet( long invlet )
{
    for( auto &entry : entries ) {
        if( entry.is_item() && ( entry.get_invlet() == invlet ) ) {
            return &entry;
        }
    }
    return nullptr;
}

size_t inventory_column::get_entry_indent( const inventory_entry &entry ) const
{
    if( !entry.is_item() ) {
        return 0;
    }
    size_t res = 2;
    if( get_option<bool>("ITEM_SYMBOLS" ) ) {
        res += 2;
    }
    if( allows_selecting() && multiselect ) {
        res += 2;
    }
    return res;
}

size_t inventory_column::get_entry_cell_width( const inventory_entry &entry, size_t cell_index ) const
{
    return preset.get_cell_width( entry, cell_index ) + ( cell_index == 0 ? get_entry_indent( entry ) : min_cell_gap );
}

void inventory_column::set_width( const size_t width )
{
    reset_width();
    int width_gap = get_width() - width;
    // Now adjust the width if we must
    while( width_gap != 0 ) {
        const int step = width_gap > 0 ? -1 : 1;
        size_t &cell_width = step > 0
            ? *std::min_element( cell_widths.begin(), cell_widths.end() )
            : *std::max_element( cell_widths.begin(), cell_widths.end() );
        if( cell_width == 0 ) {
            break; // This is highly unlikely to happen, but just in case
        }
        cell_width += step;
        width_gap += step;
    }
}

void inventory_column::set_height( size_t height ) {
    if( entries_per_page != height ) {
        if( height == 0 ) {
            debugmsg( "Unable to assign zero height." );
            return;
        }
        entries_per_page = height;
        paging_is_valid = false;
    }
}

void inventory_column::expand_to_fit( const inventory_entry &entry )
{
    for( size_t cell_index = 0; cell_index < cell_widths.size(); ++cell_index ) {
        size_t &cell_width = cell_widths.at( cell_index );
        cell_width = std::max( cell_width, get_entry_cell_width( entry, cell_index ) );
    }

}

void inventory_column::reset_width()
{
    std::fill( cell_widths.begin(), cell_widths.end(), 0 );
    for( const auto &entry : entries ) {
        expand_to_fit( entry );
    }
}

size_t inventory_column::get_width() const
{
    return std::accumulate( cell_widths.begin(), cell_widths.end(), 0 );
}

void inventory_column::select( size_t new_index, int step )
{
    if( new_index < entries.size() ) {
        selected_index = new_index;
        page_offset = selected_index - selected_index % entries_per_page;

        if( !entries[selected_index].is_selectable() ) {
            move_selection( step );
        }
    }
}

void inventory_column::move_selection( int step )
{
    if( step == 0 ) {
        return;
    }
    const auto get_incremented = [ this ]( size_t index, int step ) -> size_t {
        return ( index + step + entries.size() ) % entries.size();
    };

    size_t index = get_incremented( selected_index, step );
    while( entries[index] != get_selected() && ( !entries[index].is_selectable()
            || is_selected_by_category( entries[index] ) ) ) {
        index = get_incremented( index, ( step > 0 ? 1 : -1 ) );
    }

    select( index );
}

size_t inventory_column::page_of( size_t index ) const
{
    return index / entries_per_page;
}

size_t inventory_column::page_of( const inventory_entry &entry ) const
{
    return page_of( std::distance( entries.begin(), std::find( entries.begin(), entries.end(),
                                   entry ) ) );
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
    if( selected_index >= entries.size() ) {
        static const inventory_entry dummy;
        return dummy;
    }
    return entries[selected_index];
}

std::vector<inventory_entry *> inventory_column::get_all_selected()
{
    std::vector<inventory_entry *> res;

    if( allows_selecting() ) {
        for( auto &entry : entries ) {
            if( is_selected( entry ) ) {
                res.push_back( &entry );
            }
        }
    }

    return res;
}

void inventory_column::on_input( const inventory_input &input )
{
    if( empty() || !active ) {
        return; // ignore
    }

    if( input.action == "DOWN" ) {
        move_selection( 1 );
    } else if( input.action == "UP" ) {
        move_selection( -1 );
    } else if( input.action == "NEXT_TAB" ) {
        move_selection( std::max( std::min<int>( entries_per_page, entries.size() - selected_index - 1 ), 1 ) );
    } else if( input.action == "PREV_TAB" ) {
        move_selection( std::min( std::max<int>( UINT32_MAX - entries_per_page + 1, (UINT32_MAX - selected_index + 1) + 1 ), -1 ) );
    } else if( input.action == "HOME" ) {
        select( 0, 1 );
    } else if( input.action == "END" ) {
        select( entries.size() - 1, -1 );
    }
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
                                    && cur_cat->sort_rank <= new_cat->sort_rank );
    } );
    entries.insert( iter.base(), entry );
    expand_to_fit( entry );
    paging_is_valid = false;
}

void inventory_column::remove_entry( const inventory_entry &entry )
{
    const auto iter = std::find( entries.begin(), entries.end(), entry );
    if( iter == entries.end() ) {
        debugmsg( "Tried to remove a non-existing entry." );
        return;
    }
    entries.erase( iter );
    paging_is_valid = false;
}

void inventory_column::move_entries_to( inventory_column &dest )
{
    for( const auto &entry : entries ) {
        if( entry.is_item() ) {
            dest.add_entry( entry );
        }
    }
    dest.prepare_paging();
    clear();
}

void inventory_column::clear()
{
    entries.clear();
    paging_is_valid = false;
    prepare_paging();
}

long inventory_column::reassign_custom_invlets( const player &p, long min_invlet, long max_invlet )
{
    long cur_invlet = min_invlet;
    for( auto &entry : entries ) {
        if( entry.is_item() && !p.has_item( entry.get_item() ) ) {
            entry.custom_invlet = entry.is_selectable() && cur_invlet <= max_invlet ? cur_invlet++ : '\0';
        }
    }
    return cur_invlet;
}

void inventory_column::prepare_paging()
{
    if( paging_is_valid ) {
        return;
    }
    // First, remove all non-items
    const auto new_end = std::remove_if( entries.begin(),
    entries.end(), []( const inventory_entry & entry ) {
        return !entry.is_item();
    } );
    entries.erase( new_end, entries.end() );
    // Then sort them with respect to categories
    auto from = entries.begin();
    while( from != entries.end() ) {
        auto to = std::next( from );
        while( to != entries.end() && from->get_category_ptr() == to->get_category_ptr() ) {
            std::advance( to, 1 );
        }
        std::sort( from, to, []( const inventory_entry & lhs, const inventory_entry & rhs ) {
            return ( lhs.enabled && !rhs.enabled ) || ( lhs.enabled == rhs.enabled && lhs.rank < rhs.rank );
        } );
        from = to;
    }
    // Recover categories according to the number of entries per page
    const item_category *current_category = nullptr;
    for( size_t i = 0; i < entries.size(); ++i ) {
        if( entries[i].get_category_ptr() == current_category && i % entries_per_page != 0 ) {
            continue;
        }
        current_category = entries[i].get_category_ptr();
        const inventory_entry insertion = ( i % entries_per_page == entries_per_page - 1 )
                                          ? inventory_entry() // the last item on the page must not be a category
                                          : inventory_entry( current_category ); // the first item on the page must be a category
        entries.insert( entries.begin() + i, insertion );
        expand_to_fit( insertion );
    }
    // Select the uppermost possible entry
    select( 0, 1 );

    paging_is_valid = true;
}

void inventory_column::draw( WINDOW *win, size_t x, size_t y ) const
{
    if( !visible() ) {
        return;
    }

    const auto available_cell_width = [ this ]( const inventory_entry &entry, const size_t cell_index ) {
        const size_t displayed_width = cell_widths.at( cell_index );
        const size_t real_width = get_entry_cell_width( entry, cell_index );

        return displayed_width > real_width ? displayed_width - real_width : 0;
    };

    // Do the actual drawing
    for( size_t index = page_offset, line = 0; index < entries.size() && line < entries_per_page; ++index, ++line ) {
        const auto &entry = entries[index];

        if( entry.is_null() ) {
            continue;
        }

        int x1 = x + get_entry_indent( entry );
        int x2 = x;
        int yy = y + line;

        const bool selected = active && entry.is_selectable() && is_selected( entry );

        if( selected && cell_widths.size() > 1 ) {
            for( int hx = x1, hx_max = x + get_width(); hx < hx_max; ++hx ) {
                mvwputch( win, yy, hx, h_white, ' ' );
            }
        }

        for( size_t cell_index = 0, count = preset.get_cells_count(); cell_index < count; ++cell_index ) {
            if( line != 0 && cell_index != 0 && !entry.is_item() ) {
                break; // Don't show duplicated titles
            }

            x2 += cell_widths.at( cell_index );

            size_t text_width = preset.get_cell_width( entry, cell_index );
            size_t available_width = x2 - x1;

            if( text_width > available_width ) {
                // See if we can steal some of the needed width from an adjacent cell
                if( cell_index == 0 && count >= 2 ) {
                    available_width += available_cell_width( entry, 1 );
                } else if( cell_index > 0 ) {
                    available_width += available_cell_width( entry, cell_index - 1 );
                }
                text_width = std::min( text_width, available_width );
            }

            if( text_width > 0 ) {
                const int text_x = cell_index == 0 ? x1 : x2 - text_width; // Align either to the left or to the right
                const std::string text = preset.get_cell_text( entry, cell_index );

                if( !entry.enabled ) {
                    trim_and_print( win, yy, text_x, text_width, c_dkgray, "%s", remove_color_tags( text ).c_str() );
                } else if( selected ) {
                    trim_and_print( win, yy, text_x, text_width, h_white, "%s", remove_color_tags( text ).c_str() );
                } else {
                    trim_and_print( win, yy, text_x, text_width, preset.get_cell_color( entry, cell_index ), "%s", text.c_str() );
                }
            }

            x1 = x2;
        }

        if( entry.is_item() ) {
            int xx = x;
            if( entry.is_selectable() && entry.get_invlet() != '\0' ) {
                const nc_color invlet_color = g->u.assigned_invlet.count( entry.get_invlet() ) ? c_yellow : c_white;
                mvwputch( win, yy, x, invlet_color, entry.get_invlet() );
            }
            xx += 2;
            if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
                const nc_color color = entry.is_selectable() ? entry.get_item().color() : c_dkgray;
                mvwputch( win, yy, xx, color, entry.get_item().symbol() );
                xx += 2;
            }
            if( allows_selecting() && multiselect && entry.is_selectable() ) {
                if( entry.chosen_count == 0 ) {
                    mvwputch( win, yy, xx, c_dkgray, '-' );
                } else if( entry.chosen_count >= entry.get_available_count() ) {
                    mvwputch( win, yy, xx, c_ltgreen, '+' );
                } else {
                    mvwputch( win, yy, xx, c_ltgreen, '#' );
                }
            }
        }
    }
}

void selection_column::on_change( const inventory_entry &entry )
{
    inventory_entry my_entry( entry, selected_cat.get() );

    const auto iter = std::find( entries.begin(), entries.end(), my_entry );
    if( iter == entries.end() ) {
        if( my_entry.chosen_count != 0 ) {
            add_entry( my_entry );
        }
    } else if( my_entry.chosen_count == 0 ) {
        remove_entry( my_entry );
    } else {
        iter->chosen_count = my_entry.chosen_count;
        expand_to_fit( my_entry );
    }

    prepare_paging();
    // Now let's update selection
    const auto select_iter = std::find( entries.begin(), entries.end(), my_entry );
    if( select_iter != entries.end() ) {
        select( std::distance( entries.begin(), select_iter ), 1 );
    } else {
        select( entries.empty() ? 0 : entries.size() - 1, -1 ); // Just select the last one
    }
}

inventory_input inventory_selector::get_input()
{
    inventory_input res;

    res.action = ctxt.handle_input();
    res.ch = ctxt.get_raw_input().get_first_input();
    res.entry = find_entry_by_invlet( res.ch );

    if( res.entry != nullptr && !res.entry->is_selectable() ) {
        res.entry = nullptr;
    }

    return res;
}

// @todo Move it into some 'item_stack' class.
static std::vector<std::list<item *>> restack_items( const std::list<item>::iterator &from,
                                                     const std::list<item>::iterator &to )
{
    std::vector<std::list<item *>> res;

    for( auto it = from; it != to; ++it ) {
        // Find an appropriate stack to stack the item with
        auto match = std::find_if( res.begin(), res.end(), [ &it ]( const std::list<item *> &stack ) {
            return it->stacks_with( *stack.back() );
        } );

        if( match != res.end() ) {
            match->push_back( &*it );
        } else {
            res.emplace_back( 1, &*it );
        }
    }

    return res;
}

const item_category *inventory_selector::naturalize_category( const item_category &category, const tripoint &pos )
{
    const auto find_cat_by_id = [ this ]( const std::string &id ) {
        const auto iter = std::find_if( categories.begin(), categories.end(), [ &id ]( const item_category &cat ) {
            return cat.id == id;
        } );
        return iter != categories.end() ? &*iter : nullptr;
    };

    const int dist = rl_dist( p.pos(), pos );

    if( dist != 0 ) {
        const std::string suffix = direction_suffix( p.pos(), pos );
        const std::string id = string_format( "%s_%s", category.id.c_str(), suffix.c_str() );

        const auto existing = find_cat_by_id( id );
        if( existing != nullptr ) {
            return existing;
        }

        const std::string name = string_format( "%s %s", category.name.c_str(), suffix.c_str() );
        const int sort_rank = category.sort_rank + dist;
        const item_category new_category( id, name, sort_rank );

        categories.push_back( new_category );
    } else {
        const auto existing = find_cat_by_id( category.id );
        if( existing != nullptr ) {
            return existing;
        }

        categories.push_back( category );
    }

    return &categories.back();
}

void inventory_selector::add_item( inventory_column &target_column,
                                   const item_location &location,
                                   size_t stack_size, const item_category *custom_category )
{
    if( !preset.is_shown( location ) ) {
        return;
    }

    items.push_back( location.clone() );
    inventory_entry entry( &items.back(), stack_size, custom_category );

    entry.enabled = preset.is_enabled( entry.get_location() );
    entry.rank = preset.get_rank( entry.get_location() );

    target_column.add_entry( entry );
    on_entry_add( entry );

    layout_is_valid = false;
}

void inventory_selector::add_custom_items( inventory_column &target_column,
                                           const std::list<item>::iterator &from,
                                           const std::list<item>::iterator &to,
                                           const item_category &custom_category,
                                           const std::function<item_location( item * )> &locator )
{
    const auto &stacks = restack_items( from, to );
    const item_category *nat_category = nullptr;

    for( const auto &stack : stacks ) {
        auto location = locator( stack.front() );

        if( nat_category == nullptr && preset.is_shown( location ) ) {
            nat_category = naturalize_category( custom_category, location.position() );
        }

        add_item( target_column, location, stack.size(), nat_category );
    }
}

void inventory_selector::add_character_items( Character &character )
{
    auto make_char_loc = [ &character ]( item &it ) {
        return item_location( character, &it );
    };

    for( const auto &stack: character.inv.slice() ) {
        add_item( own_inv_column, make_char_loc( stack->front() ), stack->size() );
    }

    for( auto &it : character.worn ) {
        add_item( own_gear_column, make_char_loc( it ), 1, &items_worn_cat );
    }

    if( !character.weapon.is_null() ) {
        add_item( own_gear_column, make_char_loc( character.weapon ), 1, &weapon_held_cat );
    }
}

void inventory_selector::add_map_items( const tripoint &target )
{
    if( g->m.accessible_items( p.pos(), target, rl_dist( p.pos(), target ) ) ) {
        auto items = g->m.i_at( target );
        const std::string name = to_upper_case( g->m.name( target ) );
        const item_category map_cat( name, name, 100 );

        add_custom_items( map_column, items.begin(), items.end(), map_cat, [ &target ]( item * it ) {
            return item_location( target, it );
        } );
    }
}

void inventory_selector::add_vehicle_items( const tripoint &target )
{
    int part = -1;
    vehicle *veh = g->m.veh_at( target, part );

    if( veh != nullptr && ( part = veh->part_with_feature( part, "CARGO" ) ) >= 0 ) {
        auto items = veh->get_items( part );
        const std::string name = to_upper_case( veh->parts[part].name() );
        const item_category vehicle_cat( name, name, 200 );

        add_custom_items( map_column, items.begin(), items.end(), vehicle_cat, [ veh, part ]( item * it ) {
            return item_location( vehicle_cursor( *veh, part ), it );
        } );
    }
}

void inventory_selector::add_nearby_items( int radius )
{
    if( radius >= 0 ) {
        for( const auto &pos : closest_tripoints_first( radius, p.pos() ) ) {
            add_map_items( pos );
            add_vehicle_items( pos );
        }
    }
}

inventory_entry *inventory_selector::find_entry_by_invlet( long invlet )
{
    for( const auto &column : columns ) {
        const auto res = column->find_by_invlet( invlet );
        if( res != nullptr ) {
            return res;
        }
    }
    return nullptr;
}

void inventory_selector::rearrange_columns()
{
    if( !own_gear_column.empty() && is_overflown() ) {
        own_gear_column.move_entries_to( own_inv_column );
    }

    if( !map_column.empty() && is_overflown() ) {
        map_column.move_entries_to( own_inv_column );
    }
}

void inventory_selector::prepare_layout()
{
    if( layout_is_valid ) {
        return;
    }
    // This block adds categories and should go before any width evaluations
    for( auto &column : columns ) {
        column->prepare_paging();
    }
    // Handle screen overflow
    rearrange_columns();
    // If we have a single column and it occupies more than a half of
    // the available with -> expand it
    auto visible_columns = get_visible_columns();
    if( visible_columns.size() == 1 && are_columns_centered() ) {
        visible_columns.front()->set_width( getmaxx( w_inv ) - 2 );
    }

    long custom_invlet = '0';
    for( auto &column : columns ) {
        column->prepare_paging();
        custom_invlet = column->reassign_custom_invlets( p, custom_invlet, '9' );
    }

    refresh_active_column();

    layout_is_valid = true;
}


void inventory_selector::draw_header( WINDOW *w ) const
{
    trim_and_print( w, 0, 1, getmaxx( w ) - 1, c_ltgray, title.c_str() );
    if( has_available_choices() ) {
        trim_and_print( w, 1, 1, getmaxx( w ) - 1, c_dkgray, hint.c_str() );
    } else {
        trim_and_print( w, 1, 1, getmaxx( w ) - 1, c_red, _( "There are no available choices." ) );
    }

    if( !display_stats ) {
        return;
    }

    const player &dummy = get_player_for_stats();

    static const int stats_count = 2;
    static const int cells_count = 4;

    using stat = std::array<std::string, cells_count>;
    const auto disp = []( const std::string &caption, int cur_value, int max_value,
                          const std::function<std::string( int )> disp_func ) -> stat {
        const std::string color = string_from_color( cur_value > max_value ? c_red : c_ltgray );
        return {{ caption,
                  string_format( "<color_%s>%s</color>", color.c_str(), disp_func( cur_value ).c_str() ), "/",
                  string_format( "<color_ltgray>%s</color>", disp_func( max_value ).c_str() )
        }};
    };

    const std::array<stat, stats_count> stats = {{
        disp( string_format( _( "Weight (%s):" ), weight_units() ), dummy.weight_carried(),
              dummy.weight_capacity(), []( int w ) {
            return string_format( "%.1f", convert_weight( w ) );
        } ),
        disp( _( "Volume:" ), dummy.volume_carried(), dummy.volume_capacity(), []( int v ) {
            return std::to_string( v );
        } )
    }};

    std::array<int, cells_count> widths;
    for( int i = 0; i < cells_count; ++i ) {
        widths[i] = std::max( utf8_width( stats[0][i], true ), utf8_width( stats[1][i], true ) );
    }
    widths[1] += 1;

    int x = std::accumulate( widths.begin(), widths.end(), 0 ) + 1;
    nc_color base_color = c_dkgray;

    for( int i = 0; i < cells_count - 1; ++i ) {
        x -= widths[i];
        right_print( w, 0, x, c_dkgray, stats[0][i].c_str() );
        right_print( w, 1, x, c_dkgray, stats[1][i].c_str() );
    }
    print_colored_text( w, 0, getmaxx( w ) - x, base_color, base_color, stats[0][cells_count - 1].c_str() );
    print_colored_text( w, 1, getmaxx( w ) - x, base_color, base_color, stats[1][cells_count - 1].c_str() );
}

void inventory_selector::draw_footer( WINDOW *w ) const
{
    const std::string msg_str = ( navigation == navigation_mode::CATEGORY )
                                ? _( "Category selection; [TAB] switches mode, arrows select." )
                                : _( "Item selection; [TAB] switches mode, arrows select." );
    const nc_color msg_color = ( navigation == navigation_mode::CATEGORY ) ? h_white : c_ltgray;

    if( are_columns_centered() ) {
        center_print( w, getmaxy( w ) - 1, msg_color, msg_str.c_str() );
    } else {
        trim_and_print( w, getmaxy( w ) - 1, 1, getmaxx( w ), msg_color, msg_str.c_str() );
    }
}

void inventory_selector::refresh_window() const
{
    werase( w_inv );
    draw_header( w_inv );
    draw_columns( w_inv );
    wrefresh( w_inv );
}

void inventory_selector::update()
{
    prepare_layout();
    refresh_window();
}

void inventory_selector::draw_columns( WINDOW *w ) const
{
    const auto columns = get_visible_columns();
    const int free_space = getmaxx( w ) - get_visible_columns_width();
    const int max_gap = ( columns.size() > 1 ) ? free_space / ( int( columns.size() ) - 1 ) : 0;
    const int gap = are_columns_centered() ? max_gap : std::min<int>( max_gap, 8 );
    const int gap_rounding_error = ( are_columns_centered() &&
                                     columns.size() > 1 ) ? free_space % ( columns.size() - 1 ) : 0;

    size_t x = 1;
    size_t y = 2;
    size_t active_x = 0;

    for( const auto &column : columns ) {
        if( &column == &columns.back() ) {
            x += gap_rounding_error;
        }

        if( !is_active_column( *column ) ) {
            column->draw( w, x, y );
        } else {
            active_x = x;
        }

        if( column->pages_count() > 1 ) {
            mvwprintw( w, getmaxy( w ) - 2, x, _( "Page %d/%d" ),
                       column->page_index() + 1, column->pages_count() );
        }

        x += column->get_width() + gap;
    }

    get_active_column().draw( w, active_x, y );
    if( empty() ) {
        center_print( w, getmaxy( w ) / 2, c_dkgray, _( "Your inventory is empty." ) );
    }
}

inventory_selector::inventory_selector( const player &p, const inventory_selector_preset &preset )
    : p( p )
    , preset( preset )
    , own_inv_column( preset )
    , own_gear_column( preset )
    , map_column( preset )
    , columns()
    , w_inv( newwin( TERMY, TERMX, VIEW_OFFSET_Y, VIEW_OFFSET_X ) )
    , ctxt( "INVENTORY" )
{
    ctxt.register_action( "DOWN", _( "Next item" ) );
    ctxt.register_action( "UP", _( "Previous item" ) );
    ctxt.register_action( "RIGHT", _( "Confirm" ) );
    ctxt.register_action( "LEFT", _( "Switch inventory/worn" ) );
    ctxt.register_action( "CONFIRM", _( "Mark selected item" ) );
    ctxt.register_action( "QUIT", _( "Cancel" ) );
    ctxt.register_action( "CATEGORY_SELECTION" );
    ctxt.register_action( "NEXT_TAB", _( "Page down" ) );
    ctxt.register_action( "PREV_TAB", _( "Page up" ) );
    ctxt.register_action( "HOME", _( "Home" ) );
    ctxt.register_action( "END", _( "End" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" ); // For invlets

    append_column( own_inv_column );
    append_column( map_column );
    append_column( own_gear_column );
}

inventory_selector::~inventory_selector()
{
    if( w_inv != NULL ) {
        werase( w_inv );
        delwin( w_inv );
    }
    g->refresh_all();
}

bool inventory_selector::empty() const
{
    return items.empty();
}

bool inventory_selector::has_available_choices() const
{
    return std::any_of( items.begin(), items.end(), [ this ]( const item_location &loc ) {
        return preset.is_enabled( loc );
    } );
}

void inventory_selector::on_input( const inventory_input &input )
{
    if( input.action == "CATEGORY_SELECTION" ) {
        toggle_navigation_mode();
    } else if( input.action == "LEFT" ) {
        toggle_active_column();
    } else {
        for( auto &column : columns ) {
            column->on_input( input );
        }
        refresh_active_column(); // Columns can react to input by losing their activation capacity
    }
}

void inventory_selector::on_change( const inventory_entry &entry )
{
    for( auto &column : columns ) {
        column->on_change( entry );
    }
    refresh_active_column(); // Columns can react to changes by losing their activation capacity
}

const std::vector<inventory_column *> &inventory_selector::get_all_columns() const
{
    return columns;
}

std::vector<inventory_column *> inventory_selector::get_visible_columns() const
{
    std::vector<inventory_column *> res;
    for( auto &column : columns ) {
        if( column->visible() ) {
            res.push_back( column );
        }
    }
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

size_t inventory_selector::get_visible_columns_width() const
{
    static const int padding = 1 + 1; // left and right
    const auto visible_columns = get_visible_columns();
    return std::accumulate( visible_columns.begin(), visible_columns.end(), padding,
                            []( const size_t &lhs, const inventory_column *column ) {
        return lhs + ( lhs != 0 ? min_column_gap : 0 ) + column->get_width();
    } );
}

double inventory_selector::get_columns_occupancy_ratio() const
{
    const int available_width = getmaxx( w_inv );
    const int free_width = available_width - get_visible_columns_width();
    return free_width > 0 && available_width > 0 ? 1.0 - double( free_width ) / available_width : 1.0;
}

bool inventory_selector::are_columns_centered() const {
    return get_columns_occupancy_ratio() >= min_ratio_to_center;
}

void inventory_selector::toggle_active_column()
{
    size_t index = active_column_index;

    do {
        index = ( index + 1 < columns.size() ) ? index + 1 : 0;
    } while( index != active_column_index && !get_column( index ).activatable() );

    set_active_column( index );
}

void inventory_selector::toggle_navigation_mode()
{
    static const std::map<navigation_mode, navigation_mode> next_mode = {
        { navigation_mode::ITEM, navigation_mode::CATEGORY },
        { navigation_mode::CATEGORY, navigation_mode::ITEM }
    };

    navigation = next_mode.at( navigation );
    for( auto &column : columns ) {
        column->set_mode( navigation );
    }
}

void inventory_selector::append_column( inventory_column &column )
{
    column.set_mode( navigation );
    column.set_height( getmaxy( w_inv ) - 5 );

    if( columns.empty() ) {
        column.on_activate();
    }

    columns.push_back( &column );
}

item_location inventory_pick_selector::execute()
{
    while( true ) {
        update();

        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            return input.entry->get_location().clone();
        } else if( input.action == "QUIT" ) {
            static const item_location null_location;
            return null_location.clone();
        } else if( input.action == "CONFIRM" ) {
            return get_active_column().get_selected().get_location().clone();
        } else if( input.action == "RIGHT" ) {
            toggle_active_column();
        } else {
            on_input( input );
        }
    }
}

inventory_multiselector::inventory_multiselector( const player &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title ) :
    inventory_selector( p, preset ),
    selection_col( new selection_column( "SELECTION_COLUMN", selection_column_title ) )
{
    for( auto &column : get_all_columns() ) {
        column->set_multiselect( true );
    }
    append_column( *selection_col );
}

void inventory_multiselector::rearrange_columns()
{
    selection_col->set_visibility( !is_overflown() );
    inventory_selector::rearrange_columns();
}

void inventory_multiselector::on_entry_add( const inventory_entry &entry )
{
    if( entry.is_selectable() ) {
        static_cast<selection_column *>( selection_col.get() )->expand_to_fit( entry );
    }
}

inventory_compare_selector::inventory_compare_selector( const player &p ) :
    inventory_multiselector( p, default_preset, _( "ITEMS TO COMPARE" ) ) {}

std::pair<const item *, const item *> inventory_compare_selector::execute()
{
    while( true ) {
        update();

        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            toggle_entry( input.entry );
        } else if( input.action == "RIGHT" ) {
            const auto selection( get_active_column().get_all_selected() );

            for( auto &entry : selection ) {
                if( entry->chosen_count == 0 || selection.size() == 1 ) {
                    toggle_entry( entry );
                    if( compared.size() == 2 ) {
                        break;
                    }
                }
            }
        } else if( input.action == "QUIT" ) {
            return std::make_pair( nullptr, nullptr );
        } else {
            on_input( input );
        }

        if( compared.size() == 2 ) {
            const auto res = std::make_pair( &compared.back()->get_item(), &compared.front()->get_item() );
            toggle_entry( compared.back() );
            return res;
        }
    }
}

void inventory_compare_selector::toggle_entry( inventory_entry *entry )
{
    const auto iter = std::find( compared.begin(), compared.end(), entry );

    entry->chosen_count = ( iter == compared.end() ) ? 1 : 0;

    if( entry->chosen_count != 0 ) {
        compared.push_back( entry );
    } else {
        compared.erase( iter );
    }

    on_change( *entry );
}

inventory_drop_selector::inventory_drop_selector( const player &p,
        const inventory_selector_preset &preset ) :
    inventory_multiselector( p, preset, _( "ITEMS TO DROP" ) ) {}

std::list<std::pair<int, int>> inventory_drop_selector::execute()
{
    int count = 0;
    while( true ) {
        update();

        const inventory_input input = get_input();

        if( input.ch >= '0' && input.ch <= '9' ) {
            count = std::min( count, INT_MAX / 10 - 10 );
            count *= 10;
            count += input.ch - '0';
        } else if( input.entry != nullptr ) {
            set_drop_count( *input.entry, count );
            count = 0;
        } else if( input.action == "RIGHT" ) {
            for( const auto &entry : get_active_column().get_all_selected() ) {
                set_drop_count( *entry, count );
            }
            count = 0;
        } else if( input.action == "CONFIRM" ) {
            break;
        } else if( input.action == "QUIT" ) {
            return std::list<std::pair<int, int> >();
        } else {
            on_input( input );
            count = 0;
        }
    }

    std::list<std::pair<int, int>> dropped_pos_and_qty;

    for( auto drop_pair : dropping ) {
        dropped_pos_and_qty.push_back( std::make_pair( p.get_item_position( drop_pair.first ),
                                       drop_pair.second ) );
    }

    return dropped_pos_and_qty;
}

const player &inventory_drop_selector::get_player_for_stats() const
{
    std::map<item *, int> dummy_dropping;

    dummy.reset( new player( p ) );

    for( const auto &elem : dropping ) {
        dummy_dropping[&dummy->i_at( p.get_item_position( elem.first ) )] = elem.second;
    }
    for( auto &elem : dummy_dropping ) {
        if( elem.first->count_by_charges() ) {
            elem.first->mod_charges( -elem.second );
        } else {
            const int pos = dummy->get_item_position( elem.first );
            for( int i = 0; i < elem.second; ++i ) {
                dummy->i_rem( pos );
            }
        }
    }

    return *dummy;
}

void inventory_drop_selector::set_drop_count( inventory_entry &entry, size_t count )
{
    const auto iter = dropping.find( &entry.get_item() );

    if( count == 0 && iter != dropping.end() ) {
        entry.chosen_count = 0;
        dropping.erase( iter );
    } else {
        entry.chosen_count = ( count == 0 )
                             ? entry.get_available_count()
                             : std::min( count, entry.get_available_count() );
        dropping[&entry.get_item()] = entry.chosen_count;
    }

    on_change( entry );
}
