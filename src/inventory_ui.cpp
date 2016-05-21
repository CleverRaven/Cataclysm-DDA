#include "game.h"
#include "player.h"
#include "action.h"
#include "map.h"
#include "output.h"
#include "uistate.h"
#include "translations.h"
#include "options.h"
#include "messages.h"
#include "input.h"
#include "catacharset.h"
#include "item_location.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "cata_utility.h"
#include "itype.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

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

const item_filter allow_all_items = []( const item & ) { return true; };

class inventory_selector
{
    public:
        enum add_to {
            AT_BEGINNING,
            AT_END
        };
        /**
         * Extracts <B>slice</B> into @ref items, adding category entries.
         * For each item in the slice an entry that points to it is added to @ref items.
         * For a consecutive sequence of items of the same category a single
         * category entry is added in front of them or after them depending on @ref where.
         */
        void add_items( const indexed_invslice &slice, add_to where, const item_category *def_cat = nullptr );
        /**
         * Checks the selector for emptiness (absence of available items).
         */
        bool empty() const {
            return items.empty() && worn.empty();
        }
        /** Creates the inventory screen */
        inventory_selector( player &u, item_filter filter = allow_all_items );
        ~inventory_selector();

        /** Executes the selector */
        int execute_pick( const std::string &title, const int position = INT_MIN );
        // @todo opts should not be passed here. Temporary solution for further refactoring
        item_location execute_pick_map( const std::string &title, std::unordered_map<item *, item_location> &opts );
        void execute_compare( const std::string &title );
        std::list<std::pair<int, int>> execute_multidrop( const std::string &title );

    private:
        typedef std::vector<itemstack_or_category> itemstack_vector;

        enum selector_mode{
            SM_PICK,
            SM_COMPARE,
            SM_MULTIDROP
        };
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
        /** Given an action from the input_context, try to act according to it:
         * move selection around (next/previous page/item).
         * If not handle by this class it return false, otherwise true (caller should
         * ignore the action in this case). */
        bool handle_movement(const std::string &action);
        /** Update the @ref w_inv window, including wrefresh */
        void display( const std::string &title, selector_mode mode ) const;
        /** Returns the item positions of the currently selected entry, or ITEM_MIN
         * if no entry is selected. */
        int get_selected_item_position() const;
        /** Set/toggle dropping count items of currently selected item stack, see @ref set_drop_count */
        void set_selected_to_drop(int count);
        /** Select the item at position and set the correct in_inventory and current_page_offset value */
        void select_item_by_position(const int &position);
        void remove_dropping_items( player &u ) const;
        /** All the items that should be shown in the left column */
        itemstack_vector items;
        itemstack_vector worn;
        /** Number of rows that we have for printing the @ref items */
        size_t items_per_page;
        WINDOW *w_inv;
        /** Index of the first entry in @ref items on the currently shown page */
        size_t current_page_offset_i;
        /** Index of the first entry in @ref worn on the currently shown page */
        size_t current_page_offset_w;
        /** Index of the currently selected entry of @ref items */
        size_t selected_i;
        /** Index of the currently selected entry of @ref worn */
        size_t selected_w;
        bool inCategoryMode;
        bool in_inventory;
        const item_category weapon_cat;
        const item_category worn_cat;
        player &u;

        void print_inv_weight_vol(int weight_carried, int vol_carried, int vol_capacity) const;
        void print_right_column(size_t right_column_width, size_t right_column_offset) const;

        /** Returns an entry from @ref items by its invlet */
        const itemstack_or_category *invlet_to_itemstack( long invlet ) const;
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
                          size_t current_page_offset, selector_mode mode) const;
        void prepare_paging(itemstack_vector &items);
};

