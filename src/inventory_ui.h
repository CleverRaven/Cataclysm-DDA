#pragma once
#ifndef CATA_SRC_INVENTORY_UI_H
#define CATA_SRC_INVENTORY_UI_H

#include <array>
#include <climits>
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "debug.h"
#include "input_context.h"
#include "item_category.h"
#include "item_location.h"
#include "pocket_type.h"
#include "pimpl.h"
#include "translations.h"
#include "units_fwd.h"

class basecamp;
class Character;
class inventory_selector_preset;
class item;
class item_stack;
class string_input_popup;
class tinymap;
class ui_adaptor;

enum class navigation_mode : int {
    ITEM = 0,
    CATEGORY
};

enum class scroll_direction : int {
    FORWARD = 1,
    BACKWARD = -1
};

enum class toggle_mode : int {
    SELECTED = 0,
    NON_FAVORITE_NON_WORN
};

struct inventory_input;
struct navigation_mode_data;

using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

struct collation_meta_t {
    item_location tip;
    bool collapsed = true;
    bool enabled = true;
};

class inventory_entry
{
    public:
        std::vector<item_location> locations;

        size_t chosen_count = 0;
        int custom_invlet = INT_MIN;
        std::string *cached_name = nullptr;
        std::string *cached_name_full = nullptr;
        unsigned int contents_count = 0;
        size_t cached_denial_space = 0;

        inventory_entry() = default;

        explicit inventory_entry( const item_category *custom_category ) :
            custom_category( custom_category )
        {}

        // Copy with new category.  Used to copy entries into the "selected"
        // category when they are selected.
        inventory_entry( const inventory_entry &entry, const item_category *custom_category ) :
            inventory_entry( entry ) {
            this->custom_category = custom_category;
        }

        explicit inventory_entry( const std::vector<item_location> &locations,
                                  const item_category *custom_category = nullptr,
                                  bool enabled = true,
                                  const size_t chosen_count = 0,
                                  size_t generation_number = 0,
                                  item_location topmost_parent = {}, bool chevron = false ) :
            locations( locations ),
            chosen_count( chosen_count ),
            topmost_parent( std::move( topmost_parent ) ),
            generation( generation_number ),
            chevron( chevron ),
            enabled( enabled ),
            custom_category( custom_category ) {
        }

        bool operator==( const inventory_entry &other ) const;
        bool operator!=( const inventory_entry &other ) const {
            return !( *this == other );
        }

