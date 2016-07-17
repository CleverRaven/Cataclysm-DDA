#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include <climits>
#include <memory>

#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "input.h"
#include "units.h"

class Character;

struct inventory_input;

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
    private:
        std::shared_ptr<item_location> location;
        size_t stack_size;
        const item_category *custom_category;

    public:
        bool enabled = true;
        int rank = 0;
        size_t chosen_count = 0;
        long custom_invlet = LONG_MIN;

        inventory_entry( const std::shared_ptr<item_location> &location, size_t stack_size,
                         const item_category *custom_category = nullptr ) :
            location( ( location != nullptr ) ? location : std::make_shared<item_location>() ),
            stack_size( stack_size ),
            custom_category( custom_category ) {}

        inventory_entry( const std::shared_ptr<item_location> &location,
                         const item_category *custom_category = nullptr );

        inventory_entry( const item_category *custom_category = nullptr ) :
            inventory_entry( std::make_shared<item_location>(), custom_category ) {}

        inventory_entry( const inventory_entry &entry, const item_category *custom_category ) :
            inventory_entry( entry ) {
            this->custom_category = custom_category;
        }

        // used for searching the category header, only the item pointer and the category are important there
        bool operator == ( const inventory_entry &other ) const;
        bool operator != ( const inventory_entry &other ) const {
            return !( *this == other );
        }

        bool is_item() const;
        bool is_null() const;
        bool is_selectable() const {
            return enabled && is_item();
        }

        size_t get_stack_size() const {
            return stack_size;
        }

        size_t get_available_count() const;
        const item &get_item() const;
        const item_category *get_category_ptr() const;
        long get_invlet() const;

        item_location &get_location() const {
            return *location;
        }
};

class inventory_selector_preset
{
    public:
        inventory_selector_preset();

        virtual bool is_shown( const item &it ) const;
        virtual bool is_shown( const item_location &location ) const;

        virtual bool is_enabled( const item &it ) const;
        virtual bool is_enabled( const item_location &location ) const;

        virtual int get_rank( const item &it ) const;
        virtual int get_rank( const item_location &location ) const;

        virtual nc_color get_color( const item &it ) const;
        virtual nc_color get_color( const item_location &location ) const;

        std::string get_cell_text( const inventory_entry &entry, size_t cell_index ) const;
        size_t get_cell_width( const inventory_entry &entry, size_t cell_index ) const;
        nc_color get_cell_color( const inventory_entry &entry, size_t cell_index ) const;

        size_t get_cells_count() const {
            return cells.size();
        }

    protected:
        using item_text_func = std::function<std::string( const item & )>;
        using item_location_text_func = std::function<std::string( const item_location & )>;
        using entry_text_func = std::function<std::string( const inventory_entry & )>;

        using cell_pair = std::pair<std::string, entry_text_func>;

        virtual std::string get_caption( const inventory_entry &entry ) const;

        void append_cell( const item_text_func &func, const std::string &title = "" );
        void append_cell( const item_location_text_func &func, const std::string &title = "" );
        void append_cell( const entry_text_func &func, const std::string &title = "" );

        template <typename T>
        std::string to_string( T val ) const {
            return val == T() ? "-" : std::to_string( val );
        }

    private:
        const std::string bug_text;
        std::vector<cell_pair> cells;
};

const inventory_selector_preset default_preset;

class inventory_filter_preset : public inventory_selector_preset
{
    public:
        inventory_filter_preset( const item_location_filter &filter ) : filter( filter ) {}

        bool is_shown( const item_location &location ) const override {
            return filter( location );
        }

    private:
        item_location_filter filter;
};

class inventory_column
{
    public:
        inventory_column( const inventory_selector_preset &preset = default_preset ) :
            preset( preset ) {}
        virtual ~inventory_column() {}

        bool empty() const {
            return entries.empty();
        }

        // true if can be activated
        virtual bool activatable() const;
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
        std::vector<inventory_entry *> get_all_selected();
        const std::vector<inventory_entry> &get_entries() const {
            return entries;
        }

        inventory_entry *find_by_invlet( long invlet );

        void draw( WINDOW *win, size_t x, size_t y ) const;

        void add_entry( const inventory_entry &entry );
        void add_entries( const inventory_column &source );

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_mode( navigation_mode mode ) {
            this->mode = mode;
        }

        void set_width( size_t width );
        void set_height( size_t height );
        /// Returns either entry width (if @param entry is not null) or whole width of the column
        size_t get_width( const inventory_entry *entry = nullptr ) const;
        size_t get_min_width( const inventory_entry *entry = nullptr ) const;
        /// Returns next custom inventory letter
        long reassign_custom_invlets( const player &p, long min_invlet, long max_invlet );

