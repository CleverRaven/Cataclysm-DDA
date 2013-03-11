#include "game.h"
#include "setvector.h"
#include "output.h"
#include "keypress.h"
#include "player.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "crafting.h" // For the use_comps use_tools functions


bool will_flood_stop(map *m, bool (&fill)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                     int x, int y);

void game::init_construction()
{
 int id = -1;
 int tl, cl, sl;

 #define CONSTRUCT(name, difficulty, able, done) \
  sl = -1; id++; \
  constructions.push_back( new constructable(id, name, difficulty, able, done))

 #define STAGE(...)\
  tl = 0; cl = 0; sl++; \
  constructions[id]->stages.push_back(construction_stage(__VA_ARGS__));
 #define TOOL(...)   setvector(constructions[id]->stages[sl].tools[tl], \
                               __VA_ARGS__); tl++
 #define COMP(...)   setvector(constructions[id]->stages[sl].components[cl], \
                               __VA_ARGS__); cl++

/* CONSTRUCT( name, time, able, done )
 * Name is the name as it appears in the menu; 30 characters or less, please.
 * time is the time in MINUTES that it takes to finish this construction.
 *  note that 10 turns = 1 minute.
 * able is a function which returns true if you can build it on a given tile
 *  See construction.h for options, and this file for definitions.
 * done is a function which runs each time the construction finishes.
 *  This is useful, for instance, for removing the trap from a pit, or placing
 *  items after a deconstruction.
 */

 CONSTRUCT("Dig Pit", 0, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
  STAGE(t_pit, 10);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);

 CONSTRUCT("Spike Pit", 0, &construct::able_pit, &construct::done_nothing);
  STAGE(t_pit_spiked, 5);
   COMP(itm_spear_wood, 4, NULL);

 CONSTRUCT("Fill Pit", 0, &construct::able_pit, &construct::done_nothing);
  STAGE(t_pit_shallow, 5);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
  STAGE(t_dirt, 5);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);

 CONSTRUCT("Chop Down Tree", 0, &construct::able_tree, &construct::done_tree);
  STAGE(t_dirt, 10);
   TOOL(itm_ax, itm_primitive_axe, itm_chainsaw_on, NULL);

 CONSTRUCT("Chop Up Log", 0, &construct::able_log, &construct::done_log);
  STAGE(t_dirt, 20);
   TOOL(itm_ax, itm_primitive_axe, itm_chainsaw_on, NULL);

 CONSTRUCT("Move Furniture", -1, &construct::able_furniture, &construct::done_furniture);
  STAGE(t_null, 1);

 CONSTRUCT("Clean Broken Window", 0, &construct::able_broken_window,
                                     &construct::done_nothing);
  STAGE(t_window_empty, 5);

