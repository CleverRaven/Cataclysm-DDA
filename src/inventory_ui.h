#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include <climits>
#include <memory>

#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "input.h"
#include "units.h"
#include "item_location.h"

class Character;

class item;
class item_category;
class item_location;

class player;

typedef std::function<bool( const item & )> item_filter;
typedef std::function<bool( const item_location & )> item_location_filter;

enum class navigation_mode : int {
    ITEM = 0,
    CATEGORY
};

class inventory_entry
{
    public:
        item_location location;

        size_t chosen_count = 0;
        long custom_invlet = LONG_MIN;

        inventory_entry( const item_location &location, size_t stack_size,
                         const item_category *custom_category = nullptr, bool enabled = true ) :
            location( location.clone() ),
            stack_size( stack_size ),
            custom_category( custom_category ),
            enabled( enabled ) {}

        inventory_entry( const inventory_entry &entry ) :
            location( entry.location.clone() ),
            chosen_count( entry.chosen_count ),
            custom_invlet( entry.custom_invlet ),
            stack_size( entry.stack_size ),
            custom_category( entry.custom_category ),
            enabled( entry.enabled ) {}

        inventory_entry operator=( const inventory_entry &rhs ) {
            location = rhs.location.clone();
            chosen_count = rhs.chosen_count;
            custom_invlet = rhs.custom_invlet;
            stack_size = rhs.stack_size;
            custom_category = rhs.custom_category;
            enabled = rhs.enabled;
            return *this;
        }

        inventory_entry( const item_location &location, const item_category *custom_category = nullptr,
                         bool enabled = true ) :
            inventory_entry( location, location ? 1 : 0, custom_category, enabled ) {}

        inventory_entry( const item_category *custom_category = nullptr ) :
            inventory_entry( item_location(), custom_category ) {}

        inventory_entry( const inventory_entry &entry, const item_category *custom_category ) :
            inventory_entry( entry ) {
            this->custom_category = custom_category;
        }

        // used for searching the category header, only the item pointer and the category are important there
        bool operator==( const inventory_entry &other ) const;
        bool operator!=( const inventory_entry &other ) const {
            return !( *this == other );
        }

        operator bool() const {
            return get_category_ptr() != nullptr;
        }

        bool is_selectable() const {
            return enabled && location;
        }

        size_t get_stack_size() const {
            return stack_size;
        }

        size_t get_available_count() const;
        const item_category *get_category_ptr() const;
        long get_invlet() const;

    private:
        size_t stack_size;
        const item_category *custom_category;
        bool enabled = true;

};

class inventory_selector_preset
{
    public:
        inventory_selector_preset();

        virtual bool is_shown( const item_location & ) const {
            return true;
        }

        /// @return The reason why this entry cannot be selected
        virtual std::string get_denial( const item_location & ) const {
            return std::string();
        }
        /// @return Whether the first item is considered to go before the second
        virtual bool sort_compare( const item_location &lhs, const item_location &rhs ) const;

        virtual nc_color get_color( const inventory_entry &entry ) const;

        std::string get_cell_text( const inventory_entry &entry, size_t cell_index ) const;
        size_t get_cell_width( const inventory_entry &entry, size_t cell_index ) const;

        size_t get_cells_count() const {
            return cells.size();
        }

    protected:
        virtual std::string get_caption( const inventory_entry &entry ) const;

        void append_cell( const std::function<std::string( const item_location & )> &func,
                          const std::string &title = "" );
        void append_cell( const std::function<std::string( const inventory_entry & )> &func,
                          const std::string &title = "" );

    private:
        using cell_pair = std::pair<std::string, std::function<std::string( const inventory_entry & )>>;
        std::vector<cell_pair> cells;
};

const inventory_selector_preset default_preset;

class inventory_column
{
    public:
        inventory_column( const inventory_selector_preset &preset = default_preset ) :
            preset( preset ),
            entries(),
            mode( navigation_mode::ITEM ),
            active( false ),
            multiselect( false ),
            paging_is_valid( false ),
            visibility( true ),
            selected_index( 0 ),
            page_offset( 0 ),
            entries_per_page( 1 ) {

            cell_widths.resize( preset.get_cells_count(), 0 );
            min_cell_widths.resize( preset.get_cells_count(), 0 );
        }

