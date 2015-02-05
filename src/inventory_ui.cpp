#include "game.h"
#include "output.h"
#include "uistate.h"
#include "translations.h"
#include "options.h"
#include "messages.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

std::string trim_to(const std::string &text, size_t length)
{
    const size_t width = utf8_width(text.c_str());
    if(width <= length) {
        return text;
    }
    const size_t bytes_offset = cursorx_to_position(text.c_str(), length - 1, NULL, -1);
    return text.substr(0, bytes_offset) + "…";
}

/**
 * - it != NULL -> item entry, print the item name and allow selecting it, or
 * - it == NULL && category != NULL -> category header, print it specially,
 * do not allow selecting it, or
 * - it == NULL && category == NULL -> empty entry, do not display at all.
 * It is used as the last entry on a page in case the next entry would be
 * a category header.
 */
struct itemstack_or_category {
    /** The item that should be displayed here. Can be NULL. */
    const item *it;
    /** Pointer into an inventory slice, can be NULL, if not NULL, it should not
     * point to an empty list. The first entry should be the same as @ref it. */
    const std::list<item> *slice;
    /** The category of an item. */
    const item_category *category;
    /** The item position in the players inventory. It should be unique as it
     * is used by the drop map in inventory_selector. */
    int item_pos;
    itemstack_or_category(const indexed_invslice::value_type &a)
        : it(&(a.first->front())), slice(a.first), category(&it->get_category()), item_pos(a.second)
    {
    }
    itemstack_or_category(const item *a, int b)
        : it(a), slice(NULL), category(&it->get_category()), item_pos(b)
    {
    }
    itemstack_or_category(const item_category *a = NULL)
        : it(NULL), slice(NULL), category(a), item_pos(INT_MIN)
    {
    }
    // used for searching the category header, only the item pointer and the category are important there
    bool operator==(const itemstack_or_category &other) const
    {
        return category == other.category && it == other.it;
    }
};

class inventory_selector
{
    public:
        typedef std::vector<itemstack_or_category> itemstack_vector;
        /**
         * Extracts <B>slice</B> into @ref items, adding category entries.
         * For each item in the slice an entry that points to it is added to @ref items.
         * For a consecutive sequence of items of the same category a single
         * category entry is added in front of them.
         */
        void make_item_list(const indexed_invslice &slice, const item_category *def_cat = NULL);
        /**
         * Inserts additional category entries on top of each page,
         * When the last entry of a page is a category entry, inserts an empty entry
         * right before that one. The category entry goes now on the next page.
         * This is done for both list (@ref items and @ref worn).
         */
        void prepare_paging();
        /**
         * What has been selected for dropping/comparing. The key is the item position,
         * the value is the count, or -1 for dropping all. The class makes sure that
         * the count is never 0, and it is -1 only if all items should be dropped.
         * Any value > 0 means at least one item will remain after dropping.
         */
        typedef std::map<int, int> drop_map;
        drop_map dropping;
        /** when comparing: the first item to be compared, or NULL */
        item *first_item;
        /** when comparing: the second item or NULL */
        item *second_item;
        /** The input context for navigation, already contains some actions for movement.
         * See @ref handle_movement */
        input_context ctxt;
        /** Return the char plus a space that should be shown in front of an item name,
         * to indicate that this item is selected for dropping (or comparing). */
        std::string get_drop_icon(drop_map::const_iterator dit) const;
        /** Given an action from the input_context, try to act according to it:
         * move selection around (next/previous page/item).
         * If not handle by this class it return false, otherwise true (caller should
         * ignore the action in this case). */
        bool handle_movement(const std::string &action);
        /** Update the @ref w_inv window, including wrefresh */
        void display() const;
        /** Returns the item positions of the currently selected entry, or ITEM_MIN
         * if no entry is selected. */
        int get_selected_item_position() const;
        /** Set/toggle dropping count items of currently selected item stack, see @ref set_drop_count */
        void set_selected_to_drop(int count);
        /** Select the item at position and set the correct in_inventory and current_page_offset value */
        void select_item_by_position(const int &position);
        /** Starts the inventory screen
         * @param m sets @ref multidrop
         * @param c sets @ref compare
         * @param t sets @ref title
         */
        inventory_selector(bool m, bool c, const std::string &t);
        ~inventory_selector();

        void remove_dropping_items( player &u ) const;