/* CONSTRUCT("Remove Window Pane",  1, &construct::able_window_pane,
                                     &construct::done_window_pane);
  STAGE(t_window_empty, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_rock, itm_hatchet, NULL);
   TOOL(itm_screwdriver, itm_knife_butter, itm_toolset, NULL);
*/

 CONSTRUCT("Repair Door", 1, &construct::able_door_broken,
                             &construct::done_nothing);
  STAGE(t_door_c, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 3, NULL);
   COMP(itm_nail, 12, NULL);

 CONSTRUCT("Board Up Door", 0, &construct::able_door, &construct::done_nothing);
  STAGE(t_door_boarded, 8);
   TOOL(itm_hammer, itm_hammer_sledge, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 4, NULL);
   COMP(itm_nail, 8, NULL);

 CONSTRUCT("Board Up Window", 0, &construct::able_window,
                                 &construct::done_nothing);
  STAGE(t_window_boarded, 5);
   TOOL(itm_hammer, itm_hammer_sledge, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 4, NULL);
   COMP(itm_nail, 8, NULL);

 CONSTRUCT("Build Wall", 2, &construct::able_empty, &construct::done_nothing);
  STAGE(t_wall_half, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 10, NULL);
   COMP(itm_nail, 20, NULL);
  STAGE(t_wall_wood, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 10, NULL);
   COMP(itm_nail, 20, NULL);

 CONSTRUCT("Build Log Wall", 2, &construct::able_pit, &construct::done_nothing);
  STAGE(t_wall_log_half, 20);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
   COMP(itm_log, 2, NULL);
   COMP(itm_stick, 3, NULL);
  STAGE(t_wall_log, 20);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
   COMP(itm_log, 2, NULL);
   COMP(itm_stick, 3, NULL);

 CONSTRUCT("Build Palisade Wall", 2, &construct::able_pit, &construct::done_nothing);
  STAGE(t_palisade, 20);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
   COMP(itm_log, 3, NULL);
   COMP(itm_rope_30, 1, itm_rope_6, 5, NULL);

 CONSTRUCT("Build Palisade Gate", 2, &construct::able_pit, &construct::done_nothing);
  STAGE(t_palisade_gate, 20);
   TOOL(itm_shovel, itm_primitive_shovel, NULL);
   COMP(itm_log, 2, NULL);
   COMP(itm_2x4, 3, NULL);
   COMP(itm_rope_30, 1, itm_rope_6, 5, NULL);

 CONSTRUCT("Build Window", 2, &construct::able_empty,
                              &construct::done_nothing);
  STAGE(t_window_empty, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 15, itm_log, 2, NULL);
   COMP(itm_nail, 30, NULL);
  STAGE(t_window, 5);
   COMP(itm_glass_sheet, 1, NULL);
  STAGE(t_window_domestic, 5);
   TOOL(itm_saw, NULL);
   COMP(itm_nail, 4, NULL);
   COMP(itm_sheet, 2, NULL);
   COMP(itm_stick, 1, NULL);


 CONSTRUCT("Build Door", 2, &construct::able_empty,
                              &construct::done_nothing);
  STAGE(t_door_frame, 15);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 12, NULL);
   COMP(itm_nail, 24, NULL);
  STAGE(t_door_c, 15);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 4, NULL);
   COMP(itm_nail, 12, NULL);

 CONSTRUCT("Build Wire Fence",3, &construct::able_dig,
                                 &construct::done_nothing);
  STAGE(t_chainfence_posts, 20);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_rock, NULL);
   COMP(itm_pipe, 6, NULL);
   COMP(itm_scrap, 8, NULL);
  STAGE(t_chainfence_v, 20);
   COMP(itm_wire, 15, NULL);

 CONSTRUCT("Realign Fence",   0, &construct::able_chainlink,
                                 &construct::done_nothing);
  STAGE(t_chainfence_h, 0);
  STAGE(t_chainfence_v, 0);

 CONSTRUCT("Build Wire Gate", 3, &construct::able_between_walls,
                                 &construct::done_nothing);
  STAGE(t_chaingate_c, 15);
   COMP(itm_wire, 20, NULL);
   COMP(itm_steel_chunk, 3, itm_scrap, 12, NULL);
   COMP(itm_pipe, 6, NULL);

/*  Removed until we have some way of auto-aligning fences!
 CONSTRUCT("Build Fence", 1, 15, &construct::able_empty);
  STAGE(t_fence_h, 10);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, NULL);
   COMP(itm_2x4, 5, itm_nail, 8, NULL);
*/

 CONSTRUCT("Build Roof", 3, &construct::able_between_walls,
                            &construct::done_nothing);
  STAGE(t_floor, 40);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_2x4, 8, NULL);
   COMP(itm_nail, 40, NULL);

// Household stuff
 CONSTRUCT("Build Dresser", 1, &construct::able_indoors,
                                &construct::done_nothing);
  STAGE(t_dresser, 20);
   TOOL(itm_saw, NULL);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_nail, 8, NULL);
   COMP(itm_2x4, 6, NULL);

 CONSTRUCT("Build Bookcase", 1, &construct::able_indoors,
                                &construct::done_nothing);
  STAGE(t_bookcase, 20);
   TOOL(itm_saw, NULL);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_nail, 16, NULL);
   COMP(itm_2x4, 12, NULL);

 CONSTRUCT("Build Counter", 0, &construct::able_indoors,
                                &construct::done_nothing);
  STAGE(t_counter, 20);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_nail, 8, NULL);
   COMP(itm_2x4, 6, NULL);

 CONSTRUCT("Build Makeshift Bed", 0, &construct::able_indoors,
                                &construct::done_nothing);
  STAGE(t_makeshift_bed, 20);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   COMP(itm_nail, 8, NULL);
   COMP(itm_2x4, 10, NULL);
   COMP(itm_sheet, 1, NULL);

 CONSTRUCT("Tape up window", 0, &construct::able_window,
                                &construct::done_tape);
  STAGE(t_null, 2);
  COMP(itm_duct_tape, 50, NULL);

 CONSTRUCT("Deconstruct Furniture", 0, &construct::able_deconstruct,
                                &construct::done_deconstruct);
  STAGE(t_null, 20);
   TOOL(itm_hammer, itm_primitive_hammer, itm_hatchet, itm_nailgun, NULL);
   TOOL(itm_screwdriver, itm_toolset, NULL);

 CONSTRUCT("Start vehicle construction", 0, &construct::able_empty, &construct::done_vehicle);
  STAGE(t_null, 10);
   COMP(itm_frame, 1, NULL);

 CONSTRUCT("Fence Posts", 0, &construct::able_dig,
                             &construct::done_nothing);
  STAGE(t_fence_post, 5);
  TOOL(itm_hammer, itm_primitive_hammer, itm_shovel, itm_primitive_shovel, itm_rock, itm_hatchet, itm_ax, itm_primitive_axe, NULL);
  COMP(itm_spear_wood, 2, NULL);

}