        virtual ~inventory_column() {}

        bool empty() const {
            return entries.empty();
        }
        // true if can be activated
        virtual bool activatable() const;
        // true if displayed
        bool visible() const {
            return !empty() && visibility && preset.get_cells_count() > 0;
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

        inventory_entry *find_by_invlet( long invlet ) const;

        void draw( WINDOW *win, size_t x, size_t y ) const;

        void add_entry( const inventory_entry &entry );
        void remove_entry( const inventory_entry &entry );
        void move_entries_to( inventory_column &dest );
        void clear();

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_mode( navigation_mode mode ) {
            this->mode = mode;
        }

        void set_visibility( bool visibility ) {
            this->visibility = visibility;
        }

        void set_width( size_t width );
        void set_height( size_t height );
        size_t get_width() const;
        size_t get_height() const {
            return entries_per_page;
        }
        /** Expands the column to fit the new entry */
        void expand_to_fit( const inventory_entry &entry );
        /** Resets width to original (unchanged) */
        void reset_width();
        /** Returns next custom inventory letter */
        long reassign_custom_invlets( const player &p, long min_invlet, long max_invlet );

        virtual void prepare_paging();

        virtual void on_action( const std::string &action );
        virtual void on_change( const inventory_entry & ) {}

        virtual void on_activate() {
            active = true;
        }
        virtual void on_deactivate() {
            active = false;
        }

    protected:
        void select( size_t new_index, int step = 0 );
        void move_selection( int step );

        size_t page_of( size_t index ) const;
        size_t page_of( const inventory_entry &entry ) const;

        size_t get_entry_indent( const inventory_entry &entry, size_t cell_index = 0 ) const;
        size_t get_entry_cell_width( const inventory_entry &entry, size_t cell_index ) const;
        std::string get_entry_denial( const inventory_entry &entry ) const;

        std::vector<size_t> cell_widths;        /// Current cell widths (can be affected by set_width())
        std::vector<size_t> min_cell_widths;    /// Minimal cell widths (to embrace all the entries nicely)

        const inventory_selector_preset &preset;

        std::vector<inventory_entry> entries;
        navigation_mode mode;
        bool active;
        bool multiselect;
        bool paging_is_valid;
        bool visibility;

        size_t selected_index;
        size_t page_offset;
        size_t entries_per_page;
};

class selection_column : public inventory_column
{
    public:
        selection_column( const std::string &id, const std::string &name );

        virtual bool activatable() const override {
            return inventory_column::activatable() && pages_count() > 1;
        }

        virtual bool allows_selecting() const override {
            return false;
        }

        virtual void prepare_paging() override;
        virtual void on_change( const inventory_entry &entry ) override;

    private:
        const std::unique_ptr<item_category> selected_cat;
};

class inventory_selector
{
    public:
        inventory_selector( const player &u, const inventory_selector_preset &preset = default_preset );
        ~inventory_selector();
        /** These functions add items from map / vehicles */
        void add_character_items( Character &character );
        void add_map_items( const tripoint &target );
        void add_vehicle_items( const tripoint &target );
        void add_nearby_items( int radius = 1 );
        /** Assigns a title that will be shown on top of the menu */
        void set_title( const std::string &title ) {
            this->title = title;
        }
        /** Assigns a hint */
        void set_hint( const std::string &hint ) {
            this->hint = hint;
        }
        void set_display_stats( bool display_stats ) {
            this->display_stats = display_stats;
        }
        /** @return true when the selector is empty */
        bool empty() const;
        /** @return true when there are enabled entries to select */
        bool has_available_choices() const;

    protected:
        const player &u;
        const inventory_selector_preset &preset;

        /** The input context for navigation, already contains some actions for movement.
         * See @ref on_action */
        input_context ctxt;

        const item_category *naturalize_category( const item_category &category,
                const tripoint &pos );

