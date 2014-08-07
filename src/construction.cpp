#include "construction.h"

#include "game.h"
#include "output.h"
#include "player.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "item_factory.h"
#include "catacharset.h"
#include "action.h"
#include "translations.h"
#include "veh_interact.h"
#include "messages.h"

#include <algorithm>


struct construct // Construction functions.
{
    // Checks for whether terrain mod can proceed
    bool check_nothing(point) { return true; }
    bool check_empty(point); // tile is empty
    bool check_support(point); // at least two orthogonal supports
    bool check_deconstruct(point); // either terrain or furniture must be deconstructable
    bool check_up_OK(point); // tile is empty and you're not on the surface
    bool check_down_OK(point); // tile is empty and you're not on z-10 already

    // Special actions to be run post-terrain-mod
    void done_nothing(point) {}
    void done_tree(point);
    void done_trunk_log(point);
    void done_trunk_plank(point);
    void done_vehicle(point);
    void done_deconstruct(point);
    void done_dig_stair(point);
    void done_mine_downstair(point);
    void done_mine_upstair(point);
};

std::vector<construction *> constructions;

// Helper functions, nobody but us needs to call these.
static bool can_construct( const std::string &desc );
static bool can_construct( construction *con, int x, int y );
static bool can_construct( construction *con);
static bool player_can_build( player &p, const inventory &inv, construction *con );
static bool player_can_build( player &p, const inventory &pinv, const std::string &desc );
static void place_construction(const std::string &desc);

std::vector<construction *> constructions_by_desc(const std::string &description)
{
    std::vector<construction *> result;
    for( std::vector<construction *>::iterator a = constructions.begin();
         a != constructions.end(); ++a ) {
        if( (*a)->description == description ) {
            result.push_back( *a );
        }
    }
    return result;
}

bool will_flood_stop(map *m, bool (&fill)[SEEX *MAPSIZE][SEEY *MAPSIZE],
                     int x, int y);

static void load_available_constructions( std::vector<std::string> &available,
                                          bool hide_unconstructable )
{
    available.clear();
    for(std::vector<construction *>::iterator it = constructions.begin();
        it != constructions.end(); ++it ) {
        if( !hide_unconstructable || can_construct(*it) ) {
            bool already_have_it = false;
            for(std::vector<std::string>::iterator avail_it = available.begin();
                avail_it != available.end(); ++avail_it ) {
                if (*avail_it == (*it)->description) {
                    already_have_it = true;
                    break;
                }
            }
            if (!already_have_it) {
                available.push_back((*it)->description);
            }
        }
    }
}

