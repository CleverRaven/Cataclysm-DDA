#include "mission.h"
#include "game.h"
#include "name.h"
#include <sstream>
#include "omdata.h"
/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mission_start::standard(game *, mission *)
{
}

void mission_start::join(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;
}

void mission_start::infect_npc(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 if (p == NULL) {
  debugmsg("mission_start::infect_npc() couldn't find an NPC!");
  return;
 }
 p->add_disease("infection", 1, true);
 // make sure they don't have any antibiotics
 while ( !p->inv.remove_item("antibiotics").is_null() ) { /* empty */ }
}

void mission_start::place_dog(game *g, mission *miss)
{
 int city_id = g->cur_om->closest_city( g->om_location() );
 point house = g->cur_om->random_house_in_city(city_id);
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item(itypes["dog_whistle"], 0) );
 g->add_msg(_("%s gave you a dog whistle."), dev->name.c_str());

 miss->target = house;
// Make it seen on our map
 for (int x = house.x - 6; x <= house.x + 6; x++) {
  for (int y = house.y - 6; y <= house.y + 6; y++)
   g->cur_om->seen(x, y, 0) = true;
 }

 tinymap doghouse(&(g->traps));
 doghouse.load(g, house.x * 2, house.y * 2, 0, false);
 doghouse.add_spawn("mon_dog", 1, SEEX, SEEY, true, -1, miss->uid);
 doghouse.save(g->cur_om, int(g->turn), house.x * 2, house.y * 2, 0);
}

void mission_start::place_zombie_mom(game *g, mission *miss)
{
 int city_id = g->cur_om->closest_city( g->om_location() );
 point house = g->cur_om->random_house_in_city(city_id);

 miss->target = house;
// Make it seen on our map
 for (int x = house.x - 6; x <= house.x + 6; x++) {
  for (int y = house.y - 6; y <= house.y + 6; y++)
   g->cur_om->seen(x, y, 0) = true;
 }

 tinymap zomhouse(&(g->traps));
 zomhouse.load(g, house.x * 2, house.y * 2,  0, false);
 zomhouse.add_spawn("mon_zombie", 1, SEEX, SEEY, false, -1, miss->uid, Name::get(nameIsFemaleName | nameIsGivenName));
 zomhouse.save(g->cur_om, int(g->turn), house.x * 2, house.y * 2, 0);
}

