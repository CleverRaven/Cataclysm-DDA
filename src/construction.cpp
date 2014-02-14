#include "game.h"
#include "output.h"
#include "keypress.h"
#include "player.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "crafting.h" // For the use_comps use_tools functions
#include "item_factory.h"
#include "catacharset.h"
#include "action.h"
#include "translations.h"
#include "veh_interact.h"

#include <algorithm>

std::vector<construction*> constructions;

std::vector<construction*> constructions_by_desc(const std::string &description) {
    std::vector<construction*> result;
    for(std::vector<construction*>::iterator a = constructions.begin(); a != constructions.end(); ++a) {
        if((*a)->description == description) {
            result.push_back(*a);
        }
    }
    return result;
}

bool will_flood_stop(map *m, bool (&fill)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                     int x, int y);

void construction_menu()
{
    // only display constructions the player can theoretically perform
    std::vector<std::string> available;
    for (unsigned i = 0; i < constructions.size(); ++i) {
        construction *c = constructions[i];
        if (can_construct(c)) {
            bool already_have_it = false;
            for (unsigned j = 0; j < available.size(); ++j) {
                if (available[j] == c->description) {
                    already_have_it = true;
                    break;
                }
            }
            if (!already_have_it) {
                available.push_back(c->description);
            }
        }
    }

    if(available.empty()) {
        popup(_("You can not construct anything here."));
        return;
    }

    int iMaxY = TERMY;
    if (available.size()+2 < iMaxY) {
        iMaxY = available.size()+2;
    }
    if (iMaxY < FULL_SCREEN_HEIGHT) {
        iMaxY = FULL_SCREEN_HEIGHT;
    }

    WINDOW *w_con = newwin(iMaxY, FULL_SCREEN_WIDTH, (TERMY > iMaxY) ? (TERMY-iMaxY)/2 : 0, (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    draw_border(w_con);
    mvwprintz(w_con, 0, 8, c_ltred, _(" Construction "));

    mvwputch(w_con,  0, 30, c_ltgray, LINE_OXXX);
    mvwputch(w_con, iMaxY-1, 30, c_ltgray, LINE_XXOX);
    for (int i = 1; i < iMaxY-1; ++i) {
        mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
    }

    wrefresh(w_con);

 bool update_info = true;
 int select = 0;
 int chosen = 0;
 long ch;
 bool exit = false;

 inventory total_inv = g->crafting_inventory(&(g->u));

 do {
// Erase existing list of constructions
  for (int i = 1; i < iMaxY-1; i++) {
   for (int j = 1; j < 30; j++)
    mvwputch(w_con, i, j, c_black, ' ');
  }
  //Draw Scrollbar
  draw_scrollbar(w_con, select, iMaxY-2, available.size(), 1);
  // Determine where in the master list to start printing
  //int offset = select - 11;
  int offset = 0;
  if (select >= iMaxY-2)
   offset = select - iMaxY + 3;
  // Print the constructions between offset and max (or how many will fit)
  for (int i = 0; i < iMaxY-2 && (i + offset) < available.size(); i++) {
   int current = i + offset;
   nc_color col = (player_can_build(g->u, total_inv, available[current]) ?
                   c_white : c_dkgray);
   // Map menu items to hotkey letters, skipping j, k, l, and q.
   unsigned char hotkey = 97 + current;
   if (hotkey > 122)
    hotkey = hotkey - 58;

   if (current == select)
    col = hilite(col);
   // print construction name with limited length.
   // limit(28) = 30(column len) - 2(letter + ' ').
   mvwprintz(w_con, 1 + i, 1, col, "%c %s", hotkey,
             utf8_substr(available[current].c_str(), 0, 28).c_str());
  }

  if (update_info) {
   update_info = false;
   std::string current_desc = available[select];
   // Clear out lines for tools & materials
   for (int i = 1; i < iMaxY-1; i++) {
    for (int j = 31; j < 79; j++)
     mvwputch(w_con, i, j, c_black, ' ');
   }

   // Print consruction name
   mvwprintz(w_con, 1, 31, c_white, "%s", current_desc.c_str());

   // Print stages and their requirement
   int posx = 33, posy = 1;
   std::vector<construction*> options = constructions_by_desc(current_desc);
   for (unsigned i = 0; i < options.size(); ++i) {
    construction *current_con = options[i];
    if (!can_construct(current_con)) {
     continue;
    }
    nc_color color_stage = c_white;

    // display required skill and difficulty
    int pskill = g->u.skillLevel(current_con->skill);
    int diff = current_con->difficulty > 0 ? current_con->difficulty : 0;
    posy++;
    mvwprintz(w_con, posy, 31, c_white,
              _("Skill: %s"), Skill::skill(current_con->skill)->name().c_str());
    posy++;
    mvwprintz(w_con, posy, 31, (pskill >= diff ? c_white : c_red),
              _("Difficulty: %d"), diff);
    // display required terrain
    if (current_con->pre_terrain != "") {
        posy++;
        if (current_con->pre_is_furniture) {
            mvwprintz(w_con, posy, 31, color_stage, _("Replaces: %s"),
                      furnmap[current_con->pre_terrain].name.c_str());
        } else {
            mvwprintz(w_con, posy, 31, color_stage, _("Replaces: %s"),
                      termap[current_con->pre_terrain].name.c_str());
        }
    }
    // display result
    if (current_con->post_terrain != "") {
        posy++;
        if (current_con->post_is_furniture) {
            mvwprintz(w_con, posy, 31, color_stage, _("Result: %s"),
                      furnmap[current_con->post_terrain].name.c_str());
        } else {
            mvwprintz(w_con, posy, 31, color_stage, _("Result: %s"),
                      termap[current_con->post_terrain].name.c_str());
        }
    }
    // display time needed
    posy++;
    mvwprintz(w_con, posy, 31, color_stage, _("Time: %1d minutes"), current_con->time);
    // Print tools
    std::vector<bool> has_tool;
    posy++;
    posx = 33;
    for (int i = 0; i < current_con->tools.size(); i++) {
     has_tool.push_back(false);
     mvwprintz(w_con, posy, posx-2, c_white, ">");
     for (unsigned j = 0; j < current_con->tools[i].size(); j++) {
      itype_id tool = current_con->tools[i][j].type;
      nc_color col = c_red;
      if (total_inv.has_amount(tool, 1)) {
       has_tool[i] = true;
       col = c_green;
      }
      int length = utf8_width(item_controller->find_template(tool)->name.c_str());
      if (posx + length > FULL_SCREEN_WIDTH-1) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, item_controller->find_template(tool)->name.c_str());
      posx += length + 1; // + 1 for an empty space
      if (j < current_con->tools[i].size() - 1) { // "OR" if there's more
       if (posx > FULL_SCREEN_WIDTH-3) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, _("OR"));
       posx += utf8_width(_("OR"))+1;
      }
     }
     posy ++;
     posx = 33;
    }
    // Print components
    posx = 33;
    std::vector<bool> has_component;
    for (int i = 0; i < current_con->components.size(); i++) {
     has_component.push_back(false);
     mvwprintz(w_con, posy, posx-2, c_white, ">");
     for (unsigned j = 0; j < current_con->components[i].size(); j++) {
      nc_color col = c_red;
      component comp = current_con->components[i][j];
      if (( item_controller->find_template(comp.type)->is_ammo() &&
           total_inv.has_charges(comp.type, comp.count)) ||
          (!item_controller->find_template(comp.type)->is_ammo() &&
           total_inv.has_amount(comp.type, comp.count))) {
       has_component[i] = true;
       col = c_green;
      }
      int length = utf8_width(item_controller->find_template(comp.type)->name.c_str());
      if (posx + length > FULL_SCREEN_WIDTH-1) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, "%s x%d",
                item_controller->find_template(comp.type)->name.c_str(), comp.count);
      posx += length + 3; // + 2 for " x", + 1 for an empty space
      // Add more space for the length of the count
      if (comp.count < 10)
       posx++;
      else if (comp.count < 100)
       posx += 2;
      else
       posx += 3;

      if (j < current_con->components[i].size() - 1) { // "OR" if there's more
       if (posx > FULL_SCREEN_WIDTH-3) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, _("OR"));
       posx += utf8_width(_("OR"))+1;
      }
     }
     posy ++;
     posx = 33;
    }
   }
   wrefresh(w_con);
  } // Finished updating

  ch = getch();
  switch (ch) {
   case KEY_DOWN:
    update_info = true;
    if (select < available.size() - 1)
     select++;
    else
     select = 0;
    break;
   case KEY_UP:
    update_info = true;
    if (select > 0)
     select--;
    else
     select = available.size() - 1;
    break;
   case ' ':
   case KEY_ESCAPE:
   case 'q':
   case 'Q':
    exit = true;
    break;
   case '\n':
   default:
    if (ch > 64 && ch < 91) //A-Z
     chosen = ch - 65 + 26;

    else if (ch > 96 && ch < 123) //a-z
     chosen = ch - 97;

    else if (ch == '\n')
     chosen = select;

    if (chosen < available.size()) {
     if (player_can_build(g->u, total_inv, available[chosen])) {
      place_construction(available[chosen]);
      exit = true;
     } else {
      popup(_("You can't build that!"));
      select = chosen;
      for (int i = 1; i < iMaxY-1; i++)
       mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
      update_info = true;
     }
    }
    break;
  }
 } while (!exit);

    for (int i = iMaxY-FULL_SCREEN_HEIGHT; i <= iMaxY; ++i) {
        for (int j = TERRAIN_WINDOW_WIDTH; j <= FULL_SCREEN_WIDTH; ++j) {
            mvwputch(w_con, i, j, c_black, ' ');
        }
    }

 wrefresh(w_con);
 g->refresh_all();
}