    private:
        /** All the items that should be shown in the left column */
        itemstack_vector items;
        itemstack_vector worn;
        /** Number of rows that we have for printing the @ref items */
        size_t items_per_page;
        WINDOW *w_inv;
        const std::string title;
        /** Index of the first entry in @ref items on the currently shown page */
        size_t current_page_offset_i;
        /** Index of the first entry in @ref worn on the currently shown page */
        size_t current_page_offset_w;
        /** Index of the currently selected entry of @ref items */
        size_t selected_i;
        /** Index of the currently selected entry of @ref worn */
        size_t selected_w;
        /** Width and offsets of display columns: left (items in inventory),
         * middle (worn and weapon), right (items selected for dropping, optional)
         * the width of the right column can be 0 if it should not be shown. */
        size_t left_column_width;
        size_t left_column_offset;
        size_t middle_column_width;
        size_t middle_column_offset;
        size_t right_column_width;
        size_t right_column_offset;
        bool inCategoryMode;
        /** Allow selecting several items for dropping. And show selected items in the
         * right column. */
        const bool multidrop;
        /** Comparing items. Allow only two items to be selected. */
        const bool compare;
        bool warned_about_bionic;
        bool in_inventory;
        const item_category weapon_cat;
        const item_category worn_cat;

        void print_inv_weight_vol(int weight_carried, int vol_carried, int vol_capacity) const;
        void print_left_column() const;
        void print_middle_column() const;
        void print_right_column() const;
    public:
        /** Toggle item dropping for item position it_pos:
         * If count is > 0: set dropping to count
         * If the item is already marked for dropping: deactivate dropping,
         * If the item is not marked for dropping: set dropping to -1
         * The item reference is used to update @ref first_item / @ref second_item
         */
        void set_drop_count(int it_pos, int count, const item &it);
        /**
         * Same as @ref set_drop_count with single item,
         * if count is > 0: set count to -1 if it reaches/exceeds the maximal
         * droppable items of this stack (if stack.size() == 4 and count == 4, set
         * count to -1 because that means drop all).
         */
        void set_drop_count(int it_pos, int count, const std::list<item> &stack);
        void set_to_drop(int it_pos, int count);
        void print_column(const itemstack_vector &items, size_t y, size_t w, size_t selected,
                          size_t current_page_offset) const;
        void prepare_paging(itemstack_vector &items);
};

void inventory_selector::make_item_list(const indexed_invslice &slice, const item_category *def_cat)
{
    for( const auto &scit : slice ) {

        // That entry represents the item stack
        const itemstack_or_category item_entry(scit);
        const item_category *category = def_cat;
        if (category == NULL) {
            category = item_entry.category;
        }
        // Entry that represents the category header
        const itemstack_or_category cat_entry(category);
        itemstack_vector::iterator cat_iter = std::find(items.begin(), items.end(), cat_entry);
        if (cat_iter == items.end()) {
            // Category is not yet contained in the list, add it to the end
            items.push_back(cat_entry);
            cat_iter = items.end();
        } else {
            // Category is already contained, skip all the items that belong to the
            // category to insert the current item behind them (but before the next category)
            do {
                ++cat_iter;
            } while(cat_iter != items.end() && cat_iter->it != NULL);
        }
        items.insert(cat_iter, item_entry);
    }
}

void inventory_selector::prepare_paging()
{
    if (items.size() == 0) {
        in_inventory = false;
    }

    prepare_paging(items);
    prepare_paging(worn);
}

void inventory_selector::prepare_paging(itemstack_vector &items)
{
    const item_category *prev_category = NULL;
    for (size_t a = 0; a < items.size(); a++) {
        const itemstack_or_category &cur_entry = items[a];
        if (cur_entry.category != NULL) {
            prev_category = cur_entry.category;
        }
        if (cur_entry.it == NULL && a % items_per_page == items_per_page - 1) {
            // last entry on a page is a category, insert empty entry
            // to move category header to next page
            items.insert(items.begin() + a, itemstack_or_category());
            continue;
        }
        if (cur_entry.it != NULL && a > 0 && a % items_per_page == 0) {
            // first entry on a page and is not a category, insert previous category
            items.insert(items.begin() + a, itemstack_or_category(prev_category));
            continue;
        }
    }
}

void inventory_selector::print_inv_weight_vol(int weight_carried, int vol_carried,
        int vol_capacity) const
{
    // Print weight
    mvwprintw(w_inv, 0, 32, _("Weight (%s): "),
              OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? "lbs" : "kg");
    if (weight_carried >= g->u.weight_capacity()) {
        wprintz(w_inv, c_red, "%6.1f", g->u.convert_weight(weight_carried));
    } else {
        wprintz(w_inv, c_ltgray, "%6.1f", g->u.convert_weight(weight_carried));
    }
    wprintz(w_inv, c_ltgray, "/%-6.1f", g->u.convert_weight(g->u.weight_capacity()));

    // Print volume
    mvwprintw(w_inv, 0, 61, _("Volume: "));
    if (vol_carried > vol_capacity - 2) {
        wprintz(w_inv, c_red, "%3d", vol_carried);
    } else {
        wprintz(w_inv, c_ltgray, "%3d", vol_carried);
    }
    wprintw(w_inv, "/%-3d", vol_capacity - 2);
}

