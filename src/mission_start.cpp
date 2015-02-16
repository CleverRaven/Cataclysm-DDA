#include "mission.h"
#include "game.h"
#include "name.h"
#include <sstream>
#include "omdata.h"
#include "overmapbuffer.h"
#include "messages.h"
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

point target_om_ter_random(const std::string &omter, int reveal_rad, mission *miss, bool must_see)
{
    int dist = 0;
    std::vector<point> places = overmap_buffer.find_all(
        g->om_global_location(), omter, dist, must_see);
    if (places.size() == 0){
        debugmsg("Couldn't find %s", omter.c_str());
        return point();
    }
    std::vector<point> places_om;
    for (auto &i : places) {
        if (g->cur_om == overmap_buffer.get_existing_om_global(i))
            places_om.push_back(i);
    }
    const point place = places_om[rng(0,places_om.size()-1)];
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
 p->add_effect("infection", 1, num_bp, 1, true);
    // make sure they don't have any antibiotics
    p->remove_items_with( []( const item & it ) {
        return it.typeId() == "antibiotics";
    } );
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
 g->u.i_add( item("dog_whistle", 0) );
 add_msg(_("%s gave you a dog whistle."), dev->name.c_str());

 miss->target = house;
 overmap_buffer.reveal(house, 6, g->levz);

 tinymap doghouse;
 doghouse.load_abs(house.x * 2, house.y * 2, g->levz, false);
 doghouse.add_spawn("mon_dog", 1, SEEX, SEEY, true, -1, miss->uid);
 doghouse.save();
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

 tinymap zomhouse;
 zomhouse.load_abs(house.x * 2, house.y * 2, g->levz, false);
 zomhouse.add_spawn("mon_zombie", 1, SEEX, SEEY, false, -1, miss->uid, Name::get(nameIsFemaleName | nameIsGivenName));
 zomhouse.save();
}

void mission_start::place_zombie_bay(mission *miss)
{
 point site = target_om_ter_random("evac_center_9", 1, miss, false);
 tinymap bay;
 bay.load_abs(site.x * 2, site.y * 2, g->levz, false);
 bay.add_spawn("mon_zombie_electric", 1, SEEX, SEEY, false, -1, miss->uid, "Sean McLaughlin");
 bay.save();
}

void mission_start::place_caravan_ambush(mission *miss)
{
 point site = target_om_ter_random("field", 1, miss, false);
 tinymap bay;
 bay.load_abs(site.x * 2, site.y * 2, g->levz, false);
 bay.add_vehicle("cube_van", SEEX, SEEY, 0);
 bay.add_vehicle("quad_bike", SEEX+6, SEEY-5, 270, 500, -1, true);
 bay.add_vehicle("motorcycle", SEEX-2, SEEY-9, 315, 500, -1, true);
 bay.add_vehicle("motorcycle", SEEX-5, SEEY-5, 90, 500, -1, true);
 bay.draw_square_ter(t_grass, SEEX-6, SEEY-9, SEEX+6, SEEY+3);
 bay.draw_square_ter(t_dirt, SEEX-4, SEEY-7, SEEX+3, SEEY+1);
 bay.furn_set(SEEX, SEEY-4, f_ash);
 bay.spawn_item(SEEX-1, SEEY-3, "rock");
 bay.spawn_item(SEEX, SEEY-3, "rock");
 bay.spawn_item(SEEX+1, SEEY-3, "rock");
 bay.spawn_item(SEEX-1, SEEY-4, "rock");
 bay.spawn_item(SEEX+1, SEEY-4, "rock");
 bay.spawn_item(SEEX-1, SEEY-5, "rock");
 bay.spawn_item(SEEX, SEEY-5, "rock");
 bay.spawn_item(SEEX+1, SEEY-5, "rock");
 bay.trap_set(SEEX+3, SEEY-5, tr_rollmat);
 bay.trap_set(SEEX, SEEY-7, tr_rollmat);
 bay.trap_set(SEEX-3, SEEY-4, tr_fur_rollmat);
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "can_beans");
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "beer");
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "beer");
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "bottle_glass");
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "bottle_glass");
 bay.spawn_item(SEEX+rng(-6,6), SEEY+rng(-9,3), "heroin");
 bay.place_items("dresser", 75, SEEX-3, SEEY, SEEX-2, SEEY+2, true, 0 );
 bay.place_items("softdrugs", 50, SEEX-3, SEEY, SEEX-2, SEEY+2, true, 0 );
 bay.place_items("camping", 75, SEEX-3, SEEY, SEEX-2, SEEY+2, true, 0 );
 bay.spawn_item(SEEX+1, SEEY+4, "9mm_casing",1,1,0,0,true);
 bay.spawn_item(SEEX+rng(-2,3), SEEY+rng(3,6), "9mm_casing",1,1,0,0,true);
 bay.spawn_item(SEEX+rng(-2,3), SEEY+rng(3,6), "9mm_casing",1,1,0,0,true);
 bay.spawn_item(SEEX+rng(-2,3), SEEY+rng(3,6), "9mm_casing",1,1,0,0,true);
 bay.add_corpse(SEEX+1, SEEY+7);
 bay.add_corpse(SEEX, SEEY+8);
 bay.add_field(SEEX, SEEY+7,fd_blood,1);
 bay.add_field(SEEX+2, SEEY+7,fd_blood,1);
 bay.add_field(SEEX-1, SEEY+8,fd_blood,1);
 bay.add_field(SEEX+1, SEEY+8,fd_blood,1);
 bay.add_field(SEEX+2, SEEY+8,fd_blood,1);
 bay.add_field(SEEX+1, SEEY+9,fd_blood,1);
 bay.add_field(SEEX, SEEY+9,fd_blood,1);
 bay.place_npc(SEEX+3,SEEY-5, "bandit");
 bay.place_npc(SEEX, SEEY-7, "thug");
 miss->target_npc_id = bay.place_npc(SEEX-3, SEEY-4, "bandit");
 bay.save();
}

