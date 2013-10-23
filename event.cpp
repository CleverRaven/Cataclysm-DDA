#include "event.h"
#include "npc.h"
#include "game.h"
#include "rng.h"
#include "options.h"
#include "translations.h"
#include "monstergenerator.h"

void event::actualize(game *g)
{
 switch (type) {

  case EVENT_HELP: {
   npc tmp;
   int num = 1;
   if (faction_id >= 0)
    num = rng(1, 6);
   for (int i = 0; i < num; i++) {
    if (faction_id != -1) {
     faction* fac = g->faction_by_id(faction_id);
     if (fac)
      tmp.randomize_from_faction(g, fac);
     else
      debugmsg("EVENT_HELP run with invalid faction_id");
    } else
     tmp.randomize(g);
    tmp.attitude = NPCATT_DEFEND;
    tmp.posx = g->u.posx - SEEX * 2 + rng(-5, 5);
    tmp.posy = g->u.posy - SEEY * 2 + rng(-5, 5);
    g->active_npc.push_back(&tmp);
   }
  } break;

  case EVENT_ROBOT_ATTACK: {
   if (rl_dist(g->levx, g->levy, map_point.x, map_point.y) <= 4) {
    mtype *robot_type = GetMType("mon_tripod");
    if (faction_id == 0) { // The cops!
     robot_type = GetMType("mon_copbot");
     g->u.add_memorial_log(_("Became wanted by the police!"));
    }
    monster robot(robot_type);
    int robx = (g->levx > map_point.x ? 0 - SEEX * 2 : SEEX * 4),
        roby = (g->levy > map_point.y ? 0 - SEEY * 2 : SEEY * 4);
    robot.spawn(robx, roby);
    g->add_zombie(robot);
   }
  } break;

  case EVENT_SPAWN_WYRMS: {
   if (g->levz >= 0)
    return;
   g->u.add_memorial_log(_("Awoke a group of dark wyrms!"));
   monster wyrm(GetMType("mon_dark_wyrm"));
   int num_wyrms = rng(1, 4);
   for (int i = 0; i < num_wyrms; i++) {
    int tries = 0;
    int monx = -1, mony = -1;
    do {
     monx = rng(0, SEEX * MAPSIZE);
     mony = rng(0, SEEY * MAPSIZE);
     tries++;
    } while (tries < 10 && !g->is_empty(monx, mony) &&
             rl_dist(g->u.posx, g->u.posx, monx, mony) <= 2);
    if (tries < 10) {
     wyrm.spawn(monx, mony);
     g->add_zombie(wyrm);
    }
   }
   if (!one_in(25)) // They just keep coming!
    g->add_event(EVENT_SPAWN_WYRMS, int(g->turn) + rng(15, 25));
  } break;

  case EVENT_AMIGARA: {
   g->u.add_memorial_log(_("Angered a group of amigara horrors!"));
   int num_horrors = rng(3, 5);
   int faultx = -1, faulty = -1;
   bool horizontal = false;
   for (int x = 0; x < SEEX * MAPSIZE && faultx == -1; x++) {
    for (int y = 0; y < SEEY * MAPSIZE && faulty == -1; y++) {
     if (g->m.ter(x, y) == t_fault) {
      faultx = x;
      faulty = y;
      if (g->m.ter(x - 1, y) == t_fault || g->m.ter(x + 1, y) == t_fault)
       horizontal = true;
      else
       horizontal = false;
     }
    }
   }
   monster horror(GetMType("mon_amigara_horror"));
   for (int i = 0; i < num_horrors; i++) {
    int tries = 0;
    int monx = -1, mony = -1;
    do {
     if (horizontal) {
      monx = rng(faultx, faultx + 2 * SEEX - 8);
      for (int n = -1; n <= 1; n++) {
       if (g->m.ter(monx, faulty + n) == t_rock_floor)
        mony = faulty + n;
      }
     } else { // Vertical fault
      mony = rng(faulty, faulty + 2 * SEEY - 8);
      for (int n = -1; n <= 1; n++) {
       if (g->m.ter(faultx + n, mony) == t_rock_floor)
        monx = faultx + n;
      }
     }
     tries++;
    } while ((monx == -1 || mony == -1 || g->is_empty(monx, mony)) &&
             tries < 10);
    if (tries < 10) {
     horror.spawn(monx, mony);
     g->add_zombie(horror);
    }
   }
  } break;

  case EVENT_ROOTS_DIE:
   g->u.add_memorial_log(_("Destroyed a triffid grove."));
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_root_wall && one_in(3))
      g->m.ter_set(x, y, t_underbrush);
    }
   }
   break;

  case EVENT_TEMPLE_OPEN: {
   g->u.add_memorial_log(_("Opened a strange temple."));
   bool saw_grate = false;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_grate) {
      g->m.ter_set(x, y, t_stairs_down);
      if (!saw_grate && g->u_see(x, y))
       saw_grate = true;
     }
    }
   }
   if (saw_grate)
    g->add_msg(_("The nearby grates open to reveal a staircase!"));
  } break;

  case EVENT_TEMPLE_FLOOD: {
   bool flooded = false;

   ter_id flood_buf[SEEX*MAPSIZE][SEEY*MAPSIZE];
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
     flood_buf[x][y] = g->m.ter(x, y);
   }
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_water_sh) {
      bool deepen = false;
      for (int wx = x - 1;  wx <= x + 1 && !deepen; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !deepen; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp)
         deepen = true;
       }
      }
      if (deepen) {
       flood_buf[x][y] = t_water_dp;
       flooded = true;
      }
     } else if (g->m.ter(x, y) == t_rock_floor) {
      bool flood = false;
      for (int wx = x - 1;  wx <= x + 1 && !flood; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !flood; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp || g->m.ter(wx, wy) == t_water_sh)
         flood = true;
       }
      }
      if (flood) {
       flood_buf[x][y] = t_water_sh;
       flooded = true;
      }
     }
    }
   }
   if (!flooded)
    return; // We finished flooding the entire chamber!