char invlet_or_space(const item &it)
{
    return (it.invlet == 0) ? ' ' : it.invlet;
}

std::string inventory_selector::get_drop_icon(drop_map::const_iterator dit) const
{
    if (!multidrop && !compare) {
        return "";
    } else if (dit == dropping.end()) {
        return "- ";
    } else if (dit->second == -1) {
        return "+ ";
    } else {
        return "# ";
    }
}

void inventory_selector::print_middle_column() const
{
    print_column(worn, middle_column_offset, middle_column_width, selected_w, current_page_offset_w);
}

void inventory_selector::print_left_column() const
{
    print_column(items, left_column_offset, left_column_width, selected_i, current_page_offset_i);
}

void inventory_selector::print_column(const itemstack_vector &items, size_t y, size_t w,
                                      size_t selected, size_t current_page_offset) const
{
    nc_color selected_line_color = inCategoryMode ? c_white_red : h_white;
    if ((&items == &this->items) != in_inventory) {
        selected_line_color = inCategoryMode ? c_ltgray_red : h_ltgray;
    }
    int cur_line = 2;
    for (size_t a = 0; a + current_page_offset < items.size() && a < items_per_page; a++, cur_line++) {
        const itemstack_or_category &cur_entry = items[a + current_page_offset];
        if (cur_entry.category == NULL) {
            continue;
        }
        if (cur_entry.it == NULL) {
            const std::string name = trim_to(cur_entry.category->name, w);
            mvwprintz(w_inv, cur_line, y, c_magenta, "%s", name.c_str());
            continue;
        }
        const item &it = *cur_entry.it;
        std::string item_name = it.display_name();
        if (cur_entry.slice != NULL) {
            const size_t count = cur_entry.slice->size();
            if (count > 1) {
                item_name = string_format("%d %s", count, it.display_name(count).c_str());
            }
        }
        nc_color name_color = it.color_in_inventory();
        nc_color invlet_color = c_white;
        if (g->u.assigned_invlet.count(it.invlet)) {
            invlet_color = c_yellow;
        }
        if (a + current_page_offset == selected) {
            name_color = selected_line_color;
            invlet_color = selected_line_color;
        }
        item_name = get_drop_icon(dropping.find(cur_entry.item_pos)) + item_name;
        item_name = trim_to(item_name, w - 2); // 2 for the invlet & space
        if (it.invlet != 0) {
            mvwputch(w_inv, cur_line, y, invlet_color, it.invlet);
        }
        mvwprintz(w_inv, cur_line, y + 2, name_color, "%s", item_name.c_str());
    }
}

void inventory_selector::print_right_column() const
{
    if (right_column_width == 0) {
        return;
    }
    player &u = g->u;
    int drp_line = 1;
    drop_map::const_iterator dit = dropping.find(-1);
    if (dit != dropping.end()) {
        std::string item_name = u.weapname();
        if (dit->second == -1) {
            item_name.insert(0, "+ ");
        } else {
            item_name = string_format("# %s {%d}", item_name.c_str(), dit->second);
        }
        const char invlet = invlet_or_space(u.weapon);
        item_name = trim_to(item_name, right_column_width - 2); // 2 for the invlet & space
        mvwprintz(w_inv, drp_line, right_column_offset, c_ltblue, "%c %s", invlet, item_name.c_str());
        drp_line++;
    }
    for (size_t k = 0; k < u.worn.size(); k++) {
        // worn items can not be dropped partially
        if (dropping.count(player::worn_position_to_index(k)) == 0) {
            continue;
        }
        const char invlet = invlet_or_space(u.worn[k]);
        std::string item_name = trim_to(u.worn[k].display_name(),
                                        right_column_width - 4); // 2 for the invlet '+' &  2 space
        mvwprintz(w_inv, drp_line, right_column_offset, c_cyan, "%c + %s", invlet, item_name.c_str());
        drp_line++;
    }
    for( const auto &elem : dropping ) {
        if( elem.first < 0 ) { // worn or wielded item, already displayed above
            continue;
        }
        const std::list<item> &stack = u.inv.const_stack( elem.first );
        const item &it = stack.front();
        const char invlet = invlet_or_space(it);
        const int count = elem.second;
        const int display_count = (count == -1) ? (it.charges >= 0) ? it.charges : stack.size() : count;
        const nc_color col = it.color_in_inventory();
        std::string item_name = it.tname( display_count );
        if (count == -1) {
            if (stack.size() > 1) {
                item_name = string_format("%d %s", stack.size(), item_name.c_str());
            } else {
                item_name.insert(0, "+ ");
            }
        } else {
            item_name = string_format("# %s {%d}", item_name.c_str(), count);
        }
        item_name = trim_to(item_name, right_column_width - 2); // 2 for the invlet & space
        mvwprintz(w_inv, drp_line, right_column_offset, col, "%c %s", invlet, item_name.c_str());
        drp_line++;
    }
}