void mission_start::place_jabberwock(game *g, mission *miss)
{
 int dist = 0;
 point site = g->cur_om->find_closest(g->om_location(), "forest_thick", dist, false);
 miss->target = site;
// Make it seen on our map
 for (int x = site.x - 6; x <= site.x + 6; x++) {
  for (int y = site.y - 6; y <= site.y + 6; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 tinymap grove(&(g->traps));
 grove.load(g, site.x * 2, site.y * 2,  0, false);
 grove.add_spawn("mon_jabberwock", 1, SEEX, SEEY, false, -1, miss->uid, "NONE");
 grove.save(g->cur_om, int(g->turn), site.x * 2, site.y * 2, 0);
}

void mission_start::kill_100_z(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you
 miss->monster_type = "mon_zombie";
 int killed = 0;
 killed += g->kill_count("mon_zombie");
 miss->monster_kill_goal = 100+killed;//your kill score must increase by 100
}

void mission_start::kill_horde_master(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you
 int dist = 0;//pick one of the below locations for the horde to haunt
 point site = g->cur_om->find_closest(g->om_location(), "office_tower_1", dist, false);
 if (site.x == -1 && site.y == -1 )
    site = g->cur_om->find_closest(g->om_location(), "hotel_tower_1_8", dist, false);
 if (site.x == -1 && site.y == -1)
    site = g->cur_om->find_closest(g->om_location(), "school_5", dist, false);
 if (site.x == -1 && site.y == -1)
    site = g->cur_om->find_closest(g->om_location(), "forest_thick", dist, false);
 miss->target = site;
// Make it seen on our map
 for (int x = site.x - 6; x <= site.x + 6; x++) {
  for (int y = site.y - 6; y <= site.y + 6; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 tinymap tile(&(g->traps));
 tile.load(g, site.x * 2, site.y * 2,  0, false);
 tile.add_spawn("mon_zombie_master", 1, SEEX, SEEY, false, -1, miss->uid, "Demonic Soul");
 tile.add_spawn("mon_zombie_brute",3,SEEX,SEEY);
 tile.add_spawn("mon_zombie_fast",3,SEEX,SEEY);
 if (SEEX > 1 && SEEX < OMAPX && SEEY > 1 && SEEY < OMAPY){
 for (int x = SEEX - 1; x <= SEEX + 1; x++) {
  for (int y = SEEY - 1; y <= SEEY + 1; y++)
   tile.add_spawn("mon_zombie",rng(3,10),x,y);
   tile.add_spawn("mon_zombie_fast",rng(0,2),SEEX,SEEY);
 }
}
 tile.add_spawn("mon_zombie_necro",2,SEEX,SEEY);
 tile.add_spawn("mon_zombie_hulk",1,SEEX,SEEY);
 tile.save(g->cur_om, int(g->turn), site.x * 2, site.y * 2, 0);
}

void mission_start::place_npc_software(game *g, mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item(itypes["usb_drive"], 0) );
 g->add_msg(_("%s gave you a USB drive."), dev->name.c_str());

 std::string type = "house";

 switch (dev->myclass) {
 case NC_HACKER:
  miss->item_id = "software_hacking";
  break;
 case NC_DOCTOR:
  miss->item_id = "software_medical";
  type = "s_pharm";
  miss->follow_up = MISSION_GET_ZOMBIE_BLOOD_ANAL;
  break;
 case NC_SCIENTIST:
  miss->item_id = "software_math";
  break;
 default:
  miss->item_id = "software_useless";
 }

    int dist = 0;
    point place;
    if (type == "house") {
        int city_id = g->cur_om->closest_city( g->om_location() );
        place = g->cur_om->random_house_in_city(city_id);
    } else {
        place = g->cur_om->find_closest(g->om_location(), type, dist, false);
    }
    miss->target = place;

// Make it seen on our map
 for (int x = place.x - 6; x <= place.x + 6; x++) {
  for (int y = place.y - 6; y <= place.y + 6; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 tinymap compmap(&(g->traps));
 compmap.load(g, place.x * 2, place.y * 2, 0, false);
 point comppoint;

    oter_id oter = g->cur_om->ter(place.x, place.y, 0);
    if (oter == "house_north" || oter == "house_east"
            || oter == "house_south" || oter == "house_west") {
        std::vector<point> valid;
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (compmap.ter(x, y) == t_floor && compmap.furn(x, y) == f_null) {
                    bool okay = false;
                    for (int x2 = x - 1; x2 <= x + 1 && !okay; x2++) {
                        for (int y2 = y - 1; y2 <= y + 1 && !okay; y2++) {
                            if (compmap.furn(x2, y2) == f_bed || compmap.furn(x2, y2) == f_dresser) {
                                okay = true;
                                valid.push_back( point(x, y) );
                            }
                        }
                    }
                }
            }
        }
        if (valid.empty()) {
            comppoint = point( rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7) );
        } else {
            comppoint = valid[rng(0, valid.size() - 1)];
        }
    } else if (oter == "s_pharm_north") {
        bool found = false;
        for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
            for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
                if (compmap.ter(x, y) == t_floor) {
                    found = true;
                    comppoint = point(x, y);
                }
            }
        }
    } else if (oter == "s_pharm_east") {
        bool found = false;
        for (int x = 0; x < SEEX * 2 && !found; x++) {
            for (int y = SEEY * 2 - 1; y > 0 && !found; y--) {
                if (compmap.ter(x, y) == t_floor) {
                    found = true;
                    comppoint = point(x, y);
                }
            }
        }
    } else if (oter == "s_pharm_south") {
        bool found = false;
        for (int x = 0; x < SEEX * 2 && !found; x++) {
            for (int y = 0; y < SEEY * 2 && !found; y++) {
                if (compmap.ter(x, y) == t_floor) {
                    found = true;
                    comppoint = point(x, y);
                }
            }
        }
    } else if (oter == "s_pharm_west") {
        bool found = false;
        for (int x = SEEX * 2 - 1; x > 0 && !found; x--) {
            for (int y = 0; y < SEEY * 2 && !found; y++) {
                if (compmap.ter(x, y) == t_floor) {
                    found = true;
                    comppoint = point(x, y);
                }
            }
        }
    }

 compmap.ter_set(comppoint.x, comppoint.y, t_console);
 computer *tmpcomp = compmap.add_computer(comppoint.x, comppoint.y, string_format(_("%s's Terminal"), dev->name.c_str()), 0);
 tmpcomp->mission_id = miss->uid;
 tmpcomp->add_option(_("Download Software"), COMPACT_DOWNLOAD_SOFTWARE, 0);

 compmap.save(g->cur_om, int(g->turn), place.x * 2, place.y * 2, 0);
}

