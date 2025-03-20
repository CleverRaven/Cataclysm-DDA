#pragma once
#ifndef CATA_SRC_ADVANCED_INV_PANE_H
#define CATA_SRC_ADVANCED_INV_PANE_H

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "advanced_inv_area.h"
#include "advanced_inv_listitem.h"
#include "cursesdef.h"
#include "item_location.h"
#include "units_fwd.h"

class item;
struct advanced_inv_pane_save_state;

enum advanced_inv_sortby {
    SORTBY_NONE,
    SORTBY_NAME,
    SORTBY_WEIGHT,
    SORTBY_VOLUME,
    SORTBY_DENSITY,
    SORTBY_CHARGES,
    SORTBY_CATEGORY,
    SORTBY_DAMAGE,
    SORTBY_AMMO,
    SORTBY_SPOILAGE,
    SORTBY_PRICE,
    SORTBY_PRICEPERVOLUME,
    SORTBY_PRICEPERWEIGHT,
    SORTBY_STACKS
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
        void set_area( const advanced_inv_area &square, bool in_vehicle_cargo = false );
        void restore_area();
        aim_location get_area() const {
            return area;
        }
        bool prev_in_vehicle() const {
            return prev_viewing_cargo;
        }
        bool in_vehicle() const {
            return viewing_cargo;
        }
        advanced_inv_pane_save_state *save_state;
        void save_settings() const;
        void load_settings( int saved_area_idx,
                            const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares, bool is_re_enter );
        /**
         * Index of the selected item (index of @ref items),
         */
        int index = 0;
        advanced_inv_sortby sortby = advanced_inv_sortby::SORTBY_NONE;
        catacurses::window window;
        std::vector<advanced_inv_listitem> items;
        /**
         * Whether to recalculate the content of this pane.
         */
        bool recalc = false;
        item_location target_item_after_recalc;

        /**
        * The active container item in container view.
        */
        item_location container;
        /**
        * The original location from which container view was entered.
        */
        aim_location container_base_loc = NUM_AIM_LOCATIONS;
        /**
        * The line number of the other pane's container, if it's inside this pane's aim location.
        */
        int other_cont = -1;

        void add_items_from_area( advanced_inv_area &square, bool vehicle_override = false );
        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        /**
         * @param it The item to check, only the name member is examined.
         * @return Whether the item should be filtered (and not shown).
         */
        bool is_filtered( const advanced_inv_listitem &it ) const;
        /**
         * Same as the other, but checks the real item.
         */
        bool is_filtered( const item &it ) const;
        /**
         * Scroll @ref index, by given offset,
         * @param offset Must not be 0.
         */
        void scroll_by( int offset );
        /**
         * Scroll @ref index, by given offset in pages,
         * @param linesPerPage Amount of lines that can be displayed per page.
         * @param offset Must be either +1 or -1.
         */
        void scroll_page( int linesPerPage, int offset );
        /**
         * Scroll @ref index, in category mode by given offset.
         * @param offset Must be either +1 or -1
         */
        void scroll_category( int offset );
        void scroll_to_start();
        void scroll_to_end();
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        advanced_inv_listitem *get_cur_item_ptr();
        /**
         * @return free volume capacity of the pane's container or area
         */
        units::volume free_volume( const advanced_inv_area &square ) const;
        /**
         * @return free weight capacity of the pane's container or area
         */
        units::mass free_weight_capacity() const;
        /**
         * Set the filter string and update filter_function.
         */
        void set_filter( const std::string &new_filter );
        std::string get_filter() const {
            return filter;
        };
    private:
        /**
         * The current filter string. And function representing that filter.
         */
        std::string filter;
        std::function<bool( const item & )> filter_function;
        /** Only add offset to index, but wrap around! */
        void mod_index( int offset );
};
#endif // CATA_SRC_ADVANCED_INV_PANE_H
