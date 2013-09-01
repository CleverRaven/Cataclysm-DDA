
#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "options.h"
#include "auto_pickup.h"
#include "mapbuffer.h"
#include "debug.h"
#include "editmap.h"
#include "bodypart.h"
#include "map.h"
#include "object.h"
#include "output.h"
#include "uistate.h"
#include "item_factory.h"
#include "advanced_inv.h"
#include "helper.h"
#include "text_snippets.h"
#include "catajson.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "catacharset.h"
#include "translations.h"
#include "rules.h"
//#include "player.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "debug.h"
#include "artifactdata.h"
#include "weather.h"
#include "weather_const.h"
#include "bench.h"
#if (defined _WIN32 || defined __WIN32__)
#include <windows.h>
#include <tchar.h>
#endif

#ifdef _MSC_VER
// MSVC doesn't have c99-compatible "snprintf", so do what picojson does and use _snprintf_s instead
#define snprintf _snprintf_s
#endif

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

/*
 * Save to opened character.sav
 */
void game::serialize(std::ofstream & fout) {
/*
 * save format version. If an alteration is made that breaks saves, please bump this version and
 * make a new copy in serialize_load.
 */
    const int savever = 3;

switch (savever) {
    default: {
/*
 * Format version 2: Interim format. Still resembles a hairball, but it's at least a multi-line hairball;
 * Data is segmented for readabilty, stability, and gradual conversion into something closer to sanity.
 */
        // Header
        fout << "# version " << savever << std::endl;
        // First, write out basic game state information.
        fout << int(turn) << "  " << int(last_target) << " " << int(run_mode) << " " <<
             mostseen << " " << nextinv << " " << next_npc_id << " " <<
             next_faction_id << " " << next_mission_id << " " << int(nextspawn) << std::endl;

        // future weather (for now)
        fout << save_weather();
        fout << std::endl;

        // current map coordinates
        fout << levx << " " << levy << " " << levz << " " << cur_om->pos().x <<
             " " << cur_om->pos().y << " " << std::endl;

        // Next, the scent map.
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
                fout << grscent[i][j] << " ";
            }
        }

        // Now save all monsters.
        fout << std::endl << num_zombies() << std::endl;

        for (int i = 0; i < num_zombies(); i++) {
            fout << _z[i].save_info() << std::endl;
            fout << _z[i].inv.size() << std::endl;
            for( std::vector<item>::iterator it = _z[i].inv.begin(); it != _z[i].inv.end(); ++it ) {
                fout << it->save_info() << std::endl;
            }
        }

        for (int i = 0; i < num_monsters; i++) { // Save the kill counts, too.
            fout << kills[i] << " ";
        }
        fout << std::endl;

        // And finally the player.
        fout << u.save_info() << std::endl;

        fout << std::endl;
        ////////
    } break;
 case 0: {
// old instabile mess, which is only preserved for testing and only usable past
// 15612a3 anyway.
 // First, write out basic game state information.
 fout << int(turn) << " " << int(last_target) << " " << int(run_mode) << " " <<
         mostseen << " " << nextinv << " " << next_npc_id << " " <<
     next_faction_id << " " << next_mission_id << " " << int(nextspawn) << " ";

 fout << save_weather();

 fout << levx << " " << levy << " " << levz << " " << cur_om->pos().x <<
         " " << cur_om->pos().y << " " << std::endl;
 // Next, the scent map.
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   fout << grscent[i][j] << " ";
 }
 // Now save all monsters.
 fout << std::endl << num_zombies() << std::endl;
 for (int i = 0; i < num_zombies(); i++) {
     fout << _z[i].save_info() << std::endl;
     fout << _z[i].inv.size() << std::endl;
     for( std::vector<item>::iterator it = _z[i].inv.begin(); it != _z[i].inv.end(); ++it )
     {
         fout << it->save_info() << std::endl;
     }
 }
 for (int i = 0; i < num_monsters; i++)	// Save the kill counts, too.
  fout << kills[i] << " ";
 // And finally the player.
 fout << u.save_info() << std::endl;
 fout << std::endl;

 } break;
/////
}

}

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
 * Parse an open .sav file.
 */
