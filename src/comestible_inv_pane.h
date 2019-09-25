#pragma once
#ifndef COMESTIBLE_INV_PANE_H
#define COMESTIBLE_INV_PANE_H
#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "comestible_inv_area.h"
#include "comestible_inv_listitem.h"
#include "color.h"

#include <array>
#include <functional>
#include <list>
#include <string>
#include <vector>
#include <utility>

struct legend_data {
    point position;
    std::string message;
    nc_color color;
    legend_data( std::string message, point position, nc_color color ): message( message ),
        color( color ) {
        this->position = point( 25, 1 ) + position;
    }
};

class comestible_inventory_pane
{
    protected:
        // where items come from. Should never be null
        comestible_inv_area *cur_area;
        bool viewing_cargo = false;
        bool is_compact;

        units::volume volume;
        units::mass weight;
        //supplied by parent to say which columns need to be displayed
        std::vector<comestible_inv_columns> columns;

        std::vector<comestible_inv_listitem *> items;

        //true when user is editing item filter
        bool filter_edit;
        /**
         * The current filter string.
         */
        std::string filter;
        /**
         * @param it The item to check, oly the name member is examined.
         * @return Whether the item should be filtered (and not shown).
         */
        bool is_filtered( const comestible_inv_listitem &it ) const;
        bool is_filtered( const item &it ) const;

        int print_header( comestible_inv_area &sel_area );
        /**
         * Insert additional category headers on the top of each page.
         */
        void paginate();

        void add_items_from_area( comestible_inv_area &area, bool from_cargo, units::volume &ret_volume,
                                  units::mass &ret_weight );
        virtual comestible_inv_listitem *create_listitem( std::list<item *> list, int index,
                comestible_inv_area &area, bool from_vehicle ) = 0;
        virtual comestible_inv_listitem *create_listitem( const item_category *cat = nullptr ) = 0;

        void print_items();

        /** Scroll to next non-header entry */
        void skip_category_headers( int offset );
        /** Only add offset to index, but wrap around! */
        void mod_index( int offset );

        void clear_items();

        mutable std::map<std::string, std::function<bool( const item & )>> filtercache;

    public:

        comestible_inventory_pane(
            std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS> &areas ) : all_areas(
                    areas ) {};
        virtual ~comestible_inventory_pane();

        /**
         * Index of the selected item (index of @ref items),
         */
        int index;
        catacurses::window window;
        //this filter is supplied by parent, applied before user input filtering
        std::function<bool( const item & )> special_filter;
        //list of shortcut reminders (created manually by parent)
        std::string title;
        std::vector<legend_data> additional_info;

        int itemsPerPage;

        bool inCategoryMode;

        comestible_inv_columns sortby;
        // secondary sort, in case original returns == items
        comestible_inv_columns default_sortby;

        /**
         * Whether to recalculate the content of this pane.
         * Implies @ref redraw.
         */
        bool needs_recalc;
        bool needs_redraw;

        virtual void init( int items_per_page, catacurses::window w );
        void save_settings( bool reset );

        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        void recalc();
        void redraw();

        /**
         * Scroll @ref index, by given offset, set redraw to true,
         * @param offset Must not be 0.
         */
        void scroll_by( int offset );
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        comestible_inv_listitem *get_cur_item_ptr();
        /**
         * Set the filter string
         */
        void set_filter( const std::string &new_filter );
        void start_user_filtering( int h, int w );

        //adds things this can sort on. List is provided by parent
        void add_sort_entries( uilist &sm );

        std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS> &all_areas;
        comestible_inv_area &get_square( comestible_inv_area_info::aim_location loc ) {
            return  all_areas[loc];
        }
        comestible_inv_area &get_square( size_t loc ) {
            return all_areas[loc];
        }
        // set the pane's area via its square, and whether it is viewing a vehicle's cargo
        void set_area( comestible_inv_area &square, bool show_vehicle ) {
            cur_area = &square;
            viewing_cargo = cur_area->has_vehicle() && show_vehicle;
        }
        comestible_inv_area &get_area() const {
            return *cur_area;
        }
        bool is_in_vehicle() const {
            return viewing_cargo;
        }
};

class comestible_inventory_pane_food : public comestible_inventory_pane
{
        using comestible_inventory_pane::comestible_inventory_pane;
        void init( int items_per_page, catacurses::window w ) override;
        comestible_inv_listitem *create_listitem( std::list<item *> list, int index,
                comestible_inv_area &area, bool from_vehicle ) override;
        comestible_inv_listitem *create_listitem( const item_category *cat = nullptr ) override;
};
class comestible_inventory_pane_bio: public comestible_inventory_pane
{
        using comestible_inventory_pane::comestible_inventory_pane;
        void init( int items_per_page, catacurses::window w ) override;
        comestible_inv_listitem *create_listitem( std::list<item *> list, int index,
                comestible_inv_area &area, bool from_vehicle ) override;
        comestible_inv_listitem *create_listitem( const item_category *cat = nullptr ) override;
};
#endif
