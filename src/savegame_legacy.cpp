#include "game.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "options.h"
#include "auto_pickup.h"
#include "mapbuffer.h"
#include "debug.h"
#include "map.h"
#include "output.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "translations.h"
#include "debug.h"
#include "weather.h"
#include "monstergenerator.h"
// for legacy classdata loaders
#include "inventory.h"
#include "item.h"
#include "monster.h"
#include "npc.h"
#include "overmap.h"
#include "player.h"
#include "pldata.h"
#include "profession.h"
#include "skill.h"
#include "vehicle.h"
#include "filesystem.h"

#include "mission.h"
#include "faction.h"
#include "savegame.h"

#if !defined(_MSC_VER)
#include <unistd.h>
#endif

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>

#define ARRAY_SIZE(array) ( sizeof( array ) / sizeof( array[0] ) )
/*
 * Properly reuse a stringstream object for line by line parsing
 */
inline std::stringstream & stream_line(std::ifstream & f, std::stringstream & s, std::string & buf) {
    s.clear();
    s.str("");
    getline(f, buf);
    s.str(buf);
    return s;
}

/*
 * Convenience macro for the above
 */
#define parseline() stream_line(fin,linein,linebuf)

/*
 * Parse an open .sav file in an old, now broken format to preserve a game across versions
 */
