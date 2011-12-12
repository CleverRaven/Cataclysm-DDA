#include "map.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include <cmath>
#include <stdlib.h>
#include <fstream>

#define SGN(a) (((a)<0) ? -1 : 1)

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

map::map()
{
 nulter = t_null;
 nultrap = tr_null;
}

map::map(std::vector<itype*> *itptr, std::vector<itype_id> (*miptr)[num_itloc],
         std::vector<trap*> *trptr)
{
 nulter = t_null;
 nultrap = tr_null;
 itypes = itptr;
 mapitems = miptr;
 traps = trptr;
 for (int n = 0; n < my_MAPSIZE() * my_MAPSIZE(); n++) {
  for (int x = 0; x < SEEX; x++) {
   for (int y = 0; y < SEEY; y++) {
    grid[n].trp[x][y] = tr_null;
    grid[n].rad[x][y] = 0;
   }
  }
 }
}

map::~map()
{
}

ter_id& map::ter(int x, int y)
{
 if (!inbounds(x, y)) {
  nulter = t_null;
  return nulter;	// Out-of-bounds - null terrain 
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 return grid[nonant].ter[x][y];
}

std::string map::tername(int x, int y)
{
 return terlist[ter(x, y)].name;
}

std::string map::features(int x, int y)
{
// This is used in an info window that is 46 characters wide, and is expected
// to take up one line.  So, make sure it does that.
 std::string ret;
 if (has_flag(bashable, x, y))
  ret += "Smashable. ";	// 11 chars (running total)
 if (has_flag(diggable, x, y))
  ret += "Diggable. ";	// 21 chars
 if (has_flag(rough, x, y))
  ret += "Rough. ";	// 28 chars
 if (has_flag(sharp, x, y))
  ret += "Sharp. ";	// 35 chars
 return ret;
}

int map::move_cost(int x, int y)
{
 return terlist[ter(x, y)].movecost;
}

bool map::trans(int x, int y)
{
 // Control statement is a problem. Normally returning false on an out-of-bounds
 // is how we stop rays from going on forever.  Instead we'll have to include
 // this check in the ray loop.
 return terlist[ter(x, y)].flags & mfb(transparent) &&
        (field_at(x, y).type == 0 ||	// Fields may obscure the view, too
        fieldlist[field_at(x, y).type].transparent[field_at(x, y).density - 1]);
}

bool map::has_flag(t_flag flag, int x, int y)
{
 return terlist[ter(x, y)].flags & mfb(flag);
}

bool map::is_destructable(int x, int y)
{
 return (has_flag(bashable, x, y) ||
         (move_cost(x, y) == 0 && !has_flag(transparent, x, y)));
}

bool map::is_outside(int x, int y)
{
 return (ter(x    , y    ) != t_floor && ter(x - 1, y - 1) != t_floor &&
         ter(x - 1, y    ) != t_floor && ter(x - 1, y + 1) != t_floor &&
         ter(x    , y - 1) != t_floor && ter(x    , y    ) != t_floor &&
         ter(x    , y + 1) != t_floor && ter(x + 1, y - 1) != t_floor &&
         ter(x + 1, y    ) != t_floor && ter(x + 1, y + 1) != t_floor &&
         ter(x    , y    ) != t_floor_wax &&
         ter(x - 1, y - 1) != t_floor_wax &&
         ter(x - 1, y    ) != t_floor_wax &&
         ter(x - 1, y + 1) != t_floor_wax &&
         ter(x    , y - 1) != t_floor_wax &&
         ter(x    , y    ) != t_floor_wax &&
         ter(x    , y + 1) != t_floor_wax &&
         ter(x + 1, y - 1) != t_floor_wax &&
         ter(x + 1, y    ) != t_floor_wax &&
         ter(x + 1, y + 1) != t_floor_wax   );
}

bool map::flammable_items_at(int x, int y)
{
 for (int i = 0; i < i_at(x, y).size(); i++) {
  item *it = &(i_at(x, y)[i]);
  if (it->made_of(PAPER) || it->made_of(WOOD) || it->made_of(COTTON) ||
      it->made_of(POWDER) || it->made_of(VEGGY) || it->is_ammo() ||
      it->type->id == itm_whiskey || it->type->id == itm_vodka ||
      it->type->id == itm_rum || it->type->id == itm_tequila)
   return true;
 }
 return false;
}

point map::random_outdoor_tile()
{
 std::vector<point> options;
 for (int x = 0; x < SEEX * my_MAPSIZE(); x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE(); y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 if (options.empty()) // Nowhere is outdoors!
  return point(-1, -1);

 return options[rng(0, options.size() - 1)];
}

bool map::bash(int x, int y, int str, std::string &sound)
{
 sound = "";
 bool smashed_web = false;
 if (field_at(x, y).type == fd_web) {
  smashed_web = true;
  field_at(x, y) = field();
 }

 for (int i = 0; i < i_at(x, y).size(); i++) {	// Destroy glass items (maybe)
  if (i_at(x, y)[i].made_of(GLASS) && one_in(2)) {
   if (sound == "")
    sound = "A " + i_at(x, y)[i].tname() + " shatters!  ";
   else
    sound = "Some items shatter!  ";
   for (int j = 0; j < i_at(x, y)[i].contents.size(); j++)
    i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
   i_rem(x, y, i);
   i--;
  }
 }
 switch (ter(x, y)) {
 case t_wall_wood:
  if (str >= rng(0, 120)) {
   sound += "crash!";
   ter(x, y) = t_dirt;
   int num_boards = rng(8, 20);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;
 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  if (str >= rng(0, 40)) {
   sound += "smash!";
   ter(x, y) = t_door_b;
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;
 case t_door_b:
  if (str >= rng(0, 30)) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   int num_boards = rng(2, 6);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;
 case t_window:
 case t_window_alarm:
  if (str >= rng(0, 6)) {
   sound += "glass breaking!";
   ter(x, y) = t_window_frame;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;
 case t_door_boarded:
  if (str >= dice(3, 50)) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   int num_boards = rng(0, 2);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;
 case t_window_boarded:
  if (str >= dice(3, 30)) {
   sound += "crash!";
   ter(x, y) = t_window_frame;
   int num_boards = rng(0, 2) * rng(0, 1);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;
 case t_paper:
  if (str >= dice(2, 6) - 2) {
   sound += "rrrrip!";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "slap!";
   return true;
  }
  break;
 case t_toilet:
  if (str >= dice(8, 4) - 8) {
   sound += "porcelain breaking!";
   ter(x, y) = t_rubble;
   return true;
  } else {
   sound += "whunk!";
   return true;
  }
  break;
 case t_dresser:
 case t_bookcase:
  if (str >= dice(3, 45)) {
   sound += "smash!";
   ter(x, y) = t_floor;
   int num_boards = rng(4, 12);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "whump.";
   return true;
  }
  break;
 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
 case t_door_glass_c:
  if (str >= rng(0, 20)) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;
 case t_reinforced_glass_h:
 case t_reinforced_glass_v:
  if (str >= rng(60, 100)) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;
 case t_tree_young:
  if (str >= rng(0, 50)) {
   sound += "crunch!";
   ter(x, y) = t_underbrush;
   int num_sticks = rng(0, 3);
   for (int i = 0; i < num_sticks; i++)
    add_item(x, y, (*itypes)[itm_stick], 0);
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;
 case t_underbrush:
  if (str >= rng(0, 30) && !one_in(4)) {
   sound += "crunch.";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "brush.";
   return true;
  }
  break;
 case t_marloss:
  if (str > rng(0, 40)) {
   sound += "crunch!";
   ter(x, y) = t_fungus;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;
 case t_vat:
  if (str >= dice(2, 20)) {
   sound += "ker-rash!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "plunk.";
   return true;
  }
 case t_crate_c:
 case t_crate_o:
  if (str >= dice(4, 20)) {
   sound += "smash";
   ter(x, y) = t_dirt;
   int num_boards = rng(1, 5);
   for (int i = 0; i < num_boards; i++)
    add_item(x, y, (*itypes)[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
   
 }
 if (move_cost(x, y) == 0) {
  sound += "thump!";
  return true;
 }
 return smashed_web;// If we kick empty space, the action is cancelled
}

// map::destroy is only called (?) if the terrain is NOT bashable.
void map::destroy(game *g, int x, int y, bool makesound)
{
 switch (ter(x, y)) {
 case t_gas_pump:
  if (one_in(4))
   g->explosion(x, y, 40, 0, true);
  else
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
     if (move_cost(i, j) > 0 && one_in(3))
      add_item(i, j, g->itypes[itm_gasoline], 0);
     if (move_cost(i, j) > 0 && one_in(6))
      add_item(i, j, g->itypes[itm_steel_chunk], 0);
    }
   }
  ter(x, y) = t_rubble;
  break;
 case t_door_c:
 case t_door_b:
 case t_door_locked:
 case t_door_boarded:
  ter(x, y) = t_door_frame;
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(6))
     add_item(i, j, g->itypes[itm_2x4], 0);
   }
  }
  break;
 case t_wall_v:
 case t_wall_h:
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(5))
     add_item(i, j, g->itypes[itm_rock], 0);
    if (move_cost(i, j) > 0 && one_in(4))
     add_item(i, j, g->itypes[itm_2x4], 0);
   }
  }
  ter(x, y) = t_rubble;
  break;
 default:
  if (has_flag(explodes, x, y))
   g->explosion(x, y, 40, 0, true);
  ter(x, y) = t_rubble;
 }
 if (makesound)
  g->sound(x, y, 40, "SMASH!!");
}

void map::shoot(game *g, int x, int y, int &dam, bool hit_items, unsigned flags)
{

 if (has_flag(alarmed, x, y) && !g->event_queued(EVENT_WANTED)) {
  g->sound(g->u.posx, g->u.posy, 30, "An alarm sounds!");
  g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
 }

 switch (ter(x, y)) {

 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  dam -= rng(15, 30);
  if (dam > 0)
   ter(x, y) = t_door_b;
  break;

 case t_door_b:
  if (hit_items || one_in(8)) {	// 1 in 8 chance of hitting the door
   dam -= rng(10, 30);
   if (dam > 0)
    ter(x, y) = t_door_frame;
  } else
   dam -= rng(0, 1);
  break;

 case t_door_boarded:
  dam -= rng(15, 35);
  if (dam > 0)
   ter(x, y) = t_door_b;
  break;

 case t_window:
 case t_window_alarm:
  dam -= rng(0, 5);
  ter(x, y) = t_window_frame;
  break;

 case t_window_boarded:
  dam -= rng(10, 30);
  if (dam > 0)
   ter(x, y) = t_window_frame;
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
  dam -= rng(0, 8);
  ter(x, y) = t_floor;
  break;

 case t_paper:
  dam -= rng(4, 16);
  if (dam > 0)
   ter(x, y) = t_dirt;
  if (flags & mfb(IF_AMMO_INCENDIARY))
   add_field(g, x, y, fd_fire, 1);
  break;

 case t_gas_pump:
  if (hit_items || one_in(3)) {
   if (dam > 15) {
    if (flags & mfb(IF_AMMO_INCENDIARY) || flags & mfb(IF_AMMO_FLAME))
     g->explosion(x, y, 40, 0, true);
    else {
     for (int i = x - 2; i <= x + 2; i++) {
      for (int j = y - 2; j <= y + 2; j++) {
       if (move_cost(i, j) > 0 && one_in(3))
        add_item(i, j, g->itypes[itm_gasoline], 0);
      }
     }
    }
    ter(x, y) = t_gas_pump_smashed;
   }
   dam -= 60;
  }
  break;

 case t_vat:
  if (dam >= 10) {
   g->sound(x, y, 15, "ke-rash!");
   ter(x, y) = t_floor;
  } else
   dam = 0;
  break;

 default:
  if (move_cost(x, y) == 0 && !trans(x, y))
   dam = 0;	// TODO: Bullets can go through some walls?
  else
   dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
 }

 if (flags & mfb(IF_AMMO_TRAIL) && !one_in(4))
  add_field(g, x, y, fd_smoke, rng(1, 2));

// Now, destroy items on that tile.

 if ((move_cost(x, y) == 2 && !hit_items) || !inbounds(x, y))
  return;	// Items on floor-type spaces won't be shot up.

 bool destroyed;
 for (int i = 0; i < i_at(x, y).size(); i++) {
  destroyed = false;
  switch (i_at(x, y)[i].type->m1) {
   case GLASS:
   case PAPER:
    if (dam > rng(2, 8) && one_in(i_at(x, y)[i].volume()))
     destroyed = true;
    break;
   case PLASTIC:
    if (dam > rng(2, 10) && one_in(i_at(x, y)[i].volume() * 3))
     destroyed = true;
    break;
   case VEGGY:
   case FLESH:
    if (dam > rng(10, 40))
     destroyed = true;
    break;
   case COTTON:
   case WOOL:
    i_at(x, y)[i].damage++;
    if (i_at(x, y)[i].damage >= 5)
     destroyed = true;
    break;
  }
  if (destroyed) {
   for (int j = 0; j < i_at(x, y)[i].contents.size(); j++)
    i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
   i_rem(x, y, i);
   i--;
  }
 }
}

bool map::hit_with_acid(game *g, int x, int y)
{
 if (move_cost(x, y) != 0)
  return false; // Didn't hit the tile!

 switch (ter(x, y)) {
  case t_wall_glass_v:
  case t_wall_glass_h:
  case t_wall_glass_v_alarm:
  case t_wall_glass_h_alarm:
  case t_vat:
   ter(x, y) = t_floor;
   break;

  case t_door_c:
  case t_door_locked:
  case t_door_locked_alarm:
   if (one_in(3))
    ter(x, y) = t_door_b;
   break;

  case t_door_b:
   if (one_in(4))
    ter(x, y) = t_door_frame;
   else
    return false;
   break;

  case t_window:
  case t_window_alarm:
   ter(x, y) = t_window_empty;
   break;

  case t_wax:
   ter(x, y) = t_floor_wax;
   break;

  case t_toilet:
  case t_gas_pump:
  case t_gas_pump_smashed:
   return false;

  case t_card_science:
  case t_card_military:
   ter(x, y) = t_card_reader_broken;
   break;
 }

 return true;
}

void map::marlossify(int x, int y)
{
 int type = rng(1, 9);
 switch (type) {
  case 1:
  case 2:
  case 3:
  case 4: ter(x, y) = t_fungus;      break;
  case 5:
  case 6:
  case 7: ter(x, y) = t_marloss;     break;
  case 8: ter(x, y) = t_tree_fungal; break;
  case 9: ter(x, y) = t_slime;       break;
 }
}

bool map::open_door(int x, int y, bool inside)
{
 if (ter(x, y) == t_door_c) {
  ter(x, y) = t_door_o;
  return true;
 } else if (ter(x, y) == t_door_metal_c) {
  ter(x, y) = t_door_metal_o;
  return true;
 } else if (ter(x, y) == t_door_glass_c) {
  ter(x, y) = t_door_glass_o;
  return true;
 } else if (inside &&
            (ter(x, y) == t_door_locked || ter(x, y) == t_door_locked_alarm)) {
  ter(x, y) = t_door_o;
  return true;
 }
 return false;
}

void map::translate(ter_id from, ter_id to)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE(); x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE(); y++) {
   if (ter(x, y) == from)
    ter(x, y) = to;
  }
 }
}