void construction_menu()
{
    static bool hide_unconstructable = false;
    // only display constructions the player can theoretically perform
    std::vector<std::string> available;
    load_available_constructions( available, hide_unconstructable );

    if(available.empty()) {
        popup(_("You can not construct anything here."));
        return;
    }

    int iMaxY = TERMY;
    if (available.size() + 2 < iMaxY) {
        iMaxY = available.size() + 2;
    }
    if (iMaxY < FULL_SCREEN_HEIGHT) {
        iMaxY = FULL_SCREEN_HEIGHT;
    }

    WINDOW *w_con = newwin( iMaxY, FULL_SCREEN_WIDTH, (TERMY > iMaxY) ? (TERMY - iMaxY) / 2 : 0,
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0 );
    draw_border(w_con);
    mvwprintz(w_con, 0, 8, c_ltred, _(" Construction "));

    mvwputch(w_con,  0, 30, c_ltgray, LINE_OXXX);
    mvwputch(w_con, iMaxY - 1, 30, c_ltgray, LINE_XXOX);
    for( int i = 1; i < iMaxY - 1; ++i ) {
        mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
    }

    wrefresh(w_con);

    bool update_info = true;
    int select = 0;
    int oldselect = 0;
    int chosen = 0;
    int offset = 0;
    int oldoffset = 0;
    bool exit = false;

    inventory total_inv = g->crafting_inventory(&(g->u));

    input_context ctxt("CONSTRUCTION");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("PAGE_UP");
    ctxt.register_action("PAGE_DOWN");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("TOGGLE_UNAVAILABLE_CONSTRUCTIONS");
    ctxt.register_action("QUIT");
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::string hotkeys = ctxt.get_available_single_char_hotkeys();

    do {
        // Erase existing list of constructions
        for( int i = 1; i < iMaxY - 1; i++ ) {
            for( int j = 1; j < 30; j++ ) {
                mvwputch(w_con, i, j, c_black, ' ');
            }
        }
        // Determine where in the master list to start printing
        if( OPTIONS["MENU_SCROLL"] ) {
            if (available.size() > iMaxY) {
                offset = select - (iMaxY - 1) / 2;

                if (offset < 0) {
                    offset = 0;
                } else if (offset + iMaxY -2   > available.size()) {
                    offset = available.size() - iMaxY + 2;
                }
             }
        } else {
            if( select < offset ) {
                offset = select;
            } else if( select >= offset + iMaxY -2 ) {
                offset = 1 + select - iMaxY + 2;
            }
        }
        // Print the constructions between offset and max (or how many will fit)
        for (int i = 0; i < iMaxY - 2 && (i + offset) < available.size(); i++) {
            int current = i + offset;
            nc_color col = ( player_can_build(g->u, total_inv, available[current]) &&
                             can_construct( available[current] ) ) ? c_white : c_dkgray;
            if (current == select) {
                col = hilite(col);
            }
            // print construction name with limited length.
            // limit(28) = 30(column len) - 2(letter + ' ').
            // If we run out of hotkeys, just stop assigning them.
            std::string cur_name = available[current].c_str();
            mvwprintz(w_con, 1 + i, 1, col, "%c %s",
                      (current < hotkeys.size()) ? hotkeys[current] : ' ',
                      utf8_truncate(cur_name, 27).c_str());
        }

        if (update_info) {
            update_info = false;
            std::string current_desc = available[select];
            // Clear out lines for tools & materials
            for (int i = 1; i < iMaxY - 1; i++) {
                for (int j = 31; j < 79; j++) {
                    mvwputch(w_con, i, j, c_black, ' ');
                }
            }

            // Print instructions for toggling recipe hiding.
            mvwprintz(w_con, iMaxY - 2, 31, c_white, "%s", _("Press ';' to toggle unavailable constructions."));

            // Print construction name
            mvwprintz(w_con, 1, 31, c_white, "%s", current_desc.c_str());

            // Print stages and their requirement
            int posy = 1;
            std::vector<construction *> options = constructions_by_desc(current_desc);
            for(std::vector<construction *>::iterator it = options.begin();
                it != options.end(); ++it) {
                construction *current_con = *it;
                if( hide_unconstructable && !can_construct(current_con) ) {
                    continue;
                }
                nc_color color_stage = c_white;

                // display required skill and difficulty
                int pskill = g->u.skillLevel(current_con->skill);
                int diff = (current_con->difficulty > 0) ? current_con->difficulty : 0;
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
                posy += current_con->print_time(w_con, posy, 31, FULL_SCREEN_WIDTH - 31 - 1, color_stage);
                posy += current_con->print_tools(w_con, posy, 31, FULL_SCREEN_WIDTH - 31 - 1, color_stage, total_inv);
                posy += current_con->print_components(w_con, posy, 31, FULL_SCREEN_WIDTH - 31 - 1, color_stage, total_inv);
            }
        } // Finished updating

        //Draw Scrollbar.
        //Doing it here lets us refresh the entire window all at once.
        draw_scrollbar(w_con, select, iMaxY - 2, available.size(), 1);

        const std::string action = ctxt.handle_input();
        const long raw_input_char = ctxt.get_raw_input().get_first_input();

        if (action == "DOWN") {
            update_info = true;
            if (select < available.size() - 1) {
                select++;
            } else {
                select = 0;
            }
        } else if (action == "UP") {
            update_info = true;
            if (select > 0) {
                select--;
            } else {
                select = available.size() - 1;
            }
        } else if (action == "PAGE_DOWN") {
            update_info = true;
            select += 15;
            if ( select > available.size() - 1 ) {
                select = available.size() - 1;
            }
        } else if (action == "PAGE_UP") {
            update_info = true;
            select -= 15;
            if (select < 0) {
                select = 0;
            }
        } else if (action == "QUIT") {
            exit = true;
        } else if (action == "HELP_KEYBINDINGS") {
            hotkeys = ctxt.get_available_single_char_hotkeys();
        } else if (action == "TOGGLE_UNAVAILABLE_CONSTRUCTIONS") {
            update_info = true;
            hide_unconstructable = !hide_unconstructable;
            std::swap(select, oldselect);
            std::swap(offset, oldoffset);
            load_available_constructions( available, hide_unconstructable );
        } else if (action == "ANY_INPUT" || action == "CONFIRM") {
            if (action == "CONFIRM") {
                chosen = select;
            } else {
                // Get the index corresponding to the key pressed.
                chosen = hotkeys.find_first_of(raw_input_char);
                if( chosen == std::string::npos ) {
                    continue;
                }
            }
            if (chosen < available.size()) {
                if (player_can_build(g->u, total_inv, available[chosen])) {
                    place_construction(available[chosen]);
                    exit = true;
                } else {
                    popup(_("You can't build that!"));
                    select = chosen;
                    for (int i = 1; i < iMaxY - 1; i++) {
                        mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
                    }
                    update_info = true;
                }
            }
        }
    } while (!exit);

    wrefresh(w_con);
    delwin(w_con);
    g->refresh_all();
}

static bool player_can_build(player &p, const inventory &pinv, const std::string &desc)
{
    // check all with the same desc to see if player can build any
    std::vector<construction *> cons = constructions_by_desc(desc);
    for (std::vector<construction *>::iterator it = cons.begin();
         it != cons.end(); ++it) {
        if (player_can_build(p, pinv, *it)) {
            return true;
        }
    }
    return false;
}

static bool player_can_build(player &p, const inventory &pinv, construction *con)
{
    if (p.skillLevel(con->skill) < con->difficulty) {
        return false;
    }
    return con->can_make_with_inventory( pinv );
}

static bool can_construct( const std::string &desc )
{
    // check all with the same desc to see if player can build any
    std::vector<construction *> cons = constructions_by_desc(desc);
    for(std::vector<construction *>::iterator it = cons.begin();
        it != cons.end(); ++it) {
        if(can_construct(*it)) {
            return true;
        }
    }
    return false;
}

static bool can_construct(construction *con, int x, int y)
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

static bool can_construct(construction *con)
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

