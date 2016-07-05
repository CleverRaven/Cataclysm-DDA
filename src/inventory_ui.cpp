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
#include "itype.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

bool inventory_entry::operator == ( const inventory_entry &other ) const {
    return get_category_ptr() == other.get_category_ptr() && location == other.location;
}

bool inventory_entry::is_item() const {
    return location->get_item() != nullptr;
}

bool inventory_entry::is_null() const {
    return get_category_ptr() == nullptr;
}

size_t inventory_entry::get_available_count() const {
    if( is_item() && get_stack_size() == 1 ) {
        return get_item().count_by_charges() ? get_item().charges : 1;
    } else {
        return get_stack_size();
    }
}

const item &inventory_entry::get_item() const {
    if( location->get_item() == nullptr ) {
        static const item nullitem;
        debugmsg( "Tried to access an empty item." );
        return nullitem;
    }
    return *location->get_item();
}

long inventory_entry::get_invlet() const {
    return ( custom_invlet != LONG_MIN ) ? custom_invlet : is_item() ? get_item().invlet : '\0';
}

const item_category *inventory_entry::get_category_ptr() const {
    return ( custom_category != nullptr )
        ? custom_category : is_item() ? &get_item().get_category() : nullptr;
}

inventory_entry *inventory_column::find_by_invlet( long invlet ) const
{
    for( const auto &entry : entries ) {
        if( entry.is_item() && ( entry.get_invlet() == invlet ) ) {
            return const_cast<inventory_entry *>( &entry );
        }
    }
    return nullptr;
}

size_t inventory_column::get_width() const
{
    size_t res = 0;
    for( const auto &entry : entries ) {
        res = std::max( res, get_entry_width( entry ) );
    }
    return res;
}

void inventory_column::select( size_t new_index )
{
    if( new_index != selected_index && new_index < entries.size() ) {
        selected_index = new_index;
        page_offset = selected_index - selected_index % entries_per_page;
    }
}

size_t inventory_column::page_of( size_t index ) const {
    return index / entries_per_page;
}