bool map::close_door(int x, int y)
{
 if (ter(x, y) == t_door_o) {
  ter(x, y) = t_door_c;
  return true;
 } else if (ter(x, y) == t_door_metal_o) {
  ter(x, y) = t_door_metal_c;
  return true;
 } else if (ter(x, y) == t_door_glass_o) {
  ter(x, y) = t_door_glass_c;
  return true;
 }
 return false;
}

int& map::radiation(int x, int y)
{
 if (!inbounds(x, y)) {
  nulrad = 0;
  return nulrad;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 return grid[nonant].rad[x][y];
}

std::vector<item>& map::i_at(int x, int y)
{
 if (!inbounds(x, y)) {
  nulitems.clear();
  return nulitems;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 return grid[nonant].itm[x][y];
}

item map::water_from(int x, int y)
{
 item ret((*itypes)[itm_water], 0);
 if (ter(x, y) == t_water_sh && one_in(3))
  ret.poison = rng(1, 4);
 else if (ter(x, y) == t_water_dp && one_in(4))
  ret.poison = rng(1, 4);
 else if (ter(x, y) == t_sewage)
  ret.poison = rng(1, 7);
 else if (ter(x, y) == t_toilet && !one_in(3))
  ret.poison = rng(1, 3);

 return ret;
}

void map::i_rem(int x, int y, int index)
{
 if (index > i_at(x, y).size() - 1)
  return;
 i_at(x, y).erase(i_at(x, y).begin() + index);
}

void map::i_clear(int x, int y)
{
 i_at(x, y).clear();
}

point map::find_item(item *it)
{
 point ret;
 for (ret.x = 0; ret.x < SEEX * my_MAPSIZE(); ret.x++) {
  for (ret.y = 0; ret.y < SEEY * my_MAPSIZE(); ret.y++) {
   for (int i = 0; i < i_at(ret.x, ret.y).size(); i++) {
    if (it == &i_at(ret.x, ret.y)[i])
     return ret;
   }
  }
 }
 ret.x = -1;
 ret.y = -1;
 return ret;
}

void map::add_item(int x, int y, itype* type, int birthday)
{
 item tmp(type, birthday);
 tmp = tmp.in_its_container(itypes);
 if (tmp.made_of(LIQUID) && has_flag(swimmable, x, y))
  return;
 add_item(x, y, tmp);
}

void map::add_item(int x, int y, item new_item)
{
 if (!inbounds(x, y))
  return;
 if (new_item.made_of(LIQUID) && has_flag(swimmable, x, y))
  return;
 if (has_flag(noitem, x, y) || i_at(x, y).size() >= 26) {// Too many items there
  std::vector<point> okay;
  for (int i = x - 1; i <= x + 1; i++) {
   for (int j = y - 1; j <= y + 1; j++) {
    if (inbounds(i, j) && move_cost(i, j) > 0 && !has_flag(noitem, i, j) &&
        i_at(i, j).size() < 26)
     okay.push_back(point(i, j));
   }
  }
  if (okay.size() == 0) {
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
     if (inbounds(i, j) && move_cost(i, j) > 0 && !has_flag(noitem, i, j) &&
         i_at(i, j).size() < 26)
      okay.push_back(point(i, j));
    }
   }
  }
  if (okay.size() == 0)	// STILL?
   return;
  point choice = okay[rng(0, okay.size() - 1)];
  add_item(choice.x, choice.y, new_item);
  return;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 grid[nonant].itm[x][y].push_back(new_item);
}

void map::process_active_items(game *g)
{
 it_tool* tmp;
 iuse use;
 for (int i = 0; i < SEEX * my_MAPSIZE(); i++) {
  for (int j = 0; j < SEEY * my_MAPSIZE(); j++) {
   for (int n = 0; n < i_at(i, j).size(); n++) {
    if (i_at(i, j)[n].active) {
     tmp = dynamic_cast<it_tool*>(i_at(i, j)[n].type);
     (use.*tmp->use)(g, &(g->u), &i_at(i, j)[n], true);
     if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge == 0)
      i_at(i, j)[n].charges--;
     if (i_at(i, j)[n].charges <= 0) {
      (use.*tmp->use)(g, &(g->u), &i_at(i, j)[n], false);
      if (tmp->revert_to == itm_null) {
       i_at(i, j).erase(i_at(i, j).begin() + n);
       n--;
      } else
       i_at(i, j)[n].type = g->itypes[tmp->revert_to];
     }
    }
   }
  }
 }
}

