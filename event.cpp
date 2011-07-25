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
  default:
   break; // Nothing happens for other events
 }
}