size_t inventory_column::page_of( const inventory_entry &entry ) const {
    return page_of( std::distance( entries.begin(), std::find( entries.begin(), entries.end(), entry ) ) );
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

const inventory_entry &inventory_column::get_selected() const {
    if( selected_index >= entries.size() ) {
        static const inventory_entry dummy;
        return dummy;
    }
    return entries[selected_index];
}

std::vector<inventory_entry *> inventory_column::get_all_selected() const
{
    std::vector<inventory_entry *> res;

    if( allows_selecting() ) {
        for( const auto &entry : entries ) {
            if( is_selected( entry ) ) {
                res.push_back( const_cast<inventory_entry *>( &entry ) );
            }
        }
    }

    return res;
}

void inventory_column::on_action( const std::string &action )
{
    if( empty() || !active ) {
        return; // ignore
    }

    const auto move_selection = [ this ]( int step ) {
        const auto get_incremented = [ this ]( size_t index, int step ) -> size_t {
            return ( index + step + entries.size() ) % entries.size();
        };

        size_t index = get_incremented( selected_index, step );
        while( entries[index] != get_selected() && ( !entries[index].is_item() || is_selected_by_category( entries[index] ) ) ) {
            index = get_incremented( index, ( step > 0 ? 1 : -1 ) );
        }

        select( index );
    };

    if( action == "DOWN" ) {
        move_selection( 1 );
    } else if( action == "UP" ) {
        move_selection( -1 );
    } else if( action == "NEXT_TAB" ) {
        move_selection( std::max( std::min<int>( entries_per_page, entries.size() - selected_index - 1 ), 1 ) );
    } else if( action == "PREV_TAB" ) {
        move_selection( std::min( std::max<int>( -entries_per_page, -selected_index + 1 ), -1 ) );
    } else if( action == "HOME" ) {
        select( 1 );
    } else if( action == "END" ) {
        select( entries.size() - 1 );
    }
}

void inventory_column::add_entry( const inventory_entry &entry )
{
    if( entry.is_item() ) {
        const auto iter = std::find_if( entries.rbegin(), entries.rend(),
            [ &entry ]( const inventory_entry &cur ) {
                return cur.get_category_ptr() == entry.get_category_ptr()
                    || cur.get_category_ptr()->sort_rank <= entry.get_category_ptr()->sort_rank;
            } );
        entries.insert( iter.base(), entry );
    }
}

void inventory_column::add_entries( const inventory_column &source )
{
    for( const auto &entry : source.entries ) {
        add_entry( entry );
    }
}

void inventory_column::prepare_paging( size_t new_entries_per_page )
{
    if( new_entries_per_page != 0 ) { // Keep default otherwise
        entries_per_page = new_entries_per_page;
    }
    const auto new_end = std::remove_if( entries.begin(), entries.end(), []( const inventory_entry &entry ) {
        return !entry.is_item();
    } );
    entries.erase( new_end, entries.end() );

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
}

std::string inventory_column::get_entry_text( const inventory_entry &entry ) const
{
    std::ostringstream res;

    if( entry.is_item() ) {
        if( OPTIONS["ITEM_SYMBOLS"] ) {
            res << entry.get_item().symbol() << ' ';
        }

        if( multiselect ) {
            if( entry.chosen_count == 0 ) {
                res << "<color_c_dkgray>-";
            } else if( entry.chosen_count >= entry.get_available_count() ) {
                res << "<color_c_ltgreen>+";
            } else {
                res << "<color_c_ltgreen>#";
            }
            res << " </color>";
        }

        const size_t count = entry.get_stack_size();
        if( count > 1 ) {
            res << count << ' ';
        }
        res << entry.get_item().display_name( count );
    } else if( entry.get_category_ptr() != nullptr ) {
        res << entry.get_category_ptr()->name;
    }
    return res.str();
}

nc_color inventory_column::get_entry_color( const inventory_entry &entry ) const
{
    if( entry.is_item() ) {
        return ( active && is_selected( entry ) )
            ? h_white
            : ( custom_colors && entry.custom_color != c_unset )
                ? entry.custom_color
                : entry.get_item().color_in_inventory();
    } else {
        return c_magenta;
    }
}

size_t inventory_column::get_entry_indent( const inventory_entry &entry ) const {
    return entry.is_item() ? 2 : 0;
}

size_t inventory_column::get_entry_width( const inventory_entry &entry ) const {
    return get_entry_indent( entry ) + utf8_width( get_entry_text( entry ), true );
}

void inventory_column::draw( WINDOW *win, size_t x, size_t y ) const
{
    for( size_t i = page_offset, line = 0; i < entries.size() && line < entries_per_page; ++i, ++line ) {
        const auto &entry = entries[i];
        if( entry.is_null() ) {
            continue;
        }

        trim_and_print( win, y + line, x + get_entry_indent( entry ), getmaxx( win ) - x,
                        get_entry_color( entry ), "%s", get_entry_text( entry ).c_str() );

        const auto invlet = entry.get_invlet();
        if( invlet != '\0' ) {
            const nc_color invlet_color = g->u.assigned_invlet.count( invlet ) ? c_yellow : c_white;
            mvwputch( win, y + line, x, invlet_color, invlet );
        }
    }
}

void selection_column::reserve_width_for( const inventory_column &column )
{
    for( const auto &entry : column.get_entries() ) {
        if( entry.is_item() ) {
            reserved_width = std::max( reserved_width, get_entry_width( entry ) );
        }
    }
}

void selection_column::prepare_paging( size_t new_entries_per_page )
{
    inventory_column::prepare_paging( new_entries_per_page );
    if( entries.empty() ) { // Category must always persist
        entries.emplace_back( &selected_cat );
    }
}

void selection_column::on_change( const inventory_entry &entry )
{
    inventory_entry my_entry( entry, &selected_cat );

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
        select( std::distance( entries.begin(), select_iter ) );
    } else {
        select( entries.empty() ? 0 : entries.size() - 1 ); // Just select the last one
    }
}

std::string selection_column::get_entry_text( const inventory_entry &entry ) const
{
    std::ostringstream res;

    if( entry.is_item() ) {
        if( OPTIONS["ITEM_SYMBOLS"] ) {
            res << entry.get_item().symbol() << ' ';
        }

        const size_t available_count = entry.get_available_count();
        if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
            res << string_format( _( "%d of %d"), entry.chosen_count, available_count ) << ' ';
        } else if( available_count != 1 ) {
            res << available_count << ' ';
        }
        res << entry.get_item().display_name( available_count );
    } else {
        res << inventory_column::get_entry_text( entry ) << ' ';

        if( entries.size() > 1 ) {
            res << string_format( "(%d)", entries.size() - 1 );
        } else {
            res << _( "(NONE)" );
        }
    }

    return res.str();
}

