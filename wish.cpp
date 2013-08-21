#include "game.h"
#include "output.h"
#include "keypress.h"
#include "item_factory.h"
#include <sstream>
#include "text_snippets.h"

#define LESS(a, b) ((a)<(b)?(a):(b))

void game::wish()
{
 WINDOW* w_search = newwin(2, FULL_SCREEN_WIDTH, 0, 0);
 WINDOW* w_list = newwin(FULL_SCREEN_HEIGHT-2, 30, 2,  0);
 WINDOW* w_info = newwin(FULL_SCREEN_HEIGHT-2, 50, 2, 30);
 int a = 0, shift = 0, result_selected = 0;
 int ch = '.';
 bool search = false;
 bool incontainer = false;
 bool changed = false;
 std::string pattern = "";
 std::string info;
 std::vector<int> search_results;
 // use this item for info display, but NOT for instantiation
 item tmp;
 tmp.corpse = mtypes[0];
 do {
  werase(w_search);
  werase(w_info);
  werase(w_list);
  mvwprintw(w_search, 0, 0, _("('/' to search, Esc to stop, <> to switch between results)"));
  mvwprintz(w_search, 1, 0, c_white, _("Wish for a: "));
  if (search) {
   if (ch == '\n') {
    search = false;
    ch = '.';
   }
   else if (ch == 27) // Escape to stop searching
   {
    search = false;
    ch = '.';
   } else if (ch == KEY_BACKSPACE || ch == 127) {
    if (pattern.length() > 0)
    {
     pattern.erase(pattern.end() - 1);
     search_results.clear();
     changed = true;
    }
    else
    {
     changed = false;
    }
   } else if (ch == '>' || ch == KEY_NPAGE) {
    if (!search_results.empty()) {
     result_selected++;
     if (result_selected >= search_results.size())
      result_selected = 0;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > standard_itype_ids.size()) {
      a = shift + 23 - standard_itype_ids.size();
      shift = standard_itype_ids.size() - 23;
     }
    }
    changed = false;
   } else if (ch == '<' || ch == KEY_PPAGE) {
    if (!search_results.empty()) {
     result_selected--;
     if (result_selected < 0)
      result_selected = search_results.size() - 1;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > standard_itype_ids.size()) {
      a = shift + 23 - standard_itype_ids.size();
      shift = standard_itype_ids.size() - 23;
     }
    }
    changed = false;
   } else {
    pattern += ch;
    search_results.clear();
    changed = true;
   }

   // If the pattern hasn't changed or we've stopped searching, no need to update
   if (search && changed) {
    // If the pattern is blank, search just returns all items, which will be listed anyway
    if (pattern.length() > 0)
    {
     for (int i = 0; i < standard_itype_ids.size(); i++) {
      if (item_controller->find_template(standard_itype_ids[i])->name.find(pattern) != std::string::npos) {
       shift = i;
       a = 0;
       result_selected = 0;
       if (shift + 23 > standard_itype_ids.size()) {
        a = shift + 23 - standard_itype_ids.size();
        shift = standard_itype_ids.size() - 23;
       }
       search_results.push_back(i);
      }
     }
     if (search_results.size() > 0) {
      shift = search_results[0];
      a = 0;
     }
    }
    else // The pattern is blank, so jump back to the top of the list
    {
     shift = 0;
    }
   }

  } else {	// Not searching; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == '/') {
    search = true;
   }
   if (ch == 'f') incontainer = !incontainer;
   if (( ch == '>' || ch == KEY_NPAGE ) && !search_results.empty()) {
    result_selected++;
    if (result_selected >= search_results.size())
     result_selected = 0;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > standard_itype_ids.size()) {
     a = shift + 23 - standard_itype_ids.size();
     shift = standard_itype_ids.size() - 23;
    }
   } else if (( ch == '<' || ch == KEY_PPAGE ) && !search_results.empty()) {
    result_selected--;
    if (result_selected < 0)
     result_selected = search_results.size() - 1;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > standard_itype_ids.size()) {
     a = shift + 23 - standard_itype_ids.size();
     shift = standard_itype_ids.size() - 23;
    }
   }
  }
  int search_string_length = pattern.length();
  if (!search_results.empty())
   mvwprintz(w_search, 1, 12, c_green, "%s", pattern.c_str());
  else if (pattern.length() > 0)
  {
   mvwprintz(w_search, 1, 12, c_red, _("\"%s \" not found!"),pattern.c_str());
   search_string_length += 1;
  }
  if (incontainer)
   mvwprintz(w_search, 0, 70, c_ltblue, _("contained"));
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 0) shift = 0;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > standard_itype_ids.size()) shift = standard_itype_ids.size() - 23;
  }
  for (int i = 0; i < 23 && i+shift < standard_itype_ids.size(); i++) {
   nc_color col = c_ltgray;
   if (i == a)
    col = h_ltgray;
   mvwprintz(w_list, i, 0, col, item_controller->find_template(standard_itype_ids[i+shift])->name.c_str());
   wprintz(w_list, item_controller->find_template(standard_itype_ids[i+shift])->color, "%c%", item_controller->find_template(standard_itype_ids[i+shift])->sym);
  }
  tmp.make(item_controller->find_template(standard_itype_ids[a + shift]));
  tmp.bday = turn;
  if (tmp.is_tool())
   tmp.charges = dynamic_cast<it_tool*>(tmp.type)->max_charges;
  else if (tmp.is_ammo())
   tmp.charges = 100;
  else if (tmp.is_gun())
   tmp.charges = 0;
  else if (tmp.is_gunmod() && (tmp.has_flag("MODE_AUX") ||
			       tmp.typeId() == "spare_mag"))
   tmp.charges = 0;
  else
   tmp.charges = -1;
  // Should be a flag, but we're out at the moment
  if( tmp.is_stationary() )
  {
    tmp.note = SNIPPET.assign( (dynamic_cast<it_stationary*>(tmp.type))->category );
  }
  info = tmp.info(true);
  mvwprintw(w_info, 0, 0, info.c_str());
  wrefresh(w_search);
  wrefresh(w_info);
  wrefresh(w_list);
  if (search)
  {
   curs_set(1);
   ch = mvgetch(1, search_string_length+12);
  }
  else
  {
   curs_set(0);
   ch = input();
  }
 } while (ch != '\n' && !(ch==KEY_ESCAPE && !search));
 clear();

 if(ch=='\n')
 {
  // Allow for multiples
  curs_set(1);
  mvprintw(0, 0, _("How many do you want? (default is 1): "));
  char str[5];
  int count = 1;
  echo();
  getnstr(str, 5);
  noecho();
  count = atoi(str);
  if (count<=0)
  {
   count = 1;
  }
  curs_set(0);

  item granted = item_controller->create(standard_itype_ids[a + shift], turn);
  mvprintw(2, 0, _("Wish granted - %d x %s."), count, granted.type->name.c_str());
  tmp.invlet = nextinv;
  for (int i=0; i<count; i++)
  {
   if (!incontainer)
    u.i_add(granted);
   else
    u.i_add(granted.in_its_container(&itypes));
  }
  advance_nextinv();
  getch();
 }
 delwin(w_info);
 delwin(w_list);
}