void inventory_selector::display() const
{
    const size_t &current_page_offset = in_inventory ? current_page_offset_i : current_page_offset_w;
    werase(w_inv);
    mvwprintw(w_inv, 0, 0, title.c_str());
    if (multidrop) {
        mvwprintw(w_inv, 1, 0, _("To drop x items, type a number and then the item hotkey."));
    }
    std::string msg_str;
    nc_color msg_color;
    if (inCategoryMode) {
        msg_str = _("Category selection; [TAB] switches mode, arrows select.");
        msg_color = c_white_red;
    } else {
        msg_str = _("Item selection; [TAB] switches mode, arrows select.");
        msg_color = h_white;
    }
    mvwprintz(w_inv, items_per_page + 4, FULL_SCREEN_WIDTH - utf8_width(msg_str.c_str()),
              msg_color, msg_str.c_str());
    print_left_column();
    print_middle_column();
    print_right_column();
    const size_t max_size = in_inventory ? items.size() : worn.size();
    const size_t max_pages = (max_size + items_per_page - 1) / items_per_page;
    mvwprintw(w_inv, items_per_page + 4, 1, _("Page %d/%d"), current_page_offset / items_per_page + 1,
              max_pages);
    if (multidrop) {
        // Make copy, remove to be dropped items from that
        // copy and let the copy recalculate the volume capacity
        // (can be affected by various traits).
        player tmp = g->u;
        // first round: remove weapon & worn items, start with larges worn index
        for( const auto &elem : dropping ) {
            if( elem.first == -1 && elem.second == -1 ) {
                tmp.remove_weapon();
            } else if( elem.first == -1 && elem.second != -1 ) {
                tmp.weapon.charges -= elem.second;
            } else if( elem.first < 0 ) {
                tmp.worn.erase( tmp.worn.begin() + player::worn_position_to_index( elem.first ) );
            }
        }
        remove_dropping_items(tmp);
        print_inv_weight_vol(tmp.weight_carried(), tmp.volume_carried(), tmp.volume_capacity());
    } else {
        print_inv_weight_vol(g->u.weight_carried(), g->u.volume_carried(), g->u.volume_capacity());
    }
    if (!multidrop && !compare) {
        mvwprintw(w_inv, 1, 61, _("Hotkeys:  %d/%d "),
                  g->u.allocated_invlets().size(), inv_chars.size());
    }
    wrefresh(w_inv);
}

