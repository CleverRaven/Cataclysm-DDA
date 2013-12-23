#include "game.h"
#include "output.h"
#include "uistate.h"
#include "keypress.h"
#include "translations.h"
#include "item_factory.h"
#include "options.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

typedef std::vector<item_category> CategoriesVector;

std::vector<int> find_firsts(indexed_invslice &slice, CategoriesVector &CATEGORIES)
{
    static const item_category category_on_ground(
        "GROUND:",
        _("GROUND:"),
        -1000 // should be the first category
    );

    std::vector<int> firsts;
    CATEGORIES.clear();
    CATEGORIES.push_back(category_on_ground);
    for (int i = 0; i < slice.size(); i++) {
        item& it = slice[i].first->front();
        const item_category &category = it.get_category();
        if(std::find(CATEGORIES.begin(), CATEGORIES.end(), category) == CATEGORIES.end()) {
            CATEGORIES.push_back(category);
        }
    }
    for (int i = 0; i < (CATEGORIES.size()-1); i++) {
        firsts.push_back(-1);
    }

    for (int i = 0; i < slice.size(); i++) {
        item& it = slice[i].first->front();
        const item_category &category = it.get_category();
        for(size_t j = 0; j < firsts.size(); j++) {
            if(firsts[j] == -1 && CATEGORIES[j+1] == category) {
                firsts[j] = i;
                break;
            }
        }
    }

    return firsts;
}

void print_inv_weight_vol(WINDOW* w_inv, int weight_carried, int vol_carried)
{
    // Print weight
    mvwprintw(w_inv, 0, 32, _("Weight (%s): "),
              OPTIONS["USE_METRIC_WEIGHTS"].getValue() == "lbs" ? "lbs" : "kg");
    if (weight_carried >= g->u.weight_capacity())
    {
        wprintz(w_inv, c_red, "%6.1f", g->u.convert_weight(weight_carried));
    }
    else
    {
        wprintz(w_inv, c_ltgray, "%6.1f", g->u.convert_weight(weight_carried));
    }
    wprintz(w_inv, c_ltgray, "/%-6.1f", g->u.convert_weight(g->u.weight_capacity()));

    // Print volume
    mvwprintw(w_inv, 0, 61, _("Volume: "));
    if (vol_carried > g->u.volume_capacity() - 2)
    {
        wprintz(w_inv, c_red, "%3d", vol_carried);
    }
    else
    {
        wprintz(w_inv, c_ltgray, "%3d", vol_carried);
    }
    wprintw(w_inv, "/%-3d", g->u.volume_capacity() - 2);
}

// dropped_weapon==0 -> weapon is not dropped
// dropped_weapon==-1 -> weapon is dropped (whole stack)
// dropped_weapon>0 -> part of the weapon stack is dropped
void print_inv_statics(WINDOW* w_inv, std::string title,
                       std::vector<char> dropped_items, int dropped_weapon)
{
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());

 print_inv_weight_vol(w_inv, g->u.weight_carried(), g->u.volume_carried());

// Print our weapon
 int n_items = 0;
 mvwprintz(w_inv, 2, 45, c_magenta, _("WEAPON:"));
 if (g->u.is_armed()) {
  n_items++;
  if (dropped_weapon != 0)
   mvwprintz(w_inv, 3, 45, c_white, "%c %c %s", g->u.weapon.invlet,
             dropped_weapon == -1 ? '+' : '#',
             g->u.weapname().c_str());
  else
   mvwprintz(w_inv, 3, 45, g->u.weapon.color_in_inventory(), "%c - %s",
             g->u.weapon.invlet, g->u.weapname().c_str());
 } else
  mvwprintz(w_inv, 3, 45, c_ltgray, g->u.weapname().c_str());