static void place_construction(const std::string &desc)
{
    g->refresh_all();
    inventory total_inv = g->crafting_inventory(&(g->u));

    std::vector<construction *> cons = constructions_by_desc(desc);
    std::map<point, construction *> valid;
    for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
        for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
            if (x == g->u.posx && y == g->u.posy) {
                y++;
            }
            for (std::vector<construction *>::iterator it = cons.begin();
                 it != cons.end(); ++it) {
                if (can_construct(*it, x, y)
                    && player_can_build(g->u, total_inv, *it)) {
                    valid[point(x, y)] = *it;
                }
            }
        }
    }

    for (std::map<point, construction *>::iterator it = valid.begin();
         it != valid.end(); ++it) {
        int x = it->first.x, y = it->first.y;
        g->m.drawsq(g->w_terrain, g->u, x, y, true, false);
    }
    wrefresh(g->w_terrain);

    int dirx, diry;
    if (!choose_adjacent(_("Construct where?"), dirx, diry)) {
        return;
    }

    point choice(dirx, diry);
    if (valid.find(choice) == valid.end()) {
        add_msg(m_info, _("You cannot build there!"));
        return;
    }

    construction *con = valid[choice];
    g->u.assign_activity(ACT_BUILD, con->time, con->id);
    g->u.activity.placement = choice;
}

void complete_construction()
{
    construction *built = constructions[g->u.activity.index];

    g->u.practice( built->skill, std::max(built->difficulty, 1) * 10,
                   (int)(built->difficulty * 1.25) );
    for (auto it = built->components.begin(); it != built->components.end(); ++it) {
        // Tried issuing rope for WEB_ROPE here.  Didn't arrive in time for the
        // gear check.  Ultimately just coded a bypass in crafting.cpp.
        if (!it->empty()) {
            g->consume_items(&(g->u), *it);
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
            g->is_empty(p.x, p.y) && g->m.tr_at(p.x, p.y) == tr_null &&
            g->m.i_at(p.x, p.y).empty() && g->m.veh_at(p.x, p.y) == NULL);
}

bool construct::check_support(point p)
{
    // need two or more orthogonally adjacent supports
    int num_supports = 0;
    if (g->m.move_cost(p.x, p.y) == 0) {
        return false;
    }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x, p.y - 1)) {
        ++num_supports;
    }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x, p.y + 1)) {
        ++num_supports;
    }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x - 1, p.y)) {
        ++num_supports;
    }
    if (g->m.has_flag("SUPPORTS_ROOF", p.x + 1, p.y)) {
        ++num_supports;
    }
    return num_supports >= 2;
}

bool construct::check_deconstruct(point p)
{
    if (g->m.has_furn(p.x, p.y)) {
        return g->m.furn_at(p.x, p.y).deconstruct.can_do;
    }
    // terrain can only be deconstructed when there is no furniture in the way
    return g->m.ter_at(p.x, p.y).deconstruct.can_do;
}

bool construct::check_up_OK(point)
{
    // You're not going above +OVERMAP_HEIGHT.
    return (g->levz < OVERMAP_HEIGHT);
}

bool construct::check_down_OK(point)
{
    // You're not going below -OVERMAP_DEPTH.
    return (g->levz > -OVERMAP_DEPTH);
}

void construct::done_tree(point p)
{
    int x = 0, y = 0;
    while (!choose_direction(_("Press a direction for the tree to fall in:"), x, y)) {
        // try again
    }
    x = p.x + x * 3 + rng(-1, 1);
    y = p.y + y * 3 + rng(-1, 1);
    std::vector<point> tree = line_to(p.x, p.y, x, y, rng(1, 8));
    for (std::vector<point>::iterator it = tree.begin();
         it != tree.end(); ++it) {
        g->m.destroy(it->x, it->y, true);
        g->m.ter_set(it->x, it->y, t_trunk);
    }
}

void construct::done_trunk_log(point p)
{
    g->m.spawn_item(p.x, p.y, "log", rng(5, 15), 0, calendar::turn);
}

void construct::done_trunk_plank(point p)
{
    (void)p; //unused
    int num_logs = rng(5, 15);
    for( int i = 0; i < num_logs; ++i ) {
        item tmplog( "log", int(calendar::turn) );
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
        furn_t &f = g->m.furn_at(p.x, p.y);
        if (!f.deconstruct.can_do) {
            add_msg(m_info, _("That %s can not be disassembled!"), f.name.c_str());
            return;
        }
        if (f.deconstruct.furn_set.empty()) {
            g->m.furn_set(p.x, p.y, f_null);
        } else {
            g->m.furn_set(p.x, p.y, f.deconstruct.furn_set);
        }
        add_msg(_("You disassemble the %s."), f.name.c_str());
        g->m.spawn_item_list(f.deconstruct.items, p.x, p.y);
        // Hack alert.
        // Signs have cosmetics associated with them on the submap since 
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        g->m.delete_signage(p.x, p.y);
    } else {
        ter_t &t = g->m.ter_at(p.x, p.y);
        if (!t.deconstruct.can_do) {
            add_msg(_("That %s can not be disassembled!"), t.name.c_str());
            return;
        }
        if (t == t_console_broken) {
            if (g->u.skillLevel("electronics") >= 1) {
                g->u.practice(electronics, 2, 4);
            }
        }
        g->m.ter_set(p.x, p.y, t.deconstruct.ter_set);
        add_msg(_("You disassemble the %s."), t.name.c_str());
        g->m.spawn_item_list(t.deconstruct.items, p.x, p.y);
    }
}