void inventory_selector::add_items(const indexed_invslice &slice, add_to where, const item_category *def_cat)
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
            // Category is not yet contained in the list, add it
            switch ( where ) {
                case AT_BEGINNING:
                    cat_iter = ++items.insert( items.begin(), cat_entry );
                    break;
                case AT_END:
                    items.push_back( cat_entry );
                    cat_iter = items.end();
                    break;
            }
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

const itemstack_or_category *inventory_selector::invlet_to_itemstack( long invlet ) const
{
    const auto find_among = [ invlet ]( const itemstack_vector &stacks ) -> const itemstack_or_category* {
        for( const auto &it : stacks ) {
            if( it.it != nullptr && it.it->invlet == invlet ) {
                return &it;
            }
        }
        return nullptr;
    };
    const auto itemstack = find_among( items );
    if( itemstack != nullptr ) {
        return itemstack;
    }
    return find_among( worn );
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
    mvwprintw(w_inv, 0, 32, _("Weight (%s): "), weight_units());
    nc_color weight_color;
    if (weight_carried > u.weight_capacity()) {
        weight_color = c_red;
    } else {
        weight_color = c_ltgray;
    }
    wprintz(w_inv, weight_color, "%6.1f", convert_weight(weight_carried) + 0.05 ); // +0.05 to round up;
    wprintz(w_inv, c_ltgray, "/%-6.1f", convert_weight(u.weight_capacity()));

    // Print volume
    mvwprintw(w_inv, 0, 61, _("Volume: "));
    if (vol_carried > vol_capacity) {
        wprintz(w_inv, c_red, "%3d", vol_carried);
    } else {
        wprintz(w_inv, c_ltgray, "%3d", vol_carried);
    }
    wprintw(w_inv, "/%-3d", vol_capacity);
}

char invlet_or_space(const item &it)
{
    return (it.invlet == 0) ? ' ' : it.invlet;
}

void inventory_selector::print_column(const itemstack_vector &items, size_t y, size_t w,
                                      size_t selected, size_t current_page_offset, selector_mode mode) const
{
    const auto get_drop_icon = [ this, mode ]( const drop_map::const_iterator &dit ) -> std::string {
        if( mode == SM_PICK ) {
            return "";
        } else if( dit == dropping.end() ) {
            return "- ";
        } else if( dit->second == -1 ) {
            return "+ ";
        } else {
            return "# ";
        }
    };

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
            trim_and_print( w_inv, cur_line, y, w, c_magenta,
                            "%s", cur_entry.category->name.c_str() );
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
        if (u.assigned_invlet.count(it.invlet)) {
            invlet_color = c_yellow;
        }
        if (a + current_page_offset == selected) {
            name_color = selected_line_color;
            invlet_color = selected_line_color;
        }
        item_name = get_drop_icon(dropping.find(cur_entry.item_pos)) + item_name;
        if (it.invlet != 0) {
            mvwputch(w_inv, cur_line, y, invlet_color, it.invlet);
        }
        if (OPTIONS["ITEM_SYMBOLS"]) {
            item_name = string_format("%c %s", it.symbol(), item_name.c_str());
        }
        trim_and_print(w_inv, cur_line, y + 2, w - 2, name_color, "%s", item_name.c_str());
    }
}

void inventory_selector::print_right_column( size_t right_column_width, size_t right_column_offset ) const
{
    if (right_column_width == 0) {
        return;
    }
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
        trim_and_print(w_inv, drp_line, right_column_offset, right_column_width - 4, c_ltblue, "%c %s", invlet, item_name.c_str());
        drp_line++;
    }
    auto iter = u.worn.begin();
    for (size_t k = 0; k < u.worn.size(); k++, ++iter) {
        // worn items can not be dropped partially
        if (dropping.count(player::worn_position_to_index(k)) == 0) {
            continue;
        }
        const char invlet = invlet_or_space(*iter);
        trim_and_print(w_inv, drp_line, right_column_offset, right_column_width - 4, c_cyan, "%c + %s", invlet, iter->display_name().c_str());
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
        trim_and_print(w_inv, drp_line, right_column_offset, right_column_width - 2, col, "%c %s", invlet, item_name.c_str());
        drp_line++;
    }
}

