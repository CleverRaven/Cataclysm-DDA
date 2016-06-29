#include "game.h"
#include "player.h"
#include "action.h"
#include "map.h"
#include "output.h"
#include "uistate.h"
#include "translations.h"
#include "options.h"
#include "messages.h"
#include "input.h"
#include "catacharset.h"
#include "item_location.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "cata_utility.h"
#include "itype.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

struct inventory_entry {
    /** The item that should be displayed here. Can be NULL. */
    const item *it;
    /** Pointer into an inventory slice, can be NULL, if not NULL, it should not
     * point to an empty list. The first entry should be the same as @ref it. */
    const std::list<item> *slice;
    /** The category of an item. */
    const item_category *category;
    /** The item position in the players inventory. It should be unique as it
     * is used by the drop map in inventory_selector. */
    int item_pos;
    size_t chosen_count = 0;

    inventory_entry( const indexed_invslice::value_type &slice, const item_category *cat = nullptr )
        : it( &( slice.first->front() ) ),
          slice( slice.first ),
          category( ( cat != nullptr ) ? cat : &it->get_category() ),
          item_pos( slice.second ) {}

    inventory_entry( const item *it, int pos, const item_category *cat = nullptr )
        : it( it ),
          slice( nullptr ),
          category( ( cat != nullptr ) ? cat : &it->get_category() ),
          item_pos( pos ) {}

    inventory_entry( const item_category *cat = nullptr )
        : it( nullptr ),
          slice( nullptr ),
          category( cat ),
          item_pos( INT_MIN ) {}
    // used for searching the category header, only the item pointer and the category are important there
    bool operator == ( const inventory_entry &other) const {
        return category == other.category && it == other.it;
    }

    bool operator != ( const inventory_entry &other ) const {
        return !( *this == other );
    }

    size_t get_available_count() const {
        return ( item_pos != INT_MIN ) ? inventory::num_items_at_position( item_pos ) : 0;
    }

    bool is_item() const {
        return it != nullptr;
    }

    bool is_null() const {
        return category == nullptr;
    }
};

enum class navigation_mode : int {
    ITEM = 0,
    CATEGORY
};

class inventory_column {
    public:
        inventory_column() :
            entries(),
            active( false ),
            mode( navigation_mode::ITEM ),
            selected_index( 1 ),
            page_offset( 0 ),
            entries_per_page( 1 ) {}

        virtual ~inventory_column() {}

        bool empty() const {
            return entries.empty();
        }
        // true if can be activated
        virtual bool activatable() const {
            return !empty();
        }
        // true if allows selecting entries
        virtual bool allows_selecting() const {
            return activatable();
        }

        size_t page_index() const {
            return page_of( page_offset );
        }

        size_t pages_count() const {
            return page_of( entries.size() + entries_per_page - 1 );
        }

        bool is_selected( const inventory_entry &entry ) const;
        bool is_selected_by_category( const inventory_entry &entry ) const;

        const inventory_entry &get_selected() const;
        std::vector<inventory_entry *> get_all_selected() const;
        const std::vector<inventory_entry> &get_entries() const {
            return entries;
        }

        inventory_entry *find_by_invlet( long invlet ) const;

        void draw( WINDOW *win, size_t x, size_t y ) const;

        void add_entry( const inventory_entry &entry );
        void add_entries( const inventory_column &source );
        void add_entries( const indexed_invslice &slice, const item_category *def_cat = nullptr );

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_mode( navigation_mode mode ) {
            this->mode = mode;
        }

        virtual size_t get_width() const;
        virtual void prepare_paging( size_t new_entries_per_page = 0 ); // Zero means unchanged

        virtual void on_action( const std::string &action );
        virtual void on_change( const inventory_entry & ) {}

        virtual void on_activate() {
            active = true;
        }
        virtual void on_deactivate() {
            active = false;
        }

    protected:
        void select( size_t new_index );

        size_t page_of( size_t index ) const {
            return index / entries_per_page;
        }

        size_t page_of( const inventory_entry &entry ) const {
            return page_of( std::distance( entries.begin(), std::find( entries.begin(), entries.end(), entry ) ) );
        }

        virtual std::string get_entry_text( const inventory_entry &entry ) const;
        virtual nc_color get_entry_color( const inventory_entry &entry ) const;

        size_t get_entry_indent( const inventory_entry &entry ) const {
            return entry.is_item() ? 2 : 0;
        }