void map::use_amount(point origin, int range, itype_id type, int quantity,
                     bool use_container)
{
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin.x, origin.y, x, y) < radius)
     y++; // Skip over already-examined tiles
    else {
     for (int n = 0; n < i_at(x, y).size() && quantity > 0; n++) {
      item* curit = &(i_at(x, y)[n]);
      bool used_contents = false;
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        quantity--;
        curit->contents.erase(curit->contents.begin() + m);
        m--;
        used_contents = true;
       }
      }
      if (use_container && used_contents) {
       i_rem(x, y, n);
       n--;
      } else if (curit->type->id == type && quantity > 0) {
       quantity--;
       i_rem(x, y, n);
       n--;
      }
     }
    }
   }
  }
 }
}

void map::use_charges(point origin, int range, itype_id type, int quantity)
{
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin.x, origin.y, x, y) < radius)
     y++; // Skip over already-examined tiles
    else {
     for (int n = 0; n < i_at(x, y).size(); n++) {
      item* curit = &(i_at(x, y)[n]);
// Check contents first
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        if (curit->contents[m].charges <= quantity) {
         quantity -= curit->contents[m].charges;
         if (curit->contents[m].destroyed_at_zero_charges()) {
          curit->contents.erase(curit->contents.begin() + m);
          m--;
         } else
          curit->contents[m].charges = 0;
        } else {
         curit->contents[m].charges -= quantity;
         return;
        }
       }
      }
