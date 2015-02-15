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
#include "mission.h"
#include "faction.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "messages.h"
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
#include "mapsharing.h"

#include "savegame.h"
#include "tile_id_data.h"

/*
 * Changes that break backwards compatibility should bump this number, so the game can
 * load a legacy format loader.
 */
const int savegame_version = 23;
const int savegame_minver_game = 11;

/*
 * This is a global set by detected version header in .sav, maps.txt, or overmap.
 * This allows loaders for classes that exist in multiple files (such as item) to have
 * support for backwards compatibility as well.
 */
int savegame_loading_version = savegame_version;

////////////////////////////////////////////////////////////////////////////////////////
///// on runtime populate lookup tables.
std::map<std::string, int> obj_type_id;

void game::init_savedata_translation_tables() {
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
        json.member("turn", (int)calendar::turn);
        json.member("calendar_start", (int)calendar::start);
        json.member( "last_target", (int)last_target );
        json.member( "run_mode", (int)safe_mode );
        json.member( "mostseen", mostseen );
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
        for( auto &elem : grscent ) {
            for( auto val : elem ) {

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
        for( auto &elem : kills ) {
            json.member( elem.first, elem.second );
        }
        json.end_object();

        json.member( "player", u );
        Messages::serialize( json );

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

    int tmpturn, tmpcalstart = 0, tmpspawn, tmprun, tmptar, comx, comy;
    JsonIn jsin(fin);
    try {
        JsonObject data = jsin.get_object();

        data.read("turn",tmpturn);
        data.read("calendar_start",tmpcalstart);
        data.read("last_target",tmptar);
        data.read("run_mode", tmprun);
        data.read("mostseen", mostseen);
        data.read("nextspawn",tmpspawn);
        data.read("levx",levx);
        data.read("levy",levy);
        data.read("levz",levz);
        data.read("om_x",comx);
        data.read("om_y",comy);

        calendar::turn = tmpturn;
        calendar::start = tmpcalstart;
        nextspawn = tmpspawn;

        cur_om = &overmap_buffer.get(comx, comy);
        m.load(levx, levy, levz, true, cur_om);

        safe_mode = static_cast<safe_mode_type>( tmprun );
        if (OPTIONS["SAFEMODE"] && safe_mode == SAFE_MODE_OFF) {
            safe_mode = SAFE_MODE_ON;
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
            for( auto &elem : grscent ) {
                for( auto &elem_j : elem ) {
                    if (count == 0) {
                        linein >> stmp >> count;
                    }
                    count--;
                    elem_j = stmp;
                }
            }
        }

        JsonArray vdata = data.get_array("active_monsters");
        clear_zombies();
        while (vdata.has_more()) {
            monster montmp;
            vdata.read_next(montmp);
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
        for( const auto &member : members ) {
            kills[member] = odata.get_int( member );
        }

        data.read("player", u);
        Messages::deserialize( data );

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
       lightning_active = (line.compare("lightning: 1") == 0);
   } else {
       lightning_active = false;
   }
    if (fin.peek() == 's') {
        std::string line, label;
        getline(fin, line);
        int seed(0);
        std::stringstream liness(line);
        liness >> label >> seed;
        weatherSeed = seed;
    }
}

void game::save_weather(std::ofstream & fout) {
    fout << "# version " << savegame_version << std::endl;
    fout << "lightning: " << (lightning_active ? "1" : "0") << std::endl;
    fout << "seed: " << weatherSeed;
}
///// overmap
void overmap::unserialize(std::ifstream & fin, std::string const & plrfilename,
                          std::string const & terfilename) {
    // DEBUG VARS
    int nummg = 0;
    char datatype;
    int cx, cy, cz, cs, cp, cd, cdying, horde, tx, ty, intr;
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
        if ( unserialize_legacy(fin, plrfilename, terfilename) == true ) {
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
                            if( otermap.count( tmp_ter ) > 0 ) {
                                tmp_otid = tmp_ter;
                            } else if( tmp_ter.compare( 0, 7, "mall_a_" ) == 0 &&
                                       otermap.count( tmp_ter + "_north" ) > 0 ) {
                                tmp_otid = tmp_ter + "_north";
                            } else if( tmp_ter.compare( 0, 13, "necropolis_a_" ) == 0 &&
                                       otermap.count( tmp_ter + "_north" ) > 0 ) {
                                tmp_otid = tmp_ter + "_north";
                            } else {
                                debugmsg("Loaded bad ter!  %s; ter %s", terfilename.c_str(), tmp_ter.c_str());
                                tmp_otid = 0;
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
            // save compatiblity hack: read the line, initialze new members to 0,
            // "parse" line,
            std::string tmp;
            getline(fin, tmp);
            std::istringstream buffer(tmp);
            horde = 0;
            tx = 0;
            ty = 0;
            intr = 0;
            buffer >> cstr >> cx >> cy >> cz >> cs >> cp >> cd >> cdying >> horde >> tx >> ty >>intr;
            mongroup mg( cstr, cx, cy, cz, cs, cp );
            // Bugfix for old saves: population of 2147483647 is far too much and will
            // crash the game. This specific number was caused by a bug in
            // overmap::add_mon_group.
            if( mg.population == 2147483647ul ) {
                mg.population = rng( 1, 10 );
            }
            mg.diffuse = cd;
            mg.dying = cdying;
            mg.horde = horde;
            mg.set_target( tx, ty );
            mg.interest = intr;
            add_mon_group( mg );
            nummg++;
        } else if( datatype == 'M' ) {
            tripoint mon_loc;
            monster new_monster;
            fin >> mon_loc.x >> mon_loc.y >> mon_loc.z;
            std::string data;
            getline( fin, data );
            new_monster.deserialize( data );
            monster_map.insert( std::make_pair( std::move(mon_loc),
                                                std::move(new_monster) ) );
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
                case 'I': npc_inventory.push_back(tmp);                 break;
                case 'C': npc_inventory.back().contents.push_back(tmp); break;
                case 'W': last->worn.push_back(tmp);                    break;
                case 'w': last->weapon = tmp;                           break;
                case 'c': last->weapon.contents.push_back(tmp);         break;
                }
            }
        } else if ( datatype == '!' ) { // temporary holder for future sanity
            std::string tmpstr;
            getline(fin, tmpstr);
            if ( tmpstr.size() > 1 && ( tmpstr[0] == '{' || tmpstr[1] == '{' ) ) {
                std::stringstream derp;
                derp << tmpstr;
                JsonIn jsin(derp);
                try {
                    JsonObject data = jsin.get_object();

                    if ( data.read("region_id",tmpstr) ) { // temporary, until option DEFAULT_REGION becomes start_scenario.region_id
                        if ( settings.id != tmpstr ) {
                            std::unordered_map<std::string, regional_settings>::const_iterator rit =
                                region_settings_map.find( tmpstr );
                            if ( rit != region_settings_map.end() ) {
                                // temporary; user changed option, this overmap should remain whatever it was set to.
                                settings = rit->second; // todo optimize
                            } else { // ruh-roh! user changed option and deleted the .json with this overmap's region. We'll have to become current default. And whine about it.
                                std::string tmpopt = ACTIVE_WORLD_OPTIONS["DEFAULT_REGION"].getValue();
                                rit = region_settings_map.find( tmpopt );
                                if ( rit == region_settings_map.end() ) { // ...oy. Hopefully 'default' exists. If not, it's crashtime anyway.
                                    debugmsg("               WARNING: overmap uses missing region settings '%s'                 \n\
                ERROR, 'default_region' option uses missing region settings '%s'. Falling back to 'default'               \n\
                ....... good luck.                 \n",
                                              tmpstr.c_str(), tmpopt.c_str() );
                                    // fallback means we already loaded default and got a warning earlier.
                                } else {
                                    debugmsg("               WARNING: overmap uses missing region settings '%s', falling back to '%s'                \n",
                                              tmpstr.c_str(), tmpopt.c_str() );
                                    // fallback means we already loaded ACTIVE_WORLD_OPTIONS["DEFAULT_REGION"]
                                }
                            }
                        }
                    }
                } catch(std::string jsonerr) {
                    debugmsg("load overmap: json error\n%s", jsonerr.c_str() );
                    // just continue with default region
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
            } else if (datatype == 'E') { //Load explored areas
                sfin >> z;

                std::string dataline;
                getline(sfin, dataline); // Chomp endl

                int count = 0;
                int explored;
                if (z >= 0 && z < OVERMAP_LAYERS) {
                    for (int j = 0; j < OMAPY; j++) {
                        for (int i = 0; i < OMAPX; i++) {
                            if (count == 0) {
                                sfin >> explored >> count;
                            }
                            count--;
                            layer[z].explored[i][j] = (explored == 1);
                        }
                    }
                }
            } else if (datatype == 'N') { // Load notes
                om_note tmp;
                sfin >> tmp.x >> tmp.y;
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

// Note: this may throw io errors from std::ofstream
void overmap::save() const
{
    std::ofstream fout;
    fout.exceptions(std::ios::badbit | std::ios::failbit);
    std::string const plrfilename = overmapbuffer::player_filename(loc.x, loc.y);
    std::string const terfilename = overmapbuffer::terrain_filename(loc.x, loc.y);

    // Player specific data
    fout.open(plrfilename.c_str());

    fout << "# version " << savegame_version << std::endl;

    for (int z = 0; z < OVERMAP_LAYERS; ++z) {
        fout << "L " << z << std::endl;
        int count = 0;
        int lastvis = -1;
        int lastexp = -1;
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

        //So we don't break saves that don't have this data, we add a new type.
        fout << "E " << z << std::endl;
        count = 0;

        for (int j = 0; j < OMAPY; j++) {
            for (int i = 0; i < OMAPX; i++) {
                int e = (layer[z].explored[i][j] ? 1 : 0);
                if (e != lastexp) {
                    if (count) {
                        fout << count << " ";
                    }
                    lastexp = e;
                    fout << e << " ";
                    count = 1;
                } else {
                    count++;
                }
            }
        }
        fout << count;
        fout << std::endl;

        for (auto &i : layer[z].notes) {
            fout << "N " << i.x << " " << i.y << " " << std::endl << i.text << std::endl;
        }
    }
    fout.close();

    // World terrain data
    fopen_exclusive(fout, terfilename.c_str(), std::ios_base::trunc);
    if(!fout.is_open()) {
        return;
    }
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

    try {
        fout << "! ";
        JsonOut json(fout, false);
        json.start_object();
        json.member("region_id", settings.id); // temporary, to allow user to manually switch regions during play until regionmap is done.
        json.end_object();
    } catch (std::string e) {
        //debugmsg("error saving overmap: %s", e.c_str());
    }
    fout << std::endl;

    for( auto &mgv : zg ) {
        auto &mg = mgv.second;
        fout << "Z " << mg.type << " " << mg.posx << " " << mg.posy << " " <<
            mg.posz << " " << int(mg.radius) << " " << mg.population << " " <<
            mg.diffuse << " " << mg.dying << " " <<
            mg.horde << " " << mg.tx << " " << mg.ty << " " << mg.interest << std::endl;
    }
    for (auto &i : cities)
        fout << "t " << i.x << " " << i.y << " " << i.s << std::endl;
    for (auto &i : roads_out)
        fout << "R " << i.x << " " << i.y << std::endl;
    for (auto &i : radios)
        fout << "T " << i.x << " " << i.y << " " << i.strength <<
            " " << i.type << " " << std::endl << i.message << std::endl;

    for( const auto &mdata : monster_map ) {
        fout << "M " << mdata.first.x << " " << mdata.first.y << " " << mdata.first.z <<
            " " << mdata.second.serialize() << std::endl;
    }

    // store tracked vehicle locations and names
    for( const auto &elem : vehicles ) {
        int id = elem.first;
        om_vehicle v = elem.second;
        fout << "v " << id << " " << v.name << " " << v.x << " " << v.y << std::endl;
    }

    //saving the npcs
    for (auto &i : npcs)
        fout << "n " << i->save_info() << std::endl;

    fclose_exclusive(fout, terfilename.c_str());
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
        for (auto &i : active_missions) {
            i.serialize(json);
        }
        json.end_array();

        json.member("factions");
        json.start_array();
        for (auto &i : factions) {
            i.serialize(json);
        }
        json.end_array();

        json.end_object();
    } catch (std::string e) {
        debugmsg("error saving to master.gsav: %s", e.c_str());
    }
}

