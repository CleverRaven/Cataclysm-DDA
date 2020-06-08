#pragma once
#ifndef CATA_SRC_INVENTORY_UI_H
#define CATA_SRC_INVENTORY_UI_H

#include <cassert>
#include <climits>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <array>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "item.h"
#include "item_location.h"
#include "memory_fast.h"
#include "pimpl.h"
#include "units.h"
#include "item_category.h"

class Character;
class item;
class player;
class string_input_popup;
struct tripoint;
class ui_adaptor;

enum class navigation_mode : int {
    ITEM = 0,
    CATEGORY
};

enum class scroll_direction : int {
    FORWARD = 1,
    BACKWARD = -1
};

struct navigation_mode_data;
struct inventory_input;

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

class inventory_entry
{
    public:
        std::vector<item_location> locations;

        size_t chosen_count = 0;
        int custom_invlet = INT_MIN;
        std::string cached_name;

        inventory_entry() = default;

        inventory_entry( const item_category *custom_category ) :
            custom_category( custom_category )
        {}

        // Copy with new category.  Used to copy entries into the "selected"
        // category when they are selected.
        inventory_entry( const inventory_entry &entry, const item_category *custom_category ) :
            inventory_entry( entry ) {
            this->custom_category = custom_category;
        }

        inventory_entry( const std::vector<item_location> &locations,
                         const item_category *custom_category = nullptr,
                         bool enabled = true ) :
            locations( locations ),
            custom_category( custom_category ),
            enabled( enabled )
        {}

        bool operator==( const inventory_entry &other ) const;
        bool operator!=( const inventory_entry &other ) const {
            return !( *this == other );
        }

        operator bool() const {
            return !is_null();
        }
        /** Whether the entry is null (dummy) */
        bool is_null() const {
            return get_category_ptr() == nullptr;
        }
        /**
         * Whether the entry is an item.
         */
        bool is_item() const {
            return !locations.empty();
        }
        /** Whether the entry is a category */
        bool is_category() const {
            return !is_null() && !is_item();
        }
        /** Whether the entry can be selected */
        bool is_selectable() const {
            return is_item() && enabled;
        }

        const item_location &any_item() const {
            assert( !locations.empty() );
            return locations.front();
        }

        size_t get_stack_size() const {
            return locations.size();
        }

        int get_total_charges() const;
        int get_selected_charges() const;

        size_t get_available_count() const;
        const item_category *get_category_ptr() const;
        int get_invlet() const;
        nc_color get_invlet_color() const;
        void update_cache();

    private:
        const item_category *custom_category = nullptr;
        bool enabled = true;
    protected:
        // indents the entry if it is contained in an item
        bool _indent = true;

};

class inventory_selector_preset
{
    public:
        inventory_selector_preset();
        virtual ~inventory_selector_preset() = default;

        /** Does this entry satisfy the basic preset conditions? */
        virtual bool is_shown( const item_location & ) const {
            return true;
        }

        /**
         * The reason why this entry cannot be selected.
         * @return Either the reason of denial or empty string if it's accepted.
         */
        virtual std::string get_denial( const item_location & ) const {
            return std::string();
        }
        /** Whether the first item is considered to go before the second. */
        virtual bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const;
        /** Color that will be used to display the entry string. */
        virtual nc_color get_color( const inventory_entry &entry ) const;

        std::string get_denial( const inventory_entry &entry ) const;
        /** Text in the cell */
        std::string get_cell_text( const inventory_entry &entry, size_t cell_index ) const;
        /** @return Whether the cell is a stub */
        bool is_stub_cell( const inventory_entry &entry, size_t cell_index ) const;
        /** Number of cells in the preset. */
        size_t get_cells_count() const {
            return cells.size();
        }
        /** Whether items should make new stacks if components differ */
        bool get_checking_components() const {
            return check_components;
        }

        virtual std::function<bool( const inventory_entry & )> get_filter( const std::string &filter )
        const;

        bool indent_entries() const {
            return _indent_entries;
        }

