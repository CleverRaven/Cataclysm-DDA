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

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

struct inventory_input {
    std::string action;
    long ch;
    inventory_entry *entry;
};

class selection_column_preset: public inventory_selector_preset
{
    public:
        selection_column_preset() = default;

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

class selection_column : public inventory_column
{
    public:
        selection_column( const std::string &id, const std::string &name ) :
            inventory_column( preset ),
            selected_cat( new item_category( id, name, 0 ) ) {}

        void reserve_width_for( const inventory_column &column ) {
            size_t width = 0;
            for( const auto &entry : column.get_entries() ) {
                width = std::max( get_width( &entry ), width );
            }
            set_width( width );
        }

        virtual bool activatable() const override {
            return inventory_column::activatable() && pages_count() > 1;
        }

        virtual bool allows_selecting() const override {
            return false;
        }

        virtual void prepare_paging() override {
            inventory_column::prepare_paging();
            if( empty() ) { // Category must always persist
                add_entry( selected_cat.get() );
            }
        }

        virtual void on_change( const inventory_entry &entry ) override;

    private:
        const std::unique_ptr<item_category> selected_cat;
        selection_column_preset preset;
};

bool inventory_entry::operator == ( const inventory_entry &other ) const
{
    return get_category_ptr() == other.get_category_ptr() && location == other.location;
}

inventory_entry::inventory_entry( const std::shared_ptr<item_location> &location,
                                  const item_category *custom_category ) :
    inventory_entry( location, ( location->get_item() != nullptr ) ? 1 : 0, custom_category ) {}

bool inventory_entry::is_item() const
{
    return location->get_item() != nullptr;
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
    if( location->get_item() == nullptr ) {
        static const item nullitem;
        debugmsg( "Tried to access an empty item." );
        return nullitem;
    }
    return *location->get_item();
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

inventory_selector_preset::inventory_selector_preset() : bug_text( _( "it's a bug!" ) )
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
    return is_shown( *location.get_item() );
}

bool inventory_selector_preset::is_enabled( const item & ) const
{
    return true;
}

bool inventory_selector_preset::is_enabled( const item_location &location ) const
{
    return is_enabled( *location.get_item() );
}

int inventory_selector_preset::get_rank( const item & ) const
{
    return 0;
}

int inventory_selector_preset::get_rank( const item_location &location ) const
{
    return get_rank( *location.get_item() );
}

nc_color inventory_selector_preset::get_color( const item &it ) const
{
    return it.color_in_inventory();
}

nc_color inventory_selector_preset::get_color( const item_location &location ) const
{
    return get_color( *location.get_item() );
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

void inventory_selector_preset::append_cell( const item_text_func &func, const std::string &title )
{
    append_cell( [ func ]( const inventory_entry & entry ) { // Don't pass by reference
        return func( entry.get_item() );
    }, title );
}

void inventory_selector_preset::append_cell( const item_location_text_func &func,
        const std::string &title )
{
    append_cell( [ func ]( const inventory_entry & entry ) { // Don't pass by reference
        return func( entry.get_location() );
    }, title );
}

void inventory_selector_preset::append_cell( const entry_text_func &func, const std::string &title )
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

size_t inventory_column::get_cell_width( const inventory_entry &entry, size_t cell_index ) const
{
    size_t res = preset.get_cell_width( entry, cell_index );
    if( cell_index == 0 ) {
        res += get_entry_indent( entry );
    }
    return res;
}

size_t inventory_column::get_max_cell_width( size_t cell_index ) const
{
    size_t res = 0;
    for( const auto &entry : entries ) {
        res = std::max( get_cell_width( entry, cell_index ), res );
    }
    return res;
}

size_t inventory_column::get_width( const inventory_entry *entry ) const
{
    return std::max( get_min_width( entry ), assigned_width );
}

size_t inventory_column::get_min_width( const inventory_entry *entry ) const
{
    const size_t cells_count = preset.get_cells_count();
    size_t res = 0;

    for( size_t index = 0; index < cells_count; ++index ) {
        if( entry == nullptr ) {
            res += get_max_cell_width( index );
        } else {
            res += get_cell_width( *entry, index );
        }
    }
    if( cells_count > 1 ) {
        res += min_cell_margin * ( cells_count - 1 );
    }
    return res;
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
    if( allows_selecting() && multiselect ) {
        res += 2;
    }
    return res;
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
        return cur.get_category_ptr() == entry.get_category_ptr()
            || cur.get_category_ptr()->sort_rank <= entry.get_category_ptr()->sort_rank;
    } );
    entries.insert( iter.base(), entry );
    paging_is_valid = false;
}