bool player_can_build(player &p, inventory pinv, const std::string &desc)
{
    // check all with the same desc to see if player can build any
    std::vector<construction*> cons = constructions_by_desc(desc);
    for (unsigned i = 0; i < cons.size(); ++i) {
        if (player_can_build(p, pinv, cons[i])) {
            return true;
        }
    }
    return false;
}

bool player_can_build(player &p, inventory pinv, construction *con)
{
    if (p.skillLevel(con->skill) < con->difficulty) {
        return false;
    }

    bool has_tool = false;
    bool has_component = false;
    bool tools_required = false;
    bool components_required = false;

    for (int j = 0; j < con->tools.size(); j++) {
        if (con->tools[j].size() > 0) {
            tools_required = true;
            has_tool = false;
            for (unsigned k = 0; k < con->tools[j].size(); k++) {
                if (pinv.has_amount(con->tools[j][k].type, 1)) {
                    has_tool = true;
                    con->tools[j][k].available = 1;
                } else {
                    con->tools[j][k].available = -1;
                }
            }
            if (!has_tool) { // missing one of the tools for this stage
                break;
            }
        }
    }

    for (int j = 0; j < con->components.size(); ++j) {
        if (con->components[j].size() > 0) {
            components_required = true;
            has_component = false;
            for (unsigned k = 0; k < con->components[j].size(); k++) {
                if (( item_controller->find_template(con->components[j][k].type)->is_ammo() &&
                      pinv.has_charges(con->components[j][k].type,
                                       con->components[j][k].count)    ) ||
                    (!item_controller->find_template(con->components[j][k].type)->is_ammo() &&
                      pinv.has_amount (con->components[j][k].type,
                                       con->components[j][k].count)    ))
                {
                    has_component = true;
                    con->components[j][k].available = 1;
                } else {
                    con->components[j][k].available = -1;
                }
            }
            if (!has_component) { // missing one of the comps for this stage
                break;
            }
        }
    }

    return (has_component || !components_required) &&
           (has_tool || !tools_required);
}

