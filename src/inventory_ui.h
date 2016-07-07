#ifndef INVENTORY_UI_H
#define INVENTORY_UI_H

#include <climits>
#include <memory>

#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "input.h"

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

const item_location_filter allow_all_items = []( const item_location & )
{
    return true;
};

class inventory_entry
{
    private:
        std::shared_ptr<item_location> location;
        size_t stack_size;
        const item_category *custom_category;
        long custom_invlet;

    public:
        size_t chosen_count = 0;
        nc_color custom_color = c_unset;

        inventory_entry( const std::shared_ptr<item_location> &location, size_t stack_size,
                         const item_category *custom_category = nullptr,
                         nc_color custom_color = c_unset, long custom_invlet = LONG_MIN )
            : location( ( location != nullptr ) ? location :
                        std::make_shared<item_location>() ), // to make sure that location != nullptr always
              stack_size( stack_size ),
              custom_category( custom_category ),
              custom_invlet( custom_invlet ),
              custom_color( custom_color ) {}

        inventory_entry( const std::shared_ptr<item_location> &location,
                         const item_category *custom_category = nullptr, nc_color custom_color = c_unset,
                         long custom_invlet = LONG_MIN );

        inventory_entry( const item_category *custom_category = nullptr )
            : inventory_entry( std::make_shared<item_location>(), custom_category ) {}

        inventory_entry( const inventory_entry &entry, const item_category *custom_category )
            : inventory_entry( entry ) {
            this->custom_category = custom_category;
        }

        // used for searching the category header, only the item pointer and the category are important there
        bool operator == ( const inventory_entry &other ) const;
        bool operator != ( const inventory_entry &other ) const {
            return !( *this == other );
        }

        bool is_item() const;
        bool is_null() const;

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

class inventory_column
{
    public:
        inventory_column() :
            entries(),
            mode( navigation_mode::ITEM ),
            active( false ),
            multiselect( false ),
            custom_colors( false ),
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

        void set_multiselect( bool multiselect ) {
            this->multiselect = multiselect;
        }

        void set_custom_colors( bool custom_colors ) {
            this->custom_colors = custom_colors;
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
        size_t page_of( size_t index ) const;
        size_t page_of( const inventory_entry &entry ) const;

        virtual std::string get_entry_text( const inventory_entry &entry ) const;
        virtual nc_color get_entry_color( const inventory_entry &entry ) const;
        size_t get_entry_indent( const inventory_entry &entry ) const;
        size_t get_entry_width( const inventory_entry &entry ) const;

        std::vector<inventory_entry> entries;
        navigation_mode mode;
        bool active;
        bool multiselect;
        bool custom_colors;
        size_t selected_index;
        size_t page_offset;
        size_t entries_per_page;
};

class selection_column : public inventory_column
{
    public:
        selection_column( const std::string &id, const std::string &name );

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

    private:
        const std::unique_ptr<item_category> selected_cat;
        size_t reserved_width;
};

class inventory_selector
{
    public:
        inventory_selector( player &u, const std::string &title,
                            const item_location_filter &filter = allow_all_items );
        ~inventory_selector();
        /** These functions add items from map / vehicles */
        void add_map_items( const tripoint &target );
        void add_vehicle_items( const tripoint &target );
        void add_nearby_items( int radius = 1 );
        /** Checks the selector for emptiness (absence of available entries). */
        bool empty() const;

    protected:
        player &u;
        /** The input context for navigation, already contains some actions for movement.
         * See @ref on_action */
        input_context ctxt;

        void add_custom_items( const std::list<item>::const_iterator &from,
                               const std::list<item>::const_iterator &to,
                               const std::string &title,
                               const std::function<std::shared_ptr<item_location>( item * )> &locator );

        /** Refreshes item categories */
        void prepare_columns( bool multiselect );
        /** Given an action from the input_context, try to act according to it. */
        void on_action( const std::string &action );
        /** Entry has been changed */
        void on_change( const inventory_entry &entry );

        void refresh_window() const;

        virtual void draw( WINDOW *w ) const;

        void draw_inv_weight_vol( WINDOW *w, int weight_carried, int vol_carried, int vol_capacity ) const;
        void draw_inv_weight_vol( WINDOW *w ) const;

        /** Returns an entry from @ref entries by its invlet */
        inventory_entry *find_entry_by_invlet( long invlet ) const;

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
        void insert_selection_column( const std::string &id, const std::string &name );

    private:
        static const long min_custom_invlet = '0';
        static const long max_custom_invlet = '9';
        long cur_custom_invlet = min_custom_invlet;

        const std::string title;
        const item_location_filter filter;
        WINDOW *w_inv;

        std::vector<std::unique_ptr<inventory_column>> columns;
        std::unique_ptr<inventory_column> custom_column;
        size_t active_column_index;
        std::list<item_category> categories;
        navigation_mode navigation;

        void insert_column( decltype( columns )::iterator position,
                            std::unique_ptr<inventory_column> &new_column );
};

class inventory_pick_selector : public inventory_selector
{
    public:
        inventory_pick_selector( player &u, const std::string &title,
                                 const item_location_filter &filter = allow_all_items );
        item_location &execute();

    protected:
        std::unique_ptr<item_location> null_location;

        virtual void draw( WINDOW *w ) const override;
};

class inventory_compare_selector : public inventory_selector
{
    public:
        inventory_compare_selector( player &u, const std::string &title,
                                    const item_location_filter &filter = allow_all_items );
        std::pair<const item *, const item *> execute();

    protected:
        std::vector<inventory_entry *> compared;

        virtual void draw( WINDOW *w ) const override;
        void toggle_entry( inventory_entry *entry );
};

class inventory_drop_selector : public inventory_selector
{
    public:
        inventory_drop_selector( player &u, const std::string &title,
                                 const item_location_filter &filter = allow_all_items );
        std::list<std::pair<int, int>> execute();

    protected:
        std::map<const item *, int> dropping;

        virtual void draw( WINDOW *w ) const override;
        /** Toggle item dropping */
        void set_drop_count( inventory_entry &entry, size_t count );
        void remove_dropping_items( player &u ) const;
};

#endif