void construct::done_dig_stair(point p)
{
 tinymap tmpmap;
 // Upper left corner of the current active map (levx/levy) plus half active map width.
 // The player is always in the center tile of that 11x11 square.
 tmpmap.load(g->levx + (MAPSIZE/2), g->levy + (MAPSIZE / 2), g->levz - 1, false, g->cur_om);
 bool danger_lava = false;
 bool danger_open = false;
 const int omtilesz=SEEX * 2;  // KA101's 1337 copy & paste skillz
    real_coords rc( g->m.getabs(p.x % SEEX, p.y % SEEY) );

    point omtile_align_start(
        g->m.getlocal( rc.begin_om_pos() )
    );
    // Smart and perceptive folks can pick up on Bad Stuff Below farther out
    int prox = ( (((g->u.int_cur) + (g->u.per_cur)) / 2) - 8 );
    if (prox <= 0) {
        prox = 1;
    }
  for (int i = omtile_align_start.x; i <= omtile_align_start.x + omtilesz; i++) {
      for (int j = omtile_align_start.y; j <= omtile_align_start.y + omtilesz; j++) {
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.ter(i, j) == t_lava)) {
              danger_lava = true;
          }
          // This ought to catch anything that's open-space
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.move_cost(i, j) >= 2 )) {
              danger_open = true; // You might not know what's down there!
          }
      }
  }
  if (danger_lava || danger_open) { // Bad Stuff detected.  Are you sure?
      g->m.ter_set(p.x, p.y, t_pit); // You dug down a bit before detecting the problem
      if (danger_lava) {
          if (!(query_yn(_("The rock feels much warmer than normal. Proceed?"))) ) {
              // refund components!
              if (!(g->u.has_trait("WEB_ROPE"))) {
                  item rope("rope_30", 0);
                  g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
              }
              // presuming 2x4 to conserve lumber.
              g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 8);
              return;
          }
      }
      if (danger_open) {
          if (!(query_yn(_("As you dig, the rock starts sounding hollow. Proceed?"))) ) {
              // refund components!
              if (!(g->u.has_trait("WEB_ROPE"))) {
                  item rope("rope_30", 0);
                  g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
              }
              // presuming 2x4 to conserve lumber.
              g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 8);
              return;
          }
      }
  }
  if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) == 0) { // Solid rock or a wall.  Safe enough.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You strike deeply into the earth."));
          g->u.hunger += 15;
          g->u.fatigue += 20;
          g->u.thirst += 15;
          g->u.mod_pain(8);
      }
      else {
          add_msg(_("You dig a stairway, adding sturdy timbers and a rope for safety."));
          g->u.hunger += 25;
          g->u.fatigue += 30;
          g->u.thirst += 25;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work, though."));
              g->u.mod_pain(8); // Backbreaking work, mining!
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_down); // There's the top half
      // We need to write to submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_stairs_up); // and there's the bottom half.
      tmpmap.save();
   }
   else if (tmpmap.ter(p.x % SEEX, p.y % SEEY) == t_lava) { // Oooooops
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(m_warning, _("You strike deeply--above a magma flow!"));
          g->u.hunger += 15;
          g->u.fatigue += 20;
          g->u.thirst += 15;
          g->u.mod_pain(4);
      }
      else {
          add_msg(m_warning, _("You just tunneled into lava!"));
          g->u.hunger += 25;
          g->u.fatigue += 30;
          g->u.thirst += 25;
          if (!(g->u.has_trait("NOPAIN"))) {
              g->u.mod_pain(4); // Backbreaking work, mining!
          }
      }
      g->u.add_memorial_log(pgettext("memorial_male", "Dug a shaft into lava."),
                       pgettext("memorial_female", "Dug a shaft into lava."));
      // Now to see if you go swimming.  Same idea as the sinkhole.
      if ( ((g->u.skillLevel("carpentry")) + (g->u.per_cur)) > ((g->u.str_cur) +
          (rng(5,10))) ) {
              add_msg(_("You avoid collapsing the rock underneath you."));
              add_msg(_("Lashing your lumber together, you make a stable platform."));
              g->m.ter_set(p.x, p.y, t_pit);
          }
      else {
          g->m.ter_set(p.x, p.y, t_hole); // Collapse handled here.
          add_msg(_("The rock gives way beneath you!"));
          add_msg(_("Your timbers plummet into the lava!"));
          if (g->u.has_amount("grapnel", 1)) {
              add_msg(_("You desperately throw your grappling hook!"));
              int throwroll = rng(g->u.skillLevel("throw"),
                      g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
              if (throwroll >= 9) { // Little tougher here than in a sinkhole
              add_msg(_("The grappling hook catches something!"));
              if (rng(g->u.skillLevel("unarmed"),
                      g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
              // Determine safe places for the character to get pulled to
              std::vector<point> safe;
                  for (int i = p.x - 1; i <= p.x + 1; i++) {
                    for (int j = p.y - 1; j <= p.y + 1; j++) {
                      if (g->m.move_cost(i, j) > 0) {
                          safe.push_back(point(i, j));
                      }
                    }
                  }
                  if (safe.empty()) {
                      add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                      g->u.use_amount("grapnel", 1);
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                      g->vertical_move(-1, true);
                  } else {
                      add_msg(_("You pull yourself to safety!"));
                      int index = rng(0, safe.size() - 1);
                      g->u.posx = safe[index].x;
                      g->u.posy = safe[index].y;
                      g->update_map(g->u.posx, g->u.posy);
                  }
              } else {
                    add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                    g->u.moves -= 100;
                    g->u.use_amount("grapnel", 1);
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                    g->vertical_move(-1, true);
                }
              } else {
                  add_msg(m_bad, _("Your throw misses completely, and you fall into the lava!"));
                  if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                      g->u.use_amount("grapnel", 1);
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                  }
                g->vertical_move(-1, true);
              }
          } else if (g->u.has_trait("WEB_ROPE")) {
              // There are downsides to using one's own product...
              int webroll = rng(g->u.skillLevel("carpentry"),
                      g->u.skillLevel("carpentry") + g->u.per_cur + g->u.int_cur);
              if (webroll >= 11) {
                  add_msg(_("Luckily, you'd attached a web..."));
                  // Bigger you are, the larger the strain
                  int stickroll = rng(g->u.skillLevel("carpentry"),
                      g->u.skillLevel("carpentry") + g->u.dex_cur - g->u.str_cur);
                  if (stickroll >= 8) {
                      add_msg(_("Your web holds firm!"));
                      if (rng(g->u.skillLevel("unarmed"),
                          g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
                          // Determine safe places for the character to get pulled to
                          std::vector<point> safe;
                          for (int i = p.x - 1; i <= p.x + 1; i++) {
                            for (int j = p.y - 1; j <= p.y + 1; j++) {
                              if (g->m.move_cost(i, j) > 0) {
                                  safe.push_back(point(i, j));
                              }
                            }
                          }
                          if (safe.empty()) {
                              add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                              g->vertical_move(-1, true);
                          } else {
                              add_msg(_("You pull yourself to safety!"));
                              int index = rng(0, safe.size() - 1);
                              g->u.posx = safe[index].x;
                              g->u.posy = safe[index].y;
                              g->update_map(g->u.posx, g->u.posy);
                          }
                      } else {
                            add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                            g->u.moves -= 100;
                            g->vertical_move(-1, true);
                        }
                      } else {
                          add_msg(m_bad, _("The sudden strain pulls your web free, and you fall into the lava!"));
                          g->vertical_move(-1, true);
                      }
                  }
          } else {
          // You have a rope because you needed one to construct
          // (You aren't charged it here because you lose it at end/construction)
          add_msg(_("You desperately throw your rope!"));
              int throwroll = rng(g->u.skillLevel("throw"),
                      g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
              if (throwroll >= 11) { // No hook, so good luck with that
              add_msg(_("The rope snags and holds!"));
              if (rng(g->u.skillLevel("unarmed"),
                      g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
              // Determine safe places for the character to get pulled to
              std::vector<point> safe;
                  for (int i = p.x - 1; i <= p.x + 1; i++) {
                    for (int j = p.y - 1; j <= p.y + 1; j++) {
                      if (g->m.move_cost(i, j) > 0) {
                          safe.push_back(point(i, j));
                      }
                    }
                  }
                  if (safe.empty()) {
                      add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                      g->vertical_move(-1, true);
                  } else {
                      add_msg(_("You pull yourself to safety!"));
                      add_msg(_("The rope gives way and plummets, just as you escape."));
                      int index = rng(0, safe.size() - 1);
                      g->u.posx = safe[index].x;
                      g->u.posy = safe[index].y;
                      g->update_map(g->u.posx, g->u.posy);
                  }
              } else {
                    add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                    g->u.moves -= 100;
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                    g->vertical_move(-1, true);
                }
              } else {
                  add_msg(m_bad, _("Your throw misses completely, and you fall into the lava!"));
                  if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                  }
                g->vertical_move(-1, true);
              }
          }
      }
   }
   else if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) >= 2) { // Empty non-lava terrain.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You strike deeply into the earth, and break into open space."));
          g->u.hunger += 10; // Less heavy work, but making the ladder's still fatiguing
          g->u.fatigue += 20;
          g->u.thirst += 10;
          g->u.mod_pain(4);
      }
      else {
          add_msg(_("You dig into a preexisting space, and improvise a ladder."));
          g->u.hunger += 20;
          g->u.fatigue += 30;
          g->u.thirst += 20;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work, though."));
              g->u.mod_pain(4); // Backbreaking work, mining!
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_down); // There's the top half
      // Again, need to use submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_ladder_up); // and there's the bottom half.
      // And save to the center coordinate of the current active map.
      tmpmap.save();
   }
}