void inventory_selector::display( const std::string &title, selector_mode mode ) const
{
    const size_t left_column_width = ( mode == SM_PICK ) ? TERMX / 2 : 40; // Do we really need this difference?
    const size_t left_column_offset = 1;
    const size_t middle_column_width = std::min<int>( TERMX - left_column_width - 1, 40 );
    const size_t middle_column_offset = left_column_width + left_column_offset + 1;
    const size_t current_page_offset = in_inventory ? current_page_offset_i : current_page_offset_w;

    werase(w_inv);
    mvwprintw(w_inv, 0, 0, title.c_str());

    if( mode != SM_PICK ) {
        const size_t right_column_width = std::max<int>( 0, TERMX - left_column_width - middle_column_width - 2 );
        const size_t right_column_offset = middle_column_width + middle_column_offset + 1;

        print_right_column( right_column_width, right_column_offset );
    } else {
        mvwprintw(w_inv, 1, 61, _("Hotkeys:  %d/%d "), u.allocated_invlets().size(), inv_chars.size());
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
    mvwprintz(w_inv, items_per_page + 4, FULL_SCREEN_WIDTH - utf8_width(msg_str),
              msg_color, msg_str.c_str());
    print_column(items, left_column_offset, left_column_width, selected_i, current_page_offset_i, mode);
    print_column(worn, middle_column_offset, middle_column_width, selected_w, current_page_offset_w, mode);
    const size_t max_size = in_inventory ? items.size() : worn.size();
    const size_t max_pages = (max_size + items_per_page - 1) / items_per_page;
    mvwprintw(w_inv, items_per_page + 4, 1, _("Page %d/%d"), current_page_offset / items_per_page + 1,
              max_pages);
    if (mode == SM_MULTIDROP) {
        // Make copy, remove to be dropped items from that
        // copy and let the copy recalculate the volume capacity
        // (can be affected by various traits).
        player tmp = u;
        // first round: remove weapon & worn items, start with larges worn index
        for( const auto &elem : dropping ) {
            if( elem.first == -1 && elem.second == -1 ) {
                tmp.remove_weapon();
            } else if( elem.first == -1 && elem.second != -1 ) {
                tmp.weapon.charges -= elem.second;
            } else if( elem.first < 0 ) {
                tmp.i_rem( elem.first );
            }
        }
        remove_dropping_items(tmp);
        print_inv_weight_vol(tmp.weight_carried(), tmp.volume_carried(), tmp.volume_capacity());
        mvwprintw(w_inv, 1, 0, _("To drop x items, type a number and then the item hotkey."));
    } else {
        print_inv_weight_vol(u.weight_carried(), u.volume_carried(), u.volume_capacity());
    }
    if( empty() ) {
        mvwprintw(w_inv, 2, 0, _( "Your inventory is empty." ) );
    }
    wrefresh(w_inv);
}

inventory_selector::inventory_selector( player &u, item_filter filter )
    : dropping()
    , first_item(NULL)
    , second_item(NULL)
    , ctxt("INVENTORY")
    , items()
    , worn()
    , items_per_page(TERMY - 5) // gives us 5 lines for messages/help text/status/...
    , w_inv(NULL)
    , current_page_offset_i(0)
    , current_page_offset_w(0)
    , selected_i(1) // first is the category header
    , selected_w(1) // ^^
    , inCategoryMode(false)
    , in_inventory(true)
    , weapon_cat("WEAPON", _("WEAPON:"), 0)
    , worn_cat("ITEMS WORN", _("ITEMS WORN:"), 0)
    , u(u)
{
    w_inv = newwin(TERMY, TERMX, VIEW_OFFSET_Y, VIEW_OFFSET_X);

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

    add_items( u.inv.indexed_slice_filter_by( filter ), AT_END );

    if( u.is_armed() && filter( u.weapon ) ) {
        worn.push_back(itemstack_or_category(&weapon_cat));
        worn.push_back(itemstack_or_category(&u.weapon, -1));
    }
    auto iter = u.worn.begin();
    for( size_t i = 0, filtered = 0; i < u.worn.size(); ++i, ++iter ) {
        if( !filter( *iter ) ) {
            continue;
        }
        if( filtered++ == 0 ) {
            worn.push_back( itemstack_or_category( &worn_cat ) );
        }
        worn.push_back(itemstack_or_category(&*iter, player::worn_position_to_index(i)));
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
    if( empty() ) {
        return false;
    }
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
            pos = abs(position) - ((u.weapon.is_null()) ? 2 : 1);
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
        item& armor = u.i_at( it_pos );
        if( armor.is_null() ) {
            return; // invalid it_pos -> ignore
        }
        if (count > 0) {
            count = -1; // can only drop a whole worn item
        }
        set_drop_count(it_pos, count, armor);
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

int inventory_selector::execute_pick( const std::string &title, const int position )
{
    prepare_paging();
    select_item_by_position( position );

    while( true ) {
        display( title, SM_PICK );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto itemstack = invlet_to_itemstack( ch );

        if( itemstack != nullptr ) {
            return itemstack->item_pos;
        } else if ( handle_movement( action ) ) {
            continue;
        } else if ( action == "CONFIRM" || action == "RIGHT" ) {
            return get_selected_item_position();
        } else if ( action == "QUIT" ) {
            return INT_MIN;
        }
    }
}

item_location inventory_selector::execute_pick_map( const std::string &title, std::unordered_map<item *, item_location> &opts )
{
    prepare_paging();

    while( true ) {
        display( title, SM_PICK );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto itemstack = invlet_to_itemstack( ch );

        if( itemstack != nullptr ) {
            const auto it = opts.find( const_cast<item*>( itemstack->it ) );
            if( it != opts.end() ) {
                return std::move( it->second );
            }
            set_to_drop( itemstack->item_pos, 0 );
            return item_location( u, first_item );
        } else if( handle_movement( action ) ) {
            // continue with comparison below
        } else if( action == "QUIT" ) {
            return item_location();

        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            set_selected_to_drop( 0 );

            // Item in inventory
            if( get_selected_item_position() != INT_MIN ) {
                return item_location( u, first_item );
            }
            // Item on ground or in vehicle
            auto it = opts.find( first_item );
            if( it != opts.end() ) {
                return std::move( it->second );
            }
            return item_location();
        }
    }
}

void inventory_selector::execute_compare( const std::string &title )
{
    prepare_paging();

    inventory_selector::drop_map prev_droppings;
    while(true) {
        display( title, SM_COMPARE );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto itemstack = invlet_to_itemstack( ch );

        if( itemstack != nullptr ) {
            set_drop_count( itemstack->item_pos, 0, *itemstack->it );
        } else if( handle_movement( action ) ) {
            // continue with comparison below
        } else if( action == "QUIT" ) {
            break;
        } else if(action == "RIGHT") {
            set_selected_to_drop( 0 );
        }
        if (second_item != NULL) {
            std::vector<iteminfo> vItemLastCh, vItemCh;
            std::string sItemLastCh, sItemCh, sItemTn;
            first_item->info( true, vItemCh );
            sItemCh = first_item->tname();
            sItemTn = first_item->type_name();
            second_item->info(true, vItemLastCh);
            sItemLastCh = second_item->tname();

            int iScrollPos = 0;
            int iScrollPosLast = 0;
            int ch = ( int ) ' ';
            do {
                draw_item_info( 0, ( TERMX - VIEW_OFFSET_X * 2 ) / 2, 0, TERMY - VIEW_OFFSET_Y * 2,
                               sItemLastCh, sItemTn, vItemLastCh, vItemCh, iScrollPosLast, true ); //without getch()
                ch = draw_item_info( ( TERMX - VIEW_OFFSET_X * 2) / 2, (TERMX - VIEW_OFFSET_X * 2 ) / 2,
                                    0, TERMY - VIEW_OFFSET_Y * 2, sItemCh, sItemTn, vItemCh, vItemLastCh, iScrollPos );

                if( ch == KEY_PPAGE ) {
                    iScrollPos--;
                    iScrollPosLast--;
                } else if( ch == KEY_NPAGE ) {
                    iScrollPos++;
                    iScrollPosLast++;
                }
            } while ( ch == KEY_PPAGE || ch == KEY_NPAGE );

            dropping = prev_droppings;
            second_item = NULL;
        } else {
            prev_droppings = dropping;
        }
    }
}

std::list<std::pair<int, int>> inventory_selector::execute_multidrop( const std::string &title )
{
    prepare_paging();

    int count = 0;
    while( true ) {
        display( title, SM_MULTIDROP );

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        const auto itemstack = invlet_to_itemstack( ch );

        if( ch >= '0' && ch <= '9' ) {
            count = std::min( count, INT_MAX / 10 - 10 );
            count *= 10;
            count += ch - '0';
        } else if( itemstack != nullptr ) {
            set_to_drop( itemstack->item_pos, count );
            count = 0;
        } else if( handle_movement( action ) ) {
            count = 0;
            continue;
        } else if( action == "RIGHT" ) {
            set_selected_to_drop( count );
            count = 0;
        } else if( action == "CONFIRM" ) {
            break;
        } else if( action == "QUIT" ) {
            return std::list<std::pair<int, int> >();
        }
    }

    std::list<std::pair<int, int>> dropped_pos_and_qty;

    for( auto drop_pair : dropping ) {
        int num_to_drop = drop_pair.second;
        if( num_to_drop == -1 ) {
            num_to_drop = inventory::num_items_at_position( drop_pair.first );
        }
        dropped_pos_and_qty.push_back( std::make_pair( drop_pair.first, num_to_drop ) );
    }

    return dropped_pos_and_qty;
}

// Display current inventory.
int game::inv( const int position )
{
    u.inv.restack( &u );
    u.inv.sort();

    return inventory_selector( u ).execute_pick( _( "Inventory:" ), position );
}

int game::inv_for_filter( const std::string &title, item_filter filter, const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, filter );

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item." ) : none_message;
        popup( msg, PF_GET_KEY );
        return INT_MIN;
    }

    return inv_s.execute_pick( title );
}

int game::inv_for_all( const std::string &title, const std::string &none_message )
{
    const std::string msg = ( none_message.empty() ) ? _( "Your inventory is empty." ) : none_message;
    return inv_for_filter( title, allow_all_items, msg );
}

int game::inv_for_activatables( const player &p, const std::string &title )
{
    return inv_for_filter( title, [ &p ]( const item &it ) {
        return p.rate_action_use( it ) != HINT_CANT;
    }, _( "You don't have any items you can use." ) );
}

int game::inv_for_flag( const std::string &flag, const std::string &title )
{
    return inv_for_filter( title, [ &flag ]( const item &it ) {
        return it.has_flag( flag );
    } );
}

int game::inv_for_id( const itype_id &id, const std::string &title )
{
    return inv_for_filter( title, [ &id ]( const item &it ) {
        return it.type->id == id;
    }, string_format( _( "You don't have a %s." ), item::nname( id ).c_str() ) );
}

int game::inv_for_tools_powered_by( const ammotype &battery_id, const std::string &title )
{
    return inv_for_filter( title, [ &battery_id ]( const item & it ) {
        return it.is_tool() && it.ammo_type() == battery_id;
    }, string_format( _( "You don't have %s-powered tools." ), ammo_name( battery_id ).c_str() ) );
}

int game::inv_for_equipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item &it ) {
        return u.is_worn( it );
    }, _( "You don't wear anything." ) );
}