        explicit operator bool() const {
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
        /**
        *  Whether it is hidden in inventory screen.
        * */
        bool is_hidden( std::optional<bool> const &hide_entries_override ) const;

        /** Whether the entry can be selected */
        bool is_selectable() const {
            return is_item() && enabled;
        }

        bool is_highlightable( bool skip_unselectable ) const {
            return is_item() && ( enabled || !skip_unselectable );
        }

        bool is_collated() const {
            return static_cast<bool>( collation_meta );
        }

        bool is_collation_header() const {
            return is_collated() && chevron;
        }

        bool is_collation_entry() const {
            return is_collated() && !chevron;
        }

        const item_location &any_item() const {
            if( locations.empty() ) {
                debugmsg( "inventory_entry::any_item called on a non-item entry.  "
                          "Test inventory_entry::is_item before calling this function." );
                return item_location::nowhere;
            } else {
                return locations.front();
            }
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
        bool highlight_as_parent = false;
        bool highlight_as_child = false;
        bool collapsed = false;
        // topmost visible parent, used for visibility checks
        item_location topmost_parent;
        std::shared_ptr<collation_meta_t> collation_meta;
        size_t generation = 0;
        bool chevron = false;
        int indent = 0;
        mutable bool enabled = true;
        void cache_denial( inventory_selector_preset const &preset ) const;
        mutable std::optional<std::string> denial;

        void set_custom_category( const item_category *category ) {
            custom_category = category;
        }

        void reset_collation() {
            if( is_collation_header() ) {
                chevron = false;
            }
            collation_meta.reset();
        }
        struct entry_cell_cache_t {
            nc_color color = c_unset;
            std::vector<std::string> text;
            int lang_version = 0;
        };

        const entry_cell_cache_t &get_entry_cell_cache( inventory_selector_preset const &preset ) const;
        void make_entry_cell_cache( inventory_selector_preset const &preset,
                                    bool update_only = true ) const;
        void reset_entry_cell_cache() const;

    private:
        mutable item_category const *custom_category = nullptr;
    protected:
        // indents the entry if it is contained in an item
        bool _indent = true;
        mutable std::optional<entry_cell_cache_t> entry_cell_cache;

};

struct inventory_selector_save_state;
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
        virtual bool cat_sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const;
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

        pocket_type get_pocket_type() const {
            return _pk_type;
        }

        virtual std::function<bool( const inventory_entry & )> get_filter( const std::string &filter )
        const;

        bool indent_entries() const {
            return _indent_entries;
        }

        bool collate_entries() const {
            return _collate_entries;
        }

        inventory_selector_save_state *save_state = nullptr;

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
        bool _collate_entries = false;

        pocket_type _pk_type = pocket_type::CONTAINER;

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
        explicit inventory_holster_preset( item_location holster, Character *c )
            : holster( std::move( holster ) ), who( c ) {
        }
        const item_location &get_holster() const;
        /** Does this entry satisfy the basic preset conditions? */
        bool is_shown( const item_location &contained ) const override;
        std::string get_denial( const item_location &it ) const override;
    private:
        // this is the item that we are putting something into
        item_location holster;
        Character *who = nullptr;
};

const inventory_selector_preset default_preset;

class inventory_column
{
    public:
        explicit inventory_column( const inventory_selector_preset &preset = default_preset );

        virtual ~inventory_column() = default;

        bool empty() const {
            return entries.empty();
        }
        /**
         * Can this column be activated?
         * @return Whether the column contains selectable entries.
         * Note: independent from 'allows_selecting'
         */
        virtual bool activatable() const;
        /** Is this column visible? */
        bool visible() const {
            return !empty() && visibility && preset.get_cells_count() > 0;
        }
        /**
         * Does this column allow selecting?
         * "Cosmetic" columns (list of selected items) can explicitly prohibit selecting.
         * Note: independent from 'activatable'
         */
        virtual bool allows_selecting() const {
            return true;
        }

        size_t page_index() const {
            return page_of( page_offset );
        }

        size_t pages_count() const {
            return page_of( entries.size() + entries_per_page - 1 );
        }

        bool has_available_choices() const;
        bool is_selected( const inventory_entry &entry ) const;
        bool is_highlighted( const inventory_entry &entry ) const;

        /**
         * Does this entry belong to the selected category?
         * When @ref navigation_mode::ITEM is used it's equivalent to @ref is_selected().
         */
        bool is_selected_by_category( const inventory_entry &entry ) const;

        const inventory_entry &get_highlighted() const;
        inventory_entry &get_highlighted();
        std::vector<inventory_entry *> get_all_selected() const;
        using get_entries_t = std::vector<inventory_entry *>;
        using ffilter_t = std::function<bool( const inventory_entry &entry )>;
        get_entries_t get_entries( const ffilter_t &filter_func,
                                   bool include_hidden = false ) const;

        // orders the child entries in this column to be under their parent
        void order_by_parent();

        inventory_entry *find_by_invlet( int invlet ) const;
        inventory_entry *find_by_location( item_location const &loc, bool hidden = false ) const;

        void draw( const catacurses::window &win, const point &p,
                   std::vector< std::pair<inclusive_rectangle<point>, inventory_entry *>> &rect_entry_map );

        inventory_entry *add_entry( const inventory_entry &entry );
        void move_entries_to( inventory_column &dest );
        void clear();
        void set_stack_favorite( inventory_entry &entry, bool favorite );

