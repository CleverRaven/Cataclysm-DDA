#pragma once
#ifndef comestible_inv_H
#define comestible_inv_H
#include "cursesdef.h"
#include "comestible_inv_pane.h"

#include <array>
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

        virtual void display();
    protected:
        virtual input_context register_actions();
        virtual std::string process_actions( input_context &ctxt );

        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width = 3;
        const int minimap_height = 3;
        void redraw_minimap();
        char minimap_get_sym() const;

        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;

        //comestible_inventory_pane *pane;
        std::unique_ptr< comestible_inventory_pane> pane;
        //static const comestible_inventory_pane null_pane;
        std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS> squares;

        catacurses::window head;
        catacurses::window window;

        bool exit;

        virtual void init();
        //sets recalculate and redraw for this and pane
        void refresh( bool needs_recalc, bool needs_redraw );
        void save_settings();
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        // any additional info, relevant for menu, but not part of main data
        virtual void set_additional_info( std::vector<legend_data> data = {} );
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
};

class comestible_inventory_food : public comestible_inventory
{
    public:
    protected:
        void init() override;
        input_context register_actions() override;
        std::string process_actions( input_context &ctxt ) override;
        void set_additional_info( std::vector<legend_data> data ) override;
    private:
        //try to find a way to warm up/defrost an item, and do it
        void heat_up( item *it );
};

class comestible_inventory_bio : public comestible_inventory
{
    public:
    protected:
        void init() override;
        void set_additional_info( std::vector<legend_data> data ) override;
};

#endif