void mission_start::place_bandit_cabin(mission *miss)
{
 point site = target_om_ter_random("bandit_cabin", 1, miss, false);
 tinymap cabin;
 cabin.load_abs(site.x * 2, site.y * 2, g->levz, false);
 cabin.trap_set(SEEX-5, SEEY-6, tr_landmine_buried);
 cabin.trap_set(SEEX-7, SEEY-7, tr_landmine_buried);
 cabin.trap_set(SEEX-4, SEEY-7, tr_landmine_buried);
 cabin.trap_set(SEEX-12, SEEY-1, tr_landmine_buried);
 miss->target_npc_id = cabin.place_npc(SEEX, SEEY, "bandit");
 cabin.save();
}

void mission_start::place_informant(mission *miss)
{
 point site = target_om_ter_random("evac_center_19", 1, miss, false);
 tinymap bay;
 bay.load_abs(site.x * 2, site.y * 2, g->levz, false);
 miss->target_npc_id = bay.place_npc(SEEX, SEEY, "evac_guard3");
 bay.save();

 site = target_om_ter_random("evac_center_7", 1, miss, false);
 tinymap bay2;
 bay2.load_abs(site.x * 2, site.y * 2, g->levz, false);
 bay2.place_npc(SEEX+rng(-3,3), SEEY+rng(-3,3), "scavenger_hunter");
 bay2.save();
 site = target_om_ter_random("evac_center_17", 1, miss, false);
}

void mission_start::place_grabber(mission *miss)
{
 point site = target_om_ter_random("field", 5, miss, false);
 tinymap there;
 there.load_abs(site.x * 2, site.y * 2, g->levz, false);
 there.add_spawn("mon_graboid", 1, SEEX+rng(-3,3), SEEY+rng(-3,3));
 there.add_spawn("mon_graboid", 1, SEEX, SEEY, false, -1, miss->uid, "Little Guy");
 there.save();
}

void mission_start::place_bandit_camp(mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 g->u.i_add( item("ruger_redhawk", 0, false) );
 g->u.i_add( item("44magnum", 0, false) );
 g->u.i_add( item("holster", 0, false) );
 g->u.i_add( item("badge_marshal", 0, false) );
 add_msg(m_good, _("%s has instated you as a marshal!"), p->name.c_str());
 // Ideally this would happen at the end of the mission
 // (you're told that they entered your image into the databases, etc)
 // but better to get it working.
 g->u.toggle_mutation("PROF_FED");

 point site = target_om_ter_random("bandit_camp_1", 1, miss, false);
 tinymap bay1;
 bay1.load_abs(site.x * 2, site.y * 2, g->levz, false);
 miss->target_npc_id = bay1.place_npc(SEEX+5, SEEY-3, "bandit");
 bay1.save();
}