        void set_collapsed( inventory_entry &entry, bool collapse );

        /** Highlights the specified location. */
        bool highlight( const item_location &loc, bool front_only = false );

        /**
         * Change the highlight.
         * @param new_index Index of the entry to highlight.
         * @param dir If the entry is not highlightable, move in the specified direction
         */
        void highlight( size_t new_index, scroll_direction dir );

        size_t get_highlighted_index() const {
            return highlighted_index;
        }

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_visibility( bool visibility ) {
            this->visibility = visibility;
        }

        void set_width( size_t new_width );
        void set_height( size_t new_height );
        size_t get_width() const;
        size_t get_height() const;
        /** Expands the column to fit the new entry. */
        void expand_to_fit( inventory_entry &entry, bool with_denial = true );
        /** Resets width to original (unchanged). */
        virtual void reset_width( const std::vector<inventory_column *> &all_columns );
        /** Returns next custom inventory letter. */
        int reassign_custom_invlets( const Character &p, int min_invlet, int max_invlet );
        int reassign_custom_invlets( int cur_idx, std::string_view pickup_chars );
        /** Reorder entries, repopulate titles, adjust to the new height. */
        virtual void prepare_paging( const std::string &filter = "" );
        /**
         * Event handlers
         */
        virtual void on_input( const inventory_input &input );
        /** The entry has been changed. */
        virtual void on_change( const inventory_entry &entry );
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

        virtual void set_filter( const std::string &filter );

        // whether or not to indent contained entries
        bool indent_entries() const {
            if( indent_entries_override ) {
                return *indent_entries_override;
            } else {
                return preset.indent_entries();
            }
        }

        bool collate_entries() const {
            return preset.collate_entries();
        }

        void set_indent_entries_override( bool entry_override ) {
            indent_entries_override = entry_override;
        }

        void clear_indent_entries_override() {
            indent_entries_override = std::nullopt;
        }

        void invalidate_paging() {
            paging_is_valid = false;
        }

        /** Toggle being able to highlight unselectable entries*/
        void toggle_skip_unselectable( bool skip );
        void collate();
        void uncollate();
        virtual void cycle_hide_override();

    protected:
        /**
         * Move the selection.
         */
        void move_selection( scroll_direction dir );
        void move_selection_page( scroll_direction dir );
        void scroll_selection_page( scroll_direction dir );

        size_t next_highlightable_index( size_t index, scroll_direction dir ) const;

        size_t page_of( size_t index ) const;
        size_t page_of( const inventory_entry &entry ) const;

        bool sort_compare( inventory_entry const &lhs, inventory_entry const &rhs );
        bool indented_sort_compare( inventory_entry const &lhs, inventory_entry const &rhs );
        bool collated_sort_compare( inventory_entry const &lhs, inventory_entry const &rhs );

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
        size_t get_entry_cell_width( const inventory_entry &entry, size_t cell_index ) const;
        /** Sum of the cell widths */
        size_t get_cells_width() const;

        const inventory_selector_preset &preset;

        using entries_t = std::vector<inventory_entry>;
        entries_t entries;
        entries_t entries_hidden;
        navigation_mode mode = navigation_mode::ITEM;
        bool active = false;
        bool multiselect = false;
        bool paging_is_valid = false;
        bool visibility = true;

        size_t highlighted_index = std::numeric_limits<size_t>::max();
        size_t page_offset = 0;
        size_t entries_per_page = std::numeric_limits<size_t>::max();
        size_t height = std::numeric_limits<size_t>::max();
        size_t reserved_width = 0;
        std::optional<bool> hide_entries_override = std::nullopt;

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

        std::optional<bool> indent_entries_override = std::nullopt;
        /** @return Number of visible cells */
        size_t visible_cells() const;
        void _get_entries( get_entries_t *res, entries_t const &ent,
                           const ffilter_t &filter_func ) const;
        static void _move_entries_to( entries_t const &ent, inventory_column &dest );
        static void _reset_collation( entries_t &ent );

