#pragma once
#ifndef ADVANCED_INV_H
#define ADVANCED_INV_H

#include <cctype>
#include <cstddef>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "advanced_inv_area.h"

class uilist;
class vehicle;
class item;

enum advanced_inv_sortby {
    SORTBY_NONE,
    SORTBY_NAME,
    SORTBY_WEIGHT,
    SORTBY_VOLUME,
    SORTBY_CHARGES,
    SORTBY_CATEGORY,
    SORTBY_DAMAGE,
    SORTBY_AMMO,
    SORTBY_SPOILAGE
};

struct sort_case_insensitive_less : public std::binary_function< char, char, bool > {
    bool operator()( char x, char y ) const {
        return toupper( static_cast< unsigned char >( x ) ) < toupper( static_cast< unsigned char >( y ) );
    }
};

void advanced_inv();

/**
 * Cancels ongoing move all action.
 * TODO: Make this not needed.
 */
void cancel_aim_processing();


// see item_factory.h
class item_category;

/**
 * Entry that is displayed in a adv. inv. pane. It can either contain a
 * single item or a category header or nothing (empty entry).
 * Most members are used only for sorting.
 */
struct advanced_inv_listitem {
    using itype_id = std::string;
    /**
     * Index of the item in the itemstack.
     */
    int idx;
    /**
     * The location of the item, never AIM_ALL.
     */
    aim_location area;
    // the id of the item
    itype_id id;
    // The list of items, and empty when a header
    std::list<item *> items;
    /**
     * The displayed name of the item/the category header.
     */
    std::string name;
    /**
     * Name of the item (singular) without damage (or similar) prefix, used for sorting.
     */
    std::string name_without_prefix;
    /**
     * Whether auto pickup is enabled for this item (based on the name).
     */
    bool autopickup;
    /**
     * The stack count represented by this item, should be >= 1, should be 1
     * for anything counted by charges.
     */
    int stacks;
    /**
     * The volume of all the items in this stack, used for sorting.
     */
    units::volume volume;
    /**
     * The weight of all the items in this stack, used for sorting.
     */
    units::mass weight;
    /**
     * The item category, or the category header.
     */
    const item_category *cat;
    /**
     * Is the item stored in a vehicle?
     */
    bool from_vehicle;
    /**
     * Whether this is a category header entry, which does *not* have a reference
     * to an item, only @ref cat is valid.
     */
    bool is_category_header() const;

    /** Returns true if this is an item entry */
    bool is_item_entry() const;
    /**
     * Create a category header entry.
     * @param cat The category reference, must not be null.
     */
    advanced_inv_listitem( const item_category *cat );
    /**
     * Creates an empty entry, both category and item pointer are null.
     */
    advanced_inv_listitem();
    /**
     * Create a normal item entry.
     * @param an_item The item pointer. Must not be null.
     * @param index The index
     * @param count The stack size
     * @param area The source area. Must not be AIM_ALL.
     * @param from_vehicle Is the item from a vehicle cargo space?
     */
    advanced_inv_listitem( item *an_item, int index, int count,
                           aim_location area, bool from_vehicle );
    /**
     * Create a normal item entry.
     * @param list The list of item pointers.
     * @param index The index
     * @param area The source area. Must not be AIM_ALL.
     * @param from_vehicle Is the item from a vehicle cargo space?
     */
    advanced_inv_listitem( const std::list<item *> &list, int index,
                           aim_location area, bool from_vehicle );
};

/**
 * Displayed pane, what is shown on the screen.
 */
class advanced_inventory_pane
{
    private:
        aim_location area = NUM_AIM_LOCATIONS;
        aim_location prev_area = area;
        // pointer to the square this pane is pointing to
        bool viewing_cargo = false;
        bool prev_viewing_cargo = false;
    public:
        // set the pane's area via its square, and whether it is viewing a vehicle's cargo
        void set_area( advanced_inv_area &square, bool in_vehicle_cargo = false ) {
            prev_area = area;
            prev_viewing_cargo = viewing_cargo;
            area = square.id;
            viewing_cargo = square.can_store_in_vehicle() && in_vehicle_cargo;
        }
        void restore_area() {
            area = prev_area;
            viewing_cargo = prev_viewing_cargo;
        }
        aim_location get_area() const {
            return area;
        }
        bool prev_in_vehicle() const {
            return prev_viewing_cargo;
        }
        bool in_vehicle() const {
            return viewing_cargo;
        }
        bool on_ground() const {
            return area > AIM_INVENTORY && area < AIM_DRAGGED;
        }
        /**
         * Index of the selected item (index of @ref items),
         */
        int index;
        advanced_inv_sortby sortby;
        catacurses::window window;
        std::vector<advanced_inv_listitem> items;
        /**
         * The current filter string.
         */
        std::string filter;
        /**
         * Whether to recalculate the content of this pane.
         * Implies @ref redraw.
         */
        bool recalc;
        /**
         * Whether to redraw this pane.
         */
        bool redraw;

