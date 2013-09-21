
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
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 9;

/*
 * This is a global set by detected version header in .sav, maps.txt, or overmap.
 * This allows loaders for classes that exist in multiple files (such as item) to have
 * support for backwards compatibility as well.
 */
int savegame_loading_version = savegame_version;

////////////////////////////////////////////////////////////////////////////////////////
///// game.sav

/*
 * Save to opened character.sav
 */
void game::serialize(std::ofstream & fout) {
/*
 * Format version 9: Hybrid format. Ordered, line by line mix of json chunks, and stringstream bits that don't
 * really make sense as json. New data can be added to the basic game state json, or tacked onto the end as a
 * new line.
 * To prevent (or encourage) confusion, there is no version 8. (cata 0.8 uses v7)
 */
        // Header
        fout << "# version " << savegame_version << std::endl;

        std::map<std::string, picojson::value> data;
        // basic game state information.
        data["turn"] = pv( (int)turn );
        data["last_target"] = pv( (int)last_target );
        data["run_mode"] = pv( (int)run_mode );
        data["mostseen"] = pv( mostseen );
        data["nextinv"] = pv( (int)nextinv );
        data["next_npc_id"] = pv( next_npc_id );
        data["next_faction_id"] = pv( next_faction_id );
        data["next_mission_id"] = pv( next_mission_id );
        data["nextspawn"] = pv( (int)nextspawn );
        // current map coordinates
        data["levx"] = pv( levx );
        data["levy"] = pv( levy );
        data["levz"] = pv( levz );
        data["om_x"] = pv( cur_om->pos().x );
        data["om_y"] = pv( cur_om->pos().y );
        fout << pv(data).serialize();
        fout << std::endl;

        // Weather. todo: move elsewhere
        fout << save_weather();
        fout << std::endl;

        // Next, the scent map.
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
                fout << grscent[i][j] << " ";
            }
        }

        // Now save all monsters. First the amount
        fout << std::endl << num_zombies() << std::endl;
        // Then each monster + inv in a 1 line json string
        for (int i = 0; i < num_zombies(); i++) {
            fout << _z[i].save_info() << std::endl;
        }

        // save killcounts.
        // todo: When monsters get stringid, make a json_save
        for (int i = 0; i < num_monsters; i++) {
            fout << kills[i] << " ";
        }
        fout << std::endl;

        // And finally the player.
        // u.save_info dumps player + contents in a single json line, followed by memorial log
        // one entry per line starting with '|'
        fout << u.save_info() << std::endl; 

        fout << std::endl;
        ////////
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
   if ( fin.peek() == '#' ) {
       std::string vline;
       getline(fin, vline);
       std::string tmphash, tmpver;
       int savedver=-1;
       std::stringstream vliness(vline);
       vliness >> tmphash >> tmpver >> savedver;
       if ( tmpver == "version" && savedver != -1 ) {
           savegame_loading_version = savedver;
       }
   }
   if (savegame_loading_version != savegame_version) {
       if ( unserialize_legacy(fin) == true ) {
            return;
       } else {
           popup_nowait(_("Cannot find loader for save data in old version %d, attempting to load as current version %d."),savegame_loading_version, savegame_version);
       }
   }
       // Format version 9. After radical compatibility breaking changes, raise savegame_version, cut below and add to
       // unserialize_legacy in savegame_legacy.cpp
            std::string linebuf;
            std::stringstream linein;


            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tmpinv;
            picojson::value pval;
            parseline() >> pval;
            std::string jsonerr = picojson::get_last_error();
            if ( ! jsonerr.empty() ) {
                debugmsg("Bad save json\n%s", jsonerr.c_str() );
            }
            picojson::object &pdata = pval.get<picojson::object>();

            picoint(pdata,"turn",tmpturn);
            picoint(pdata,"last_target",tmptar);
            picoint(pdata,"run_mode", tmprun);
            picoint(pdata,"mostseen", mostseen);
            picoint(pdata,"nextinv", tmpinv);
            nextinv = (char)tmpinv;
            picoint(pdata,"next_npc_id", next_npc_id);
            picoint(pdata,"next_faction_id", next_faction_id);
            picoint(pdata,"next_mission_id", next_mission_id);
            picoint(pdata,"nextspawn",tmpspawn);

            getline(fin, linebuf); // Does weather need to be loaded in this order? Probably not,
            load_weather(linebuf); // but better safe than jackie chan expressions later

            picoint(pdata,"levx",levx);
            picoint(pdata,"levy",levy);
            picoint(pdata,"levz",levz);
            picoint(pdata,"om_x",comx);
            picoint(pdata,"om_y",comy);

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
            for (int i = 0; i < nummon; i++)
            {
                getline(fin, data);
                montmp.load_info(data, &mtypes);
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
            // end .sav version 9
}
////////////////////////////////////////////////////////////////////////////////////////
///// overmap

////////////////////////////////////////////////////////////////////////////////////////
///// mapbuffer