bool can_construct(construction *con, int x, int y)
{
    // see if the special pre-function checks out
    construct test;
    bool place_okay = (test.*(con->pre_special))(point(x, y));
    // see if the terrain type checks out
    if (con->pre_terrain != "") {
        if (con->pre_is_furniture) {
            furn_id f = furnmap[con->pre_terrain].loadid;
            place_okay &= (g->m.furn(x, y) == f);
        } else {
            ter_id t = termap[con->pre_terrain].loadid;
            place_okay &= (g->m.ter(x, y) == t);
        }
    }
    // see if the flags check out
    if (!con->pre_flags.empty()) {
        std::set<std::string>::iterator it;
        for (it = con->pre_flags.begin(); it != con->pre_flags.end(); ++it) {
            place_okay &= g->m.has_flag(*it, x, y);
        }
    }
    // make sure the construction would actually do something
    if (con->post_terrain != "") {
        if (con->post_is_furniture) {
            furn_id f = furnmap[con->post_terrain].loadid;
            place_okay &= (g->m.furn(x, y) != f);
        } else {
            ter_id t = termap[con->post_terrain].loadid;
            place_okay &= (g->m.ter(x, y) != t);
        }
    }
    return place_okay;
}

bool can_construct(construction *con)
{
    for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
        for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
            if (x == g->u.posx && y == g->u.posy) {
                y++;
            }
            if (can_construct(con, x, y)) {
                return true;
            }
        }
    }
    return false;
}

