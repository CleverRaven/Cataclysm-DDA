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

        void init();
        //sets recalculate and redraw for this and pane
        void refresh( bool needs_recalc, bool needs_redraw );
        void save_settings();
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        // any additional info, relevant for menu, but not part of main data
        void set_additional_info();
        /**
         * Translate an action ident from the input context to an comestible_inv_area.
         * @param action one of ctxt.register_action
         */
        comestible_inv_area *get_area( const std::string &action );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu();

        //try to find a way to warm up/defrost an item, and do it
        void heat_up( item *it );
};

#endif
