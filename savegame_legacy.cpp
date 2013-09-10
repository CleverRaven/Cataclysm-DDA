
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
#include "item_factory.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "translations.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include "debug.h"
#include "artifactdata.h"
#include "weather.h"

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
bool	 game::unserialize_legacy(std::ifstream & fin) {

   switch (savegame_loading_version) {
       case 3: {

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
            return true;
        ////////
        } break;

        case 0: {
         // todo: make savegame_legacy.cpp?
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
         return true;
        ///////////////////////////////////// legacy save
        } break;
    }
    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete overmap. These can linger unless the player moves around to all explored areas.
 */

bool overmap::unserialize_legacy(game *g, std::ifstream & fin, std::string const & plrfilename, std::string const & terfilename) {
   return false; // stub
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Parse an open, obsolete maps.txt.
 */
bool mapbuffer::unserialize_legacy(std::ifstream & fin ) {
   return false;
}