        size_t get_entry_width( const inventory_entry &entry ) const {
            return get_entry_indent( entry ) + utf8_width( get_entry_text( entry ), true );
        }

        std::vector<inventory_entry> entries;
        bool active;
        bool multiselect;
        navigation_mode mode;
        size_t selected_index;
        size_t page_offset;
        size_t entries_per_page;
};

class selection_column : public inventory_column {
    public:
        selection_column( const std::string &id, const std::string &name )
            : inventory_column(),
              selected_cat( id, name, 0 ),
              reserved_width( 0 ) {}

        void reserve_width_for( const inventory_column &column );

        virtual bool activatable() const override {
            return inventory_column::activatable() && pages_count() > 1;
        }

        virtual bool allows_selecting() const override {
            return false;
        }

        virtual size_t get_width() const override {
            return std::max( inventory_column::get_width(), reserved_width );
        }

        virtual void prepare_paging( size_t new_entries_per_page = 0 ) override; // Zero means unchanged
        virtual void on_change( const inventory_entry &entry ) override;

    protected:
        virtual std::string get_entry_text( const inventory_entry &entry ) const override;
        virtual nc_color get_entry_color( const inventory_entry &entry ) const override;

    private:
        const item_category selected_cat;
        size_t reserved_width;
};

const item_filter allow_all_items = []( const item & ) { return true; };

class inventory_selector
{
    public:
        /**
         * Extracts <B>slice</B> into @ref entries, adding category entries.
         * For each item in the slice an entry that points to it is added to @ref entries.
         * For a consecutive sequence of entries of the same category a single
         * category entry is added.
         */
        void add_entries( const indexed_invslice &slice, const item_category *def_cat = nullptr );
        /**
         * Checks the selector for emptiness (absence of available entries).
         */
        bool empty() const {
            for( const auto &column : columns ) {
                if( !column->empty() ) {
                    return false;
                }
            }
            return custom_column == nullptr || custom_column->empty();
        }
        /** Creates the inventory screen */
        inventory_selector( player &u, item_filter filter = allow_all_items );
        ~inventory_selector();

        /** Executes the selector */
        int execute_pick( const std::string &title );
        // @todo opts should not be passed here. Temporary solution for further refactoring
        item_location execute_pick_map( const std::string &title, std::unordered_map<item *, item_location> &opts );
        void execute_compare( const std::string &title );
        std::list<std::pair<int, int>> execute_multidrop( const std::string &title );

    private:
        enum selector_mode{
            SM_PICK,
            SM_COMPARE,
            SM_MULTIDROP
        };

        std::vector<std::unique_ptr<inventory_column>> columns;
        std::unique_ptr<inventory_column> custom_column;
        size_t active_column_index;

        /**
         * Inserts additional category entries on top of each page,
         * When the last entry of a page is a category entry, inserts an empty entry
         * right before that one. The category entry goes now on the next page.
         */
        void prepare_columns( bool multiselect );
        /**
         * What has been selected for dropping/comparing. The key is the item position,
         * the value is the count, or -1 for dropping all. The class makes sure that
         * the count is never 0, and it is -1 only if all items should be dropped.
         * Any value > 0 means at least one item will remain after dropping.
         */
        std::map<int, int> dropping;
        /** The input context for navigation, already contains some actions for movement.
         * See @ref on_action */
        input_context ctxt;
        /** Given an action from the input_context, try to act according to it. */
        void on_action( const std::string &action );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );
        /** Update the @ref w_inv window, including wrefresh */
        void display( const std::string &title, selector_mode mode ) const;

        void remove_dropping_items( player &u ) const;
        WINDOW *w_inv;

        navigation_mode navigation;

        const item_category weapon_cat;
        const item_category worn_cat;

        player &u;

        void print_inv_weight_vol(int weight_carried, int vol_carried, int vol_capacity) const;

        /** Returns an entry from @ref entries by its invlet */
        inventory_entry *find_entry_by_invlet( long invlet ) const;
        /** Toggle item dropping for item position it_pos:
         * If count is > 0: set dropping to count
         * If the item is already marked for dropping: deactivate dropping.
         */
        void set_drop_count( int it_pos, int count, const item &it );
        void set_drop_count( inventory_entry &entry, size_t count );

        inventory_column &get_column( size_t index ) const;
        inventory_column &get_active_column() const {
            return get_column( active_column_index );
        }

        void set_active_column( size_t index );
        size_t get_columns_width() const;

        bool column_can_fit( const inventory_column &column ) {
            return column.get_width() + get_columns_width() <= size_t( getmaxx( w_inv ) );
        }
        bool is_active_column( const inventory_column &column ) const {
            return &column == &get_active_column();
        }

        void toggle_active_column();
        void refresh_active_column() {
            if( !get_active_column().activatable() ) {
                toggle_active_column();
            }
        }
        void toggle_navigation_mode();

        void insert_column( decltype( columns )::iterator position, std::unique_ptr<inventory_column> &new_column );
        void insert_selection_column( const std::string &id, const std::string &name );
};

