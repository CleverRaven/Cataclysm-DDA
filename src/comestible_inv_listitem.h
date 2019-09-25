#pragma once
#ifndef COMESTIBLE_INV_LISTITEM_H
#define COMESTIBLE_INV_LISTITEM_H
#include "cursesdef.h"
#include "units.h"
#include "comestible_inv_area.h"
#include "color.h"
#include "player.h"

#include <array>
#include <functional>
#include <list>
#include <string>
#include <vector>
#include <utility>

class item_category;

//TODO: when changing this, also change comestible_inv_pane.cpp - get_col_data(). Make sure order is preserved.
enum comestible_inv_columns {
    COLUMN_NAME,

    COLUMN_SRC,
    COLUMN_AMOUNT,
    COLUMN_WEIGHT,
    COLUMN_VOLUME,

    COLUMN_CALORIES,
    COLUMN_QUENCH,
    COLUMN_JOY,
    COLUMN_EXPIRES,
    COLUMN_SHELF_LIFE,

    COLUMN_ENERGY,

    COLUMN_SORTBY_CATEGORY, //not actual clumn
    COLUMN_NUM_ENTRIES
};

enum comestible_select_state {
    SELECTSTATE_NONE,
    SELECTSTATE_SELECTED,
    SELECTSTATE_CATEGORY
};
struct col_data {

    public:
        comestible_inv_columns id;
    private:
        std::string col_name;
    public:
        int width;
        char hotkey;
    private:
        std::string sort_name;

    public:
        std::string get_col_name() const {
            return _( col_name );
        }
        std::string get_sort_name() const {
            return _( sort_name );
        }

        col_data( comestible_inv_columns id, std::string col_name, int width, char hotkey,
                  std::string sort_name ) :
            id( id ),
            col_name( col_name ), width( width ), hotkey( hotkey ), sort_name( sort_name ) {

        }
};

class comestible_inv_listitem
{
    public:
    // *INDENT-OFF*
    static col_data get_col_data(comestible_inv_columns col) {
        static std::array<col_data, COLUMN_NUM_ENTRIES> col_data = { {
            {COLUMN_NAME,       translate_marker("Name (charges)"),0, 'n', translate_marker("name")},

            {COLUMN_SRC,        translate_marker("src"),         4, ';', ""},
            {COLUMN_AMOUNT,     translate_marker("amt"),         5, ';', ""},

            {COLUMN_WEIGHT,     translate_marker("weight"),      7, 'w', translate_marker("weight")},
            {COLUMN_VOLUME,     translate_marker("volume"),      7, 'v', translate_marker("volume")},

            {COLUMN_CALORIES,   translate_marker("calories"),    9, 'c', translate_marker("calories")},
            {COLUMN_QUENCH,     translate_marker("quench"),      7, 'q', translate_marker("quench")},
            {COLUMN_JOY,        translate_marker("joy"),         4, 'j', translate_marker("joy")},
            {COLUMN_EXPIRES,    translate_marker("expires in"),  11,'s', translate_marker("spoilage")},
            {COLUMN_SHELF_LIFE, translate_marker("shelf life"),  11,'s', translate_marker("shelf life")},

            {COLUMN_ENERGY,     translate_marker("energy"),      11,'e', translate_marker("energy")},

            {COLUMN_SORTBY_CATEGORY,translate_marker("fake"),    0,'c', translate_marker("category")}
        } };
        return col_data[col];
    }
    // *INDENT-ON*
    public:
        /**
         * Index of the item in the itemstack.
         */
        int idx;
    private:
        /**
         * The location of the item, should nevere be AREA_TYPE_MULTI
         */
        comestible_inv_area &area;
    public:

        using itype_id = std::string;
        itype_id id;
        // The list of items, and empty when a header
        std::list<item *> items;

        std::string name;
        // Name of the item (singular) without damage (or similar) prefix, used for sorting.
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

        // from all the items in a stack
        units::volume volume;
        units::mass weight;

