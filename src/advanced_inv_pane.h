#pragma once
#ifndef ADVANCED_INV_PANE_H
#define ADVANCED_INV_PANE_H
#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "advanced_inv_area.h"
#include "advanced_inv_listitem.h"
#include "color.h"
#include "uistate.h"

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

class advanced_inv_pane
{
    protected:
        advanced_inv_pane_save_state &save_state;
        // where items come from. Should never be null
        advanced_inv_area *cur_area;
        bool viewing_cargo = false;
        bool is_compact;

        units::volume volume;
        units::mass weight;
        //supplied by parent to say which columns need to be displayed
        std::vector<advanced_inv_columns> columns;

        std::vector<advanced_inv_listitem *> items;

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
        bool is_filtered( const advanced_inv_listitem &it ) const;
        bool is_filtered( const item &it ) const;

        int print_header( advanced_inv_area &sel_area );
        /**
         * Insert additional category headers on the top of each page.
         */
        void paginate();

        virtual void add_items_from_area( advanced_inv_area &area, bool from_cargo,
                                          units::volume &ret_volume,
                                          units::mass &ret_weight );
        virtual advanced_inv_listitem *create_listitem( std::list<item *> list, int index,
                advanced_inv_area &area, bool from_vehicle ) = 0;
        virtual advanced_inv_listitem *create_listitem( const item_category *cat = nullptr ) = 0;

        void print_items();
        virtual advanced_inv_select_state item_selected_state( bool selected );

        /** Scroll to next non-header entry */
        void skip_category_headers( int offset );
        /** Only add offset to index, but wrap around! */
        void mod_index( int offset );

        void clear_items();

        mutable std::map<std::string, std::function<bool( const item & )>> filtercache;

    public:
        /**
         * Index of the selected item (index of @ref items),
         */
        int index;
        catacurses::window window;
        //this filter is supplied by parent, applied before user input filtering
        std::function<bool( const item & )> special_filter;
        std::string title;
        //if child class wants to display any extra data at the header
        std::vector<legend_data> additional_info;

        int itemsPerPage;

        bool inCategoryMode;

        advanced_inv_columns sortby;
        // secondary sort, in case sortby values are equal
        advanced_inv_columns default_sortby;

        bool needs_recalc; //implies redraw
        bool needs_redraw;

        advanced_inv_pane(
            std::array<advanced_inv_area, advanced_inv_area_info::NUM_AIM_LOCATIONS> &areas,
            advanced_inv_pane_save_state &save_state ) : save_state( save_state ), all_areas(
                    areas ) {};
        virtual ~advanced_inv_pane();

        virtual void init( int items_per_page, catacurses::window w );
        void save_settings( bool reset );

        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        void recalc();
        virtual void redraw();

        /**
         * Scroll @ref index, by given offset, set redraw to true,
         * @param offset Must not be 0.
         */
        void scroll_by( int offset );
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        advanced_inv_listitem *get_cur_item_ptr();

        void set_filter( const std::string &new_filter );
        void start_user_filtering( int h, int w );

        // adds entries to uilist when user tries to sort
        void add_sort_entries( uilist &sm );

        std::array<advanced_inv_area, advanced_inv_area_info::NUM_AIM_LOCATIONS> &all_areas;
        advanced_inv_area &get_square( advanced_inv_area_info::aim_location loc ) {
            return  all_areas[loc];
        }
        advanced_inv_area &get_square( size_t loc ) {
            return all_areas[loc];
        }
        virtual void set_area( advanced_inv_area &square, bool show_vehicle ) {
            cur_area = &square;
            viewing_cargo = cur_area->has_vehicle() && show_vehicle;
            index = 0;
        }
        advanced_inv_area &get_area() const {
            return *cur_area;
        }
        bool is_in_vehicle() const {
            return viewing_cargo;
        }
};
#endif