// Now check the actual item
      if (curit->type->id == type) {
       if (curit->charges <= quantity) {
        quantity -= curit->charges;
        if (curit->destroyed_at_zero_charges()) {
         i_rem(x, y, n);
         n--;
        } else
         curit->charges = 0;
       } else {
        curit->charges -= quantity;
        return;
       }
      }
     }
    }
   }
  }
 }
}
 
trap_id& map::tr_at(int x, int y)
{
 if (!inbounds(x, y)) {
  nultrap = tr_null;
  return nultrap;	// Out-of-bounds, return our null trap
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 if (x < 0 || x >= SEEX || y < 0 || y >= SEEY) {
  debugmsg("tr_at contained bad x:y %d:%d", x, y);
  nultrap = tr_null;
  return nultrap;	// Out-of-bounds, return our null trap
 }
 
 return grid[nonant].trp[x][y];
}

void map::add_trap(int x, int y, trap_id t)
{
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 grid[nonant].trp[x][y] = t;
}

void map::disarm_trap(game *g, int x, int y)
{
 if (tr_at(x, y) == tr_null) {
  debugmsg("Tried to disarm a trap where there was none (%d %d)", x, y);
  return;
 }
 int diff = g->traps[tr_at(x, y)]->difficulty;
 int roll = rng(g->u.sklevel[sk_traps], 4 * g->u.sklevel[sk_traps]);
 while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50)
  roll++;
 if (roll >= diff) {
  g->add_msg("You disarm the trap!");
  std::vector<itype_id> comp = g->traps[tr_at(x, y)]->components;
  for (int i = 0; i < comp.size(); i++) {
   if (comp[i] != itm_null)
    add_item(x, y, g->itypes[comp[i]], 0);
  }
  tr_at(x, y) = tr_null;
 } else if (roll >= diff * .8)
  g->add_msg("You fail to disarm the trap.");
 else {
  g->add_msg("You fail to disarm the trap, and you set it off!");
  trap* tr = g->traps[tr_at(x, y)];
  trapfunc f;
  (f.*(tr->act))(g, x, y);
 }
}
 
