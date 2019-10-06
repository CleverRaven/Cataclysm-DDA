#pragma once
#ifndef ADVANCED_INV_H
#define ADVANCED_INV_H
#include "cursesdef.h"
#include "advanced_inv_pane.h"

#include <array>
#include <string>
#include <vector>

class uilist;
class vehicle;
class item;


void comestible_inv();
// see item_factory.h
class item_category;

class advanced_inv_base
{
    public:
        advanced_inv_base();
        ~advanced_inv_base();

        virtual void display();
        virtual void init();
    protected:
        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        //set by children
        advanced_inv_save_state *save_state;
        bool show_help;
        bool reset_on_exit;

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width = 3;
        const int minimap_height = 3;

        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;

        std::unique_ptr< advanced_inv_pane> pane;
        std::array<advanced_inv_area, advanced_inv_area_info::NUM_AIM_LOCATIONS> squares;

        catacurses::window head;

        // if true it will close the menu
        bool exit;
        // used to determine if we need to return back after next activity is done
        bool is_full_exit();
        /*
        use save_settings() anywhere in the code
        use save_settings_internal() in the destructor
        need both so that children can save settings correctly
        */
        virtual void save_settings();

        virtual void init_pane();
        virtual input_context register_actions();
        virtual std::string process_actions( input_context &ctxt );

        virtual void redraw_pane();
        virtual void redraw_minimap();
        char minimap_get_sym( advanced_inv_pane *pane ) const;
        //sets recalculate and redraw for this and pane
        virtual void refresh( bool needs_recalc, bool needs_redraw );
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        // any additional info, relevant for menu, but not part of main data
        virtual void set_additional_info( std::vector<legend_data> data = {} );
        /**
         * Translate an action ident from the input context to an advanced_inv_area.
         * return nullptr if action doesn't correspond to any area
         * sets error if area is invalid
         */
        advanced_inv_area *get_area( const std::string &action, std::string &error );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu();
        virtual advanced_inv_pane *get_pane() const {
            return pane.get();
        }
    private:
        //need this because virtual/override doesn't work with destructors
        void save_settings_internal();
};

#endif
