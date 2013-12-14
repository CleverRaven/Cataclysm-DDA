
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
#include "weather.h"

#include "savegame.h"
#include "tile_id_data.h"

/*
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 12;
const int savegame_minver_game = 11;
//const int savegame_minver_map = 11;
const int savegame_minver_overmap = 12;

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

        JsonOut json(fout, true); // pretty-print

        json.start_object();
        // basic game state information.
        json.member( "turn", (int)turn );
        json.member( "last_target", (int)last_target );
        json.member( "run_mode", (int)run_mode );
        json.member( "mostseen", mostseen );
        json.member( "nextinv", (int)nextinv );
        json.member( "next_npc_id", next_npc_id );
        json.member( "next_faction_id", next_faction_id );
        json.member( "next_mission_id", next_mission_id );
        json.member( "nextspawn", (int)nextspawn );
        // current map coordinates
        json.member( "levx", levx );
        json.member( "levy", levy );
        json.member( "levz", levz );
        json.member( "om_x", cur_om->pos().x );
        json.member( "om_y", cur_om->pos().y );

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
        json.member( "grscent", rle_out.str() );

        // Then each monster
        json.member( "active_monsters", critter_tracker.list() );
        json.member( "stair_monsters", coming_to_stairs );

        // save killcounts.
        json.member( "kills" );
        json.start_object();
        for (std::map<std::string, int>::iterator kill = kills.begin(); kill != kills.end(); ++kill){
            json.member( kill->first, kill->second );
        }
        json.end_object();

        json.member( "player", u );

        json.end_object();
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
void game::unserialize(std::ifstream & fin)
{
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
   if (savegame_loading_version != savegame_version &&
            savegame_loading_version < savegame_minver_game ) {
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
    JsonIn jsin(fin);
    try {
        JsonObject data = jsin.get_object();

        data.read("turn",tmpturn);
        data.read("last_target",tmptar);
        data.read("run_mode", tmprun);
        data.read("mostseen", mostseen);
        data.read("nextinv", tmpinv);
        nextinv = (char)tmpinv;
        data.read("next_npc_id", next_npc_id);
        data.read("next_faction_id", next_faction_id);
        data.read("next_mission_id", next_mission_id);
        data.read("nextspawn",tmpspawn);
        data.read("levx",levx);
        data.read("levy",levy);
        data.read("levz",levz);
        data.read("om_x",comx);
        data.read("om_y",comy);

        turn = tmpturn;
        nextspawn = tmpspawn;

        cur_om = &overmap_buffer.get(this, comx, comy);
        m.load(this, levx, levy, levz);

        run_mode = tmprun;
        if (OPTIONS["SAFEMODE"] && run_mode == 0) {
            run_mode = 1;
        }
        autosafemode = OPTIONS["AUTOSAFEMODE"];
        safemodeveh = OPTIONS["SAFEMODEVEH"];
        last_target = tmptar;

        linebuf="";
        if ( data.read("grscent",linebuf) ) {
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

        JsonArray vdata = data.get_array("active_monsters");
        clear_zombies();
        while (vdata.has_more()) {
            monster montmp;
            vdata.read_next(montmp);
            montmp.setkeep(true);
            add_zombie(montmp);
        }

        vdata = data.get_array("stair_monsters");
        coming_to_stairs.clear();
        while (vdata.has_more()) {
            monster stairtmp;
            vdata.read_next(stairtmp);
            coming_to_stairs.push_back(stairtmp);
        }

        JsonObject odata = data.get_object("kills");
        std::set<std::string> members = odata.get_member_names();
        for (std::set<std::string>::const_iterator it = members.begin();
                it != members.end(); ++it) {
            kills[*it] = odata.get_int(*it);
        }

        data.read("player", u);

    } catch (std::string jsonerr) {
        debugmsg("Bad save json\n%s", jsonerr.c_str() );
        return;
    }
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

   //Check for "lightning:" marker - if absent, ignore
   if (fin.peek() == 'l') {
       std::string line;
       getline(fin, line);
       lightning_active = ((*line.end()) == '1');
   } else {
       lightning_active = false;
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
    fout << "lightning: " << (lightning_active ? "1" : "0") << std::endl;
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

            std::string tmp_ter;
            oter_id tmp_otid(0);
            if (z >= 0 && z < OVERMAP_LAYERS) {
                int count = 0;
                for (int j = 0; j < OMAPY; j++) {
                    for (int i = 0; i < OMAPX; i++) {
                        if (count == 0) {
                            fin >> tmp_ter >> count;
                            if (otermap.find(tmp_ter) == otermap.end()) {
                                debugmsg("Loaded bad ter!  %s; ter %s", terfilename.c_str(), tmp_ter.c_str());
                                tmp_otid = 0;
                            } else {
                                tmp_otid = tmp_ter;
                            }
                        }
                        count--;
                        layer[z].terrain[i][j] = tmp_otid; //otermap[tmp_ter].loadid;
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
        } else if ( datatype == 'v' ) {
            om_vehicle v;
            int id;
            fin >> id >> v.name >> v.x >> v.y;
            vehicles[id]=v;
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
        oter_id last_tertype(-1);
        for (int j = 0; j < OMAPY; j++) {
            for (int i = 0; i < OMAPX; i++) {
                oter_id t = layer[z].terrain[i][j];
                if (t != last_tertype) {
                    if (count) {
                        fout << count << " ";
                    }
                    last_tertype = t;
                    fout << std::string(t) << " ";
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

    // store tracked vehicle locations and names
    for (std::map<int, om_vehicle>::const_iterator it = vehicles.begin();
            it != vehicles.end(); it++)
    {
        int id = it->first;
        om_vehicle v = it->second;
        fout << "v " << id << " " << v.name << " " << v.x << " " << v.y << std::endl;
    }

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
   if (savegame_loading_version != savegame_version && savegame_loading_version < 11) {
       if ( unserialize_master_legacy(fin) == true ) {
            return;
       } else {
           popup_nowait(_("Cannot find loader for save data in old version %d, attempting to load as current version %d."),savegame_loading_version, savegame_version);
       }
   }
    try {
        // single-pass parsing example
        JsonIn jsin(fin);
        jsin.start_object();
        while (!jsin.end_object()) {
            std::string name = jsin.get_member_name();
            if (name == "next_mission_id") {
                next_mission_id = jsin.get_int();
            } else if (name == "next_faction_id") {
                next_faction_id = jsin.get_int();
            } else if (name == "next_npc_id") {
                next_npc_id = jsin.get_int();
            } else if (name == "active_missions") {
                jsin.start_array();
                while (!jsin.end_array()) {
                    mission mis;
                    mis.deserialize(jsin);
                    active_missions.push_back(mis);
                }
            } else if (name == "factions") {
                jsin.start_array();
                while (!jsin.end_array()) {
                    faction fac;
                    fac.deserialize(jsin);
                    factions.push_back(fac);
                }
            } else {
                // silently ignore anything else
                jsin.skip_value();
            }
        }
    } catch (std::string e) {
        debugmsg("error loading master.gsav: %s", e.c_str());
    }
}

void game::serialize_master(std::ofstream &fout) {
    fout << "# version " << savegame_version << std::endl;
    try {
        JsonOut json(fout, true); // pretty-print
        json.start_object();

        json.member("next_mission_id", next_mission_id);
        json.member("next_faction_id", next_faction_id);
        json.member("next_npc_id", next_npc_id);

        json.member("active_missions");
        json.start_array();
        for (int i = 0; i < active_missions.size(); ++i) {
            active_missions[i].serialize(json);
        }
        json.end_array();

        json.member("factions");
        json.start_array();
        for (int i = 0; i < factions.size(); ++i) {
            factions[i].serialize(json);
        }
        json.end_array();

        json.end_object();
    } catch (std::string e) {
        debugmsg("error saving to master.gsav: %s", e.c_str());
    }
}