inventory_selector::inventory_selector(bool m, bool c, const std::string &t)
    : dropping()
    , first_item(NULL)
    , second_item(NULL)
    , ctxt("INVENTORY")
    , items()
    , worn()
    , items_per_page(TERMY - 5) // gives us 5 lines for messages/help text/status/...
    , w_inv(NULL)
    , title(t)
    , current_page_offset_i(0)
    , current_page_offset_w(0)
    , selected_i(1) // first is the category header
    , selected_w(1) // ^^
    , inCategoryMode(false)
    , multidrop(m)
    , compare(c)
    , warned_about_bionic(false)
    , in_inventory(true)
    , weapon_cat("WEAPON", _("WEAPON:"), 0)
    , worn_cat("ITEMS WORN", _("ITEMS WORN:"), 0)
{
    w_inv = newwin(TERMY, TERMX, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    if (compare || multidrop) {
        left_column_width = 40;
        left_column_offset = 0;
        middle_column_width = std::min<int>( TERMX - left_column_width - 1, 40 );
        right_column_width = std::max<int>( 0, TERMX - left_column_width - middle_column_width - 2 );
    } else {
        left_column_width = TERMX / 2;
        left_column_offset = 0;
        middle_column_width = TERMX - left_column_width - 1;
        right_column_width = 0;
    }
    middle_column_offset = left_column_width + left_column_offset + 1;
    right_column_offset = middle_column_width + middle_column_offset + 1;
    ctxt.register_action("DOWN", _("Next item"));
    ctxt.register_action("UP", _("Previous item"));
    ctxt.register_action("RIGHT", _("Confirm"));
    ctxt.register_action("LEFT", _("Switch inventory/worn"));
    ctxt.register_action("CONFIRM", _("Mark selected item"));
    ctxt.register_action("QUIT", _("Cancel"));
    ctxt.register_action("CATEGORY_SELECTION");
    ctxt.register_action("NEXT_TAB", _("Page down"));
    ctxt.register_action("PREV_TAB", _("Page up"));
    ctxt.register_action("HELP_KEYBINDINGS");
    // For invlets
    ctxt.register_action("ANY_INPUT");

    player &u = g->u;
    if (u.is_armed()) {
        worn.push_back(itemstack_or_category(&weapon_cat));
        worn.push_back(itemstack_or_category(&u.weapon, -1));
    }
    if (!u.worn.empty()) {
        worn.push_back(itemstack_or_category(&worn_cat));
    }
    for (size_t i = 0; i < u.worn.size(); i++) {
        worn.push_back(itemstack_or_category(&u.worn[i], player::worn_position_to_index(i)));
    }
}

inventory_selector::~inventory_selector()
{
    if (w_inv != NULL) {
        werase(w_inv);
        delwin(w_inv);
    }
    g->refresh_all();
}

bool inventory_selector::handle_movement(const std::string &action)
{
    const itemstack_vector &items = in_inventory ? this->items : this->worn;
    size_t &selected = in_inventory ? selected_i : selected_w;
    size_t &current_page_offset = in_inventory ? current_page_offset_i : current_page_offset_w;
    const item_category *cur_cat = (selected < items.size()) ? items[selected].category : NULL;

    if (action == "CATEGORY_SELECTION") {
        inCategoryMode = !inCategoryMode;
    } else if (action == "LEFT") {
        if (this->items.size() > 0) {
            in_inventory = !in_inventory;
        }
    } else if (action == "DOWN") {
        selected++;
        if (inCategoryMode) {
            while (selected < items.size() && items[selected].category == cur_cat) {
                selected++;
            }
        }
        // skip non-item entries, those can not be selected!
        while (selected < items.size() && items[selected].it == NULL) {
            selected++;
        }
        if (selected >= items.size()) { // wrap around
            selected = 1; // first is the category!
        }
        current_page_offset = selected - (selected % items_per_page);
    } else if (action == "UP") {
        selected--;
        if (inCategoryMode && selected == 0) {
            selected = items.size() - 1;
        }
        if (inCategoryMode) {
            while (selected < items.size() && items[selected].category == cur_cat) {
                selected--;
            }
        }
        // skip non-item entries, those can not be selected!
        while (selected < items.size() && items[selected].it == NULL) {
            selected--;
        }
        if (selected >= items.size()) { // wrap around, actually overflow
            selected = items.size() - 1; // the last is always an item entry
        }
        current_page_offset = selected - (selected % items_per_page);
    } else if (action == "NEXT_TAB") {
        selected += items_per_page;
        // skip non-item entries, those can not be selected!
        while (selected < items.size() && items[selected].it == NULL) {
            selected++;
        }
        if (selected >= items.size()) { // wrap around
            selected = 1; // first is the category!
        }
        current_page_offset = selected - (selected % items_per_page);
    } else if (action == "PREV_TAB") {
        selected -= items_per_page;
        // skip non-item entries, those can not be selected!
        while (selected < items.size() && items[selected].it == NULL) {
            selected--;
        }
        if (selected >= items.size()) { // wrap around, actually overflow
            selected = items.size() - 1; // the last is always an item entry
        }
        current_page_offset = selected - (selected % items_per_page);
    } else {
        return false;
    }
    return true;
}

void inventory_selector::select_item_by_position(const int &position)
{
    if (position != INT_MIN) {
        int pos = position;

        if (pos == -1) {
            //weapon
            in_inventory = false;
            return;

        } else if (pos < -1) {
            //worn
            in_inventory = false;
            pos = abs(position) - ((g->u.weapon.is_null()) ? 2 : 1);
        }

        const itemstack_vector &items = in_inventory ? this->items : this->worn;
        size_t &selected = in_inventory ? selected_i : selected_w;
        size_t &current_page_offset = in_inventory ? current_page_offset_i : current_page_offset_w;

        //skip headers
        int iHeaderOffset = 0;
        for( auto &item : items ) {
            if( item.it == NULL ) {
                iHeaderOffset++;

            } else if( item.item_pos == pos ) {
                break;
            }
        }

        selected = pos + iHeaderOffset;
        current_page_offset = selected - (selected % items_per_page);
    }
}

int inventory_selector::get_selected_item_position() const
{
    const itemstack_vector &items = in_inventory ? this->items : this->worn;
    const size_t &selected = in_inventory ? selected_i : selected_w;
    if (selected < items.size() && items[selected].it != NULL) {
        return items[selected].item_pos;
    }
    return INT_MIN;
}

void inventory_selector::set_selected_to_drop(int count)
{
    const itemstack_vector &items = in_inventory ? this->items : this->worn;
    const size_t &selected = in_inventory ? selected_i : selected_w;
    if (selected >= items.size()) {
        return;
    }
    const itemstack_or_category &cur_entry = items[selected];
    if (cur_entry.it != NULL && cur_entry.slice != NULL) {
        set_drop_count(cur_entry.item_pos, count, *cur_entry.slice);
    } else if (cur_entry.it != NULL) {
        if (count > 0 && (!cur_entry.it->count_by_charges() || count >= cur_entry.it->charges)) {
            count = -1;
        }
        set_drop_count(cur_entry.item_pos, count, *cur_entry.it);
    }
}

void inventory_selector::set_to_drop(int it_pos, int count)
{
    player &u = g->u;
    if (it_pos == -1) { // weapon
        if (u.weapon.is_null()) {
            return;
        }
        if (count > 0 && (!u.weapon.count_by_charges() || count >= u.weapon.charges)) {
            count = -1; // drop whole item, because it can not be separated, or the requested count means all
        }
        // Must bypass the set_drop_count() that takes a stack,
        // because it must get a direct reference to weapon.
        set_drop_count(it_pos, count, u.weapon);
    } else if (it_pos < -1) { // worn
        const size_t wpos = player::worn_position_to_index(it_pos);
        if (wpos >= u.worn.size()) {
            return; // invalid it_pos -> ignore
        }
        if (count > 0) {
            count = -1; // can only drop a whole worn item
        }
        set_drop_count(it_pos, count, u.worn[wpos]);
    } else { // inventory
        const std::list<item> &stack = u.inv.const_stack(it_pos);
        if (stack.empty()) {
            return; // invalid it_pos -> ignore
        }
        set_drop_count(it_pos, count, stack);
    }
}

void inventory_selector::set_drop_count(int it_pos, int count, const std::list<item> &stack)
{
    if (stack.size() == 1) {
        const item &it = stack.front();
        if (count > 0 && (!it.count_by_charges() || count >= it.charges)) {
            count = -1; // drop whole item, because it can not be separated, or count is big enough anyway
        }
    } else if (count > 0 && (size_t) count >= stack.size()) {
        count = -1; // count indicates whole stack anyway
    } else if (stack.empty()) {
        return;
    }
    set_drop_count(it_pos, count, stack.front());
}

void inventory_selector::set_drop_count(int it_pos, int count, const item &it)
{
    // "dropping" when comparing means select for comparison, valid for bionics
    if (it_pos == -1 && g->u.weapon.has_flag("NO_UNWIELD") && !compare) {
        if (!warned_about_bionic) {
            popup(_("You cannot drop your %s."), g->u.weapon.tname().c_str());
            warned_about_bionic = true;
        }
        return;
    }
    // count 0 means toggle, if already selected for dropping, drop none
    drop_map::iterator iit = dropping.find(it_pos);
    if (count == 0 && iit != dropping.end()) {
        dropping.erase(iit);
        if (first_item == &it) {
            first_item = second_item;
            second_item = NULL;
        } else if (second_item == &it) {
            second_item = NULL;
        }
    } else {
        // allow only -1 or anything > 0
        dropping[it_pos] = (count <= 0) ? -1 : count;
        if (first_item == NULL || first_item == &it) {
            first_item = const_cast<item *>(&it);
        } else {
            second_item = const_cast<item *>(&it);
        }
    }
}

void inventory_selector::remove_dropping_items( player &u ) const
{
    // We iterate backwards because deletion will invalidate later indices.
    for( inventory_selector::drop_map::const_reverse_iterator a = dropping.rbegin();
         a != dropping.rend(); ++a ) {
        if( a->first < 0 ) { // weapon or armor, handled separately
            continue;
        }
        const int count = a->second;
        item &tmpit = u.inv.find_item( a->first );
        if( tmpit.count_by_charges() ) {
            long charges = tmpit.charges;
            if( count != -1 && count < charges ) {
                tmpit.charges -= count;
            } else {
                u.inv.remove_item( a->first );
            }
        } else {
            size_t max_count = u.inv.const_stack( a->first ).size();
            if( count != -1 && ( size_t )count < max_count ) {
                max_count = count;
            }
            for( size_t i = 0; i < max_count; i++ ) {
                u.inv.remove_item( a->first );
            }
        }
    }
}

int game::display_slice(indexed_invslice &slice, const std::string &title, const int &position)
{
    inventory_selector inv_s(false, false, title);
    inv_s.make_item_list(slice);
    inv_s.prepare_paging();
    inv_s.select_item_by_position(position);

    while(true) {
        inv_s.display();
        const std::string action = inv_s.ctxt.handle_input();
        const long ch = inv_s.ctxt.get_raw_input().get_first_input();
        const int item_pos = g->u.invlet_to_position(static_cast<char>(ch));
        if (item_pos != INT_MIN) {
            return item_pos;
        } else if (inv_s.handle_movement(action)) {
            continue;
        } else if (action == "CONFIRM" || action == "RIGHT") {
            return inv_s.get_selected_item_position();
        } else if (action == "QUIT") {
            return INT_MIN;
        } else {
            return INT_MIN;
        }
    }
}

// Display current inventory.
int game::inv(const std::string &title, const int &position)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice slice = u.inv.slice_filter();
    return display_slice(slice, title, position);
}

