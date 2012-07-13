#include "game.h"
#include "output.h"
#include "keypress.h"
#include <string>
#include <vector>

std::string CATEGORIES[8] =
 {"FIREARMS:", "AMMUNITION:", "CLOTHING:", "COMESTIBLES:",
  "TOOLS:", "BOOKS:", "WEAPONS:", "OTHER:"};

void print_inv_statics(game *g, WINDOW* w_inv, std::string title,
                       std::vector<char> dropped_items)
{
// Print our header
 mvwprintw(w_inv, 0, 0, title.c_str());

// Print weight
 mvwprintw(w_inv, 0, 40, "Weight: ");
 if (g->u.weight_carried() >= g->u.weight_capacity() * .25)
  wprintz(w_inv, c_red, "%d", g->u.weight_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", g->u.weight_carried());
 wprintz(w_inv, c_ltgray, "/%d/%d", int(g->u.weight_capacity() * .25),
                                    g->u.weight_capacity());

// Print volume
 mvwprintw(w_inv, 0, 60, "Volume: ");
 if (g->u.volume_carried() > g->u.volume_capacity() - 2)
  wprintz(w_inv, c_red, "%d", g->u.volume_carried());
 else
  wprintz(w_inv, c_ltgray, "%d", g->u.volume_carried());
 wprintw(w_inv, "/%d", g->u.volume_capacity() - 2);

// Print our weapon
 mvwprintz(w_inv, 2, 40, c_magenta, "WEAPON:");
 int dropping_weapon = false;
 for (int i = 0; i < dropped_items.size() && !dropping_weapon; i++) {
  if (dropped_items[i] == g->u.weapon.invlet)
   dropping_weapon = true;
 }
 if (g->u.is_armed()) {
  if (dropping_weapon)
   mvwprintz(w_inv, 3, 40, c_white, "%c + %s", g->u.weapon.invlet,
             g->u.weapname().c_str());
  else
   mvwprintz(w_inv, 3, 40, g->u.weapon.color_in_inventory(&(g->u)), "%c - %s",
             g->u.weapon.invlet, g->u.weapname().c_str());
 } else if (g->u.weapon.is_style())
  mvwprintz(w_inv, 3, 40, c_ltgray, "%c - %s",
            g->u.weapon.invlet, g->u.weapname().c_str());
 else
  mvwprintz(w_inv, 3, 42, c_ltgray, g->u.weapname().c_str());
// Print worn items
 if (g->u.worn.size() > 0)
  mvwprintz(w_inv, 5, 40, c_magenta, "ITEMS WORN:");
 for (int i = 0; i < g->u.worn.size(); i++) {
  bool dropping_armor = false;
  for (int j = 0; j < dropped_items.size() && !dropping_armor; j++) {
   if (dropped_items[j] == g->u.worn[i].invlet)
    dropping_armor = true;
  }
  if (dropping_armor)
   mvwprintz(w_inv, 6 + i, 40, c_white, "%c + %s", g->u.worn[i].invlet,
             g->u.worn[i].tname(g).c_str());
  else
   mvwprintz(w_inv, 6 + i, 40, c_ltgray, "%c - %s", g->u.worn[i].invlet,
             g->u.worn[i].tname(g).c_str());
 }
}
 
std::vector<int> find_firsts(inventory &inv)
{
 std::vector<int> firsts;
 for (int i = 0; i < 8; i++)
  firsts.push_back(-1);

 for (int i = 0; i < inv.size(); i++) {
       if (firsts[0] == -1 && inv[i].is_gun())
   firsts[0] = i;
  else if (firsts[1] == -1 && inv[i].is_ammo())
   firsts[1] = i;
  else if (firsts[2] == -1 && inv[i].is_armor())
   firsts[2] = i;
  else if (firsts[3] == -1 &&
           (inv[i].is_food() || inv[i].is_food_container()))
   firsts[3] = i;
  else if (firsts[4] == -1 && (inv[i].is_tool() || inv[i].is_gunmod() ||
                               inv[i].is_bionic()))
   firsts[4] = i;
  else if (firsts[5] == -1 && inv[i].is_book())
   firsts[5] = i;
  else if (firsts[6] == -1 && inv[i].is_weap())
   firsts[6] = i;
  else if (firsts[7] == -1 && inv[i].is_other())
   firsts[7] = i;
 }

 return firsts;
}
 
// Display current inventory.
char game::inv(std::string title)
{
 WINDOW* w_inv = newwin(25, 80, 0, 0);
 const int maxitems = 20;	// Number of items to show at one time.
 char ch = '.';
 int start = 0, cur_it;
 u.sort_inv();
 u.inv.restack(&u);
 std::vector<char> null_vector;
 print_inv_statics(this, w_inv, title, null_vector);
// Gun, ammo, weapon, armor, food, tool, book, other
 std::vector<int> firsts = find_firsts(u.inv);

 do {
  if (ch == '<' && start > 0) { // Clear lines and shift
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) { // Clear lines and shift
   start = cur_it;
   mvwprintw(w_inv, maxitems + 2, 12, "            ");
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                    ");
// Print category header
   for (int i = 0; i < 8; i++) {
    if (cur_it == firsts[i]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size()) {
    mvwputch (w_inv, cur_line, 0, c_white, u.inv[cur_it].invlet);
    mvwprintz(w_inv, cur_line, 1, u.inv[cur_it].color_in_inventory(&u), " %s",
              u.inv[cur_it].tname(this).c_str());
    if (u.inv.stack_at(cur_it).size() > 1)
     wprintw(w_inv, " [%d]", u.inv.stack_at(cur_it).size());
    if (u.inv[cur_it].charges > 0)
     wprintw(w_inv, " (%d)", u.inv[cur_it].charges);
    else if (u.inv[cur_it].contents.size() == 1 &&
             u.inv[cur_it].contents[0].charges > 0)
     wprintw(w_inv, " (%d)", u.inv[cur_it].contents[0].charges);
   }
   cur_line++;
  }
  if (start > 0)
   mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
  if (cur_it < u.inv.size())
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
 u.sort_inv();
 u.inv.restack(&u);
 WINDOW* w_inv = newwin(25, 80, 0, 0);
 const int maxitems = 20;    // Number of items to show at one time.
 int dropping[u.inv.size()]; // Count of how many we'll drop from each stack
 for (int i = 0; i < u.inv.size(); i++)
  dropping[i] = 0;
 int count = 0; // The current count
 std::vector<char> weapon_and_armor; // Always single, not counted
 bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
 print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
// Gun, ammo, weapon, armor, food, tool, book, other
 std::vector<int> firsts = find_firsts(u.inv);

 char ch = '.';
 int start = 0, cur_it;
 do {
  if (ch == '<' && start > 0) {
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
   start -= maxitems;
   if (start < 0)
    start = 0;
   mvwprintw(w_inv, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < u.inv.size()) {
   start = cur_it;
   mvwprintw(w_inv, maxitems + 2, 12, "            ");
   for (int i = 1; i < 25; i++)
    mvwprintz(w_inv, i, 0, c_black, "                                        ");
  }
  int cur_line = 2;
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
// Clear the current line;
   mvwprintw(w_inv, cur_line, 0, "                                    ");
// Print category header
   for (int i = 0; i < 8; i++) {
    if (cur_it == firsts[i]) {
     mvwprintz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
     cur_line++;
    }
   }
   if (cur_it < u.inv.size()) {
    mvwputch (w_inv, cur_line, 0, c_white, u.inv[cur_it].invlet);
    char icon = '-';
    if (dropping[cur_it] >= u.inv.stack_at(cur_it).size())
     icon = '+';
    else if (dropping[cur_it] > 0)
     icon = '#';
    nc_color col = (dropping[cur_it] == 0 ? c_ltgray : c_white);
    mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon,
              u.inv[cur_it].tname(this).c_str());
    if (u.inv.stack_at(cur_it).size() > 1)
     wprintz(w_inv, col, " [%d]", u.inv.stack_at(cur_it).size());
    if (u.inv[cur_it].charges > 0)
     wprintz(w_inv, col, " (%d)", u.inv[cur_it].charges);
    else if (u.inv[cur_it].contents.size() == 1 &&
             u.inv[cur_it].contents[0].charges > 0)
     wprintw(w_inv, " (%d)", u.inv[cur_it].contents[0].charges);
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
   int index = u.inv.index_by_letter(ch);
   if (index == -1) { // Not from inventory
    int found = false;
    for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
     if (weapon_and_armor[i] == ch) {
      weapon_and_armor.erase(weapon_and_armor.begin() + i);
      found = true;
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
     }
    }
    if (!found) {
     if (ch == u.weapon.invlet && u.weapon.type->id > num_items &&
         u.weapon.type->id < num_all_items) {
      if (!warned_about_bionic)
       add_msg("You cannot drop your %s.", u.weapon.tname(this).c_str());
      warned_about_bionic = true;
     } else {
      weapon_and_armor.push_back(ch);
      print_inv_statics(this, w_inv, "Multidrop:", weapon_and_armor);
     }
    }
   } else {
    if (count == 0) {
     if (dropping[index] == 0)
      dropping[index] = u.inv.stack_at(index).size();
     else
      dropping[index] = 0;
    } else if (count >= u.inv.stack_at(index).size())
     dropping[index] = u.inv.stack_at(index).size();
    else
     dropping[index] = count;
   }
   count = 0;
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
  for (int j = 0; j < dropping[i]; j++) {
   if (current_stack >= 0) {
    if (u.inv.stack_at(current_stack).size() == 1) {
     ret.push_back(u.inv.remove_item(current_stack));
     current_stack--;
    } else
     ret.push_back(u.inv.remove_item(current_stack));
   }
  }
  current_stack++;
 }

 for (int i = 0; i < weapon_and_armor.size(); i++)
  ret.push_back(u.i_rem(weapon_and_armor[i]));

 return ret;
}