inventory_entry *inventory_column::find_by_invlet( long invlet ) const
{
    for( const auto &entry : entries ) {
        if( entry.is_item() && entry.it->invlet == invlet ) {
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

bool inventory_column::is_selected( const inventory_entry &entry ) const
{
    return entry == get_selected() || ( multiselect && is_selected_by_category( entry ) );
}

bool inventory_column::is_selected_by_category( const inventory_entry &entry ) const
{
    return entry.is_item() && mode == navigation_mode::CATEGORY
                           && entry.category == get_selected().category
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
                return cur.category == entry.category || cur.category->sort_rank <= entry.category->sort_rank;
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

void inventory_column::add_entries( const indexed_invslice &slice, const item_category *def_cat )
{
    for( const auto &scit : slice ) {
        add_entry( inventory_entry( scit, def_cat ) );
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
        if( entries[i].category == current_category && i % entries_per_page != 0 ) {
            continue;
        }

        current_category = entries[i].category;
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
            res << entry.it->symbol() << ' ';
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

        const size_t count = ( entry.slice != nullptr ) ? entry.slice->size() : 1;
        if( count > 1 ) {
            res << count << ' ';
        }
        res << entry.it->display_name( count );
    } else if( entry.category != nullptr ) {
        res << entry.category->name;
    }
    return res.str();
}

nc_color inventory_column::get_entry_color( const inventory_entry &entry ) const
{
    if( entry.is_item() ) {
        return ( active && is_selected( entry ) ) ? h_white : entry.it->color_in_inventory();
    } else {
        return c_magenta;
    }
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

        if( entry.is_item() && entry.it->invlet != '\0' ) {
            const nc_color invlet_color = g->u.assigned_invlet.count( entry.it->invlet ) ? c_yellow : c_white;
            mvwputch( win, y + line, x, invlet_color, entry.it->invlet );
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
    inventory_entry my_entry( entry );
    my_entry.category = &selected_cat;

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
            res << entry.it->symbol() << ' ';
        }

        size_t count;
        if( entry.chosen_count > 0 && entry.chosen_count < entry.get_available_count() ) {
            count = entry.get_available_count();
            res << string_format( _( "%d of %d"), entry.chosen_count, count ) << ' ';
        } else {
            count = ( entry.slice != nullptr ) ? entry.slice->size() : 1;
            if( count > 1 ) {
                res << count << ' ';
            }
        }
        res << entry.it->display_name( count );
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

nc_color selection_column::get_entry_color( const inventory_entry &entry ) const
{
    if( !entry.is_item() || is_selected( entry ) ) {
        return inventory_column::get_entry_color( entry );
    } else if( entry.item_pos == -1 ) {
        return c_ltblue;
    } else if( entry.item_pos <= -2 ) {
        return c_cyan;
    }

    return entry.it->color_in_inventory();
}

void inventory_selector::add_entries( const indexed_invslice &slice, const item_category *def_cat )
{
    if( custom_column == nullptr ) {
        custom_column.reset( new inventory_column() );
    }
    custom_column->add_entries( slice, def_cat );
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
        // first round: remove weapon & worn items, start with larges worn index
        for( const auto &elem : dropping ) {
            if( elem.first == -1 && elem.second == -1 ) {
                tmp.remove_weapon();
            } else if( elem.first == -1 && elem.second != -1 ) {
                tmp.weapon.charges -= elem.second;
            } else if( elem.first < 0 ) {
                tmp.i_rem( elem.first );
            }
        }
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

inventory_selector::inventory_selector( player &u, item_filter filter )
    : columns()
    , active_column_index( 0 )
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

    first_column->add_entries( u.inv.indexed_slice_filter_by( filter ) );

    if( !first_column->empty() ) {
        insert_column( columns.end(), first_column );
    }

    if( u.is_armed() && filter( u.weapon ) ) {
        second_column->add_entry( inventory_entry( &u.weapon, -1, &weapon_cat ) );
    }

    size_t i = 0;
    for( const auto &it : u.worn ) {
        if( filter( it ) ) {
            second_column->add_entry( inventory_entry( &it, player::worn_position_to_index( i ), &worn_cat ) );
        }
        ++i;
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
    const auto iter = dropping.find( entry.item_pos );

    if( count == 0 && iter != dropping.end() ) {
        entry.chosen_count = 0;
        dropping.erase( iter );
    } else {
        entry.chosen_count = ( count == 0 )
            ? entry.get_available_count()
            : std::min( count, entry.get_available_count() );
        dropping[entry.item_pos] = entry.chosen_count;
    }

    on_change( entry );
}

void inventory_selector::remove_dropping_items( player &u ) const
{
    // We iterate backwards because deletion will invalidate later indices.
    for( auto a = dropping.rbegin(); a != dropping.rend(); ++a ) {
        if( a->first < 0 ) { // weapon or armor, handled separately
            continue;
        }
        const int count = a->second;
        item &tmpit = u.inv.find_item( a->first );
        if( tmpit.count_by_charges() ) {
            long charges = tmpit.charges;
            if( count != -1 && count < charges ) {
                tmpit.charges -= count;
            } else {
                u.inv.remove_item( a->first );
            }
        } else {
            size_t max_count = u.inv.const_stack( a->first ).size();
            if( count != -1 && ( size_t )count < max_count ) {
                max_count = count;
            }
            for( size_t i = 0; i < max_count; i++ ) {
                u.inv.remove_item( a->first );
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
        insert_column( columns.end(), new_column );
    }
}

int inventory_selector::execute_pick( const std::string &title )
{
    prepare_columns( false );

    while( true ) {
        display( title, SM_PICK );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto entry = find_entry_by_invlet( ch );

        if( entry != nullptr ) {
            return entry->item_pos;
        } else if ( action == "CONFIRM" || action == "RIGHT" ) {
            return get_active_column().get_selected().item_pos;
        } else if ( action == "QUIT" ) {
            return INT_MIN;
        } else {
            on_action( action );
        }
    }
}

item_location inventory_selector::execute_pick_map( const std::string &title, std::unordered_map<item *, item_location> &opts )
{
    prepare_columns( false );

    while( true ) {
        display( title, SM_PICK );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto entry = find_entry_by_invlet( ch );

        if( entry != nullptr ) {
            item *it = const_cast<item *>( entry->it );
            const auto iter = opts.find( it );
            return ( iter != opts.end() ) ? std::move( iter->second ) : item_location( u, it );
        } else if( action == "QUIT" ) {
            return item_location();

        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            const auto selected = get_active_column().get_selected();
            const auto it = const_cast<item *>( selected.it );

            // Item in inventory
            if( selected.item_pos != INT_MIN ) {
                return item_location( u, it );
            }
            // Item on ground or in vehicle
            auto iter = opts.find( it );
            if( iter != opts.end() ) {
                return std::move( iter->second );
            }

            return item_location();
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
            const item *first_item = compared.back()->it;
            const item *second_item = compared.front()->it;

            std::vector<iteminfo> vItemLastCh, vItemCh;
            std::string sItemLastCh, sItemCh, sItemTn;

            first_item->info( true, vItemCh );
            sItemCh = first_item->tname();
            sItemTn = first_item->type_name();
            second_item->info(true, vItemLastCh);
            sItemLastCh = second_item->tname();

            int iScrollPos = 0;
            int iScrollPosLast = 0;
            int ch = ( int ) ' ';
            do {
                draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                               sItemLastCh, sItemTn, vItemLastCh, vItemCh, iScrollPosLast, true ); //without getch()
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
        dropped_pos_and_qty.push_back( std::make_pair( drop_pair.first, drop_pair.second ) );
    }

    return dropped_pos_and_qty;
}

void game::interactive_inv()
{
    static const std::set<int> allowed_selections = { { ' ', '.', 'q', '=', '\n', KEY_LEFT, KEY_ESCAPE } };

    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u );

    int res;
    do {
        const int pos = inv_s.execute_pick( _( "Inventory:" ) );
        if( pos == INT_MIN ) {
            break;
        }
        refresh_all();
        res = inventory_item_menu( pos );
    } while( allowed_selections.count( res ) != 0 );
}

int game::inv_for_filter( const std::string &title, item_filter filter, const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, filter );

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item." ) : none_message;
        popup( msg, PF_GET_KEY );
        return INT_MIN;
    }

    return inv_s.execute_pick( title );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = ( none_message.empty() ) ? _( "Your inventory is empty." ) : none_message;
    return inv_for_filter( title, allow_all_items, msg );
}

int game::inv_for_activatables( const player &p, const std::string &title )
{
    return inv_for_filter( title, [ &p ]( const item &it ) {
        return p.rate_action_use( it ) != HINT_CANT;
    }, _( "You don't have any items you can use." ) );
}

int game::inv_for_flag( const std::string &flag, const std::string &title )
{
    return inv_for_filter( title, [ &flag ]( const item &it ) {
        return it.has_flag( flag );
    } );
}

int game::inv_for_id( const itype_id &id, const std::string &title )
{
    return inv_for_filter( title, [ &id ]( const item &it ) {
        return it.typeId() == id;
    }, string_format( _( "You don't have a %s." ), item::nname( id ).c_str() ) );
}

int game::inv_for_tools_powered_by( const ammotype &battery_id, const std::string &title )
{
    return inv_for_filter( title, [ &battery_id ]( const item & it ) {
        return it.is_tool() && it.ammo_type() == battery_id;
    }, string_format( _( "You don't have %s-powered tools." ), ammo_name( battery_id ).c_str() ) );
}

int game::inv_for_equipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item &it ) {
        return u.is_worn( it );
    }, _( "You don't wear anything." ) );
}

int game::inv_for_unequipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item &it ) {
        return it.is_armor() && !u.is_worn( it );
    }, _( "You don't have any items to wear." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_map_splice( filter, filter, filter, title, radius, none_message );
}

item_location game::inv_map_splice(
    item_filter inv_filter, item_filter ground_filter, item_filter vehicle_filter,
    const std::string &title, int radius, const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, inv_filter );

    std::list<item_category> categories;
    int rank = -1000;

    // items are stacked per tile considering vehicle and map tiles separately
    // in the below loops identical items on the same tile are grouped into lists
    // each element of stacks represents one tile and is a vector of such lists
    std::vector<std::vector<std::list<item>>> stacks;

    // an indexed_invslice is created for each map or vehicle tile
    // each list of items created above for the tile will be added to it
    std::vector<indexed_invslice> slices;

    // of one of the above lists so use this as the key when storing the item location
    std::unordered_map<item *, item_location> opts;

    // the closest 10 items also have their location added to the invlets vector
    const char min_invlet = '0';
    const char max_invlet = '9';
    char cur_invlet = min_invlet;

    for( const auto &pos : closest_tripoints_first( radius, g->u.pos() ) ) {
        // second get all matching items on the map within radius
        if( m.accessible_items( g->u.pos(), pos, radius ) ) {
            auto items = m.i_at( pos );

            // create a new slice and stack for the current map tile
            stacks.emplace_back();
            slices.emplace_back();

            // reserve sufficient capacity to ensure reallocation is not required
            auto &current_stack = stacks.back();
            current_stack.reserve( items.size() );

            for( item &it : items ) {
                if( ground_filter( it ) ) {
                    auto match = std::find_if( current_stack.begin(),
                    current_stack.end(), [&]( const std::list<item> &e ) {
                        return it.stacks_with( e.back() );
                    } );
                    if( match != current_stack.end() ) {
                        match->push_back( it );
                    } else {
                        // item doesn't stack with any previous so start new list and append to current indexed_invslice
                        current_stack.emplace_back( 1, it );
                        slices.back().emplace_back( &current_stack.back(), INT_MIN );
                        opts.emplace( &current_stack.back().front(), item_location( pos, &it ) );

                        current_stack.back().front().invlet = ( cur_invlet <= max_invlet ) ? cur_invlet++ : 0;
                    }
                }
            }
            std::string name = trim( std::string( _( "GROUND" ) ) + " " + direction_suffix( g->u.pos(), pos ) );
            categories.emplace_back( name, name, rank++ );
            inv_s.add_entries( slices.back(), &categories.back() );
        }

        // finally get all matching items in vehicle cargo spaces
        int part = -1;
        vehicle *veh = m.veh_at( pos, part );
        if( veh && part >= 0 ) {
            part = veh->part_with_feature( part, "CARGO" );
            if( part != -1 ) {
                auto items = veh->get_items( part );

                // create a new slice and stack for the current vehicle part
                stacks.emplace_back();
                slices.emplace_back();

                // reserve sufficient capacity to ensure reallocation is not required
                auto &current_stack = stacks.back();
                current_stack.reserve( items.size() );

                for( item &it : items ) {
                    if( vehicle_filter( it ) ) {
                        auto match = std::find_if( current_stack.begin(),
                        current_stack.end(), [&]( const std::list<item> &e ) {
                            return it.stacks_with( e.back() );
                        } );
                        if( match != current_stack.end() ) {
                            match->push_back( it );
                        } else {
                            // item doesn't stack with any previous so start new list and append to current indexed_invslice
                            current_stack.emplace_back( 1, it );
                            slices.back().emplace_back( &current_stack.back(), INT_MIN );
                            opts.emplace( &current_stack.back().front(), item_location( vehicle_cursor( *veh, part ), &it ) );

                            current_stack.back().front().invlet = ( cur_invlet <= max_invlet ) ? cur_invlet++ : 0;
                        }
                    }
                }
                std::string name = trim( std::string( _( "VEHICLE" ) )  + " " + direction_suffix( g->u.pos(), pos ) );
                categories.emplace_back( name, name, rank-- );
                inv_s.add_entries( slices.back(), &categories.back() );
            }
        }
    }

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item at hand." ) : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }
    return inv_s.execute_pick_map( title, opts );
}

item *game::inv_map_for_liquid(const item &liquid, const std::string &title, int radius)
{
    // Vehicle filter shouldn't allow buckets
    auto sealable_filter = [&]( const item &candidate ) {
        return candidate.get_remaining_capacity_for_liquid( liquid, false ) > 0;
    };

    // Map filter should allow buckets
    auto bucket_filter = [&]( const item &candidate ) {
        return candidate.get_remaining_capacity_for_liquid( liquid, true ) > 0;
    };

    // Inventory filter should allow only held buckets
    auto inv_filter = [&]( const item &candidate ) {
        return candidate.get_remaining_capacity_for_liquid( liquid, &candidate == &g->u.weapon ) > 0;
    };

    return inv_map_splice( inv_filter, bucket_filter, sealable_filter, title, radius,
                           string_format( _( "You don't have a suitable container for carrying %s." ),
                           liquid.type_name( 1 ).c_str() ) ).get_item();
}

int inventory::num_items_at_position( int const position )
{
    if( position < -1 ) {
        const item& armor = g->u.i_at( position );
        return armor.count_by_charges() ? armor.charges : 1;
    } else if( position == -1 ) {
        return g->u.weapon.count_by_charges() ? g->u.weapon.charges : 1;
    } else {
        const std::list<item> &stack = g->u.inv.const_stack(position);
        if( stack.size() == 1 ) {
            return stack.front().count_by_charges() ?
                stack.front().charges : 1;
        } else {
            return stack.size();
        }
    }
}

std::list<std::pair<int, int>> game::multidrop()
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, [ this ]( const item &it ) -> bool {
        return u.can_unwield( it, false );
    } );
    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }
    return inv_s.execute_multidrop( _( "Multidrop:" ) );
}