bool game::unserialize_legacy(std::ifstream & fin) {

   switch (savegame_loading_version) {
       case 10:
       case 9: {
       // Format version 9. After radical compatibility breaking changes, raise savegame_version, cut below and add to
       // unserialize_legacy in savegame_legacy.cpp
            std::string linebuf;
            std::stringstream linein;
            JsonIn jsin(parseline());
            JsonObject pdata = jsin.get_object();

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tmpinv;
            pdata.read("turn", tmpturn);
            pdata.read("last_target", tmptar);
            pdata.read("run_mode", tmprun);
            pdata.read("mostseen", mostseen);
            pdata.read("nextinv", tmpinv);
            pdata.read("next_npc_id", next_npc_id);
            pdata.read("next_faction_id", next_faction_id);
            pdata.read("next_mission_id", next_mission_id);
            pdata.read("nextspawn", tmpspawn);

            getline(fin, linebuf); // Does weather need to be loaded in this order? Probably not,
            load_legacy_future_weather(linebuf); // but better safe than jackie chan expressions later

            pdata.read("levx",levx);
            pdata.read("levy",levy);
            pdata.read("levz",levz);
            pdata.read("om_x",comx);
            pdata.read("om_y",comy);

            calendar::turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(comx, comy);
            m.load(levx, levy, levz, true, cur_om);

            safe_mode = static_cast<safe_mode_type>( tmprun );
            if( OPTIONS["SAFEMODE"] && safe_mode == SAFE_MODE_OFF ) {
                safe_mode = SAFE_MODE_ON;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            last_target = tmptar;

            safemodeveh = OPTIONS["SAFEMODEVEH"];

            // Next, the scent map.
            parseline();

            for( auto &elem : grscent ) {
                for( auto &elem_j : elem ) {
                    linein >> elem_j;
                }
            }

            // Now the number of monsters...
            int nummon;
            parseline() >> nummon;

            // ... and the data on each one.
            std::string data;
            clear_zombies();
            monster montmp;
            for (int i = 0; i < nummon; i++ )
            {
                getline(fin, data);
                montmp.load_info(data);
                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            if ( linein.peek() == '{' ) {
                try {
                    JsonIn kjin(linein);
                    kjin.read(kills);
                } catch (std::string jsonerr) {
                    debugmsg("Bad killcount json\n%s", jsonerr.c_str() );
                }
            } else {
                for (size_t kk = 0; kk < ARRAY_SIZE(legacy_mon_id) && !linein.eof(); kk++) {
                        // load->int->str->int (possibly shifted)
                        linein >> kills[legacy_mon_id[kk]];
                }
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(data);
            u.load_memorial_file( fin );
            // end .sav version 9

            return true;
       } break;

       case 7:
       case 5:
       case 4: {
   // Format version 4-?: Interim format. Still resembles a hairball, but it's at least a multi-line hairball;
   // Data is segmented for readabilty, stability, and gradual conversion into something closer to sanity.
            std::string linebuf;
            std::stringstream linein;

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tempinv;

            // tempinv is a no-longer used field, so is discarded.
            parseline() >> tmpturn >> tmptar >> tmprun >> mostseen >> tempinv >> next_npc_id >>
                next_faction_id >> next_mission_id >> tmpspawn;

            getline(fin, linebuf);
            load_legacy_future_weather(linebuf);

            parseline() >> levx >> levy >> levz >> comx >> comy;

            calendar::turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(comx, comy);
            m.load(levx, levy, levz, true, cur_om);

            safe_mode = static_cast<safe_mode_type>( tmprun );
            if( OPTIONS["SAFEMODE"] && safe_mode == SAFE_MODE_OFF ) {
                safe_mode = SAFE_MODE_ON;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            safemodeveh = OPTIONS["SAFEMODEVEH"];
            last_target = tmptar;

            // Next, the scent map.
            parseline();

            for( auto &elem : grscent ) {
                for( auto &elem_j : elem ) {
                    linein >> elem_j;
                }
            }
            // Now the number of monsters...
            int nummon;
            parseline() >> nummon;

            // ... and the data on each one.
            std::string data;
            clear_zombies();
            monster montmp;
            int num_items;
            for (int i = 0; i < nummon; i++)
            {
                getline(fin, data);
                montmp.load_info(data);

                fin >> num_items;
                // Chomp the endline after number of items.
                getline( fin, data );
                for (int j = 0; j < num_items; j++) {
                    getline( fin, data );
                    montmp.inv.push_back( item( data ) );
                }

                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            for (size_t kk = 0; kk < ARRAY_SIZE(legacy_mon_id) && !linein.eof(); kk++) {
                    // load->int->str->int (possibly shifted)
                    linein >> kills[legacy_mon_id[kk]];
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(data);
            u.load_memorial_file( fin );

            // And the player's inventory...
            u.inv.load_invlet_cache( fin );

            char item_place;
            std::string itemdata;
            // We need a temporary vector of items.  Otherwise, when we encounter an item
            // which is contained in another item, the auto-sort/stacking behavior of the
            // player's inventory may cause the contained item to be misplaced.
            std::list<item> tmpinv;
            while (!fin.eof()) {
                fin >> item_place;
                if (!fin.eof()) {
                    getline(fin, itemdata);
                    if ( item_place == 'I' || item_place == 'C' || item_place == 'W' ||
                         item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
                        item tmpitem(itemdata);
                        if (item_place == 'I') {
                            tmpinv.push_back(tmpitem);
                        } else if (item_place == 'C') {
                            tmpinv.back().contents.push_back(tmpitem);
                        } else if (item_place == 'W') {
                            u.worn.push_back(tmpitem);
                        } else if (item_place == 'S') {
                            u.worn.back().contents.push_back(tmpitem);
                        } else if (item_place == 'w') {
                            u.weapon = tmpitem;
                        } else if (item_place == 'c') {
                            u.weapon.contents.push_back(tmpitem);
                        }
                    }
                }
            }
            // Now dump tmpinv into the player's inventory
            u.inv.add_stack(tmpinv);
            return true;
        ////////
       } break;
       case 3: {

            std::string linebuf;
            std::stringstream linein;

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tempinv;

            // tempenv gets the value of the no-longer-used nextinv variable, so we discard it.
            parseline() >> tmpturn >> tmptar >> tmprun >> mostseen >> tempinv >> next_npc_id >>
                next_faction_id >> next_mission_id >> tmpspawn;

            getline(fin, linebuf);
            load_legacy_future_weather(linebuf);

            parseline() >> levx >> levy >> levz >> comx >> comy;

            calendar::turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(comx, comy);
            m.load(levx, levy, levz, true, cur_om);

            safe_mode = static_cast<safe_mode_type>( tmprun );
            if( OPTIONS["SAFEMODE"] && safe_mode == SAFE_MODE_OFF ) {
                safe_mode = SAFE_MODE_ON;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            safemodeveh = OPTIONS["SAFEMODEVEH"];
            last_target = tmptar;

            // Next, the scent map.
            parseline();

            for( auto &elem : grscent ) {
                for( auto &elem_j : elem ) {
                    linein >> elem_j;
                }
            }
            // Now the number of monsters...
            int nummon;
            parseline() >> nummon;

            // ... and the data on each one.
            std::string data;
            clear_zombies();
            monster montmp;
            int num_items;
            for (int i = 0; i < nummon; i++)
            {
                getline(fin, data);
                montmp.load_info(data);

                fin >> num_items;
                // Chomp the endline after number of items.
                getline( fin, data );
                for (int j = 0; j < num_items; j++) {
                    getline( fin, data );
                    montmp.inv.push_back( item( data ) );
                }

                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            for (size_t kk = 0; kk < ARRAY_SIZE(legacy_mon_id) && !linein.eof(); kk++) {
                    // load->int->str->int (possibly shifted)
                    linein >> kills[legacy_mon_id[kk]];
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(data);
            u.load_memorial_file( fin );

            // And the player's inventory...
            u.inv.load_invlet_cache( fin );

            char item_place;
            std::string itemdata;
            // We need a temporary vector of items.  Otherwise, when we encounter an item
            // which is contained in another item, the auto-sort/stacking behavior of the
            // player's inventory may cause the contained item to be misplaced.
            std::list<item> tmpinv;
            while (!fin.eof()) {
                fin >> item_place;
                if (!fin.eof()) {
                    getline(fin, itemdata);
                    if ( item_place == 'I' || item_place == 'C' || item_place == 'W' ||
                         item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
                        item tmpitem(itemdata);
                        if (item_place == 'I') {
                            tmpinv.push_back(tmpitem);
                        } else if (item_place == 'C') {
                            tmpinv.back().contents.push_back(tmpitem);
                        } else if (item_place == 'W') {
                            u.worn.push_back(tmpitem);
                        } else if (item_place == 'S') {
                            u.worn.back().contents.push_back(tmpitem);
                        } else if (item_place == 'w') {
                            u.weapon = tmpitem;
                        } else if (item_place == 'c') {
                            u.weapon.contents.push_back(tmpitem);
                        }
                    }
                }
            }
            // Now dump tmpinv into the player's inventory
            u.inv.add_stack(tmpinv);
            return true;
        ////////
        } break;

        case 0: {
         // todo: make savegame_legacy.cpp?
/*
original 'structure', which globs game/weather/location & killcount/player data onto the same lines.
*/
         int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tempinv;
         // tempinv gets the legacy nextinv value and is discarded.
         fin >> tmpturn >> tmptar >> tmprun >> mostseen >> tempinv >> next_npc_id >>
             next_faction_id >> next_mission_id >> tmpspawn;

         load_legacy_future_weather(fin);

         fin >> levx >> levy >> levz >> comx >> comy;

         calendar::turn = tmpturn;
         nextspawn = tmpspawn;

         cur_om = &overmap_buffer.get(comx, comy);
         m.load(levx, levy, levz, true, cur_om);

         safe_mode = static_cast<safe_mode_type>( tmprun );
         if( OPTIONS["SAFEMODE"] && safe_mode == SAFE_MODE_OFF ) {
            safe_mode = SAFE_MODE_ON;
         }
         autosafemode = OPTIONS["AUTOSAFEMODE"];
         safemodeveh = OPTIONS["SAFEMODEVEH"];
         last_target = tmptar;

        // Next, the scent map.
         for( auto &elem : grscent ) {
             for( auto &elem_j : elem )
                 fin >> elem_j;
         }
        // Now the number of monsters...
         int nummon;
         fin >> nummon;
        // ... and the data on each one.
         std::string data;
         clear_zombies();
         monster montmp;
         char junk;
         int num_items;
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         for (int i = 0; i < nummon; i++) {
          getline(fin, data);
          montmp.load_info(data);

          fin >> num_items;
          // Chomp the endline after number of items.
          getline( fin, data );
          for (int j = 0; j < num_items; j++) {
              getline( fin, data );
              montmp.inv.push_back( item( data ) );
          }

          add_zombie(montmp);
         }
        // And the kill counts;
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         for( auto &elem : legacy_mon_id )
             fin >> kills[elem];
        // Finally, the data on the player.
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         getline(fin, data);
         u.load_info(data);
        // And the player's inventory...
         u.inv.load_invlet_cache( fin );

         char item_place;
         std::string itemdata;
        // We need a temporary vector of items.  Otherwise, when we encounter an item
        // which is contained in another item, the auto-sort/stacking behavior of the
        // player's inventory may cause the contained item to be misplaced.
         std::list<item> tmpinv;
         while (!fin.eof()) {
          fin >> item_place;
          if (!fin.eof()) {
           getline(fin, itemdata);
           if ( item_place == 'I' || item_place == 'C' || item_place == 'W' || item_place == 'S' || item_place == 'w' || item_place == 'c' ) {
               item tmpitem(itemdata);
               if (item_place == 'I') {
                   tmpinv.push_back(tmpitem);
               } else if (item_place == 'C') {
                   tmpinv.back().contents.push_back(tmpitem);
               } else if (item_place == 'W') {
                   u.worn.push_back(tmpitem);
               } else if (item_place == 'S') {
                   u.worn.back().contents.push_back(tmpitem);
               } else if (item_place == 'w') {
                   u.weapon = tmpitem;
               } else if (item_place == 'c') {
                   u.weapon.contents.push_back(tmpitem);
               }
           }
          }
         }
        // Now dump tmpinv into the player's inventory
         u.inv.add_stack(tmpinv);
         return true;
        ///////////////////////////////////// legacy save
        } break;
    }
    return false;
}

void game::load_legacy_future_weather(std::string data)
{
    weather_log.clear();
    std::istringstream fin;
    fin.str(data);
    load_legacy_future_weather(fin);

}

void game::load_legacy_future_weather(std::istream &fin)
{
    int tmpnextweather, tmpweather, tmptemp, num_segments;
    weather_segment new_segment;

    fin >> num_segments >> tmpnextweather >> tmpweather >> tmptemp;

    weather = weather_type(tmpweather);
    temperature = tmptemp;
    nextweather = tmpnextweather;

    for( int i = 0; i < num_segments - 1; ++i)
    {
        fin >> tmpnextweather >> tmpweather >> tmptemp;
        new_segment.weather = weather_type(tmpweather);
        new_segment.temperature = tmptemp;
        new_segment.deadline = tmpnextweather;
        weather_log[ tmpnextweather ] = new_segment;
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete overmap. These can linger unless the player moves around to all explored areas.
 */

const size_t num_ter_types = 304;
const char* oter_legacy[num_ter_types] = {
    "",
    "crater",
    "field",
    "forest",
    "forest_thick",
    "forest_water",
    "hiway_ns",
    "hiway_ew",
    "road_null",
    "road_ns",
    "road_ew",
    "road_ne",
    "road_es",
    "road_sw",
    "road_wn",
    "road_nes",
    "road_new",
    "road_nsw",
    "road_esw",
    "road_nesw",
    "road_nesw_manhole",
    "bridge_ns",
    "bridge_ew",
    "river_center",
    "river_c_not_ne",
    "river_c_not_nw",
    "river_c_not_se",
    "river_c_not_sw",
    "river_north",
    "river_east",
    "river_south",
    "river_west",
    "river_ne",
    "river_se",
    "river_sw",
    "river_nw",
    "house_north",
    "house_east",
    "house_south",
    "house_west",
    "house_base_north",
    "house_base_east",
    "house_base_south",
    "house_base_west",
    "s_lot",
    "park",
    "pool",
    "s_gas_north",
    "s_gas_east",
    "s_gas_south",
    "s_gas_west",
    "s_pharm_north",
    "s_pharm_east",
    "s_pharm_south",
    "s_pharm_west",
    "office_doctor_north",
    "office_doctor_east",
    "office_doctor_south",
    "office_doctor_west",
    "office_cubical_north",
    "office_cubical_east",
    "office_cubical_south",
    "office_cubical_west",
    "apartments_con_tower_1_entrance","apartments_con_tower_1",
    "apartments_mod_tower_1_entrance","apartments_mod_tower_1",
    "office_tower_1_entrance",
    "office_tower_1",
    "office_tower_b_entrance",
    "office_tower_b",
    "church_north",
    "church_east",
    "church_south",
    "church_west",
    "cathedral_1_entrance",
    "cathedral_1",
    "cathedral_b_entrance",
    "cathedral_b",
    "s_grocery_north",
    "s_grocery_east",
    "s_grocery_south",
    "s_grocery_west",
    "s_hardware_north",
    "s_hardware_east",
    "s_hardware_south",
    "s_hardware_west",
    "s_electronics_north",
    "s_electronics_east",
    "s_electronics_south",
    "s_electronics_west",
    "s_sports_north",
    "s_sports_east",
    "s_sports_south",
    "s_sports_west",
    "s_liquor_north",
    "s_liquor_east",
    "s_liquor_south",
    "s_liquor_west",
    "s_gun_north",
    "s_gun_east",
    "s_gun_south",
    "s_gun_west",
    "s_clothes_north",
    "s_clothes_east",
    "s_clothes_south",
    "s_clothes_west",
    "s_library_north",
    "s_library_east",
    "s_library_south",
    "s_library_west",
    "s_restaurant_north",
    "s_restaurant_east",
    "s_restaurant_south",
    "s_restaurant_west",
    "s_restaurant_fast_north",
    "s_restaurant_fast_east",
    "s_restaurant_fast_south",
    "s_restaurant_fast_west",
    "s_restaurant_coffee_north",
    "s_restaurant_coffee_east",
    "s_restaurant_coffee_south",
    "s_restaurant_coffee_west",
    "sub_station_north",
    "sub_station_east",
    "sub_station_south",
    "sub_station_west",
    "s_garage_north",
    "s_garage_east",
    "s_garage_south",
    "s_garage_west",
    "cabin_strange",
    "cabin_strange_b",
    "cabin",
    "dirtlot",
    "farm",
    "farm_field",
    "police_north",
    "police_east",
    "police_south",
    "police_west",
    "bank_north",
    "bank_east",
    "bank_south",
    "bank_west",
    "bar_north",
    "bar_east",
    "bar_south",
    "bar_west",
    "pawn_north",
    "pawn_east",
    "pawn_south",
    "pawn_west",
    "mil_surplus_north",
    "mil_surplus_east",
    "mil_surplus_south",
    "mil_surplus_west",
    "furniture_north",
    "furniture_east",
    "furniture_south",
    "furniture_west",
    "abstorefront_north",
    "abstorefront_east",
    "abstorefront_south",
    "abstorefront_west",
    "megastore_entrance",
    "megastore",
    "hospital_entrance",
    "hospital",
    "public_works_entrance",
    "public_works",
    "school_1",
    "school_2",
    "school_3",
    "school_4",
    "school_5",
    "school_6",
    "school_7",
    "school_8",
    "school_9",
    "prison_1",
    "prison_2",
    "prison_3",
    "prison_4",
    "prison_5",
    "prison_6",
    "prison_7",
    "prison_8",
    "prison_9",
    "prison_b",
    "prison_b_entrance",
    "hotel_tower_1_1",
    "hotel_tower_1_2",
    "hotel_tower_1_3",
    "hotel_tower_1_4",
    "hotel_tower_1_5",
    "hotel_tower_1_6",
    "hotel_tower_1_7",
    "hotel_tower_1_8",
    "hotel_tower_1_9","hotel_tower_b_1",
    "hotel_tower_b_2",
    "hotel_tower_b_3",
    "mansion_entrance",
    "mansion",
    "fema_entrance",
    "fema",
    "station_radio_north",
    "station_radio_east",
    "station_radio_south",
    "station_radio_west",
    "shelter",
    "shelter_under",
    "lmoe",
    "lmoe_under",
    "lab",
    "lab_stairs",
    "lab_core",
    "lab_finale",
    "ice_lab",
    "ice_lab_stairs",
    "ice_lab_core",
    "ice_lab_finale",
    "nuke_plant_entrance",
    "nuke_plant",
    "bunker",
    "outpost",
    "silo",
    "silo_finale",
    "temple",
    "temple_stairs",
    "temple_core",
    "temple_finale",
    "sewage_treatment",
    "sewage_treatment_hub",
    "sewage_treatment_under",
    "mine_entrance",
    "mine_shaft",
    "mine",
    "mine_down",
    "mine_finale",
    "spiral_hub",
    "spiral",
    "radio_tower",
    "toxic_dump",
    "haz_sar_entrance",
    "haz_sar",
    "haz_sar_entrance_b1",
    "haz_sar_b1",
    "cave",
    "cave_rat",
    "hive",
    "fungal_bloom",
    "spider_pit",
    "spider_pit_under",
    "anthill",
    "slimepit",
    "slimepit_down",
    "triffid_grove",
    "triffid_roots",
    "triffid_finale",
    "basement",
    "cavern",
    "rock",
    "rift",
    "hellmouth",
    "subway_station",
    "subway_ns",
    "subway_ew",
    "subway_ne",
    "subway_es",
    "subway_sw",
    "subway_wn",
    "subway_nes",
    "subway_new",
    "subway_nsw",
    "subway_esw",
    "subway_nesw",
    "sewer_ns",
    "sewer_ew",
    "sewer_ne",
    "sewer_es",
    "sewer_sw",
    "sewer_wn",
    "sewer_nes",
    "sewer_new",
    "sewer_nsw",
    "sewer_esw",
    "sewer_nesw",
    "ants_ns",
    "ants_ew",
    "ants_ne",
    "ants_es",
    "ants_sw",
    "ants_wn",
    "ants_nes",
    "ants_new",
    "ants_nsw",
    "ants_esw",
    "ants_nesw",
    "ants_food",
    "ants_larvae",
    "ants_queen",
    "tutorial"
};

bool overmap::unserialize_legacy(std::ifstream & fin, std::string const & plrfilename, std::string const & terfilename) {
    switch (savegame_loading_version) {
        case 11:
        case 10:
        case 9:
        case 8:
        case 7:
        case 6:
        case 5:
        case 4:
        case 3:
        case 2:
        case 1:
        case 0: {

            // DEBUG VARS
            int nummg = 0;
            char datatype;
            int cx, cy, cz, cs, cp, cd, cdying;
            std::string cstr;
            city tmp;
            std::list<item> npc_inventory;

            int z = 0; // assumption
            while (fin >> datatype) {
                if (datatype == 'L') { // Load layer data, and switch to layer
                    fin >> z;

                    int tmp_ter;
                    if (z >= 0 && z < OVERMAP_LAYERS) {
                        int count = 0;
                        for( size_t j = 0; j < OMAPY; j++) {
                            for( size_t i = 0; i < OMAPX; i++) {
                                if (count == 0) {
                                    fin >> tmp_ter >> count;
                                    if (tmp_ter < 0 || tmp_ter >= (int)num_ter_types) {
                                        debugmsg("Loaded bad ter!  %s; ter %d", terfilename.c_str(), tmp_ter);
                                    }
                                }
                                count--;
                                layer[z].terrain[i][j] = oter_id(oter_legacy[tmp_ter]);
                                layer[z].visible[i][j] = false;
                            }
                        }
                    } else {
                        debugmsg("Loaded z level out of range (z: %d)", z);
                    }
                } else if (datatype == 'Z') { // Monster group
                    fin >> cstr >> cx >> cy >> cz >> cs >> cp >> cd >> cdying;
                    mongroup mg(cstr, cx, cy, cz, cs, cp);
                    mg.diffuse = cd;
                    mg.dying = cdying;
                    add_mon_group( mg );
                    nummg++;
                } else if (datatype == 't') { // City
                    fin >> cx >> cy >> cs;
                    tmp.x = cx; tmp.y = cy; tmp.s = cs;
                    cities.push_back(tmp);
                } else if (datatype == 'R') { // Road leading out
                    fin >> cx >> cy;
                    tmp.x = cx; tmp.y = cy; tmp.s = 0;
                    roads_out.push_back(tmp);
                } else if (datatype == 'T') { // Radio tower
                    radio_tower tmp;
                    int tmp_type;
                    fin >> tmp.x >> tmp.y >> tmp.strength >> tmp_type;
                    tmp.type = (radio_type)tmp_type;
                    getline(fin, tmp.message); // Chomp endl
                    getline(fin, tmp.message);
                    radios.push_back(tmp);
                } else if (datatype == 'n') { // NPC
                    // When we start loading a new NPC, check to see if we've
                    // accumulated items for assignment to an NPC.
                    if (!npc_inventory.empty() && !npcs.empty()) {
                        npcs.back()->inv.add_stack(npc_inventory);
                        npc_inventory.clear();
                    }
                    std::string npcdata;
                    getline(fin, npcdata);
                    npc * tmp = new npc();
                    tmp->load_info(npcdata);
                    npcs.push_back(tmp);
                } else if (datatype == 'P') {
                    // Chomp the invlet_cache, since the npc doesn't use it.
                    std::string itemdata;
                    getline(fin, itemdata);
                } else if (datatype == 'I' || datatype == 'C' || datatype == 'W' ||
                           datatype == 'w' || datatype == 'c') {
                    std::string itemdata;
                    getline(fin, itemdata);
                    if (npcs.empty()) {
                        debugmsg("Overmap %d:%d:%d tried to load object data, without an NPC!\n%s",
                                 loc.x, loc.y, itemdata.c_str());
                    } else {
                        item tmp(itemdata);
                        npc* last = npcs.back();
                        switch (datatype) {
                        case 'I':
                            npc_inventory.push_back(tmp);
                            break;
                        case 'C':
                            npc_inventory.back().contents.push_back(tmp);
                            break;
                        case 'W':
                            last->worn.push_back(tmp);
                            break;
                        case 'w':
                            last->weapon = tmp;
                            break;
                        case 'c':
                            last->weapon.contents.push_back(tmp);
                            break;
                        }
                    }
                }
            }

            // If we accrued an npc_inventory, assign it now
            if (!npc_inventory.empty() && !npcs.empty()) {
                npcs.back()->inv.add_stack(npc_inventory);
            }

            std::ifstream sfin;
            // Private/per-character data
            sfin.open(plrfilename.c_str());
            if ( fin.peek() == '#' ) { // not handling muilti-version seen cache
                std::string vline;
                getline(fin, vline);
            }
            if (sfin.is_open()) { // Load private seen data
                int z = 0; // assumption
                while (sfin >> datatype) {
                    if (datatype == 'L') {  // Load layer data, and switch to layer
                        sfin >> z;

                        std::string dataline;
                        getline(sfin, dataline); // Chomp endl

                        int count = 0;
                        int vis;
                        if (z >= 0 && z < OVERMAP_LAYERS) {
                            for( size_t j = 0; j < OMAPY; j++) {
                                for( size_t i = 0; i < OMAPX; i++) {
                                    if (count == 0) {
                                        sfin >> vis >> count;
                                    }
                                    count--;
                                    layer[z].visible[i][j] = (vis == 1);
                                }
                            }
                        }
                    } else if (datatype == 'N') { // Load notes
                        om_note tmp;
                        sfin >> tmp.x >> tmp.y >> tmp.num;
                        getline(sfin, tmp.text); // Chomp endl
                        getline(sfin, tmp.text);
                        if (z >= 0 && z < OVERMAP_LAYERS) {
                            layer[z].notes.push_back(tmp);
                        }
                    }
                }
                sfin.close();
            }
            return true;
        } break;

    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete maps.txt.
 */
static bool unserialize_legacy(std::ifstream & fin ) {
    switch (savegame_loading_version) {
        case 9:
        case 7:
        case 6:
        case 5:
        case 4:
        case 3:
        case 0: {

        std::map<tripoint, submap*>::iterator it;
        int itx, ity, t, d, a, num_submaps, num_loaded = 0;
        item it_tmp;
        std::string databuff;
        // While later revisions have an int ter_t association, it is non-static and generated
        // at runtime, wholly dependant on json ordering. The definitive association is a
        // string ID. legacy_ter_id translates the saved ints to a string, and like the
        // current loader, instead of doing string lookups while parsing a potentially huge
        // amount of tiles, we do so beforehand.
        
        // Ditto for furniture
        std::map<int, int> furn_key;
        std::string fstr;
        for(int i=0; i < num_legacy_furn; i++) {
            fstr = legacy_furn_id[i];
            if ( furnmap.find(fstr) == furnmap.end() ) {
                debugmsg("Can't find furniture '%s' (%d)",fstr.c_str(), i );
                furn_key[i] = furnmap["f_null"].loadid;
            } else {
                furn_key[i] = furnmap[fstr].loadid;
            }
        }
        // Process terrain after furniture to allow for furniture changes due to updated terrain
        std::map<int, int> ter_key;
        std::string tstr;
        for(int i=0; i < num_legacy_ter; i++) {
            tstr = legacy_ter_id[i];
            if ( termap.find(tstr) == termap.end() ) {
                if (tstr == "t_rubble") {
                    ter_key[i] = termap["t_dirt"].loadid;
                    furn_key[i] = furnmap["f_rubble"].loadid;
                } else if (tstr == "t_wreckage") {
                    ter_key[i] = termap["t_dirt"].loadid;
                    furn_key[i] = furnmap["f_wreckage"].loadid;
                } else if (tstr == "t_ash") {
                    ter_key[i] = termap["t_dirt"].loadid;
                    furn_key[i] = furnmap["f_ash"].loadid;
                } else {
                    debugmsg("Can't find terrain '%s' (%d)",tstr.c_str(), i );
                    ter_key[i] = termap["t_null"].loadid;
                }
            } else {
                ter_key[i] = termap[tstr].loadid;
            }
        }

        // it's a...
        std::map<int, int> trap_key;
        std::string trstr;
        for (int i = 0; i < num_legacy_trap; i++) {
           trstr = legacy_trap_id[ i ];
           if ( trapmap.find( trstr ) == trapmap.end() ) {
              debugmsg("Can't find trap '%s' (%d)",trstr.c_str(), i );
              trap_key[i] = trapmap["tr_null"];
           } else {
              trap_key[i] = trapmap[trstr];
           }
        }


         fin >> num_submaps;

         while (!fin.eof()) {
          if (num_loaded % 100 == 0)
           popup_nowait(_("Please wait as the map loads [%d/%d]"),
                        num_loaded, num_submaps);
          int locx, locy, locz, turn, temperature;
          std::unique_ptr<submap> sm(new submap());
          fin >> locx >> locy >> locz >> turn >> temperature;
          if(fin.eof()) {
              break;
          }
          sm->turn_last_touched = turn;
          sm->temperature = temperature;
          int turndif = int(calendar::turn) - turn;
          if (turndif < 0)
           turndif = 0;
        // Load terrain
          for( size_t j = 0; j < SEEY; j++) {
           for( size_t i = 0; i < SEEX; i++) {
            int tmpter;
            fin >> tmpter;
            tmpter = ter_key[tmpter];
            sm->ter[i][j] = ter_id(tmpter);
            sm->set_furn(i, j, f_null);
            sm->itm[i][j].clear();
            sm->set_trap(i, j, tr_null);
           }
          }
        // Load irradiation
          int radtmp;
          int count = 0;
          for( size_t j = 0; j < SEEY; j++) {
           for( size_t i = 0; i < SEEX; i++) {
            if (count == 0) {
             fin >> radtmp >> count;
             radtmp -= int(turndif / 100); // Radiation slowly decays
             if (radtmp < 0) {
              radtmp = 0;
             }
            }
            count--;
            sm->set_radiation(i, j, radtmp);
           }
          }
        // Load items and traps and fields and spawn points and vehicles
          std::string string_identifier;
          do {
           fin >> string_identifier; // "----" indicates end of this submap
           t = 0;
           if (string_identifier == "I") {
            fin >> itx >> ity;
            getline(fin, databuff); // Clear out the endline
            getline(fin, databuff);
            it_tmp.load_info(databuff);
            if( it_tmp.is_emissive() ) {
                sm->update_lum_add(it_tmp, itx, ity);
            }
            sm->itm[itx][ity].push_back(it_tmp);
            if( it_tmp.active ) {
                sm->active_items.add( std::prev(sm->itm[itx][ity].end()), point( itx, ity ) );
            }
           } else if (string_identifier == "C") {
            getline(fin, databuff); // Clear out the endline
            getline(fin, databuff);
            it_tmp.load_info(databuff);
            sm->itm[itx][ity].back().put_in(it_tmp);
           } else if (string_identifier == "T") {
            fin >> itx >> ity >> t;
            sm->set_trap(itx, ity, trap_id(t));
           } else if (string_identifier == "f") {
            fin >> itx >> ity >> t;
            sm->set_furn(itx, ity, furn_id(furn_key[t]));
           } else if (string_identifier == "F") {
            fin >> itx >> ity >> t >> d >> a;
            if(!sm->fld[itx][ity].findField(field_id(t)))
             sm->field_count++;
            sm->fld[itx][ity].addField(field_id(t), d, a);
           } else if (string_identifier == "S") {
            char tmpfriend;
            int tmpfac = -1, tmpmis = -1;
            std::string spawnname;
            fin >> t >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
            spawn_point tmp(legacy_mon_id[t], a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                            spawnname);
            sm->spawns.push_back(tmp);
           } else if (string_identifier == "V") {
            vehicle *veh = new vehicle();
            veh->load (fin);
            //veh.smx = gridx;
            //veh.smy = gridy;
            g->m.vehicle_list.insert(veh);
            sm->vehicles.push_back(veh);
           } else if (string_identifier == "c") {
            getline(fin, databuff);
            sm->comp.load_data(databuff);
           } else if (string_identifier == "B") {
            getline(fin, databuff);
            sm->camp.load_data(databuff);
           } else if (string_identifier == "G") {
             std::string s;
            int j;
            int i;
            fin >> j >> i;
            getline(fin,s);
            sm->set_graffiti(j, i, s);
           }
          } while (string_identifier != "----" && !fin.eof());

          MAPBUFFER.add_submap( locx, locy, locz, sm );
          num_loaded++;
         }
         return true;
       } break;
     }
   return false;
}

static int unserialize_keys( std::ifstream &fin, std::map<int, int> &ter_key,
                             std::map<int, int> &furn_key, std::map<int, int> &trap_key)
{
    const int savegame_minver_map = 11;
    if ( fin.peek() == '#' ) {
        std::string vline;
        getline(fin, vline);
        std::string tmphash, tmpver;
        int savedver = -1;
        std::stringstream vliness(vline);
        vliness >> tmphash >> tmpver >> savedver;
        if ( tmpver == "version" && savedver != -1 ) {
            savegame_loading_version = savedver;
        }
    }
    if (savegame_loading_version != savegame_version &&
        savegame_loading_version < savegame_minver_map) {
        // We're version x but this is a save from version y, let's check to see if there's a loader
        if ( unserialize_legacy(fin) == true ) { // loader returned true, we're done.
            return 0;
        } else {
            // no unserialize_legacy for version y, continuing onwards towards possible disaster.
            // Or not?
            popup_nowait(_("Cannot find loader for map save data in old version %d,"
                           " attempting to load as current version %d."),
                         savegame_loading_version, savegame_version);
        }
    }

    std::stringstream jsonbuff;
    std::string databuff;
    int num_submaps = 0;
    getline(fin, databuff);
    jsonbuff.str(databuff);
    JsonIn jsin(jsonbuff);

    ter_key.clear();
    furn_key.clear();
    trap_key.clear();

    jsin.start_object();
    while (!jsin.end_object()) {
        std::string name = jsin.get_member_name();
        if (name == "listsize") {
            num_submaps = jsin.get_int();
        } else if (name == "terrain_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string tstr = jsin.get_string();
                if ( termap.find(tstr) == termap.end() ) {
                    if (tstr == "t_rubble" || tstr == "t_wreckage" || tstr == "t_ash" ||
                          tstr == "t_metal" || tstr == "t_skylight" || tstr == "t_emergency_light" ||
                          tstr == "t_emergency_light_flicker") {
                        ter_key[i] = termap["t_dirt"].loadid;
                    } else {
                        debugmsg("Can't find terrain '%s' (%d)", tstr.c_str(), i);
                    }
                } else {
                    ter_key[i] = termap[tstr].loadid;
                }
                ++i;
            }
        } else if (name == "furniture_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string fstr = jsin.get_string();
                if ( furnmap.find(fstr) == furnmap.end() ) {
                    debugmsg("Can't find furniture '%s' (%d)", fstr.c_str(), i);
                } else {
                    furn_key[i] = furnmap[fstr].loadid;
                }
                ++i;
            }
        } else if (name == "trap_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string trstr = jsin.get_string();
                if ( trapmap.find(trstr) == trapmap.end() ) {
                    debugmsg("Can't find trap '%s' (%d)", trstr.c_str(), i);
                } else {
                    trap_key[i] = trapmap[trstr];
                }
                ++i;
            }
        } else {
            debugmsg("unrecognized mapbuffer json member '%s'", name.c_str());
            jsin.skip_value();
        }
    }

    if (trap_key.empty()) { // old, snip when this moves to legacy
        for (int i = 0; i < num_legacy_trap; i++) {
            std::string trstr = legacy_trap_id[i];
            if ( trapmap.find( trstr ) == trapmap.end() ) {
                debugmsg("Can't find trap '%s' (%d)", trstr.c_str(), i);
                trap_key[i] = trapmap["tr_null"];
            } else {
                trap_key[i] = trapmap[trstr];
            }
        }
    }
    return num_submaps;
}

static int load_keys( std::string worldname, std::map<int, int> &ter_key,
                      std::map<int, int> &furn_key, std::map<int, int> &trap_key )
{
    int num_submaps = 0;
    std::ifstream fin;
    std::stringstream world_map_path;
    world_map_path << world_generator->all_worlds[worldname]->world_path << "/maps";
    std::stringstream map_key_file;
    map_key_file << world_map_path.str() << "/map.key";
    fin.open( map_key_file.str().c_str() );
    if( !fin.is_open() ) {
        // Inform caller that there is no key file.
        return -1;
    }
    num_submaps = unserialize_keys( fin, ter_key, furn_key, trap_key );
    fin.close();
    // We don't need the legacy key file any more.
    unlink( map_key_file.str().c_str() );
    return num_submaps;
}

// Legacy submap chunk loader.
static void unserialize_legacy_submaps( std::ifstream &fin, const int num_submaps,
                                        std::map<int, int> &ter_key,
                                        std::map<int, int> &furn_key,
                                        std::map<int, int> &trap_key )
{
    std::map<tripoint, submap *>::iterator it;
    int num_loaded = 0;
    item it_tmp;
    std::string databuff;
    std::string st;

    while (!fin.eof()) {
        if( num_submaps > 100 && num_loaded % 100 == 0 ) {
            popup_nowait(_("Please wait as the map loads [%d/%d]"),
                         num_loaded, num_submaps);
        }

        int locx, locy, locz, turn, temperature;
        std::unique_ptr<submap> sm(new submap());
        fin >> locx >> locy >> locz >> turn >> temperature;
        if(fin.eof()) {
            break;
        }
        sm->turn_last_touched = turn;
        sm->temperature = temperature;
        int turndif = int(calendar::turn) - turn;
        if (turndif < 0) {
            turndif = 0;
        }

        // Load terrain
        for( size_t j = 0; j < SEEY; j++) {
            for( size_t i = 0; i < SEEX; i++) {
                int tmpter;
                fin >> tmpter;
                tmpter = ter_key[tmpter];
                sm->ter[i][j] = ter_id(tmpter);

                sm->set_furn(i, j, f_null);
                sm->itm[i][j].clear();
                sm->lum[i][j] = 0;
                sm->set_trap(i, j, tr_null);
            }
        }
        // Load irradiation
        int radtmp;
        int count = 0;
        for( size_t j = 0; j < SEEY; j++) {
            for( size_t i = 0; i < SEEX; i++) {
                if (count == 0) {
                    fin >> radtmp >> count;
                    radtmp -= int(turndif / 100); // Radiation slowly decays
                    if (radtmp < 0) {
                        radtmp = 0;
                    }
                }
                count--;
                sm->set_radiation(i, j, radtmp);
            }
        }
        // Load items and traps and fields and spawn points and vehicles
        std::string string_identifier;
        // Parts of the loop seem to rely on these variables NOT being reset.
        // INSANITY!
        int itx = 0;
        int ity = 0;
        int d = 0;
        int a = 0;
        do {
            if(fin.eof()) {
                // file has ended, but the submap-separator string
                // "----" has not been read, something's wrong, skip
                // this probably damaged/invalid submap.
                return;
            }
            fin >> string_identifier; // "----" indicates end of this submap
            int t = 0;

            st = "";
            if (string_identifier == "I") {
                fin >> itx >> ity;
                getline(fin, databuff); // Clear out the endline
                getline(fin, databuff);
                it_tmp.load_info(databuff);
                if (it_tmp.is_emissive()) {
                    sm->update_lum_add(it_tmp, itx, ity);
                }
                sm->itm[itx][ity].push_back(it_tmp);
                if (it_tmp.active) {
                    sm->active_items.add( std::prev(sm->itm[itx][ity].end()), point( itx, ity ) );
                }
            } else if (string_identifier == "C") {
                getline(fin, databuff); // Clear out the endline
                getline(fin, databuff);
                it_tmp.load_info(databuff);
                sm->itm[itx][ity].back().put_in(it_tmp);
            } else if (string_identifier == "T") {
                fin >> itx >> ity >> t;
                sm->set_trap(itx, ity, trap_id(trap_key[t]));
            } else if (string_identifier == "f") {
                fin >> itx >> ity >> t;
                sm->set_furn(itx, ity, furn_id(furn_key[t]));
            } else if (string_identifier == "F") {
                fin >> itx >> ity >> t >> d >> a;
                if(!sm->fld[itx][ity].findField(field_id(t))) {
                    sm->field_count++;
                }
                sm->fld[itx][ity].addField(field_id(t), d, a);
            } else if (string_identifier == "S") {
                char tmpfriend;
                int tmpfac = -1, tmpmis = -1;
                std::string spawnname;
                fin >> st >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
                spawn_point tmp((st), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                                spawnname);
                sm->spawns.push_back(tmp);
            } else if (string_identifier == "V") {
                vehicle *veh = new vehicle();
                veh->load (fin);
                sm->vehicles.push_back(veh);
            } else if (string_identifier == "c") {
                getline(fin, databuff);
                sm->comp.load_data(databuff);
            } else if (string_identifier == "B") {
                getline(fin, databuff);
                sm->camp.load_data(databuff);
            } else if (string_identifier == "G") {
                std::string s;
                int j;
                int i;
                fin >> j >> i;
                getline(fin, s);
                sm->set_graffiti(j, i, s);
            }
        } while (string_identifier != "----");

        MAPBUFFER.add_submap( locx, locy, locz, sm );
        num_loaded++;
    }
}

// Entry point for loading all the map files.
// We laod map files on demand now, so if the map saves are recent this does nothing.
void mapbuffer::load( std::string worldname )
{
    std::ifstream fin;
    std::stringstream worldmap;
    int num_submaps = 0;

    std::map<int, int> ter_key;
    std::map<int, int> furn_key;
    std::map<int, int> trap_key;

    worldmap << world_generator->all_worlds[worldname]->world_path << "/maps.txt";

    // Handle older monolithic map file.
    fin.open( worldmap.str().c_str() );
    if( fin.is_open() ) {
        // If we have a maps.txt, load it, then get rid of it.
        num_submaps = unserialize_keys( fin, ter_key, furn_key, trap_key );
        unserialize_legacy_submaps( fin, num_submaps, ter_key, furn_key, trap_key );
        fin.close();
        // Save the data and unload it at the same time.
        save( true );
        unlink( worldmap.str().c_str() );
        return;
    }

    // If we don't have a monolithic maps.txt, we have either a chunked-up legacy map,
    // or a json based map.
    std::stringstream world_map_path;
    world_map_path << world_generator->all_worlds[worldname]->world_path << "/maps";
    num_submaps = load_keys( worldname, ter_key, furn_key, trap_key );
    if( num_submaps > 0 ) {
        // Chunked up saves need to be fully loaded to convert them to json.
        // Hopefully this is the last time we ever need to do this.
        auto const map_files = get_files_from_path(".map", world_map_path.str(), true, true);
        for( auto &map_file : map_files ) {
            fin.open( map_file.c_str() );
            unserialize_legacy_submaps( fin, num_submaps, ter_key, furn_key, trap_key );
            fin.close();
            // Write out and clear map data as its read.
            save( true );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// old stringstream based class loadgame functions. For saves from < 0.8 to git sep 20 '13

///// player.h
void player::load_legacy(std::stringstream & dump)
{
 int inveh, vctrl;
 itype_id styletmp;
 std::string prof_ident;

 dump >> position.x >> position.y >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> driving_recoil >>
         inveh >> vctrl >> grab_point.x >> grab_point.y >> scent >> moves >>
         underwater >> dodges_left >> blocks_left >> oxygen >> active_mission >>
         focus_pool >> male >> prof_ident >> healthy >> styletmp;

         // Bionic power scale has been changed.
         max_power_level *= 25;
         power_level *= 25;

 if (profession::exists(prof_ident)) {
  prof = profession::prof(prof_ident);
 } else {
  prof = profession::generic();
  debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
 }

 activity.load_legacy(dump);
 player_activity temp_backlog;
 temp_backlog.load_legacy(dump);
 backlog.push_front( temp_backlog );

 in_vehicle = inveh != 0;
 controlling_vehicle = vctrl != 0;
 style_selected = styletmp;

 std::string sTemp = "";
 for( size_t i = 0; i < mutation_data.size(); i++ ) {
    dump >> sTemp;
    if (sTemp == "TRAITS_END") {
        break;
    } else {
        my_traits.insert(sTemp);
    }
 }

 for( size_t i = 0; i < mutation_data.size(); i++ ) {
    dump >> sTemp;
    if (sTemp == "MUTATIONS_END") {
        break;
    } else {
        my_mutations[sTemp]; // Creates a new entry with default values
    }
 }

 set_highest_cat_level();
 drench_mut_calc();

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_bp; i++)
  dump >> temp_cur[i] >> temp_conv[i] >> frostbite_timer[i];

 for( auto &skill : Skill::skills ) {
     dump >> skillLevel( skill );
 }

 int num_recipes;
 std::string rec_name;
 dump >> num_recipes;
 for (int i = 0; i < num_recipes; ++i)
 {
  dump >> rec_name;
  learned_recipes[rec_name] = (recipe *)recipe_by_name(rec_name);
 }

 int numstyles;
 itype_id styletype;
 dump >> numstyles;
 for (int i = 0; i < numstyles; i++) {
  dump >> styletype;
  ma_styles.push_back( styletype );
 }

 int numill;
 disease illtmp;
 int temp_bpart;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
     dump >> illtmp.type >> illtmp.duration >> illtmp.intensity
          >> temp_bpart;
     illtmp.bp = (body_part)temp_bpart;
     illness.push_back(illtmp);
 }

 int numadd = 0;
 int typetmp;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio = 0;
 bionic_id biotype;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> biotype >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = biotype;
  my_bionics.push_back(biotmp);
 }

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  // Load morale properties in structure order.
  int mortype;
  std::string item_id;
  dump >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
        if( item::type_is_defined( item_id ) ) {
            mortmp.item_type = item::find_type( item_id );
        } else {
            mortmp.item_type = nullptr;
        }

  dump >> mortmp.bonus >> mortmp.duration >> mortmp.decay_start
       >> mortmp.age;
  morale.push_back(mortmp);
 }

 int nummis = 0;
 int mistmp;
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  active_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  completed_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  failed_missions.push_back(mistmp);
 }

 stats & pstats = *lifetime_stats();

 dump >> pstats.squares_walked;

 recalc_sight_limits();
 if( grab_point.x != 0 || grab_point.y != 0 ) {
    // ancient save only supported grabbed vehicles.
    grab_type = OBJECT_VEHICLE;
 }
}

///// pldata.h
 void player_activity::load_legacy(std::stringstream &dump)
 {
  int tmp, tmptype, tmpinvlet;
  std::string tmpname;
  dump >> tmptype >> moves_left >> index >> tmpinvlet >> tmpname >> placement.x >> placement.y >> tmp;
  name = tmpname.substr(4);
  type = activity_type(tmptype);
  // can't actually save in the middle of an activity with invlets, so not supporting legacy is ok.
  position = INT_MIN;
  for (int i = 0; i < tmp; i++) {
   int tmp2;
   dump >> tmp2;
   values.push_back(tmp2);
  }
 }


///// npc.h
void npc::load_legacy(std::stringstream & dump) {
    std::string tmpname;
    int deathtmp, deadtmp, classtmp, npc_id;
 dump >> npc_id;
 setID(npc_id);
// Standard player stuff
 do {
  dump >> tmpname;
  if (tmpname != "||")
   name += tmpname + " ";
 } while (tmpname != "||");
 name = name.substr(0, name.size() - 1); // Strip off trailing " "
 dump >> position.x >> position.y >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> hunger >> thirst >>
         fatigue >> stim >> pain >> pkill >> radiation >> cash >> recoil >>
         scent >> moves >> underwater >> dodges_left >> oxygen >> deathtmp >>
         deadtmp >> classtmp >> patience;

 if (deathtmp == 1)
  marked_for_death = true;
 else
  marked_for_death = false;

 if (deadtmp == 1)
  dead = true;
 else
  dead = false;

 myclass = npc_class(classtmp);

 std::string sTemp = "";
 for( size_t i = 0; i < mutation_data.size(); i++ ) {
    dump >> sTemp;
    if (sTemp == "TRAITS_END") {
        break;
    } else {
        my_traits.insert(sTemp);
    }
 }

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for( auto &skill : Skill::skills ) {
     dump >> skillLevel( skill );
 }

 itype_id tmpstyle;
 int numstyle;
 dump >> numstyle;
 for (int i = 0; i < numstyle; i++) {
  dump >> tmpstyle;
  ma_styles.push_back(tmpstyle);
 }

 int typetmp;
 std::string disease_type_tmp;
 int numill;
 dump >> numill;
 disease illtmp;
 for (int i = 0; i < numill; i++) {
  dump >> disease_type_tmp >> illtmp.duration;
  illtmp.type = disease_type_tmp;
  illness.push_back(illtmp);
 }
 int numadd;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }
 bionic_id tmpbionic;
 int numbio;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> tmpbionic >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = bionic_id(tmpbionic);
  my_bionics.push_back(biotmp);
 }
// Special NPC stuff
 int misstmp, flagstmp, tmpatt, agg, bra, col, alt;
 int omx, omy;
 dump >> agg >> bra >> col >> alt >> wandx >> wandy >> wandf >> omx >> omy >>
         mapz >> mapx >> mapy >> plx >> ply >> goal.x >> goal.y >> goal.z >> misstmp >>
         flagstmp >> fac_id >> tmpatt;
 mapx += omx * OMAPX * 2;
 mapy += omy * OMAPY * 2;
 personality.aggression = agg;
 personality.bravery = bra;
 personality.collector = col;
 personality.altruism = alt;
 mission = npc_mission(misstmp);
 flags = flagstmp;
 attitude = npc_attitude(tmpatt);

 op_of_u.load_legacy(dump);
 chatbin.load_legacy(dump);
 combat_rules.load_legacy(dump);
}

 void npc_opinion::load_legacy(std::stringstream &info)
 {
  int tmpsize;
  info >> trust >> fear >> value >> anger >> owed >> tmpsize;
  for (int i = 0; i < tmpsize; i++) {
   int tmptype, tmpskill;
   std::string tmpitem;
   npc_favor tmpfavor;
   info >> tmptype >> tmpfavor.value >> tmpitem >> tmpskill;
   tmpfavor.type = npc_favor_type(tmptype);
   tmpfavor.item_id = tmpitem;
   tmpfavor.skill = Skill::skill(tmpskill);
   favors.push_back(tmpfavor);
  }
 }

 void npc_combat_rules::load_legacy(std::istream &data)
 {
  int tmpen;
  data >> tmpen >> use_guns >> use_grenades >> use_silent;
  engagement = combat_engagement(tmpen);
 }

 void npc_chatbin::load_legacy(std::stringstream &info)
 {
  int tmpsize_miss, tmpsize_assigned, tmptopic;
  std::string skill_ident;
  info >> tmptopic >> mission_selected >> tempvalue >> skill_ident >>
          tmpsize_miss >> tmpsize_assigned;
  first_topic = talk_topic(tmptopic);
  skill = skill_ident == "none" ? NULL : Skill::skill(skill_ident);
  for (int i = 0; i < tmpsize_miss; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions.push_back(tmpmiss);
  }
  for (int i = 0; i < tmpsize_assigned; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions_assigned.push_back(tmpmiss);
  }
 }

///// inventory.h
void inventory::load_invlet_cache( std::ifstream &fin ) {
    // Lines are of the format "P itemname abcde".
    while( fin.peek() == 'P' ) {
        std::string invlet_cache_line;
        getline( fin, invlet_cache_line );
        int first_sym = invlet_cache_line.find_first_of(' ', 2);
        std::string item_type( invlet_cache_line, 2, first_sym - 2 );
        std::vector<char> symbol_vec( invlet_cache_line.begin() + first_sym + 1,
                                      invlet_cache_line.end() );
        invlet_cache[ item_type ] = symbol_vec;
    }
}

///// skill.h
std::istream& operator>>(std::istream& is, SkillLevel& obj) {
  int level; int exercise; bool isTraining; int lastPracticed;

  is >> level >> exercise >> isTraining >> lastPracticed;

  obj = SkillLevel(level, exercise, isTraining, lastPracticed);

  return is;
}

///// monster.h


void monster::load_legacy(std::stringstream & dump) {
    int idtmp, plansize, speed, faction_dummy;
    dump >> idtmp >> position.x >> position.y >> wandx >> wandy >> wandf >> moves >> speed >>
         hp >> sp_timeout[0] >> plansize >> friendly >> faction_dummy >> mission_id >>
         no_extra_death_drops >> dead >> anger >> morale;

    // load->int->str->int (possibly shifted)
    type = GetMType( legacy_mon_id[idtmp] );

    Creature::set_speed_base( speed );

    point ptmp;
    plans.clear();
    for (int i = 0; i < plansize; i++) {
        dump >> ptmp.x >> ptmp.y;
        plans.push_back(ptmp);
    }
}
///// item.h

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars );

void item::load_legacy(std::stringstream & dump) {
    clear();
    std::string idtmp, ammotmp, item_tag, mode;
    int lettmp, damtmp, acttmp, corp, tag_count;
	int owned; // Ignoring an obsolete member. 
    dump >> lettmp >> idtmp >> charges >> damtmp >> tag_count;
    for( int i = 0; i < tag_count; ++i )
    {
        dump >> item_tag;
        if( itag2ivar(item_tag, item_vars ) == false ) {
            item_tags.insert( item_tag );
        }
    }

    dump >> burnt >> poison >> ammotmp >> owned >> bday >>
         mode >> acttmp >> corp >> mission_id >> player_id;
    if (corp != -1)
        corpse = GetMType(legacy_mon_id[corp]);
    else
        corpse = NULL;
    getline(dump, name);
    if (name == " ''")
        name = "";
    else {
        size_t pos = name.find_first_of("@@");
        while (pos != std::string::npos)  {
            name.replace(pos, 2, "\n");
            pos = name.find_first_of("@@");
        }
        name = name.substr(2, name.size() - 3); // s/^ '(.*)'$/\1/
    }
    set_gun_mode( mode );

    if( idtmp == "UPS_on" ) {
        idtmp = "UPS_off";
    } else if( idtmp == "adv_UPS_on" ) {
        idtmp = "adv_UPS_off" ;
    }
    make(idtmp);

    invlet = char(lettmp);
    damage = damtmp;
    active = false;
    if (acttmp == 1) {
        active = true;
    }
    set_curammo( ammotmp );
}

///// vehicle.h
// Matches vpart_id integers from old saves (ranging from 72260f0 2013-08-22 onward, including 0.8)
// This never changes, unless things get renamed / removed.
//
const std::string legacy_vpart_id[74] = {"null", "seat", "saddle", "bed", "frame_horizontal", "frame_vertical",
    "frame_cross", "frame_nw", "frame_ne", "frame_se", "frame_sw", "frame_horizontal_2", "frame_vertical_2",
    "frame_cover", "frame_handle", "board_horizontal", "board_vertical", "board_nw", "board_ne", "board_se",
    "board_sw", "aisle_horizontal", "aisle_vertical", "trunk_floor", "roof", "door", "door_opaque",
    "door_internal", "windshield", "blade_horizontal", "blade_vertical", "spike", "wheel", "wheel_wide",
    "wheel_underbody", "wheel_bicycle", "wheel_motorbike", "wheel_small", "wheel_caster", "engine_1cyl",
    "engine_vtwin", "engine_inline4", "engine_v6", "engine_v8", "engine_electric", "engine_electric_large",
    "engine_plasma", "foot_pedals", "gas_tank", "storage_battery", "minireactor", "hydrogen_tank", "water_tank",
    "trunk", "box", "controls", "muffler", "seatbelt", "solar_panel", "kitchen_unit", "welding_rig", "m249",
    "flamethrower", "plasma_gun", "fusion_gun", "plating_steel", "plating_superalloy", "plating_spiked",
    "plating_hard", "headlight", "reinforced_windshield", "horn_bicycle", "horn_car", "horn_big"};

void vehicle::load_legacy(std::ifstream &stin) {
    int fdir, mdir, skd, prts, cr_on, li_on, tag_count;
    std::string vehicle_tag;
    stin >>
        posx >>
        posy >>
        fdir >>
        mdir >>
        turn_dir >>
        velocity >>
        cruise_velocity >>
        cr_on >>
        li_on >>
        turret_mode >>
        skd >>
        of_turn_carry >>
        prts;
    face.init (fdir);
    move.init (mdir);
    skidding = skd != 0;
    cruise_on = cr_on != 0;
    lights_on = li_on != 0;
    std::string databuff;
    getline(stin, databuff); // Clear EoL
    getline(stin, name); // read name
    int itms = 0;
    for (int p = 0; p < prts; p++)
    {
        int pid, pdx, pdy, php, pam, pbld, pbig, pflag, pass, pnit;
        stin >> pid >> pdx >> pdy >> php >> pam >> pbld >> pbig >> pflag >> pass >> pnit;
        getline(stin, databuff); // Clear EoL
        vehicle_part new_part;
        new_part.setid(legacy_vpart_id[ pid ]);
        new_part.mount.x = pdx;
        new_part.mount.y = pdy;
        new_part.hp = php;
        new_part.blood = pbld;
        new_part.bigness = pbig;
        new_part.flags = pflag;
        new_part.passenger_id = pass;
        new_part.amount = pam;
        parts.push_back (new_part);
        for (int j = 0; j < pnit; j++)
        {
            itms++;
            getline(stin, databuff);
            item itm;
            itm.load_info(databuff);
            add_item(j, itm);
            int ncont;
            stin >> ncont; // how many items inside container
            getline(stin, databuff); // Clear EoL
            for (int k = 0; k < ncont; k++)
            {
                getline(stin, databuff);
                item citm;
                citm.load_info(databuff);
                new_part.items.back().put_in (citm);
            }
        }
    }
    /* After loading, check if the vehicle is from the old rules and is missing
     * frames. */
    add_missing_frames();
    refresh();

    stin >> tag_count;
    for( int i = 0; i < tag_count; ++i )
    {
        stin >> vehicle_tag;
        tags.insert( vehicle_tag );
    }
    getline(stin, databuff); // Clear EoL
}


bool game::unserialize_master_legacy(std::ifstream & fin) {
// First, get the next ID numbers for each of these
 std::string data;
 char junk;
 fin >> next_mission_id >> next_faction_id >> next_npc_id;
 int num_missions, num_factions;

 fin >> num_missions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_missions; i++) {
  mission tmpmiss;
  tmpmiss.load_info(fin);
  active_missions.push_back(tmpmiss);
 }

 fin >> num_factions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_factions; i++) {
  getline(fin, data);
  faction tmp;
  tmp.load_info(data);
  factions.push_back(tmp);
 }
 return true;
}
