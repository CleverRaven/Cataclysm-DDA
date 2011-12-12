#include "mission.h"
#include "game.h"
#include <sstream>

/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mission_start::standard(game *g, mission *miss)
{
}

void mission_start::place_dog(game *g, mission *miss)
{
 int city_id = g->cur_om.closest_city( g->om_location() );
 point house = g->cur_om.random_house_in_city(city_id);

 miss->target = house;
// Make it seen on our map
 for (int x = house.x - 6; x <= house.x + 6; x++) {
  for (int y = house.y - 6; y <= house.y + 6; y++)
   g->cur_om.seen(x, y) = true;
 }

 tinymap doghouse(&(g->itypes), &(g->mapitems), &(g->traps));
 doghouse.load(g, house.x * 2, house.y * 2);
 doghouse.add_spawn(mon_dog, 1, SEEX, SEEY, true, -1, miss->uid);
 doghouse.save(&(g->cur_om), int(g->turn), house.x * 2, house.y * 2);
}

void mission_start::place_zombie_mom(game *g, mission *miss)
{
 int city_id = g->cur_om.closest_city( g->om_location() );
 point house = g->cur_om.random_house_in_city(city_id);

 miss->target = house;
// Make it seen on our map
 for (int x = house.x - 6; x <= house.x + 6; x++) {
  for (int y = house.y - 6; y <= house.y + 6; y++)
   g->cur_om.seen(x, y) = true;
 }

 tinymap zomhouse(&(g->itypes), &(g->mapitems), &(g->traps));
 zomhouse.load(g, house.x * 2, house.y * 2);
 zomhouse.add_spawn(mon_zombie, 1, SEEX, SEEY, false, -1, miss->uid,
                    random_first_name(false));
 zomhouse.save(&(g->cur_om), int(g->turn), house.x * 2, house.y * 2);
}
