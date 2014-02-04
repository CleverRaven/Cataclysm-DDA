#include "mission.h"
#include "game.h"
#include "name.h"
#include <sstream>
#include "omdata.h"
#include "overmapbuffer.h"
/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

/**
 * Set target of mission to closest overmap terrain of that type,
 * reveal the area around it (uses overmapbuffer::reveal with reveal_rad),
 * and returns the mission target.
 */
point target_om_ter(const std::string &omter, int reveal_rad, mission *miss, bool must_see)
{
    int dist = 0;
    const point place = overmap_buffer.find_closest(
        g->om_global_location(), omter, dist, must_see);
    if(place != overmap::invalid_point && reveal_rad >= 0) {
        overmap_buffer.reveal(place, reveal_rad, g->levz);
    }
    miss->target = place;
    return place;
}

void mission_start::standard( mission *)
{
}

void mission_start::join(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;
}

void mission_start::infect_npc(mission *miss)
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

void mission_start::place_dog(mission *miss)
{
 int city_id = g->cur_om->closest_city( g->om_location() );
 point house = g->cur_om->random_house_in_city(city_id);
 // make it global coordinates
 house.x += g->cur_om->pos().x * OMAPX;
 house.y += g->cur_om->pos().y * OMAPY;
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item(itypes["dog_whistle"], 0) );
 g->add_msg(_("%s gave you a dog whistle."), dev->name.c_str());

 miss->target = house;
 overmap_buffer.reveal(house, 6, g->levz);

 tinymap doghouse(&(g->traps));
 // Get overmap, crop house coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(house.x, house.y);
 doghouse.load(house.x * 2, house.y * 2, g->levz, false, &om);
 doghouse.add_spawn("mon_dog", 1, SEEX, SEEY, true, -1, miss->uid);
 doghouse.save(&om, int(g->turn), house.x * 2, house.y * 2, g->levz);
}

void mission_start::place_zombie_mom(mission *miss)
{
 int city_id = g->cur_om->closest_city( g->om_location() );
 point house = g->cur_om->random_house_in_city(city_id);
 // make it global coordinates
 house.x += g->cur_om->pos().x * OMAPX;
 house.y += g->cur_om->pos().y * OMAPY;

 miss->target = house;
 overmap_buffer.reveal(house, 6, g->levz);

 tinymap zomhouse(&(g->traps));
 // Get overmap, crop house coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(house.x, house.y);
 zomhouse.load(house.x * 2, house.y * 2, g->levz, false, &om);
 zomhouse.add_spawn("mon_zombie", 1, SEEX, SEEY, false, -1, miss->uid, Name::get(nameIsFemaleName | nameIsGivenName));
 zomhouse.save(&om, int(g->turn), house.x * 2, house.y * 2, g->levz);
}

void mission_start::place_jabberwock(mission *miss)
{
    point site = target_om_ter("forest_thick", 6, miss, false);
 tinymap grove(&(g->traps));
 // Get overmap, crop site coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(site.x, site.y);
 grove.load(site.x * 2, site.y * 2, g->levz, false, &om);
 grove.add_spawn("mon_jabberwock", 1, SEEX, SEEY, false, -1, miss->uid, "NONE");
 grove.save(&om, int(g->turn), site.x * 2, site.y * 2, g->levz);
}

void mission_start::kill_100_z(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you
 miss->monster_type = "mon_zombie";
 int killed = 0;
 killed += g->kill_count("mon_zombie");
 miss->monster_kill_goal = 100+killed;//your kill score must increase by 100
}

void mission_start::kill_horde_master(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you
 int dist = 0;//pick one of the below locations for the horde to haunt
 point site = overmap_buffer.find_closest(g->om_global_location(), "office_tower_1", dist, false);
 if (site == overmap::invalid_point)
    site = overmap_buffer.find_closest(g->om_global_location(), "hotel_tower_1_8", dist, false);
 if (site == overmap::invalid_point)
    site = overmap_buffer.find_closest(g->om_global_location(), "school_5", dist, false);
 if (site == overmap::invalid_point)
    site = overmap_buffer.find_closest(g->om_global_location(), "forest_thick", dist, false);
 miss->target = site;
 overmap_buffer.reveal(site, 6, g->levz);
 tinymap tile(&(g->traps));
 // Get overmap, crop site coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(site.x, site.y);
 tile.load(site.x * 2, site.y * 2, g->levz, false, &om);
 tile.add_spawn("mon_zombie_master", 1, SEEX, SEEY, false, -1, miss->uid, "Demonic Soul");
 tile.add_spawn("mon_zombie_brute",3,SEEX,SEEY);
 tile.add_spawn("mon_zombie_dog",3,SEEX,SEEY);
 if (SEEX > 1 && SEEX < OMAPX && SEEY > 1 && SEEY < OMAPY){
 for (int x = SEEX - 1; x <= SEEX + 1; x++) {
  for (int y = SEEY - 1; y <= SEEY + 1; y++)
   tile.add_spawn("mon_zombie",rng(3,10),x,y);
   tile.add_spawn("mon_zombie_dog",rng(0,2),SEEX,SEEY);
 }
}
 tile.add_spawn("mon_zombie_necro",2,SEEX,SEEY);
 tile.add_spawn("mon_zombie_hulk",1,SEEX,SEEY);
 tile.save(&om, int(g->turn), site.x * 2, site.y * 2, g->levz);
}