// @todo Move it into some 'item_stack' class.
std::vector<std::list<item *>> restack_items( const std::list<item>::const_iterator &from,
                                              const std::list<item>::const_iterator &to )
{
    std::vector<std::list<item *>> res;

    for( auto it = from; it != to; ++it ) {
        auto match = std::find_if( res.begin(), res.end(),
            [ &it ]( const std::list<item *> &e ) {
                return it->stacks_with( *const_cast<item *>( e.back() ) );
            } );

        if( match != res.end() ) {
            match->push_back( const_cast<item *>( &*it ) );
        } else {
            res.emplace_back( 1, const_cast<item *>( &*it ) );
        }
    }

    return res;
}

void inventory_selector::add_custom_items( const std::list<item>::const_iterator &from,
                                           const std::list<item>::const_iterator &to,
                                           const std::string &title,
                                           const std::function<std::shared_ptr<item_location>( item * )> &locator )
{
    const auto &stacks = restack_items( from, to );

    for( const auto &stack : stacks ) {
        const auto &location = locator( stack.front() );
        if( filter( *location ) ) {
            const std::string name = trim( string_format( _( "%s %s" ), to_upper_case( title ).c_str(),
                                                          direction_suffix( u.pos(), location->position() ).c_str() ) );
            if( categories.empty() || categories.back().id != name ) {
                categories.emplace_back( name, name, INT_MIN + int( categories.size() ) );
            }
            if( custom_column == nullptr ) {
                custom_column.reset( new inventory_column() );
            }
            const long invlet = ( cur_custom_invlet <= max_custom_invlet ) ? cur_custom_invlet++ : '\0';
            custom_column->add_entry( inventory_entry( location, stack.size(), &categories.back(), c_unset, invlet ) );
        }
    }
}

void inventory_selector::add_map_items( const tripoint &target )
{
    if( g->m.accessible_items( u.pos(), target, rl_dist( u.pos(), target ) ) ) {
        const auto items = g->m.i_at( target );
        add_custom_items( items.begin(), items.end(), g->m.name( target ), [ &target ]( item *it ) {
            return std::make_shared<item_location>( target, it );
        } );
    }
}

void inventory_selector::add_vehicle_items( const tripoint &target )
{
    int part = -1;
    vehicle *veh = g->m.veh_at( target, part );

    if( veh != nullptr && ( part = veh->part_with_feature( part, "CARGO" ) ) >= 0 ) {
        const auto items = veh->get_items( part );
        add_custom_items( items.begin(), items.end(), veh->parts[part].name(), [ veh, part ]( item *it ) {
            return std::make_shared<item_location>( vehicle_cursor( *veh, part ), it );
        } );
    }
}

void inventory_selector::add_nearby_items( int radius )
{
    if( radius >= 0 ) {
        for( const auto &pos : closest_tripoints_first( radius, u.pos() ) ) {
            add_map_items( pos );
            add_vehicle_items( pos );
        }
    }
}

inventory_entry *inventory_selector::find_entry_by_invlet( long invlet ) const
{
    for( const auto &column : columns ) {
        const auto res = column->find_by_invlet( invlet );
        if( res != nullptr ) {
            return res;
        }
    }
    return nullptr;
}

void inventory_selector::prepare_columns( bool multiselect )
{
    for( auto &column : columns ) {
        column->set_multiselect( multiselect );
    }

    if( custom_column != nullptr ) {
        if( columns.empty() || column_can_fit( *custom_column ) ) {
            // Make the column second if possible
            const auto position = ( !columns.empty() ) ? std::next( columns.begin() ) : columns.begin();

            custom_column->set_multiselect( multiselect );
            insert_column( position, custom_column );
        } else {
            columns.front()->add_entries( *custom_column );
            custom_column.release();
        }
    }

    for( auto &column : columns ) {
        column->prepare_paging( getmaxy( w_inv ) - 5 );
    }

    refresh_active_column();
}