    protected:
        /** Text of the first column (default: item name) */
        virtual std::string get_caption( const inventory_entry &entry ) const;
        /**
         * Append a new cell to the preset.
         * @param func The function that returns text for the cell.
         * @param title Title of the cell.
         * @param stub The cell won't be "revealed" if it contains only this value
         */
        void append_cell( const std::function<std::string( const item_location & )> &func,
                          const std::string &title = std::string(),
                          const std::string &stub = std::string() );
        void append_cell( const std::function<std::string( const inventory_entry & )> &func,
                          const std::string &title = std::string(),
                          const std::string &stub = std::string() );
        bool check_components = false;

        // whether to indent contained entries in the menu
        bool _indent_entries = true;

    private:
        class cell_t
        {
            public:
                cell_t( const std::function<std::string( const inventory_entry & )> &func,
                        const std::string &title, const std::string &stub ) :
                    title( title ),
                    stub( stub ),
                    func( func ) {}

                std::string get_text( const inventory_entry &entry ) const;

                std::string title;
                std::string stub;

            private:
                std::function<std::string( const inventory_entry & )> func;
        };

        std::vector<cell_t> cells;
};

class inventory_holster_preset : public inventory_selector_preset
{
    public:
        inventory_holster_preset( const item_location &holster ) : holster( holster ) {}

        /** Does this entry satisfy the basic preset conditions? */
        bool is_shown( const item_location &contained ) const override {
            return holster->can_contain( *contained ) && !holster->has_item( *contained );
        }
    private:
        // this is the item that we are putting something into
        item_location holster;
};

const inventory_selector_preset default_preset;

class inventory_column
{
    public:
        inventory_column( const inventory_selector_preset &preset = default_preset ) : preset( preset ) {
            cells.resize( preset.get_cells_count() );
        }

        virtual ~inventory_column() = default;

        bool empty() const {
            return entries.empty();
        }
        /**
         * Can this column be activated?
         * @return Whether the column contains selectable entries.
         */
        virtual bool activatable() const;
        /** Is this column visible? */
        bool visible() const {
            return !empty() && visibility && preset.get_cells_count() > 0;
        }
        /**
         * Does this column allow selecting?
         * "Cosmetic" columns (list of selected items) can explicitly prohibit selecting.
         */
        virtual bool allows_selecting() const {
            return activatable();
        }

        size_t page_index() const {
            return page_of( page_offset );
        }

        size_t pages_count() const {
            return page_of( entries.size() + entries_per_page - 1 );
        }

        bool has_available_choices() const;
        bool is_selected( const inventory_entry &entry ) const;

        /**
         * Does this entry belong to the selected category?
         * When @ref navigation_mode::ITEM is used it's equivalent to @ref is_selected().
         */
        bool is_selected_by_category( const inventory_entry &entry ) const;

        const inventory_entry &get_selected() const;
        std::vector<inventory_entry *> get_all_selected() const;
        std::vector<inventory_entry *> get_entries(
            const std::function<bool( const inventory_entry &entry )> &filter_func ) const;

        // orders the child entries in this column to be under their parent
        void order_by_parent();

        inventory_entry *find_by_invlet( int invlet ) const;

        void draw( const catacurses::window &win, size_t x, size_t y ) const;

        void add_entry( const inventory_entry &entry );
        void move_entries_to( inventory_column &dest );
        void clear();
        void set_stack_favorite( const item_location &location, bool favorite );

        /** Selects the specified location. */
        bool select( const item_location &loc );

        /**
         * Change the selection.
         * @param new_index Index of the entry to select.
         * @param dir If the entry is not selectable, move in the specified direction
         */
        void select( size_t new_index, scroll_direction dir );

        size_t get_selected_index() {
            return selected_index;
        }

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_visibility( bool visibility ) {
            this->visibility = visibility;
        }

        void set_width( size_t new_width, const std::vector<inventory_column *> &all_columns );
        void set_height( size_t new_height );
        size_t get_width() const;
        size_t get_height() const;
        /** Expands the column to fit the new entry. */
        void expand_to_fit( const inventory_entry &entry );
        /** Resets width to original (unchanged). */
        virtual void reset_width( const std::vector<inventory_column *> &all_columns );
        /** Returns next custom inventory letter. */
        int reassign_custom_invlets( const player &p, int min_invlet, int max_invlet );
        /** Reorder entries, repopulate titles, adjust to the new height. */
        virtual void prepare_paging( const std::string &filter = "" );
        /**
         * Event handlers
         */
        virtual void on_input( const inventory_input &input );
        /** The entry has been changed. */
        virtual void on_change( const inventory_entry & ) {}
        /** The column has been activated. */
        virtual void on_activate() {
            active = true;
        }
        /** The column has been deactivated. */
        virtual void on_deactivate() {
            active = false;
        }
        /** Selection mode has been changed. */
        virtual void on_mode_change( navigation_mode mode ) {
            this->mode = mode;
        }