void game::construction_menu()
{
 int iMaxY = (VIEWY*2)+1;
 if (constructions.size()+2 < iMaxY)
  iMaxY = constructions.size()+2;
 if (iMaxY < 25)
  iMaxY = 25;

 WINDOW *w_con = newwin(iMaxY, 80, 0, 0);
 wborder(w_con, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_con, 0, 8, c_ltred, " Construction ");

 mvwputch(w_con,  0, 30, c_ltgray, LINE_OXXX);
 mvwputch(w_con, iMaxY-1, 30, c_ltgray, LINE_XXOX);
 for (int i = 1; i < iMaxY-1; i++)
  mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);

 mvwprintz(w_con,  1, 31, c_white, "Difficulty:");

 wrefresh(w_con);

 bool update_info = true;
 int select = 0;
 long ch;

 inventory total_inv = crafting_inventory();

 do {
// Erase existing list of constructions
  for (int i = 1; i < iMaxY-1; i++) {
   for (int j = 1; j < 30; j++)
    mvwputch(w_con, i, j, c_black, ' ');
  }
// Determine where in the master list to start printing
  //int offset = select - 11;
  int offset = 0;
  if (select >= iMaxY-2)
   offset = select - iMaxY + 3;
// Print the constructions between offset and max (or how many will fit)
  for (int i = 0; i < iMaxY-2 && (i + offset) < constructions.size(); i++) {
   int current = i + offset;
   nc_color col = (player_can_build(u, total_inv, constructions[current]) ?
                   c_white : c_dkgray);
   // Map menu items to hotkey letters, skipping j, k, l, and q.
   char hotkey = 97 + current;
   if (hotkey > 122)
    hotkey = hotkey - 58;

   if (current == select)
    col = hilite(col);
   mvwprintz(w_con, 1 + i, 1, col, "%c %s", hotkey, constructions[current]->name.c_str());
  }

  if (update_info) {
   update_info = false;
   constructable* current_con = constructions[select];
// Print difficulty
   int pskill = u.skillLevel("carpentry");
   int diff = current_con->difficulty > 0 ? current_con->difficulty : 0;
   mvwprintz(w_con, 1, 43, (pskill >= diff ? c_white : c_red),
             "%d   ", diff);
// Clear out lines for tools & materials
   for (int i = 2; i < iMaxY-1; i++) {
    for (int j = 31; j < 79; j++)
     mvwputch(w_con, i, j, c_black, ' ');
   }

// Print stages and their requirements
   int posx = 33, posy = 2;
   for (int n = 0; n < current_con->stages.size(); n++) {
     nc_color color_stage = (player_can_build(u, total_inv, current_con, n,
					      false, true) ?
                            c_white : c_dkgray);
    mvwprintz(w_con, posy, 31, color_stage, "Stage %d: %s", n + 1,
              current_con->stages[n].terrain == t_null? "" : terlist[current_con->stages[n].terrain].name.c_str());
    posy++;
// Print tools
    construction_stage stage = current_con->stages[n];
    bool has_tool[3] = {stage.tools[0].empty(),
                        stage.tools[1].empty(),
                        stage.tools[2].empty()};
    for (int i = 0; i < 3 && !has_tool[i]; i++) {
     posy++;
     posx = 33;
     for (int j = 0; j < stage.tools[i].size(); j++) {
      itype_id tool = stage.tools[i][j];
      nc_color col = c_red;
      if (total_inv.has_amount(tool, 1)) {
       has_tool[i] = true;
       col = c_green;
      }
      int length = itypes[tool]->name.length();
      if (posx + length > 79) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, itypes[tool]->name.c_str());
      posx += length + 1; // + 1 for an empty space
      if (j < stage.tools[i].size() - 1) { // "OR" if there's more
       if (posx > 77) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, "OR");
       posx += 3;
      }
     }
    }
// Print components
    posy++;
    posx = 33;
    bool has_component[3] = {stage.components[0].empty(),
                             stage.components[1].empty(),
                             stage.components[2].empty()};
    for (int i = 0; i < 3; i++) {
     posx = 33;
     while (has_component[i])
      i++;
     for (int j = 0; j < stage.components[i].size() && i < 3; j++) {
      nc_color col = c_red;
      component comp = stage.components[i][j];
      if (( itypes[comp.type]->is_ammo() &&
           total_inv.has_charges(comp.type, comp.count)) ||
          (!itypes[comp.type]->is_ammo() &&
           total_inv.has_amount(comp.type, comp.count))) {
       has_component[i] = true;
       col = c_green;
      }
      int length = itypes[comp.type]->name.length();
      if (posx + length > 79) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, "%s x%d",
                itypes[comp.type]->name.c_str(), comp.count);
      posx += length + 3; // + 2 for " x", + 1 for an empty space
// Add more space for the length of the count
      if (comp.count < 10)
       posx++;
      else if (comp.count < 100)
       posx += 2;
      else
       posx += 3;

      if (j < stage.components[i].size() - 1) { // "OR" if there's more
       if (posx > 77) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, "OR");
       posx += 3;
      }
     }
     posy++;
    }
   }
   wrefresh(w_con);
  } // Finished updating

  ch = getch();
  switch (ch) {
   case KEY_DOWN:
    update_info = true;
    if (select < constructions.size() - 1)
     select++;
    else
     select = 0;
    break;
   case KEY_UP:
    update_info = true;
    if (select > 0)
     select--;
    else
     select = constructions.size() - 1;
    break;
   case ' ':
   case KEY_ESCAPE:
    ch = 'q';
    break;
   case '\n':
   default:
    if (ch > 64 && ch < 91) //A-Z
     ch = ch - 65 + 26;

    if (ch > 96 && ch < 123) //a-z
     ch = ch - 97;

    if (ch == '\n')
     ch = select;

    if (ch < constructions.size()) {
     if (player_can_build(u, total_inv, constructions[ch])) {
      place_construction(constructions[ch]);
      ch = 'q';
     } else {
      popup("You can't build that!");
      if (ch != '\n')
       select = ch;
      for (int i = 1; i < iMaxY-1; i++)
       mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
      update_info = true;
     }
    }
    break;
  }
 } while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

 for (int i = iMaxY-25; i < iMaxY+1; i++) {
  for (int j = (VIEWX*2)+1; j < 81; j++)
   mvwputch(w_con, i, j, c_black, ' ');
 }

 wrefresh(w_con);
 refresh_all();
}