void inventory_selector::print_inv_weight_vol(int weight_carried, int vol_carried,
        int vol_capacity) const
{
    // Print weight
    mvwprintw(w_inv, 0, 32, _("Weight (%s): "), weight_units());
    nc_color weight_color;
    if (weight_carried > u.weight_capacity()) {
        weight_color = c_red;
    } else {
        weight_color = c_ltgray;
    }
    wprintz(w_inv, weight_color, "%6.1f", convert_weight(weight_carried) + 0.05 ); // +0.05 to round up;
    wprintz(w_inv, c_ltgray, "/%-6.1f", convert_weight(u.weight_capacity()));

    // Print volume
    mvwprintw(w_inv, 0, 61, _("Volume: "));
    if (vol_carried > vol_capacity) {
        wprintz(w_inv, c_red, "%3d", vol_carried);
    } else {
        wprintz(w_inv, c_ltgray, "%3d", vol_carried);
    }
    wprintw(w_inv, "/%-3d", vol_capacity);
}

void inventory_selector::display( const std::string &title, selector_mode mode ) const
{
    werase(w_inv);
    mvwprintw(w_inv, 0, 0, title.c_str());

    // Position of inventory columns is adaptive. They're aligned to the left if they occupy less than 2/3 of the screen.
    // Otherwise they're aligned symmetrically to the center of the screen.
    static const float min_ratio_to_center = 1.f / 3;
    const int free_space = getmaxx( w_inv ) - get_columns_width();
    const bool center_align = std::abs( float( free_space ) / getmaxx( w_inv ) ) <= min_ratio_to_center;

    const int max_gap = ( columns.size() > 1 ) ? free_space / ( int( columns.size() ) - 1 ) : 0;
    const int gap = center_align ? max_gap : std::min<int>( max_gap, 4 );
    const int gap_rounding_error = ( center_align && columns.size() > 1 ) ? free_space % ( columns.size() - 1 ) : 0;

    size_t x = 1;
    size_t y = 2;
    size_t active_x = 0;

    for( const auto &column : columns ) {
        if( &column == &columns.back() ) {
            x += gap_rounding_error;
        }

        if( !is_active_column( *column ) ) {
            column->draw( w_inv, x, y );
        } else {
            active_x = x;
        }

        if( column->pages_count() > 1 ) {
            mvwprintw( w_inv, getmaxy( w_inv ) - 2, x, _( "Page %d/%d" ),
                       column->page_index() + 1, column->pages_count() );
        }

        x += column->get_width() + gap;
    }

    get_active_column().draw( w_inv, active_x, y );

    if( mode == SM_PICK ) {
        mvwprintw(w_inv, 1, 61, _("Hotkeys:  %d/%d "), u.allocated_invlets().size(), inv_chars.size());
    }

    if (mode == SM_MULTIDROP) {
        // Make copy, remove to be dropped items from that
        // copy and let the copy recalculate the volume capacity
        // (can be affected by various traits).
        player tmp = u;
        remove_dropping_items(tmp);
        print_inv_weight_vol(tmp.weight_carried(), tmp.volume_carried(), tmp.volume_capacity());
        mvwprintw(w_inv, 1, 0, _("To drop x items, type a number and then the item hotkey."));
    } else {
        print_inv_weight_vol(u.weight_carried(), u.volume_carried(), u.volume_capacity());
    }
    if( empty() ) {
        center_print( w_inv, getmaxy( w_inv ) / 2, c_dkgray, _( "Your inventory is empty." ) );
    }

    const std::string msg_str = ( navigation == navigation_mode::CATEGORY )
        ? _( "Category selection; [TAB] switches mode, arrows select." )
        : _( "Item selection; [TAB] switches mode, arrows select." );
    const nc_color msg_color = ( navigation == navigation_mode::CATEGORY ) ? h_white : c_ltgray;

    if( center_align ) {
        center_print( w_inv, getmaxy( w_inv ) - 1, msg_color, msg_str.c_str() );
    } else {
        trim_and_print( w_inv, getmaxy( w_inv ) - 1, 1, getmaxx( w_inv ), msg_color, msg_str.c_str() );
    }

    wrefresh(w_inv);
}

