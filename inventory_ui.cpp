#include "game.h"
#include "output.h"
#include "keypress.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

const int iCategorieNum = 11;
std::string CATEGORIES[iCategorieNum] =
 {"GROUND:", "FIREARMS:", "AMMUNITION:", "CLOTHING:", "FOOD/DRINKS:",
  "TOOLS:", "BOOKS:", "WEAPONS:", "MODS/BIONICS", "MEDICINE/DRUGS", "OTHER:"};

//TODO: make this function not have issues with items that are classed as multiple things
std::vector<int> find_firsts(invslice &slice)
{
    std::vector<int> firsts;
    for (int i = 0; i < iCategorieNum-1; i++) {
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

void print_inv_statics(game *g, WINDOW* w_inv, std::string title,
                       std::vector<char> dropped_items)
{
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());

// Print weight
 mvwprintw(w_inv, 0, 45, "Weight: ");
 if (g->u.weight_carried() >= g->u.weight_capacity() * .25)
  wprintz(w_inv, c_red, "%d", g->u.weight_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", g->u.weight_carried());
 wprintz(w_inv, c_ltgray, "/%d", int(g->u.weight_capacity() * .25));//, g->u.weight_capacity());

// Print volume
 mvwprintw(w_inv, 0, 62, "Volume: ");
 if (g->u.volume_carried() > g->u.volume_capacity() - 2)
  wprintz(w_inv, c_red, "%d", g->u.volume_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", g->u.volume_carried());
 wprintw(w_inv, "/%d", g->u.volume_capacity() - 2);

// Print our weapon
 int n_items = 0;
 mvwprintz(w_inv, 2, 45, c_magenta, "WEAPON:");
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
   mvwprintz(w_inv, 3, 45, g->u.weapon.color_in_inventory(&(g->u)), "%c - %s",
             g->u.weapon.invlet, g->u.weapname().c_str());
 } else if (g->u.weapon.is_style()) {
  n_items++;
  mvwprintz(w_inv, 3, 45, c_ltgray, "%c - %s",
            g->u.weapon.invlet, g->u.weapname().c_str());
 } else
  mvwprintz(w_inv, 3, 45, c_ltgray, g->u.weapname().c_str());
// Print worn items
 if (g->u.worn.size() > 0)
  mvwprintz(w_inv, 5, 45, c_magenta, "ITEMS WORN:");
 for (int i = 0; i < g->u.worn.size(); i++) {
  n_items++;
  bool dropping_armor = false;
  for (int j = 0; j < dropped_items.size() && !dropping_armor; j++) {
   if (dropped_items[j] == g->u.worn[i].invlet)
    dropping_armor = true;
  }
  if (dropping_armor)
   mvwprintz(w_inv, 6 + i, 45, c_white, "%c + %s", g->u.worn[i].invlet,
             g->u.worn[i].tname(g).c_str());
  else
   mvwprintz(w_inv, 6 + i, 45, c_ltgray, "%c - %s", g->u.worn[i].invlet,
             g->u.worn[i].tname(g).c_str());
 }

 // Print items carried
 for (std::string::const_iterator invlet = inv_chars.begin();
      invlet != inv_chars.end(); invlet++) {
   n_items += ((g->u.inv.item_by_letter(*invlet).is_null()) ? 0 : 1);
 }
 mvwprintw(w_inv, 1, 62, "Items:  %d/%d ", n_items, inv_chars.size());
}

// Display current inventory.
char game::inv(std::string title)
{
 WINDOW* w_inv = newwin(((VIEWY < 12) ? 25 : VIEWY*2+1), ((VIEWX < 12) ? 80 : VIEWX*2+56), VIEW_OFFSET_Y, VIEW_OFFSET_X);
 const int maxitems = (VIEWY < 12) ? 20 : VIEWY*2-4;    // Number of items to show at one time.
 int ch = (int)'.';
 int start = 0, cur_it;
 u.inv.sort();
 u.inv.restack(&u);
 invslice slice = u.inv.slice(0, u.inv.size());
 std::vector<char> null_vector;
 print_inv_statics(this, w_inv, title, null_vector);
// Gun, ammo, weapon, armor, food, tool, book, other
 std::vector<int> firsts = find_firsts(slice);

 do {
  if (( ch == '<' || ch == KEY_PPAGE ) && start > 0) { // Clear lines and shift
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
  }
  if ((ch == '>' || ch == KEY_NPAGE ) && cur_it < u.inv.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");
// Print category header
   for (int i = 1; i < iCategorieNum; i++) {
    if (cur_it == firsts[i-1]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < slice.size()) {
    item& it = slice[cur_it]->front();
    mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
    mvwprintz(w_inv, cur_line, 1, it.color_in_inventory(&u), " %s",
              it.tname(this).c_str());
    if (slice[cur_it]->size() > 1)
     wprintw(w_inv, " [%d]", slice[cur_it]->size());
    if (it.charges > 0)
     wprintw(w_inv, " (%d)", it.charges);
    else if (it.contents.size() == 1 &&
             it.contents[0].charges > 0)
     wprintw(w_inv, " (%d)", it.contents[0].charges);
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, "> More items");
  wrefresh(w_inv);
  ch = getch();
 } while (ch == '<' || ch == '>' || ch == KEY_NPAGE || ch == KEY_PPAGE );
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
 return ch;
}

char game::inv_type(std::string title, item_cat inv_item_type)
{
// this function lists inventory objects by type
// refer to enum item_cat in itype.h for list of categories

 WINDOW* w_inv = newwin(((VIEWY < 12) ? 25 : VIEWY*2+1), ((VIEWX < 12) ? 80 : VIEWX*2+56), VIEW_OFFSET_Y, VIEW_OFFSET_X);
 const int maxitems = (VIEWY < 12) ? 20 : VIEWY*2-4;    // Number of items to show at one time.
 char ch = '.';
 int start = 0, cur_it;
 u.inv.sort();
 u.inv.restack(&u);
 std::vector<char> null_vector;
 print_inv_statics(this, w_inv, title, null_vector);
// Gun, ammo, weapon, armor, food, tool, book, other

// Create the reduced inventory
 inventory reduced_inv = u.inv.filter_by_category(inv_item_type, u);

 invslice slice = reduced_inv.slice(0, reduced_inv.size());
 std::vector<int> firsts = find_firsts(slice);

 do {
  if (ch == '<' && start > 0) { // Clear lines and shift
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
  }
  if (ch == '>' && cur_it < reduced_inv.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");

   for (int i = 1; i < iCategorieNum; i++) {
    if (cur_it == firsts[i-1]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }

   if (cur_it < slice.size())
   {
    item& it = slice[cur_it]->front();
    mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
    mvwprintz(w_inv, cur_line, 1, it.color_in_inventory(&u), " %s",
              it.tname(this).c_str());
    if (slice[cur_it]->size() > 1)
     wprintw(w_inv, " [%d]", slice[cur_it]->size());
    if (it.charges > 0)
     wprintw(w_inv, " (%d)", it.charges);
    else if (it.contents.size() == 1 &&
             it.contents[0].charges > 0)
     wprintw(w_inv, " (%d)", it.contents[0].charges);
   cur_line++;
   }
//   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < reduced_inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, "> More items");
  wrefresh(w_inv);
  ch = getch();
 } while (ch == '<' || ch == '>');
 werase(w_inv);
 delwin(w_inv);
 erase();
 refresh_all();
 return ch;
}

std::vector<item> game::multidrop()
{
 u.inv.sort();
 u.inv.restack(&u);
 WINDOW* w_inv = newwin(((VIEWY < 12) ? 25 : VIEWY*2+1), ((VIEWX < 12) ? 80 : VIEWX*2+56), VIEW_OFFSET_Y, VIEW_OFFSET_X);
 const int maxitems = (VIEWY < 12) ? 20 : VIEWY*2-4;    // Number of items to show at one time.
 int dropping[u.inv.size()]; // Count of how many we'll drop from each stack
 for (int i = 0; i < u.inv.size(); i++)
  dropping[i] = 0;
 int count = 0; // The current count
 std::vector<char> weapon_and_armor; // Always single, not counted
 bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
 print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);

 char ch = '.';
 int start = 0, cur_it;
 invslice stacks = u.inv.slice(0, u.inv.size());
 std::vector<int> firsts = find_firsts(stacks);
 do {
  if (ch == '<' && start > 0) {
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) {
   start = cur_it;
   mvwprintw(w_inv, maxitems + 4, 12, "            ");
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems+3; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                             ");
// Print category header
   for (int i = 1; i < iCategorieNum; i++) {
    if (cur_it == firsts[i-1]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < stacks.size()) {
    item& it = stacks[cur_it]->front();
    mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
    char icon = '-';
    if (dropping[cur_it] >= stacks[cur_it]->size())
     icon = '+';
    else if (dropping[cur_it] > 0)
     icon = '#';
    nc_color col = (dropping[cur_it] == 0 ? c_ltgray : c_white);
    mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
              it.tname(this).c_str());
    if (stacks[cur_it]->size() > 1)
     wprintz(w_inv, col, " [%d]", stacks[cur_it]->size());
    if (it.charges > 0)
     wprintz(w_inv, col, " (%d)", it.charges);
    else if (it.contents.size() == 1 &&
             it.contents[0].charges > 0)
     wprintw(w_inv, " (%d)", it.contents[0].charges);
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size())
   mvwprintw(w_inv, maxitems + 4, 12, "> More items");
  wrefresh(w_inv);
  ch = getch();
  if (ch >= '0' && ch <= '9') {
   ch -= '0';
   count *= 10;
   count += ch;
  } else if (u.has_item(ch)) {
   item& it = u.inv.item_by_letter(ch);
   if (it.is_null()) { // Not from inventory
    int found = false;
    for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
     if (weapon_and_armor[i] == ch) {
      weapon_and_armor.erase(weapon_and_armor.begin() + i);
      found = true;
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
     }
    }
    if (!found) {
     if ( ch == u.weapon.invlet &&
          std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(), u.weapon.type->id) != unreal_itype_ids.end()){
      if (!warned_about_bionic)
       add_msg("You cannot drop your %s.", u.weapon.tname(this).c_str());
      warned_about_bionic = true;
     } else {
      weapon_and_armor.push_back(ch);
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
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
       if (dropping[index] == 0)
        dropping[index] = -1;
       else
        dropping[index] = 0;
      }
    else
      {
       if (dropping[index] == 0)
        dropping[index] = stacks[index]->size();
       else
        dropping[index] = 0;
      }
    }

    else if (count >= stacks[index]->size() && !it.count_by_charges())
       dropping[index] = stacks[index]->size();
    else
      dropping[index] = count;

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

 int current_stack = 0;
 int max_size = u.inv.size();
 for (int i = 0; i < max_size; i++) {

  if (dropping[i] == -1) {  // drop whole stack of charges
   ret.push_back(u.inv.remove_item_by_letter(stacks[current_stack]->front().invlet));
  }

  for (int j = 0; j < dropping[i]; j++) {
   if (stacks[current_stack]->begin()->count_by_charges()) {      // dropping parts of stacks
    int tmpcount = dropping[i];

    if (tmpcount >= stacks[current_stack]->begin()->charges) {
      ret.push_back(u.inv.remove_item_by_letter(stacks[current_stack]->front().invlet));
    } else {
      // u.inv.stack_at(current_stack)[0].charges -= tmpcount;
      // (ZwodahS : I move this code into inventory.cpp instead)
      ret.push_back(u.inv.remove_item_by_letter_and_quantity(stacks[current_stack]->front().invlet, tmpcount));
    }
    j = dropping[i];
   } else {
    if (current_stack >= 0) {
    if (stacks[current_stack]->size() == 1) {
     ret.push_back(u.inv.remove_item_by_letter(stacks[current_stack]->front().invlet));
    } else
     ret.push_back(u.inv.remove_item_by_letter(stacks[current_stack]->front().invlet));
    }
   }
  }
  current_stack++;
 }

 for (int i = 0; i < weapon_and_armor.size(); i++)
  ret.push_back(u.i_rem(weapon_and_armor[i]));

 return ret;
}

void game::compare(int iCompareX, int iCompareY)
{
 int examx, examy;
 char ch = '.';

 if (iCompareX != -999 && iCompareX != -999) {
  examx = iCompareX;
  examy = iCompareY;
 } else {
  mvwprintw(w_terrain, 0, 0, "Compare where? (Direction button)");
  wrefresh(w_terrain);

  ch = input();
  last_action += ch;
  if (ch == KEY_ESCAPE || ch == 'q')
   return;
  if (ch == '\n' || ch == 'I')
   ch = '.';
  get_direction(this, examx, examy, ch);
  if (examx == -2 || examy == -2) {
   add_msg("Invalid direction.");
   return;
  }
 }
 examx += u.posx;
 examy += u.posy;
 std::vector <item> here = m.i_at(examx, examy);
 std::vector <item> grounditems;
 //Filter out items with the same name (keep only one of them)
 std::map <std::string, bool> dups;
 for (int i = 0; i < here.size(); i++) {
  if (!dups[here[i].tname(this).c_str()]) {
   grounditems.push_back(here[i]);
   dups[here[i].tname(this).c_str()] = true;
  }
 }
 //Only the first 10 Items due to numbering 0-9
 const int groundsize = (grounditems.size() > 10 ? 10 : grounditems.size());
 u.inv.sort();
 u.inv.restack(&u);
 
 invslice stacks = u.inv.slice(0, u.inv.size());

 WINDOW* w_inv = newwin(TERMY-VIEW_OFFSET_Y*2, TERMX-VIEW_OFFSET_X*2, VIEW_OFFSET_Y, VIEW_OFFSET_X);
 int maxitems = TERMY-5-VIEW_OFFSET_Y*2;    // Number of items to show at one time.
 int compare[u.inv.size() + groundsize]; // Count of how many we'll drop from each stack
 bool bFirst = false; // First Item selected
 bool bShowCompare = false;
 char cLastCh;
 for (int i = 0; i < u.inv.size() + groundsize; i++)
  compare[i] = 0;
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
 int start = 0, cur_it;
 do {
  if (ch == '<' && start > 0) {
   for (int i = 1; i < maxitems+4; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                             ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 4, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size() + groundsize) {
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
   for (int i = iHeaderOffset; i < iCategorieNum; i++) {
    if (cur_it == firsts[i-iHeaderOffset]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size() + groundsize) {
    char icon = '-';
    if (compare[cur_it] == 1)
     icon = '+';
    if (cur_it < groundsize) {
     mvwputch (w_inv, cur_line, 0, c_white, '1'+((cur_it<9) ? cur_it: -1));
     nc_color col = (compare[cur_it] == 0 ? c_ltgray : c_white);
     mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
               grounditems[cur_it].tname(this).c_str());
    } else {
     item& it = stacks[cur_it-groundsize]->front();
     mvwputch (w_inv, cur_line, 0, c_white, it.invlet);
     nc_color col = (compare[cur_it] == 0 ? c_ltgray : c_white);
     mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
               it.tname(this).c_str());
     if (stacks[cur_it-groundsize]->size() > 1)
      wprintz(w_inv, col, " [%d]", stacks[cur_it-groundsize]->size());
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
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size() + groundsize)
   mvwprintw(w_inv, maxitems + 4, 12, "> More items");
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
    if (compare[index+groundsize] == 1)
    {
     compare[index+groundsize] = 0;
     bFirst = false;
    } else {
     if (!bFirst)
     {
      compare[index+groundsize] = 1;
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
   if (compare[ch-'1'+iZero] == 1)
   {
    compare[ch-'1'+iZero] = 0;
    bFirst = false;
   } else {
    if (!bFirst)
    {
     compare[ch-'1'+iZero] = 1;
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
    sItemLastCh = grounditems[cLastCh-'1'+iZero].tname(this);
   } else {
    u.i_at(cLastCh).info(true, &vItemLastCh);
    sItemLastCh = u.i_at(cLastCh).tname(this);
   }

   if (ch >= '0' && ch <= '9') {
    int iZero = 0;
    if (ch == '0') {
     iZero = 10;
    }

    grounditems[ch-'1'+iZero].info(true, &vItemCh);
    sItemCh = grounditems[ch-'1'+iZero].tname(this);
   } else {
    u.i_at(ch).info(true, &vItemCh);
    sItemCh = u.i_at(ch).tname(this);
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