void place_construction(const std::string &desc)
{
    g->refresh_all();
    inventory total_inv = g->crafting_inventory(&(g->u));

    std::vector<construction*> cons = constructions_by_desc(desc);
    std::map<point,construction*> valid;
    for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
        for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
            if (x == g->u.posx && y == g->u.posy) {
                y++;
            }
            for (unsigned i = 0; i < cons.size(); ++i) {
                if (can_construct(cons[i], x, y)
                        && player_can_build(g->u, total_inv, cons[i])) {
                    valid[point(x, y)] = cons[i];
                }
            }
        }
    }

    for (std::map<point,construction*>::iterator it = valid.begin();
            it != valid.end(); ++it) {
        int x = it->first.x, y = it->first.y;
        g->m.drawsq(g->w_terrain, g->u, x, y, true, false);
    }
    wrefresh(g->w_terrain);

    int dirx, diry;
    if (!g->choose_adjacent(_("Contruct where?"), dirx, diry)) {
        return;
    }

    point choice(dirx, diry);
    if (valid.find(choice) == valid.end()) {
        g->add_msg(_("You cannot build there!"));
        return;
    }

    construction *con = valid[choice];
    g->u.assign_activity(ACT_BUILD, con->time * 1000, con->id);
    g->u.activity.placement = choice;
}

void complete_construction()
{
    construction *built = constructions[g->u.activity.index];

    g->u.practice(g->turn, built->skill, std::max(built->difficulty, 1) * 10);
    for (int i = 0; i < built->components.size(); i++) {
        if (!built->components[i].empty()) {
            g->consume_items(&(g->u), built->components[i]);
        }
    }

    // Make the terrain change
    int terx = g->u.activity.placement.x, tery = g->u.activity.placement.y;
    if (built->post_terrain != "") {
        if (built->post_is_furniture) {
            g->m.furn_set(terx, tery, built->post_terrain);
        } else {
            g->m.ter_set(terx, tery, built->post_terrain);
        }
    }

    // clear the activity
    g->u.activity.type = ACT_NULL;

    // This comes after clearing the activity, in case the function interrupts
    // activities
    construct effects;
    (effects.*(built->post_special))(point(terx, tery));
}

bool construct::check_empty(point p)
{
    return (g->m.has_flag("FLAT", p.x, p.y) && !g->m.has_furn(p.x, p.y) &&
            g->is_empty(p.x, p.y) && g->m.tr_at(p.x, p.y) == tr_null);
}

bool construct::check_support(point p)
{
    // need two or more orthogonally adjacent supports
    int num_supports = 0;
    if (g->m.move_cost(p.x, p.y) == 0) { return false; }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x, p.y - 1)) { ++num_supports; }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x, p.y + 1)) { ++num_supports; }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x - 1, p.y)) { ++num_supports; }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x + 1, p.y)) { ++num_supports; }
    return num_supports >= 2;
}