inventory_selector::inventory_selector( player &u, const item_location_filter &filter )
    : columns()
    , active_column_index( 0 )
    , filter( filter )
    , dropping()
    , ctxt("INVENTORY")
    , w_inv( newwin( TERMY, TERMX, VIEW_OFFSET_Y, VIEW_OFFSET_X ) )
    , navigation( navigation_mode::ITEM )
    , weapon_cat("WEAPON", _("WEAPON HELD"), 0)
    , worn_cat("ITEMS WORN", _("ITEMS WORN"), 0)
    , u(u)
{
    ctxt.register_action("DOWN", _("Next item"));
    ctxt.register_action("UP", _("Previous item"));
    ctxt.register_action("RIGHT", _("Confirm"));
    ctxt.register_action("LEFT", _("Switch inventory/worn"));
    ctxt.register_action("CONFIRM", _("Mark selected item"));
    ctxt.register_action("QUIT", _("Cancel"));
    ctxt.register_action("CATEGORY_SELECTION");
    ctxt.register_action("NEXT_TAB", _("Page down"));
    ctxt.register_action("PREV_TAB", _("Page up"));
    ctxt.register_action("HOME", _("Home"));
    ctxt.register_action("END", _("End"));
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("ANY_INPUT"); // For invlets

    std::unique_ptr<inventory_column> first_column( new inventory_column() );
    std::unique_ptr<inventory_column> second_column( new inventory_column() );

    for( size_t i = 0; i < u.inv.size(); ++i ) {
        const auto &stack = u.inv.const_stack( i );
        const auto location = std::make_shared<item_location>( u, const_cast<item *>( &stack.front() ) );

        if( filter( *location ) ) {
            first_column->add_entry( inventory_entry( location, stack.size() ) );
        }
    }

    if( !first_column->empty() ) {
        insert_column( columns.end(), first_column );
    }

    if( u.is_armed() ) {
        const auto location = std::make_shared<item_location>( u, &u.weapon );
        if( filter( *location ) ) {
            second_column->add_entry( inventory_entry( location, &weapon_cat, c_ltblue ) );
        }
    }

    for( auto &it : u.worn ) {
        const auto location = std::make_shared<item_location>( u, &it );
        if( filter( *location ) ) {
            second_column->add_entry( inventory_entry( location, &worn_cat, c_cyan ) );
        }
    }

    if( !second_column->empty() ) {
        insert_column( columns.end(), second_column );
    }
}

