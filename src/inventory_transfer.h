#pragma once
#ifndef INVENTORY_TRANSFER_H
#define INVENTORY_TRANSFER_H

#include "advanced_inv_base.h"
#include "advanced_inv_pane.h"
#include "advanced_inv_listitem.h"
#include "advanced_inv_area.h"

class inventory_transfer : public advanced_inv_base
{
    public:
        /*
        Retrieves data from iteratror and call move_all_items()
        Iterator is set by setup_move_all()
        */
        void move_all_items_iteration();
        inventory_transfer();
        ~inventory_transfer();
    protected:
        std::unique_ptr< advanced_inv_pane> pane_right;
        advanced_inv_pane *cur_pane;

        void init_pane() override;
        void save_settings() override;
        input_context register_actions() override;
        std::string process_actions( input_context &ctxt ) override;
        void redraw_pane() override;
        void redraw_minimap() override;
        void refresh( bool needs_recalc, bool needs_redraw ) override;
        advanced_inv_pane *get_pane() const override {
            return cur_pane;
        }

        advanced_inv_area *query_destination( bool &to_vehicle );
        int query_charges( const advanced_inv_area &darea, bool to_vehicle, const std::string &action );
        void move_item( advanced_inv_area &move_to, bool use_cargo, int amount_to_move );

        /*
        To move items we have to exit/re-enter multiple times
        This functions sets up iterator that keeps track of items we moved so far.
        Iterator is used by move_all_items_iteration()
        */
        bool setup_move_all( advanced_inv_area &move_to, bool to_vehicle );
        bool move_all_items( advanced_inv_area &move_from, bool from_vehicle, advanced_inv_area &move_to,
                             bool to_vehicle, std::function<bool( const item & )> filter );

        void set_active_pane( advanced_inv_pane *p );
        advanced_inv_pane *get_other_pane();
    private:
        //need this because virtual/override doesn't work with destructors
        void save_settings_internal();
};

class inventory_transfer_pane : public advanced_inv_pane
{
    public:
        using advanced_inv_pane::advanced_inv_pane;
        using advanced_inv_pane::init;
        void init( int items_per_page, catacurses::window w, advanced_inv_pane *other_pane );
        //void save_settings(advanced_inv_pane_save_state& save_state, bool reset) override;
        advanced_inv_listitem *create_listitem( std::list<item *> list, int index,
                                                advanced_inv_area &area, bool from_vehicle ) override;
        advanced_inv_listitem *create_listitem( const item_category *cat = nullptr ) override;
        void set_area( advanced_inv_area &square, bool show_vehicle ) override;
        void add_items_from_area( advanced_inv_area &area, bool from_cargo, units::volume &ret_volume,
                                  units::mass &ret_weight ) override;
        // same as base, but recolored to account for is_active
        void redraw() override;
        advanced_inv_select_state item_selected_state( bool selected ) override;

        void set_active( bool active ) {
            is_active = active;
        }
        bool can_transef_all() {
            return volume <= other->get_area().free_volume( other->is_in_vehicle() );
        }
        advanced_inv_pane *get_other_pane() {
            return other;
        }
        bool has_items() {
            return !items.empty();
        }
        // used to perform item moving iteration by comestible_inventory_bio::move_all_items_iteration()
        std::function<bool( const item & )> get_filter_func();
    protected:
        advanced_inv_pane *other;
        bool is_active;
};

class inventory_transfer_listitem : public advanced_inv_listitem
{
    public:
        inventory_transfer_listitem( const item_category *cat = nullptr );
        inventory_transfer_listitem( const std::list<item *> &list, int index,
                                     advanced_inv_area &area, bool from_vehicle );
    protected:
        void set_print_color( nc_color &retval, nc_color default_col ) override;
        std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2, bool &retval )>
        compare_function( advanced_inv_columns sortby ) override;
    private:
        void init( const item &it );
};

// Render a fancy ASCII grid at the left of the menu.
class query_destination_callback : public uilist_callback
{
    private:
        std::array<advanced_inv_area, advanced_inv_area_info::NUM_AIM_LOCATIONS> &_squares;
        void draw_squares( const uilist *menu );
    public:
        query_destination_callback(
            std::array<advanced_inv_area, advanced_inv_area_info::NUM_AIM_LOCATIONS> &squares ) : _squares(
                    squares ) {}
        void select( int /*entnum*/, uilist *menu ) override {
            draw_squares( menu );
        }
};

#endif