void mission_start::place_priest_diary(game *g, mission *miss)
{
 point place;
 int city_id = g->cur_om->closest_city( g->om_location() );
 place = g->cur_om->random_house_in_city(city_id);
 miss->target = place;
 for (int x = place.x - 2; x <= place.x + 2; x++) {
  for (int y = place.y - 2; y <= place.y + 2; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 tinymap compmap(&(g->traps));
 compmap.load(g, place.x * 2, place.y * 2, 0, false);
 point comppoint;

  std::vector<point> valid;
  for (int x = 0; x < SEEX * 2; x++) {
   for (int y = 0; y < SEEY * 2; y++) {
    if (compmap.furn(x, y) == f_bed || compmap.furn(x, y) == f_dresser || compmap.furn(x, y) == f_indoor_plant || compmap.furn(x, y) == f_cupboard) {
      valid.push_back( point(x, y) );
    }
   }
  }
  if (valid.empty())
   comppoint = point( rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7) );
  else
   comppoint = valid[rng(0, valid.size() - 1)];
 compmap.spawn_item(comppoint.x, comppoint.y, "priest_diary");
 compmap.save(g->cur_om, int(g->turn), place.x * 2, place.y * 2, 0);
}

void mission_start::place_deposit_box(game *g, mission *miss)
{
    npc *p = g->find_npc(miss->npc_id);
    p->attitude = NPCATT_FOLLOW;//npc joins you
    int dist = 0;
    point site = g->cur_om->find_closest(g->om_location(), "bank", dist, false);
    if (site.x == -1 && site.y == -1) {
        site = g->cur_om->find_closest(g->om_location(), "office_tower_1", dist, false);
    }
    miss->target = site;

 for (int x = site.x - 2; x <= site.x + 2; x++) {
  for (int y = site.y - 2; y <= site.y + 2; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 tinymap compmap(&(g->traps));
 compmap.load(g, site.x * 2, site.y * 2, 0, false);
 point comppoint;
  std::vector<point> valid;
  for (int x = 0; x < SEEX * 2; x++) {
   for (int y = 0; y < SEEY * 2; y++) {
    if (compmap.ter(x, y) == t_floor) {
     bool okay = false;
     for (int x2 = x - 1; x2 <= x + 1 && !okay; x2++) {
      for (int y2 = y - 1; y2 <= y + 1 && !okay; y2++) {
       if (compmap.ter(x2, y2) == t_wall_metal_h|| compmap.ter(x2, y2) == t_wall_metal_v) {
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
compmap.spawn_item(comppoint.x, comppoint.y, "safe_box");
compmap.save(g->cur_om, int(g->turn), site.x * 2, site.y * 2, 0);
}

void mission_start::reveal_lab_black_box(game *g, mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item(itypes["black_box"], 0) );
  g->add_msg(_("%s gave you back the black box."), dev->name.c_str());
 }
 int dist = 0;
 point place = g->cur_om->find_closest(g->om_location(), "lab", dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 miss->target = place;
}

void mission_start::open_sarcophagus(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;
 if (p != NULL) {
  g->u.i_add( item(itypes["sarcophagus_access_code"], 0) );
  g->add_msg(_("%s gave you sarcophagus access code."), p->name.c_str());
 }
 int dist = 0;
 point place = g->cur_om->find_closest(g->om_location(), "haz_sar", dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 miss->target = place;
}

void mission_start::reveal_hospital(game *g, mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item(itypes["vacutainer"], 0) );
  g->add_msg(_("%s gave you a vacutainer."), dev->name.c_str());
 }
 int dist = 0;
 point place = g->cur_om->find_closest(g->om_location(), "hospital", dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om->seen(x, y, 0) = true;
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
    if (g->cur_om->is_safe(check.x, check.y, g->levz)) {
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

void mission_start::point_prison(game *g, mission *miss)
{
 int dist = 0;
 point place = g->cur_om->find_closest(g->om_location(), "prison_5", dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 miss->target = place;
}

void mission_start::point_cabin_strange(game *g, mission *miss)
{
 int dist = 0;
 point place = g->cur_om->find_closest(g->om_location(), "cabin_strange", dist,
                                      false);
 for (int x = place.x - 3; x <= place.x + 3; x++) {
  for (int y = place.y - 3; y <= place.y + 3; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 miss->target = place;
}

void mission_start::recruit_tracker(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you

 int dist = 0;
 point site = g->cur_om->find_closest(g->om_location(), "cabin", dist, false);
 miss->target = site;
 miss->recruit_class = NC_COWBOY;

// Make it seen on our map
for (int x = site.x - 2; x <= site.x + 2; x++) {
  for (int y = site.y - 2; y <= site.y + 2; y++)
   g->cur_om->seen(x, y, 0) = true;
 }
 npc * temp = new npc();
 temp->normalize(g);
 temp->randomize(g, NC_COWBOY);
 temp->omx = site.x*2;
 temp->omy = site.y*2;
 temp->mapx = site.x*2;
 temp->mapy = site.y*2;
 temp->posx = 11;
 temp->posy = 11;
 temp->attitude = NPCATT_TALK;
 temp->mission = NPC_MISSION_SHOPKEEP;
 temp->personality.aggression -= 1;
 temp->op_of_u.owed = 10; int mission_index = g->reserve_mission(MISSION_JOIN_TRACKER, temp->getID());
 if (mission_index != -1)
    temp->chatbin.missions.push_back(mission_index);
 g->cur_om->npcs.push_back(temp);
}

void mission_start::place_book(game *, mission *)
{
}
