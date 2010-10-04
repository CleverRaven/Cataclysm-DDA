#include "game.h"
#include "output.h"

#define LESS(a, b) ((a)<(b)?(a):(b))

void game::wish()
{
 int a = 0, shift = 0;
 int line;
 char ch;
 bool grep = false, found = false;;
 int search;
 std::string pattern;
 std::string info;
 item tmp;
 tmp.corpse = mtypes[0];
 do {
  erase();
  mvprintw(0, 0, "Wish for: ");
  if (grep) {
   found = false;
   if (ch == '\n') {
    grep = false;
    found = true;
    pattern = "";
   } else {
    pattern = pattern + ch;
    if (ch == ' ')
     ch = '.';	// Don't escape the wish function yet!
   }
   for (search = 0; search < itypes.size() && !found; search++) {
    if (itypes[search]->name.find(pattern) != std::string::npos) {
     shift = search;
     a = 0;
     if (shift + 23 > itypes.size()) {
      a = shift + 23 - itypes.size();
      shift = itypes.size() - 23;
     }
     found = true;
    }
   }
   if (found)
    mvprintw(1, 0, "%s               ", pattern.c_str());
   else if (grep)
    mvprintz(1, 0, c_red, "%s not found!             ", pattern.c_str());
   else
    mvprintw(1, 0, "                      ");
  } else {	// Not grepping; scroll by keys
   if (ch == 'j') a++;
   if (ch == 'k') a--;
   if (ch == '/') { 
    grep = true;
    pattern =  "";
   }
  }
  if (a < 0) {
   a = 0;
   shift--;
   if (shift < 0) shift = 0;
  }
  if (a > 22) {
   a = 22;
   shift++;
   if (shift + 23 > itypes.size()) shift = itypes.size() - 23;
  }
  for (int i = 1; i < LESS(24, itypes.size()); i++) {
   mvprintz(i, 40, c_white, itypes[i-1+shift]->name.c_str());
   printz(itypes[i-1+shift]->color, "%c%", itypes[i-1+shift]->sym);
  }
  tmp.make(itypes[a + shift]);
  if (tmp.is_tool())
   tmp.charges = dynamic_cast<it_tool*>(tmp.type)->def_charges;
  else if (tmp.is_ammo())
   tmp.charges = 100;
  info = tmp.info();
  line = 2;
  mvprintw(line, 1, info.c_str());
  ch = getch();
 } while (ch != ' ');
 clear();
 mvprintw(0, 0, "\nWish granted.");
 tmp.invlet = nextinv;
 u.i_add(tmp);
 advance_nextinv();
 getch();
}