void game::monster_wish()
{
 WINDOW* w_list = newwin(25, 30, 0,  0);
 WINDOW* w_info = newwin(25, 50, 0, 30);
 int a = 0, shift = 1, result_selected = 0;
 int ch = '.';
 bool search = false, friendly = false;
 std::string pattern;
 std::string info;
 std::vector<int> search_results;
 monster tmp;
 do {
  werase(w_info);
  werase(w_list);
  mvwprintw(w_list, 0, 0, _("Spawn a: "));
  if (search) {
   if (ch == '\n') {
    search = false;
    ch = '.';
   } else if (ch == KEY_BACKSPACE || ch == 127) {
    if (pattern.length() > 0)
     pattern.erase(pattern.end() - 1);
   } else if (ch == '>' || ch == KEY_NPAGE) {
    search = false;
    if (!search_results.empty()) {
     result_selected++;
     if (result_selected > search_results.size())
      result_selected = 0;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > mtypes.size()) {
      a = shift + 23 - mtypes.size();
      shift = mtypes.size() - 23;
     }
    }
   } else if (ch == '<' || ch == KEY_PPAGE) {
    search = false;
    if (!search_results.empty()) {
     result_selected--;
     if (result_selected < 0)
      result_selected = search_results.size() - 1;
     shift = search_results[result_selected];
     a = 0;
     if (shift + 23 > mtypes.size()) {
      a = shift + 23 - mtypes.size();
      shift = mtypes.size() - 23;
     }
    }
   } else {
    pattern += ch;
    search_results.clear();
   }

   if (search) {
    for (int i = 1; i < mtypes.size(); i++) {
     if (mtypes[i]->name.find(pattern) != std::string::npos) {
      shift = i;
      a = 0;
      result_selected = 0;
      if (shift + 23 > mtypes.size()) {
       a = shift + 23 - mtypes.size();
       shift = mtypes.size() - 23;
      }
      search_results.push_back(i);
     }
    }
   }

  } else {	// Not searching; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == 'f') friendly = !friendly;
   if (ch == '/') {
    search = true;
    pattern =  "";
    search_results.clear();
   }
   if (( ch == '>' || ch == KEY_NPAGE ) && !search_results.empty()) {
    result_selected++;
    if (result_selected > search_results.size())
     result_selected = 0;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > mtypes.size()) {
     a = shift + 23 - mtypes.size();
     shift = mtypes.size() - 23;
    }
   } else if (( ch == '<' || ch == KEY_PPAGE ) && !search_results.empty()) {
    result_selected--;
    if (result_selected < 0)
     result_selected = search_results.size() - 1;
    shift = search_results[result_selected];
    a = 0;
    if (shift + 23 > mtypes.size()) {
     a = shift + 23 - mtypes.size();
     shift = mtypes.size() - 23;
    }
   }
  }
  if (!search_results.empty())
   mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
  else if (pattern.length() > 0)
   mvwprintz(w_list, 0, 11, c_red, _("%s not found!            "),pattern.c_str());
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 1) shift = 1;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > mtypes.size()) shift = mtypes.size() - 23;
  }
  for (int i = 1; i < 24; i++) {
   nc_color col = c_white;
   if (i == a + 1)
    col = h_white;
   mvwprintz(w_list, i, 0, col, mtypes[i-1+shift]->name.c_str());
   wprintz(w_list, mtypes[i-1+shift]->color, " %c%", mtypes[i-1+shift]->sym);
  }
  tmp = monster(mtypes[a + shift]);
  if (friendly)
   tmp.friendly = -1;
  tmp.print_info(this, w_info);
  wrefresh(w_info);
  wrefresh(w_list);
  if (search)
   ch = getch();
  else
   ch = input();
 } while (ch != '\n');
 clear();
 delwin(w_info);
 delwin(w_list);
 refresh_all();
 wrefresh(w_terrain);
 point spawn = look_around();
 if (spawn.x == -1)
  return;
 tmp.spawn(spawn.x, spawn.y);
 z.push_back(tmp);
}