int game::inv_for_unequipped( const std::string &title )
{
    return inv_for_filter( title, [ this ]( const item &it ) {
        return it.is_armor() && !u.is_worn( it );
    }, _( "You don't have any items to wear." ) );
}

item_location game::inv_map_splice( item_filter filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_map_splice( filter, filter, filter, title, radius, none_message );
}

item_location game::inv_map_splice(
    item_filter inv_filter, item_filter ground_filter, item_filter vehicle_filter,
    const std::string &title, int radius, const std::string &none_message )
{
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, inv_filter );

    std::list<item_category> categories;
    int rank = -1000;

    // items are stacked per tile considering vehicle and map tiles separately
    // in the below loops identical items on the same tile are grouped into lists
    // each element of stacks represents one tile and is a vector of such lists
    std::vector<std::vector<std::list<item>>> stacks;

    // an indexed_invslice is created for each map or vehicle tile
    // each list of items created above for the tile will be added to it
    std::vector<indexed_invslice> slices;

    // inv_s.first_item will later contain the chosen item as a pointer to first item
    // of one of the above lists so use this as the key when storing the item location
    std::unordered_map<item *, item_location> opts;

    // the closest 10 items also have their location added to the invlets vector
    const char min_invlet = '0';
    const char max_invlet = '9';
    char cur_invlet = min_invlet;

    for( const auto &pos : closest_tripoints_first( radius, g->u.pos() ) ) {
        // second get all matching items on the map within radius
        if( m.accessible_items( g->u.pos(), pos, radius ) ) {
            auto items = m.i_at( pos );

            // create a new slice and stack for the current map tile
            stacks.emplace_back();
            slices.emplace_back();

            // reserve sufficient capacity to ensure reallocation is not required
            auto &current_stack = stacks.back();
            current_stack.reserve( items.size() );

            for( item &it : items ) {
                if( ground_filter( it ) ) {
                    auto match = std::find_if( current_stack.begin(),
                    current_stack.end(), [&]( const std::list<item> &e ) {
                        return it.stacks_with( e.back() );
                    } );
                    if( match != current_stack.end() ) {
                        match->push_back( it );
                    } else {
                        // item doesn't stack with any previous so start new list and append to current indexed_invslice
                        current_stack.emplace_back( 1, it );
                        slices.back().emplace_back( &current_stack.back(), INT_MIN );
                        opts.emplace( &current_stack.back().front(), item_location( pos, &it ) );

                        current_stack.back().front().invlet = ( cur_invlet <= max_invlet ) ? cur_invlet++ : 0;
                    }
                }
            }
            std::string name = trim( std::string( _( "GROUND" ) ) + " " + direction_suffix( g->u.pos(), pos ) );
            categories.emplace_back( name, name, rank-- );
            inv_s.add_items( slices.back(), inventory_selector::AT_END, &categories.back() );
        }

        // finally get all matching items in vehicle cargo spaces
        int part = -1;
        vehicle *veh = m.veh_at( pos, part );
        if( veh && part >= 0 ) {
            part = veh->part_with_feature( part, "CARGO" );
            if( part != -1 ) {
                auto items = veh->get_items( part );

                // create a new slice and stack for the current vehicle part
                stacks.emplace_back();
                slices.emplace_back();

                // reserve sufficient capacity to ensure reallocation is not required
                auto &current_stack = stacks.back();
                current_stack.reserve( items.size() );

                for( item &it : items ) {
                    if( vehicle_filter( it ) ) {
                        auto match = std::find_if( current_stack.begin(),
                        current_stack.end(), [&]( const std::list<item> &e ) {
                            return it.stacks_with( e.back() );
                        } );
                        if( match != current_stack.end() ) {
                            match->push_back( it );
                        } else {
                            // item doesn't stack with any previous so start new list and append to current indexed_invslice
                            current_stack.emplace_back( 1, it );
                            slices.back().emplace_back( &current_stack.back(), INT_MIN );
                            opts.emplace( &current_stack.back().front(), item_location( vehicle_cursor( *veh, part ), &it ) );

                            current_stack.back().front().invlet = ( cur_invlet <= max_invlet ) ? cur_invlet++ : 0;
                        }
                    }
                }
                std::string name = trim( std::string( _( "VEHICLE" ) )  + " " + direction_suffix( g->u.pos(), pos ) );
                categories.emplace_back( name, name, rank-- );
                inv_s.add_items( slices.back(), inventory_selector::AT_END, &categories.back() );
            }
        }
    }

    if( inv_s.empty() ) {
        const std::string msg = ( none_message.empty() ) ? _( "You don't have the necessary item at hand." ) : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }
    return inv_s.execute_pick_map( title, opts );
}