// Check if we should print a message
   if (flood_buf[g->u.posx][g->u.posy] != g->m.ter(g->u.posx, g->u.posy)) {
    if (flood_buf[g->u.posx][g->u.posy] == t_water_sh) {
     g->add_msg(_("Water quickly floods up to your knees."));
     g->u.add_memorial_log(_("Water level reached knees."));
    } else { // Must be deep water!
     g->add_msg(_("Water fills nearly to the ceiling!"));
     g->u.add_memorial_log(_("Water level reached the ceiling."));
     g->plswim(g->u.posx, g->u.posy);
    }
   }
// flood_buf is filled with correct tiles; now copy them back to g->m
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
       g->m.ter_set(x, y, flood_buf[x][y]);
   }
   g->add_event(EVENT_TEMPLE_FLOOD, int(g->turn) + rng(2, 3));
  } break;

  case EVENT_TEMPLE_SPAWN: {
   std::string montype = "mon_null";
   switch (rng(1, 4)) {
    case 1: montype = "mon_sewer_snake";  break;
    case 2: montype = "mon_centipede";    break;
    case 3: montype = "mon_dermatik";     break;
    case 4: montype = "mon_spider_widow"; break;
   }
   monster spawned( GetMType(montype) );
   int tries = 0, x, y;
   do {
    x = rng(g->u.posx - 5, g->u.posx + 5);
    y = rng(g->u.posy - 5, g->u.posy + 5);
    tries++;
   } while (tries < 20 && !g->is_empty(x, y) &&
            rl_dist(x, y, g->u.posx, g->u.posy) <= 2);
   if (tries < 20) {
    spawned.spawn(x, y);
    g->add_zombie(spawned);
   }
  } break;

  default:
   break; // Nothing happens for other events
 }
}

void event::per_turn(game *g)
{
 switch (type) {
  case EVENT_WANTED: {
   // About once every 10 minutes. Suppress in classic zombie mode.
   if (g->levz >= 0 && one_in(100) && !ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
    monster eyebot(GetMType("mon_eyebot"));
    eyebot.faction_id = faction_id;
    point place = g->m.random_outdoor_tile();
    if (place.x == -1 && place.y == -1)
     return; // We're safely indoors!
    eyebot.spawn(place.x, place.y);
    g->add_zombie(eyebot);
    if (g->u_see(place.x, place.y))
     g->add_msg(_("An eyebot swoops down nearby!"));
   }
  } break;

  case EVENT_SPAWN_WYRMS:
   if (g->levz >= 0) {
    turn--;
    return;
   }
   if (int(g->turn) % 3 == 0)
    g->add_msg(_("You hear screeches from the rock above and around you!"));
   break;

  case EVENT_AMIGARA:
   g->add_msg(_("The entire cavern shakes!"));
   break;

  case EVENT_TEMPLE_OPEN:
   g->add_msg(_("The earth rumbles."));
   break;


  default:
   break; // Nothing happens for other events
 }
}