bool game::player_can_build(player &p, inventory inv, constructable* con,
                            const int level, bool cont, bool exact_level)
{
 int last_level = level;

 // default behavior: return true if any of the stages up to L can be constr'd
 // if exact_level, require that this level be constructable
 if (p.skillLevel("carpentry") < con->difficulty)
  return false;

 if (level < 0)
  last_level = con->stages.size();

 int start = 0;
 if (cont)
  start = level;

 bool can_build_any = false;
 for (int i = start; i < con->stages.size() && i <= last_level; i++) {
  construction_stage stage = con->stages[i];
  bool has_tool = false;
  bool has_component = false;
  bool tools_required = false;
  bool components_required = false;

  for (int j = 0; j < 3; j++) {
   if (stage.tools[j].size() > 0) {
    tools_required = true;
    has_tool = false;
    for (int k = 0; k < stage.tools[j].size() && !has_tool; k++) {
     if (inv.has_amount(stage.tools[j][k], 1))
      has_tool = true;
    }
    if (!has_tool)  // missing one of the tools for this stage
     break;
   }
   if (stage.components[j].size() > 0) {
    components_required = true;
    has_component = false;
    for (int k = 0; k < stage.components[j].size() && !has_component; k++) {
     if (( itypes[stage.components[j][k].type]->is_ammo() &&
	   inv.has_charges(stage.components[j][k].type,
			   stage.components[j][k].count)    ) ||
         (!itypes[stage.components[j][k].type]->is_ammo() &&
          inv.has_amount (stage.components[j][k].type,
                          stage.components[j][k].count)    ))
      has_component = true;
    }
    if (!has_component)  // missing one of the comps for this stage
     break;
   }

  }  // j in [0,2]
  can_build_any |= (has_component || !components_required) &&
    (has_tool || !tools_required);
  if (exact_level && (i == level)) {
      return ((has_component || !components_required) &&
	      (has_tool || !tools_required));
  }
 }  // stage[i]
 return can_build_any;
}