        virtual void prepare_paging();

        virtual void on_input( const inventory_input &input );
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

        size_t get_cell_width( const inventory_entry &entry, size_t cell_index ) const;
        size_t get_max_cell_width( size_t cell_index ) const;

        size_t get_entry_indent( const inventory_entry &entry ) const;

        std::vector<inventory_entry> entries;

    private:
        const int min_cell_margin = 1;

        const inventory_selector_preset &preset;
        navigation_mode mode = navigation_mode::ITEM;

        bool active = false;
        bool multiselect = false;
        bool paging_is_valid = false;

        size_t selected_index = 0;
        size_t page_offset = 0;
        size_t entries_per_page = 1;
        size_t assigned_width = 0;
};

class inventory_selector
{
    public:
        inventory_selector( const player &p, const inventory_selector_preset &preset = default_preset );
        ~inventory_selector();
        /** These functions add items from map / vehicles */
        void add_character_items( Character &character );
        void add_map_items( const tripoint &target );
        void add_vehicle_items( const tripoint &target );
        void add_nearby_items( int radius = 1 );
        /** Assigns a title that will be shown on top of the menu */
        void set_title( const std::string &title );
        /** Checks the selector for emptiness (absence of available entries). */
        bool empty() const;

    protected:
        const player &p;
        const inventory_selector_preset &preset;

        std::vector<std::shared_ptr<inventory_column>> columns;
        std::shared_ptr<inventory_column> custom_column;

        inventory_input get_input();

        void add_item( const std::shared_ptr<item_location> &location,
                       size_t stack_size = 1,
                       const std::string &custom_cat_title = "" );

        void add_items( const std::list<item>::iterator &from,
                        const std::list<item>::iterator &to,
                        const std::string &title,
                        const std::function<std::shared_ptr<item_location>( item * )> &locator );

        /** Given an action from the input_context, try to act according to it. */
        void on_input( const inventory_input &input );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );

        void refresh_window() const;

        /** Refreshes item categories */
        virtual void prepare_columns();
        virtual void draw( WINDOW *w ) const;

        void draw_inv_weight_vol( WINDOW *w, int weight_carried, units::volume vol_carried,
                                  units::volume vol_capacity ) const;
        void draw_inv_weight_vol( WINDOW *w ) const;

        /** @return an entry from @ref entries by its invlet */
        inventory_entry *find_entry_by_invlet( long invlet );

        inventory_column &get_column( size_t index ) const;
        inventory_column &get_active_column() const {
            return get_column( active_column_index );
        }

        void set_active_column( size_t index );
        size_t get_columns_width() const;
        /** @return percentage of the window occupied by columns */
        double get_columns_occupancy_ratio() const;

        bool column_can_fit( const inventory_column &column ) {
            return column.get_width() + get_columns_width() <= size_t( getmaxx( w_inv ) );
        }
        bool is_active_column( const inventory_column &column ) const {
            return &column == &get_active_column();
        }

        void insert_column( decltype( columns )::iterator position,
                            std::shared_ptr<inventory_column> &new_column );
        void toggle_active_column();
        void refresh_active_column() {
            if( !get_active_column().activatable() ) {
                toggle_active_column();
            }
        }
        void toggle_navigation_mode();
        void reassign_custom_invlets();

    private:
        WINDOW *w_inv;

        std::string title;
        size_t active_column_index;
        std::list<item_category> categories;
        navigation_mode navigation;
        input_context ctxt;
};

class inventory_pick_selector : public inventory_selector
{
    public:
        inventory_pick_selector( const player &p,
                                 const inventory_selector_preset &preset = default_preset );
        item_location &execute();

    protected:
        void draw( WINDOW *w ) const override;

    private:
        std::shared_ptr<item_location> null_location;
};

class inventory_multiselector : public inventory_selector
{
    public:
        inventory_multiselector( const player &p, const inventory_selector_preset &preset = default_preset,
                                 const std::string &selection_column_title = "" );
    protected:
        void prepare_columns() override;

    private:
        std::shared_ptr<inventory_column> selection_col;
};

class inventory_compare_selector : public inventory_multiselector
{
    public:
        inventory_compare_selector( const player &p );
        std::pair<const item *, const item *> execute();

    protected:
        std::vector<inventory_entry *> compared;

        void draw( WINDOW *w ) const override;
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

        void draw( WINDOW *w ) const override;
        /** Toggle item dropping */
        void set_drop_count( inventory_entry &entry, size_t count );
        void remove_dropping_items( player &u ) const;
};

#endif