void construct::done_tree(point p)
{
    mvprintz(0, 0, c_red, _("Press a direction for the tree to fall in:"));
    int x = 0, y = 0;
    do get_direction(x, y, input());
    while (x == -2 || y == -2);
    x = p.x + x * 3 + rng(-1, 1);
    y = p.y + y * 3 + rng(-1, 1);
    std::vector<point> tree = line_to(p.x, p.y, x, y, rng(1, 8));
    for (unsigned i = 0; i < tree.size(); i++) {
        g->m.destroy(tree[i].x, tree[i].y, true);
        g->m.ter_set(tree[i].x, tree[i].y, t_trunk);
    }
}

void construct::done_trunk_log(point p)
{
    g->m.spawn_item(p.x, p.y, "log", rng(5, 15), 0, g->turn);
}

void construct::done_trunk_plank(point p)
{
    (void)p; //unused
    int num_logs = rng(5, 15);
    for( int i = 0; i < num_logs; ++i ) {
        item tmplog(itypes["log"], int(g->turn), g->nextinv);
        iuse::cut_log_into_planks( &(g->u), &tmplog);
    }
}


void construct::done_vehicle(point p)
{
    std::string name = string_input_popup(_("Enter new vehicle name:"), 20);
    if(name.empty()) {
        name = _("Car");
    }

    vehicle *veh = g->m.add_vehicle ("none", p.x, p.y, 270, 0, 0);

    if (!veh) {
        debugmsg ("error constructing vehicle");
        return;
    }
    veh->name = name;

    if (g->u.lastconsumed == "hdframe") {
        veh->install_part (0, 0, "hdframe_vertical_2");
    } else if (g->u.lastconsumed == "frame_wood") {
        veh->install_part (0, 0, "frame_wood_vertical_2");
    } else if (g->u.lastconsumed == "xlframe") {
        veh->install_part (0, 0, "xlframe_vertical_2");
    } else {
        veh->install_part (0, 0, "frame_vertical_2");
    }

    // Update the vehicle cache immediately,
    // or the vehicle will be invisible for the first couple of turns.
    g->m.update_vehicle_cache(veh, true);
}