        void set_filter( const std::string &filter );

        // whether or not to indent contained entries
        bool indent_entries() const {
            if( indent_entries_override ) {
                return *indent_entries_override;
            } else {
                return preset.indent_entries();
            }
        }

        void set_indent_entries_override( bool entry_override ) {
            indent_entries_override = entry_override;
        }

    protected:
        struct entry_cell_cache_t {
            bool assigned = false;
            nc_color color = c_unset;
            std::string denial;
            std::vector<std::string> text;
        };

        /**
         * Move the selection.
         */
        void move_selection( scroll_direction dir );
        void move_selection_page( scroll_direction dir );

        size_t next_selectable_index( size_t index, scroll_direction dir ) const;

        size_t page_of( size_t index ) const;
        size_t page_of( const inventory_entry &entry ) const;
        /**
         * Indentation of the entry.
         * @param entry The entry to check
         * @returns Either left indent when it's zero, or a gap between cells.
         */
        size_t get_entry_indent( const inventory_entry &entry ) const;
        /**
         *  Overall cell width.
         *  If corresponding cell is not empty (its width is greater than zero),
         *  then a value returned by  inventory_column::get_entry_indent() is added to the result.
         */
        size_t get_entry_cell_width( size_t index, size_t cell_index ) const;
        size_t get_entry_cell_width( const inventory_entry &entry, size_t cell_index ) const;
        /** Sum of the cell widths */
        size_t get_cells_width() const;

        entry_cell_cache_t make_entry_cell_cache( const inventory_entry &entry ) const;
        const entry_cell_cache_t &get_entry_cell_cache( size_t index ) const;

        const inventory_selector_preset &preset;

        std::vector<inventory_entry> entries;
        std::vector<inventory_entry> entries_unfiltered;
        navigation_mode mode = navigation_mode::ITEM;
        bool active = false;
        bool multiselect = false;
        bool paging_is_valid = false;
        bool visibility = true;

        size_t selected_index = 0;
        size_t page_offset = 0;
        size_t entries_per_page = std::numeric_limits<size_t>::max();
        size_t height = std::numeric_limits<size_t>::max();
        size_t reserved_width = 0;

    private:
        struct cell_t {
            size_t current_width = 0;   /// Current cell widths (can be affected by set_width())
            size_t real_width = 0;      /// Minimal cell widths (to embrace all the entries nicely)

            bool visible() const {
                return current_width > 0;
            }
            /** @return Gap before the cell. Negative value means the cell is shrunk */
            int gap() const {
                return current_width - real_width;
            }
        };

        std::vector<cell_t> cells;
        mutable std::vector<entry_cell_cache_t> entries_cell_cache;

        cata::optional<bool> indent_entries_override = cata::nullopt;
        /** @return Number of visible cells */
        size_t visible_cells() const;
};

class selection_column : public inventory_column
{
    public:
        selection_column( const std::string &id, const std::string &name );
        ~selection_column() override;

        bool activatable() const override {
            return inventory_column::activatable() && pages_count() > 1;
        }

        bool allows_selecting() const override {
            return false;
        }

        void reset_width( const std::vector<inventory_column *> &all_columns ) override;

        void prepare_paging( const std::string &filter = "" ) override;

        void on_change( const inventory_entry &entry ) override;
        void on_mode_change( navigation_mode ) override {
            // Intentionally ignore mode change.
        }

    private:
        const pimpl<item_category> selected_cat;
        inventory_entry last_changed;
};