// Print worn items
 if (g->u.worn.size() > 0)
  mvwprintz(w_inv, 5, 45, c_magenta, _("ITEMS WORN:"));
 for (int i = 0; i < g->u.worn.size(); i++) {
  n_items++;
  bool dropped_armor = false;
  for (int j = 0; j < dropped_items.size() && !dropped_armor; j++) {
   if (dropped_items[j] == g->u.worn[i].invlet)
    dropped_armor = true;
  }
  if (dropped_armor)
   mvwprintz(w_inv, 6 + i, 45, c_white, "%c + %s", g->u.worn[i].invlet,
             g->u.worn[i].tname().c_str());
  else
   mvwprintz(w_inv, 6 + i, 45, c_ltgray, "%c - %s", g->u.worn[i].invlet,
             g->u.worn[i].tname().c_str());
 }

 // Print items carried
 for (std::string::const_iterator invlet = inv_chars.begin();
      invlet != inv_chars.end(); ++invlet) {
   n_items += ((g->u.inv.item_by_letter(*invlet).is_null()) ? 0 : 1);
 }
 mvwprintw(w_inv, 1, 61, _("Hotkeys:  %d/%d "), n_items, inv_chars.size());
}

int game::display_slice(indexed_invslice& slice, const std::string& title)
{
    WINDOW* w_inv = newwin( TERRAIN_WINDOW_HEIGHT,
                            TERRAIN_WINDOW_WIDTH + (use_narrow_sidebar() ? 45 : 55),
                            VIEW_OFFSET_Y, VIEW_OFFSET_X );
    const int maxitems = TERRAIN_WINDOW_HEIGHT - 5;

    int ch = (int)'.';
    int start = 0, cur_it = 0, max_it;
    std::vector<char> null_vector;
    print_inv_statics(w_inv, title, null_vector, 0);
    // Gun, ammo, weapon, armor, food, tool, book, other

    CategoriesVector CATEGORIES;
    std::vector<int> firsts = find_firsts(slice, CATEGORIES);

    int selected =- 1;
    int selected_pos = INT_MIN;

    int next_category_at = 0;
    int prev_category_at = 0;
    bool inCategoryMode = false;

    std::vector<int> category_order;
    category_order.reserve(firsts.size());

    // Items are not guaranteed to be in the same order as their categories, in fact they almost never are.
    // So we sort the categories by which items actually show up first in the inventory.
    for (int current_item = 0; current_item < slice.size(); ++current_item) {
        for (int i = 1; i < CATEGORIES.size(); ++i) {
            if (current_item == firsts[i - 1]) {
                category_order.push_back(i - 1);
            }
        }
    }

 do {
  if (( ch == '<' || ch == KEY_PPAGE ) && start > 0) { // Clear lines and shift
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
   if ( selected > -1 ) selected = start; // oy, the cheese
  }
  if (( ch == '>' || ch == KEY_NPAGE ) && cur_it < slice.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   if ( selected < start && selected > -1 ) selected = start;
  }
  int cur_line = 2;
  max_it = 0;

  // Find the inventory position of the first item in the previous and next category (in relation
  // to the currently selected category).
  for (int i = 0; i < category_order.size(); ++i)
  {
	  if (selected > firsts[category_order[i]] && prev_category_at <= firsts[category_order[i]])
	  {
		  prev_category_at = firsts[category_order[i]];
	  }

	  if (selected < firsts[category_order[i]] && next_category_at <= selected)
	  {
		  next_category_at = firsts[category_order[i]];
	  }
  }

  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");

   for (int i = 1; i < CATEGORIES.size(); i++) {
    if (cur_it == firsts[i-1]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].name.c_str());
     cur_line++;
    }
   }

   if (cur_it < slice.size())
   {
       item& it = slice[cur_it].first->front();
       if(cur_it == selected) {
           selected_pos=slice[cur_it].second;
       }
       nc_color selected_line_color = inCategoryMode ? c_white_red : h_white;
       mvwputch(w_inv, cur_line, 0, (cur_it == selected ? selected_line_color : c_white), it.invlet);
       mvwprintz(w_inv, cur_line, 1, (cur_it == selected ? selected_line_color : it.color_in_inventory()), " %s",
                 it.display_name().c_str());
       if (slice[cur_it].first->size() > 1) {
           wprintw(w_inv, " x %d", slice[cur_it].first->size());
       }
       cur_line++;
       max_it = cur_it;
   }
  }

  if (inCategoryMode)
  {
	  mvwprintz(w_inv, maxitems + 4, 32, c_white_red, _("In category select mode! Press [TAB] to enter item select mode."));
  }
  else
  {
	  mvwprintz(w_inv, maxitems + 4, 32, h_white, _("In item select mode! Press [TAB] to enter category select mode."));
  }

  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, _("< Go Back"));
  if (cur_it < slice.size())
   mvwprintw(w_inv, maxitems + 4, 12, _("> More items"));
  wrefresh(w_inv);

  input_context ctxt("INVENTORY");
  ctxt.register_action("ANY_INPUT");
  ctxt.handle_input();
  ch = ctxt.get_raw_input().get_first_input();

  if (ch == '\t')
  {
	  inCategoryMode = !inCategoryMode;
  }
  else if ( ch == KEY_DOWN ) {
    if ( selected < 0 ) {
      selected = start;
    } else {
		if (inCategoryMode)
		{
			selected < firsts[category_order[category_order.size() - 1]] ? selected = next_category_at : 0;
		}
		else
		{
			selected++;
		}
		
		next_category_at = prev_category_at = 0;
    }

    if ( selected > max_it ) {
      if( cur_it < u.inv.size() ) {
        ch='>';
      } else {
        selected = u.inv.size() - 1; // wraparound?
      }
    }
  } else if ( ch == KEY_UP ) {
	inCategoryMode ? selected = prev_category_at : selected--;
	next_category_at = prev_category_at = 0;

    if ( selected < -1 ) {
      selected = -1; // wraparound?
    } else if ( selected < start ) {
      if ( start > 0 ) {
        for (int i = 1; i < maxitems+4; i++)
         mvwprintz(w_inv, i, 0, c_black, "                                             ");
        start -= maxitems;
        if (start < 0)
         start = 0;
        mvwprintw(w_inv, maxitems + 4, 0, "         ");
      }
    }
  } else if ( ch == '\n' || ch == KEY_RIGHT ) {
    ch = '\n';
  }

 } while (ch == '<' || ch == '>' || ch == '\t' || ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_UP || ch == KEY_DOWN );
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
 if (ch == '\n') {  // user hit enter (or equivalent).
  return selected_pos;
 } else {  // user hit an invlet (or exited out, which will resolve to INT_MIN)
   return u.invlet_to_position((char)ch);
 }
}

