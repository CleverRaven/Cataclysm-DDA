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

void mission_start::infect_npc(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 if (p == NULL) {
  debugmsg("mission_start::infect_npc() couldn't find an NPC!");
  return;
 }
 p->add_disease(DI_INFECTION, -1, g);
 for (int i = 0; i < p->inv.size(); i++) {
  if (p->inv[i].type->id == itm_antibiotics) {
   p->inv.remove_stack(i);
   i--;
  }
 }
}

void mission_start::place_dog(game *g, mission *miss)
{
 int city_id = g->cur_om.closest_city( g->om_location() );
 point house = g->cur_om.random_house_in_city(city_id);
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item(g->itypes[itm_dog_whistle], 0) );
 g->add_msg("%s gave you a dog whistle.", dev->name.c_str());

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

void mission_start::place_npc_software(game *g, mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item(g->itypes[itm_usb_drive], 0) );
 g->add_msg("%s gave you a USB drive.", dev->name.c_str());

 oter_id ter = ot_house_north;

 switch (dev->myclass) {
 case NC_HACKER:
  miss->item_id = itm_software_hacking;
  break;
 case NC_DOCTOR:
  miss->item_id = itm_software_medical;
  ter = ot_s_pharm_north;
  miss->follow_up = MISSION_GET_ZOMBIE_BLOOD_ANAL;
  break;
 case NC_SCIENTIST:
  miss->item_id = itm_software_math;
  break;
 default:
  miss->item_id = itm_software_useless;
 }

 int dist = 0;
 point place;
 if (ter == ot_house_north) {
  int city_id = g->cur_om.closest_city( g->om_location() );
  place = g->cur_om.random_house_in_city(city_id);
 } else
  place = g->cur_om.find_closest(g->om_location(), ter, 4, dist, false);
 miss->target = place;
// Make it seen on our map
 for (int x = place.x - 6; x <= place.x + 6; x++) {
  for (int y = place.y - 6; y <= place.y + 6; y++)
   g->cur_om.seen(x, y) = true;
 }
 tinymap compmap(&(g->itypes), &(g->mapitems), &(g->traps));
 compmap.load(g, place.x * 2, place.y * 2);
 point comppoint;

 switch (g->cur_om.ter(place.x, place.y)) {
 case ot_house_north:
 case ot_house_east:
 case ot_house_west:
 case ot_house_south: {
  std::vector<point> valid;
  for (int x = 0; x < SEEX * 2; x++) {
   for (int y = 0; y < SEEY * 2; y++) {
    if (compmap.ter(x, y) == t_floor) {
     bool okay = false;
     for (int x2 = x - 1; x2 <= x + 1 && !okay; x2++) {
      for (int y2 = y - 1; y2 <= y + 1 && !okay; y2++) {
       if (compmap.ter(x2, y2) == t_bed || compmap.ter(x2, y2) == t_dresser) {
        okay = true;
        valid.push_back( point(x, y) );
       }
      }
     }
    }
   }
  }
  if (valid.empty())
   comppoint = point( rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7) );
  else
   comppoint = valid[rng(0, valid.size() - 1)];
 } break;
 case ot_s_pharm_north: {
  bool found = false;
  for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
   for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
    if (compmap.ter(x, y) == t_floor) {
     found = true;
     comppoint = point(x, y);
    }
   }
  }
 } break;
 case ot_s_pharm_east: {
  bool found = false;
  for (int x = 0; x < SEEX * 2 && !found; x++) {
   for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
    if (compmap.ter(x, y) == t_floor) {
     found = true;
     comppoint = point(x, y);
    }
   }
  }
 } break;
 case ot_s_pharm_south: {
  bool found = false;
  for (int x = 0; x < SEEX * 2 && !found; x++) {
   for (int y = 0; y < SEEY * 2 && !found; y++) {
    if (compmap.ter(x, y) == t_floor) {
     found = true;
     comppoint = point(x, y);
    }
   }
  }
 } break;
 case ot_s_pharm_west: {
  bool found = false;
  for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
   for (int y = 0; y < SEEY * 2 && !found; y++) {
    if (compmap.ter(x, y) == t_floor) {
     found = true;
     comppoint = point(x, y);
    }
   }
  }
 } break;
 }

 std::stringstream compname;
 compname << dev->name << "'s Terminal";
 compmap.ter(comppoint.x, comppoint.y) = t_console;
 computer *tmpcomp = compmap.add_computer(comppoint.x, comppoint.y,
                                          compname.str(), 0);
 tmpcomp->mission_id = miss->uid;
 tmpcomp->add_option("Download Software", COMPACT_DOWNLOAD_SOFTWARE, 0);

 compmap.save(&(g->cur_om), int(g->turn), place.x * 2, place.y * 2);
}

void mission_start::reveal_hospital(game *g, mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item(g->itypes[itm_vacutainer], 0) );
  g->add_msg("%s gave you a vacutainer.", dev->name.c_str());
 }
 int dist = 0;
 point place = g->cur_om.find_closest(g->om_location(), ot_hospital, 1, dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om.seen(x, y) = true;
 }
 miss->target = place;
}

void mission_start::find_safety(game *g, mission *miss)
{
 point place = g->om_location();
 bool done = false;
 for (int radius = 0; radius <= 20 && !done; radius++) {
  for (int dist = 0 - radius; dist <= radius && !done; dist++) {
   int offset = rng(0, 3); // Randomizes the direction we check first
   for (int i = 0; i <= 3 && !done; i++) { // Which direction?
    point check = place;
    switch ( (offset + i) % 4 ) {
     case 0: check.x += dist; check.y -= radius; break;
     case 1: check.x += dist; check.y += radius; break;
     case 2: check.y += dist; check.x -= radius; break;
     case 3: check.y += dist; check.x += radius; break;
    }
    if (g->cur_om.is_safe(check.x, check.y)) {
     miss->target = check;
     done = true;
    }
   }
  }
 }
 if (!done) { // Couldn't find safety; so just set the target to far away
  switch ( rng(0, 3) ) {
   case 0: miss->target = point(place.x - 20, place.y - 20); break;
   case 1: miss->target = point(place.x - 20, place.y + 20); break;
   case 2: miss->target = point(place.x + 20, place.y - 20); break;
   case 3: miss->target = point(place.x + 20, place.y + 20); break;
  }
 }
}

void mission_start::place_book(game *g, mission *miss)
{
}
