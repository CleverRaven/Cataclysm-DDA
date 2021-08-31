#pragma once
#ifndef CATA_SRC_ITEM_CONTENTS_UI_H
#define CATA_SRC_ITEM_CONTENTS_UI_H

#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "ui_manager.h"

class item;
class string_input_popup;

class item_contents_ui
{
        /**The input context for navigation of the menu **/
        input_context ctxt;

        /** Pointer reference to the item whose contents are being examined.  An item_location
        * would probably be preferred, but can't currently turn a const item* into an
         * item_location **/
        const item *pContaining_item;

        /** A collection of pointers to the items contained withing pContaining_item.  Should only
        * be used directly for creation of vFilteredItemList.  **/
        std::vector<const item *> vItemList;

        /** A collection of pointers to the items contained withing pContaining_item.  Filtered
        * based on their display_name() in item_contents_ui::set_filter() **/
        std::vector<const item *> vFilteredItemList;

        /** Window where the name of the containing item is listed **/
        catacurses::window w_head;

        /** Window where the list of contained items is drawn (on the left), and the footer is
        * drawn (including keybinding tips and the filter widget.  Note: Since this includes the
         * footer, it spans full-width, and overlaps with w_iteminfo **/
        catacurses::window w_data;

        /** Window where detailed information on the selected contained item is shown.  Note:
        * Since w_data includes the footer, it spans full-width, and overlaps with this window **/
        catacurses::window w_item_info;

        //General ui variables
        int ui_width;

        //Variables relating to w_data window.  NOTE: data_width is the usable width for the item list
        //NOT the width of w_data
        int data_lines;
        int data_height;
        int data_width;
        int max_item_name_width;
        int selected_item_line;
        int max_items;

        //Header and footer variables
        int foot_height;
        int head_height;

        //Variables relating to the w_iteminfo window
        int item_info_width;
        int item_info_scroll;

        /** String used to filter item list.  Used in ::set_filter() and displayed in ::draw_footer() **/
        std::string filter_string;

        //Keybinding display variables
        int keybinding_x;
        std::vector<std::string> keybinding_tips;

        //Reference to the ui object.  TODO: This probably shouldn't be a class member variable
        ui_adaptor ui;

        cata::optional<point> cursor_pos;

        /** Draws a full box border around w_head and w_data.  Does not separate out w_item_info or
        * the footer **/
        void draw_borders();
        /** Draws the display name of the containing item to act as the header **/
        void draw_header();
        /** Draws the filtered list of contained items.  Ensures proper scrolling and highlights the
        * currently selected item **/
        void draw_item_list();
        /** Draws the 'e'xamine results for the currently selected contained item in w_item_info.
        * If, for some reason, no item is selected, should show an error message.  **/
        void draw_details();
        /** Draw the keybindings of interest and current filter string in the footer**/
        void draw_footer();
        /** Provides a popup for entering filter_string, then creates a fresh vFilteredItemList based on
        * the display_name() of each item in vItemList **/
        void set_filter();

        /** Used to handle input of filter_string **/
        std::unique_ptr<string_input_popup> spopup;
    public:
        item_contents_ui( const item *containing_item );
        /** Primary interface function for the class.  After creation, call this to display the UI **/
        void execute();
};


#endif // CATA_SRC_ITEM_CONTENTS_UI_H