item *game::inv_map_for_liquid(const item &liquid, const std::string &title, int radius)
{
    auto sealable_filter = [&]( const item &candidate ) {
        return candidate.get_remaining_capacity_for_liquid( liquid, false ) > 0;
    };

    auto bucket_filter = [&]( const item &candidate ) {
        return candidate.get_remaining_capacity_for_liquid( liquid, true ) > 0;
    };

    // Buckets can only be filled when on the ground
    return inv_map_splice( sealable_filter, bucket_filter, sealable_filter, title, radius,
                           string_format( _( "You don't have a suitable container for carrying %s." ),
                           liquid.type_name( 1 ).c_str() ) ).get_item();
}

int inventory::num_items_at_position( int const position )
{
    if( position < -1 ) {
        const item& armor = g->u.i_at( position );
        return armor.count_by_charges() ? armor.charges : 1;
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
    u.inv.restack( &u );
    u.inv.sort();

    inventory_selector inv_s( u, [ this ]( const item &it ) -> bool {
        return u.can_unwield( it, false );
    } );
    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return std::list<std::pair<int, int> >();
    }
    return inv_s.execute_multidrop( _( "Multidrop:" ) );
}

void game::compare()
{
    tripoint dir;
    int &dirx = dir.x;
    int &diry = dir.y;

    if( choose_direction(_("Compare where?"), dirx, diry ) ) {
        refresh_all();
        compare( tripoint( dirx, diry, 0 ) );
    }
    refresh_all();
}

