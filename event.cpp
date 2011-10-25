#include "event.h"
#include "npc.h"
#include "game.h"
#include "rng.h"

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
    g->active_npc.push_back(tmp);
   }
  } break;

  case EVENT_ROBOT_ATTACK: {
   if (rl_dist(g->levx, g->levy, map_point.x, map_point.y) <= 4) {
    mtype *robot_type = g->mtypes[mon_tripod];
    if (faction_id == 0) // The cops!
     robot_type = g->mtypes[mon_copbot];
    monster robot(robot_type);
    int robx = (g->levx > map_point.x ? 0 - SEEX * 2 : SEEX * 4),
        roby = (g->levy > map_point.y ? 0 - SEEY * 2 : SEEY * 4);
    robot.spawn(robx, roby);
    g->z.push_back(robot);
   }
  } break;

  case EVENT_SPAWN_WYRMS: {
   if (g->levz >= 0)
    return;
   monster wyrm(g->mtypes[mon_dark_wyrm]);
   int num_wyrms = rng(2, 6);
   for (int i = 0; i < num_wyrms; i++) {
    int tries = 0;
    int monx = -1, mony = -1;
    do {
     monx = rng(0, SEEX * 3);
     mony = rng(0, SEEY * 3);
     tries++;
    } while (tries < 10 && !g->is_empty(monx, mony) &&
             rl_dist(g->u.posx, g->u.posx, monx, mony) <= 2);
    if (tries < 10) {
     wyrm.spawn(monx, mony);
     g->z.push_back(wyrm);
    }
   }
   if (!one_in(15)) // They just keep coming!
    g->add_event(EVENT_SPAWN_WYRMS, int(g->turn) + rng(15, 25));
  } break;

  case EVENT_AMIGARA: {
   int num_horrors = rng(3, 5);
   int faultx = -1, faulty = -1;
   bool horizontal;
   for (int x = 0; x < SEEX * 3 && faultx == -1; x++) {
    for (int y = 0; y < SEEY * 3 && faulty == -1; y++) {
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
   monster horror(g->mtypes[mon_amigara_horror]);
   for (int i = 0; i < num_horrors; i++) {
    int tries = 0;
    int monx = -1, mony = -1;
    do {
     tries = 0;
     if (horizontal) {
      monx = rng(0, SEEX * 3);
      for (int n = -1; n <= 1; n++) {
       if (g->m.ter(monx, faulty + n) == t_rock_floor)
        mony = faulty + n;
      }
     } else { // Vertical fault
      mony = rng(0, SEEY * 3);
      for (int n = -1; n <= 1; n++) {
       if (g->m.ter(faultx + n, mony) == t_rock_floor)
        monx = faultx + n;
      }
     }
     tries++;
    } while (!g->is_empty(monx, mony) && tries < 10);
    horror.spawn(monx, mony);
    g->z.push_back(horror);
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
   if (g->levz >= 0 && one_in(100)) { // About once every 10 minutes
    monster eyebot(g->mtypes[mon_eyebot]);
    eyebot.faction_id = faction_id;
    point place = g->m.random_outdoor_tile();
    if (place.x == -1 && place.y == -1)
     return; // We're safely indoors!
    eyebot.spawn(place.x, place.y);
    g->z.push_back(eyebot);
    int t;
    if (g->u_see(place.x, place.y, t))
     g->add_msg("An eyebot swoops down nearby!");
   }
  } break;

  case EVENT_SPAWN_WYRMS:
   if (g->levz >= 0) {
    turn--;
    return;
   }
   if (int(g->turn) % 3 == 0)
    g->add_msg("You hear screeches from the rock above and around you!");
   break;

  case EVENT_AMIGARA:
   g->add_msg("The entire cavern shakes!");
   break;

  default:
   break; // Nothing happens for other events
 }
}