void mission_start::place_npc_software(mission *miss)
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
        // make it global coordinates
        place.x += g->cur_om->pos().x * OMAPX;
        place.y += g->cur_om->pos().y * OMAPY;
    } else {
        place = overmap_buffer.find_closest(g->om_global_location(), type, dist, false);
    }
    miss->target = place;
    overmap_buffer.reveal(place, 6, g->levz);

 tinymap compmap(&(g->traps));
 // Get overmap, crop place coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(place.x, place.y);
 compmap.load(place.x * 2, place.y * 2, g->levz, false, &om);
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

 compmap.save(&om, int(g->turn), place.x * 2, place.y * 2, g->levz);
}

void mission_start::place_priest_diary(mission *miss)
{
 point place;
 int city_id = g->cur_om->closest_city( g->om_location() );
 place = g->cur_om->random_house_in_city(city_id);
 // make it global coordinates
 place.x += g->cur_om->pos().x * OMAPX;
 place.y += g->cur_om->pos().y * OMAPY;
 miss->target = place;
 overmap_buffer.reveal(place, 2, g->levz);
 tinymap compmap(&(g->traps));
 // Get overmap, crop place coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(place.x, place.y);
 compmap.load(place.x * 2, place.y * 2, g->levz, false, &om);
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
 compmap.save(&om, int(g->turn), place.x * 2, place.y * 2, g->levz);
}

void mission_start::place_deposit_box(mission *miss)
{
    npc *p = g->find_npc(miss->npc_id);
    p->attitude = NPCATT_FOLLOW;//npc joins you
    int dist = 0;
    point site = overmap_buffer.find_closest(g->om_global_location(), "bank", dist, false);
    if (site == overmap::invalid_point) {
        site = overmap_buffer.find_closest(g->om_global_location(), "office_tower_1", dist, false);
    }
    miss->target = site;
    overmap_buffer.reveal(site, 2, g->levz);

 tinymap compmap(&(g->traps));
 // Get overmap, crop site coords to be valid inside that overmap
 overmap &om = overmap_buffer.get_om_global(site.x, site.y);
 compmap.load(site.x * 2, site.y * 2, g->levz, false, &om);
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
compmap.save(&om, int(g->turn), site.x * 2, site.y * 2, g->levz);
}

void mission_start::reveal_lab_black_box(mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item(itypes["black_box"], 0) );
  g->add_msg(_("%s gave you back the black box."), dev->name.c_str());
 }
    target_om_ter("lab", 3, miss, false);
}

void mission_start::open_sarcophagus(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;
 if (p != NULL) {
  g->u.i_add( item(itypes["sarcophagus_access_code"], 0) );
  g->add_msg(_("%s gave you sarcophagus access code."), p->name.c_str());
 }
    target_om_ter("haz_sar", 3, miss, false);
}

void mission_start::reveal_hospital(mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item(itypes["vacutainer"], 0) );
  g->add_msg(_("%s gave you a vacutainer."), dev->name.c_str());
 }
    target_om_ter("hospital", 3, miss, false);
}

void mission_start::find_safety(mission *miss)
{
 const tripoint place = g->om_global_location();
 for (int radius = 0; radius <= 20; radius++) {
  for (int dist = 0 - radius; dist <= radius; dist++) {
   int offset = rng(0, 3); // Randomizes the direction we check first
   for (int i = 0; i <= 3; i++) { // Which direction?
    tripoint check = place;
    switch ( (offset + i) % 4 ) {
     case 0: check.x += dist; check.y -= radius; break;
     case 1: check.x += dist; check.y += radius; break;
     case 2: check.y += dist; check.x -= radius; break;
     case 3: check.y += dist; check.x += radius; break;
    }
    if (overmap_buffer.is_safe(check)) {
     miss->target = point(check.x, check.y);
     return;
    }
   }
  }
 }
 // Couldn't find safety; so just set the target to far away
 switch ( rng(0, 3) ) {
   case 0: miss->target = point(place.x - 20, place.y - 20); break;
   case 1: miss->target = point(place.x - 20, place.y + 20); break;
   case 2: miss->target = point(place.x + 20, place.y - 20); break;
   case 3: miss->target = point(place.x + 20, place.y + 20); break;
 }
}

void mission_start::point_prison(mission *miss)
{
    target_om_ter("prison_5", 3, miss, false);
}

void mission_start::point_cabin_strange(mission *miss)
{
    target_om_ter("cabin_strange", 3, miss, false);
}

void mission_start::recruit_tracker(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_FOLLOW;//npc joins you

 point site = target_om_ter("cabin", 2, miss, false);
 miss->recruit_class = NC_COWBOY;
 // get  the overmap of the new npc, crop site to valid values inside that overmap
 overmap &om = overmap_buffer.get_om_global(site.x, site.y);

 npc * temp = new npc();
 temp->normalize();
 temp->randomize(NC_COWBOY);
 // NPCs spawn with submap coordinates, site is in overmap terrain coords
 temp->spawn_at(&om, site.x * 2, site.y * 2, g->levz);
 temp->posx = 11;
 temp->posy = 11;
 temp->attitude = NPCATT_TALK;
 temp->mission = NPC_MISSION_SHOPKEEP;
 temp->personality.aggression -= 1;
 temp->op_of_u.owed = 10; int mission_index = g->reserve_mission(MISSION_JOIN_TRACKER, temp->getID());
 if (mission_index != -1)
    temp->chatbin.missions.push_back(mission_index);
}

void mission_start::place_book( mission *)
{
}