void game::mutation_wish()
{
    WINDOW* w_list = newwin(25, 30, 0,  0);
    WINDOW* w_info = newwin(25, 50, 0, 30);

    std::vector<std::string> vTraits;
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        vTraits.push_back(iter->first);
    }

    int result_selected = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;
    int iContentHeight = 23;
    int ch = '.';

    bool search = false;
    std::string pattern;
    std::string info;
    std::vector<int> search_results;

    do {
        werase(w_info);
        werase(w_list);
        mvwprintw(w_list, 0, 0, _("Mutate: "));

        if (ch == '>' || ch == KEY_NPAGE) {
            search = false;
            if (!search_results.empty()) {
                result_selected++;
                if (result_selected >= search_results.size()) {
                    result_selected = 0;
                }

                iCurrentLine = search_results[result_selected];
            }
        } else if (ch == '<' || ch == KEY_PPAGE) {
            search = false;
            if (!search_results.empty()) {
                result_selected--;
                if (result_selected < 0) {
                    result_selected = search_results.size() - 1;
                }

                iCurrentLine = search_results[result_selected];
            }
        } else if (search) {
            if (ch == KEY_BACKSPACE || ch == 127) {
                if (pattern.length() > 0) {
                    pattern.erase(pattern.end() - 1);
                }

            } else {
                pattern += ch;
                search_results.clear();
            }

            if (search) {
                for (int i = 0; i < vTraits.size(); i++) {
                    if (traits[vTraits[i]].name.find(pattern) != std::string::npos) {
                        result_selected = 0;
                        search_results.push_back(i);
                    }
                }

                if (search_results.size() > 0) {
                    iCurrentLine = search_results[result_selected];
                }
            }

        } else {	// Not searching; scroll by keys
            if (ch == 'j') {
                iCurrentLine++;
                if (iCurrentLine >= vTraits.size()) {
                    iCurrentLine = 0;
                }
            } else if (ch == 'k') {
                iCurrentLine--;
                if (iCurrentLine < 0) {
                    iCurrentLine = vTraits.size()-1;
                }
            } else if (ch == '/') {
                search = true;
                pattern =  "";
                search_results.clear();
            }
        }

        if (!search_results.empty()) {
            mvwprintz(w_list, 0, 11, c_green, "%s               ", pattern.c_str());
        } else if (pattern.length() > 0) {
            mvwprintz(w_list, 0, 11, c_red, _("%s not found!            "),pattern.c_str());
        }

        if (vTraits.size() > iContentHeight) {
            iStartPos = iCurrentLine - (iContentHeight - 1) / 2;

            if (iStartPos < 0) {
                iStartPos = 0;
            } else if (iStartPos + iContentHeight > vTraits.size()) {
                iStartPos = vTraits.size() - iContentHeight;
            }
        }

        //Draw Mutations
        for (int i = iStartPos; i < vTraits.size(); i++) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > vTraits.size()) ? vTraits.size() : iContentHeight)) {
                nc_color cLineColor = c_white;

                mvwprintz(w_list, 1 + i - iStartPos, 0, (iCurrentLine == i) ? hilite(cLineColor) : cLineColor, traits[vTraits[i]].name.c_str());
            }
        }

        mvwprintw(w_info, 1, 0, mutation_data[vTraits[iCurrentLine]].valid ? _("Valid") : _("Nonvalid"));
        int line2 = 2;

        bool found = false;
        std::string sCat = "";
        for (std::map<std::string, std::vector<std::string> >::iterator iter = mutations_category.begin(); iter != mutations_category.end(); ++iter) {
            std::vector<std::string> group = mutations_category[iter->first];

            for (int j = 0; !found && j < group.size(); j++) {
                if (group[j] == vTraits[iCurrentLine]) {
                    found = true;
                    sCat = iter->first;
                    break;
                }
            }
        }

        mvwprintw(w_info, line2, 0, _("Prereqs:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].prereqs.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].prereqs[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Cancels:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].cancels.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].cancels[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Becomes:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].replacements.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].replacements[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Add-ons:"));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].additions.size(); j++) {
            mvwprintw(w_info, line2, 9, traits[ mutation_data[vTraits[iCurrentLine]].additions[j] ].name.c_str());
            line2++;
        }

        mvwprintw(w_info, line2, 0, _("Category: "));
        for (int j = 0; j < mutation_data[vTraits[iCurrentLine]].category.size(); j++) {
            mvwprintw(w_info, line2, 10, mutation_data[vTraits[iCurrentLine]].category[j].c_str());
            line2++;
        }

        wrefresh(w_info);
        wrefresh(w_list);

        if (search) {
            ch = getch();
        } else {
            ch = input();
        }
    } while (ch != '\n' && ch != 27);
    clear();

    if (ch == '\n') {
        do {
            u.mutate_towards(this, vTraits[iCurrentLine]);
        } while (!u.has_trait(vTraits[iCurrentLine]));
    }

    delwin(w_info);
    delwin(w_list);
}