// Display current inventory.
int game::inv(const std::string& title)
{
 u.inv.restack(&u);
 u.inv.sort();
 indexed_invslice slice = u.inv.slice_filter();
 return display_slice(slice ,title);
}

int game::inv_activatable(std::string title)
{
 u.inv.restack(&u);
 u.inv.sort();
 indexed_invslice activatables = u.inv.slice_filter_by_activation(u);
 return display_slice(activatables,title);
}

int game::inv_type(std::string title, item_cat inv_item_type)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_category(inv_item_type, u);
    return display_slice(reduced_inv,title);
}

int game::inv_for_liquid(const item &liquid, const std::string title, bool auto_choose_single)
{
    u.inv.restack(&u);
    u.inv.sort();
    indexed_invslice reduced_inv = u.inv.slice_filter_by_capacity_for_liquid(liquid);
    if (auto_choose_single && reduced_inv.size() == 1) {
        std::list<item>* cont_stack = reduced_inv[0].first;
        if (cont_stack->size() > 0) {
            return reduced_inv[0].second;
        }
    }
    return display_slice(reduced_inv, title);
}

std::vector<item> game::multidrop()
{
    WINDOW* w_inv = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH + (use_narrow_sidebar() ? 45 : 55), VIEW_OFFSET_Y, VIEW_OFFSET_X);
    const int maxitems = TERRAIN_WINDOW_HEIGHT - 5;

    u.inv.restack(&u);
    u.inv.sort();
    int drp_line_width=getmaxx(w_inv)-90;
    const std::string drp_line_padding = ( drp_line_width > 1 ? std::string(drp_line_width, ' ') : " ");
    std::map<int, int> dropping; // Count of how many we'll drop from each position
    int count = 0; // The current count
    std::vector<char> dropped_armor; // Always single, not counted
    int dropped_weapon = 0;
    bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
    print_inv_statics(w_inv, _("Multidrop:"), dropped_armor, dropped_weapon);
    int base_weight = u.weight_carried();
    int base_volume = u.volume_carried();

    int ch = (int)'.';
    int start = 0, cur_it = 0, max_it;
    indexed_invslice stacks = u.inv.slice_filter();
    CategoriesVector CATEGORIES;
    std::vector<int> firsts = find_firsts(stacks, CATEGORIES);
    int selected = -1;
    int selected_pos = INT_MIN;

    int next_category_at = 0;
    int prev_category_at = 0;
    bool inCategoryMode = false;

    std::vector<int> category_order;
    category_order.reserve(firsts.size());

    // Items are not guaranteed to be in the same order as their categories, in fact they almost never are.
    // So we sort the categories by which items actually show up first in the inventory.
    for (int current_item = 0; current_item < u.inv.size(); ++current_item) {
        for (int i = 1; i < CATEGORIES.size(); ++i) {
            if (current_item == firsts[i - 1]) {
                category_order.push_back(i - 1);
            }
        }
    }

    do {
        // Find the inventory position of the first item in the previous and next category (in relation
        // to the currently selected category).
        for (int i = 0; i < category_order.size(); ++i) {
            if (selected > firsts[category_order[i]] && prev_category_at <= firsts[category_order[i]]) {
                prev_category_at = firsts[category_order[i]];
            }

            if (selected < firsts[category_order[i]] && next_category_at <= selected) {
                next_category_at = firsts[category_order[i]];
            }
        }

        inventory drop_subset = u.inv.subset(dropping);
        if (dropped_weapon == -1) {
            drop_subset.add_item(u.weapon, false, false);
        } else if (dropped_weapon > 0) {
            item &tmp = drop_subset.add_item(u.weapon, false, false);
            tmp.charges = dropped_weapon;
        }
        int new_weight = base_weight - drop_subset.weight();
        int new_volume = base_volume - drop_subset.volume();
        for (int i = 0; i < dropped_armor.size(); ++i) {
            new_weight -= u.i_at(dropped_armor[i]).weight();
        }
        print_inv_weight_vol(w_inv, new_weight, new_volume);
        int cur_line = 2;
        max_it = 0;
        int drp_line = 1;
        // Print weapon to be dropped, the first position is reserved for high visibility
        mvwprintw(w_inv, 0, 90, "%s", drp_line_padding.c_str());
        if (dropped_weapon != 0) {
            const char disp_char = (dropped_weapon == -1) ? '+' : '#';
            mvwprintz(w_inv, 0, 90, c_ltblue, "%c %c %s", u.weapon.invlet, disp_char, u.weapname().c_str());
            mvwprintw(w_inv, drp_line, 90, "%s", drp_line_padding.c_str());
            drp_line++;
        }
        // Print worn items to be dropped
        bool dropping_a = false;
        if (u.worn.size() > 0){
            for (int k = 0; k < u.worn.size(); k++) {
                bool dropping_w = false;
                for (int j = 0; j < dropped_armor.size() && !dropping_w; j++) {
                    if (dropped_armor[j] == u.worn[k].invlet) {
                        dropping_w = true;
                        dropping_a = true;
                        mvwprintw(w_inv, drp_line, 90, "%s", drp_line_padding.c_str());
                        mvwprintz(w_inv, drp_line, 90, c_cyan, "%c + %s", u.worn[k].invlet, u.worn[k].tname().c_str());
                        drp_line++;
                    }
                }
            }
        }
        if(dropping_a) {
            mvwprintw(w_inv, drp_line, 90, "%s", drp_line_padding.c_str());
            drp_line++;
        }
        for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
            // Clear the current line;
            mvwprintw(w_inv, cur_line, 0, "                                             ");
            mvwprintw(w_inv, drp_line, 90, "%s", drp_line_padding.c_str());
            mvwprintw(w_inv, drp_line + 1, 90, "%s", drp_line_padding.c_str());
            // Print category header
            for (int i = 1; i < CATEGORIES.size(); i++) {
                if (cur_it == firsts[i-1]) {
                    mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].name.c_str());
                    cur_line++;
                }
            }

            if ( selected < start && selected > -1 ) selected = start;

            if (cur_it < stacks.size()) {
                item& it = stacks[cur_it].first->front();
                if( cur_it == selected) {
                    selected_pos = stacks[cur_it].second;
                }
                nc_color selected_line_color = inCategoryMode ? c_white_red : h_white;
                mvwputch (w_inv, cur_line, 0, (cur_it == selected ? selected_line_color : c_white), it.invlet);
                char icon = '-';
                if (dropping[cur_it] >= (it.count_by_charges() ? it.charges : stacks[cur_it].first->size())) {
                    icon = '+';
                } else if (dropping[cur_it] > 0) {
                    icon = '#';
                }
                nc_color col = ( cur_it == selected ? selected_line_color : (dropping[cur_it] == 0 ? c_ltgray : c_white ) );
                mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
                          it.tname().c_str());
                if (stacks[cur_it].first->size() > 1) {
                    wprintz(w_inv, col, " x %d", stacks[cur_it].first->size());
                }
                if (it.charges > 0) {
                    wprintz(w_inv, col, " (%d)", it.charges);
                } else if (it.contents.size() == 1 &&
                           it.contents[0].charges > 0) {
                    wprintw(w_inv, " (%d)", it.contents[0].charges);
                }
                if (icon=='+'||icon=='#') {
                    mvwprintz(w_inv, drp_line, 90, col, "%c %c %s", it.invlet, icon, it.tname().c_str());
                    if (icon=='+') {
                        if (stacks[cur_it].first->size() > 1) {
                            wprintz(w_inv, col, " x %d", stacks[cur_it].first->size());
                        }
                        if (it.charges > 0) {
                            wprintz(w_inv, col, " (%d)", it.charges);
                        }
                    }
                    if (icon=='#') {
                        wprintz(w_inv, col, " {%d}", dropping[cur_it]);
                    }
                    drp_line++;
                }
            }
            cur_line++;
            max_it=cur_it;
        }

        if (inCategoryMode) {
            mvwprintz(w_inv, maxitems + 4, 32, c_white_red, _("In category select mode! Press [TAB] to enter item select mode."));
        } else {
            mvwprintz(w_inv, maxitems + 4, 32, h_white, _("In item select mode! Press [TAB] to enter category select mode."));
        }

        if (start > 0) {
            mvwprintw(w_inv, maxitems + 4, 0, _("< Go Back"));
        }
        if (cur_it < u.inv.size()) {
            mvwprintw(w_inv, maxitems + 4, 12, _("> More items"));
        }
        wrefresh(w_inv);