void game::place_construction(constructable *con)
{
 refresh_all();
 inventory total_inv = crafting_inventory();

 std::vector<point> valid;
 for (int x = u.posx - 1; x <= u.posx + 1; x++) {
  for (int y = u.posy - 1; y <= u.posy + 1; y++) {
   if (x == u.posx && y == u.posy)
    y++;
   construct test;
   bool place_okay = (test.*(con->able))(this, point(x, y));
   for (int i = 0; i < con->stages.size() && !place_okay; i++) {
    if (m.ter(x, y) == con->stages[i].terrain)
     place_okay = true;
   }

   if (place_okay) {
// Make sure we're not trying to continue a construction that we can't finish
    int starting_stage = 0, max_stage = -1;
    for (int i = 0; i < con->stages.size(); i++) {
     if (m.ter(x, y) == con->stages[i].terrain)
      starting_stage = i + 1;
    }
    for(int i = starting_stage; i < con->stages.size(); i++) {
     if (player_can_build(u, total_inv, con, i, true, true))
       max_stage = i;
     else
       break;
    }
    if (max_stage >= starting_stage) {
     valid.push_back(point(x, y));
     m.drawsq(w_terrain, u, x, y, true, false);
     wrefresh(w_terrain);
    }
   }
  }
 }
 mvprintz(0, 0, c_red, "Pick a direction in which to construct:");
 int dirx, diry;
 get_direction(this, dirx, diry, input());
 if (dirx == -2) {
  add_msg("Invalid direction.");
  return;
 }
 dirx += u.posx;
 diry += u.posy;
 bool point_is_okay = false;
 for (int i = 0; i < valid.size() && !point_is_okay; i++) {
  if (valid[i].x == dirx && valid[i].y == diry)
   point_is_okay = true;
 }
 if (!point_is_okay) {
  add_msg("You cannot build there!");
  return;
 }

// Figure out what stage to start at, and what stage is the maximum
 int starting_stage = 0, max_stage = 0;
 for (int i = 0; i < con->stages.size(); i++) {
  if (m.ter(dirx, diry) == con->stages[i].terrain)
   starting_stage = i + 1;
  if (player_can_build(u, total_inv, con, i, true))
   max_stage = i;
 }

 u.assign_activity(ACT_BUILD, con->stages[starting_stage].time * 1000, con->id);

 u.moves = 0;
 std::vector<int> stages;
 for (int i = starting_stage; i <= max_stage; i++)
  stages.push_back(i);
 u.activity.values = stages;
 u.activity.placement = point(dirx, diry);
}

void game::complete_construction()
{
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 int stage_num = u.activity.values[0];
 constructable *built = constructions[u.activity.index];
 construction_stage stage = built->stages[stage_num];
 std::vector<component> player_use;
 std::vector<component> map_use;

 u.practice("carpentry", built->difficulty * 10);
 if (built->difficulty == 0)
   u.practice("carpentry", 10);
 for (int i = 0; i < 3; i++) {
  if (!stage.components[i].empty())
   consume_items(stage.components[i]);
 }

// Make the terrain change
 int terx = u.activity.placement.x, tery = u.activity.placement.y;
 if (stage.terrain != t_null)
  m.ter(terx, tery) = stage.terrain;

// Strip off the first stage in our list...
 u.activity.values.erase(u.activity.values.begin());
// ...and start the next one, if it exists
 if (u.activity.values.size() > 0) {
  construction_stage next = built->stages[u.activity.values[0]];
  u.activity.moves_left = next.time * 1000;
 } else // We're finished!
  u.activity.type = ACT_NULL;

// This comes after clearing the activity, in case the function interrupts
// activities
 construct effects;
 (effects.*(built->done))(this, point(terx, tery));
}

bool construct::able_empty(game *g, point p)
{
 return (g->m.move_cost(p.x, p.y) == 2);
}