        void add_items_from_area( advanced_inv_area &square, bool vehicle_override = false );
        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        /**
         * @param it The item to check, oly the name member is examined.
         * @return Whether the item should be filtered (and not shown).
         */
        bool is_filtered( const advanced_inv_listitem &it ) const;
        /**
         * Same as the other, but checks the real item.
         */
        bool is_filtered( const item &it ) const;
        /**
         * Scroll @ref index, by given offset, set redraw to true,
         * @param offset Must not be 0.
         */
        void scroll_by( int offset );
        /**
         * Scroll the index in category mode by given offset.
         * @param offset Must be either +1 or -1
         */
        void scroll_category( int offset );
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        advanced_inv_listitem *get_cur_item_ptr();
        /**
         * Set the filter string, disables filtering when the filter string is empty.
         */
        void set_filter( const std::string &new_filter );
        /**
         * Insert additional category headers on the top of each page.
         */
        void paginate( size_t itemsPerPage );
    private:
        /** Scroll to next non-header entry */
        void skip_category_headers( int offset );
        /** Only add offset to index, but wrap around! */
        void mod_index( int offset );

        mutable std::map<std::string, std::function<bool( const item & )>> filtercache;
};

class advanced_inventory
{
    public:
        advanced_inventory();
        ~advanced_inventory();

        void display();

        /**
         * Converts from screen relative location to game-space relative location
         * for control rotation in isometric mode.
        */
        static aim_location screen_relative_location( aim_location area );

        static char get_location_key( aim_location area );

        advanced_inv_area &get_one_square( const aim_location &loc ) {
            return squares[loc];
        }
    private:
        /**
         * Refers to the two panes, used as index into @ref panes.
         */
        enum side {
            left  = 0,
            right = 1,
            NUM_PANES = 2
        };
        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        // swap the panes and windows via std::swap()
        void swap_panes();

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width  = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();
        char get_minimap_sym( side p ) const;

        bool inCategoryMode;

        int itemsPerPage;
        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;
        /**
         * Which panels is active (item moved from there).
         */
        side src;
        /**
         * Which panel is the destination (items want to go to there).
         */
        side dest;
        /**
         * True if (and only if) the filter of the active panel is currently
         * being edited.
         */
        bool filter_edit;
        /**
         * Two panels (left and right) showing the items, use a value of @ref side
         * as index.
         */
        std::array<advanced_inventory_pane, NUM_PANES> panes;
        static const advanced_inventory_pane null_pane;
        std::array<advanced_inv_area, NUM_AIM_LOCATIONS> squares;

        catacurses::window head;
        catacurses::window left_window;
        catacurses::window right_window;

        bool exit;

        // store/load settings (such as index, filter, etc)
        void save_settings( bool only_panes );
        void load_settings();
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        // returns true if currently processing a routine
        // (such as `MOVE_ALL_ITEMS' with `AIM_ALL' source)
        bool is_processing() const;

        static std::string get_sortname( advanced_inv_sortby sortby );
        bool move_all_items( bool nested_call = false );
        void print_items( advanced_inventory_pane &pane, bool active );
        void recalc_pane( side p );
        void redraw_pane( side p );
        // Returns the x coordinate where the header started. The header is
        // displayed right of it, everything left of it is till free.
        int print_header( advanced_inventory_pane &pane, aim_location sel );
        void init();
        /**
         * Translate an action ident from the input context to an aim_location.
         * @param action Action ident to translate
         * @param ret If the action ident referred to a location, its id is stored
         * here. Only valid when the function returns true.
         * @return true if the action did refer to an location (which has been
         * stored in ret), false otherwise.
         */
        static bool get_square( const std::string &action, aim_location &ret );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu( advanced_inventory_pane &pane );
        /**
         * Checks whether one can put items into the supplied location.
         * If the supplied location is AIM_ALL, query for the actual location
         * (stores the result in def) and check that destination.
         * @return false if one can not put items in the destination, true otherwise.
         * The result true also indicates the def is not AIM_ALL (because the
         * actual location has been queried).
         */
        bool query_destination( aim_location &def );
        /**
         * Move content of source container into destination container (destination pane = AIM_CONTAINER)
         * @param src_container Source container
         * @param dest_container Destination container
         */
        bool move_content( item &src_container, item &dest_container );
        /**
         * Setup how many items/charges (if counted by charges) should be moved.
         * @param destarea Where to move to. This must not be AIM_ALL.
         * @param sitem The source item, it must contain a valid reference to an item!
         * @param action The action we are querying
         * @param amount The input value is ignored, contains the amount that should
         *      be moved. Only valid if this returns true.
         * @return false if nothing should/can be moved. True only if there can and
         *      should be moved. A return value of true indicates that amount now contains
         *      a valid item count to be moved.
         */
        bool query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                            const std::string &action, int &amount );

        static char get_direction_key( aim_location area );
};

#endif