field& map::field_at(int x, int y)
{
 if (!inbounds(x, y)) {
  nulfield = field();
  return nulfield;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 return grid[nonant].fld[x][y];
}

bool map::add_field(game *g, int x, int y, field_id t, unsigned char density)
{
 if (field_at(x, y).type == fd_web && t == fd_fire)
  density++;
 else if (!field_at(x, y).is_null()) // Blood & bile are null too
  return false;
 if (density > 3)
  density = 3;
 if (density <= 0)
  return false;
 field_at(x, y) = field(t, density, 0);
 if (g != NULL && x == g->u.posx && y == g->u.posy &&
     field_at(x, y).is_dangerous()) {
  g->cancel_activity();
  g->add_msg("You're in a %s!", fieldlist[t].name[density - 1].c_str());
 }
 return true;
}

computer* map::computer_at(int x, int y)
{
 if (!inbounds(x, y))
  return NULL;
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;
 if (grid[nonant].comp.name == "")
  return NULL;
 return &(grid[nonant].comp);
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(game *g, WINDOW* w)
{
 int t = 0;
 int light = g->u.sight_range(g->light_level());
 for  (int realx = g->u.posx - SEEX; realx <= g->u.posx + SEEX; realx++) {
  for (int realy = g->u.posy - SEEY; realy <= g->u.posy + SEEY; realy++) {
   int dist = rl_dist(g->u.posx, g->u.posy, realx, realy);
   if (dist > light) {
    if (g->u.has_disease(DI_BOOMERED))
     mvwputch(w, realx+SEEX - g->u.posx, realy+SEEY - g->u.posy, c_magenta,'#');
    else
     mvwputch(w, realx+SEEX - g->u.posx, realy+SEEY - g->u.posy, c_dkgray, '#');
   } else if (dist <= g->u.clairvoyance() ||
              sees(g->u.posx, g->u.posy, realx, realy, light, t))
    drawsq(w, g->u, realx, realy, false, true);
  }
 }
 mvwputch(w, SEEY, SEEX, g->u.color(), '@');
}

void map::drawsq(WINDOW* w, player &u, int x, int y, bool invert,
                 bool show_items)
{
 if (!inbounds(x, y))
  return;	// Out of bounds
 int k = x + SEEX - u.posx;
 int j = y + SEEY - u.posy;
 nc_color tercol;
 char sym = terlist[ter(x, y)].sym;
 bool hi = false;
 if (u.has_disease(DI_BOOMERED))
  tercol = c_magenta;
 else if ((u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) ||
          u.has_active_bionic(bio_night_vision))
  tercol = c_ltgreen;
 else
  tercol = terlist[ter(x, y)].color;
 if (move_cost(x, y) == 0 && has_flag(swimmable, x, y) && !u.underwater)
  show_items = false;	// Can only see underwater items if WE are underwater
// If there's a trap here, and we have sufficient perception, draw that instead
 if (tr_at(x, y) != tr_null &&
     u.per_cur - u.encumb(bp_eyes) >= (*traps)[tr_at(x, y)]->visibility) {
  tercol = (*traps)[tr_at(x, y)]->color;
  if ((*traps)[tr_at(x, y)]->sym == '%') {
   switch(rng(1, 5)) {
    case 1: sym = '*'; break;
    case 2: sym = '0'; break;
    case 3: sym = '8'; break;
    case 4: sym = '&'; break;
    case 5: sym = '+'; break;
   }
  } else
   sym = (*traps)[tr_at(x, y)]->sym;
 }
// If there's a field here, draw that instead (unless its symbol is %)
 if (field_at(x, y).type != fd_null) {
  tercol = fieldlist[field_at(x, y).type].color[field_at(x, y).density - 1];
  if (fieldlist[field_at(x, y).type].sym == '*') {
   switch (rng(1, 5)) {
    case 1: sym = '*'; break;
    case 2: sym = '0'; break;
    case 3: sym = '8'; break;
    case 4: sym = '&'; break;
    case 5: sym = '+'; break;
   }
  } else if (fieldlist[field_at(x, y).type].sym != '%')
   sym = fieldlist[field_at(x, y).type].sym;
 }
// If there's items here, draw those instead
 if (show_items && i_at(x, y).size() > 0 && field_at(x, y).is_null()) {
  if ((terlist[ter(x, y)].sym != '.'))
   hi = true;
  else {
   tercol = i_at(x, y)[i_at(x, y).size() - 1].color();
   if (i_at(x, y).size() > 1)
    invert = !invert;
   sym = i_at(x, y)[i_at(x, y).size() - 1].symbol();
  }
 }
 if (invert)
  mvwputch_inv(w, j, k, tercol, sym);
 else if (hi)
  mvwputch_hi (w, j, k, tercol, sym);
 else
  mvwputch    (w, j, k, tercol, sym);
}

/*
map::sees based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
bool map::sees(int Fx, int Fy, int Tx, int Ty, int range, int &tc)
{
 int dx = Tx - Fx;
 int dy = Ty - Fy;
 int ax = abs(dx) << 1;
 int ay = abs(dy) << 1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;
 
 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (inbounds(x,y)));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (inbounds(x,y)));
  }
  return false;
 }
}

bool map::clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min,
                     int cost_max, int &tc)
{
 int dx = Tx - Fx;
 int dy = Ty - Fy;
 int ax = abs(dx) << 1;
 int ay = abs(dy) << 1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;
 
 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            inbounds(x, y));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            inbounds(x,y));
  }
  return false;
 }
}

// Bash defaults to true.
std::vector<point> map::route(int Fx, int Fy, int Tx, int Ty, bool bash)
{
/* TODO: If the origin or destination is out of bound, figure out the closest
 * in-bounds point and go to that, then to the real origin/destination.
 */

 if (!inbounds(Fx, Fy) || !inbounds(Tx, Ty)) {
  int linet;
  if (sees(Fx, Fy, Tx, Ty, -1, linet))
   return line_to(Fx, Fy, Tx, Ty, linet);
  else {
   std::vector<point> empty;
   return empty;
  }
 }
// First, check for a simple straight line on flat ground
 int linet = 0;
 if (clear_path(Fx, Fy, Tx, Ty, -1, 2, 2, linet))
  return line_to(Fx, Fy, Tx, Ty, linet);
/*
 if (move_cost(Tx, Ty) == 0)
  debugmsg("%d:%d wanted to move to %d:%d, a %s!", Fx, Fy, Tx, Ty,
           tername(Tx, Ty).c_str());
 if (move_cost(Fx, Fy) == 0)
  debugmsg("%d:%d, a %s, wanted to move to %d:%d!", Fx, Fy,
           tername(Fx, Fy).c_str(), Tx, Ty);
*/
 std::vector<point> open;
 astar_list list[SEEX * my_MAPSIZE()][SEEY * my_MAPSIZE()];
 int score	[SEEX * my_MAPSIZE()][SEEY * my_MAPSIZE()];
 int gscore	[SEEX * my_MAPSIZE()][SEEY * my_MAPSIZE()];
 point parent	[SEEX * my_MAPSIZE()][SEEY * my_MAPSIZE()];
 int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
 if (Tx < Fx) {
  startx = Tx - 4;
  endx = Fx + 4;
 }
 if (Ty < Fy) {
  starty = Ty - 4;
  endy = Fy + 4;
 }
 if (startx < 0)
  startx = 0;
 if (starty < 0)
  starty = 0;
 if (endx > SEEX * my_MAPSIZE() - 1)
  endx = SEEX * my_MAPSIZE() - 1;
 if (endy > SEEY * my_MAPSIZE() - 1)
  endy = SEEY * my_MAPSIZE() - 1;

 for (int x = startx; x <= endx; x++) {
  for (int y = starty; y <= endy; y++) {
   list  [x][y] = ASL_NONE; // Init to not being on any list
   score [x][y] = 0;        // No score!
   gscore[x][y] = 0;        // No score!
   parent[x][y] = point(-1, -1);
  }
 }
 list[Fx][Fy] = ASL_OPEN;
 open.push_back(point(Fx, Fy));

 bool done = false;

 do {
  //debugmsg("Open.size() = %d", open.size());
  int best = 9999;
  int index = -1;
  for (int i = 0; i < open.size(); i++) {
   if (i == 0 || score[open[i].x][open[i].y] < best) {
    best = score[open[i].x][open[i].y];
    index = i;
   }
  }
  for (int x = open[index].x - 1; x <= open[index].x + 1; x++) {
   for (int y = open[index].y - 1; y <= open[index].y + 1; y++) {
    if (x == open[index].x && y == open[index].y)
     y++;	// Skip the current square
    if (x == Tx && y == Ty) {
     done = true;
     parent[x][y] = open[index];
    } else if (x >= startx && x <= endx && y >= starty && y <= endy &&
               (move_cost(x, y) > 0 || (bash && has_flag(bashable, x, y)))) {
     if (list[x][y] == ASL_NONE) {	// Not listed, so make it open
      list[x][y] = ASL_OPEN;
      open.push_back(point(x, y));
      parent[x][y] = open[index];
      gscore[x][y] = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       gscore[x][y] += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (bash && has_flag(bashable, x, y)))
       gscore[x][y] += 18;	// Worst case scenario with damage penalty
      score[x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
     } else if (list[x][y] == ASL_OPEN) { // It's open, but make it our child
      int newg = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       newg += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (bash && has_flag(bashable, x, y)))
       newg += 18;	// Worst case scenario with damage penalty
      if (newg < gscore[x][y]) {
       gscore[x][y] = newg;
       parent[x][y] = open[index];
       score [x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
      }
     }
    }
   }
  }
  list[open[index].x][open[index].y] = ASL_CLOSED;
  open.erase(open.begin() + index);
 } while (!done && open.size() > 0);

 std::vector<point> tmp;
 std::vector<point> ret;
 if (done) {
  point cur(Tx, Ty);
  while (cur.x != Fx || cur.y != Fy) {
   //debugmsg("Retracing... (%d:%d) => [%d:%d] => (%d:%d)", Tx, Ty, cur.x, cur.y, Fx, Fy);
   tmp.push_back(cur);
   if (rl_dist(cur.x, cur.y, parent[cur.x][cur.y].x, parent[cur.x][cur.y].y)>1){
    debugmsg("Jump in our route! %d:%d->%d:%d", cur.x, cur.y,
             parent[cur.x][cur.y].x, parent[cur.x][cur.y].y);
    return ret;
   }
   cur = parent[cur.x][cur.y];
  }
  for (int i = tmp.size() - 1; i >= 0; i--)
   ret.push_back(tmp[i]);
 }
 return ret;
}

void map::save(overmap *om, unsigned int turn, int x, int y)
{
 for (int gridx = 0; gridx < my_MAPSIZE(); gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE(); gridy++)
   saven(om, turn, x, y, gridx, gridy);
 }
}