bool construct::able_tree(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_tree);
}

bool construct::able_log(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_log);
}

bool construct::able_furniture(game *g, point p)
{
 int required_str = 0;

 switch(g->m.ter(p.x, p.y)) {
  case t_fridge:
  case t_glass_fridge:
   required_str = 10;
   break;
  case t_bookcase:
  case t_locker:
   required_str = 9;
   break;
  case t_dresser:
  case t_rack:
  case t_chair:
  case t_armchair:
  case t_bench:
   required_str = 8;
   break;
  default:
   //Not a furniture we can move
   return false;
 }

 if( g->u.str_cur < required_str ) {
  return false;
 }

 return true;
}

bool construct::able_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_frame ||
         g->m.ter(p.x, p.y) == t_window_empty ||
         g->m.ter(p.x, p.y) == t_window_domestic ||
         g->m.ter(p.x, p.y) == t_window);
}

bool construct::able_empty_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_empty);
}

bool construct::able_window_pane(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window || g->m.ter(p.x, p.y) == t_window_domestic || g->m.ter(p.x, p.y) == t_window_alarm);
}

bool construct::able_broken_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_frame);
}

bool construct::able_door(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_door_c ||
         g->m.ter(p.x, p.y) == t_door_b ||
         g->m.ter(p.x, p.y) == t_door_o ||
         g->m.ter(p.x, p.y) == t_door_locked);
}

bool construct::able_door_broken(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_door_b);
}

bool construct::able_wall(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_wall_h || g->m.ter(p.x, p.y) == t_wall_v ||
         g->m.ter(p.x, p.y) == t_wall_wood);
}

bool construct::able_wall_wood(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_wall_wood);
}

bool construct::able_indoors(game *g, point p)
{
 return (g->m.is_indoor(p.x, p.y));
}

bool construct::able_dig(game *g, point p)
{
 return (g->m.has_flag(diggable, p.x, p.y));
}

bool construct::able_chainlink(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_chainfence_v || g->m.ter(p.x, p.y) == t_chainfence_h);
}

bool construct::able_pit(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_pit);//|| g->m.ter(p.x, p.y) == t_pit_shallow);
}

bool construct::able_between_walls(game *g, point p)
{
 return (g->m.has_flag(supports_roof, p.x -1, p.y) && g->m.has_flag(supports_roof, p.x +1, p.y) ||
         g->m.has_flag(supports_roof, p.x -1, p.y) && g->m.has_flag(supports_roof, p.x, p.y +1) ||
         g->m.has_flag(supports_roof, p.x -1, p.y) && g->m.has_flag(supports_roof, p.x, p.y -1) ||
         g->m.has_flag(supports_roof, p.x +1, p.y) && g->m.has_flag(supports_roof, p.x, p.y +1) ||
         g->m.has_flag(supports_roof, p.x +1, p.y) && g->m.has_flag(supports_roof, p.x, p.y -1) ||
         g->m.has_flag(supports_roof, p.x, p.y -1) && g->m.has_flag(supports_roof, p.x, p.y +1));
}

bool construct::able_deconstruct(game *g, point p)
{
  return (g->m.has_flag(deconstruct, p.x, p.y));
}

void construct::done_window_pane(game *g, point p)
{
 g->m.add_item(g->u.posx, g->u.posy, g->itypes[itm_glass_sheet], 0);
}

void construct::done_furniture(game *g, point p)
{
 mvprintz(0, 0, c_red, "Press a direction for the furniture to move (. to cancel):");
 int x = 0, y = 0;
 //Keep looping until we get a valid direction or a cancel.
 while(true){
  do
   get_direction(g, x, y, input());
  while (x == -2 || y == -2);
  if(x == 0 && y == 0)
   return;
  x += p.x;
  y += p.y;
  if(!g->m.ter(x, y) == t_floor || !g->is_empty(x, y)) {
   mvprintz(0, 0, c_red, "Can't move furniture there! Choose a direction with open floor.");
   continue;
  }
  break;
 }

 g->m.ter(x, y) = g->m.ter(p.x, p.y);
 g->m.ter(p.x, p.y) = t_floor;

 //Move all Items within a container
 std::vector <item> vItemMove = g->m.i_at(p.x, p.y);
 for (int i=0; i < vItemMove.size(); i++)
  g->m.add_item(x, y, vItemMove[i]);

 g->m.i_clear(p.x, p.y);
}