        bool skip_unselectable = false;
        bool _collated = false;
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

        void set_filter( const std::string &filter ) override;
        void cycle_hide_override() override;

    private:
        const pimpl<item_category> selected_cat;
        inventory_entry last_changed;
};

class inventory_selector
{
    public:
        explicit inventory_selector( Character &u,
                                     const inventory_selector_preset &preset = default_preset );
        virtual ~inventory_selector();
        /** These functions add items from map / vehicles. */
        bool add_contained_items( item_location &container );
        bool add_contained_items( item_location &container, inventory_column &column,
                                  const item_category *custom_category = nullptr, item_location const &topmost_parent = {},
                                  int indent = 0 );
        void add_contained_gunmods( Character &you, item &gun );
        void add_contained_ebooks( item_location &container );
        void add_character_items( Character &character );
        void add_map_items( const tripoint &target );
        void add_vehicle_items( const tripoint &target );
        void add_nearby_items( int radius = 1 );
        void add_remote_map_items( tinymap *remote_map, const tripoint &target );
        void add_basecamp_items( const basecamp &camp );
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
        uint64_t item_entry_count() const;
        drop_location get_only_choice() const;

        /** Apply filter string to all columns */
        void set_filter( const std::string &str );
        /** Get last filter string set by set_filter or entered by player */
        std::string get_filter() const;

        enum selector_invlet_type {
            SELECTOR_INVLET_DEFAULT,
            SELECTOR_INVLET_NUMERIC,
            SELECTOR_INVLET_ALPHA
        };
        /** Set the letter group to use for automatic inventory letters */
        void set_invlet_type( selector_invlet_type type ) {
            this->invlet_type_ = type;
        }
        /** @return the letter group to use for automatic inventory letters */
        selector_invlet_type invlet_type() {
            return this->invlet_type_;
        }
        /** Set whether to show inventory letters */
        void show_invlet( bool show ) {
            this->use_invlet = show;
        }
        /** @return true if invlets should be used on this menu */
        bool showing_invlet() const {
            return this->use_invlet;
        }

        void categorize_map_items( bool toggle );

        // An array of cells for the stat lines. Example: ["Weight (kg)", "10", "/", "20"].
        using stat = std::array<std::string, 4>;
        using stats = std::array<stat, 3>;

    protected:
        Character &u;
        const inventory_selector_preset &preset;

        /**
         * The input context for navigation, already contains some actions for movement.
         */
        input_context ctxt;

        const item_category *naturalize_category( const item_category &category,
                const tripoint &pos );

        inventory_entry *add_entry( inventory_column &target_column,
                                    std::vector<item_location> &&locations,
                                    const item_category *custom_category = nullptr,
                                    size_t chosen_count = 0, item_location const &topmost_parent = {},
                                    bool chevron = false );
        bool add_entry_rec( inventory_column &entry_column, inventory_column &children_column,
                            item_location &loc, item_category const *entry_category = nullptr,
                            item_category const *children_category = nullptr,
                            item_location const &topmost_parent = {}, int indent = 0 );

        bool drag_drop_item( item *sourceItem, item *destItem );

        inventory_input get_input();
        inventory_input process_input( const std::string &action, int ch );

        /** Given an action from the input_context, try to act according to it.
        * Should handle all actions standard to derived classes. **/
        void on_input( const inventory_input &input );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );

        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();

        size_t get_layout_width() const;
        size_t get_layout_height() const;

        /**
         * Query user for a string and return it.
         * @param val The default value to have set in the query prompt.
         * @return A tuple of a bool and string, bool is true if user confirmed.
         */
        std::pair< bool, std::string > query_string( const std::string &val, bool end_with_toggle = false );
        /** Query the user for a filter and apply it. */
        void query_set_filter();
        /** Query the user for count and return it. */
        int query_count( char init = 0, bool end_with_toggle = false );

        /** Tackles screen overflow */
        virtual void rearrange_columns( size_t client_width );