        comestible_select_state is_selected;
        nc_color menu_color;
        nc_color menu_color_dark;

        virtual void print_columns( std::vector<comestible_inv_columns> columns,
                                    comestible_select_state selected_state, catacurses::window window, int right_bound,
                                    int cur_print_y );
    protected:
        virtual bool print_string_column( comestible_inv_columns col, std::string &print_str,
                                          nc_color &print_col );
        virtual bool print_int_column( comestible_inv_columns col, std::string &print_str,
                                       nc_color &print_col );
        virtual bool print_default_columns( comestible_inv_columns col, std::string &print_str,
                                            nc_color &print_col );
        virtual void print_name( catacurses::window window, int right_bound, int cur_print_y );
        virtual std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
        compare_function( comestible_inv_columns sortby );

        void set_print_color( nc_color &retval, nc_color default_col );

    public:
        std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2 )>
        get_sort_function( comestible_inv_columns sortby, comestible_inv_columns default_sortby );

        char const *set_string_params( nc_color &print_color, int value, bool need_highlight = false );
        /**
        * The item category, or the category header.
        */
        const item_category *cat;

        bool from_vehicle;

        const islot_comestible &get_edible_comestible( player &p, const item &it ) const;

        std::string get_time_left_rounded( player &p, const item &it ) const;
        /**
         * Whether this is a category header entry, which does *not* have a reference
         * to an item, only @ref cat is valid.
         */
        bool is_category_header() const;
        bool is_item_entry() const;

        comestible_inv_area &get_area() {
            return area;
        }

        comestible_inv_listitem( const item_category *cat = nullptr );
        /**
         * Create a normal item entry.
         * @param list The list of item pointers.
         * @param index The index
         * @param area The source area. Must not be AIM_ALL.
         * @param from_vehicle Is the item from a vehicle cargo space?
         */
        comestible_inv_listitem( const std::list<item *> &list, int index,
                                 comestible_inv_area &area, bool from_vehicle );

        virtual ~comestible_inv_listitem() = default;

    private:
        void init( const item &it );
};

class comestible_inv_listitem_food : public comestible_inv_listitem
{
    public:
        comestible_inv_listitem_food( const item_category *cat = nullptr );
        comestible_inv_listitem_food( const std::list<item *> &list, int index,
                                      comestible_inv_area &area, bool from_vehicle );
        void print_columns( std::vector<comestible_inv_columns> columns,
                            comestible_select_state selected_state, catacurses::window window, int right_bound,
                            int cur_print_y ) override;
    private:
        std::string shelf_life;
        std::string exipres_in;
        int calories;
        int quench;
        int joy;
        // item conditions, shown before name. Just for display.
        int cond_size = 4;
        std::vector<std::pair<char, nc_color >> cond;
        ret_val<edible_rating> eat_error = ret_val<edible_rating>::make_success();
    protected:
        void init( const item &it );
        bool print_string_column( comestible_inv_columns col, std::string &print_str,
                                  nc_color &print_col ) override;
        bool print_int_column( comestible_inv_columns col, std::string &print_str,
                               nc_color &print_col ) override;
        void print_name( catacurses::window window, int right_bound, int cur_print_y ) override;
        std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
        compare_function( comestible_inv_columns sortby ) override;
};

class comestible_inv_listitem_bio: public comestible_inv_listitem
{
    public:
        comestible_inv_listitem_bio( const item_category *cat = nullptr );
        comestible_inv_listitem_bio( const std::list<item *> &list, int index,
                                     comestible_inv_area &area, bool from_vehicle );
    private:
        int energy;
    protected:
        bool print_int_column( comestible_inv_columns col, std::string &print_str,
                               nc_color &print_col ) override;
        std::function<bool( const comestible_inv_listitem *d1, const comestible_inv_listitem *d2, bool &retval )>
        compare_function( comestible_inv_columns sortby ) override;
};

#endif