void construct::done_tree(game *g, point p)
{
 mvprintz(0, 0, c_red, "Press a direction for the tree to fall in:");
 int x = 0, y = 0;
 do
  get_direction(g, x, y, input());
 while (x == -2 || y == -2);
 x = p.x + x * 3 + rng(-1, 1);
 y = p.y + y * 3 + rng(-1, 1);
 std::vector<point> tree = line_to(p.x, p.y, x, y, rng(1, 8));
 for (int i = 0; i < tree.size(); i++) {
  g->m.destroy(g, tree[i].x, tree[i].y, true);
  g->m.ter(tree[i].x, tree[i].y) = t_log;
 }
}

void construct::done_log(game *g, point p)
{
 g->m.add_item(p.x, p.y, g->itypes[itm_log], int(g->turn), rng(5, 15));
}


void construct::done_vehicle(game *g, point p)
{
    std::string name = string_input_popup("Enter new vehicle name", 20);
    vehicle *veh = g->m.add_vehicle (g, veh_custom, p.x, p.y, 270);
    if (!veh)
    {
        debugmsg ("error constructing vehicle");
        return;
    }
    veh->name = name;
    veh->install_part (0, 0, vp_frame_v2);
}

void construct::done_tape(game *g, point p)
{
  g->add_msg("You tape up the %s.", g->m.tername(p.x, p.y).c_str());
  switch (g->m.ter(p.x, p.y))
  {
    case t_window_alarm:
      g->m.ter(p.x, p.y) = t_window_alarm_taped;

    case t_window_domestic:
      g->m.ter(p.x, p.y) = t_window_domestic_taped;

    case t_window:
      g->m.ter(p.x, p.y) = t_window_taped;
  }

}

void construct::done_deconstruct(game *g, point p)
{
  g->add_msg("You disassemble the %s.", g->m.tername(p.x, p.y).c_str());
  switch (g->m.ter(p.x, p.y))
  {
    case t_makeshift_bed:
    case t_bed:
    case t_armchair:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 9);
      g->m.add_item(p.x, p.y, g->itypes[itm_rag], 0, 9);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,8));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_door_c:
    case t_door_o:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 3);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,12));
      g->m.ter(p.x, p.y) = t_door_frame;
    break;
    case t_window_domestic:
      g->m.add_item(p.x, p.y, g->itypes[itm_stick], 0);
      g->m.add_item(p.x, p.y, g->itypes[itm_sheet], 0, 1);
      g->m.add_item(p.x, p.y, g->itypes[itm_glass_sheet], 0);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, 3);
      g->m.ter(p.x, p.y) = t_window_empty;
    break;

    case t_window:
      g->m.add_item(p.x, p.y, g->itypes[itm_glass_sheet], 0);
      g->m.ter(p.x, p.y) = t_window_empty;
    break;

    case t_backboard:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 4);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,10));
      g->m.ter(p.x, p.y) = t_pavement;
    break;

    case t_sandbox:
    case t_bench:
    case t_crate_o:
    case t_crate_c:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 4);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,10));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_chair:
    case t_cupboard:
    case t_desk:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 4);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,10));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_slide:
      g->m.add_item(p.x, p.y, g->itypes[itm_steel_plate], 0);
      g->m.add_item(p.x, p.y, g->itypes[itm_pipe], 0, rng(4,8));
      g->m.ter(p.x, p.y) = t_grass;
    break;

    case t_rack:
    case t_monkey_bars:
      g->m.add_item(p.x, p.y, g->itypes[itm_pipe], 0, rng(6,12));
      g->m.ter(p.x, p.y) = t_grass;
    break;

    case t_fridge:
      g->m.add_item(p.x, p.y, g->itypes[itm_scrap], 0, rng(2,6));
      g->m.add_item(p.x, p.y, g->itypes[itm_steel_chunk], 0, rng(2,3));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_counter:
    case t_dresser:
    case t_table:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 6);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,8));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_pool_table:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 4);
      g->m.add_item(p.x, p.y, g->itypes[itm_rag], 0, 4);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(6,10));
      g->m.ter(p.x, p.y) = t_floor;
    break;

    case t_bookcase:
      g->m.add_item(p.x, p.y, g->itypes[itm_2x4], 0, 12);
      g->m.add_item(p.x, p.y, g->itypes[itm_nail], 0, rng(12,16));
      g->m.ter(p.x, p.y) = t_floor;
    break;
  }

}