void construct::done_deconstruct(point p)
{
    if (g->m.has_furn(p.x, p.y)) {
        std::string furn_here = g->m.get_furn(p.x, p.y);
        g->add_msg(_("You disassemble the %s."), g->m.furnname(p.x, p.y).c_str());
        if(furn_here == "f_makeshift_bed" || furn_here == "" || furn_here == "f_beg" || furn_here == "f_armchair" || furn_here == "f_sofa") {
            g->m.spawn_item(p.x, p.y, "2x4", 10);
            g->m.spawn_item(p.x, p.y, "rag", rng(10,15));
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,8));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_bench" || furn_here == "f_crate_o" || furn_here == "f_crate_c" || furn_here == "f_chair" || furn_here == "f_cupboard" || furn_here == "f_desk" || furn_here == "f_bulletin") {
            g->m.spawn_item(p.x, p.y, "2x4", 4);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_locker") {
            g->m.spawn_item(p.x, p.y, "sheet_metal", rng(1,2));
            g->m.spawn_item(p.x, p.y, "pipe", rng(4,8));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_rack") {
            g->m.spawn_item(p.x, p.y, "pipe", rng(6,12));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_oven") {
            g->m.spawn_item(p.x, p.y, "scrap",       rng(2,6));
            g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
            g->m.spawn_item(p.x, p.y, "element",     rng(1,4));
            g->m.spawn_item(p.x, p.y, "pilot_light", 1);
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_fridge") {
            g->m.spawn_item(p.x, p.y, "scrap", rng(2,8));
            g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
            g->m.spawn_item(p.x, p.y, "hose", 1);
            g->m.spawn_item(p.x, p.y, "cu_pipe", rng(2, 5));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_glass_fridge") {
            g->m.spawn_item(p.x, p.y, "scrap", rng(2,6));
            g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
            g->m.spawn_item(p.x, p.y, "hose", 1);
            g->m.spawn_item(p.x, p.y, "glass_sheet", 1);
            g->m.spawn_item(p.x, p.y, "cu_pipe", rng(3, 6));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_counter" || furn_here == "f_dresser" || furn_here == "f_table") {
            g->m.spawn_item(p.x, p.y, "2x4", 6);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,8));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_pool_table") {
            g->m.spawn_item(p.x, p.y, "2x4", 4);
            g->m.spawn_item(p.x, p.y, "rag", 4);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_bookcase") {
            g->m.spawn_item(p.x, p.y, "2x4", 12);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(12,16));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_washer") {
            g->m.spawn_item(p.x, p.y, "pipe", 1);
            g->m.spawn_item(p.x, p.y, "scrap",       rng(2,6));
            g->m.spawn_item(p.x, p.y, "steel_chunk",       rng(1,3));
            g->m.spawn_item(p.x, p.y, "sheet_metal",       rng(2,6));
            g->m.spawn_item(p.x, p.y, "cable",       rng(1,15));
            g->m.spawn_item(p.x, p.y, "cu_pipe",       rng(2,5));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_dryer") {
            g->m.spawn_item(p.x, p.y, "scrap",       rng(2,6));
            g->m.spawn_item(p.x, p.y, "steel_chunk",       rng(1,3));
            g->m.spawn_item(p.x, p.y, "sheet_metal",       rng(2,6));
            g->m.spawn_item(p.x, p.y, "cable",       rng(1,15));
            g->m.spawn_item(p.x, p.y, "element",       rng(1,3));
            g->m.furn_set(p.x, p.y, f_null);
        } else if(furn_here == "f_exercise") {
            g->m.spawn_item(p.x, p.y, "pipe", 1);
            g->m.spawn_item(p.x, p.y, "steel_chunk", 1);
            g->m.spawn_item(p.x, p.y, "scrap",       rng(2,6));
            g->m.furn_set(p.x, p.y, f_null);
        } else {
            g->add_msg(_("You have to push away %s first."), g->m.furnname(p.x, p.y).c_str());
        }
    
    } else {
        g->add_msg(_("You disassemble the %s."), g->m.tername(p.x, p.y).c_str());
        std::string ter_here = g->m.get_ter(p.x, p.y);
        if(ter_here == "t_door_c" || ter_here == "t_door_o") {
            g->m.spawn_item(p.x, p.y, "2x4", 4);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,12));
            g->m.ter_set(p.x, p.y, t_door_frame);
        } else if(ter_here == "t_rdoor_c" || ter_here == "t_rdoor_o") {
            g->m.spawn_item(p.x, p.y, "2x4", 24);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(36,48));
            g->m.ter_set(p.x, p.y, t_door_c);
        } else if(ter_here == "t_curtains" || ter_here == "t_window_domestic") {
            g->m.spawn_item(g->u.posx, g->u.posy, "stick");
            g->m.spawn_item(g->u.posx, g->u.posy, "sheet", 2);
            g->m.spawn_item(g->u.posx, g->u.posy, "glass_sheet");
            g->m.spawn_item(g->u.posx, g->u.posy, "nail", 0, rng(3,4));
            g->m.spawn_item(g->u.posx, g->u.posy, "string_36", 0, 1);
            g->m.ter_set(p.x, p.y, t_window_empty);
        } else if(ter_here == "t_window") {
            g->m.spawn_item(p.x, p.y, "glass_sheet");
            g->m.ter_set(p.x, p.y, t_window_empty);
        } else if(ter_here == "t_blackboard") {
            g->m.spawn_item(p.x, p.y, "2x4", 4);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
            g->m.ter_set(p.x, p.y, t_pavement);
        } else if(ter_here == "t_sandbox") {
            g->m.spawn_item(p.x, p.y, "2x4", 4);
            g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
            g->m.ter_set(p.x, p.y, t_floor);
        } else if(ter_here == "t_slide") {
            g->m.spawn_item(p.x, p.y, "sheet_metal");
            g->m.spawn_item(p.x, p.y, "pipe", rng(4,8));
            g->m.ter_set(p.x, p.y, t_grass);
        } else if(ter_here == "t_monkey_bars") {
            g->m.spawn_item(p.x, p.y, "pipe", rng(6,12));
            g->m.ter_set(p.x, p.y, t_grass);
        } else if(ter_here == "t_radio_controls" || ter_here == "t_console" || ter_here == "t_console_broken") {
            g->m.spawn_item(p.x, p.y, "processor", rng(1,2));
            g->m.spawn_item(p.x, p.y, "RAM", rng(4,8));
            g->m.spawn_item(p.x, p.y, "cable", rng(4,6));
            g->m.spawn_item(p.x, p.y, "small_lcd_screen", rng(1,2));
            g->m.spawn_item(p.x, p.y, "e_scrap", rng(10,16));
            g->m.spawn_item(p.x, p.y, "circuit", rng(6,10));
            g->m.spawn_item(p.x, p.y, "power_supply", rng(2,4));
            g->m.spawn_item(p.x, p.y, "amplifier", rng(2,4));
            g->m.spawn_item(p.x, p.y, "plastic_chunk", rng(10,12));
            g->m.spawn_item(p.x, p.y, "scrap", rng(6,8));
            g->m.ter_set(p.x, p.y, t_floor);
        }
    }
}