int game::inv_activatable(std::string title)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice activatables = u.inv.slice_filter_by_activation(u);
    return display_slice(activatables, title);
}

int game::inv_type(std::string title, item_cat inv_item_type)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_category(inv_item_type, u);
    return display_slice(reduced_inv, title);
}

int game::inv_for_liquid(const item &liquid, const std::string title, bool auto_choose_single)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_capacity_for_liquid(liquid);
    if (auto_choose_single && reduced_inv.size() == 1) {
        std::list<item> *cont_stack = reduced_inv[0].first;
        if (! cont_stack->empty() ) {
            return reduced_inv[0].second;
        }
    }
    return display_slice(reduced_inv, title);
}

int game::inv_for_salvage(const std::string title)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_salvageability();
    return display_slice(reduced_inv, title);
}

item *game::inv_map_for_liquid(const item &liquid, const std::string title)
{
    auto here = m.i_at(g->u.posx(), g->u.posy());
    typedef std::vector< std::list<item> > pseudo_inventory;
    pseudo_inventory grounditems;
    indexed_invslice grounditems_slice;
    std::vector<item *> ground_containers;

    std::set<std::string> dups;
    for( auto candidate = here.begin(); candidate != here.end(); ++candidate ) {
        if( candidate->get_remaining_capacity_for_liquid( liquid ) > 0 ) {
            if( dups.count( candidate->tname() ) == 0 ) {
                grounditems.push_back( std::list<item>( 1, *candidate ) );

                if( grounditems.size() <= 10 ) {
                    grounditems.back().front().invlet = '0' + grounditems.size() - 1;
                } else {
                    grounditems.back().front().invlet = ' ';
                }
                dups.insert( candidate->tname() );

                ground_containers.push_back( &*candidate );
            }
        }
    }

    for (size_t a = 0; a < grounditems.size(); a++) {
        // avoid INT_MIN, as it can be confused with "no item at all"
        grounditems_slice.push_back(indexed_invslice::value_type(&grounditems[a], INT_MIN + a + 1));
    }
    static const item_category category_on_ground(
        "GROUND:",
        _("GROUND:"),
        -1000
    );

    u.inv.restack(&u);
    u.inv.sort();
    const indexed_invslice stacks = u.inv.slice_filter_by_capacity_for_liquid(liquid);

    inventory_selector inv_s(false, true, title);
    inv_s.make_item_list(grounditems_slice, &category_on_ground);
    inv_s.make_item_list(stacks);
    inv_s.prepare_paging();

    inventory_selector::drop_map prev_droppings;
    while (true) {
        inv_s.display();
        const std::string action = inv_s.ctxt.handle_input();
        const long ch = inv_s.ctxt.get_raw_input().get_first_input();
        const int item_pos = g->u.invlet_to_position(static_cast<char>(ch));

        if (item_pos != INT_MIN) {
            inv_s.set_to_drop(item_pos, 0);
            return inv_s.first_item;
        } else if (ch >= '0' && ch <= '9' && (size_t)(ch - '0') < grounditems_slice.size()) {
            const int ip = ch - '0';
            return ground_containers[ip];
        } else if (inv_s.handle_movement(action)) {
            // continue with comparison below
        } else if (action == "QUIT") {
            return NULL;
        } else if (action == "RIGHT" || action == "CONFIRM") {

            inv_s.set_selected_to_drop(0);

            for( size_t i = 0; i < grounditems_slice.size(); i++) {
                if( &grounditems_slice[i].first->front() == inv_s.first_item ) {
                    return ground_containers[i];
                }
            }

            return inv_s.first_item;
        }
    }
}