void map::load(game *g, int wx, int wy)
{
 for (int gridx = 0; gridx < my_MAPSIZE(); gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE(); gridy++) {
   if (!loadn(g, wx, wy, gridx, gridy))
    loadn(g, wx, wy, gridx, gridy);
  }
 }
}

void map::shift(game *g, int wx, int wy, int sx, int sy)
{
// Special case of 0-shift; refresh the map
 if (sx == 0 && sy == 0) {
  for (int gridx = 0; gridx < my_MAPSIZE(); gridx++) {
   for (int gridy = 0; gridy < my_MAPSIZE(); gridy++) {
    if (!loadn(g, wx+sx, wy+sy, gridx, gridy))
     loadn(g, wx+sx, wy+sy, gridx, gridy);
   }
  }
  return;
 }
// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// wx and wy are our position in the world, for saving/loading purposes.

 if (sx >= 0) {
  for (int gridx = 0; gridx < my_MAPSIZE(); gridx++) {
   if (sy >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE(); gridy++) {
     if (gridx < sx || gridy < sy)
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     if (gridx + sx < my_MAPSIZE() && gridy + sy < my_MAPSIZE())
      copy_grid(gridx + gridy * my_MAPSIZE(),
                gridx + sx + (gridy + sy) * my_MAPSIZE());
     else if (!loadn(g, wx + sx, wy + sy, gridx, gridy))
      loadn(g, wx + sx, wy + sy, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE() - 1; gridy >= 0; gridy--) {
     if (gridx < sx || gridy - my_MAPSIZE() >= sy)
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     if (gridx + sx < my_MAPSIZE() && gridy + sy >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE(),
                gridx + sx + (gridy + sy) * my_MAPSIZE());
     else if (!loadn(g, wx + sx, wy + sy, gridx, gridy))
      loadn(g, wx + sx, wy + sy, gridx, gridy);
    }
   }
  }
 } else { // sx < 0; work through it backwards
  for (int gridx = my_MAPSIZE() - 1; gridx >= 0; gridx--) {
   if (sy >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE(); gridy++) {
     if (gridx - my_MAPSIZE() >= sx || gridy < sy)
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     if (gridx + sx >= 0 && gridy + sy < my_MAPSIZE())
      copy_grid(gridx + gridy * my_MAPSIZE(),
                gridx + sx + (gridy + sy) * my_MAPSIZE());
     else if (!loadn(g, wx + sx, wy + sy, gridx, gridy))
      loadn(g, wx + sx, wy + sy, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE() - 1; gridy >= 0; gridy--) {
     if (gridx - my_MAPSIZE() >= sx || gridy - my_MAPSIZE() >= sy)
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     if (gridx + sx >= 0 && gridy + sy >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE(),
                gridx + sx + (gridy + sy) * my_MAPSIZE());
     else if (!loadn(g, wx + sx, wy + sy, gridx, gridy))
      loadn(g, wx + sx, wy + sy, gridx, gridy);
    }
   }
  }
 }
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
void map::saven(overmap *om, unsigned int turn, int worldx, int worldy,
                int gridx, int gridy)
{
 int n = gridx + gridy * my_MAPSIZE();
// Open appropriate file.
 char buff[32];
 sprintf(buff, "save/m.%d.%d.%d", om->posx * OMAPX * 2 + worldx + gridx,
                                  om->posy * OMAPY * 2 + worldy + gridy,
                                  om->posz);
 std::ofstream fout;
 fout.open(buff);

// Dump the turn this was last visited on.
 fout << turn << std::endl;
// Dump the terrain. Add 33 to ter to get a human-readable number or symbol.
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++)
   fout << int(grid[n].ter[i][j]) << " ";
  fout << std::endl;
 }
