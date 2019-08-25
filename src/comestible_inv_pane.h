#pragma once
#ifndef COMESTIBLE_INV_PANE_H
#define COMESTIBLE_INV_PANE_H
#include "cursesdef.h"
#include "point.h"
#include "units.h"
#include "game.h"
#include "ui.h"
#include "itype.h"
#include "comestible_inv_area.h"

#include <cctype>
#include <cstddef>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>


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

struct legend_data {
    char shortcut;
    nc_color color;
    std::string message;
    legend_data(char shortcut, nc_color color, std::string message):shortcut(shortcut), color(color), message(message) {

    }
};

/**
 * Entry that is displayed in a adv. inv. pane. It can either contain a
 * single item or a category header or nothing (empty entry).
 * Most members are used only for sorting.
 */
class comestible_inv_listitem {
public:
    using itype_id = std::string;
    /**
     * Index of the item in the itemstack.
     */
    int idx;
    /**
     * The location of the item, never AIM_ALL.
     */
    //comestible_inv_area_info::aim_location area; 
    comestible_inv_area* area;
    // the id of the item
    itype_id id;
    // The list of items, and empty when a header
    std::list<item*> items;
    /**
     * The displayed name of the item/the category header.
     */
    std::string name;
    /**
     * Name of the item (singular) without damage (or similar) prefix, used for sorting.
     */
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
    /**
     * The volume of all the items in this stack, used for sorting.
     */
    units::volume volume;
    /**
     * The weight of all the items in this stack, used for sorting.
     */
    units::mass weight;

    /**
     * TODO:docs
     */
    std::string shelf_life;
    std::string exipres_in;    
    int calories;
    int quench;
    int joy;
    bool is_mushy;
    int energy;

    comestible_select_state is_selected;
    nc_color menu_color;
    nc_color menu_color_dark;

    void print_columns(std::vector<comestible_inv_columns> columns, comestible_select_state selected_state, bool is_compact, catacurses::window window, int right_bound, int cur_print_y);
private:
    bool print_string_column(comestible_inv_columns col, catacurses::window window, int cur_print_x, int cur_print_y);
    bool print_int_column(comestible_inv_columns col, catacurses::window window, int cur_print_x, int cur_print_y);
    void print_name(bool is_compact, catacurses::window window, int right_bound, int cur_print_y);
    bool print_default_columns(comestible_inv_columns col, catacurses::window window, int cur_print_x, int cur_print_y);
    void set_print_color(nc_color& retval, nc_color default_col);
public:
    

    char const* set_string_params(nc_color& print_color, int value, bool need_highlight = false);

    /**
     * The item category, or the category header.
     */
    const item_category* cat;
    /**
     * Is the item stored in a vehicle?
     */
    bool from_vehicle;


    //TODO
    const islot_comestible& get_edible_comestible(player& p, const item& it) const;

    std::string get_time_left_rounded(player& p, const item& it) const;

    std::string time_to_comestible_str(const time_duration& d) const;
    /**
     * Whether this is a category header entry, which does *not* have a reference
     * to an item, only @ref cat is valid.
     */
    bool is_category_header() const;

    /** Returns true if this is an item entry */
    bool is_item_entry() const;
    /**
     * Create a category header entry.
     * @param cat The category reference, must not be null.
     */
    comestible_inv_listitem(const item_category* cat);

    /**
     * Creates an empty entry, both category and item pointer are null.
     */
    comestible_inv_listitem();
    /**
     * Create a normal item entry.
     * @param an_item The item pointer. Must not be null.
     * @param index The index
     * @param count The stack size
     * @param area The source area. Must not be AIM_ALL.
     * @param from_vehicle Is the item from a vehicle cargo space?
     */
    comestible_inv_listitem(item* an_item, int index, int count,
        comestible_inv_area* area, bool from_vehicle);
    /**
     * Create a normal item entry.
     * @param list The list of item pointers.
     * @param index The index
     * @param area The source area. Must not be AIM_ALL.
     * @param from_vehicle Is the item from a vehicle cargo space?
     */
    comestible_inv_listitem(const std::list<item*>& list, int index,
        comestible_inv_area* area, bool from_vehicle);
};