void game::compare( const tripoint &offset )
{
    const tripoint examp = u.pos() + offset;

    std::vector<std::list<item>> grounditems;
    indexed_invslice grounditems_slice;

    if( !m.has_flag( "SEALED", u.pos() ) ) {
        auto here = m.i_at( examp );
        //Filter out items with the same name (keep only one of them)
        std::set<std::string> dups;
        for (size_t i = 0; i < here.size(); i++) {
            if (dups.count(here[i].tname()) == 0) {
                grounditems.push_back(std::list<item>(1, here[i]));

                //Only the first 10 items get a invlet
                if ( grounditems.size() <= 10 ) {
                    // invlet: '0' ... '9'
                    grounditems.back().front().invlet = '0' + grounditems.size() - 1;
                }

                dups.insert(here[i].tname());
            }
        }
        for (size_t a = 0; a < grounditems.size(); a++) {
            // avoid INT_MIN, as it can be confused with "no item at all"
            grounditems_slice.push_back(indexed_invslice::value_type(&grounditems[a], INT_MIN + a + 1));
        }
    }
    static const item_category category_on_ground(
        "GROUND:",
        _("GROUND:"),
        -1000
    );

    u.inv.restack(&u);
    u.inv.sort();

    inventory_selector inv_s( u );

    inv_s.add_items( grounditems_slice, inventory_selector::AT_BEGINNING, &category_on_ground );
    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }
    inv_s.execute_compare( _( "Compare:" ) );
}