class inventory_selector
{
    public:
        inventory_selector( player &u, const inventory_selector_preset &preset = default_preset );
        virtual ~inventory_selector();
        /** These functions add items from map / vehicles. */
        void add_contained_items( item_location &container );
        void add_contained_items( item_location &container, inventory_column &column );
        void add_character_items( Character &character );
        void add_map_items( const tripoint &target );
        void add_vehicle_items( const tripoint &target );
        void add_nearby_items( int radius = 1 );
        /** Remove all items */
        void clear_items();
        /** Assigns a title that will be shown on top of the menu. */
        void set_title( const std::string &title ) {
            this->title = title;
        }
        /** Assigns a hint. */
        void set_hint( const std::string &hint ) {
            this->hint = hint;
        }
        /** Specify whether the header should show stats (weight and volume). */
        void set_display_stats( bool display_stats ) {
            this->display_stats = display_stats;
        }
        /** @return true when the selector is empty. */
        bool empty() const;
        /** @return true when there are enabled entries to select. */
        bool has_available_choices() const;

        /** Apply filter string to all columns */
        void set_filter( const std::string &str );
        /** Get last filter string set by set_filter or entered by player */
        std::string get_filter() const;

        // An array of cells for the stat lines. Example: ["Weight (kg)", "10", "/", "20"].
        using stat = std::array<std::string, 4>;
        using stats = std::array<stat, 2>;

        bool keep_open = false;

    protected:
        player &u;
        const inventory_selector_preset &preset;

        /**
         * The input context for navigation, already contains some actions for movement.
         */
        input_context ctxt;

        const item_category *naturalize_category( const item_category &category,
                const tripoint &pos );

        void add_entry( inventory_column &target_column,
                        std::vector<item_location> &&locations,
                        const item_category *custom_category = nullptr );

        void add_item( inventory_column &target_column,
                       item_location &&location,
                       const item_category *custom_category = nullptr );

        void add_items( inventory_column &target_column,
                        const std::function<item_location( item * )> &locator,
                        const std::vector<std::list<item *>> &stacks,
                        const item_category *custom_category = nullptr );

        inventory_input get_input();

        /** Given an action from the input_context, try to act according to it. */
        void on_input( const inventory_input &input );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );

        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();

        size_t get_layout_width() const;
        size_t get_layout_height() const;

        void set_filter();

        /** Tackles screen overflow */
        virtual void rearrange_columns( size_t client_width );

        static stats get_weight_and_volume_stats(
            units::mass weight_carried, units::mass weight_capacity,
            const units::volume &volume_carried, const units::volume &volume_capacity );

        /** Get stats to display in top right.
         *
         * By default, computes volume/weight numbers for @c u */
        virtual stats get_raw_stats() const;

        std::vector<std::string> get_stats() const;
        std::pair<std::string, nc_color> get_footer( navigation_mode m ) const;

        size_t get_header_height() const;
        size_t get_header_min_width() const;
        size_t get_footer_min_width() const;

        /** @return an entry from all entries by its invlet */
        inventory_entry *find_entry_by_invlet( int invlet ) const;

        const std::vector<inventory_column *> &get_all_columns() const {
            return columns;
        }
        std::vector<inventory_column *> get_visible_columns() const;

    private:
        // These functions are called from resizing/redraw callbacks of ui_adaptor
        // and should not be made protected or public.
        void prepare_layout( size_t client_width, size_t client_height );
        void prepare_layout();

        void resize_window( int width, int height );
        void refresh_window() const;

        void draw_header( const catacurses::window &w ) const;
        void draw_footer( const catacurses::window &w ) const;
        void draw_columns( const catacurses::window &w ) const;
        void draw_frame( const catacurses::window &w ) const;

    public:
        /**
         * Select a location
         * @param loc Location to select
         * @return true on success.
         */
        bool select( const item_location &loc );

        const inventory_entry &get_selected() {
            return get_active_column().get_selected();
        }

        void select_position( std::pair<size_t, size_t> position ) {
            prepare_layout();
            set_active_column( position.first );
            get_active_column().select( position.second, scroll_direction::BACKWARD );
        }

        std::pair<size_t, size_t> get_selection_position() {
            std::pair<size_t, size_t> position;
            position.first = active_column_index;
            position.second = get_active_column().get_selected_index();
            return position;
        }

        inventory_column &get_column( size_t index ) const;
        inventory_column &get_active_column() const {
            return get_column( active_column_index );
        }

        void toggle_categorize_contained();
        void set_active_column( size_t index );

    protected:
        size_t get_columns_width( const std::vector<inventory_column *> &columns ) const;
        /** @return Percentage of the window occupied by columns */
        double get_columns_occupancy_ratio( size_t client_width ) const;
        /** @return Do the visible columns need to be center-aligned */
        bool are_columns_centered( size_t client_width ) const;
        /** @return Are visible columns wider than available width */
        bool is_overflown( size_t client_width ) const;

        bool is_active_column( const inventory_column &column ) const {
            return &column == &get_active_column();
        }

        void append_column( inventory_column &column );

        /**
         * Activates either previous or next column.
         * @param dir Forward - next column, backward - previous.
         */
        void toggle_active_column( scroll_direction dir );

        void refresh_active_column() {
            if( !get_active_column().activatable() ) {
                toggle_active_column( scroll_direction::FORWARD );
            }
        }
        void toggle_navigation_mode();

        const navigation_mode_data &get_navigation_data( navigation_mode m ) const;

    private:
        catacurses::window w_inv;

        weak_ptr_fast<ui_adaptor> ui;

        std::unique_ptr<string_input_popup> spopup;

        std::vector<inventory_column *> columns;

        std::string title;
        std::string hint;
        size_t active_column_index;
        std::list<item_category> categories;
        navigation_mode mode;

        inventory_column own_inv_column;     // Column for own inventory items
        inventory_column own_gear_column;    // Column for own gear (weapon, armor) items
        inventory_column map_column;         // Column for map and vehicle items

        const int border = 1;                // Width of the window border
        std::string filter;

        bool is_empty = true;
        bool display_stats = true;

    public:
        std::string action_bound_to_key( char key ) const;
};