/* back to (int)getch() as input() mangles arrow keys
  ch = input();
*/
        ch = getch();

        if (ch == '\t') {
            inCategoryMode = !inCategoryMode;
        }
        else if ( ch == '<' || ch == KEY_PPAGE ) {
            if( start > 0) {
                for (int i = 1; i < maxitems+4; i++)
                    mvwprintz(w_inv, i, 0, c_black, "                                             ");
                start -= maxitems;
                if (start < 0)
                    start = 0;
                mvwprintw(w_inv, maxitems + 4, 0, "         ");
                if ( selected > -1 ) selected = start; // oy, the cheese
            }
        } else if ( ch == '>' || ch == KEY_NPAGE ) {
            if ( cur_it < u.inv.size()) {
                start = cur_it;
                mvwprintw(w_inv, maxitems + 4, 12, "            ");
                for (int i = 1; i < maxitems+4; i++)
                    mvwprintz(w_inv, i, 0, c_black, "                                             ");
                if ( selected < start && selected > -1 ) selected = start;
            }
        } else if ( ch == KEY_DOWN ) {
            if ( selected < 0 ) {
                selected = start;
            } else {
                if (inCategoryMode) {
                    selected < firsts[category_order[category_order.size() - 1]] ? selected = next_category_at : 0;
                } else {
                    selected++;
                }

                next_category_at = prev_category_at = 0;
            }

            if ( selected > max_it ) {
                if( cur_it < u.inv.size() ) {
                    start = cur_it;
                    mvwprintw(w_inv, maxitems + 4, 12, "            ");
                    for (int i = 1; i < maxitems+4; i++)
                        mvwprintz(w_inv, i, 0, c_black, "                                             ");
                } else {
                    selected = u.inv.size() - 1; // wraparound?
                }
            }
        } else if ( ch == KEY_UP ) {
            inCategoryMode ? selected = prev_category_at : selected--;
            next_category_at = prev_category_at = 0;

            if ( selected < -1 ) {
                selected = -1; // wraparound?
            } else if ( selected < start ) {
                if ( start > 0 ) {
                    for (int i = 1; i < maxitems+4; i++)
                        mvwprintz(w_inv, i, 0, c_black, "                                             ");
                    start -= maxitems;
                    if (start < 0)
                        start = 0;
                    mvwprintw(w_inv, maxitems + 4, 0, "         ");
                }
            }
        } else if (ch >= '0'&& ch <= '9') {
            ch = (char)ch - '0';
            count *= 10;
            count += ch;
        } else { // todo: reformat and maybe rewrite
            item* it;
            int it_pos;
            if ( ch == '\t' || ch == KEY_RIGHT || ch == KEY_LEFT ) {
                it_pos = selected_pos;
                it = &u.inv.find_item(it_pos);
            } else {
                it = &u.inv.item_by_letter((char)ch);
                it_pos = u.inv.position_by_item(it);
            }
            if (it == 0 || it->is_null()) { // Not from inventory
                int found = false;
                for (int i = 0; i < dropped_armor.size() && !found; i++) {
                    if (dropped_armor[i] == ch) {
                        dropped_armor.erase(dropped_armor.begin() + i);
                        found = true;
                        print_inv_statics(w_inv, _("Multidrop:"), dropped_armor, dropped_weapon);
                    }
                }
                if (!found && ch == u.weapon.invlet && !u.weapon.is_null()) {
                    if (std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(), u.weapon.type->id) != unreal_itype_ids.end()) {
                        if (!warned_about_bionic) {
                            add_msg(_("You cannot drop your %s."), u.weapon.tname().c_str());
                            warned_about_bionic = true;
                        }
                    } else {
                        // user selected weapon, which is a normal item
                        if (count == 0) {
                            // No count given, invert the selection status: drop all <-> drop none
                            if (dropped_weapon == 0) {
                                dropped_weapon = -1;
                            } else {
                                dropped_weapon = 0;
                            }
                        } else if (u.weapon.count_by_charges() && count < u.weapon.charges) {
                            // can drop part of weapon and count is valid for this
                            dropped_weapon = count;
                        } else {
                            dropped_weapon = -1;
                        }
                        count = 0;
                        print_inv_statics(w_inv, _("Multidrop:"), dropped_armor, dropped_weapon);
                    }
                }
            } else {
                int index = -1;
                for (int i = 0; i < stacks.size(); ++i) {
                    if (stacks[i].first->front().invlet == it->invlet) {
                        index = i;
                        break;
                    }
                }
                if (index == -1) {
                    debugmsg("Inventory got out of sync with inventory slice?");
                }
                if (count == 0) {
                    if (it->count_by_charges()) {
                        if (dropping[it_pos] == 0) {
                            dropping[it_pos] = -1;
                        } else {
                            dropping[it_pos] = 0;
                        }
                    } else {
                        if (dropping[it_pos] == 0) {
                            dropping[it_pos] = stacks[index].first->size();
                        } else {
                            dropping[it_pos] = 0;
                        }
                    }
                } else if (count >= stacks[index].first->size() && !it->count_by_charges()) {
                    dropping[it_pos] = stacks[index].first->size();
                } else {
                    dropping[it_pos] = count;
                }

                count = 0;
            }
        }
    } while (ch != '\n' && ch != KEY_ESCAPE && ch != ' ');
    werase(w_inv);
    delwin(w_inv);
    erase();
    refresh_all();

    std::vector<item> ret;

    if (ch != '\n')
        return ret; // Canceled!

    // We iterate backwards because deletion will invalidate later indices.
    for (std::map<int,int>::reverse_iterator it = dropping.rbegin(); it != dropping.rend(); ++it) {
        if (it->second == -1)
            ret.push_back( u.inv.remove_item( it->first));
        else if (it->second && u.inv.find_item( it->first).count_by_charges()) {
            int charges = u.inv.find_item( it->first).charges;// >= it->second ? : it->second;
            ret.push_back( u.inv.reduce_charges( it->first, it->second > charges ? charges : it->second));
        } else if (it->second)
            for (int j = it->second; j > 0; j--)
                ret.push_back( u.inv.remove_item( it->first));
    }
    if (dropped_weapon == -1) {
        ret.push_back(u.remove_weapon());
    } else if (dropped_weapon > 0) {
        ret.push_back(u.weapon);
        u.weapon.charges -= dropped_weapon;
        ret.back().charges = dropped_weapon;
    }

    for (int i = 0; i < dropped_armor.size(); i++)
    {
        int wornpos = u.invlet_to_position(dropped_armor[i]);
        if (wornpos == INT_MIN || !u.takeoff(wornpos, true))
        {
            continue;
        }

        // Item could have been dropped after taking it off
        if (&u.inv.item_by_letter(dropped_armor[i]) != &u.inv.nullitem)
        {
            ret.push_back(u.i_rem(dropped_armor[i]));
        }
    }

    return ret;
}