        static stat get_weight_and_length_stat( units::mass weight_carried,
                                                units::mass weight_capacity, const units::length &longest_length );
        static stat get_volume_stat( const units::volume
                                     &volume_carried, const units::volume &volume_capacity, const units::volume &largest_free_volume );
        static stat get_holster_stat( const units::volume
                                      &holster_volume, int used_holsters, int total_holsters );
        static stats get_weight_and_volume_and_holster_stats(
            units::mass weight_carried, units::mass weight_capacity,
            const units::volume &volume_carried, const units::volume &volume_capacity,
            const units::length &longest_length, const units::volume &largest_free_volume,
            const units::volume &holster_volume, int used_holsters, int total_holsters );

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

        inventory_entry *find_entry_by_coordinate( const point &coordinate ) const;
        inventory_entry *find_entry_by_location( item_location &loc ) const;

        const std::vector<inventory_column *> &get_all_columns() const {
            return columns;
        }
        std::vector<inventory_column *> get_visible_columns() const;

        std::vector< std::pair<inclusive_rectangle<point>, inventory_entry *>> rect_entry_map;
        /** Highlight parent and contents of highlighted item.
        */
        void highlight();

        /**
         * Show detailed item information for the selected item.
         *
         * Called from on_input() after user input of EXAMINE action.
         * Also called from on_input() on action EXAMINE_CONTENTS if sitem has no contents
         *
         * @param sitem the item to examine **/
        void action_examine( const item_location &sitem );

        virtual void reassign_custom_invlets();
        std::vector<inventory_column *> columns;

        // NOLINTNEXTLINE(cata-use-named-point-constants)
        point _fixed_origin{ -1, -1 }, _fixed_size{ -1, -1 };
        bool _categorize_map_items = false;
        bool force_single_column = false;

    private:
        // These functions are called from resizing/redraw callbacks of ui_adaptor
        // and should not be made protected or public.
        void prepare_layout( size_t client_width, size_t client_height );
        void prepare_layout();

        void resize_window( int width, int height );
        void refresh_window();

        void draw_header( const catacurses::window &w ) const;
        void draw_footer( const catacurses::window &w ) const;
        void draw_columns( const catacurses::window &w );
        void draw_frame( const catacurses::window &w ) const;
        void _add_map_items( tripoint const &target, item_category const &cat, item_stack &items,
                             std::function<item_location( item & )> const &floc );

    public:
        /**
         * Highlight a location
         * @param loc Location to highlight
         * @return true on success.
         */
        bool highlight( const item_location &loc, bool hidden = false, bool front_only = false );

        const inventory_entry &get_highlighted() const {
            return get_active_column().get_highlighted();
        }

        item_location get_collation_next() const;

        void highlight_position( std::pair<size_t, size_t> position ) {
            prepare_layout();
            set_active_column( position.first );
            get_active_column().highlight( position.second, scroll_direction::BACKWARD );
        }

        bool highlight_one_of( const std::vector<item_location> &locations, bool hidden = false );

        std::pair<size_t, size_t> get_highlighted_position() const {
            std::pair<size_t, size_t> position;
            position.first = active_column_index;
            position.second = get_active_column().get_highlighted_index();
            return position;
        }

        inventory_column &get_column( size_t index ) const;
        inventory_column &get_active_column() const {
            return get_column( active_column_index );
        }

        void toggle_categorize_contained();
        void set_active_column( size_t index );
        void toggle_skip_unselectable();

        enum class uimode : int {
            categories = 0,
            hierarchy,
            last,
        };

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
        bool use_invlet = true;
        selector_invlet_type invlet_type_ = SELECTOR_INVLET_DEFAULT;
        size_t entry_generation_number = 0;

        static bool skip_unselectable;

        uimode _uimode = uimode::categories;

        void _categorize( inventory_column &col );
        void _uncategorize( inventory_column &col );

    public:
        std::string action_bound_to_key( char key ) const;
};