void construct::done_mine_downstair(point p)
{
 tinymap tmpmap;
 // Upper left corner of the current active map (levx/levy) plus half active map width.
 // The player is always in the center tile of that 11x11 square.
 tmpmap.load(g->levx + (MAPSIZE/2), g->levy + (MAPSIZE / 2), g->levz - 1, false, g->cur_om);
 bool danger_lava = false;
 bool danger_open = false;
 const int omtilesz=SEEX * 2;  // KA101's 1337 copy & paste skillz
    real_coords rc( g->m.getabs(p.x % SEEX, p.y % SEEY) );

    point omtile_align_start(
        g->m.getlocal( rc.begin_om_pos() )
    );
    // Smart and perceptive folks can pick up on Bad Stuff Below farther out
    // Tougher with the noisy J-Hammer though
    int prox = ( (((g->u.int_cur) + (g->u.per_cur)) / 2) - 10 );
    if (prox <= 0) {
        prox = 1;
    }
  for (int i = omtile_align_start.x; i <= omtile_align_start.x + omtilesz; i++) {
      for (int j = omtile_align_start.y; j <= omtile_align_start.y + omtilesz; j++) {
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.ter(i, j) == t_lava)) {
              danger_lava = true;
          }
          // This ought to catch anything that's open-space
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.move_cost(i, j) >= 2 )) {
              danger_open = true; // You might not know what's down there!
          }
      }
  }
  if (danger_lava || danger_open) { // Bad Stuff detected.  Are you sure?
      g->m.ter_set(p.x, p.y, t_pit); // You dug down a bit before detecting the problem
      if (danger_lava) {
          if (!(query_yn(_("The rock feels much warmer than normal. Proceed?"))) ) {
              // refund components!
              if (!(g->u.has_trait("WEB_ROPE"))) {
                  item rope("rope_30", 0);
                  g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
              }
              // presuming 2x4 to conserve lumber.
              g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 12);
              return;
          }
      }
      if (danger_open) {
          if (!(query_yn(_("As you dig, the rock starts sounding hollow. Proceed?"))) ) {
              // refund components!
              if (!(g->u.has_trait("WEB_ROPE"))) {
                  item rope("rope_30", 0);
                  g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
              }
              // presuming 2x4 to conserve lumber.
              g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 12);
              return;
          }
      }
  }
  if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) == 0) { // Solid rock or a wall.  Safe enough.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You delve ever deeper into the earth."));
          g->u.hunger += 25;
          g->u.fatigue += 30;
          g->u.thirst += 25;
          g->u.mod_pain(10); // NOPAIN is a Prototype trait so shouldn't be present here
      }
      else {
          add_msg(_("You drill out a passage, heading deeper underground."));
          g->u.hunger += 35;
          g->u.fatigue += 40;
          g->u.thirst += 35;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work."));
              g->u.mod_pain(10); // Backbreaking work, mining!
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_down); // There's the top half
      // We need to write to submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_stairs_up); // and there's the bottom half.
      tmpmap.save();
   }
   else if (tmpmap.ter(p.x % SEEX, p.y % SEEY) == t_lava) { // Oooooops
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(m_warning, _("You delve down directly above a magma flow!"));
          g->u.hunger += 25;
          g->u.fatigue += 30;
          g->u.thirst += 25;
          g->u.mod_pain(4);
      }
      else {
          add_msg(m_warning, _("You just mined into lava!"));
          g->u.hunger += 35;
          g->u.fatigue += 40;
          g->u.thirst += 35;
          if (!(g->u.has_trait("NOPAIN"))) {
              g->u.mod_pain(4);
          }
      }
      g->u.add_memorial_log(pgettext("memorial_male", "Mined into lava."),
                       pgettext("memorial_female", "Mined into lava."));
      // Now to see if you go swimming.  Same idea as the sinkhole.
      if ( ((g->u.skillLevel("carpentry")) + (g->u.per_cur)) > ((g->u.str_cur) +
          (rng(5,10))) ) {
              add_msg(_("You avoid collapsing the rock underneath you."));
              add_msg(_("Lashing your lumber together, you make a stable platform."));
              g->m.ter_set(p.x, p.y, t_pit);
          }
      else {
          g->m.ter_set(p.x, p.y, t_hole); // Collapse handled here.
          add_msg(_("The rock gives way beneath you!"));
          add_msg(_("Your timbers plummet into the lava!"));
          if (g->u.has_amount("grapnel", 1)) {
              add_msg(_("You desperately throw your grappling hook!"));
              int throwroll = rng(g->u.skillLevel("throw"),
                      g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
              if (throwroll >= 9) { // Little tougher here than in a sinkhole
              add_msg(_("The grappling hook catches something!"));
              if (rng(g->u.skillLevel("unarmed"),
                      g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
              // Determine safe places for the character to get pulled to
              std::vector<point> safe;
                  for (int i = p.x - 1; i <= p.x + 1; i++) {
                    for (int j = p.y - 1; j <= p.y + 1; j++) {
                      if (g->m.move_cost(i, j) > 0) {
                          safe.push_back(point(i, j));
                      }
                    }
                  }
                  if (safe.empty()) {
                      add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                      g->u.use_amount("grapnel", 1);
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                      g->vertical_move(-1, true);
                  } else {
                      add_msg(_("You pull yourself to safety!"));
                      int index = rng(0, safe.size() - 1);
                      g->u.posx = safe[index].x;
                      g->u.posy = safe[index].y;
                      g->update_map(g->u.posx, g->u.posy);
                  }
              } else {
                    add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                    g->u.moves -= 100;
                    g->u.use_amount("grapnel", 1);
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                    g->vertical_move(-1, true);
                }
              } else {
                  add_msg(m_bad, _("Your throw misses completely, and you fall into the lava!"));
                  if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                      g->u.use_amount("grapnel", 1);
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "grapnel");
                  }
                g->vertical_move(-1, true);
              }
          } else if (g->u.has_trait("WEB_ROPE")) {
              // There are downsides to using one's own product...
              int webroll = rng(g->u.skillLevel("carpentry"),
                      g->u.skillLevel("carpentry") + g->u.per_cur + g->u.int_cur);
              if (webroll >= 11) {
                  add_msg(_("Luckily, you'd attached a web..."));
                  // Bigger you are, the larger the strain
                  int stickroll = rng(g->u.skillLevel("carpentry"),
                      g->u.skillLevel("carpentry") + g->u.dex_cur - g->u.str_cur);
                  if (stickroll >= 8) {
                      add_msg(_("Your web holds firm!"));
                      if (rng(g->u.skillLevel("unarmed"),
                          g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
                          // Determine safe places for the character to get pulled to
                          std::vector<point> safe;
                          for (int i = p.x - 1; i <= p.x + 1; i++) {
                            for (int j = p.y - 1; j <= p.y + 1; j++) {
                              if (g->m.move_cost(i, j) > 0) {
                                  safe.push_back(point(i, j));
                              }
                            }
                          }
                          if (safe.empty()) {
                              add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                              g->vertical_move(-1, true);
                          } else {
                              add_msg(_("You pull yourself to safety!"));
                              int index = rng(0, safe.size() - 1);
                              g->u.posx = safe[index].x;
                              g->u.posy = safe[index].y;
                              g->update_map(g->u.posx, g->u.posy);
                          }
                      } else {
                            add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                            g->u.moves -= 100;
                            g->vertical_move(-1, true);
                        }
                      } else {
                          add_msg(m_bad, _("The sudden strain pulls your web free, and you fall into the lava!"));
                          g->vertical_move(-1, true);
                      }
                  }
          } else {
          // You have a rope because you needed one to construct
          // (You aren't charged it here because you lose it at end/construction)
          add_msg(_("You desperately throw your rope!"));
              int throwroll = rng(g->u.skillLevel("throw"),
                      g->u.skillLevel("throw") + g->u.str_cur + g->u.dex_cur);
              if (throwroll >= 11) { // No hook, so good luck with that
              add_msg(_("The rope snags and holds!"));
              if (rng(g->u.skillLevel("unarmed"),
                      g->u.skillLevel("unarmed") + g->u.str_cur) > 7) {
              // Determine safe places for the character to get pulled to
              std::vector<point> safe;
                  for (int i = p.x - 1; i <= p.x + 1; i++) {
                    for (int j = p.y - 1; j <= p.y + 1; j++) {
                      if (g->m.move_cost(i, j) > 0) {
                          safe.push_back(point(i, j));
                      }
                    }
                  }
                  if (safe.empty()) {
                      add_msg(m_bad, _("There's nowhere to pull yourself to, and you fall!"));
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                      g->vertical_move(-1, true);
                  } else {
                      add_msg(_("You pull yourself to safety!"));
                      add_msg(_("The rope gives way and plummets, just as you escape."));
                      int index = rng(0, safe.size() - 1);
                      g->u.posx = safe[index].x;
                      g->u.posy = safe[index].y;
                      g->update_map(g->u.posx, g->u.posy);
                  }
              } else {
                    add_msg(m_bad, _("You're not strong enough to pull yourself out..."));
                    g->u.moves -= 100;
                    g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                    g->vertical_move(-1, true);
                }
              } else {
                  add_msg(m_bad, _("Your throw misses completely, and you fall into the lava!"));
                  if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) {
                      g->m.spawn_item(g->u.posx + rng(-1, 1), g->u.posy + rng(-1, 1), "rope_30");
                  }
                g->vertical_move(-1, true);
              }
          }
      }
   }
   else if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) >= 2) { // Empty non-lava terrain.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You delve ever deeper into the earth, and break into open space."));
          g->u.hunger += 20; // Less heavy work, but making the ladder's still fatiguing
          g->u.fatigue += 30;
          g->u.thirst += 20;
          g->u.mod_pain(4);
      }
      else {
          add_msg(_("You mine into a preexisting space, and improvise a ladder."));
          g->u.hunger += 30;
          g->u.fatigue += 40;
          g->u.thirst += 30;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work."));
              g->u.mod_pain(4);
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_down); // There's the top half
      // Again, need to use submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_ladder_up); // and there's the bottom half.
      // And save to the center coordinate of the current active map.
      tmpmap.save();
   }
}