        void add_item( inventory_column &target_column,
                       const item_location &location,
                       size_t stack_size = 1,
                       const item_category *custom_category = nullptr );

        void add_items( inventory_column &target_column,
                        const std::function<item_location( item * )> &locator,
                        const std::vector<std::list<item *>> &stacks,
                        const item_category *custom_category = nullptr );

        /** Given an action from the input_context, try to act according to it. */
        void on_action( const std::string &action );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );

        void prepare_layout();
        void refresh_window() const;
        void update();

        /** Tackles screen overflow */
        virtual void rearrange_columns();
        /** Returns player for volume/weight numbers */
        virtual const player &get_player_for_stats() const {
            return u;
        }

        int get_header_height() const;
        int get_column_height() const;

        void draw_header( WINDOW *w ) const;
        void draw_footer( WINDOW *w ) const;
        void draw_columns( WINDOW *w ) const;

        /** @return an entry from @ref entries by its invlet */
        inventory_entry *find_entry_by_invlet( long invlet ) const;

        const std::vector<inventory_column *> &get_all_columns() const {
            return columns;
        }
        std::vector<inventory_column *> get_visible_columns() const;

        inventory_column &get_column( size_t index ) const;
        inventory_column &get_active_column() const {
            return get_column( active_column_index );
        }

        void set_active_column( size_t index );
        size_t get_columns_width( const std::vector<inventory_column *> &columns ) const;
        /** @return Percentage of the window occupied by columns */
        double get_columns_occupancy_ratio() const;
        /** @return Do the visible columns need to be center-aligned */
        bool are_columns_centered() const;
        /** @return Are visible columns wider than available width */
        bool is_overflown() const;

        bool is_active_column( const inventory_column &column ) const {
            return &column == &get_active_column();
        }

        void append_column( inventory_column &column );

        /**
         * Activates either previous or next column.
         * @param direction Positive number or zero - next column, negative - previous.
         */
        void toggle_active_column( int direction = 0 );

        void refresh_active_column() {
            if( !get_active_column().activatable() ) {
                toggle_active_column();
            }
        }
        void toggle_navigation_mode();
        void reassign_custom_invlets();

        /** Entry has been added */
        virtual void on_entry_add( const inventory_entry & ) {}

    private:
        WINDOW *w_inv;

        std::list<item_location> items;
        std::vector<inventory_column *> columns;

        std::string title;
        std::string hint;
        size_t active_column_index;
        std::list<item_category> categories;
        navigation_mode navigation;

        inventory_column own_inv_column;     // Column for own inventory items
        inventory_column own_gear_column;    // Column for own gear (weapon, armor) items
        inventory_column map_column;         // Column for map and vehicle items

        bool display_stats = true;
        bool layout_is_valid = false;
};

class inventory_pick_selector : public inventory_selector
{
    public:
        inventory_pick_selector( const player &p,
                                 const inventory_selector_preset &preset = default_preset ) :
            inventory_selector( p, preset ) {}

        item_location execute();
};

class inventory_multiselector : public inventory_selector
{
    public:
        inventory_multiselector( const player &p, const inventory_selector_preset &preset = default_preset,
                                 const std::string &selection_column_title = "" );
    protected:
        virtual void rearrange_columns() override;
        virtual void on_entry_add( const inventory_entry &entry ) override;

    private:
        std::unique_ptr<inventory_column> selection_col;
};

class inventory_compare_selector : public inventory_multiselector
{
    public:
        inventory_compare_selector( const player &p );
        std::pair<const item *, const item *> execute();

    protected:
        std::vector<inventory_entry *> compared;

        void toggle_entry( inventory_entry *entry );
};

class inventory_drop_selector : public inventory_multiselector
{
    public:
        inventory_drop_selector( const player &p,
                                 const inventory_selector_preset &preset = default_preset );
        std::list<std::pair<int, int>> execute();

    protected:
        std::map<const item *, int> dropping;
        mutable std::unique_ptr<player> dummy;

        const player &get_player_for_stats() const;
        /** Toggle item dropping */
        void set_drop_count( inventory_entry &entry, size_t count );
};

#endif