inventory_selector::stat display_stat( const std::string &caption, int cur_value, int max_value,
                                       const std::function<std::string( int )> &disp_func );

class inventory_pick_selector : public inventory_selector
{
    public:
        inventory_pick_selector( player &p,
                                 const inventory_selector_preset &preset = default_preset ) :
            inventory_selector( p, preset ) {}

        item_location execute();
};

class inventory_multiselector : public inventory_selector
{
    public:
        inventory_multiselector( player &p, const inventory_selector_preset &preset = default_preset,
                                 const std::string &selection_column_title = "" );
    protected:
        void rearrange_columns( size_t client_width ) override;

    private:
        std::unique_ptr<inventory_column> selection_col;
};

class inventory_compare_selector : public inventory_multiselector
{
    public:
        inventory_compare_selector( player &p );
        std::pair<const item *, const item *> execute();

    protected:
        std::vector<const item *> compared;

        void toggle_entry( inventory_entry *entry );
};

// This and inventory_drop_selectors should probably both inherit from a higher-abstraction "action selector".
// Should accept a function to calculate dummy values.
class inventory_iuse_selector : public inventory_multiselector
{
    public:
        using GetStats = std::function<stats( const std::map<const item *, int> & )>;
        inventory_iuse_selector( player &p,
                                 const std::string &selector_title,
                                 const inventory_selector_preset &preset = default_preset,
                                 const GetStats & = {} );
        drop_locations execute();

    protected:
        stats get_raw_stats() const override;
        void set_chosen_count( inventory_entry &entry, size_t count );

    private:
        GetStats get_stats;
        std::map<const item *, int> to_use;
        size_t max_chosen_count;
};

class inventory_drop_selector : public inventory_multiselector
{
    public:
        inventory_drop_selector( player &p,
                                 const inventory_selector_preset &preset = default_preset,
                                 const std::string &selection_column_title = _( "ITEMS TO DROP" ) );
        drop_locations execute();
    protected:
        stats get_raw_stats() const override;
        /** Toggle item dropping */
        void set_chosen_count( inventory_entry &entry, size_t count );
        void process_selected( int &count, const std::vector<inventory_entry *> &selected );

    private:
        void deselect_contained_items();
        std::vector<std::pair<item_location, int>> dropping;
        size_t max_chosen_count;
};

#endif // CATA_SRC_INVENTORY_UI_H