template <>
struct enum_traits<inventory_selector::uimode> {
    static constexpr inventory_selector::uimode last = inventory_selector::uimode::last;
};

inventory_selector::stat display_stat( const std::string &caption, int cur_value, int max_value,
                                       const std::function<std::string( int )> &disp_func );

class inventory_pick_selector : public inventory_selector
{
    public:
        explicit inventory_pick_selector( Character &p,
                                          const inventory_selector_preset &preset = default_preset ) :
            inventory_selector( p, preset ) {
            drag_enabled = false;
        }
        bool drag_enabled;
        item_location execute();
};

class container_inventory_selector : public inventory_pick_selector
{
    public:
        explicit container_inventory_selector( Character &p, item_location &loc,
                                               const inventory_selector_preset &preset = default_preset ) :
            inventory_pick_selector( p, preset ), loc( loc ) {}

    protected:
        stats get_raw_stats() const override;

    private:
        item_location loc;
};

std::vector<item_location> get_possible_reload_targets( const item_location &target );
class ammo_inventory_selector : public inventory_selector
{
    public:
        explicit ammo_inventory_selector( Character &you, const item_location &reload_loc,
                                          const inventory_selector_preset &preset = default_preset );

        drop_location execute();
        void set_all_entries_chosen_count();
    private:
        void mod_chosen_count( inventory_entry &entry, int val );
        const item_location reload_loc;
};

class inventory_multiselector : public inventory_selector
{
    public:
        using GetStats = std::function<stats( const std::vector<std::pair<item_location, int>> )>;
        explicit inventory_multiselector( Character &p,
                                          const inventory_selector_preset &preset = default_preset,
                                          const std::string &selection_column_title = "",
                                          const GetStats & = {},
                                          bool allow_select_contained = false );
        drop_locations execute( bool allow_empty = false );
        void toggle_entry( inventory_entry &entry, size_t count );
    protected:
        void rearrange_columns( size_t client_width ) override;
        size_t max_chosen_count;
        void set_chosen_count( inventory_entry &entry, size_t count );
        void deselect_contained_items();
        void toggle_entries( int &count, toggle_mode mode = toggle_mode::SELECTED );
        std::vector<std::pair<item_location, int>> to_use;
        std::vector<item_location> usable_locs;
        bool allow_select_contained;
        virtual void on_toggle() {};
        void on_input( const inventory_input &input );
        int count = 0;
        stats get_raw_stats() const override;
        void toggle_categorize_contained();
    private:
        std::unique_ptr<inventory_column> selection_col;
        GetStats get_stats;
};

class inventory_haul_selector : public inventory_multiselector
{
    public:
        explicit inventory_haul_selector( Character &p );
        void apply_selection( std::vector<item_location> &items );
};

class haul_selector_preset : public inventory_selector_preset
{
    public:
        bool is_shown( const item_location &item ) const override;
        std::string get_denial( const item_location &item ) const override;
};

class inventory_compare_selector : public inventory_multiselector
{
    public:
        explicit inventory_compare_selector( Character &p );
        std::pair<const item *, const item *> execute();

    protected:
        std::vector<const item *> compared;
        void toggle_entry( inventory_entry *entry );
};

class inventory_drop_selector : public inventory_multiselector
{
    public:
        explicit inventory_drop_selector(
            Character &p,
            const inventory_selector_preset &preset = default_preset,
            const std::string &selection_column_title = _( "ITEMS TO DROP" ),
            bool warn_liquid = true );
        drop_locations execute();
    protected:
        stats get_raw_stats() const override;

    private:
        bool warn_liquid;
};

class inventory_insert_selector : public inventory_drop_selector
{
    public:
        explicit inventory_insert_selector(
            Character &p,
            const inventory_holster_preset &preset,
            const std::string &selection_column_title = _( "ITEMS TO INSERT" ),
            bool warn_liquid = true );
    protected:
        stats get_raw_stats() const override;
};