void load_construction(JsonObject &jo)
{
    construction *con = new construction;
    JsonArray temp;

    con->description = _(jo.get_string("description").c_str());
    con->skill = _(jo.get_string("skill", "carpentry").c_str());
    con->difficulty = jo.get_int("difficulty");
    con->time = jo.get_int("time");

    temp = jo.get_array("tools");
    while (temp.has_more()) {
        std::vector<component> tool_choices;
        JsonArray ja = temp.next_array();
        while (ja.has_more()) {
            std::string name = ja.next_string();
            tool_choices.push_back(component(name, 1));
        }
        con->tools.push_back(tool_choices);
    }

    temp = jo.get_array("components");
    while (temp.has_more()) {
        std::vector<component> comp_choices;
        JsonArray ja = temp.next_array();
        while (ja.has_more()) {
            JsonArray comp = ja.next_array();
            std::string name = comp.get_string(0);
            int quant = comp.get_int(1);
            comp_choices.push_back(component(name, quant));
        }
        con->components.push_back(comp_choices);
    }

    con->pre_terrain = jo.get_string("pre_terrain", "");
    if (con->pre_terrain.size() > 1
            && con->pre_terrain[0] == 'f'
            && con->pre_terrain[1] == '_') {
        con->pre_is_furniture = true;
    } else {
        con->pre_is_furniture = false;
    }

    con->post_terrain = jo.get_string("post_terrain", "");
    if (con->post_terrain.size() > 1
            && con->post_terrain[0] == 'f'
            && con->post_terrain[1] == '_') {
        con->post_is_furniture = true;
    } else {
        con->post_is_furniture = false;
    }

    con->pre_flags = jo.get_tags("pre_flags");

    std::string prefunc = jo.get_string("pre_special", "");
    if (prefunc == "check_empty") {
        con->pre_special = &construct::check_empty;
    } else if (prefunc == "check_support") {
        con->pre_special = &construct::check_support;
    } else {
        // should probably print warning if not ""
        con->pre_special = &construct::check_nothing;
    }

    std::string postfunc = jo.get_string("post_special", "");
    if (postfunc == "done_tree") {
        con->post_special = &construct::done_tree;
    } else if (postfunc == "done_trunk_log") {
        con->post_special = &construct::done_trunk_log;
    } else if (postfunc == "done_trunk_plank") {
        con->post_special = &construct::done_trunk_plank;
    } else if (postfunc == "done_vehicle") {
        con->post_special = &construct::done_vehicle;
    } else if (postfunc == "done_deconstruct") {
        con->post_special = &construct::done_deconstruct;
    } else {
        // ditto, should probably warn here
        con->post_special = &construct::done_nothing;
    }

    con->id = constructions.size();
    constructions.push_back(con);
}

void reset_constructions() {
    for(std::vector<construction*>::iterator a = constructions.begin(); a != constructions.end(); ++a) {
        delete *a;
    }
    constructions.clear();
}