// Dump the radiation
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++) 
   fout << grid[n].rad[i][j] << " ";
 }
 fout << std::endl;

// Items section; designate it with an I.  Then check itm[][] for each square
//   in the grid and print the coords and the item's details.
// Also, this wastes space since we print the coords for each item, when we
//   could be printing a list of items for each coord (except the empty ones)
 item tmp;
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++) {
   for (int k = 0; k < grid[n].itm[i][j].size(); k++) {
    tmp = grid[n].itm[i][j][k];
    fout << "I " << i << " " << j << std::endl;
    fout << tmp.save_info() << std::endl;
    for (int l = 0; l < tmp.contents.size(); l++)
     fout << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
   }
  }
 }
// Output the traps
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++) {
   if (grid[n].trp[i][j] != tr_null)
    fout << "T " << i << " " << j << " " << grid[n].trp[i][j] <<
    std::endl;
  }
 }

// Output the fields
 field tmpf;
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++) {
   tmpf = grid[n].fld[i][j];
   if (tmpf.type != fd_null)
    fout << "F " << i << " " << j << " " << int(tmpf.type) << " " <<
            int(tmpf.density) << " " << tmpf.age << std::endl;
  }
 }
// Output the spawn points
 spawn_point tmpsp;
 for (int i = 0; i < grid[n].spawns.size(); i++) {
  tmpsp = grid[n].spawns[i];
  fout << "S " << int(tmpsp.type) << " " << tmpsp.count << " " << tmpsp.posx <<
          " " << tmpsp.posy << " " << tmpsp.faction_id << " " <<
          tmpsp.mission_id << (tmpsp.friendly ? " 1 " : " 0 ") <<
          tmpsp.name << std::endl;
 }
// Output the computer
 if (grid[n].comp.name != "")
  fout << "c " << grid[n].comp.save_data() << std::endl;
// Close the file; that's all we need.
 fout.close();
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2
bool map::loadn(game *g, int worldx, int worldy, int gridx, int gridy)
{
 char fname[32];
 char line[SEEX];
 char ch = 0;
 int gridn = gridx + gridy * my_MAPSIZE();
 int itx, ity, t, d, a, old_turn;
 bool fields_here = false;
 item it_tmp;
 std::ifstream mapin;
 std::string databuff;
 grid[gridn].comp = computer();

 sprintf(fname, "save/m.%d.%d.%d", g->cur_om.posx * OMAPX * 2 + worldx + gridx,
                                   g->cur_om.posy * OMAPY * 2 + worldy + gridy,
                                   g->cur_om.posz);
 mapin.open(fname);
 if (mapin.is_open()) {
// Load turn number
  mapin >> old_turn;
// Turns since last visited
  int turndif = (int(g->turn) > old_turn ? int(g->turn) - old_turn : 0);
  mapin.getline(line, 1);
// Load terrain
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int tmpter;
    mapin >> tmpter;
    grid[gridn].ter[i][j] = ter_id(tmpter);
    grid[gridn].itm[i][j].clear();
    grid[gridn].trp[i][j] = tr_null;
    grid[gridn].fld[i][j] = field();
   }
  }
// Load irradiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int radtmp;
    mapin >> radtmp;
    radtmp -= int(turndif / 100);	// Radiation slowly decays
    if (radtmp < 0)
     radtmp = 0;
    grid[gridn].rad[i][j] = radtmp;
   }
  }
