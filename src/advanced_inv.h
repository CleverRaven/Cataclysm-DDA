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
#include "advanced_inv_listitem.h"
#include "advanced_inv_pane.h"

class uilist;
class vehicle;
class item;

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
        std::string get_location_key( aim_location area );

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