class pickup_selector : public inventory_multiselector
{
    public:
        explicit pickup_selector( Character &p, const inventory_selector_preset &preset = default_preset,
                                  const std::string &selection_column_title = _( "ITEMS TO PICK UP" ),
                                  const std::optional<tripoint> &where = std::nullopt );
        drop_locations execute();
        void apply_selection( std::vector<drop_location> selection );
    protected:
        stats get_raw_stats() const override;
        void reassign_custom_invlets() override;
    private:
        bool wield( int &count );
        bool wear();
        void remove_from_to_use( item_location &it );
        void add_reopen_activity();
        const std::optional<tripoint> where;
};

class unload_selector : public inventory_pick_selector
{
    public:
        explicit unload_selector( Character &p, const inventory_selector_preset &preset = default_preset );
        std::pair<item_location, bool> execute();
    private:
        std::string hint_string();
};

/**
 * Class for opening a container and quickly examining the items contained within
 *
 * Class that lists inventory entries in a pane on the left, and shows the results of 'e'xamining
 * the selected item on the right.  To use, create it, add_contained_items(), then execute().
 * TODO: Ideally, add_contained_items could be done automatically on creation without duplicating
 * that code from inventory_selector. **/
#define EXAMINED_CONTENTS_UNCHANGED 0
#define EXAMINED_CONTENTS_WITH_CHANGES 1
#define NO_CONTENTS_TO_EXAMINE 2
class inventory_examiner : public inventory_selector
{
    private:
        int examine_window_scroll;
        int scroll_item_info_lines;

        void force_max_window_size();

    protected:
        item_location parent_item;
        item_location selected_item;
        catacurses::window w_examine;
        bool changes_made;
        bool parent_was_collapsed;

    public:
        explicit inventory_examiner( Character &p,
                                     item_location item_to_look_inside,
                                     const inventory_selector_preset &preset = default_preset ) :
            inventory_selector( p, preset ) {
            force_max_window_size();
            examine_window_scroll = 0;
            selected_item = item_location::nowhere;
            parent_item = std::move( item_to_look_inside );
            changes_made = false;
            parent_was_collapsed = false;

            //Space in inventory isn't particularly relevant, so don't display it
            set_display_stats( false );

            setup();
        }

        /**
         * If parent_item has no contents or is otherwise unsuitable for inventory_examiner, return false.  Otherwise, true
        **/
        bool check_parent_item();

        /**
         * If the parent_item had items hidden, re-hides them.  Determines the appropriate return value for execute()
        *
         * Called at the end of execute().
         * Checks if anything was changed (e.g. show/hide contents), and selects the appropriate return value
              **/
        int cleanup() const;

        /**
         * Draw the details of sitem in the w_examine window
        **/
        void draw_item_details( const item_location &sitem );

        /**
         * Method to display the inventory_examiner menu.
         *
        * Sets up ui_adaptor callbacks for w_examine to draw the item detail pane and allow it to be resized
         * Figures out which item is currently selected and calls draw_item_details
        * Passes essentially everything else back to inventory_selector for handling.
         * If the user changed something while looking through the item's contents (e.g. collapsing a
         * container), it should return EXAMINED_CONTENTS_WITH_CHANGES to inform the parent window.
         * If the parent_item has no contents to examine, it should return NO_CONTENTS_TO_EXAMINE, telling
         * the parent window to examine the item with action_examine()
         **/
        int execute();

        /**
         * Does initial setup work prior to display of the window
         **/
        void setup();
};

bool is_worn_ablative( item_location const &container, item_location const &child );

struct inventory_selector_save_state {
    public:
        inventory_selector::uimode uimode = inventory_selector::uimode::categories;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonObject const &jo );
};
extern inventory_selector_save_state inventory_ui_default_state;
extern inventory_selector_save_state pickup_sel_default_state;
extern inventory_selector_save_state pickup_ui_default_state;

#endif // CATA_SRC_INVENTORY_UI_H