void construct::done_mine_upstair(point p)
{
 tinymap tmpmap;
 // Upper left corner of the current active map (levx/levy) plus half active map width.
 // The player is always in the center tile of that 11x11 square.
 tmpmap.load(g->levx + (MAPSIZE/2), g->levy + (MAPSIZE / 2), g->levz + 1, false, g->cur_om);
 bool danger_lava = false;
 bool danger_open = false;
 bool danger_liquid = false;
 const int omtilesz=SEEX * 2;  // KA101's 1337 copy & paste skillz
    real_coords rc( g->m.getabs(p.x % SEEX, p.y % SEEY) );

    point omtile_align_start(
        g->m.getlocal( rc.begin_om_pos() )
    );
    // Smart and perceptive folks can pick up on Bad Stuff Above farther out
    // Tougher with the noisy J-Hammer though
    int prox = ( (((g->u.int_cur) + (g->u.per_cur)) / 2) - 10 );
    if (prox <= 0) {
        prox = 1;
    }
  for (int i = omtile_align_start.x; i <= omtile_align_start.x + omtilesz; i++) {
      for (int j = omtile_align_start.y; j <= omtile_align_start.y + omtilesz; j++) {
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.ter(i, j) == t_lava)) {
              danger_lava = true;
          }
          // This ought to catch anything that's open-space
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && (tmpmap.move_cost(i, j) >= 2 )) {
              danger_open = true; // You might not know what's up there!
          }
          // Coming up into a river or sewer line could ruin your whole day!
          if (rl_dist(p.x % SEEX, p.y % SEEY, i, j) <= prox && ( (tmpmap.ter(i, j) == t_water_sh) ||
          (tmpmap.ter(i, j) == t_sewage) || (tmpmap.ter(i, j) == t_water_dp) ||
          (tmpmap.ter(i, j) == t_water_pool) ) ) {
              danger_liquid = true;
          }
      }
  }
  if (danger_lava || danger_open || danger_liquid) { // Bad Stuff detected.  Are you sure?
      g->m.ter_set(p.x, p.y, t_rock_floor); // You dug a bit before discovering the problem
      if (danger_lava) {
          add_msg(m_warning, _("The rock overhead feels hot.  You decide *not* to mine magma."));
          // refund components!
          if (!(g->u.has_trait("WEB_ROPE"))) {
              item rope("rope_30", 0);
              g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
          }
          // presuming 2x4 to conserve lumber.
          g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 12);
          return;
      }
      if (danger_open) {
          if (!(query_yn(_("As you dig, the rock starts sounding hollow. Proceed?"))) ) {
              // refund components!
              if (!(g->u.has_trait("WEB_ROPE"))) {
                  item rope("rope_30", 0);
                  g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
              }
              // presuming 2x4 to conserve lumber.
              g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 12);
              return;
          }
      }
      if (danger_liquid) {
          add_msg(m_warning, _("The rock above is rather damp.  You decide *not* to mine water."));
          // refund components!
          if (!(g->u.has_trait("WEB_ROPE"))) {
              item rope("rope_30", 0);
              g->m.add_item_or_charges(g->u.posx, g->u.posy, rope);
          }
          // presuming 2x4 to conserve lumber.
          g->m.spawn_item(g->u.posx, g->u.posy,"2x4", 12);
          return;
      }
  }
  if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) == 0) { // Solid rock or a wall.  Safe enough.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You carve upward and breach open a space."));
          g->u.hunger += 35;
          g->u.fatigue += 40;
          g->u.thirst += 35;
          g->u.mod_pain(15); // NOPAIN is a THRESH_MEDICAL trait so shouldn't be present here
      }
      else {
          add_msg(_("You drill out a passage, heading for the surface."));
          g->u.hunger += 45;
          g->u.fatigue += 50;
          g->u.thirst += 45;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work."));
              g->u.mod_pain(15); // Backbreaking work, mining!
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_up); // There's the bottom half
      // We need to write to submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_stairs_down); // and there's the top half.
      tmpmap.save();
   }
   else if (tmpmap.move_cost(p.x % SEEX, p.y % SEEY) >= 2) { // Empty non-lava terrain.
      if (g->u.has_trait("PAINRESIST_TROGLO") || g->u.has_trait("STOCKY_TROGLO")) {
          add_msg(_("You carve upward, and break into open space."));
          g->u.hunger += 30; // Tougher to go up than down.
          g->u.fatigue += 40;
          g->u.thirst += 30;
          g->u.mod_pain(5);
      }
      else {
          add_msg(_("You drill up into a preexisting space."));
          g->u.hunger += 40;
          g->u.fatigue += 50;
          g->u.thirst += 40;
          if (!(g->u.has_trait("NOPAIN"))) {
              add_msg(m_bad, _("You're quite sore from all that work."));
              g->u.mod_pain(5);
          }
      }
      g->m.ter_set(p.x, p.y, t_stairs_up); // There's the bottom half
      // Again, need to use submap-local coordinates.
      tmpmap.ter_set(p.x % SEEX, p.y % SEEY, t_stairs_down); // and there's the top half.
      // And save to the center coordinate of the current active map.
      tmpmap.save();
   }
}

