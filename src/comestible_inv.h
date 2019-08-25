#pragma once
#ifndef comestible_inv_H
#define comestible_inv_H
#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "game.h"
#include "itype.h"
#include "comestible_inv_pane.h"

#include <cctype>
#include <cstddef>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

class uilist;
class vehicle;
class item;


void comestible_inv( int bio = -1 );
// see item_factory.h
class item_category;

class comestible_inventory
{
    public:
        comestible_inventory();
        ~comestible_inventory();

        void display( int bio );
    private:
        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();
        char minimap_get_sym() const;

        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;

        comestible_inventory_pane pane;
        //static const comestible_inventory_pane null_pane;
        std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS> squares;

        catacurses::window head;
        catacurses::window window;

        bool exit;

        // store/load settings (such as index, filter, etc)
        void save_settings();
        //void load_settings();
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        void redo( bool needs_recalc, bool needs_redraw );
        // returns true if currently processing a routine
        // (such as `MOVE_ALL_ITEMS' with `AIM_ALL' source)
        //bool is_processing() const;

        //static std::string get_sortname( comestible_inv_sortby sortby );
        //bool move_all_items(bool nested_call = false);
        //void print_items( comestible_inventory_pane &pane );
        //void recalc_pane();
        //void redraw_pane();
        // Returns the x coordinate where the header started. The header is
        // displayed right of it, everything left of it is till free.
        //int print_header( comestible_inventory_pane &pane, aim_location sel );
        void init();
        void set_pane_legend();
        /**
         * Translate an action ident from the input context to an aim_location.
         * @param action Action ident to translate
         * @param ret If the action ident referred to a location, its id is stored
         * here. Only valid when the function returns true.
         * @return true if the action did refer to an location (which has been
         * stored in ret), false otherwise.
         */
        comestible_inv_area *get_square( const std::string &action );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu();
        /**
         * Checks whether one can put items into the supplied location.
         * If the supplied location is AIM_ALL, query for the actual location
         * (stores the result in def) and check that destination.
         * @return false if one can not put items in the destination, true otherwise.
         * The result true also indicates the def is not AIM_ALL (because the
         * actual location has been queried).
         */
        //bool query_destination(aim_location& def);
        /**
         * Move content of source container into destination container (destination pane = AIM_CONTAINER)
         * @param src_container Source container
         * @param dest_container Destination container
         */
        //bool move_content( item &src_container, item &dest_container );
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
        /*bool query_charges(aim_location destarea, const comestible_inv_listitem& sitem,
            const std::string& action, int& amount);*/

        //void menu_square( uilist &menu );

        //std::string get_location_key( aim_location area );
        //char get_direction_key( aim_location area );

        /**
         * Converts from screen relative location to game-space relative location
         * for control rotation in isometric mode.
        */
        //static aim_location screen_relative_location( aim_location area );

        //char const *set_string_params( nc_color &print_color, int value, bool selected,
        //                               bool need_highlight = false );
        //nc_color set_string_params(int value, bool need_highlight = false);
        //time_duration get_time_left( player &p, const item &it ) const;
        //const islot_comestible &get_edible_comestible( player &p, const item &it ) const;
        //std::string get_time_left_rounded( player &p, const item &it ) const;
        //std::string time_to_comestible_str( const time_duration &d ) const;

        void heat_up( item *it );
};

#endif