int game::inv_for_flag(const std::string flag, const std::string title, bool auto_choose_single)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_flag(flag);
    if (auto_choose_single && reduced_inv.size() == 1) {
        std::list<item> *cont_stack = reduced_inv[0].first;
        if (! cont_stack->empty() ) {
            return reduced_inv[0].second;
        }
    }
    return display_slice(reduced_inv, title);
}

int game::inv_for_filter(const std::string title, const item_filter filter )
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by( filter );
    return display_slice(reduced_inv, title);
}

int inventory::num_items_at_position( int position )
{
    if( position < -1 ) {
        return g->u.worn[ player::worn_position_to_index(position) ].count_by_charges() ?
            g->u.worn[ player::worn_position_to_index(position) ].charges : 1;
    } else if( position == -1 ) {
        return g->u.weapon.count_by_charges() ? g->u.weapon.charges : 1;
    } else {
        const std::list<item> &stack = g->u.inv.const_stack(position);
        if( stack.size() == 1 ) {
            return stack.front().count_by_charges() ?
                stack.front().charges : 1;
        } else {
            return stack.size();
        }
    }
}

std::list<std::pair<int, int>> game::multidrop()
{
    u.inv.restack(&u);
    u.inv.sort();
    const indexed_invslice stacks = u.inv.slice_filter();
    inventory_selector inv_s(true, false, _("Multidrop:"));
    inv_s.make_item_list(stacks);
    inv_s.prepare_paging();
    int count = 0;
    while(true) {
        inv_s.display();
        const std::string action = inv_s.ctxt.handle_input();
        const long ch = inv_s.ctxt.get_raw_input().get_first_input();
        const int item_pos = g->u.invlet_to_position(static_cast<char>(ch));
        if (ch >= '0' && ch <= '9') {
            count = std::max( 0, count * 10 + ((char)ch - '0') );
        } else if (item_pos != INT_MIN) {
            inv_s.set_to_drop(item_pos, count);
            count = 0;
        } else if (inv_s.handle_movement(action)) {
            count = 0;
            continue;
        } else if (action == "CONFIRM") {
            break;
        } else if (action == "QUIT") {
            return std::list<std::pair<int, int> >();
        } else if (action == "RIGHT") {
            inv_s.set_selected_to_drop(count);
            count = 0;
        }
    }

    std::list<std::pair<int, int>> dropped_pos_and_qty;

    for( auto drop_pair : inv_s.dropping ) {
        int num_to_drop = drop_pair.second;
        if( num_to_drop == -1 ) {
            num_to_drop = inventory::num_items_at_position( drop_pair.first );
        }
        dropped_pos_and_qty.push_back( std::make_pair( drop_pair.first, num_to_drop ) );
    }

    return dropped_pos_and_qty;
}