void load_construction(JsonObject &jo)
{
    construction *con = new construction;

    con->description = _(jo.get_string("description").c_str());
    con->skill = jo.get_string("skill", "carpentry");
    con->difficulty = jo.get_int("difficulty");
    con->load(jo);
    // constructions use different time units in json, this makes it compatible
    // with recipes/requirements, TODO: should be changed in json
    con->time *= 1000;

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
    } else if (prefunc == "check_deconstruct") {
        con->pre_special = &construct::check_deconstruct;
    } else if (prefunc == "check_up_OK") {
        con->pre_special = &construct::check_up_OK;
    } else if (prefunc == "check_down_OK") {
        con->pre_special = &construct::check_down_OK;
    } else {
        if (prefunc != "") {
            debugmsg("Unknown pre_special function: %s", prefunc.c_str());
        }
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
    } else if (postfunc == "done_dig_stair") {
        con->post_special = &construct::done_dig_stair;
    } else if (postfunc == "done_mine_downstair") {
        con->post_special = &construct::done_mine_downstair;
    } else if (postfunc == "done_mine_upstair") {
        con->post_special = &construct::done_mine_upstair;
    } else {
        if (postfunc != "") {
            debugmsg("Unknown post_special function: %s", postfunc.c_str());
        }
        con->post_special = &construct::done_nothing;
    }

    con->id = constructions.size();
    constructions.push_back(con);
}

