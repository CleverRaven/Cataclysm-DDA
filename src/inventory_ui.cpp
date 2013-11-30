#include "game.h"
#include "output.h"
#include "uistate.h"
#include "keypress.h"
#include "translations.h"
#include "options.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

std::vector<std::string> CATEGORIES;

void init_inventory_categories()
{
    CATEGORIES.push_back(_("GROUND:"));
    CATEGORIES.push_back(_("FIREARMS:"));
    CATEGORIES.push_back(_("AMMUNITION:"));
    CATEGORIES.push_back(_("CLOTHING:"));
    CATEGORIES.push_back(_("FOOD/DRINKS:"));
    CATEGORIES.push_back(_("TOOLS:"));
    CATEGORIES.push_back(_("BOOKS:"));
    CATEGORIES.push_back(_("WEAPONS:"));
    CATEGORIES.push_back(_("MODS/BIONICS:"));
    CATEGORIES.push_back(_("MEDICINE/DRUGS:"));
    CATEGORIES.push_back(_("OTHER:"));
}

//TODO: make this function not have issues with items that are classed as multiple things
std::vector<int> find_firsts(invslice &slice)
{
    std::vector<int> firsts;
    for (int i = 0; i < (CATEGORIES.size()-1); i++) {
        firsts.push_back(-1);
    }

    for (int i = 0; i < slice.size(); i++) {
        item& it = slice[i]->front();
        if (firsts[0] == -1 && it.is_gun()) {
            firsts[0] = i;
        } else if (firsts[1] == -1 && it.is_ammo()) {
            firsts[1] = i;
        } else if (firsts[2] == -1 && it.is_armor()) {
            firsts[2] = i;
        } else if (firsts[3] == -1 && it.is_food_container()) {
            firsts[3] = i;
        } else if (it.is_food()) {
            it_comest* comest = dynamic_cast<it_comest*>(it.type);
            if (firsts[3] == -1 && comest->comesttype != "MED") {
                firsts[3] = i;
            } else if (firsts[8] == -1 && comest->comesttype == "MED") {
                firsts[8] = i;
            }
        } else if (firsts[4] == -1 && it.is_tool()) {
            firsts[4] = i;
        } else if (firsts[5] == -1 && it.is_book()) {
            firsts[5] = i;
        } else if (firsts[6] == -1 && it.is_weap()) {
            firsts[6] = i;
        } else if (firsts[7] == -1 && (it.is_gunmod() || it.is_bionic())) {
            firsts[7] = i;
        } else if (firsts[9] == -1 && it.is_other()) {
            firsts[9] = i;
        }
    }

    return firsts;
}

void print_inv_weight_vol(game *g, WINDOW* w_inv, int weight_carried, int vol_carried)
{
    // Print weight
    mvwprintw(w_inv, 0, 39, _("Weight: "));
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

void print_inv_statics(game *g, WINDOW* w_inv, std::string title,
                       std::vector<char> dropped_items)
{
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());

 print_inv_weight_vol(g, w_inv, g->u.weight_carried(), g->u.volume_carried());

// Print our weapon
 int n_items = 0;
 mvwprintz(w_inv, 2, 45, c_magenta, _("WEAPON:"));
 int dropping_weapon = false;
 for (int i = 0; i < dropped_items.size() && !dropping_weapon; i++) {
  if (dropped_items[i] == g->u.weapon.invlet)
   dropping_weapon = true;
 }
 if (g->u.is_armed()) {
  n_items++;
  if (dropping_weapon)
   mvwprintz(w_inv, 3, 45, c_white, "%c + %s", g->u.weapon.invlet,
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
  bool dropping_armor = false;
  for (int j = 0; j < dropped_items.size() && !dropping_armor; j++) {
   if (dropped_items[j] == g->u.worn[i].invlet)
    dropping_armor = true;
  }
  if (dropping_armor)
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
 mvwprintw(w_inv, 1, 62, _("Items:  %d/%d "), n_items, inv_chars.size());
}

char game::inv(inventory& inv, std::string title)
{
    WINDOW* w_inv = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH + (use_narrow_sidebar() ? 45 : 55), VIEW_OFFSET_Y, VIEW_OFFSET_X);
    const int maxitems = TERRAIN_WINDOW_HEIGHT - 5;

 int ch = (int)'.';
 int start = 0, cur_it = 0, max_it;
 inv.sort();
 std::vector<char> null_vector;
 print_inv_statics(this, w_inv, title, null_vector);
// Gun, ammo, weapon, armor, food, tool, book, other

 invslice slice = inv.slice(0, inv.size());
 std::vector<int> firsts = find_firsts(slice);

 int selected =- 1;
 int selected_char = (int)' ';
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
  if (( ch == '>' || ch == KEY_NPAGE ) && cur_it < inv.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   if ( selected < start && selected > -1 ) selected = start;
  }
  int cur_line = 2;
  max_it = 0;
  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");

   for (int i = 1; i < CATEGORIES.size(); i++) {
    if (cur_it == firsts[i-1]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }

   if (cur_it < slice.size())
   {
    item& it = slice[cur_it]->front();
    if(cur_it==selected) selected_char=(int)it.invlet;
    mvwputch (w_inv, cur_line, 0, (cur_it == selected ? h_white : c_white), it.invlet);
    mvwprintz(w_inv, cur_line, 1, (cur_it == selected ? h_white : it.color_in_inventory() ), " %s",
              it.tname().c_str());
    if (slice[cur_it]->size() > 1) {
     wprintw(w_inv, " x %d", slice[cur_it]->size());
    }
    cur_line++;
    max_it=cur_it;
   }

  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, _("< Go Back"));
  if (cur_it < inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, _("> More items"));
  wrefresh(w_inv);

  input_context ctxt("INVENTORY");
  ctxt.register_action("ANY_INPUT");
  ctxt.handle_input();
  ch = ctxt.get_raw_input().get_first_input();

  if ( ch == KEY_DOWN ) {
    if ( selected < 0 ) {
      selected = start;
    } else {
      selected++;
    }
    if ( selected > max_it ) {
      if( cur_it < u.inv.size() ) {
        ch='>';
      } else {
        selected = u.inv.size() - 1; // wraparound?
      }
    }
  } else if ( ch == KEY_UP ) {
    selected--;
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
    ch = selected_char;
  }

 } while (ch == '<' || ch == '>' || ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_UP || ch == KEY_DOWN );
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
 return (char)ch;
}