void game::unserialize(std::ifstream & fin) {
   int loadver=0; // version to load; set by '# version n' header
   if ( fin.peek() == '#' ) {
       std::string vline;
       getline(fin, vline);
       std::string tmphash, tmpver;
       int savedver=-1;
       std::stringstream vliness(vline);
       vliness >> tmphash >> tmpver >> savedver;
       if ( tmpver == "version" && savedver != -1 ) {
           loadver = savedver;
       }
   }
   switch (loadver) {
/*
       case ???: {
       } break;
*/
       case 3:
       case 2: {
/*
 * Format version 3: Temporary format. Still resembles a hairball, but it's at least a multi-line hairball;
 * Data is segmented for readabilty, stability, and gradual conversion into something closer to sanity.
 */
            std::string linebuf;
            std::stringstream linein;

            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy;

            parseline() >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
                next_faction_id >> next_mission_id >> tmpspawn;

            getline(fin, linebuf);
            load_weather(linebuf);

            parseline() >> levx >> levy >> levz >> comx >> comy;

            turn = tmpturn;
            nextspawn = tmpspawn;

            cur_om = &overmap_buffer.get(this, comx, comy);
            m.load(this, levx, levy, levz);

            run_mode = tmprun;
            if (OPTIONS["SAFEMODE"] && run_mode == 0) {
                run_mode = 1;
            }
            autosafemode = OPTIONS["AUTOSAFEMODE"];
            last_target = tmptar;

            // Next, the scent map.
            parseline();

            for (int i = 0; i < SEEX *MAPSIZE; i++) {
                for (int j = 0; j < SEEY * MAPSIZE; j++) {
                    linein >> grscent[i][j];
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
                montmp.load_info(data, &mtypes);

                fin >> num_items;
                // Chomp the endline after number of items.
                getline( fin, data );
                for (int i = 0; i < num_items; i++) {
                    getline( fin, data );
                    montmp.inv.push_back( item( data, this ) );
                }

                add_zombie(montmp);
            }

            // And the kill counts;
            parseline();
            int kk;
            for (kk = 0; kk < num_monsters && !linein.eof(); kk++) {
                linein >> kills[kk];
            }
            if ( kk != num_monsters ) {
                debugmsg("Warning, number of monsters changed from %d to %d", kk+1, num_monsters );
            }

            // Finally, the data on the player.
            getline(fin, data);
            u.load_info(this, data);

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
                        item tmpitem(itemdata, this);
#ifdef has_baseobject
                        tmpitem.parentref = &u;
#endif
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
        ////////
        } break;

        default: {
/*
original 'structure', which globs game/weather/location & killcount/player data onto the same lines.
*/
         int tmpturn, tmpspawn, tmprun, tmptar, comx, comy;
         fin >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
             next_faction_id >> next_mission_id >> tmpspawn;

         load_weather(fin);

         fin >> levx >> levy >> levz >> comx >> comy;

         turn = tmpturn;
         nextspawn = tmpspawn;

         cur_om = &overmap_buffer.get(this, comx, comy);
         m.load(this, levx, levy, levz);

         run_mode = tmprun;
         if (OPTIONS["SAFEMODE"] && run_mode == 0)
          run_mode = 1;
         autosafemode = OPTIONS["AUTOSAFEMODE"];
         last_target = tmptar;

        // Next, the scent map.
         for (int i = 0; i < SEEX * MAPSIZE; i++) {
          for (int j = 0; j < SEEY * MAPSIZE; j++)
           fin >> grscent[i][j];
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
          montmp.load_info(data, &mtypes);

          fin >> num_items;
          // Chomp the endline after number of items.
          getline( fin, data );
          for (int i = 0; i < num_items; i++) {
              getline( fin, data );
              montmp.inv.push_back( item( data, this ) );
          }

          add_zombie(montmp);
         }
        // And the kill counts;
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         for (int i = 0; i < num_monsters; i++)
          fin >> kills[i];
        // Finally, the data on the player.
         if (fin.peek() == '\n')
          fin.get(junk); // Chomp that pesky endline
         getline(fin, data);
         u.load_info(this, data);
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
               item tmpitem(itemdata, this);
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
        ///////////////////////////////////// legacy save
        } break;
    }
}