/**
 * Displayed pane, what is shown on the screen.
 */
class comestible_inventory_pane
{
private:
    comestible_inv_area* cur_area;
    // pointer to the square this pane is pointing to
    bool viewing_cargo = false;
    bool is_compact;
public:

    // set the pane's area via its square, and whether it is viewing a vehicle's cargo
    void set_area(comestible_inv_area* square, bool show_vehicle) {
        cur_area = square;
        viewing_cargo = square->has_vehicle() && show_vehicle;
    }
    comestible_inv_area* get_area() const {
        return cur_area;
    }
    bool is_in_vehicle() const {
        return viewing_cargo;
    }
    //bool on_ground() const {
    //    return area > AIM_INVENTORY && area < AIM_DRAGGED;
    //}
    /**
     * Index of the selected item (index of @ref items),
     */
    int index;
    comestible_inv_columns sortby;
    catacurses::window window;
    std::vector<comestible_inv_listitem> items;

    std::function<bool(const item&)> special_filter;

    std::vector<legend_data> legend;
    std::string title;

    bool inCategoryMode;

    units::volume volume;
    units::mass weight;
    bool filter_edit;

    /**
     * The current filter string.
     */
    std::string filter;
    /**
     * Food or Drug filter. True = show food.
     */
    //bool filter_show_food;
    /**
     * Whether to recalculate the content of this pane.
     * Implies @ref redraw.
     */
    bool needs_recalc;
    /**
     * Whether to redraw this pane.
     */
    bool needs_redraw;

    void add_items_from_area(comestible_inv_area* area, bool from_cargo, units::volume& ret_volume, units::mass& ret_weight);
    /**
     * Makes sure the @ref index is valid (if possible).
     */
    void fix_index();
    void recalc();
    void redraw();
    std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS>* squares;
    comestible_inv_area* get_square(comestible_inv_area_info::aim_location loc) {
        return  &((*squares)[loc]);
    }
    comestible_inv_area* get_square(size_t loc) {
        return &((*squares)[loc]);
    }
    int print_header(comestible_inv_area* sel_area);
    /**
     * @param it The item to check, oly the name member is examined.
     * @return Whether the item should be filtered (and not shown).
     */
    bool is_filtered(const comestible_inv_listitem& it) const;
    /**
     * Same as the other, but checks the real item.
     */
    bool is_filtered(const item& it) const;
    /**
     * Scroll @ref index, by given offset, set redraw to true,
     * @param offset Must not be 0.
     */
    void scroll_by(int offset);
    /**
     * @return either null, if @ref index is invalid, or the selected
     * item in @ref items.
     */
    comestible_inv_listitem* get_cur_item_ptr();
    /**
     * Set the filter string, disables filtering when the filter string is empty.
     */
    void set_filter(const std::string& new_filter);
    /**
     * Insert additional category headers on the top of each page.
     */
    void paginate();

    //TODO:docs'n'stuffs
    int itemsPerPage;
    std::vector<comestible_inv_columns> columns;
    comestible_inv_columns default_sortby;
   
    void add_sort_enries(uilist& sm);
    void init(std::vector<comestible_inv_columns> c, int items_per_page, catacurses::window w, std::array<comestible_inv_area, comestible_inv_area_info::NUM_AIM_LOCATIONS>* s);
    void save_settings(bool reset);
    void do_filter(int h, int w);
    void print_items();
private:
    /** Scroll to next non-header entry */
    void skip_category_headers(int offset);
    /** Only add offset to index, but wrap around! */
    void mod_index(int offset);

    mutable std::map<std::string, std::function<bool(const item&)>> filtercache;
};
#endif