inventory_selector::~inventory_selector()
{
    if (w_inv != NULL) {
        werase(w_inv);
        delwin(w_inv);
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

void inventory_selector::on_action( const std::string &action )
{
    if( action == "CATEGORY_SELECTION" ) {
        toggle_navigation_mode();
    } else if( action == "LEFT" ) {
        toggle_active_column();
    } else {
        for( auto &column : columns ) {
            column->on_action( action );
        }
        refresh_active_column(); // Columns can react to actions by losing their activation capacity
    }
}

void inventory_selector::on_change( const inventory_entry &entry )
{
    for( auto &column : columns ) {
        column->on_change( entry );
    }
    refresh_active_column(); // Columns can react to changes by losing their activation capacity
}

void inventory_selector::set_drop_count( inventory_entry &entry, size_t count )
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

void inventory_selector::remove_dropping_items( player &dummy ) const
{
    std::map<item *, int> dummy_dropping;

    for( const auto &elem : dropping ) {
        dummy_dropping[&dummy.i_at( u.get_item_position( elem.first ) )] = elem.second;
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

inventory_column &inventory_selector::get_column( size_t index ) const
{
    if( index >= columns.size() ) {
        static inventory_column dummy;
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

void inventory_selector::insert_column( decltype( columns )::iterator position, std::unique_ptr<inventory_column> &column )
{
    assert( column != nullptr );

    if( columns.empty() ) {
        column->on_activate();
    }
    column->set_mode( navigation );
    columns.insert( position, std::move( column ) );
}

void inventory_selector::insert_selection_column( const std::string &id, const std::string &name )
{
    std::unique_ptr<inventory_column> new_column( new selection_column( id, name ) );

    for( const auto &column : columns ) {
        static_cast<selection_column *>( new_column.get() )->reserve_width_for( *column );
    }

    if( column_can_fit( *new_column ) ) { // Insert only if it will be visible. Ignore otherwise.
        new_column->set_custom_colors( true );
        insert_column( columns.end(), new_column );
    }
}

item_location &inventory_selector::execute_pick( const std::string &title )
{
    prepare_columns( false );

    while( true ) {
        display( title, SM_PICK );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto entry = find_entry_by_invlet( ch );

        if( entry != nullptr ) {
            return entry->get_location();
        } else if( action == "QUIT" ) {
            return null_location;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            return get_active_column().get_selected().get_location();
        } else {
            on_action( action );
        }
    }
}

void inventory_selector::execute_compare( const std::string &title )
{
    insert_selection_column( "ITEMS TO COMPARE", _( "ITEMS TO COMPARE" ) );
    prepare_columns( true );

    std::vector<inventory_entry *> compared;

    const auto toggle_entry = [ this, &compared ]( inventory_entry *entry ) {
        const auto iter = std::find( compared.begin(), compared.end(), entry );

        entry->chosen_count = ( iter == compared.end() ) ? 1 : 0;

        if( entry->chosen_count != 0 ) {
            compared.push_back( entry );
        } else {
            compared.erase( iter );
        }

        on_change( *entry );
    };

    while(true) {
        display( title, SM_COMPARE );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto entry = find_entry_by_invlet( ch );

        if( entry != nullptr ) {
            toggle_entry( entry );
        } else if(action == "RIGHT") {
            const auto selection( get_active_column().get_all_selected() );

            for( auto &entry : selection ) {
                if( entry->chosen_count == 0 || selection.size() == 1 ) {
                    toggle_entry( entry );
                    if( compared.size() == 2 ) {
                        break;
                    }
                }
            }
        } else if( action == "QUIT" ) {
            break;
        } else {
            on_action( action );
        }

        if( compared.size() == 2 ) {
            const item &first_item = compared.back()->get_item();
            const item &second_item = compared.front()->get_item();

            std::vector<iteminfo> vItemLastCh, vItemCh;
            std::string sItemLastCh, sItemCh, sItemLastTn, sItemTn;

            first_item.info( true, vItemCh );
            sItemCh = first_item.tname();
            sItemTn = first_item.type_name();
            second_item.info(true, vItemLastCh);
            sItemLastCh = second_item.tname();
            sItemLastTn = second_item.type_name();

            int iScrollPos = 0;
            int iScrollPosLast = 0;
            int ch = ( int ) ' ';
            do {
                draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                               sItemLastCh, sItemLastTn, vItemLastCh, vItemCh, iScrollPosLast, true ); //without getch()
                ch = draw_item_info( ( TERMX - VIEW_OFFSET_X * 2) / 2, (TERMX - VIEW_OFFSET_X * 2 ) / 2,
                                    0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh, iScrollPos );

                if( ch == KEY_PPAGE ) {
                    iScrollPos--;
                    iScrollPosLast--;
                } else if( ch == KEY_NPAGE ) {
                    iScrollPos++;
                    iScrollPosLast++;
                }
            } while ( ch == KEY_PPAGE || ch == KEY_NPAGE );

            toggle_entry( compared.back() );
        }
    }
}

std::list<std::pair<int, int>> inventory_selector::execute_multidrop( const std::string &title )
{
    insert_selection_column( "ITEMS TO DROP", _( "ITEMS TO DROP" ) );
    prepare_columns( true );

    int count = 0;
    while( true ) {
        display( title, SM_MULTIDROP );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        auto entry = find_entry_by_invlet( ch );

        if( ch >= '0' && ch <= '9' ) {
            count = std::min( count, INT_MAX / 10 - 10 );
            count *= 10;
            count += ch - '0';
        } else if( entry != nullptr ) {
            set_drop_count( *entry, count );
            count = 0;
        } else if( action == "RIGHT" ) {
            for( const auto &entry : get_active_column().get_all_selected() ) {
                set_drop_count( *entry, count );
            }
            count = 0;
        } else if( action == "CONFIRM" ) {
            break;
        } else if( action == "QUIT" ) {
            return std::list<std::pair<int, int> >();
        } else {
            on_action( action );
            count = 0;
        }
    }

    std::list<std::pair<int, int>> dropped_pos_and_qty;

    for( auto drop_pair : dropping ) {
        dropped_pos_and_qty.push_back( std::make_pair( u.get_item_position( drop_pair.first ), drop_pair.second ) );
    }

    return dropped_pos_and_qty;
}