void game::compare(int iCompareX, int iCompareY)
{
 int examx, examy;
 std::vector <item> grounditems;
 int ch = (int)'.';

 if (iCompareX != -999 && iCompareY != -999) {
  examx = u.posx + iCompareX;
  examy = u.posy + iCompareY;
 } else if (!choose_adjacent(_("Compare where?"),examx,examy)) {
     return;
 }

 std::vector <item> here = m.i_at(examx, examy);
 //Filter out items with the same name (keep only one of them)
 std::map <std::string, bool> dups;
 for (int i = 0; i < here.size(); i++) {
  if (!dups[here[i].tname().c_str()]) {
   grounditems.push_back(here[i]);
   dups[here[i].tname().c_str()] = true;
  }
 }
 //Only the first 10 Items due to numbering 0-9
 const int groundsize = (grounditems.size() > 10 ? 10 : grounditems.size());
 u.inv.restack(&u);
 u.inv.sort();

 indexed_invslice stacks = u.inv.slice_filter();

 WINDOW* w_inv = newwin(TERMY-VIEW_OFFSET_Y*2, TERMX-VIEW_OFFSET_X*2, VIEW_OFFSET_Y, VIEW_OFFSET_X);
 int maxitems = TERMY-5-VIEW_OFFSET_Y*2;    // Number of items to show at one time.
 std::vector<int> compare_list; // Count of how many we'll drop from each stack
 bool bFirst = false; // First Item selected
 bool bShowCompare = false;
 char cLastCh = 0;
 compare_list.resize(u.inv.size() + groundsize, 0);
 std::vector<char> dropped_armor; // Always single, not counted
 int dropped_weapon = 0;
 print_inv_statics(w_inv, "Compare:", dropped_armor, dropped_weapon);
// Gun, ammo, weapon, armor, food, tool, book, other
 CategoriesVector CATEGORIES;
 std::vector<int> first = find_firsts(stacks, CATEGORIES);
 std::vector<int> firsts;
 if (groundsize > 0) {
  firsts.push_back(0);
 }
 for (int i = 0; i < first.size(); i++) {
  firsts.push_back((first[i] >= 0) ? first[i]+groundsize : -1);
 }
 ch = '.';
 int start = 0, cur_it = 0;
 do {
  if (( ch == '<' || ch == KEY_PPAGE ) && start > 0) {
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
  }
  if (( ch == '>' || ch == KEY_NPAGE ) && cur_it < u.inv.size() + groundsize) {
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
  }
  int cur_line = 2;
  int iHeaderOffset = (groundsize > 0) ? 0 : 1;

  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");
// Print category header
   for (int i = iHeaderOffset; i < CATEGORIES.size(); i++) {
    if (cur_it == firsts[i-iHeaderOffset]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].name.c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size() + groundsize) {
    char icon = '-';
    if (compare_list[cur_it] == 1)
     icon = '+';
    if (cur_it < groundsize) {
     mvwputch (w_inv, cur_line, 0, c_white, '1'+((cur_it<9) ? cur_it: -1));
     nc_color col = (compare_list[cur_it] == 0 ? c_ltgray : c_white);
     mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
               grounditems[cur_it].tname().c_str());
    } else {
     item& it = stacks[cur_it-groundsize].first->front();
     mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
     nc_color col = (compare_list[cur_it] == 0 ? c_ltgray : c_white);
     mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
               it.tname().c_str());
     if (stacks[cur_it-groundsize].first->size() > 1)
      wprintz(w_inv, col, " x %d", stacks[cur_it-groundsize].first->size());
     if (it.charges > 0)
      wprintz(w_inv, col, " (%d)", it.charges);
     else if (it.contents.size() == 1 &&
              it.contents[0].charges > 0)
      wprintw(w_inv, " (%d)", it.contents[0].charges);
    }
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, _("< Go Back"));
  if (cur_it < u.inv.size() + groundsize)
   mvwprintw(w_inv, maxitems + 4, 12, _("> More items"));
  wrefresh(w_inv);
  ch = getch();
  if (u.has_item((char)ch)) {
   item& it = u.inv.item_by_letter((char)ch);
   if (it.is_null()) { // Not from inventory
    bool found = false;
    for (int i = 0; i < dropped_armor.size() && !found; i++) {
     if (dropped_armor[i] == ch) {
      dropped_armor.erase(dropped_armor.begin() + i);
      found = true;
      bFirst = false;
      print_inv_statics(w_inv, "Compare:", dropped_armor, dropped_weapon);
     }
    }
    if (!found && dropped_weapon == -1 && ch == u.weapon.invlet) {
     found = true;
     bFirst = false;
     dropped_weapon = 0;
     print_inv_statics(w_inv, "Compare:", dropped_armor, dropped_weapon);
    }
    if (!found) {

     if ( ch == u.weapon.invlet &&
          std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(), u.weapon.type->id) != unreal_itype_ids.end()){
      //Do Bionic stuff here?!
     } else {
      if (!bFirst)
      {
       dropped_weapon = -1;
       print_inv_statics(w_inv, "Compare:", dropped_armor, dropped_weapon);
       bFirst = true;
       cLastCh = ch;
      } else {
       bShowCompare = true;
      }
     }
    }
   } else {
    int index = -1;
    for (int i = 0; i < stacks.size(); ++i) {
     if (stacks[i].first->front().invlet == it.invlet) {
      index = i;
      break;
     }
    }
    if (index == -1) {
     debugmsg("Inventory got out of sync with inventory slice?");
    }
    if (compare_list[index+groundsize] == 1)
    {
     compare_list[index+groundsize] = 0;
     bFirst = false;
    } else {
     if (!bFirst)
     {
      compare_list[index+groundsize] = 1;
      bFirst = true;
      cLastCh = ch;
     } else {
      bShowCompare = true;
     }
    }
   }
  } else if ((ch >= '1' && ch <= '9' && ch-'1' < groundsize) || (ch == '0' && groundsize == 10)) {
   //Ground Items
   int iZero = 0;
   if (ch == '0') {
    iZero = 10;
   }
   if (compare_list[ch-'1'+iZero] == 1)
   {
    compare_list[ch-'1'+iZero] = 0;
    bFirst = false;
   } else {
    if (!bFirst)
    {
     compare_list[ch-'1'+iZero] = 1;
     bFirst = true;
     cLastCh = ch;
    } else {
     bShowCompare = true;
    }
   }
  }
  if (bShowCompare) {
   std::vector<iteminfo> vItemLastCh, vItemCh;
   std::string sItemLastCh, sItemCh;
   if (cLastCh >= '0' && cLastCh <= '9') {
    int iZero = 0;
    if (cLastCh == '0') {
     iZero = 10;
    }

    grounditems[cLastCh-'1'+iZero].info(true, &vItemLastCh);
    sItemLastCh = grounditems[cLastCh-'1'+iZero].tname();
   } else {
    u.i_at(cLastCh).info(true, &vItemLastCh);
    sItemLastCh = u.i_at(cLastCh).tname();
   }

   if (ch >= '0' && ch <= '9') {
    int iZero = 0;
    if (ch == '0') {
     iZero = 10;
    }

    grounditems[ch-'1'+iZero].info(true, &vItemCh);
    sItemCh = grounditems[ch-'1'+iZero].tname();
   } else {
    item& item = u.i_at((char)ch);
    item.info(true, &vItemCh);
    sItemCh = item.tname();
   }

   compare_split_screen_popup(0, (TERMX-VIEW_OFFSET_X*2)/2, TERMY-VIEW_OFFSET_Y*2, sItemLastCh, vItemLastCh, vItemCh
                              , -1, true);//without getch()
   compare_split_screen_popup((TERMX-VIEW_OFFSET_X*2)/2, (TERMX-VIEW_OFFSET_X*2)/2, TERMY-VIEW_OFFSET_Y*2, sItemCh, vItemCh, vItemLastCh);

   wclear(w_inv);
   print_inv_statics(w_inv, "Compare:", dropped_armor, dropped_weapon);
   bShowCompare = false;
  }
 } while (ch != '\n' && ch != KEY_ESCAPE && ch != ' ');
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
}