void mission_start::place_jabberwock(mission *miss)
{
    point site = target_om_ter("forest_thick", 6, miss, false);
 tinymap grove;
 grove.load_abs(site.x * 2, site.y * 2, g->levz, false);
 grove.add_spawn("mon_jabberwock", 1, SEEX, SEEY, false, -1, miss->uid, "NONE");
 grove.save();
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
 tinymap tile;
 tile.load_abs(site.x * 2, site.y * 2, g->levz, false);
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
 tile.save();
}

void mission_start::place_npc_software(mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev == NULL) {
  debugmsg("Couldn't find NPC! %d", miss->npc_id);
  return;
 }
 g->u.i_add( item("usb_drive", 0) );
 add_msg(_("%s gave you a USB drive."), dev->name.c_str());

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

 tinymap compmap;
 compmap.load_abs(place.x * 2, place.y * 2, g->levz, false);
 point comppoint;

    oter_id oter = g->cur_om->ter(place.x, place.y, 0);
    if( is_ot_type("house", oter) || is_ot_type("s_pharm", oter) || oter == "" ) {
        std::vector<point> valid;
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (compmap.ter(x, y) == t_floor && compmap.furn(x, y) == f_null) {
                    bool okay = false;
                    int wall = 0;
                    for (int x2 = x - 1; x2 <= x + 1 && !okay; x2++) {
                        for (int y2 = y - 1; y2 <= y + 1 && !okay; y2++) {
                            if (compmap.furn(x2, y2) == f_bed || compmap.furn(x2, y2) == f_dresser) {
                                okay = true;
                                valid.push_back( point(x, y) );
                            }
                            if ( compmap.has_flag_ter("WALL", x2, y2) ) {
                                wall++;
                            }
                        }
                    }
                    if ( wall == 5 ) {
                        if ( compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, NORTH ) &&
                             compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, SOUTH ) &&
                             compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, WEST ) &&
                             compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, EAST ) ) {
                            valid.push_back( point(x, y) );
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
    }

 compmap.ter_set(comppoint.x, comppoint.y, t_console);
 computer *tmpcomp = compmap.add_computer(comppoint.x, comppoint.y, string_format(_("%s's Terminal"), dev->name.c_str()), 0);
 tmpcomp->mission_id = miss->uid;
 tmpcomp->add_option(_("Download Software"), COMPACT_DOWNLOAD_SOFTWARE, 0);
 compmap.save();
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
 tinymap compmap;
 compmap.load_abs(place.x * 2, place.y * 2, g->levz, false);
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
 compmap.save();
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

 tinymap compmap;
 compmap.load_abs(site.x * 2, site.y * 2, g->levz, false);
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
compmap.save();
}

void mission_start::reveal_lab_black_box(mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item("black_box", 0) );
  add_msg(_("%s gave you back the black box."), dev->name.c_str());
 }
    target_om_ter("lab", 3, miss, false);
}

void mission_start::open_sarcophagus(mission *miss)
{
    npc *p = g->find_npc(miss->npc_id);
    if (p != NULL) {
        p->attitude = NPCATT_FOLLOW;
        g->u.i_add( item("sarcophagus_access_code", 0) );
        add_msg(m_good, _("%s gave you sarcophagus access code."), p->name.c_str());
    } else {
        DebugLog( D_ERROR, DC_ALL ) << "mission_start: open_sarcophagus() <= Can't find NPC";
    }
    target_om_ter("haz_sar", 3, miss, false);
}

void mission_start::reveal_hospital(mission *miss)
{
 npc* dev = g->find_npc(miss->npc_id);
 if (dev != NULL) {
  g->u.i_add( item("vacutainer", 0) );
  add_msg(_("%s gave you a vacutainer."), dev->name.c_str());
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

 npc * temp = new npc();
 temp->normalize();
 temp->randomize(NC_COWBOY);
 // NPCs spawn with submap coordinates, site is in overmap terrain coords
 temp->spawn_at( site.x * 2, site.y * 2, g->levz );
 temp->setx( 11 );
 temp->sety( 11 );
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