void inventory_column::add_entries( const inventory_column &source )
{
    for( const auto &entry : source.entries ) {
        if( entry.is_item() ) {
            add_entry( entry );
        }
    }
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
    // Recover categories according to the new number of entries per page
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
    }
    // Select the uppermost possible entry
    select( 0, 1 );

    paging_is_valid = true;
}

void inventory_column::draw( WINDOW *win, size_t x, size_t y ) const
{
    std::vector<size_t> cell_widths;
    // Cache expensive calculations of cell widths
    cell_widths.reserve( preset.get_cells_count() );
    for( size_t cell_index = 0; cell_index < preset.get_cells_count(); ++cell_index ) {
        cell_widths.push_back( get_max_cell_width( cell_index ) );
    }
    // Calculate cell margin and rounding error
    const int num_of_margins = std::max( int( preset.get_cells_count() ) - 1, 0 );
    const int room_for_margins = std::max( int( assigned_width - get_min_width() ), 0 );
    const int cell_margin = std::max( num_of_margins > 0 ? room_for_margins / num_of_margins : 0,
                                      min_cell_margin );
    const int column_width_sum = std::accumulate( cell_widths.begin(), cell_widths.end(), 0 );
    const int rounding_error = std::max( int( get_width() - column_width_sum - num_of_margins *
                                         cell_margin ), 0 );
    const int first_cell_margin = cell_margin + rounding_error;
    // Do the actual drawing
    for( size_t index = page_offset, line = 0; index < entries.size() &&
         line < entries_per_page; ++index, ++line ) {
        const auto &entry = entries[index];

        if( entry.is_null() ) {
            continue;
        }

        int x1 = x + get_entry_indent( entry );
        int x2 = x;
        int txe = INT_MAX;

        const int yy = y + line;

        for( size_t cell_index = 0, count = preset.get_cells_count(); cell_index < count; ++cell_index ) {
            const std::string txt = preset.get_cell_text( entry, cell_index );
            const int txt_len = utf8_width( txt, true );

            if( line != 0 && cell_index != 0 && !entry.is_item() ) {
                break; // Don't show duplicated titles
            }

            x2 += cell_widths.at( cell_index );

            const int tx = ( cell_index == 0 ) ? x1 : x2 - txt_len; // Align to the right if not the first
            const int mw = x2 - tx;

            if( !entry.enabled ) {
                trim_and_print( win, yy, tx, mw, c_dkgray, "%s", remove_color_tags( txt ).c_str() );
            } else if( active && entry.is_selectable() && is_selected( entry ) ) {
                trim_and_print( win, yy, tx, mw, h_white, "%s", txt.c_str() );
                if( tx > txe ) { // Need to fill the gap?
                    mvwprintz( win, yy, txe, h_white, std::string( tx - txe, ' ' ).c_str() );
                }
            } else {
                trim_and_print( win, yy, tx, mw, preset.get_cell_color( entry, cell_index ), "%s", txt.c_str() );
            }
            // Compensate the rounding error right after the first cell
            x2 += ( cell_index == 0 ) ? first_cell_margin : cell_margin;
            x1 = x2;
            txe = tx + txt_len;
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

void inventory_column::set_width( const size_t width )
{
    assigned_width = width;
}

void inventory_column::set_height( size_t height ) {
    if( entries_per_page != height ) {
        entries_per_page = height;
        paging_is_valid = false;
    }
}

void selection_column::on_change( const inventory_entry &entry )
{
    inventory_entry my_entry( entry, selected_cat.get() );

    const auto iter = std::find( entries.begin(), entries.end(), my_entry );
    if( my_entry.chosen_count != 0 ) {
        if( iter == entries.end() ) {
            add_entry( my_entry );
        } else {
            iter->chosen_count = my_entry.chosen_count;
        }
    } else {
        entries.erase( iter );
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
std::vector<std::list<item *>> restack_items( const std::list<item>::iterator &from,
                            const std::list<item>::iterator &to )
{
    std::vector<std::list<item *>> res;

    for( auto it = from; it != to; ++it ) {
        auto match = std::find_if( res.begin(), res.end(),
        [ &it ]( const std::list<item *> &e ) {
            return it->stacks_with( *e.back() );
        } );

        if( match != res.end() ) {
            match->push_back( &*it );
        } else {
            res.emplace_back( 1, &*it );
        }
    }

    return res;
}

void inventory_selector::add_item( const std::shared_ptr<item_location> &location,
                                   size_t stack_size,
                                   const std::string &custom_cat_title )
{
    if( !preset.is_shown( *location ) ) {
        return;
    }

    const bool is_custom = !custom_cat_title.empty();
    const item_category *cat_ptr = ( *location )->type->category;

    if( is_custom ) {
        const std::string name =
            trim( string_format( _( "%s %s" ),
                                 to_upper_case( custom_cat_title ).c_str(),
                                 direction_suffix( p.pos(), location->position() ).c_str() ) );

        auto cat_iter = std::find_if( categories.begin(), categories.end(),
        [ &name ]( const item_category & category ) {
            return category.name == name;
        } );
        if( cat_iter == categories.end() ) {
            cat_iter = categories.insert( cat_iter, item_category( name, name,
                                          INT_MIN + int( categories.size() ) ) );
        }
        cat_ptr = &*cat_iter;
    }

    inventory_entry entry( location, stack_size, cat_ptr );

    entry.enabled = preset.is_enabled( entry.get_location() );
    entry.rank = preset.get_rank( entry.get_location() );

    inventory_column *column_ptr;
    if( is_custom ) {
        if( custom_column == nullptr ) {
            custom_column.reset( new inventory_column( preset ) );
        }
        column_ptr = custom_column.get();
    } else {
        if( columns.empty() ) {
            std::shared_ptr<inventory_column> new_column( new inventory_column( preset ) );
            insert_column( columns.end(), new_column );
        }
        column_ptr = columns.back().get();
    }

    column_ptr->add_entry( entry );
}

void inventory_selector::add_items( const std::list<item>::iterator &from,
                                    const std::list<item>::iterator &to,
                                    const std::string &title,
                                    const std::function<std::shared_ptr<item_location>( item * )> &locator )
{
    const auto &stacks = restack_items( from, to );
    for( const auto &stack : stacks ) {
        add_item( locator( stack.front() ), stack.size(), title );
    }
}

void inventory_selector::add_character_items( Character &character )
{
    for( const auto &stack: character.inv.slice() ) {
        add_item( std::make_shared<item_location>( character, &stack->front() ), stack->size() );
    }

    if( !character.weapon.is_null() ) {
        const std::string cat_title = _( "WEAPON HELD" );
        add_item( std::make_shared<item_location>( character, &character.weapon ), 1, cat_title );
    }

    if( !character.worn.empty() ) {
        const std::string cat_title = _( "ITEMS WORN" );
        for( auto &it : character.worn ) {
            add_item( std::make_shared<item_location>( character, &it ), 1, cat_title );
        }
    }
}

void inventory_selector::add_map_items( const tripoint &target )
{
    if( g->m.accessible_items( p.pos(), target, rl_dist( p.pos(), target ) ) ) {
        auto items = g->m.i_at( target );
        add_items( items.begin(), items.end(), g->m.name( target ), [ &target ]( item * it ) {
            return std::make_shared<item_location>( target, it );
        } );
    }
}

void inventory_selector::add_vehicle_items( const tripoint &target )
{
    int part = -1;
    vehicle *veh = g->m.veh_at( target, part );

    if( veh != nullptr && ( part = veh->part_with_feature( part, "CARGO" ) ) >= 0 ) {
        auto items = veh->get_items( part );
        add_items( items.begin(), items.end(), veh->parts[part].name(), [ veh, part ]( item * it ) {
            return std::make_shared<item_location>( vehicle_cursor( *veh, part ), it );
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

void inventory_selector::set_title( const std::string &title )
{
    this->title = title;
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

void inventory_selector::prepare_columns()
{
    // First reset width of all columns
    for( auto &column : columns ) {
        column->set_width( column->get_min_width() );
    }
    // See what to do with the custom column
    if( custom_column != nullptr ) {
        if( columns.empty() || column_can_fit( *custom_column ) ) {
            // Make the column second if possible
            const auto position = ( !columns.empty() ) ? std::next( columns.begin() ) : columns.begin();
            insert_column( position, custom_column );
        } else {
            columns.front()->add_entries( *custom_column );
        }
        custom_column = nullptr;
    }

    long custom_invlet = '0';
    for( auto &column : columns ) {
        column->prepare_paging();
        custom_invlet = column->reassign_custom_invlets( p, custom_invlet, '9' );
    }
    // If we have a single column and it occupies more than a half of
    // the available with -> expand it
    if( columns.size() == 1 && get_columns_occupancy_ratio() >= 0.5 ) {
        columns.front()->set_width( getmaxx( w_inv ) - 2 );
    }

    refresh_active_column();
}

void inventory_selector::draw_inv_weight_vol( WINDOW *w, int weight_carried, units::volume vol_carried,
        units::volume vol_capacity) const
{
    // Print weight
    mvwprintw( w, 0, 32, _( "Weight (%s): " ), weight_units() );
    nc_color weight_color;
    if( weight_carried > p.weight_capacity() ) {
        weight_color = c_red;
    } else {
        weight_color = c_ltgray;
    }
    wprintz( w, weight_color, "%6.1f", convert_weight( weight_carried ) + 0.05 ); // +0.05 to round up;
    wprintz( w, c_ltgray, "/%-6.1f", convert_weight( p.weight_capacity() ) );

    // Print volume
    mvwprintw(w, 0, 61, _("Volume (ml): "));
    if (vol_carried > vol_capacity) {
        wprintz(w, c_red, "%3d", to_milliliter( vol_carried ) );
    } else {
        wprintz(w, c_ltgray, "%3d", to_milliliter( vol_carried ) );
    }
    wprintw(w, "/%-3d", to_milliliter( vol_capacity ) );
}

void inventory_selector::draw_inv_weight_vol( WINDOW *w ) const
{
    draw_inv_weight_vol( w, p.weight_carried(), p.volume_carried(), p.volume_capacity() );
}

void inventory_selector::refresh_window() const
{
    werase( w_inv );
    draw( w_inv );
    wrefresh( w_inv );
}

void inventory_selector::draw( WINDOW *w ) const
{
    mvwprintw( w, 0, 0, title.c_str() );

    // Position of inventory columns is adaptive
    const bool center_align = get_columns_occupancy_ratio() >= 0.5;

    const int free_space = getmaxx( w ) - get_columns_width();
    const int max_gap = ( columns.size() > 1 ) ? free_space / ( int( columns.size() ) - 1 ) : 0;
    const int gap = center_align ? max_gap : std::min<int>( max_gap, 8 );
    const int gap_rounding_error = ( center_align &&
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

    const std::string msg_str = ( navigation == navigation_mode::CATEGORY )
                                ? _( "Category selection; [TAB] switches mode, arrows select." )
                                : _( "Item selection; [TAB] switches mode, arrows select." );
    const nc_color msg_color = ( navigation == navigation_mode::CATEGORY ) ? h_white : c_ltgray;

    if( center_align ) {
        center_print( w, getmaxy( w ) - 1, msg_color, msg_str.c_str() );
    } else {
        trim_and_print( w, getmaxy( w ) - 1, 1, getmaxx( w ), msg_color, msg_str.c_str() );
    }
}

inventory_selector::inventory_selector( const player &p, const inventory_selector_preset &preset )
    : p( p )
    , preset( preset )
    , columns()
    , w_inv( newwin( TERMY, TERMX, VIEW_OFFSET_Y, VIEW_OFFSET_X ) )
    , active_column_index( 0 )
    , navigation( navigation_mode::ITEM )
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
    for( const auto &column : columns ) {
        if( !column->empty() ) {
            return false;
        }
    }
    return custom_column == nullptr || custom_column->empty();
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

size_t inventory_selector::get_columns_width() const
{
    size_t res = 2; // padding on both sides

    for( const auto &column : columns ) {
        res += column->get_width();
    }
    return res;
}

double inventory_selector::get_columns_occupancy_ratio() const
{
    const int available_width = getmaxx( w_inv );
    const int free_width = available_width - get_columns_width();
    return free_width > 0 && available_width > 0 ? 1.0 - double( free_width ) / available_width : 1.0;
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

void inventory_selector::insert_column( decltype( columns )::iterator position,
                                        std::shared_ptr<inventory_column> &column )
{
    assert( column != nullptr );

    column->set_mode( navigation );
    column->set_height( getmaxy( w_inv ) - 5 );

    if( columns.empty() ) {
        column->on_activate();
    }

    columns.insert( position, column );
}

inventory_pick_selector::inventory_pick_selector( const player &p,
        const inventory_selector_preset &preset ) :
    inventory_selector( p, preset ), null_location( new item_location() ) {}

item_location &inventory_pick_selector::execute()
{
    prepare_columns();

    while( true ) {
        refresh_window();

        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            return input.entry->get_location();
        } else if( input.action == "QUIT" ) {
            return *null_location;
        } else if( input.action == "RIGHT" || input.action == "CONFIRM" ) {
            return get_active_column().get_selected().get_location();
        } else {
            on_input( input );
        }
    }
}

void inventory_pick_selector::draw( WINDOW *w ) const
{
    inventory_selector::draw( w );
    mvwprintw( w, 1, 61, _( "Hotkeys:  %d/%d " ), p.allocated_invlets().size(), inv_chars.size() );
    draw_inv_weight_vol( w );
}

inventory_multiselector::inventory_multiselector( const player &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title ) :
    inventory_selector( p, preset ),
    selection_col( new selection_column( "SELECTION_COLUMN", selection_column_title ) ) {}

void inventory_multiselector::prepare_columns()
{
    const auto sel_col = static_cast<selection_column *>( selection_col.get() );

    if( std::find( columns.begin(), columns.end(), selection_col ) == columns.end() ) {
        insert_column( columns.end(), selection_col );
    }
    for( auto &column : columns ) {
        column->set_multiselect( true );
        sel_col->reserve_width_for( *column );
    }
    if( custom_column != nullptr ) {
        custom_column->set_multiselect( true );
        sel_col->reserve_width_for( *custom_column );
    }
    inventory_selector::prepare_columns();
}

inventory_compare_selector::inventory_compare_selector( const player &p ) :
    inventory_multiselector( p, default_preset, _( "ITEMS TO COMPARE" ) ) {}

std::pair<const item *, const item *> inventory_compare_selector::execute()
{
    prepare_columns();

    while( true ) {
        refresh_window();

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

void inventory_compare_selector::draw( WINDOW *w ) const
{
    inventory_selector::draw( w );
    draw_inv_weight_vol( w );
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
    prepare_columns();

    int count = 0;
    while( true ) {
        refresh_window();

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

void inventory_drop_selector::draw( WINDOW *w ) const
{
    inventory_selector::draw( w );
    // Make copy, remove to be dropped items from that
    // copy and let the copy recalculate the volume capacity
    // (can be affected by various traits).
    player tmp( p );
    remove_dropping_items( tmp );
    draw_inv_weight_vol( w, tmp.weight_carried(), tmp.volume_carried(), tmp.volume_capacity() );
    mvwprintw( w, 1, 0, _( "To drop x items, type a number and then the item hotkey." ) );
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

void inventory_drop_selector::remove_dropping_items( player &dummy ) const
{
    std::map<item *, int> dummy_dropping;

    for( const auto &elem : dropping ) {
        dummy_dropping[&dummy.i_at( p.get_item_position( elem.first ) )] = elem.second;
    }
    for( auto &elem : dummy_dropping ) {
        if( elem.first->count_by_charges() ) {
            elem.first->mod_charges( -elem.second );
        } else {
            const int pos = dummy.get_item_position( elem.first );
            for( int i = 0; i < elem.second; ++i ) {
                dummy.i_rem( pos );
            }
        }
    }
}