// Display current inventory.
char game::inv(std::string title)
{
 u.inv.restack(&u);
 u.inv.sort();
 return inv(u.inv,title);
}

char game::inv_activatable(std::string title)
{
 u.inv.restack(&u);
 u.inv.sort();
 inventory activatables = u.inv.filter_by_activation(u);
 return inv(activatables,title);
}

char game::inv_type(std::string title, item_cat inv_item_type)
{
    u.inv.restack(&u);
    u.inv.sort();
    inventory reduced_inv = u.inv.filter_by_category(inv_item_type, u);
    return inv(reduced_inv,title);
}

char game::inv_for_liquid(const item &liquid, const std::string title, bool auto_choose_single)
{
    u.inv.restack(&u);
    u.inv.sort();
    inventory reduced_inv = u.inv.filter_by_capacity_for_liquid(liquid);
    if (auto_choose_single && reduced_inv.size() == 1) {
        std::list<item> cont_stack = reduced_inv.const_stack(0);
        if (cont_stack.size() > 0) {
            return cont_stack.front().invlet;
        }
    }
    return inv(reduced_inv, title);
}

std::vector<item> game::multidrop()
{
    WINDOW* w_inv = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH + (use_narrow_sidebar() ? 45 : 55), VIEW_OFFSET_Y, VIEW_OFFSET_X);
    const int maxitems = TERRAIN_WINDOW_HEIGHT - 5;

 u.inv.restack(&u);
 u.inv.sort();
 int drp_line_width=getmaxx(w_inv)-90;
 const std::string drp_line_padding = ( drp_line_width > 1 ? std::string(drp_line_width, ' ') : " ");
 std::map<char, int> dropping; // Count of how many we'll drop from each stack
 int count = 0; // The current count
 std::vector<char> weapon_and_armor; // Always single, not counted
 bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
 print_inv_statics(this, w_inv, _("Multidrop:"), weapon_and_armor);
 int base_weight = u.weight_carried();
 int base_volume = u.volume_carried();

 int ch = (int)'.';
 int start = 0, cur_it = 0, max_it;
 invslice stacks = u.inv.slice(0, u.inv.size());
 std::vector<int> firsts = find_firsts(stacks);
 int selected=-1;
 int selected_char=(int)' ';
 do {
  inventory drop_subset = u.inv.subset(dropping);
  int new_weight = base_weight - drop_subset.weight();
  int new_volume = base_volume - drop_subset.volume();
  for (int i = 0; i < weapon_and_armor.size(); ++i) {
   new_weight -= u.i_at(weapon_and_armor[i]).weight();
  }
  print_inv_weight_vol(this, w_inv, new_weight, new_volume);
  int cur_line = 2;
  max_it = 0;
  int drp_line = 1;
  // Print weapon to be dropped, the first position is reserved for high visibility
  mvwprintw(w_inv, 0, 90, "%s", drp_line_padding.c_str());
  bool dropping_w = false;
  for (int k = 0; k < weapon_and_armor.size() && !dropping_w; k++) {
    if (weapon_and_armor[k] == u.weapon.invlet) {
      dropping_w = true;
    }
    if (dropping_w && u.is_armed()) {
      mvwprintz(w_inv, 0, 90, c_ltblue, "%c + %s", u.weapon.invlet, u.weapname().c_str());
    }
  }
  // Print worn items to be dropped
  if(dropping_w) {
    mvwprintw(w_inv, drp_line, 90, "%s", drp_line_padding.c_str());
    drp_line++;
  }
  bool dropping_a = false;
  if (u.worn.size() > 0){
    for (int k = 0; k < u.worn.size(); k++) {
      bool dropping_w = false;
      for (int j = 0; j < weapon_and_armor.size() && !dropping_w; j++) {
        if (weapon_and_armor[j] == u.worn[k].invlet) {
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
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }

   if ( selected < start && selected > -1 ) selected = start;

   if (cur_it < stacks.size()) {
    item& it = stacks[cur_it]->front();
    if(cur_it==selected) selected_char=(int)it.invlet;
    mvwputch (w_inv, cur_line, 0, (cur_it == selected ? h_white : c_white), it.invlet);
    char icon = '-';
    if (dropping[it.invlet] >= (it.count_by_charges() ? it.charges : stacks[cur_it]->size()))
     icon = '+';
    else if (dropping[it.invlet] > 0)
     icon = '#';
    nc_color col = ( cur_it == selected ? h_white : (dropping[it.invlet] == 0 ? c_ltgray : c_white ) );
    mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
              it.tname().c_str());
    if (stacks[cur_it]->size() > 1)
     wprintz(w_inv, col, " x %d", stacks[cur_it]->size());
    if (it.charges > 0)
     wprintz(w_inv, col, " (%d)", it.charges);
    else if (it.contents.size() == 1 &&
             it.contents[0].charges > 0)
     wprintw(w_inv, " (%d)", it.contents[0].charges);
    if (icon=='+'||icon=='#') {
      mvwprintz(w_inv, drp_line, 90, col, "%c %c %s", it.invlet, icon, it.tname().c_str());
      if (icon=='+'){
        if (stacks[cur_it]->size() > 1)
          wprintz(w_inv, col, " x %d", stacks[cur_it]->size());
        if (it.charges > 0)
          wprintz(w_inv, col, " (%d)", it.charges);
      }
      if (icon=='#') {
        wprintz(w_inv, col, " {%d}", dropping[it.invlet]);
      }
      drp_line++;
    }
   }
   cur_line++;
   max_it=cur_it;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, _("< Go Back"));
  if (cur_it < u.inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, _("> More items"));
  wrefresh(w_inv);
/* back to (int)getch() as input() mangles arrow keys
  ch = input();
*/
  ch = getch();
  if ( ch == '<' || ch == KEY_PPAGE ) {
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
      selected++;
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
    selected--;
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
     if ( ch == '\t' || ch == KEY_RIGHT || ch == KEY_LEFT ) {
        ch = selected_char;
     }
     if (u.has_item(ch)) {
       item& it = u.inv.item_by_letter(ch);
       if (it.is_null()) { // Not from inventory
        int found = false;
        for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
         if (weapon_and_armor[i] == ch) {
          weapon_and_armor.erase(weapon_and_armor.begin() + i);
          found = true;
          print_inv_statics(this, w_inv, _("Multidrop:"), weapon_and_armor);
         }
        }
        if (!found) {
         if ( ch == u.weapon.invlet &&
              std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(), u.weapon.type->id) != unreal_itype_ids.end()){
          if (!warned_about_bionic)
           add_msg(_("You cannot drop your %s."), u.weapon.tname().c_str());
          warned_about_bionic = true;
         } else {
          weapon_and_armor.push_back(ch);
          print_inv_statics(this, w_inv, _("Multidrop:"), weapon_and_armor);
         }
        }
       } else {
        int index = -1;
        for (int i = 0; i < stacks.size(); ++i) {
         if (stacks[i]->front().invlet == it.invlet) {
          index = i;
          break;
         }
        }
        if (index == -1) {
         debugmsg("Inventory got out of sync with inventory slice?");
        }
        if (count == 0) {
        if (it.count_by_charges())
          {
           if (dropping[it.invlet] == 0)
            dropping[it.invlet] = -1;
           else
            dropping[it.invlet] = 0;
          }
        else
          {
           if (dropping[it.invlet] == 0)
            dropping[it.invlet] = stacks[index]->size();
           else
            dropping[it.invlet] = 0;
          }
        }

        else if (count >= stacks[index]->size() && !it.count_by_charges())
           dropping[it.invlet] = stacks[index]->size();
        else
          dropping[it.invlet] = count;

       count = 0;
       }
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

 for (std::map<char,int>::iterator it = dropping.begin(); it != dropping.end(); ++it) {
  if (it->second == -1)
   ret.push_back( u.inv.remove_item_by_letter( it->first));
  else if (it->second && u.inv.item_by_letter( it->first).count_by_charges()) {
   int charges = u.inv.item_by_letter( it->first).charges;// >= it->second ? : it->second;
   ret.push_back( u.inv.remove_item_by_charges( it->first, it->second > charges ? charges : it->second));
  } else if (it->second)
   for (int j = it->second; j > 0; j--)
    ret.push_back( u.inv.remove_item_by_letter( it->first));
 }

 for (int i = 0; i < weapon_and_armor.size(); i++)
 {
     if (!u.takeoff(this, weapon_and_armor[i], true))
     {
         continue;
     }

     // Item could have been dropped after taking it off
     if (&u.inv.item_by_letter(weapon_and_armor[i]) != &u.inv.nullitem)
     {
         ret.push_back(u.i_rem(weapon_and_armor[i]));
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

 invslice stacks = u.inv.slice(0, u.inv.size());

 WINDOW* w_inv = newwin(TERMY-VIEW_OFFSET_Y*2, TERMX-VIEW_OFFSET_X*2, VIEW_OFFSET_Y, VIEW_OFFSET_X);
 int maxitems = TERMY-5-VIEW_OFFSET_Y*2;    // Number of items to show at one time.
 std::vector<int> compare_list; // Count of how many we'll drop from each stack
 bool bFirst = false; // First Item selected
 bool bShowCompare = false;
 char cLastCh = 0;
 compare_list.resize(u.inv.size() + groundsize, 0);
 std::vector<char> weapon_and_armor; // Always single, not counted
 print_inv_statics(this, w_inv, "Compare:", weapon_and_armor);
// Gun, ammo, weapon, armor, food, tool, book, other
 std::vector<int> first = find_firsts(stacks);
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
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
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
     item& it = stacks[cur_it-groundsize]->front();
     mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
     nc_color col = (compare_list[cur_it] == 0 ? c_ltgray : c_white);
     mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
               it.tname().c_str());
     if (stacks[cur_it-groundsize]->size() > 1)
      wprintz(w_inv, col, " x %d", stacks[cur_it-groundsize]->size());
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
  if (u.has_item(ch)) {
   item& it = u.inv.item_by_letter(ch);
   if (it.is_null()) { // Not from inventory
    bool found = false;
    for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
     if (weapon_and_armor[i] == ch) {
      weapon_and_armor.erase(weapon_and_armor.begin() + i);
      found = true;
      bFirst = false;
      print_inv_statics(this, w_inv, "Compare:", weapon_and_armor);
     }
    }
    if (!found) {

     if ( ch == u.weapon.invlet &&
          std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(), u.weapon.type->id) != unreal_itype_ids.end()){
      //Do Bionic stuff here?!
     } else {
      if (!bFirst)
      {
       weapon_and_armor.push_back(ch);
       print_inv_statics(this, w_inv, "Compare:", weapon_and_armor);
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
     if (stacks[i]->front().invlet == it.invlet) {
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
    u.i_at(ch).info(true, &vItemCh);
    sItemCh = u.i_at(ch).tname();
   }

   compare_split_screen_popup(0, (TERMX-VIEW_OFFSET_X*2)/2, TERMY-VIEW_OFFSET_Y*2, sItemLastCh, vItemLastCh, vItemCh);
   compare_split_screen_popup((TERMX-VIEW_OFFSET_X*2)/2, (TERMX-VIEW_OFFSET_X*2)/2, TERMY-VIEW_OFFSET_Y*2, sItemCh, vItemCh, vItemLastCh);

   wclear(w_inv);
   print_inv_statics(this, w_inv, "Compare:", weapon_and_armor);
   bShowCompare = false;
  }
 } while (ch != '\n' && ch != KEY_ESCAPE && ch != ' ');
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
}
