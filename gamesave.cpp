
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
#include "mission.h"
#include "faction.h"
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

#include "savegame.h"
#include "tile_id_data.h"

/*
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 11;

/*
 * This is a global set by detected version header in .sav, maps.txt, or overmap.
 * This allows loaders for classes that exist in multiple files (such as item) to have
 * support for backwards compatibility as well.
 */
int savegame_loading_version = savegame_version;

////////////////////////////////////////////////////////////////////////////////////////
///// on runtime populate lookup tables. This is temporary: monster_ints
std::map<std::string, int> monster_ints;
std::map<std::string, int> obj_type_id;

void game::init_savedata_translation_tables() {
    monster_ints.clear();
    for(int i=0; i < num_monsters; i++) {
        monster_ints[ monster_names[i] ] = i;
    }
    obj_type_id.clear();
    for(int i = 0; i < NUM_OBJECTS; i++) {
        obj_type_id[ obj_type_name[i] ] = i;
    }
}
////////////////////////////////////////////////////////////////////////////////////////
///// game.sav

/*
 * Save to opened character.sav
 */
void game::serialize(std::ofstream & fout) {
/*
 * Format version 12: Fully json, save the header. Weather and memorial exist elsewhere.
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

        // Next, the scent map.
        std::stringstream rle_out;
        int rle_lastval = -1;
        int rle_count = 0;
        for (int i = 0; i < SEEX * MAPSIZE; i++) {
            for (int j = 0; j < SEEY * MAPSIZE; j++) {
               int val = grscent[i][j];
               if (val == rle_lastval) {
                   rle_count++;
               } else {
                   if ( rle_count ) {
                       rle_out << rle_count << " ";
                   }
                   rle_out << val << " ";
                   rle_lastval = val;
                   rle_count = 1;
               }
            }
        }
        rle_out << rle_count;
        data["grscent"] = pv ( rle_out.str() );

        // Then each monster
        std::vector<picojson::value> amdata;
        for (int i = 0; i < num_zombies(); i++) {
            amdata.push_back( _active_monsters[i].json_save(true) );
        }
        data["active_monsters"] = pv( amdata );

        // save killcounts.
        std::map<std::string, picojson::value> killmap;
        for (std::map<std::string, int>::iterator kill = kills.begin(); kill != kills.end(); ++kill){
            killmap[kill->first] = pv(kill->second);
        }
        data["kills"] = pv( killmap );

        data["player"] = pv( u.json_save(true) );

        fout << pv(data).serialize() << std::endl;
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

void chkversion(std::istream & fin) {
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
}

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
       // Format version 12. After radical compatibility breaking changes, raise savegame_version, cut below and add to
       // unserialize_legacy in savegame_legacy.cpp
            std::string linebuf;
            std::stringstream linein;


            int tmpturn, tmpspawn, tmprun, tmptar, comx, comy, tmpinv;
            picojson::value pval;
            fin >> pval;
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

            linebuf="";
            if ( picostring(pdata,"grscent",linebuf) ) {
                linein.clear();
                linein.str(linebuf);

                int stmp;
                int count = 0;
                for (int i = 0; i < SEEX *MAPSIZE; i++) {
                    for (int j = 0; j < SEEY * MAPSIZE; j++) {
                        if (count == 0) {
                            linein >> stmp >> count;
                        }
                        count--;
                        grscent[i][j] = stmp;
                    }
                }
            }

            picojson::array * vdata = pgetarray(pdata,"active_monsters");
            clear_zombies();
            monster montmp;
            for( picojson::array::iterator pit = vdata->begin(); pit != vdata->end(); ++pit) {
                montmp.json_load( *pit, &mtypes );
                add_zombie(montmp);
            }

            picojson::object * odata = pgetmap(pdata,"kills");
            for( picojson::object::const_iterator it = odata->begin(); it != odata->end(); ++it) {
                kills[it->first] = (int)it->second.get<double>();
            }

            u.json_load( pdata["player"], this);
}

///// weather
void game::load_weather(std::ifstream & fin) {
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
     
     while(!fin.eof()) {
        std::string data;
        getline(fin, data);

        std::stringstream wl;
        wl.str(data);
        int inturn, intemp, inweather, inzone;
        wl >> inturn >> intemp >> inweather >> inzone;
        weather_segment wtm;
        wtm.weather = (weather_type)inweather;
        wtm.temperature = intemp;
        wtm.deadline = inturn;
        if ( inzone == 0 ) {
           weather_log[inturn] = wtm;
        } else {
           debugmsg("weather zones unimplemented. bad data '%s'", data.c_str() );
        }
     }
    std::map<int, weather_segment>::iterator w_it = weather_log.lower_bound(int(turn));
    if ( w_it != weather_log.end() ) {
        weather_segment cur = w_it->second;
        if ( w_it->first > int(turn) ) {
            --w_it;
            if ( w_it != weather_log.end() ) {
                cur = w_it->second;
            }
        }
        weather = cur.weather;
        temperature = cur.temperature;
        nextweather = cur.deadline;
    }
}

void game::save_weather(std::ofstream & fout) {
    fout << "# version " << savegame_version << std::endl;
    const int climatezone = 0;
    for( std::map<int, weather_segment>::const_iterator it = weather_log.begin(); it != weather_log.end(); ++it ) {
      fout << it->first
        << " " << int(it->second.temperature)
        << " " << it->second.weather
        << " " << climatezone << std::endl;
    }
}
///// overmap
void overmap::unserialize(game * g, std::ifstream & fin, std::string const & plrfilename,
                          std::string const & terfilename) {
    // DEBUG VARS
    int nummg = 0;
    char datatype;
    int cx, cy, cz, cs, cp, cd, cdying;
    std::string cstr;
    city tmp;
    std::list<item> npc_inventory;

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
        if ( unserialize_legacy(g, fin, plrfilename, terfilename) == true ) {
            return;
        }
    }

    int z = 0; // assumption
    while (fin >> datatype) {
        if (datatype == 'L') { // Load layer data, and switch to layer
            fin >> z;

            int tmp_ter;
            if (z >= 0 && z < OVERMAP_LAYERS) {
                int count = 0;
                for (int j = 0; j < OMAPY; j++) {
                    for (int i = 0; i < OMAPX; i++) {
                        if (count == 0) {
                            fin >> tmp_ter >> count;
                            if (tmp_ter < 0 || tmp_ter > num_ter_types) {
                                debugmsg("Loaded bad ter!  %s; ter %d", terfilename.c_str(), tmp_ter);
                            }
                        }
                        count--;
                        layer[z].terrain[i][j] = oter_id(tmp_ter);
                        layer[z].visible[i][j] = false;
                    }
                }
            } else {
                debugmsg("Loaded z level out of range (z: %d)", z);
            }
        } else if (datatype == 'Z') { // Monster group
            fin >> cstr >> cx >> cy >> cz >> cs >> cp >> cd >> cdying;
            zg.push_back(mongroup(cstr, cx, cy, cz, cs, cp));
            zg.back().diffuse = cd;
            zg.back().dying = cdying;
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
// When we start loading a new NPC, check to see if we've accumulated items for
//   assignment to an NPC.

            if (!npc_inventory.empty() && !npcs.empty()) {
                npcs.back()->inv.add_stack(npc_inventory);
                npc_inventory.clear();
            }
            std::string npcdata;
            getline(fin, npcdata);
            npc * tmp = new npc();
            tmp->load_info(g, npcdata);
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
                debugmsg("Overmap %d:%d:%d tried to load object data, without an NPC!",
                         loc.x, loc.y);
                debugmsg(itemdata.c_str());
            } else {
                item tmp(itemdata, g);
                npc* last = npcs.back();
                switch (datatype) {
                case 'I': npc_inventory.push_back(tmp);                 break;
                case 'C': npc_inventory.back().contents.push_back(tmp); break;
                case 'W': last->worn.push_back(tmp);                    break;
                case 'w': last->weapon = tmp;                           break;
                case 'c': last->weapon.contents.push_back(tmp);         break;
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
                    for (int j = 0; j < OMAPY; j++) {
                        for (int i = 0; i < OMAPX; i++) {
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
}


void overmap::save()
{
    if (layer == NULL) {
        debugmsg("Tried to save a null overmap");
        return;
    }

    std::ofstream fout;
    std::string const plrfilename = player_filename(loc.x, loc.y);
    std::string const terfilename = terrain_filename(loc.x, loc.y);

    // Player specific data
    fout.open(plrfilename.c_str());

    fout << "# version " << savegame_version << std::endl;

    for (int z = 0; z < OVERMAP_LAYERS; ++z) {
        fout << "L " << z << std::endl;
        int count = 0;
        int lastvis = -1;
        for (int j = 0; j < OMAPY; j++) {
            for (int i = 0; i < OMAPX; i++) {
                int v = (layer[z].visible[i][j] ? 1 : 0);
                if (v != lastvis) {
                    if (count) {
                        fout << count << " ";
                    }
                    lastvis = v;
                    fout << v << " ";
                    count = 1;
                } else {
                    count++;
                }
            }
        }
        fout << count;
        fout << std::endl;

        for (int i = 0; i < layer[z].notes.size(); i++) {
            fout << "N " << layer[z].notes[i].x << " " << layer[z].notes[i].y << " " <<
                layer[z].notes[i].num << std::endl << layer[z].notes[i].text << std::endl;
        }
    }
    fout.close();

    // World terrain data
    fout.open(terfilename.c_str(), std::ios_base::trunc);
    fout << "# version " << savegame_version << std::endl;
    for (int z = 0; z < OVERMAP_LAYERS; ++z) {
        fout << "L " << z << std::endl;
        int count = 0;
        int last_tertype = -1;
        for (int j = 0; j < OMAPY; j++) {
            for (int i = 0; i < OMAPX; i++) {
                int t = int(layer[z].terrain[i][j]);
                if (t != last_tertype) {
                    if (count) {
                        fout << count << " ";
                    }
                    last_tertype = t;
                    fout << t << " ";
                    count = 1;
                } else {
                    count++;
                }
            }
        }
        fout << count;
        fout << std::endl;
    }

    for (int i = 0; i < zg.size(); i++)
        fout << "Z " << zg[i].type << " " << zg[i].posx << " " << zg[i].posy << " " <<
            zg[i].posz << " " << int(zg[i].radius) << " " << zg[i].population << " " <<
            zg[i].diffuse << " " << zg[i].dying << std::endl;
    for (int i = 0; i < cities.size(); i++)
        fout << "t " << cities[i].x << " " << cities[i].y << " " << cities[i].s << std::endl;
    for (int i = 0; i < roads_out.size(); i++)
        fout << "R " << roads_out[i].x << " " << roads_out[i].y << std::endl;
    for (int i = 0; i < radios.size(); i++)
        fout << "T " << radios[i].x << " " << radios[i].y << " " << radios[i].strength <<
            " " << radios[i].type << " " << std::endl << radios[i].message << std::endl;

    //saving the npcs
    for (int i = 0; i < npcs.size(); i++)
        fout << "n " << npcs[i]->save_info() << std::endl;

    fout.close();
}

////////////////////////////////////////////////////////////////////////////////////////
///// mapbuffer

///////////////////////////////////////////////////////////////////////////////////////
///// master.gsav

void game::unserialize_master(std::ifstream &fin) {
   savegame_loading_version = 0;
   chkversion(fin);
   if (savegame_loading_version != savegame_version) {
       if ( unserialize_master_legacy(fin) == true ) {
            return;
       } else {
           popup_nowait(_("Cannot find loader for save data in old version %d, attempting to load as current version %d."),savegame_loading_version, savegame_version);
       }
   }
    picojson::value pval;
    fin >> pval;
    std::string jsonerr = picojson::get_last_error();
    if ( ! jsonerr.empty() ) {
        debugmsg("Bad save json\n%s", jsonerr.c_str() );
    }
    picojson::object &data = pval.get<picojson::object>();
    picoint(data, "next_mission_id", next_mission_id);
    picoint(data, "next_faction_id", next_faction_id);
    picoint(data, "next_npc_id", next_npc_id);

    picojson::array * vdata = pgetarray(data,"active_missions");
    if(vdata != NULL) {
        for( picojson::array::iterator pit = vdata->begin(); pit != vdata->end(); ++pit) {
            mission tmp;
            tmp.json_load( *pit, this );
            active_missions.push_back(tmp);
        }
    }

    vdata = pgetarray(data,"factions");
    if(vdata != NULL) {
        for( picojson::array::iterator pit = vdata->begin(); pit != vdata->end(); ++pit) {
            faction tmp;
            tmp.json_load( *pit, this );
            factions.push_back(tmp);
        }
    }
    
}

void game::serialize_master(std::ofstream &fout) {
    fout << "# version " << savegame_version << std::endl;
    std::map<std::string, picojson::value> data;
    data["next_mission_id"] = pv ( next_mission_id );
    data["next_faction_id"] = pv ( next_faction_id );
    data["next_npc_id"] = pv ( next_npc_id );

    std::vector<picojson::value> vdata;
    for (int i = 0; i < active_missions.size(); i++) {
        vdata.push_back( pv( active_missions[i].json_save() ) );
    }
    data["active_missions"] = pv( vdata );
    vdata.clear();

    for (int i = 0; i < factions.size(); i++) {
        vdata.push_back( pv( factions[i].json_save() ) );
    }
    data["factions"] = pv( vdata );
    vdata.clear();
    fout << pv( data ).serialize() << std::endl;
}