// Load items and traps and fields and spawn points
  while (!mapin.eof()) {
   t = 0;
   mapin >> ch;
   if (!mapin.eof() && ch == 'I') {
    mapin >> itx >> ity;
    getline(mapin, databuff); // Clear out the endline
    getline(mapin, databuff);
    it_tmp.load_info(databuff, g);
    grid[gridn].itm[itx][ity].push_back(it_tmp);
   } else if (!mapin.eof() && ch == 'C') {
    getline(mapin, databuff); // Clear out the endline
    getline(mapin, databuff);
    int index = grid[gridn].itm[itx][ity].size() - 1;
    it_tmp.load_info(databuff, g);
    grid[gridn].itm[itx][ity][index].put_in(it_tmp);
   } else if (!mapin.eof() && ch == 'T') {
    mapin >> itx >> ity >> t;
    grid[gridn].trp[itx][ity] = trap_id(t);
   } else if (!mapin.eof() && ch == 'F') {
    fields_here = true;
    mapin >> itx >> ity >> t >> d >> a;
    grid[gridn].fld[itx][ity] = field(field_id(t), d, a);
   } else if (!mapin.eof() && ch == 'S') {
    char tmpfriend;
    int tmpfac = -1, tmpmis = -1;
    std::string spawnname;
    mapin >> t >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
    spawn_point tmp(mon_id(t), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                    spawnname);
    grid[gridn].spawns.push_back(tmp);
   } else if (!mapin.eof() && ch == 'c') {
    getline(mapin, databuff);
    grid[gridn].comp.load_data(databuff);
   }
  }
  mapin.close();
  if (fields_here && turndif >= 8) {
   for (int i = 0; i < int(turndif / 8); i++) {
    if (!process_fields(g))
     i = int(turndif / 8) + 1;
   }
  }
 } else {// No data on this area.  Generate some!
  map tmp_map(itypes, mapitems, traps);
// overx, overy is where in the overmap we need to pull data from
// Each overmap square is two nonants; to prevent overlap, generate only at
//  squares divisible by 2.
  int newmapx = worldx + gridx - ((worldx + gridx) % 2);
  int newmapy = worldy + gridy - ((worldy + gridy) % 2);
  if (worldx + gridx < 0)
   newmapx = worldx + gridx;
  if (worldy + gridy < 0)
   newmapy = worldy + gridy;
  if (worldx + gridx < 0)
   newmapx = worldx + gridx;
  tmp_map.generate(g, &(g->cur_om), newmapx, newmapy, int(g->turn));
  mapin.close();
  return false;
 }
 return true;
}

void map::copy_grid(int to, int from)
{
 if (from < 0 || from >= my_MAPSIZE() * my_MAPSIZE() ||
     to   < 0 || to   >= my_MAPSIZE() * my_MAPSIZE()   ) {
  debugmsg("Attempted to copy grid %d to grid %d", from, to);
  return;
 }

 for (int x = 0; x < SEEX; x++) {
  for (int y = 0; y < SEEY; y++) {
   grid[to].ter[x][y] = grid[from].ter[x][y];
   grid[to].itm[x][y] = grid[from].itm[x][y];
   grid[to].trp[x][y] = grid[from].trp[x][y];
   grid[to].fld[x][y] = grid[from].fld[x][y];
   grid[to].rad[x][y] = grid[from].rad[x][y];
  }
 }

 grid[to].comp = grid[from].comp;
 grid[to].spawns.clear();
 for (int i = 0; i < grid[from].spawns.size(); i++)
  grid[to].spawns.push_back(grid[from].spawns[i]);
}

void map::spawn_monsters(game *g)
{
 for (int gx = 0; gx < my_MAPSIZE(); gx++) {
  for (int gy = 0; gy < my_MAPSIZE(); gy++) {
   int n = gx + gy * my_MAPSIZE();
   for (int i = 0; i < grid[n].spawns.size(); i++) {
    for (int j = 0; j < grid[n].spawns[i].count; j++) {
     int tries = 0;
     int mx = grid[n].spawns[i].posx, my = grid[n].spawns[i].posy;
     monster tmp(g->mtypes[grid[n].spawns[i].type]);
     tmp.spawnmapx = g->levx;
     tmp.spawnmapy = g->levy;
     tmp.faction_id = grid[n].spawns[i].faction_id;
     tmp.mission_id = grid[n].spawns[i].mission_id;
     if (grid[n].spawns[i].name != "NONE")
      tmp.unique_name = grid[n].spawns[i].name;
     if (grid[n].spawns[i].friendly)
      tmp.friendly = -1;
     int fx = mx + gx * SEEX, fy = my + gy * SEEY;

     while ((!g->is_empty(fx, fy) || !tmp.can_move_to(g->m, fx, fy)) && 
            tries < 10) {
      mx = (grid[n].spawns[i].posx + rng(-3, 3)) % SEEX;
      my = (grid[n].spawns[i].posy + rng(-3, 3)) % SEEY;
      if (mx < 0)
       mx += SEEX;
      if (my < 0)
       my += SEEY;
      fx = mx + gx * SEEX;
      fy = my + gy * SEEY;
      tries++;
     }
     if (tries != 10) {
      tmp.spawnposx = fx;
      tmp.spawnposy = fy;
      tmp.spawn(fx, fy);
      g->z.push_back(tmp);
     }
    }
   }
   grid[n].spawns.clear();
  }
 }
}

bool map::inbounds(int x, int y)
{
 if (x < 0 || x >= SEEX * my_MAPSIZE() || y < 0 || y >= SEEY * my_MAPSIZE())
  return false;
 return true;
}

tinymap::tinymap()
{
 nulter = t_null;
 nultrap = tr_null;
}

tinymap::tinymap(std::vector<itype*> *itptr,
                 std::vector<itype_id> (*miptr)[num_itloc],
                 std::vector<trap*> *trptr)
{
 nulter = t_null;
 nultrap = tr_null;
 itypes = itptr;
 mapitems = miptr;
 traps = trptr;
 for (int n = 0; n < 4; n++) {
  for (int x = 0; x < SEEX; x++) {
   for (int y = 0; y < SEEY; y++) {
    grid[n].trp[x][y] = tr_null;
    grid[n].rad[x][y] = 0;
   }
  }
 }
}

tinymap::~tinymap()
{
}

/*
void map::cast_to_nonant(int &x, int &y, int &n)
{
 if (x < 0 || x >= SEEX * my_MAPSIZE() || y < 0 || y >= SEEY * my_MAPSIZE()) {
  n = 0;
  x = -1;
  y = -1;
  return;
 }
 n = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE();

 x %= SEEX;
 y %= SEEY;

}
*/