void game::compare()
{
    tripoint dir;
    int &dirx = dir.x;
    int &diry = dir.y;

    if( choose_direction(_("Compare where?"), dirx, diry ) ) {
        compare( tripoint( dirx, diry, 0 ) );
    }
}

void game::compare( const tripoint &offset )
{
    const tripoint examp = u.pos() + offset;

    std::vector<std::list<item>> grounditems;
    indexed_invslice grounditems_slice;

    if( !m.has_flag( "SEALED", u.pos() ) ) {
        auto here = m.i_at( examp );
        //Filter out items with the same name (keep only one of them)
        std::set<std::string> dups;
        for (size_t i = 0; i < here.size(); i++) {
            if (dups.count(here[i].tname()) == 0) {
                grounditems.push_back(std::list<item>(1, here[i]));

                //Only the first 10 items get a invlet
                if ( grounditems.size() <= 10 ) {
                    // invlet: '0' ... '9'
                    grounditems.back().front().invlet = '0' + grounditems.size() - 1;
                }

                dups.insert(here[i].tname());
            }
        }
        for (size_t a = 0; a < grounditems.size(); a++) {
            // avoid INT_MIN, as it can be confused with "no item at all"
            grounditems_slice.push_back(indexed_invslice::value_type(&grounditems[a], INT_MIN + a + 1));
        }
    }

    static const item_category category_on_ground( "GROUND", _( "GROUND" ), -1000 );

    u.inv.restack(&u);
    u.inv.sort();

    inventory_selector inv_s( u );

    inv_s.add_entries( grounditems_slice, &category_on_ground );
    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }
    inv_s.execute_compare( _( "Compare:" ) );
}