void reset_constructions()
{
    for( std::vector<construction *>::iterator a = constructions.begin();
         a != constructions.end(); ++a ) {
        delete *a;
    }
    constructions.clear();
}

void check_constructions()
{
    for( std::vector<construction *>::const_iterator a = constructions.begin();
         a != constructions.end(); ++a ) {
        const construction *c = *a;
        const std::string display_name = std::string("construction ") + c->description;
        // Note: print the description as the id is just a generated number,
        // the description can be searched for in the json files.
        if (!c->skill.empty() && Skill::skill(c->skill) == NULL) {
            debugmsg("Unknown skill %s in %s", c->skill.c_str(), display_name.c_str());
        }
        c->check_consistency(display_name);
        if (!c->pre_terrain.empty() && !c->pre_is_furniture && termap.count(c->pre_terrain) == 0) {
            debugmsg("Unknown pre_terrain (terrain) %s in %s", c->pre_terrain.c_str(), display_name.c_str());
        }
        if (!c->pre_terrain.empty() && c->pre_is_furniture && furnmap.count(c->pre_terrain) == 0) {
            debugmsg("Unknown pre_terrain (furniture) %s in %s", c->pre_terrain.c_str(), display_name.c_str());
        }
        if (!c->post_terrain.empty() && !c->post_is_furniture && termap.count(c->post_terrain) == 0) {
            debugmsg("Unknown post_terrain (terrain) %s in %s", c->post_terrain.c_str(), display_name.c_str());
        }
        if (!c->post_terrain.empty() && c->post_is_furniture && furnmap.count(c->post_terrain) == 0) {
            debugmsg("Unknown post_terrain (furniture) %s in %s", c->post_terrain.c_str(), display_name.c_str());
        }
    }
}