void game::compare(int iCompareX, int iCompareY)
{
    int examx, examy;

    if (iCompareX != -999 && iCompareY != -999) {
        examx = u.posx() + iCompareX;
        examy = u.posy() + iCompareY;
    } else if (!choose_adjacent(_("Compare where?"), examx, examy)) {
        return;
    }

    auto here = m.i_at(examx, examy);
    typedef std::vector< std::list<item> > pseudo_inventory;
    pseudo_inventory grounditems;
    indexed_invslice grounditems_slice;
    //Filter out items with the same name (keep only one of them)
    //Only the first 10 Items due to numbering 0-9
    std::set<std::string> dups;
    for (size_t i = 0; i < here.size() && grounditems.size() < 10; i++) {
        if (dups.count(here[i].tname()) == 0) {
            grounditems.push_back(std::list<item>(1, here[i]));
            // invlet: '0' ... '9'
            grounditems.back().front().invlet = '0' + grounditems.size() - 1;
            dups.insert(here[i].tname());
        }
    }
    for (size_t a = 0; a < grounditems.size(); a++) {
        // avoid INT_MIN, as it can be confused with "no item at all"
        grounditems_slice.push_back(indexed_invslice::value_type(&grounditems[a], INT_MIN + a + 1));
    }
    static const item_category category_on_ground(
        "GROUND:",
        _("GROUND:"),
        -1000
    );

    u.inv.restack(&u);
    u.inv.sort();
    const indexed_invslice stacks = u.inv.slice_filter();

    inventory_selector inv_s(false, true, _("Compare:"));
    inv_s.make_item_list(grounditems_slice, &category_on_ground);
    inv_s.make_item_list(stacks);
    inv_s.prepare_paging();

    inventory_selector::drop_map prev_droppings;
    while(true) {
        inv_s.display();
        const std::string action = inv_s.ctxt.handle_input();
        const long ch = inv_s.ctxt.get_raw_input().get_first_input();
        const int item_pos = g->u.invlet_to_position(static_cast<char>(ch));
        if (item_pos != INT_MIN) {
            inv_s.set_to_drop(item_pos, 0);
        } else if (ch >= '0' && ch <= '9' && (size_t) (ch - '0') < grounditems_slice.size()) {
            const int ip = ch - '0';
            inv_s.set_drop_count(INT_MIN + 1 + ip, 0, grounditems_slice[ip].first->front());
        } else if (inv_s.handle_movement(action)) {
            // continue with comparison below
        } else if (action == "QUIT") {
            break;
        } else if (action == "RIGHT") {
            inv_s.set_selected_to_drop(0);
        }
        if (inv_s.second_item != NULL) {
            std::vector<iteminfo> vItemLastCh, vItemCh;
            std::string sItemLastCh, sItemCh;
            inv_s.first_item->info(true, &vItemCh);
            sItemCh = inv_s.first_item->tname();
            inv_s.second_item->info(true, &vItemLastCh);
            sItemLastCh = inv_s.second_item->tname();
            draw_item_info(0, (TERMX - VIEW_OFFSET_X * 2) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                           sItemLastCh, vItemLastCh, vItemCh, -1, true); //without getch()
            draw_item_info((TERMX - VIEW_OFFSET_X * 2) / 2, (TERMX - VIEW_OFFSET_X * 2) / 2,
                           0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, vItemCh, vItemLastCh);
            inv_s.dropping = prev_droppings;
            inv_s.second_item = NULL;
        } else {
            prev_droppings = inv_s.dropping;
        }
    }
}
