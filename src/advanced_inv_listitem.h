#pragma once
#ifndef ADVANCED_INV_LISTITEM_H
#define ADVANCED_INV_LISTITEM_H
#include "cursesdef.h"
#include "units.h"
#include "advanced_inv_area.h"
#include "color.h"
#include "player.h"

#include <array>
#include <functional>
#include <list>
#include <string>
#include <vector>
#include <utility>

class item_category;

//TODO: when changing this, also change get_col_data(). Make sure order is preserved.
enum advanced_inv_columns {
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

    //not actual clumns, used for sorting
    COLUMN_SORTBY_NONE,
    COLUMN_SORTBY_CATEGORY,
    COLUMN_SORTBY_AMMO,
    COLUMN_SORTBY_AMMO_TYPE,
    COLUMN_SORTBY_EXPIRES, // same as expires, but doesn't add column
    COLUMN_NUM_ENTRIES
};

enum advanced_inv_select_state {
    SELECTSTATE_NONE,
    SELECTSTATE_SELECTED,
    SELECTSTATE_CATEGORY,
    SELECTSTATE_INACTIVE
};
struct col_data {

    public:
        advanced_inv_columns id;
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

        col_data( advanced_inv_columns id, std::string col_name, int width, char hotkey,
                  std::string sort_name ) :
            id( id ),
            col_name( col_name ), width( width ), hotkey( hotkey ), sort_name( sort_name ) {

        }
};

class advanced_inv_listitem
{
    public:
    // *INDENT-OFF*
    static col_data get_col_data(advanced_inv_columns col) {
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

            {COLUMN_SORTBY_NONE,     "fake",    0,'u', translate_marker("Unsorted (recently added first)")},
            {COLUMN_SORTBY_CATEGORY, "fake",    0,'c', translate_marker("category")},
            {COLUMN_SORTBY_AMMO,     "fake",    0,'x', translate_marker("charges")},
            {COLUMN_SORTBY_AMMO_TYPE,"fake",    0,'a', translate_marker("ammo/charge type")},
            {COLUMN_SORTBY_EXPIRES,  "fake",    0,'s', translate_marker("spoilage")}
        } };
        return col_data[col];
    }
    // *INDENT-ON*
    public:
        /**
         * Index of the item in the itemstack.
         */
        int idx;
        /**
         * The stack count represented by this item, should be >= 1, should be 1
         * for anything counted by charges.
         */
        int stacks;

        // from all the items in a stack
        units::volume volume;
        units::mass weight;

        // The list of items, and empty when a header
        std::list<item *> items;

        std::string name;
        // Name of the item (singular) without damage (or similar) prefix, used for sorting.
        std::string name_without_prefix;

        // Whether auto pickup is enabled for this item (based on the name).
        bool autopickup;

        //The item category, or the category header.
        const item_category *cat;

        bool from_vehicle;
        /**
         * Create a normal item entry.
         * @param list The list of item pointers.
         * @param index The index
         * @param area The source area. Must not be AIM_ALL.
         * @param from_vehicle Is the item from a vehicle cargo space?
         */
        advanced_inv_listitem( const std::list<item *> &list, int index,
                               advanced_inv_area &area, bool from_vehicle );
        advanced_inv_listitem( const item_category *cat = nullptr );
        virtual ~advanced_inv_listitem() = default;

        /**
         * Whether this is a category header entry, which does *not* have a reference
         * to an item, only @ref cat is valid.
         */
        bool is_category_header() const;
        bool is_item_entry() const;

        advanced_inv_area &get_area() {
            return area;
        }

        virtual void print_columns( std::vector<advanced_inv_columns> columns,
                                    advanced_inv_select_state selected_state, catacurses::window window, int right_bound,
                                    int cur_print_y );
        /*
        The returned function is what actually used by std::sort
        The returned function compares d1 and d2 using sortby,
        If the primary compare returns equality, it compares d1 and d2 using default_sortby
        */
        std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2 )>
        get_sort_function( advanced_inv_columns sortby, advanced_inv_columns default_sortby );
    protected:
        using itype_id = std::string;
        itype_id id;

        // The location of the item, should nevere be AREA_TYPE_MULTI
        advanced_inv_area &area;
        //show any symbol to the left of item name
        int cond_size;
        std::vector<std::pair<char, nc_color >> cond;

        advanced_inv_select_state is_selected;
        nc_color menu_color;
        nc_color menu_color_dark;

        virtual bool print_string_column( advanced_inv_columns col, std::string &print_str,
                                          nc_color &print_col );
        virtual bool print_int_column( advanced_inv_columns col, std::string &print_str,
                                       nc_color &print_col );
        virtual bool print_default_columns( advanced_inv_columns col, std::string &print_str,
                                            nc_color &print_col );
        virtual void print_name( catacurses::window window, int right_bound, int cur_print_y );

        virtual void set_print_color( nc_color &retval, nc_color default_col );

        /*
        The returned value is a function that:
        - takes 2 listitems that need to be compared
        - returns whether 2 items are equal
        - sets retval to d1.param < d2.param (or >)

        We need to know when d1.param == d2.param, to trigger secondary sorting; so we explicitely check for it.
        When d1.param != d2.param, retval is used.
        See get_sort_function for details.
        */
        virtual std::function<bool( const advanced_inv_listitem *d1, const advanced_inv_listitem *d2, bool &retval )>
        compare_function( advanced_inv_columns sortby );

        struct sort_case_insensitive_less : public std::binary_function< char, char, bool > {
            bool operator()( char x, char y ) const {
                return toupper( static_cast<unsigned char>( x ) ) < toupper( static_cast<unsigned char>( y ) );
            }
        };
    private:
        void init( const item &it ); //called from constructor, so virtual/override doesn't work
};
#endif
