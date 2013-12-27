#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <vector>
#include <sstream>
#include "overmap.h"
#include "rng.h"
#include "line.h"
#include "game.h"
#include "npc.h"
#include "keypress.h"
#include <cstring>
#include <ostream>
#include "debug.h"
#include "cursesdef.h"
#include "options.h"
#include "catacharset.h"
#include "overmapbuffer.h"
#include "action.h"
#include "input.h"
#include "json.h"
#include <queue>
#include "mapdata.h"
#include "mapgen.h"
#define dbg(x) dout((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

#ifdef _MSC_VER
// MSVC doesn't have c99-compatible "snprintf", so do what picojson does and use _snprintf_s instead
#define snprintf _snprintf_s
#endif

#define STREETCHANCE 2
#define NUM_FOREST 250
#define TOP_HIWAY_DIST 999
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16
#define SETTLE_DICE 2
#define SETTLE_SIDES 2
#define HIVECHANCE 180 //Chance that any given forest will be a hive
#define SWAMPINESS 4 //Affects the size of a swamp
#define SWAMPCHANCE 8500 // Chance that a swamp will spawn instead of forest
enum oter_dir {
    oter_dir_north, oter_dir_east, oter_dir_west, oter_dir_south
};

map_extras no_extras(0);
map_extras road_extras(
// %%% HEL MIL SCI STA DRG SUP PRT MIN CRT FUM 1WY ART
    50, 40, 50,120,200, 30, 10,  5, 80, 10,  8,  2,  3);
map_extras field_extras(
    60, 40, 15, 40, 80, 10, 10,  3, 50, 10,  8,  1,  3);
map_extras subway_extras(
// %%% HEL MIL SCI STA DRG SUP PRT MIN CRT FUM 1WY ART
    75,  0,  5, 12,  5,  5,  0,  7,  0,  0, 20,  1,  3);
map_extras build_extras(
    90,  0,  5, 12,  0, 10,  0,  5,  5, 60,  8,  1,  3);

std::map<std::string,oter_t> otermap;
std::vector<oter_t> oterlist;

std::map<std::string, oter_t&> obasetermap;
//const regional_settings default_region_settings;
std::map<std::string, regional_settings> region_settings_map;

overmap_special overmap_specials[NUM_OMSPECS] = {

// Terrain  MIN MAX DISTANCE
{"crater",    0, 10,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_BLOB) | mfb(OMS_FLAG_CLASSIC)},

{"hive",     0, 50, 10, -1, "GROUP_BEE", 20, 60, 2, 4,
 &omspec_place::forest, mfb(OMS_FLAG_3X3)},

{"house_north",   0,100,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{"s_gas_north",   0,100,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{"cabin",   0, 30, 20, -1, "GROUP_NULL", 0, 0, 0, 0,  // Woods cabin
 &omspec_place::forest, mfb(OMS_FLAG_CLASSIC)},

{"cabin_strange",   1, 1, 20, -1, "GROUP_NULL", 0, 0, 0, 0,  // Hidden cabin
 &omspec_place::forest, mfb(OMS_FLAG_CLASSIC)},

{"lmoe",   0, 3, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_CLASSIC)},

{"farm",   0, 20, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_3X3_SECOND) |mfb(OMS_FLAG_DIRT_LOT) | mfb(OMS_FLAG_CLASSIC)},

{"temple_stairs", 0,  3, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::forest, 0},

{"lab_stairs",    0, 30,  8, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD)},

{"ice_lab_stairs",    0, 30,  8, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD)},

{"fema_entrance",    2, 5,  8, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

// Terrain  MIN MAX DISTANCE
{"bunker",    2, 10,  4, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD)},

{"outpost",    0, 10,  4, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, 0},

{"silo",    0,  1, 30, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD)},

{"radio_tower",   1, 5,  0, 20, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_CLASSIC)},

{"mansion_entrance", 0, 8, 0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{"mansion_entrance", 0, 4, 10, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{"megastore_entrance", 0, 5, 0, 10, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{"hospital_entrance", 1, 5, 3, 15, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{"public_works_entrance",    1, 3,  2, 10, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)},

{"apartments_con_tower_1_entrance",    1, 5,  -1, 2, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)},

{"apartments_mod_tower_1_entrance",    1, 4,  -1, 2, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)},

{"office_tower_1_entrance",    1, 5,  -1, 4, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)},

{"cathedral_1_entrance",    1, 2,  -1, 2, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)},

{"school_2",    1, 3,  1, 5, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_3X3_FIXED)},

{"prison_2",    1, 1,  3, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_3X3_FIXED)},

{"hotel_tower_1_2",    1, 4,  -1, 4, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_3X3_FIXED)},

{"sewage_treatment", 1, 5, 10, 20, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_PARKING_LOT) | mfb(OMS_FLAG_CLASSIC)},

{"mine_entrance",  0, 5,  15, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_PARKING_LOT)},

// Terrain  MIN MAX DISTANCE
{"anthill",    0, 30,  10, -1, "GROUP_ANT", 1000, 2000, 10, 30,
 &omspec_place::wilderness, 0},

{"spider_pit",    0,500,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::forest, 0},

{"slimepit_down",    0,  4,  0, -1, "GROUP_GOO", 100, 200, 2, 10,
 &omspec_place::land, 0},

{"fungal_bloom",  0,  3, 5, -1, "GROUP_FUNGI", 600, 1200, 30, 50,
 &omspec_place::wilderness, 0},

{"triffid_grove", 0,  4,  0, -1, "GROUP_TRIFFID", 800, 1300, 12, 20,
 &omspec_place::forest, 0},

{"river_center",  0, 10, 10, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::always, mfb(OMS_FLAG_BLOB) | mfb(OMS_FLAG_CLASSIC)},

// Terrain  MIN MAX DISTANCE
{"shelter",       5, 10, 5, 10, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{"cave",    0, 30,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, 0},

{"toxic_dump",    0, 5, 15, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_CLASSIC)},

{"s_gas_north",   10,  500,  10, 200, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_ROTATE_ROAD)},

{"haz_sar_entrance",     1,  2, 15, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC) | mfb(OMS_FLAG_2X2_SECOND)}
};


double dist(int x1, int y1, int x2, int y2)
{
 return sqrt(double((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)));
}


oter_id base_oter_id( const std::string & base ) {
    std::map<std::string, oter_t&>::const_iterator it = obasetermap.find( base );
    if ( it == obasetermap.end() ) {
        debugmsg("overmap_terrain: base id '%s' not found.", base.c_str() );
        return 0;
    } else {
        return it->second.loadid_base;
    }  
}



bool is_river(const oter_id &ter)
{
    // if the id starts with "river" or "bridge", count as a river, but this
    // is done in data init.
    // return (ter.compare(0,5,"river",5) == 0 || ter.compare(0,6,"bridge",6) == 0);
    return ter.t().is_river;
}

bool is_ot_type(const std::string &otype, const oter_id &oter)
{
    const size_t compare_size = otype.size();
    if (compare_size > oter.size()) {
        return false;
    } else {
        return std::string(oter).compare(0, compare_size, otype ) == 0;
    }

}

bool road_allowed(const oter_id &ter)
{
    return ter.t().allow_road;
}

/*
 * Pick an oter_baseid from weightlist and return rotated oter_id
 */
oter_id shop(int dir, oter_weight_list & weightlist ) // todo: rename to something better than 'shop', make it an oter_weight_list method?
{
    if ( dir > 3 ) {
        debugmsg("Bad rotation of weightlist pick: %d.", dir);
        return "";
    }

    dir = dir % 4;
    if (dir < 0) { dir += 4; }
    const int ret = weightlist.pick();

    if ( oterlist[ ret ].rotates == false ) {
        return ret;
    }
    return oterlist[ ret ].directional_peers[dir];
}

oter_id house(int dir, int chance_of_basement)
{
    static const oter_id iid_house = oter_id("house_north");
    static const oter_id iid_house_base = oter_id("house_base_north");

    if (dir < 0) {
        dir += 4;
    } else if (dir > 3) {
        debugmsg("Bad rotation of house: %d.", dir);
        return "";
    }
    return ( one_in( chance_of_basement) ? iid_house : iid_house_base ).t().directional_peers[dir];
}

map_extras& get_extras(const std::string &name)
{
    if (name == "field") {
        return field_extras;
    } else if (name == "road") {
        return road_extras;
    } else if (name == "subway") {
        return subway_extras;
    } else if (name == "build") {
        return build_extras;
    } else {
        return no_extras;
    }
}

// oter_t specific affirmatives to is_road, set at startup (todo; jsonize)
bool isroad(std::string bstr) {
    if (bstr=="road" || bstr=="bridge" ||
        bstr=="subway" || bstr=="sewer" ||
        bstr=="sewage_treatment_hub" ||
        bstr=="sewage_treatment_under" ||
        bstr == "rift" || bstr == "hellmouth") {
        return true;
    }
    return false;
}

void load_oter(oter_t & oter) {
    oter.loadid = oterlist.size();
    otermap[oter.id] = oter;
    oterlist.push_back(oter);
}

/*
 * load mapgen functions from an overmap_terrain json entry
 * suffix is for roads/subways/etc which have "_straight", "_curved", "_tee", "_four_way" function mappings
 */
void load_overmap_terrain_mapgens(JsonObject &jo, const std::string id_base, const std::string suffix = "")
{
    const std::string fmapkey(id_base + suffix);
    const std::string jsonkey("mapgen" + suffix);
    bool default_mapgen = jo.get_bool("default_mapgen", true);
    int default_idx = -1;
    if ( default_mapgen ) {
        if ( mapgen_cfunction_map.find( fmapkey ) != mapgen_cfunction_map.end() ) {
            oter_mapgen[fmapkey].push_back( new mapgen_function_builtin( fmapkey ) );
            default_idx = oter_mapgen[fmapkey].size() - 1;
        }
    }
    if ( jo.has_array( jsonkey ) ) {
        JsonArray ja = jo.get_array( jsonkey );
        int c=0;
        while ( ja.has_more() ) {
            if ( ja.has_object(c) ) {
                JsonObject jio = ja.next_object();
                load_mapgen_function( jio, fmapkey, default_idx );
            }
            c++;
        }
    }
}

void load_overmap_terrain(JsonObject &jo)
{
    oter_t oter;
    long syms[4];

    oter.id = jo.get_string("id");
    oter.name = _(jo.get_string("name").c_str());
    oter.rotates = jo.get_bool("rotate", false);
    oter.line_drawing = jo.get_bool("line_drawing", false);
    if (oter.line_drawing) {
        oter.sym = jo.get_int("sym", (int)'%');
    } else if (jo.has_array("sym")) {
        JsonArray ja = jo.get_array("sym");
        for (int i = 0; i < 4; ++i) {
            syms[i] = ja.next_int();
        }
        oter.sym = syms[0];
    } else if (oter.rotates) {
        oter.sym = jo.get_int("sym");
        for (int i = 0; i < 4; ++i) {
            syms[i] = oter.sym;
        }
    } else {
        oter.sym = jo.get_int("sym");
    }

    oter.color = color_from_string(jo.get_string("color"));
    oter.see_cost = jo.get_int("see_cost");

    oter.extras = jo.get_string("extras", "none");
    oter.known_down = jo.get_bool("known_down", false);
    oter.known_up = jo.get_bool("known_up", false);
    oter.mondensity = jo.get_int("mondensity", 0);
    oter.sidewalk = jo.get_bool("sidewalk", false);
    oter.allow_road = jo.get_bool("allow_road", false);

    std::string id_base = oter.id;
    int start_iid = oterlist.size();
    oter.id_base = id_base;
    oter.loadid_base = start_iid;
    oter.directional_peers.clear();

    oter.is_road = isroad(id_base);
    oter.is_river = (id_base.compare(0,5,"river",5) == 0 || id_base.compare(0,6,"bridge",6) == 0);

    oter.id_mapgen = id_base; // What, another identifier? Whyyy...
    if ( ! oter.line_drawing ) { // ...oh
        load_overmap_terrain_mapgens(jo, id_base);
    }

    if (oter.line_drawing) {
        // add variants for line drawing
        for( int i = start_iid; i < start_iid+12; i++ ) {
            oter.directional_peers.push_back(i);
        }
        oter.id_mapgen = id_base + "_straight";
        load_overmap_terrain_mapgens(jo, id_base, "_straight");
        oter.id = id_base + "_ns";
        oter.sym = LINE_XOXO;
        load_oter(oter);

        oter.id = id_base + "_ew";
        oter.sym = LINE_OXOX;
        load_oter(oter);

        oter.id_mapgen = id_base + "_curved";
        load_overmap_terrain_mapgens(jo, id_base, "_curved");
        oter.id = id_base + "_ne";
        oter.sym = LINE_XXOO;
        load_oter(oter);

        oter.id = id_base + "_es";
        oter.sym = LINE_OXXO;
        load_oter(oter);

        oter.id = id_base + "_sw";
        oter.sym = LINE_OOXX;
        load_oter(oter);

        oter.id = id_base + "_wn";
        oter.sym = LINE_XOOX;
        load_oter(oter);

        oter.id_mapgen = id_base + "_tee";
        load_overmap_terrain_mapgens(jo, id_base, "_tee");
        oter.id = id_base + "_nes";
        oter.sym = LINE_XXXO;
        load_oter(oter);

        oter.id = id_base + "_new";
        oter.sym = LINE_XXOX;
        load_oter(oter);

        oter.id = id_base + "_nsw";
        oter.sym = LINE_XOXX;
        load_oter(oter);

        oter.id = id_base + "_esw";
        oter.sym = LINE_OXXX;
        load_oter(oter);


        oter.id_mapgen = id_base + "_four_way";
        load_overmap_terrain_mapgens(jo, id_base, "_four_way");
        oter.id = id_base + "_nesw";
        oter.sym = LINE_XXXX;
        load_oter(oter);

    } else if (oter.rotates) {
        // add north/east/south/west variants

        for( int i = start_iid; i < start_iid+5; i++ ) {
            oter.directional_peers.push_back(i);
        }

        oter.id = id_base + "_north";
        oter.sym = syms[0];
        load_oter(oter);

        oter.id = id_base + "_east";
        oter.sym = syms[1];
        load_oter(oter);

        oter.id = id_base + "_south";
        oter.sym = syms[2];
        load_oter(oter);

        oter.id = id_base + "_west";
        oter.sym = syms[3];
        load_oter(oter);

    } else {
        oter.directional_peers.push_back(start_iid);
        load_oter(oter);
    }
}


/*
 * Assemble a map of overmap_terrain base ids pointing to first members of oter groups
 * We'll do this after json loading so references can be used
 */
void finalize_overmap_terrain( ) {
    int c=0;
    for( std::vector<oter_t>::const_iterator it = oterlist.begin(); it != oterlist.end(); ++it ) {
        if ( (*it).loadid == (*it).loadid_base ) {
            if ( (*it).loadid != c ) { // might as well sanity check while we're here da? da.
                debugmsg("ERROR: oterlist[%d]: mismatch with loadid (%d). (id = %s, id_base = %s)",
                    c, (*it).loadid, (*it).id.c_str(), (*it).id_base.c_str()
                );
                // aaaaaaaand continue to inevitable crash
            }
            obasetermap.insert( std::pair<std::string, oter_t&>( (*it).id_base, oterlist[c] ) );;
        }
        c++;
    }
    // here's another sanity check, yay.
    if ( region_settings_map.find("default") == region_settings_map.end() ) {
        debugmsg("ERROR: can't find default overmap settings (region_map_settings 'default'), cataclysm pending. And not the fun kind.");
    }

    for( std::map<std::string, regional_settings>::iterator rsit = region_settings_map.begin(); rsit != region_settings_map.end(); ++rsit) {
        rsit->second.setup();
    }
};



void load_region_settings( JsonObject &jo ) {
    regional_settings new_region;
    if ( ! jo.read("id",new_region.id) ) {
        jo.throw_error("No 'id' field.");
    }
    bool strict = ( new_region.id == "default" );
    if ( ! jo.read("default_oter", new_region.default_oter) && strict ) {
        jo.throw_error("default_oter required for default ( though it should probably remain 'field' )");
    }
    if ( jo.has_object("default_groundcover") ) {
        JsonObject jio = jo.get_object("default_groundcover");
        new_region.default_groundcover_str = new sid_or_sid("t_grass",4,"t_dirt");
        if ( ! jio.read("primary", new_region.default_groundcover_str->primary_str) ||
           ! jio.read("secondary", new_region.default_groundcover_str->secondary_str) ||
           ! jio.read("ratio", new_region.default_groundcover.chance) ) {
            jo.throw_error("'default_groundcover' missing one of:\n   { \"primary\": \"ter_id\", \"secondary\": \"ter_id\", \"ratio\": (number) }\n");
        }
    } else if ( strict ) {
        jo.throw_error("'default_groundcover' required for 'default'");
    }
    if ( ! jo.read("num_forests", new_region.num_forests) && strict ) {
        jo.throw_error("num_forests required for default");
    }
    if ( ! jo.read("forest_size_min", new_region.forest_size_min) && strict ) {
        jo.throw_error("forest_size_min required for default");
    }
    if ( ! jo.read("forest_size_max", new_region.forest_size_max) && strict ) {
        jo.throw_error("forest_size_max required for default");
    }
    if ( ! jo.read("house_basement_chance", new_region.house_basement_chance) && strict ) {
        jo.throw_error("house_basement_chance required for default");
    }
    if ( ! jo.read("swamp_maxsize", new_region.swamp_maxsize) && strict ) {
        jo.throw_error("swamp_maxsize required for default");
    }
    if ( ! jo.read("swamp_river_influence", new_region.swamp_river_influence) && strict ) {
        jo.throw_error("swamp_river_influence required for default");
    }
    if ( ! jo.read("swamp_spread_chance", new_region.swamp_spread_chance) && strict ) {
        jo.throw_error("swamp_spread_chance required for default");
    }

    if ( ! jo.has_object("field_coverage") ) {
        if ( strict ) jo.throw_error("\"field_coverage\": { ... } required for default");
    } else {
        JsonObject pjo = jo.get_object("field_coverage");
        double tmpval = 0.0f;
        if ( ! pjo.read("percent_coverage", tmpval) ) pjo.throw_error("field_coverage: percent_coverage required");
        new_region.field_coverage.mpercent_coverage = (int)(tmpval * 10000.0);
        if ( ! pjo.read("default_ter", new_region.field_coverage.default_ter_str) ) pjo.throw_error("field_coverage: default_ter required");
        tmpval = 0.0f;
        if ( pjo.has_object("other") ) {
            std::string tmpstr="";
            JsonObject opjo = pjo.get_object("other");
            std::set<std::string> keys = opjo.get_member_names();
            for(std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
                tmpval = 0.0f;
                if ( *it != "//" ) {
                    if ( opjo.read(*it, tmpval) ) {
                         new_region.field_coverage.percent_str[ *it ] = tmpval;
                    }
                }
            }
        }
        if ( pjo.read("boost_chance", tmpval) && tmpval != 0.0f ) {
            new_region.field_coverage.boost_chance = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_percent_coverage", tmpval) ) pjo.throw_error("boost_chance > 0 requires boosted_percent_coverage");
            new_region.field_coverage.boosted_mpercent_coverage = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_other_percent", tmpval) ) pjo.throw_error("boost_chance > 0 requires boosted_other_percent");
            new_region.field_coverage.boosted_other_mpercent = (int)(tmpval * 10000.0);
            if ( pjo.has_object("boosted_other") ) {
                std::string tmpstr = "";
                JsonObject opjo = pjo.get_object("boosted_other");
                std::set<std::string> keys = opjo.get_member_names();
                for(std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
                    tmpval = 0.0f;
                    if ( *it != "//" ) {
                        if ( opjo.read(*it, tmpval) ) {
                             new_region.field_coverage.boosted_percent_str[ *it ] = tmpval;
                        }
                    }
                }
            } else {
                pjo.throw_error("boost_chance > 0 requires boosted_other { ... }");
            }
        }
    }

    if ( ! jo.has_object("city") ) {
        if ( strict ) jo.throw_error("\"city\": { ... } required for default");
    } else {
        JsonObject cjo = jo.get_object("city");
        if ( ! cjo.read("shop_radius", new_region.city_spec.shop_radius) && strict ) {
            jo.throw_error("city: shop_radius required for default");
        }
        if ( ! cjo.read("park_radius", new_region.city_spec.park_radius) && strict ) {
            jo.throw_error("city: park_radius required for default");
        }
        if ( ! cjo.has_object("shops") && strict ) {
            if ( strict ) jo.throw_error("city: \"shops\": { ... } required for default");
        } else {
            JsonObject wjo = cjo.get_object("shops");
            std::set<std::string> keys = wjo.get_member_names();
            for(std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
                if ( *it != "//" ) {
                    if ( wjo.has_int( *it ) ) {
                        new_region.city_spec.shops.add_item(*it, wjo.get_int(*it) );
                    }
                }
            }
        }
        if ( ! cjo.has_object("parks") && strict ) {
            if ( strict ) jo.throw_error("city: \"parks\": { ... } required for default");
        } else {
            JsonObject wjo = cjo.get_object("parks");
            std::set<std::string> keys = wjo.get_member_names();
            for(std::set<std::string>::iterator it = keys.begin(); it != keys.end(); ++it) {
                if ( *it != "//" ) {
                    if ( wjo.has_int( *it ) ) {
                        new_region.city_spec.parks.add_item(*it, wjo.get_int(*it) );
                    }
                }
            }
        }
    }
    region_settings_map[new_region.id] = new_region;
};


// *** BEGIN overmap FUNCTIONS ***

overmap::overmap()
 : loc(999, 999)
 , prefix()
 , name()
 , layer(NULL)
 , nullret("")
 , nullbool(false)
 , nullstr("")
{
// debugmsg("Warning - null overmap!");
}

overmap::overmap(int x, int y)
 : loc(x, y)
 , prefix()
 , name(g->u.name)
 , layer(NULL)
 , nullret("")
 , nullbool(false)
 , nullstr("")
{
    if (name.empty()) {
        debugmsg("Attempting to load overmap for unknown player!  Saving won't work!");
    }

    if (g->has_gametype()) {
        prefix = special_game_name(g->gametype());
    }
    // STUB: need region map:
    // settings = regionmap->calculate_settings( loc );
    const std::string rsettings_id = ACTIVE_WORLD_OPTIONS["DEFAULT_REGION"].getValue();
    std::map<std::string, regional_settings>::const_iterator rsit = region_settings_map.find( rsettings_id );

    if ( rsit == region_settings_map.end() ) {
        debugmsg("overmap(%d,%d): can't find region '%s'", x, y, rsettings_id.c_str() ); // gonna die now =[
    }
    settings = rsit->second;

    init_layers();
    open();
}

overmap::overmap(overmap const& o)
    : zg(o.zg)
    , radios(o.radios)
    , npcs(o.npcs)
    , vehicles(o.vehicles)
    , cities(o.cities)
    , roads_out(o.roads_out)
    , loc(o.loc)
    , prefix(o.prefix)
    , name(o.name)
    , layer(NULL)
{
    settings = o.settings;
    layer = new map_layer[OVERMAP_LAYERS];
    for(int z = 0; z < OVERMAP_LAYERS; ++z) {
        for(int i = 0; i < OMAPX; ++i) {
            for(int j = 0; j < OMAPY; ++j) {
                layer[z].terrain[i][j] = o.layer[z].terrain[i][j];
                layer[z].visible[i][j] = o.layer[z].visible[i][j];
            }
        }
        layer[z].notes = o.layer[z].notes;
    }
}

overmap::~overmap()
{
    if (layer) {
        delete [] layer;
        layer = NULL;
    }
}

overmap& overmap::operator=(overmap const& o)
{
    zg = o.zg;
    radios = o.radios;
    npcs = o.npcs;
    vehicles = o.vehicles;
    cities = o.cities;
    roads_out = o.roads_out;
    loc = o.loc;
    prefix = o.prefix;
    name = o.name;

    if (layer) {
        delete [] layer;
        layer = NULL;
    }
    settings = o.settings;

    layer = new map_layer[OVERMAP_LAYERS];
    for(int z = 0; z < OVERMAP_LAYERS; ++z) {
        for(int i = 0; i < OMAPX; ++i) {
            for(int j = 0; j < OMAPY; ++j) {
                layer[z].terrain[i][j] = o.layer[z].terrain[i][j];
                layer[z].visible[i][j] = o.layer[z].visible[i][j];
            }
        }
        layer[z].notes = o.layer[z].notes;
    }
    return *this;
}

void overmap::init_layers()
{
    layer = new map_layer[OVERMAP_LAYERS];
    for(int z = 0; z < OVERMAP_LAYERS; ++z) {
        oter_id default_type = (z < OVERMAP_DEPTH) ? "rock" : (z == OVERMAP_DEPTH) ? settings.default_oter : "";
        for(int i = 0; i < OMAPX; ++i) {
            for(int j = 0; j < OMAPY; ++j) {
                layer[z].terrain[i][j] = default_type;
                layer[z].visible[i][j] = false;
            }
        }
    }
}

oter_id& overmap::ter(const int x, const int y, const int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        nullret = 0;
        return nullret;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

bool& overmap::seen(int x, int y, int z)
{
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
  nullbool = false;
  return nullbool;
 }
 return layer[z + OVERMAP_DEPTH].visible[x][y];
}

// this uses om_sub (submap coordinates localized to overmap,
// aka levxy or om_pos * 2)
std::vector<mongroup*> overmap::monsters_at(int x, int y, int z)
{
 std::vector<mongroup*> ret;
 if (x < 0 || x >= OMAPX*2 || y < 0 || y >= OMAPY*2 || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT)
  return ret;
 for (int i = 0; i < zg.size(); i++) {
  if (zg[i].posz != z) { continue; }
  if (
      ( zg[i].diffuse == true ? square_dist(x, y, zg[i].posx, zg[i].posy) : trig_dist(x, y, zg[i].posx, zg[i].posy) )
    <= zg[i].radius) {
      ret.push_back(&(zg[i]));
  }
 }
 return ret;
}

// this uses om_pos (overmap tiles, aka levxy / 2)
bool overmap::is_safe(int x, int y, int z)
{
 std::vector<mongroup*> mons = monsters_at(x*2, y*2, z);
 if (mons.empty())
  return true;

 bool safe = true;
 for (int n = 0; n < mons.size() && safe; n++)
  safe = mons[n]->is_safe();

 return safe;
}

bool overmap::has_note(int const x, int const y, int const z) const
{
 if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) { return false; }

 for (int i = 0; i < layer[z + OVERMAP_DEPTH].notes.size(); i++) {
  if (layer[z + OVERMAP_DEPTH].notes[i].x == x && layer[z + OVERMAP_DEPTH].notes[i].y == y)
   return true;
 }
 return false;
}

std::string const& overmap::note(int const x, int const y, int const z) const
{
 if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) { return nullstr; }

 for (int i = 0; i < layer[z + OVERMAP_DEPTH].notes.size(); i++) {
  if (layer[z + OVERMAP_DEPTH].notes[i].x == x && layer[z + OVERMAP_DEPTH].notes[i].y == y)
   return layer[z + OVERMAP_DEPTH].notes[i].text;
 }

 return nullstr;
}

void overmap::add_note(int const x, int const y, int const z, std::string const & message)
{
 if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
  debugmsg("Attempting to add not to overmap for blank layer %d", z);
  return;
 }

 for (int i = 0; i < layer[z + OVERMAP_DEPTH].notes.size(); i++) {
  if (layer[z + OVERMAP_DEPTH].notes[i].x == x && layer[z + OVERMAP_DEPTH].notes[i].y == y) {
   if (message.empty())
    layer[z + OVERMAP_DEPTH].notes.erase(layer[z + OVERMAP_DEPTH].notes.begin() + i);
   else
    layer[z + OVERMAP_DEPTH].notes[i].text = message;
   return;
  }
 }
 if (message.length() > 0)
  layer[z + OVERMAP_DEPTH].notes.push_back(om_note(x, y, layer[z + OVERMAP_DEPTH].notes.size(), message));
}

point overmap::find_note(int const x, int const y, int const z, std::string const& text) const
{
 point ret(-1, -1);
 if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
  debugmsg("Attempting to find note on overmap for blank layer %d", z);
  return ret;
 }

 int closest = 9999;
 for (int i = 0; i < layer[z + OVERMAP_DEPTH].notes.size(); i++) {
  if (layer[z + OVERMAP_DEPTH].notes[i].text.find(text) != std::string::npos &&
      rl_dist(x, y, layer[z + OVERMAP_DEPTH].notes[i].x, layer[z + OVERMAP_DEPTH].notes[i].y) < closest) {
   closest = rl_dist(x, y, layer[z + OVERMAP_DEPTH].notes[i].x, layer[z + OVERMAP_DEPTH].notes[i].y);
   ret = point(layer[z + OVERMAP_DEPTH].notes[i].x, layer[z + OVERMAP_DEPTH].notes[i].y);
  }
 }

 return ret;
}

//This removes a npc from the overmap. The NPC is supposed to be already dead.
//This function also assumes the npc is not in the list of active npcs anymore.
void overmap::remove_npc(int npc_id)
{
    for(int i = 0; i < npcs.size(); i++)
    {
        if(npcs[i]->getID() == npc_id)
        {
            //Remove this npc from the list of overmap npcs.
            if(!npcs[i]->dead) debugmsg("overmap::remove_npc: NPC (%d) is not dead.",npc_id);
            npc * tmp = npcs[i];
            npcs.erase(npcs.begin() + i);
            delete tmp;
            return;
        }
    }
}

void overmap::remove_vehicle(int id)
{
    std::map<int, om_vehicle>::iterator om_veh = vehicles.find(id);
    if (om_veh != vehicles.end())
        vehicles.erase(om_veh);

}

int overmap::add_vehicle(vehicle *veh)
{
    int id = vehicles.size() + 1;
    // this *should* be unique but just in case
    while ( vehicles.count(id) > 0 )
        id++;

    om_vehicle tracked_veh;
    tracked_veh.x = veh->omap_x()/2;
    tracked_veh.y = veh->omap_y()/2;
    tracked_veh.name = veh->name;
    vehicles[id]=tracked_veh;

    return id;
}

point overmap::display_notes(int const z) const
{
 if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
  debugmsg("overmap::display_notes: Attempting to display notes on overmap for blank layer %d", z);
  return point(-1, -1);
 }

 WINDOW* w_notes = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                          (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                          (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 draw_border(w_notes);

 std::string title = _("Notes:");
 std::string back_msg = _("< Prev notes");
 std::string forward_msg = _("Next notes >");

 int maxitems; // Number of items to show at one time.
 char end_limit; // Last selectable line.
 if (FULL_SCREEN_HEIGHT/2 == 0) {
    maxitems = 20;
    end_limit = 't';
 }
 else {
    end_limit = 's';
    maxitems = 19;
 }
    
 char ch = '.';
 int start = 0, cur_it(0);

 int back_len = utf8_width(back_msg.c_str());

 mvwprintz(w_notes, 1, 1, c_ltgray, title.c_str());
 do{
  if (ch == '<' && start > 0) {
   for (int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++)
    for (int j = 1; j < FULL_SCREEN_WIDTH - 1; j++)
     mvwputch(w_notes, i, j, c_black, ' ');
   start -= maxitems;
   if (start < 0)
    start = 0;
  }
  if (ch == '>' && cur_it < layer[z + OVERMAP_DEPTH].notes.size()) {
   start = cur_it;
   for (int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++)
    for (int j = 1; j < FULL_SCREEN_WIDTH - 1; j++)
     mvwputch(w_notes, i, j, c_black, ' ');
  }
  int cur_line = 3;
  int last_line = -1;
  char cur_let = 'a';
  for (cur_it = start; cur_it < start + maxitems && cur_line < maxitems + 3; cur_it++) {
   if (cur_it < layer[z + OVERMAP_DEPTH].notes.size()) {
   mvwputch (w_notes, cur_line, 1, c_white, cur_let++);
   mvwprintz(w_notes, cur_line, 3, c_ltgray, "- %s", layer[z + OVERMAP_DEPTH].notes[cur_it].text.c_str());
   } else{
    last_line = cur_line - 2;
    break;
   }
   cur_line++;
  }

  if(last_line == -1)
   last_line = 23;
  if (start > 0)
   mvwprintw(w_notes, maxitems + 3, 1, back_msg.c_str());
  if (cur_it < layer[z + OVERMAP_DEPTH].notes.size())
   mvwprintw(w_notes, maxitems + 3, 2 + back_len, forward_msg.c_str());
  if(ch >= 'a' && ch <= end_limit) {
   int chosen_line = (int)(ch % (int)'a');
   if(chosen_line < last_line)
    return point(layer[z + OVERMAP_DEPTH].notes[start + chosen_line].x, layer[z + OVERMAP_DEPTH].notes[start + chosen_line].y);
  }
  mvwprintz(w_notes, 1, 40, c_white, _("Press letter to center on note"));
  mvwprintz(w_notes, FULL_SCREEN_HEIGHT-2, 40, c_white, _("Spacebar - Return to map  "));
  wrefresh(w_notes);
  ch = getch();
 } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 delwin(w_notes);
 return point(-1,-1);
}

bool overmap::has_npc(int const x, int const y, int const z) const
{
    //Check if the target overmap square has an npc in it.
    for (int n = 0; n < npcs.size(); n++) {
        if(npcs[n]->omz == z && !npcs[n]->marked_for_death)
        {
            if (npcs[n]->is_active())
            { //Active npcs have different coords. Because Cata hates you!
                if ((g->levx + (npcs[n]->posx / SEEX))/2 == x &&
                    (g->levy + (npcs[n]->posy / SEEY))/2 == y)
                    return true;
            } else if ((npcs[n]->mapx)/2 == x && (npcs[n]->mapy)/2== y)
                return true;
        }
    }
    return false;
}

bool overmap::has_vehicle(int const x, int const y, int const z, bool require_pda) const
{
    // vehicles only spawn at z level 0 (for now)
    if (!z == 0)
        return false;

    // if the player is not carrying a PDA then he cannot see the vehicle.
    if (require_pda && !g->u.has_amount("pda", 1))
        return false;

    for (std::map<int, om_vehicle>::const_iterator it = vehicles.begin();
         it != vehicles.end(); it++)
    {
        om_vehicle om_veh = it->second;
        if ( om_veh.x == x && om_veh.y == y )
            return true;
    }
    return false;
}

// int cursx = (g->levx + int(MAPSIZE / 2)) / 2,
//     cursy = (g->levy + int(MAPSIZE / 2)) / 2;

//Helper function for the overmap::draw function.
void overmap::print_npcs(WINDOW *w, int const x, int const y, int const z)
{
    int i = 0, maxnamelength = 0;
    //Check the max namelength of the npcs in the target
    for (int n = 0; n < npcs.size(); n++)
    {
        if(npcs[n]->omz == z && !npcs[n]->marked_for_death)
        {
            if (npcs[n]->is_active())
            {   //Active npcs have different coords. Because Cata hates you!
                if ((g->levx + (npcs[n]->posx / SEEX))/2 == x &&
                    (g->levy + (npcs[n]->posy / SEEY))/2 == y)
                {
                    if (npcs[n]->name.length() > maxnamelength)
                        maxnamelength = npcs[n]->name.length();
                }
            } else if ((npcs[n]->mapx)/2 == x && (npcs[n]->mapy)/2 == y) {
                if (npcs[n]->name.length() > maxnamelength)
                    maxnamelength = npcs[n]->name.length();
            }
        }
    }
    //Check if the target has an npc in it.
    for (int n = 0; n < npcs.size(); n++)
    {
        if (npcs[n]->omz == z && !npcs[n]->marked_for_death)
        {
            if (npcs[n]->is_active())
            {
                if ((g->levx + (npcs[n]->posx / SEEX))/2 == x &&
                    (g->levy + (npcs[n]->posy / SEEY))/2 == y)
                {
                    mvwprintz(w, i, 0, c_yellow, npcs[n]->name.c_str());
                    for (int j = npcs[n]->name.length(); j < maxnamelength; j++)
                        mvwputch(w, i, j, c_black, LINE_XXXX);
                    i++;
                }
            } else if ((npcs[n]->mapx)/2 == x && (npcs[n]->mapy)/2 == y)
            {
                mvwprintz(w, i, 0, c_yellow, npcs[n]->name.c_str());
                for (int j = npcs[n]->name.length(); j < maxnamelength; j++)
                    mvwputch(w, i, j, c_black, LINE_XXXX);
                i++;
            }
        }
    }
    for (int j = 0; j < i; j++)
        mvwputch(w, j, maxnamelength, c_white, LINE_XOXO);
    for (int j = 0; j < maxnamelength; j++)
        mvwputch(w, i, j, c_white, LINE_OXOX);
    mvwputch(w, i, maxnamelength, c_white, LINE_XOOX);
}

void overmap::print_vehicles(WINDOW *w, int const x, int const y, int const z)
{
    if (!z==0) // vehicles only exist on zlevel 0
        return;
    int i = 0, maxnamelength = 0;
    //Check the max namelength of the vehicles in the target
    for (std::map<int, om_vehicle>::const_iterator it = vehicles.begin();
         it != vehicles.end(); it++)
    {
        om_vehicle om_veh = it->second;
        if ( om_veh.x == x && om_veh.y == y )
        {
            if (om_veh.name.length() > maxnamelength)
                maxnamelength = om_veh.name.length();
        }
    }
    //Check if the target has a vehicle in it.
    for (std::map<int, om_vehicle>::const_iterator it = vehicles.begin();
         it != vehicles.end(); it++)
    {
        om_vehicle om_veh = it->second;
        if (om_veh.x == x && om_veh.y == y)
        {
            mvwprintz(w, i, 0, c_cyan, om_veh.name.c_str());
            for (int j = om_veh.name.length(); j < maxnamelength; j++)
                mvwputch(w, i, j, c_black, LINE_XXXX);
            i++;
        }
    }
    for (int j = 0; j < i; j++)
        mvwputch(w, j, maxnamelength, c_white, LINE_XOXO);
    for (int j = 0; j < maxnamelength; j++)
        mvwputch(w, i, j, c_white, LINE_OXOX);
    mvwputch(w, i, maxnamelength, c_white, LINE_XOOX);
}

void overmap::generate(overmap* north, overmap* east, overmap* south,
                       overmap* west)
{
 dbg(D_INFO) << "overmap::generate start...";
 erase();
 clear();
 move(0, 0);
 std::vector<city> road_points; // cities and roads_out together
 std::vector<point> river_start;// West/North endpoints of rivers
 std::vector<point> river_end; // East/South endpoints of rivers

// Determine points where rivers & roads should connect w/ adjacent maps
 const oter_id river_center("river_center"); // optimized comparison.

 if (north != NULL) {
  for (int i = 2; i < OMAPX - 2; i++) {
   if (is_river(north->ter(i,OMAPY-1, 0)))
    ter(i, 0, 0) = river_center;
   if (north->ter(i,     OMAPY - 1, 0) == river_center &&
       north->ter(i - 1, OMAPY - 1, 0) == river_center &&
       north->ter(i + 1, OMAPY - 1, 0) == river_center) {
    if (river_start.size() == 0 ||
        river_start[river_start.size() - 1].x < i - 6)
     river_start.push_back(point(i, 0));
   }
  }
  for (int i = 0; i < north->roads_out.size(); i++) {
   if (north->roads_out[i].y == OMAPY - 1)
    roads_out.push_back(city(north->roads_out[i].x, 0, 0));
  }
 }
 int rivers_from_north = river_start.size();
 if (west != NULL) {
  for (int i = 2; i < OMAPY - 2; i++) {
   if (is_river(west->ter(OMAPX - 1, i, 0)))
    ter(0, i, 0) = river_center;
   if (west->ter(OMAPX - 1, i, 0)     == river_center &&
       west->ter(OMAPX - 1, i - 1, 0) == river_center &&
       west->ter(OMAPX - 1, i + 1, 0) == river_center) {
    if (river_start.size() == rivers_from_north ||
        river_start[river_start.size() - 1].y < i - 6)
     river_start.push_back(point(0, i));
   }
  }
  for (int i = 0; i < west->roads_out.size(); i++) {
   if (west->roads_out[i].x == OMAPX - 1)
    roads_out.push_back(city(0, west->roads_out[i].y, 0));
  }
 }
 if (south != NULL) {
  for (int i = 2; i < OMAPX - 2; i++) {
   if (is_river(south->ter(i, 0, 0)))
    ter(i, OMAPY - 1, 0) = river_center;
   if (south->ter(i,     0, 0) == river_center &&
       south->ter(i - 1, 0, 0) == river_center &&
       south->ter(i + 1, 0, 0) == river_center) {
    if (river_end.size() == 0 ||
        river_end[river_end.size() - 1].x < i - 6)
     river_end.push_back(point(i, OMAPY - 1));
   }
   if (south->ter(i, 0, 0) == "road_nesw")
    roads_out.push_back(city(i, OMAPY - 1, 0));
  }
  for (int i = 0; i < south->roads_out.size(); i++) {
   if (south->roads_out[i].y == 0)
    roads_out.push_back(city(south->roads_out[i].x, OMAPY - 1, 0));
  }
 }
 int rivers_to_south = river_end.size();
 if (east != NULL) {
  for (int i = 2; i < OMAPY - 2; i++) {
   if (is_river(east->ter(0, i, 0)))
    ter(OMAPX - 1, i, 0) = river_center;
   if (east->ter(0, i, 0)     == river_center &&
       east->ter(0, i - 1, 0) == river_center &&
       east->ter(0, i + 1, 0) == river_center) {
    if (river_end.size() == rivers_to_south ||
        river_end[river_end.size() - 1].y < i - 6)
     river_end.push_back(point(OMAPX - 1, i));
   }
   if (east->ter(0, i, 0) == "road_nesw")
    roads_out.push_back(city(OMAPX - 1, i, 0));
  }
  for (int i = 0; i < east->roads_out.size(); i++) {
   if (east->roads_out[i].x == 0)
    roads_out.push_back(city(OMAPX - 1, east->roads_out[i].y, 0));
  }
 }

// Even up the start and end points of rivers. (difference of 1 is acceptable)
// Also ensure there's at least one of each.
 std::vector<point> new_rivers;
 if (north == NULL || west == NULL) {
  while (river_start.empty() || river_start.size() + 1 < river_end.size()) {
   new_rivers.clear();
   if (north == NULL)
    new_rivers.push_back( point(rng(10, OMAPX - 11), 0) );
   if (west == NULL)
    new_rivers.push_back( point(0, rng(10, OMAPY - 11)) );
   river_start.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }
 if (south == NULL || east == NULL) {
  while (river_end.empty() || river_end.size() + 1 < river_start.size()) {
   new_rivers.clear();
   if (south == NULL)
    new_rivers.push_back( point(rng(10, OMAPX - 11), OMAPY - 1) );
   if (east == NULL)
    new_rivers.push_back( point(OMAPX - 1, rng(10, OMAPY - 11)) );
   river_end.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }

// Now actually place those rivers.
 if (river_start.size() > river_end.size() && river_end.size() > 0) {
  std::vector<point> river_end_copy = river_end;
  while (!river_start.empty()) {
   int index = rng(0, river_start.size() - 1);
   if (!river_end.empty()) {
    place_river(river_start[index], river_end[0]);
    river_end.erase(river_end.begin());
   } else
    place_river(river_start[index],
                river_end_copy[rng(0, river_end_copy.size() - 1)]);
   river_start.erase(river_start.begin() + index);
  }
 } else if (river_end.size() > river_start.size() && river_start.size() > 0) {
  std::vector<point> river_start_copy = river_start;
  while (!river_end.empty()) {
   int index = rng(0, river_end.size() - 1);
   if (!river_start.empty()) {
    place_river(river_start[0], river_end[index]);
    river_start.erase(river_start.begin());
   } else
    place_river(river_start_copy[rng(0, river_start_copy.size() - 1)],
                river_end[index]);
   river_end.erase(river_end.begin() + index);
  }
 } else if (river_end.size() > 0) {
  if (river_start.size() != river_end.size())
   river_start.push_back( point(rng(OMAPX * .25, OMAPX * .75),
                                rng(OMAPY * .25, OMAPY * .75)));
  for (int i = 0; i < river_start.size(); i++)
   place_river(river_start[i], river_end[i]);
 }

// Cities and forests come next.
// These're agnostic of adjacent maps, so it's very simple.
 place_cities();
 place_forest();

// Ideally we should have at least two exit points for roads, on different sides
 if (roads_out.size() < 2) {
  std::vector<city> viable_roads;
  int tmp;
// Populate viable_roads with one point for each neighborless side.
// Make sure these points don't conflict with rivers.
// TODO: In theory this is a potential infinte loop...
  if (north == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, 0, 0)) || is_river(ter(tmp - 1, 0, 0)) ||
          is_river(ter(tmp + 1, 0, 0)) );
   viable_roads.push_back(city(tmp, 0, 0));
  }
  if (east == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(OMAPX - 1, tmp, 0)) || is_river(ter(OMAPX - 1, tmp - 1, 0))||
          is_river(ter(OMAPX - 1, tmp + 1, 0)));
   viable_roads.push_back(city(OMAPX - 1, tmp, 0));
  }
  if (south == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, OMAPY - 1, 0)) || is_river(ter(tmp - 1, OMAPY - 1, 0))||
          is_river(ter(tmp + 1, OMAPY - 1, 0)));
   viable_roads.push_back(city(tmp, OMAPY - 1, 0));
  }
  if (west == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(0, tmp, 0)) || is_river(ter(0, tmp - 1, 0)) ||
          is_river(ter(0, tmp + 1, 0)));
   viable_roads.push_back(city(0, tmp, 0));
  }
  while (roads_out.size() < 2 && !viable_roads.empty()) {
   tmp = rng(0, viable_roads.size() - 1);
   roads_out.push_back(viable_roads[tmp]);
   viable_roads.erase(viable_roads.begin() + tmp);
  }
 }

// Compile our master list of roads; it's less messy if roads_out is first
 for (int i = 0; i < roads_out.size(); i++)
  road_points.push_back(roads_out[i]);
 for (int i = 0; i < cities.size(); i++)
  road_points.push_back(cities[i]);
// And finally connect them via "highways"
 place_hiways(road_points, 0, "road");
// Place specials
 place_specials();
// Clean up our roads and rivers
 polish(0);

 // TODO: there is no reason we can't generate the sublevels in one pass
 //       for that matter there is no reason we can't as we add the entrance ways either

 // Always need at least one sublevel, but how many more
 int z = -1;
 bool requires_sub = false;
 do {
        requires_sub = generate_sub(z);
 } while(requires_sub && (--z >= -OVERMAP_DEPTH));

// Place the monsters, now that the terrain is laid out
 place_mongroups();
 place_radios();
 dbg(D_INFO) << "overmap::generate done";
}


bool overmap::generate_sub(int const z)
{
    bool requires_sub = false;
    std::vector<city> subway_points;
    std::vector<city> sewer_points;
    std::vector<city> ant_points;
    std::vector<city> goo_points;
    std::vector<city> lab_points;
    std::vector<city> ice_lab_points;
    std::vector<point> shaft_points;
    std::vector<city> mine_points;
    std::vector<point> bunker_points;
    std::vector<point> shelter_points;
    std::vector<point> lmoe_points;
    std::vector<point> cabin_strange_points;
    std::vector<point> triffid_points;
    std::vector<point> temple_points;
    std::vector<point> office_entrance_points;
    std::vector<point> office_points;
    std::vector<point> prison_points;
    std::vector<point> prison_entrance_points;
    std::vector<point> haz_sar_entrance_points;
    std::vector<point> haz_sar_points;
    std::vector<point> cathedral_entrance_points;
    std::vector<point> cathedral_points;
    std::vector<point> hotel_tower_1_points;
    std::vector<point> hotel_tower_2_points;
    std::vector<point> hotel_tower_3_points;

    // These are so common that it's worth checking first as int.
    const oter_id skip_above[5] = {
        oter_id("rock"), oter_id("forest"), oter_id("field"),
        oter_id("forest_thick"), oter_id("forest_water")
    };

    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            oter_id oter_above = ter(i, j, z + 1);

            // implicitly skip skip_above oter_ids
            bool skipme = false;
            for (int si=0; si < 5; si++) {
                if (oter_above == skip_above[si]) {
                    skipme = true;
                }
            }
            if (skipme) {
                continue;
            }

            if (is_ot_type("house_base", oter_above)) {
                ter(i, j, z) = "basement";
            } else if (is_ot_type("sub_station", oter_above)) {
                ter(i, j, z) = "subway_nesw";
                subway_points.push_back(city(i, j, 0));
            } else if (is_ot_type("prison", oter_above) &&
                       oter_above != "prison_2") {
                prison_points.push_back( point(i, j) );

            } else if (oter_above == "prison_2") {
                prison_entrance_points.push_back( point(i, j) );
            } else if (oter_above == "road_nesw_manhole") {
                ter(i, j, z) = "sewer_nesw";
                sewer_points.push_back(city(i, j, 0));
            } else if (oter_above == "sewage_treatment") {
                for (int x = i-1; x <= i+1; x++) {
                    for (int y = j-1; y <= j+1; y++) {
                        ter(x, y, z) = "sewage_treatment_under";
                    }
                }
                ter(i, j, z) = "sewage_treatment_hub";
                sewer_points.push_back(city(i, j, 0));
            } else if (oter_above == "spider_pit") {
                ter(i, j, z) = "spider_pit_under";
            } else if (oter_above == "cave" && z == -1) {
                if (one_in(3)) {
                    ter(i, j, z) = "cave_rat";
                    requires_sub = true; // rat caves are two level
                } else {
                    ter(i, j, z) = "cave";
                }
            } else if (oter_above == "cave_rat" && z == -2) {
                ter(i, j, z) = "cave_rat";
            } else if (oter_above == "anthill") {
                int size = rng(MIN_ANT_SIZE, MAX_ANT_SIZE);
                ant_points.push_back(city(i, j, size));
                zg.push_back(mongroup("GROUP_ANT", i * 2, j * 2, z, size * 1.5, rng(6000, 8000)));
            } else if (oter_above == "slimepit_down") {
                int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
                goo_points.push_back(city(i, j, size));
            } else if (oter_above == "forest_water") {
                ter(i, j, z) = "cavern";
            } else if (oter_above == "triffid_grove" ||
                       oter_above == "triffid_roots") {
             triffid_points.push_back( point(i, j) );
            } else if (oter_above == "temple_stairs") {
                temple_points.push_back( point(i, j) );
            } else if (oter_above == "lab_core" ||
                       (z == -1 && oter_above == "lab_stairs")) {
                lab_points.push_back(city(i, j, rng(1, 5 + z)));
            } else if (oter_above == "lab_stairs") {
                ter(i, j, z) = "lab";
            } else if (oter_above == "ice_lab_core" ||
                       (z == -1 && oter_above == "ice_lab_stairs")) {
                ice_lab_points.push_back(city(i, j, rng(1, 5 + z)));
            } else if (oter_above == "ice_lab_stairs") {
                ter(i, j, z) = "ice_lab";
            } else if (oter_above == "bunker" && z == -1) {
                bunker_points.push_back( point(i, j) );
            } else if (oter_above == "shelter") {
                shelter_points.push_back( point(i, j) );
            } else if (oter_above == "lmoe") {
                lmoe_points.push_back( point(i, j) );
            } else if (oter_above == "cabin_strange") {
                cabin_strange_points.push_back( point(i, j) );
            } else if (oter_above == "mine_entrance") {
                shaft_points.push_back( point(i, j) );
            } else if (oter_above == "mine_shaft" ||
                       oter_above == "mine_down"    ) {
                ter(i, j, z) = "mine";
                mine_points.push_back(city(i, j, rng(6 + z, 10 + z)));
                // technically not all finales need a sub level,
                // but at this point we don't know
                requires_sub = true;
            } else if (oter_above == "mine_finale") {
                for (int x = i - 1; x <= i + 1; x++) {
                    for (int y = j - 1; y <= j + 1; y++) {
                        ter(x, y, z) = "spiral";
                    }
                }
                ter(i, j, z) = "spiral_hub";
                zg.push_back(mongroup("GROUP_SPIRAL", i * 2, j * 2, z, 2, 200));
            } else if (oter_above == "silo") {
                if (rng(2, 7) < abs(z) || rng(2, 7) < abs(z)) {
                    ter(i, j, z) = "silo_finale";
                } else {
                    ter(i, j, z) = "silo";
                    requires_sub = true;
                }
            } else if (oter_above == "office_tower_1_entrance") {
                office_entrance_points.push_back( point(i, j) );
            } else if (oter_above == "office_tower_1") {
                office_points.push_back( point(i, j) );
            } else if (oter_above == "haz_sar_entrance") {
                haz_sar_entrance_points.push_back( point(i, j) );
            } else if (oter_above == "haz_sar") {
                haz_sar_points.push_back( point(i, j) );
            } else if (oter_above == "cathedral_1_entrance") {
                cathedral_entrance_points.push_back( point(i, j) );
            } else if (oter_above == "cathedral_1") {
                cathedral_points.push_back( point(i, j) );
            } else if (oter_above == "hotel_tower_1_7") {
                hotel_tower_1_points.push_back( point(i, j) );
            } else if (oter_above == "hotel_tower_1_8") {
                hotel_tower_2_points.push_back( point(i, j) );
            } else if (oter_above == "hotel_tower_1_9") {
                hotel_tower_3_points.push_back( point(i, j) );
            }
        }
    }

    for (int i = 0; i < goo_points.size(); i++) {
        requires_sub |= build_slimepit(goo_points[i].x, goo_points[i].y, z, goo_points[i].s);
    }
    place_hiways(sewer_points, z, "sewer");
    polish(z, "sewer");
    place_hiways(subway_points, z, "subway");
    for (int i = 0; i < subway_points.size(); i++) {
        ter(subway_points[i].x, subway_points[i].y, z) = "subway_station";
    }
    for (int i = 0; i < lab_points.size(); i++) {
        bool lab = build_lab(lab_points[i].x, lab_points[i].y, z, lab_points[i].s);
        requires_sub |= lab;
        if (!lab && ter(lab_points[i].x, lab_points[i].y, z) == "lab_core") {
            ter(lab_points[i].x, lab_points[i].y, z) = "lab";
        }
    }
    for (int i = 0; i < ice_lab_points.size(); i++) {
        bool ice_lab = build_ice_lab(ice_lab_points[i].x, ice_lab_points[i].y, z, ice_lab_points[i].s);
        requires_sub |= ice_lab;
        if (!ice_lab && ter(ice_lab_points[i].x, ice_lab_points[i].y, z) == "ice_lab_core") {
            ter(ice_lab_points[i].x, ice_lab_points[i].y, z) = "ice_lab";
        }
    }
    for (int i = 0; i < ant_points.size(); i++) {
        build_anthill(ant_points[i].x, ant_points[i].y, z, ant_points[i].s);
    }
    polish(z, "subway");
    polish(z, "ants");

    for (int i = 0; i < cities.size(); i++) {
        if (one_in(3)) {
            zg.push_back(mongroup("GROUP_CHUD", cities[i].x * 2,
                                  cities[i].y * 2, z, cities[i].s,
                                  cities[i].s * 20));
        }
        if (!one_in(8)) {
            zg.push_back(mongroup("GROUP_SEWER",
                                  cities[i].x * 2, cities[i].y * 2, z,
                                  cities[i].s * 3.5, cities[i].s * 70));
        }
    }

    place_rifts(z);
    for (int i = 0; i < mine_points.size(); i++) {
        build_mine(mine_points[i].x, mine_points[i].y, z, mine_points[i].s);
    }

    for (int i = 0; i < shaft_points.size(); i++) {
        ter(shaft_points[i].x, shaft_points[i].y, z) = "mine_shaft";
        requires_sub = true;
    }

    for (int i = 0; i < bunker_points.size(); i++) {
        ter(bunker_points[i].x, bunker_points[i].y, z) = "bunker";
    }

    for (int i = 0; i < shelter_points.size(); i++) {
        ter(shelter_points[i].x, shelter_points[i].y, z) = "shelter_under";
    }

    for (int i = 0; i < lmoe_points.size(); i++) {
        ter(lmoe_points[i].x, lmoe_points[i].y, z) = "lmoe_under";
    }

    for (int i = 0; i < cabin_strange_points.size(); i++) {
        ter(cabin_strange_points[i].x, cabin_strange_points[i].y, z) = "cabin_strange_b";
    }

    for (int i = 0; i < triffid_points.size(); i++) {
        if (z == -1) {
            ter(triffid_points[i].x, triffid_points[i].y, z) = "triffid_roots";
            requires_sub = true;
        } else {
            ter(triffid_points[i].x, triffid_points[i].y, z) = "triffid_finale";
        }
    }

    for (int i = 0; i < temple_points.size(); i++) {
        if (z == -5) {
            ter(temple_points[i].x, temple_points[i].y, z) = "temple_finale";
        } else {
            ter(temple_points[i].x, temple_points[i].y, z) = "temple_stairs";
            requires_sub = true;
        }
    }
    for (int i = 0; i < office_entrance_points.size(); i++) {
        ter(office_entrance_points[i].x, office_entrance_points[i].y, z) = "office_tower_b_entrance";
    }
    for (int i = 0; i < office_points.size(); i++) {
        ter(office_points[i].x, office_points[i].y, z) = "office_tower_b";
    }
    for (int i = 0; i < prison_points.size(); i++) {
        ter(prison_points[i].x, prison_points[i].y, z) = "prison_b";
    }
    for (int i = 0; i < prison_entrance_points.size(); i++) {
        ter(prison_entrance_points[i].x, prison_entrance_points[i].y, z) = "prison_b_entrance";
    }
    for (int i = 0; i < haz_sar_entrance_points.size(); i++) {
        ter(haz_sar_entrance_points[i].x, haz_sar_entrance_points[i].y, z-1) = "haz_sar_entrance_b1";
    }
    for (int i = 0; i < haz_sar_points.size(); i++) {
        ter(haz_sar_points[i].x, haz_sar_points[i].y, z-1) = "haz_sar_b1";
    }
    for (int i = 0; i < cathedral_entrance_points.size(); i++) {
        ter(cathedral_entrance_points[i].x, cathedral_entrance_points[i].y, z) = "cathedral_b_entrance";
    }
    for (int i = 0; i < cathedral_points.size(); i++) {
        ter(cathedral_points[i].x, cathedral_points[i].y, z) = "cathedral_b";
    }
    for (int i = 0; i < hotel_tower_1_points.size(); i++) {
        ter(hotel_tower_1_points[i].x, hotel_tower_1_points[i].y, z) = "hotel_tower_b_1";
    }
    for (int i = 0; i < hotel_tower_2_points.size(); i++) {
        ter(hotel_tower_2_points[i].x, hotel_tower_2_points[i].y, z) = "hotel_tower_b_2";
    }
    for (int i = 0; i < hotel_tower_3_points.size(); i++) {
        ter(hotel_tower_3_points[i].x, hotel_tower_3_points[i].y, z) = "hotel_tower_b_3";
    }
    return requires_sub;
}

void overmap::make_tutorial()
{
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            ter(i, j, -1) = "rock";
        }
    }
    ter(50, 50, 0) = "tutorial";
    ter(50, 50, -1) = "tutorial";
    zg.clear();
}

point overmap::find_closest(point origin, const std::string &type,
                            int &dist, bool must_be_seen)
{
    //does origin qualify?
    if (check_ot_type(type, origin.x, origin.y, 0)) {
        if (!must_be_seen || seen(origin.x, origin.y, 0)) {
            return point(origin.x, origin.y);
        }
    }

    int max = (dist == 0 ? OMAPX : dist);
    // expanding box
    for (dist = 0; dist <= max; dist++) {
        // each edge length is 2*dist-2, because corners belong to one edge
        // south is +y, north is -y
        for (int i = 0; i < dist*2-1; i++) {
            //start at northwest, scan north edge
            int x = origin.x - dist + i;
            int y = origin.y - dist;
            if (check_ot_type(type, x, y, 0)) {
                if (!must_be_seen || seen(x, y, 0)) {
                    return point(x, y);
                }
            }

            //start at southeast, scan south
            x = origin.x + dist - i;
            y = origin.y + dist;
            if (check_ot_type(type, x, y, 0)) {
                if (!must_be_seen || seen(x, y, 0)) {
                    return point(x, y);
                }
            }

            //start at southwest, scan west
            x = origin.x - dist;
            y = origin.y + dist - i;
            if (check_ot_type(type, x, y, 0)) {
                if (!must_be_seen || seen(x, y, 0)) {
                    return point(x, y);
                }
            }

            //start at northeast, scan east
            x = origin.x + dist;
            y = origin.y - dist + i;
            if (check_ot_type(type, x, y, 0)) {
                if (!must_be_seen || seen(x, y, 0)) {
                    return point(x, y);
                }
            }
        }
    }

    dist = -1;
    return point(-1, -1);
}

std::vector<point> overmap::find_all(tripoint origin, const std::string &type,
                                     int &dist, bool must_be_seen)
{
    std::vector<point> res;
    int max = (dist == 0 ? OMAPX / 2 : dist);
    for (dist = 0; dist <= max; dist++) {
        for (int x = origin.x - dist; x <= origin.x + dist; x++) {
            for (int y = origin.y - dist; y <= origin.y + dist; y++) {
                if (check_ot_type(type, x, y, origin.z)
                        && (!must_be_seen || seen(x, y, origin.z))) {
                    res.push_back(point(x, y));
                }
            }
        }
    }
    return res;
}

std::vector<point> overmap::find_terrain(const std::string &term, int zlevel)
{
    std::vector<point> found;
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            if (seen(x, y, zlevel) && otermap[ter(x, y, zlevel)].name.find(term) != std::string::npos) {
                found.push_back( point(x, y) );
            }
        }
    }
    return found;
}

int overmap::closest_city(point p)
{
 int distance = 999, ret = -1;
 for (int i = 0; i < cities.size(); i++) {
  int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
  if (dist < distance || (dist == distance && cities[i].s < cities[ret].s)) {
   ret = i;
   distance = dist;
  }
 }

 return ret;
}

point overmap::random_house_in_city(int city_id)
{
    if (city_id < 0 || city_id >= cities.size()) {
        debugmsg("overmap::random_house_in_city(%d) (max %d)", city_id,
                 cities.size() - 1);
        return point(-1, -1);
    }

    std::vector<point> valid;
    int startx = cities[city_id].x - cities[city_id].s;
    int endx   = cities[city_id].x + cities[city_id].s;
    int starty = cities[city_id].y - cities[city_id].s;
    int endy   = cities[city_id].y + cities[city_id].s;
    for (int x = startx; x <= endx; x++) {
        for (int y = starty; y <= endy; y++) {
            if (check_ot_type("house", x, y, 0)) {
                valid.push_back( point(x, y) );
            }
        }
    }
    if (valid.empty()) {
        return point(-1, -1);
    }

    return valid[ rng(0, valid.size() - 1) ];
}

int overmap::dist_from_city(point p)
{
 int distance = 999;
 for (int i = 0; i < cities.size(); i++) {
  int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
  dist -= cities[i].s;
  if (dist < distance)
   distance = dist;
 }
 return distance;
}

void overmap::draw(WINDOW *w, int z, int &cursx, int &cursy,
                   int &origx, int &origy, signed char &ch, bool blink,
                   overmap &hori, overmap &vert, overmap &diag, input_context* inp_ctxt)
{
 bool note_here = false, npc_here = false, veh_here = false;
 std::string note_text;
 int om_map_width = TERMX-28;
 int om_map_height = TERMY;

 int omx, omy;
 point target(-1, -1);
 if (g->u.active_mission >= 0 &&
     g->u.active_mission < g->u.active_missions.size())
  target = g->find_mission(g->u.active_missions[g->u.active_mission])->target;
  bool see;
  oter_id cur_ter = ot_null;
  nc_color ter_color;
  long ter_sym;
  /* First, determine if we're close enough to the edge to need an
   * adjacent overmap, and record the offsets. */
  int offx = 0;
  int offy = 0;
  if (cursx < om_map_width / 2)
  {
      offx = -1;
  }
  else if (cursx > OMAPX - 2 - (om_map_width / 2))
  {
      offx = 1;
  }
  if (cursy < (om_map_height / 2))
  {
      offy = -1;
  }
  else if (cursy > OMAPY - 2 - (om_map_height / 2))
  {
      offy = 1;
  }

  // If the offsets don't match the previously loaded ones, load the new adjacent overmaps.
  if( offx && loc.x + offx != hori.loc.x )
  {
      hori = overmap_buffer.get( loc.x + offx, loc.y );
  }
  if( offy && loc.y + offy != vert.loc.y )
  {
      vert = overmap_buffer.get( loc.x, loc.y + offy );
  }
  if( offx && offy && (loc.x + offx != diag.loc.x || loc.y + offy != diag.loc.y ) )
  {
      diag = overmap_buffer.get( loc.x + offx, loc.y + offy );
  }

// Now actually draw the map
  bool csee = false;
  oter_id ccur_ter = "";
  for (int i = -(om_map_width / 2); i < (om_map_width / 2); i++) {
   for (int j = -(om_map_height / 2);
         j <= (om_map_height / 2) + (ch == 'j' ? 1 : 0); j++) {
    omx = cursx + i;
    omy = cursy + j;
    see = false;
    npc_here = false;
    veh_here = false;
    if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) { // It's in-bounds
     cur_ter = ter(omx, omy, z);
     see = seen(omx, omy, z);
     note_here = has_note(omx, omy, z);
     if (note_here) {
         note_text = note(omx, omy, z);
     }
     //Check if there is an npc.
     npc_here = has_npc(omx,omy,z);
     // and a vehicle
     veh_here = has_vehicle(omx,omy,z);
// <Out of bounds placement>
    } else if (omx < 0) {
     omx += OMAPX;
     if (omy < 0 || omy >= OMAPY) {
      omy += (omy < 0 ? OMAPY : 0 - OMAPY);
      cur_ter = diag.ter(omx, omy, z);
      see = diag.seen(omx, omy, z);
      veh_here = diag.has_vehicle(omx, omy, z);
      note_here = diag.has_note(omx, omy, z);
      if (note_here) {
          note_text = diag.note(omx, omy, z);
      }
     } else {
      cur_ter = hori.ter(omx, omy, z);
      see = hori.seen(omx, omy, z);
      veh_here = hori.has_vehicle(omx, omy, z);
      note_here = hori.has_note(omx, omy, z);
      if (note_here) {
          note_text = hori.note(omx, omy, z);
      }
     }
    } else if (omx >= OMAPX) {
     omx -= OMAPX;
     if (omy < 0 || omy >= OMAPY) {
      omy += (omy < 0 ? OMAPY : 0 - OMAPY);
      cur_ter = diag.ter(omx, omy, z);
      see = diag.seen(omx, omy, z);
      veh_here = diag.has_vehicle(omx, omy, z);
      note_here = diag.has_note(omx, omy, z);
      if (note_here) {
          note_text = diag.note(omx, omy, z);
      }
     } else {
      cur_ter = hori.ter(omx, omy, z);
      see = hori.seen(omx, omy, z);
      veh_here = hori.has_vehicle(omx, omy, z);
      note_here = hori.has_note(omx, omy, z);
      if (note_here) {
          note_text = hori.note(omx, omy, z);
      }
     }
    } else if (omy < 0) {
     omy += OMAPY;
     cur_ter = vert.ter(omx, omy, z);
     see = vert.seen(omx, omy, z);
     veh_here = vert.has_vehicle(omx, omy, z);
     note_here = vert.has_note(omx, omy, z);
     if (note_here) {
         note_text = vert.note(omx, omy, z);
     }
    } else if (omy >= OMAPY) {
     omy -= OMAPY;
     cur_ter = vert.ter(omx, omy, z);
     see = vert.seen(omx, omy, z);
     veh_here = vert.has_vehicle(omx, omy, z);
     note_here = vert.has_note(omx, omy, z);
     if (note_here) {
         note_text = vert.note(omx, omy, z);
     }
    } else {
        debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
    }
// </Out of bounds replacement>
    if (see) {
     if (note_here && blink) {
      ter_color = c_yellow;
      if (note_text[1] == ':') {
       ter_sym = note_text[0];
      } else {
       ter_sym = 'N';
      }
     } else if (omx == origx && omy == origy && blink) {
      ter_color = g->u.color();
      ter_sym = '@';
     } else if (npc_here && blink) {
      ter_color = c_pink;
      ter_sym = '@';
     } else if (veh_here && blink) {
         ter_color = c_cyan;
         ter_sym = 'c';
     } else if (omx == target.x && omy == target.y && blink) {
      ter_color = c_red;
      ter_sym = '*';
     } else {
        if (otermap.find(cur_ter) == otermap.end()) {
            debugmsg("Bad ter %s (%d, %d)", cur_ter.c_str(), omx, omy);
        }
        ter_color = otermap[cur_ter].color;
        ter_sym = otermap[cur_ter].sym;
     }
    } else { // We haven't explored this tile yet
     ter_color = c_dkgray;
     ter_sym = '#';
    }
    if (j == 0 && i == 0) {
     mvwputch_hi (w, om_map_height / 2, om_map_width / 2,
                  ter_color, ter_sym);
     csee = see;
     ccur_ter = cur_ter;
    } else {
        mvwputch(w, (om_map_height / 2) + j, (om_map_width / 2) + i,
                 ter_color, ter_sym);
    }
   }
  }
  if (target.x != -1 && target.y != -1 && blink &&
      (target.x < cursx - om_map_height / 2 ||
                  target.x > cursx + om_map_height / 2  ||
       target.y < cursy - om_map_width / 2 ||
                  target.y > cursy + om_map_width / 2    )) {
    switch (direction_from(cursx, cursy, target.x, target.y)) {
    case NORTH:      mvwputch(w, 0, (om_map_width / 2), c_red, '^');       break;
    case NORTHEAST:  mvwputch(w, 0, om_map_width - 1, c_red, LINE_OOXX); break;
    case EAST:       mvwputch(w, (om_map_height / 2),
                                    om_map_width - 1, c_red, '>');       break;
    case SOUTHEAST:  mvwputch(w, om_map_height,
                                    om_map_width - 1, c_red, LINE_XOOX); break;
    case SOUTH:      mvwputch(w, om_map_height,
                                    om_map_height / 2, c_red, 'v');       break;
    case SOUTHWEST:  mvwputch(w, om_map_height,  0, c_red, LINE_XXOO); break;
    case WEST:       mvwputch(w, om_map_height / 2,  0, c_red, '<');       break;
    case NORTHWEST:  mvwputch(w,  0,  0, c_red, LINE_OXXO); break;
   }
  }

  if (has_note(cursx, cursy, z)) {
   note_text = note(cursx, cursy, z);
   if (note_text[1] == ':')
    note_text = note_text.substr(2, note_text.size());
   for (int i = 0; i < note_text.length(); i++)
    mvwputch(w, 1, i, c_white, LINE_OXOX);
   mvwputch(w, 1, note_text.length(), c_white, LINE_XOOX);
   mvwputch(w, 0, note_text.length(), c_white, LINE_XOXO);
   mvwprintz(w, 0, 0, c_yellow, note_text.c_str());
  } else if (has_npc(cursx, cursy, z))
    {
        print_npcs(w, cursx, cursy, z);
    } else if (has_vehicle(cursx, cursy, z))
    {
        print_vehicles(w, cursx, cursy, z);
    }


  cur_ter = ter(cursx, cursy, z);
// Draw the vertical line
  for (int j = 0; j < om_map_height; j++)
   mvwputch(w, j, om_map_width, c_white, LINE_XOXO);
// Clear the legend
  for (int i = om_map_width + 1; i < om_map_width + 55; i++) {
   for (int j = 0; j < om_map_height; j++)
   mvwputch(w, j, i, c_black, ' ');
  }

  real_coords rc;
  rc.fromomap( pos().x, pos().y, cursx, cursy );

  if (csee) {
   mvwputch(w, 1, om_map_width + 1, otermap[ccur_ter].color, otermap[ccur_ter].sym);
   std::vector<std::string> name = foldstring(otermap[ccur_ter].name,25);
   for (int i = 1; (i - 1) < name.size(); i++)
   {
       mvwprintz(w, i, om_map_width + 3, otermap[ccur_ter].color, "%s",
                 name[i-1].c_str());
   }
  } else
   mvwprintz(w, 1, om_map_width + 1, c_dkgray, _("# Unexplored"));

  if (target.x != -1 && target.y != -1) {
   int distance = rl_dist(origx, origy, target.x, target.y);
   mvwprintz(w, 3, om_map_width + 1, c_white, _("Distance to target: %d"), distance);
  }
  mvwprintz(w, 15, om_map_width + 1, c_magenta, _("Use movement keys to pan.  "));
  mvwprintz(w, 16, om_map_width + 1, c_magenta, (inp_ctxt->get_desc("CENTER") +
                                                 _(" - Center map on character")).c_str());
  mvwprintz(w, 17, om_map_width + 1, c_magenta, (inp_ctxt->get_desc("SEARCH") +
                                                 _(" - Search                 ")).c_str());
  mvwprintz(w, 18, om_map_width + 1, c_magenta, (inp_ctxt->get_desc("CREATE_NOTE") +
                                                 _(" - Add/Edit a note        ")).c_str());
  mvwprintz(w, 19, om_map_width + 1, c_magenta, (inp_ctxt->get_desc("DELETE_NOTE") +
                                                 _(" - Delete a note          ")).c_str());
  mvwprintz(w, 20, om_map_width + 1, c_magenta, (inp_ctxt->get_desc("LIST_NOTES") +
                                                 _(" - List notes             ")).c_str());
  fold_and_print(w, 21, om_map_width + 1, 27, c_magenta, (inp_ctxt->get_desc("QUIT") +
                                                          _(" - Return to game  ")).c_str());
  mvwprintz(w, getmaxy(w)-1, om_map_width + 1, c_red, string_format(_("LEVEL %i"),z).c_str());
  mvwprintz( w, getmaxy(w) - 1, om_map_width + 1, c_red, "%s, %d'%d, %d'%d",
                  string_format(_("LEVEL %i"),z).c_str(), rc.abs_om.x, rc.om_pos.x,
                  rc.abs_om.y, rc.om_pos.y );
// Done with all drawing!
  wrefresh(w);
}

//Start drawing the overmap on the screen using the (m)ap command.
point overmap::draw_overmap(int zlevel)
{
 WINDOW* w_map = newwin(TERMY, TERMX, 0, 0);
 WINDOW* w_search = newwin(13, 27, 3, TERMX-27);
 timeout(BLINK_SPEED); // Enable blinking!
 bool blink = true;
 int cursx = (g->levx + int(MAPSIZE / 2)) / 2,
     cursy = (g->levy + int(MAPSIZE / 2)) / 2;
 int origx = cursx, origy = cursy, origz = zlevel;
 signed char ch = 0;
 point ret(-1, -1);
 overmap hori, vert, diag; // Adjacent maps

 // Configure input context for navigating the map.
 input_context ictxt("OVERMAP");
 ictxt.register_action("ANY_INPUT");
 ictxt.register_directions();
 ictxt.register_action("CONFIRM");
 ictxt.register_action("LEVEL_UP");
 ictxt.register_action("LEVEL_DOWN");
 ictxt.register_action("HELP_KEYBINDINGS");

 // Actions whose keys we want to display.
 ictxt.register_action("CENTER");
 ictxt.register_action("CREATE_NOTE");
 ictxt.register_action("DELETE_NOTE");
 ictxt.register_action("SEARCH");
 ictxt.register_action("LIST_NOTES");
 ictxt.register_action("QUIT");
 std::string action;
 do {
     real_coords rc;
     rc.fromomap(g->cur_om->pos().x, g->cur_om->pos().y, cursx, cursy);
     // (cursx, cursy) are the coordinates of the overmap-terrain,
     // that is in the center of the view (relative to this overmap)
     // Those coordinates get translated to the coordinates of the
     // overmap they are on (rc.abs_om) and the coordinates of the
     // overmap-terrain on that overmap (rc.om_pos)
     overmap &center_om = overmap_buffer.get(rc.abs_om.x, rc.abs_om.y);

     center_om.draw(w_map, zlevel, rc.om_pos.x, rc.om_pos.y, origx, origy, ch, blink, hori, vert, diag, &ictxt);
     action = ictxt.handle_input();
     timeout(BLINK_SPEED); // Enable blinking!

  int dirx, diry;
  if (action != "ANY_INPUT") {
   blink = true; // If any input is detected, make the blinkies on
  }
  ictxt.get_direction(dirx, diry, action);
  if (dirx != -2 && diry != -2) {
   cursx += dirx;
   cursy += diry;
  } else if (action == "CENTER") {
   cursx = origx;
   cursy = origy;
   zlevel = origz;
  } else if (action == "LEVEL_DOWN" && zlevel > -OVERMAP_DEPTH) {
      zlevel -= 1;
  } else if (action == "LEVEL_UP" && zlevel < OVERMAP_HEIGHT) {
      zlevel += 1;
  }
  else if (action == "CONFIRM")
   ret = point(cursx, cursy);
  else if (action == "QUIT")
   ret = point(-1, -1);
  else if (action == "CREATE_NOTE") {
   timeout(-1);
   const std::string old_note = center_om.note(rc.om_pos.x, rc.om_pos.y, zlevel);
   const std::string new_note = string_input_popup(_("Note (X:TEXT for custom symbol):"), 45, old_note); // 45 char max
   if(old_note != new_note) {
     center_om.add_note(rc.om_pos.x, rc.om_pos.y, zlevel, new_note);
   }
   timeout(BLINK_SPEED);
  } else if(action == "DELETE_NOTE"){
   timeout(-1);
   if (center_om.has_note(rc.om_pos.x, rc.om_pos.y, zlevel)){
    bool res = query_yn(_("Really delete note?"));
    if (res == true)
     center_om.delete_note(rc.om_pos.x, rc.om_pos.y, zlevel);
   }
   timeout(BLINK_SPEED);
  } else if (action == "LIST_NOTES"){
   timeout(-1);
   point p = center_om.display_notes(zlevel);
   if (p.x != -1) {
    // Translate coords relative to center_om back to relative to this
    real_coords rct;
    rct.fromomap(center_om.pos().x, center_om.pos().y, p.x, p.y);
    cursx = rct.abs_pos.x / (2 * SEEX);
    cursy = rct.abs_pos.y / (2 * SEEX);
   }
   timeout(BLINK_SPEED);
   wrefresh(w_map);
  } else if (action == "SEARCH") {
   int tmpx = cursx, tmpy = cursy;
   timeout(-1);
   std::string term = string_input_popup(_("Search term:"));
   if(term.empty()) {
    continue;
   }
   timeout(BLINK_SPEED);
   center_om.draw(w_map, zlevel, rc.om_pos.x, rc.om_pos.y, origx, origy, ch, blink, hori, vert, diag, &ictxt);
   point found = center_om.find_note(rc.om_pos.x, rc.om_pos.y, zlevel, term);
   if (found.x == -1) { // Didn't find a note
    std::vector<point> terlist;
    terlist = center_om.find_terrain(term, zlevel);
    if (terlist.size() != 0){
     int i = 0;
     //Navigate through results
     do {
      //Draw search box
      draw_border(w_search);
      mvwprintz(w_search, 1, 1, c_red, _("Find place:"));
      mvwprintz(w_search, 2, 1, c_ltblue, "                         ");
      mvwprintz(w_search, 2, 1, c_ltblue, "%s", term.c_str());
      mvwprintz(w_search, 4, 1, c_white,
       _("'<' '>' Cycle targets."));
      mvwprintz(w_search, 10, 1, c_white, _("Enter/Spacebar to select."));
      mvwprintz(w_search, 11, 1, c_white, _("q or ESC to return."));
      ch = input();
      if (ch == ERR)
       blink = !blink;
      else if (ch == '<') {
       i++;
       if(i > terlist.size() - 1)
        i = 0;
      } else if(ch == '>'){
       i--;
       if(i < 0)
        i = terlist.size() - 1;
      }
      rc.om_pos.x = terlist[i].x;
      rc.om_pos.y = terlist[i].y;
      center_om.draw(w_map, zlevel, rc.om_pos.x, rc.om_pos.y, origx, origy, ch, blink, hori, vert, diag, &ictxt);
      wrefresh(w_search);
      timeout(BLINK_SPEED);
     } while(ch != '\n' && ch != ' ' && ch != 'q' && ch != KEY_ESCAPE);
     //If q is hit, return to the last position
     if(ch == 'q' || ch == KEY_ESCAPE){
      cursx = tmpx;
      cursy = tmpy;
     } else {
      // Translate coords relative to center_om back to relative to this
      real_coords rct;
      rct.fromomap(center_om.pos().x, center_om.pos().y, rc.om_pos.x, rc.om_pos.y);
      cursx = rct.abs_pos.x / (2 * SEEX);
      cursy = rct.abs_pos.y / (2 * SEEX);
     }
     ch = '.';
    }
   }
   if (found.x != -1) {
    // Translate coords relative to center_om back to relative to this
    real_coords rct;
    rct.fromomap(center_om.pos().x, center_om.pos().y, found.x, found.y);
    cursx = rct.abs_pos.x / (2 * SEEX);
    cursy = rct.abs_pos.y / (2 * SEEX);
   }
  }
  else if (action == "ANY_INPUT") { // Hit timeout on input, so make characters blink
   blink = !blink;
  }
 } while (action != "QUIT" && action != "CONFIRM");
 timeout(-1);
 werase(w_map);
 wrefresh(w_map);
 delwin(w_map);
 werase(w_search);
 wrefresh(w_search);
 delwin(w_search);
 erase();
 g->refresh_all();
 return ret;
}

void overmap::first_house(int &x, int &y)
{
    std::vector<point> valid;
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            if (ter(i, j, 0) == "shelter") {
                valid.push_back( point(i, j) );
            }
        }
    }
    if (valid.size() == 0) {
        debugmsg("Couldn't find a shelter!");
        x = 1;
        y = 1;
        return;
    }
    int index = rng(0, valid.size() - 1);
    x = valid[index].x;
    y = valid[index].y;
}

void overmap::process_mongroups()
{
 for (int i = 0; i < zg.size(); i++) {
  if (zg[i].dying) {
   zg[i].population *= .8;
   zg[i].radius *= .9;
  }
 }
}

void grow_forest_oter_id(oter_id & oid, bool swampy)
{
    if (swampy && ( oid == ot_field || oid == ot_forest ) ) {
        oid = ot_forest_water;
    } else if ( oid == ot_forest ) {
        oid = ot_forest_thick;
    } else if ( oid == ot_field ) {
        oid = ot_forest;
    }
}

void overmap::place_forest()
{

 for (int i = 0; i < settings.num_forests; i++) {
  // forx and fory determine the epicenter of the forest
  int forx = rng(0, OMAPX - 1);
  int fory = rng(0, OMAPY - 1);
  // fors determinds its basic size
  int fors = rng(settings.forest_size_min, settings.forest_size_max);
  int outer_tries = 1000;
  int inner_tries = 1000;
  for (int j = 0; j < cities.size(); j++) {
      inner_tries = 1000;
      while (dist(forx,fory,cities[j].x,cities[j].y) - fors / 2 < cities[j].s ) {
          // Set forx and fory far enough from cities
          forx = rng(0, OMAPX - 1);
          fory = rng(0, OMAPY - 1);
          // Set fors to determine the size of the forest; usually won't overlap w/ cities
          fors = rng(settings.forest_size_min, settings.forest_size_max);
          j = 0;
          if( 0 == --inner_tries ) { break; }
      }
      if( 0 == --outer_tries || 0 == inner_tries ) {
          break;
      }
  }

        if( 0 == outer_tries || 0 == inner_tries ) {
            break;
        }

        int swamps = settings.swamp_maxsize; // How big the swamp may be...
        int x = forx;
        int y = fory;

        // Depending on the size on the forest...
        for (int j = 0; j < fors; j++) {
            int swamp_chance = 0;
            for (int k = -2; k <= 2; k++) {
                for (int l = -2; l <= 2; l++) {
                    if (ter(x + k, y + l, 0) == "forest_water" ||
                        check_ot_type("river", x+k, y+l, 0)) {
                        swamp_chance += settings.swamp_river_influence;
                    }
                }
            }
            bool swampy = false;
            if (swamps > 0 && swamp_chance > 0 && !one_in(swamp_chance) &&
                (ter(x, y, 0) == "forest" || ter(x, y, 0) == "forest_thick" ||
                ter(x, y, 0) == "field" || one_in( settings.swamp_spread_chance ))) {
                // ...and make a swamp.
                ter(x, y, 0) = "forest_water";
                swampy = true;
                swamps--;
            } else if (swamp_chance == 0) {
                swamps = settings.swamp_maxsize;
            }

            // Place or embiggen forest
            for ( int mx = -1; mx < 2; mx++ ) {
                for ( int my = -1; my < 2; my++ ) {
                    grow_forest_oter_id( ter(x+mx, y+my, 0), ( mx == 0 && my == 0 ? false : swampy ) );
                }
            }
            // Random walk our forest
            x += rng(-2, 2);
            if (x < 0    ) { x = 0; }
            if (x > OMAPX) { x = OMAPX; }
            y += rng(-2, 2);
            if (y < 0    ) { y = 0; }
            if (y > OMAPY) { y = OMAPY; }
        }
    }
}

void overmap::place_river(point pa, point pb)
{
 int x = pa.x, y = pa.y;
 do {
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (y+i >= 0 && y+i < OMAPY && x+j >= 0 && x+j < OMAPX)
     ter(x+j, y+i, 0) = "river_center";
   }
  }
  if (pb.x > x && (rng(0, int(OMAPX * 1.2) - 1) < pb.x - x ||
                   (rng(0, int(OMAPX * .2) - 1) > pb.x - x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x++;
  if (pb.x < x && (rng(0, int(OMAPX * 1.2) - 1) < x - pb.x ||
                   (rng(0, int(OMAPX * .2) - 1) > x - pb.x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x--;
  if (pb.y > y && (rng(0, int(OMAPY * 1.2) - 1) < pb.y - y ||
                   (rng(0, int(OMAPY * .2) - 1) > pb.y - y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y++;
  if (pb.y < y && (rng(0, int(OMAPY * 1.2) - 1) < y - pb.y ||
                   (rng(0, int(OMAPY * .2) - 1) > y - pb.y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y--;
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
// We don't want our riverbanks touching the edge of the map for many reasons
    if ((y+i >= 1 && y+i < OMAPY - 1 && x+j >= 1 && x+j < OMAPX - 1) ||
// UNLESS, of course, that's where the river is headed!
        (abs(pb.y - (y+i)) < 4 && abs(pb.x - (x+j)) < 4))
     ter(x+j, y+i, 0) = "river_center";
   }
  }
 } while (pb.x != x || pb.y != y);
}

/*: the root is overmap::place_cities()
20:50 <kevingranade>: which is at overmap.cpp:1355 or so
20:51 <kevingranade>: the key is cs = rng(4, 17), setting the "size" of the city
20:51 <kevingranade>: which is roughly it's radius in overmap tiles
20:52 <kevingranade>: then later overmap::place_mongroups() is called
20:52 <kevingranade>: which creates a mongroup with radius city_size * 2.5 and population city_size * 80
20:53 <kevingranade>: tadaa

spawns happen at... <cue Clue music>
20:56 <kevingranade>: game:pawn_mon() in game.cpp:7380*/
void overmap::place_cities()
{
 int NUM_CITIES = dice(4, 4);
 int start_dir;
 int op_city_size = int(ACTIVE_WORLD_OPTIONS["CITY_SIZE"]);
 // Limit number of cities based on average size.
 NUM_CITIES = std::min(NUM_CITIES, int(256 / op_city_size * op_city_size));

 // Generate a list of random cities in accordance with village/town/city rules.
 int village_size = std::max(op_city_size - 2, 1);
 int town_min = std::max(op_city_size - 1, 1);
 int town_max = op_city_size + 1;
 int city_size = op_city_size + 3;

 while (cities.size() < NUM_CITIES) {
  int cx = rng(12, OMAPX - 12);
  int cy = rng(12, OMAPY - 12);
  int size = dice(town_min, town_max);
  if (one_in(6)) {
    size = city_size;
  }
  else if (one_in(3)) {
    size = village_size;
  }
  if (ter(cx, cy, 0) == settings.default_oter ) {
   ter(cx, cy, 0) = "road_nesw";
   city tmp; tmp.x = cx; tmp.y = cy; tmp.s = size;
   cities.push_back(tmp);
   start_dir = rng(0, 3);
   for (int j = 0; j < 4; j++)
    make_road(cx, cy, size, (start_dir + j) % 4, tmp);
  }
 }
}

void overmap::put_buildings(int x, int y, int dir, city town)
{
 int ychange = dir % 2, xchange = (dir + 1) % 2;
 for (int i = -1; i <= 1; i += 2) {
  if ((ter(x+i*xchange, y+i*ychange, 0) == settings.default_oter ) && !one_in(STREETCHANCE)) {
   if (rng(0, 99) > 80 * dist(x,y,town.x,town.y) / town.s)
    ter(x+i*xchange, y+i*ychange, 0) = shop( ((dir % 2)-i) % 4, settings.city_spec.shops );
   else {
    if (rng(0, 99) > 130 * dist(x, y, town.x, town.y) / town.s)
     ter(x+i*xchange, y+i*ychange, 0) = shop( ((dir % 2)-i) % 4, settings.city_spec.parks );
    else
     ter(x+i*xchange, y+i*ychange, 0) = house( ((dir % 2)-i) % 4, settings.house_basement_chance );
   }
  }
 }
}

void overmap::make_road(int cx, int cy, int cs, int dir, city town)
{
    int x = cx, y = cy;
    int c = cs, croad = cs;
    switch (dir) {
    case 0:
        while (c > 0 && y > 0 && (ter(x, y-1, 0) == settings.default_oter || c == cs)) {
            y--;
            c--;
            ter(x, y, 0) = "road_ns";
            for (int i = -1; i <= 0; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (abs(j) != abs(i) && (ter(x+j, y+i, 0) == "road_ew" ||
                                             ter(x+j, y+i, 0) == "road_ns")) {
                        ter(x, y, 0) = "road_null";
                        c = -1;
                    }
                }
            }
            put_buildings(x, y, dir, town);
            if (c < croad - 1 && c >= 2 && ter(x - 1, y, 0) == settings.default_oter &&
                                           ter(x + 1, y, 0) == settings.default_oter) {
                croad = c;
                make_road(x, y, cs - rng(1, 3), 1, town);
                make_road(x, y, cs - rng(1, 3), 3, town);
            }
        }
        if (is_road(x, y-2, 0)) {
            ter(x, y-1, 0) = "road_ns";
        }
        break;
    case 1:
        while (c > 0 && x < OMAPX-1 && (ter(x+1, y, 0) == settings.default_oter || c == cs)) {
            x++;
            c--;
            ter(x, y, 0) = "road_ew";
            for (int i = -1; i <= 1; i++) {
                for (int j = 0; j <= 1; j++) {
                    if (abs(j) != abs(i) && (ter(x+j, y+i, 0) == "road_ew" ||
                                             ter(x+j, y+i, 0) == "road_ns")) {
                        ter(x, y, 0) = "road_null";
                        c = -1;
                    }
                }
            }
            put_buildings(x, y, dir, town);
            if (c < croad-2 && c >= 3 && ter(x, y-1, 0) == settings.default_oter &&
                                         ter(x, y+1, 0) == settings.default_oter) {
                croad = c;
                make_road(x, y, cs - rng(1, 3), 0, town);
                make_road(x, y, cs - rng(1, 3), 2, town);
            }
        }
        if (is_road(x-2, y, 0)) {
            ter(x-1, y, 0) = "road_ew";
        }
        break;
    case 2:
        while (c > 0 && y < OMAPY-1 && (ter(x, y+1, 0) == settings.default_oter || c == cs)) {
            y++;
            c--;
            ter(x, y, 0) = "road_ns";
            for (int i = 0; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (abs(j) != abs(i) && (ter(x+j, y+i, 0) == "road_ew" ||
                                             ter(x+j, y+i, 0) == "road_ns")) {
                        ter(x, y, 0) = "road_null";
                        c = -1;
                    }
                }
            }
            put_buildings(x, y, dir, town);
            if (c < croad-2 && ter(x-1, y, 0) == settings.default_oter && ter(x+1, y, 0) == settings.default_oter) {
                croad = c;
                make_road(x, y, cs - rng(1, 3), 1, town);
                make_road(x, y, cs - rng(1, 3), 3, town);
            }
        }
        if (is_road(x, y+2, 0)) {
            ter(x, y+1, 0) = "road_ns";
        }
        break;
    case 3:
        while (c > 0 && x > 0 && (ter(x-1, y, 0) == settings.default_oter || c == cs)) {
            x--;
            c--;
            ter(x, y, 0) = "road_ew";
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 0; j++) {
                    if (abs(j) != abs(i) && (ter(x+j, y+i, 0) == "road_ew" ||
                                             ter(x+j, y+i, 0) == "road_ns")) {
                        ter(x, y, 0) = "road_null";
                        c = -1;
                    }
                }
            }
            put_buildings(x, y, dir, town);
            if (c < croad - 2 && c >= 3 && ter(x, y-1, 0) == settings.default_oter &&
                                           ter(x, y+1, 0) == settings.default_oter) {
                croad = c;
                make_road(x, y, cs - rng(1, 3), 0, town);
                make_road(x, y, cs - rng(1, 3), 2, town);
            }
        }
        if (is_road(x+2, y, 0)) {
            ter(x+1, y, 0) = "road_ew";
        }
        break;
    }

    cs -= rng(1, 3);
    if (cs >= 2 && c == 0) {
        int dir2;
        if (dir % 2 == 0) {
            dir2 = rng(0, 1) * 2 + 1;
        } else {
            dir2 = rng(0, 1) * 2;
        }
        make_road(x, y, cs, dir2, town);
        if (one_in(5)) {
            make_road(x, y, cs, (dir2 + 2) % 4, town);
        }
    }
}

bool overmap::build_lab(int x, int y, int z, int s)
{
    std::vector<point> generated_lab;
    ter(x, y, z) = "lab";
    for (int n = 0; n <= 1; n++) { // Do it in two passes to allow diagonals
        for (int i = 1; i <= s; i++) {
            for (int lx = x - i; lx <= x + i; lx++) {
                for (int ly = y - i; ly <= y + i; ly++) {
                    if ((ter(lx - 1, ly, z) == "lab" ||
                         ter(lx + 1, ly, z) == "lab" ||
                         ter(lx, ly - 1, z) == "lab" ||
                         ter(lx, ly + 1, z) == "lab") && one_in(i)) {
                        ter(lx, ly, z) = "lab";
                        generated_lab.push_back(point(lx,ly));
                    }
                }
            }
        }
    }

    bool generate_stairs = true;
    for (std::vector<point>::iterator it=generated_lab.begin();
         it != generated_lab.end(); it++) {
        if (ter(it->x, it->y, z+1) == "lab_stairs") {
            generate_stairs = false;
        }
    }
    if (generate_stairs && generated_lab.size() > 0) {
        int v = rng(0,generated_lab.size()-1);
        point p = generated_lab[v];
        ter(p.x, p.y, z+1) = "lab_stairs";
    }

    ter(x, y, z) = "lab_core";
    int numstairs = 0;
    if (s > 0) { // Build stairs going down
        while (!one_in(6)) {
            int stairx, stairy;
            int tries = 0;
            do {
                stairx = rng(x - s, x + s);
                stairy = rng(y - s, y + s);
                tries++;
            } while (ter(stairx, stairy, z) != "lab" && tries < 15);
            if (tries < 15) {
                ter(stairx, stairy, z) = "lab_stairs";
                numstairs++;
            }
        }
    }
    if (numstairs == 0) { // This is the bottom of the lab;  We need a finale
        int finalex, finaley;
        int tries = 0;
        do {
            finalex = rng(x - s, x + s);
            finaley = rng(y - s, y + s);
            tries++;
        } while (tries < 15 && ter(finalex, finaley, z) != "lab"
                            && ter(finalex, finaley, z) != "lab_core");
        ter(finalex, finaley, z) = "lab_finale";
    }
    zg.push_back(mongroup("GROUP_LAB", (x * 2), (y * 2), z, s, 400));

    return numstairs > 0;
}

bool overmap::build_ice_lab(int x, int y, int z, int s)
{
    std::vector<point> generated_ice_lab;
    ter(x, y, z) = "ice_lab";
    for (int n = 0; n <= 1; n++) { // Do it in two passes to allow diagonals
        for (int i = 1; i <= s; i++) {
            for (int lx = x - i; lx <= x + i; lx++) {
                for (int ly = y - i; ly <= y + i; ly++) {
                    if ((ter(lx - 1, ly, z) == "ice_lab" ||
                         ter(lx + 1, ly, z) == "ice_lab" ||
                         ter(lx, ly - 1, z) == "ice_lab" ||
                         ter(lx, ly + 1, z) == "ice_lab") && one_in(i)) {
                        ter(lx, ly, z) = "ice_lab";
                        generated_ice_lab.push_back(point(lx,ly));
                    }
                }
            }
        }
    }

    bool generate_stairs = true;
    for (std::vector<point>::iterator it = generated_ice_lab.begin();
         it != generated_ice_lab.end(); ++it) {
        if (ter(it->x, it->y, z + 1) == "ice_lab_stairs") {
            generate_stairs = false;
        }
    }
    if (generate_stairs && generated_ice_lab.size() > 0) {
        int v = rng(0,generated_ice_lab.size() - 1);
        point p = generated_ice_lab[v];
        ter(p.x, p.y, z + 1) = "ice_lab_stairs";
    }

    ter(x, y, z) = "ice_lab_core";
    int numstairs = 0;
    if (s > 0) { // Build stairs going down
        while (!one_in(6)) {
            int stairx, stairy;
            int tries = 0;
            do {
                stairx = rng(x - s, x + s);
                stairy = rng(y - s, y + s);
                tries++;
            } while (ter(stairx, stairy, z) != "ice_lab" && tries < 15);
            if (tries < 15) {
                ter(stairx, stairy, z) = "ice_lab_stairs";
                numstairs++;
            }
        }
    }
    if (numstairs == 0) { // This is the bottom of the ice_lab;  We need a finale
        int finalex, finaley;
        int tries = 0;
        do {
            finalex = rng(x - s, x + s);
            finaley = rng(y - s, y + s);
            tries++;
        } while (tries < 15 && ter(finalex, finaley, z) != "ice_lab"
                            && ter(finalex, finaley, z) != "ice_lab_core");
        ter(finalex, finaley, z) = "ice_lab_finale";
    }
    zg.push_back(mongroup("GROUP_ICE_LAB", (x * 2), (y * 2), z, s, 400));

    return numstairs > 0;
}

void overmap::build_anthill(int x, int y, int z, int s)
{
    build_tunnel(x, y, z, s - rng(0, 3), 0);
    build_tunnel(x, y, z, s - rng(0, 3), 1);
    build_tunnel(x, y, z, s - rng(0, 3), 2);
    build_tunnel(x, y, z, s - rng(0, 3), 3);
    std::vector<point> queenpoints;
    for (int i = x - s; i <= x + s; i++) {
        for (int j = y - s; j <= y + s; j++) {
            if (check_ot_type("ants", i, j, z)) {
                queenpoints.push_back(point(i, j));
            }
        }
    }
    int index = rng(0, queenpoints.size() - 1);
    ter(queenpoints[index].x, queenpoints[index].y, z) = "ants_queen";
}

void overmap::build_tunnel(int x, int y, int z, int s, int dir)
{
    if (s <= 0) {
        return;
    }
    if (!check_ot_type("ants", x, y, z)) {
        ter(x, y, z) = "ants_ns";
    }
    point next;
    switch (dir) {
        case 0: next = point(x    , y - 1);
        case 1: next = point(x + 1, y    );
        case 2: next = point(x    , y + 1);
        case 3: next = point(x - 1, y    );
    }
    if (s == 1) {
        next = point(-1, -1);
    }
    std::vector<point> valid;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (!check_ot_type("ants", i, j, z) && abs(i - x) + abs(j - y) == 1) {
                valid.push_back(point(i, j));
            }
        }
    }
    for (int i = 0; i < valid.size(); i++) {
        if (valid[i].x != next.x || valid[i].y != next.y) {
            if (one_in(s * 2)) {
                if (one_in(2)) {
                    ter(valid[i].x, valid[i].y, z) = "ants_food";
                } else {
                    ter(valid[i].x, valid[i].y, z) = "ants_larvae";
                }
            } else if (one_in(5)) {
                int dir2 = 0;
                if (valid[i].y == y - 1) { dir2 = 0; }
                if (valid[i].x == x + 1) { dir2 = 1; }
                if (valid[i].y == y + 1) { dir2 = 2; }
                if (valid[i].x == x - 1) { dir2 = 3; }
                build_tunnel(valid[i].x, valid[i].y, z, s - rng(0, 3), dir2);
            }
        }
    }
    build_tunnel(next.x, next.y, z, s - 1, dir);
}

bool overmap::build_slimepit(int x, int y, int z, int s)
{
    bool requires_sub = false;
    for (int n = 1; n <= s; n++)
    {
        for (int i = x - n; i <= x + n; i++)
        {
            for (int j = y - n; j <= y + n; j++)
            {
                if (rng(1, s * 2) >= n)
                {
                    if (one_in(8) && z > -OVERMAP_DEPTH)
                    {
                        ter(i, j, z) = "slimepit_down";
                        requires_sub = true;
                    } else {
                        ter(i, j, z) = "slimepit";
                    }
                }
            }
        }
    }

    return requires_sub;
}

void overmap::build_mine(int x, int y, int z, int s)
{
    bool finale = (s <= rng(1, 3));
    int built = 0;
    if (s < 2) {
        s = 2;
    }
    while (built < s) {
        ter(x, y, z) = "mine";
        std::vector<point> next;
        for (int i = -1; i <= 1; i += 2) {
            if (ter(x, y + i, z) == "rock") {
                next.push_back( point(x, y + i) );
            }
            if (ter(x + i, y, z) == "rock") {
                next.push_back( point(x + i, y) );
            }
        }
        if (next.empty()) { // Dead end!  Go down!
            ter(x, y, z) = (finale ? "mine_finale" : "mine_down");
            return;
        }
        point p = next[ rng(0, next.size() - 1) ];
        x = p.x;
        y = p.y;
        built++;
    }
    ter(x, y, z) = (finale ? "mine_finale" : "mine_down");
}

void overmap::place_rifts(int const z)
{
 int num_rifts = rng(0, 2) * rng(0, 2);
 std::vector<point> riftline;
 if (!one_in(4))
  num_rifts++;
 for (int n = 0; n < num_rifts; n++) {
  int x = rng(MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE);
  int y = rng(MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE);
  int xdist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE),
      ydist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE);
// We use rng(0, 10) as the t-value for this Bresenham Line, because by
// repeating this twice, we can get a thick line, and a more interesting rift.
  for (int o = 0; o < 3; o++) {
   if (xdist > ydist)
    riftline = line_to(x - xdist, y - ydist+o, x + xdist, y + ydist, rng(0,10));
   else
    riftline = line_to(x - xdist+o, y - ydist, x + xdist, y + ydist, rng(0,10));
   for (int i = 0; i < riftline.size(); i++) {
    if (i == riftline.size() / 2 && !one_in(3))
     ter(riftline[i].x, riftline[i].y, z) = "hellmouth";
    else
     ter(riftline[i].x, riftline[i].y, z) = "rift";
   }
  }
 }
}

void overmap::make_hiway(int x1, int y1, int x2, int y2, int z, const std::string &base)
{
    if (x1 == x2 && y1 == y2) {
        return;
    }

    std::priority_queue<node> nodes[2];
    bool closed[OMAPX][OMAPY] = {{false}};
    int open[OMAPX][OMAPY] = {{0}};
    int dirs[OMAPX][OMAPY] = {{0}};
    int dx[4]={1, 0, -1, 0};
    int dy[4]={0, 1, 0, -1};
    int i = 0;
    int disp = (base == "road") ? 5 : 2;

    nodes[i].push(node(x1, y1, 5, 1000));
    open[x1][y1] = 1000;

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while (!nodes[i].empty()) {
        // get the best-looking node
        node mn = nodes[i].top();
        nodes[i].pop();
        // make sure it's in bounds
        if (mn.x >= OMAPX || mn.x < 0 || mn.y >= OMAPY || mn.y < 0) {
            continue;
        }
        // mark it visited
        closed[mn.x][mn.y] = true;

        // if we've reached the end, draw the path and return
        if (mn.x == x2 && mn.y == y2) {
            int x = mn.x;
            int y = mn.y;
            while (x != x1 || y != y1) {
                int d = dirs[x][y];
                x += dx[d];
                y += dy[d];
                if (road_allowed(ter(x, y, z))) {
                    if (is_river(ter(x, y, z))) {
                        if (d == 1 || d == 3) {
                            ter(x, y, z) = "bridge_ns";
                        } else {
                            ter(x, y, z) = "bridge_ew";
                        }
                    } else {
                        ter(x, y, z) = base + "_nesw";
                    }
                }
            }
            return;
        }

        // otherwise, expand to
        for(int d = 0; d < 4; d++) {
            int x = mn.x + dx[d];
            int y = mn.y + dy[d];
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            // * tiles that don't allow roads to cross them (e.g. buildings)
            // * corners on rivers
            if (x < 1 || x > OMAPX - 2 || y < 1 || y > OMAPY - 2 ||
                closed[x][y] || !road_allowed(ter(x, y, z)) ||
                (is_river(ter(mn.x, mn.y, z)) && mn.d != d) ||
                (is_river(ter(x,    y,    z)) && mn.d != d) ) {
                continue;
            }

            node cn = node(x, y, d, 0);
            // distance to target
            cn.p += ((abs(x2 - x) + abs(y2 - y)) / disp);
            // prefer existing roads.
            cn.p += check_ot_type(base, x, y, z) ? 0 : 3;
            // and flat land over bridges
            cn.p += !is_river(ter(x, y, z)) ? 0 : 2;
            // try not to turn too much
            //cn.p += (mn.d == d) ? 0 : 1;

            // record direction to shortest path
            if (open[x][y] == 0) {
                dirs[x][y] = (d + 2) % 4;
                open[x][y] = cn.p;
                nodes[i].push(cn);
            } else if (open[x][y] > cn.p) {
                dirs[x][y] = (d + 2) % 4;
                open[x][y] = cn.p;

                // wizardry
                while (nodes[i].top().x != x || nodes[i].top().y != y) {
                    nodes[1 - i].push(nodes[i].top());
                    nodes[i].pop();
                }
                nodes[i].pop();

                if (nodes[i].size() > nodes[1-i].size()) {
                    i = 1 - i;
                }
                while (!nodes[i].empty()) {
                    nodes[1 - i].push(nodes[i].top());
                    nodes[i].pop();
                }
                i = 1 - i;
                nodes[i].push(cn);
            } else {
                // a shorter path has already been found
            }
        }
    }
}

void overmap::building_on_hiway(int x, int y, int dir)
{
    int xdif = dir * (1 - 2 * rng(0,1));
    int ydif = (1 - dir) * (1 - 2 * rng(0,1));
    int rot = 0;
    if (ydif ==  1) {
        rot = 0;
    } else if (xdif == -1) {
        rot = 1;
    } else if (ydif == -1) {
        rot = 2;
    } else if (xdif ==  1) {
        rot = 3;
    }

    switch (rng(1, 4)) {
    case 1:
        if (!is_river(ter(x + xdif, y + ydif, 0))) {
            ter(x + xdif, y + ydif, 0) = "lab_stairs";
        }
        break;
    case 2:
        if (!is_river(ter(x + xdif, y + ydif, 0))) {
            ter(x + xdif, y + ydif, 0) = "ice_lab_stairs";
        }
        break;
    case 3:
        if (!is_river(ter(x + xdif, y + ydif, 0))) {
            ter(x + xdif, y + ydif, 0) = house(rot, settings.house_basement_chance);
        }
        break;
    case 4:
        if (!is_river(ter(x + xdif, y + ydif, 0))) {
            ter(x + xdif, y + ydif, 0) = "radio_tower";
        }
        break;
    }
}

void overmap::place_hiways(std::vector<city> cities, int z, const std::string &base)
{
    if (cities.size() == 1) {
        return;
    }
    city best;
    for (int i = 0; i < cities.size(); i++) {
        int closest = -1;
        for (int j = i + 1; j < cities.size(); j++) {
            int distance = (int)dist(cities[i].x, cities[i].y, cities[j].x, cities[j].y);
            if (distance < closest || closest < 0) {
                closest = distance;
                best = cities[j];
            }
        }
        if( closest > 0 ) {
            make_hiway(cities[i].x, cities[i].y, best.x, best.y, z, base);
        }
    }
}

// Polish does both good_roads and good_rivers (and any future polishing) in
// a single loop; much more efficient
void overmap::polish(const int z, const std::string &terrain_type)
{
    const bool check_all = (terrain_type == "all");
    // Main loop--checks roads and rivers that aren't on the borders of the map
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            if (check_all || check_ot_type(terrain_type, x, y, z)) {
                if (check_ot_type("road", x, y, z)) {
                    good_road("road", x, y, z);
                } else if (check_ot_type("bridge", x, y, z) &&
                           check_ot_type("bridge", x - 1, y, z) &&
                           check_ot_type("bridge", x + 1, y, z) &&
                           check_ot_type("bridge", x, y - 1, z) &&
                           check_ot_type("bridge", x, y + 1, z)) {
                    ter(x, y, z) = "road_nesw";
                } else if (check_ot_type("subway", x, y, z)) {
                    good_road("subway", x, y, z);
                } else if (check_ot_type("sewer", x, y, z)) {
                    good_road("sewer", x, y, z);
                } else if (check_ot_type("ants", x, y, z)
                        && !check_ot_type("ants_queen", x, y, z)
                        && !check_ot_type("ants_larvae", x, y, z)
                        && !check_ot_type("ants_food", x, y, z)) {
                    good_road("ants", x, y, z);
                } else if (check_ot_type("river", x, y, z)) {
                    good_river(x, y, z);
                // Sometimes a bridge will start at the edge of a river,
                // and this looks ugly.
                // So, fix it by making that square normal road;
                // bit of a kludge but it works.
                } else if (ter(x, y, z) == "bridge_ns" &&
                           (!is_river(ter(x - 1, y, z)) ||
                            !is_river(ter(x + 1, y, z)))) {
                    ter(x, y, z) = "road_ns";
                } else if (ter(x, y, z) == "bridge_ew" &&
                           (!is_river(ter(x, y - 1, z)) ||
                            !is_river(ter(x, y + 1, z)))) {
                    ter(x, y, z) = "road_ew";
                }
            }
        }
    }

    // Fixes stretches of parallel roads--turns them into two-lane highways
    // Note that this fixes 2x2 areas...
    // a "tail" of 1x2 parallel roads may be left.
    // This can actually be a good thing; it ensures nice connections
    // Also, this leaves, say, 3x3 areas of road.
    // TODO: fix this?  courtyards etc?
    for (int y = 0; y < OMAPY - 1; y++) {
        for (int x = 0; x < OMAPX - 1; x++) {
            if (check_ot_type(terrain_type, x, y, z)) {
                if (ter(x, y, z) == "road_nes"
                        && ter(x+1, y, z) == "road_nsw"
                        && ter(x, y+1, z) == "road_nes"
                        && ter(x+1, y+1, z) == "road_nsw") {
                    ter(x, y, z) = "hiway_ns";
                    ter(x+1, y, z) = "hiway_ns";
                    ter(x, y+1, z) = "hiway_ns";
                    ter(x+1, y+1, z) = "hiway_ns";
                } else if (ter(x, y, z) == "road_esw"
                            && ter(x+1, y, z) == "road_esw"
                            && ter(x, y+1, z) == "road_new"
                            && ter(x+1, y+1, z) == "road_new") {
                    ter(x, y, z) = "hiway_ew";
                    ter(x+1, y, z) = "hiway_ew";
                    ter(x, y+1, z) = "hiway_ew";
                    ter(x+1, y+1, z) = "hiway_ew";
                }
            }
        }
    }
}

bool overmap::check_ot_type(const std::string &otype, int x, int y, int z)
{
    const oter_id oter = ter(x, y, z);
    return is_ot_type(otype, oter);
}

bool overmap::is_road(int x, int y, int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
        for (int i = 0; i < roads_out.size(); i++) {
            if (abs(roads_out[i].x - x) + abs(roads_out[i].y - y) <= 1) {
                return true;
            }
        }
    }
    return ter(x, y, z).t().is_road;
//oter_t(ter(x, y, z)).is_road;
}

bool overmap::is_road_or_highway(int x, int y, int z)
{
    if (is_road(x,y,z) || check_ot_type("hiway",x,y,z)) {
        return true;
    }
    return false;
}

void overmap::good_road(const std::string &base, int x, int y, int z)
{
    if (check_ot_type(base, x, y-1, z)) {
        if (check_ot_type(base, x+1, y, z)) {
            if (check_ot_type(base, x, y+1, z)) {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_nesw";
                } else {
                    ter(x, y, z) = base + "_nes";
                }
            } else {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_new";
                } else {
                    ter(x, y, z) = base + "_ne";
                }
            }
        } else {
            if (check_ot_type(base, x, y+1, z)) {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_nsw";
                } else {
                    ter(x, y, z) = base + "_ns";
                }
            } else {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_wn";
                } else {
                    ter(x, y, z) = base + "_ns";
                }
            }
        }
    } else {
        if (check_ot_type(base, x+1, y, z)) {
            if (check_ot_type(base, x, y+1, z)) {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_esw";
                } else {
                    ter(x, y, z) = base + "_es";
                }
            } else {
                ter(x, y, z) = base + "_ew";
            }
        } else {
            if (check_ot_type(base, x, y+1, z)) {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_sw";
                } else {
                    ter(x, y, z) = base + "_ns";
                }
            } else {
                if (check_ot_type(base, x-1, y, z)) {
                    ter(x, y, z) = base + "_ew";
                } else {
                    // No adjoining roads/etc.
                    // Happens occasionally, esp. with sewers.
                    ter(x, y, z) = base + "_nesw";
                }
            }
        }
    }
    if (ter(x, y, z) == "road_nesw" && one_in(4)) {
        ter(x, y, z) = "road_nesw_manhole";
    }
}

void overmap::good_river(int x, int y, int z)
{
    if (is_river(ter(x - 1, y, z))) {
        if (is_river(ter(x, y - 1, z))) {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    // River on N, S, E, W;
                    // but we might need to take a "bite" out of the corner
                    if (!is_river(ter(x - 1, y - 1, z))) {
                        ter(x, y, z) = "river_c_not_nw";
                    } else if (!is_river(ter(x + 1, y - 1, z))) {
                        ter(x, y, z) = "river_c_not_ne";
                    } else if (!is_river(ter(x - 1, y + 1, z))) {
                        ter(x, y, z) = "river_c_not_sw";
                    } else if (!is_river(ter(x + 1, y + 1, z))) {
                        ter(x, y, z) = "river_c_not_se";
                    } else {
                        ter(x, y, z) = "river_center";
                    }
                } else {
                    ter(x, y, z) = "river_east";
                }
            } else {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = "river_south";
                } else {
                    ter(x, y, z) = "river_se";
                }
            }
        } else {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = "river_north";
                } else {
                    ter(x, y, z) = "river_ne";
                }
            } else {
                if (is_river(ter(x + 1, y, z))) { // Means it's swampy
                    ter(x, y, z) = "forest_water";
                }
            }
        }
    } else {
        if (is_river(ter(x, y - 1, z))) {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = "river_west";
                } else { // Should never happen
                    ter(x, y, z) = "forest_water";
                }
            } else {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = "river_sw";
                } else { // Should never happen
                    ter(x, y, z) = "forest_water";
                }
            }
        } else {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = "river_nw";
                } else { // Should never happen
                    ter(x, y, z) = "forest_water";
                }
            } else { // Should never happen
                ter(x, y, z) = "forest_water";
            }
        }
    }
}

void overmap::place_specials()
{
 int placed[NUM_OMSPECS];
 for (int i = 0; i < NUM_OMSPECS; i++)
  placed[i] = 0;

 std::vector<point> sectors;
 for (int x = 0; x < OMAPX; x += OMSPEC_FREQ) {
  for (int y = 0; y < OMAPY; y += OMSPEC_FREQ)
   sectors.push_back(point(x, y));
 }

 while (!sectors.empty()) {
  int sector_pick = rng(0, sectors.size() - 1);
  int x = sectors[sector_pick].x, y = sectors[sector_pick].y;
  sectors.erase(sectors.begin() + sector_pick);
  std::vector<omspec_id> valid;
  int tries = 0;
  tripoint p;
  do {
   p = tripoint(rng(x, x + OMSPEC_FREQ - 1), rng(y, y + OMSPEC_FREQ - 1), 0);
   if (p.x >= OMAPX - 1)
    p.x = OMAPX - 2;
   if (p.y >= OMAPY - 1)
    p.y = OMAPY - 2;
   if (p.x == 0)
    p.x = 1;
   if (p.y == 0)
    p.y = 1;
   for (int i = 0; i < NUM_OMSPECS; i++) {
    omspec_place place;
    overmap_special special = overmap_specials[i];
    int min = special.min_dist_from_city, max = special.max_dist_from_city;
    point pt(p.x, p.y);
    // Skip non-classic specials if we're in classic mode
    if (ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] && !(special.flags & mfb(OMS_FLAG_CLASSIC))) continue;
    if ((placed[ omspec_id(i) ] < special.max_appearances || special.max_appearances <= 0) &&
        (min == -1 || dist_from_city(pt) >= min) &&
        (max == -1 || dist_from_city(pt) <= max) &&
        (place.*special.able)(this, special.flags, p))
     valid.push_back( omspec_id(i) );
   }
   tries++;
  } while (valid.empty() && tries < 20); // Done looking for valid spot

  if (!valid.empty()) { // We found a valid spot!
// Place the MUST HAVE ones first, to try and guarantee that they appear
   std::vector<omspec_id> must_place;
   for (int i = 0; i < valid.size(); i++) {
    if (placed[ valid[i] ] < overmap_specials[ valid[i] ].min_appearances)
     must_place.push_back(valid[i]);
   }
   if (must_place.empty()) {
    int selection = rng(0, valid.size() - 1);
    overmap_special special = overmap_specials[ valid[selection] ];
    placed[ valid[selection] ]++;
    place_special(special, p);
   } else {
    int selection = rng(0, must_place.size() - 1);
    overmap_special special = overmap_specials[ must_place[selection] ];
    placed[ must_place[selection] ]++;
    place_special(special, p);
   }
  } // Done with <Found a valid spot>

 } // Done picking sectors...
}


// find the id for a specified rotation of a rotatable oter_t
oter_id overmap::rotate(const oter_id &oter, int dir)
{
    const oter_t & otert = oter;
    if (! otert.rotates ) {
        debugmsg("%s does not rotate.", oter.c_str());
        return oter;
    }
    if (dir < 0) {
        dir += 4;
    } else if (dir > 3) {
        debugmsg("Bad rotation for %s: %d.", oter.c_str(), dir);
        return oter;
    }
    return otert.directional_peers[dir];
}

void overmap::place_special(overmap_special special, tripoint p)
{
    bool rotated = false;
    int city = -1;
    // First, place terrain...
    ter(p.x, p.y, p.z) = special.ter;
    // Next, obey any special effects the flags might have
    if (special.flags & mfb(OMS_FLAG_ROTATE_ROAD)) {
        if (is_road_or_highway(p.x, p.y - 1, p.z)) {
            ter(p.x, p.y, p.z) = rotate(special.ter, 0);
            rotated = true;
        } else if (is_road_or_highway(p.x + 1, p.y, p.z)) {
            ter(p.x, p.y, p.z) = rotate(special.ter, 1);
            rotated = true;
        } else if (is_road_or_highway(p.x, p.y + 1, p.z)) {
            ter(p.x, p.y, p.z) = rotate(special.ter, 2);
            rotated = true;
        } else if (is_road_or_highway(p.x - 1, p.y, p.z)) {
            ter(p.x, p.y, p.z) = rotate(special.ter, 3);
            rotated = true;
        }
    }

    if (!rotated && special.flags & mfb(OMS_FLAG_ROTATE_RANDOM)) {
        ter(p.x, p.y, p.z) = rotate(special.ter, rng(0, 3));
    }

    if (special.flags & mfb(OMS_FLAG_ROAD)) {
        int closest = -1, distance = 999;
        for (int i = 0; i < cities.size(); i++) {
            int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
            if (dist < distance) {
                closest = i;
                distance = dist;
            }
        }
        if (special.flags & (mfb(OMS_FLAG_2X2_SECOND) | mfb(OMS_FLAG_3X3_FIXED))) {
            city = closest;
        } else {
            make_hiway(p.x, p.y, cities[closest].x, cities[closest].y, p.z, "road");
        }
    }

    if (special.flags & mfb(OMS_FLAG_3X3)) {
        for (int x = p.x; x < p.x + 3; x++) {
            for (int y = p.y; y < p.y + 3; y++) {
                if (x == p.x && y == p.y) {
                    y++; // Already handled
                }
                ter(x, y, p.z) = special.ter;
            }
        }
    }

    if (special.flags & mfb(OMS_FLAG_3X3_SECOND)) {
        size_t e_pos = std::string(special.ter).find("_entrance",0,9);
        std::string ter_base;
        if (e_pos != std::string::npos) {
            // strip "_entrance" to get the base oter_id
            ter_base = std::string(special.ter).substr(0, e_pos);
        } else if (special.ter == "farm") {
            ter_base = std::string(special.ter) + "_field";
        } else {
            ter_base = std::string(special.ter);
        }
        for (int x = p.x; x < p.x + 3; x++) {
            for (int y = p.y; y < p.y + 3; y++) {
                ter(x, y, p.z) = ter_base;
            }
        }

        if (is_road(p.x + 3, p.y + 1, p.z)) { // Road to east
            ter(p.x + 2, p.y + 1, p.z) = special.ter;
        } else if (is_road(p.x + 1, p.y + 3, p.z)) { // Road to south
            ter(p.x + 1, p.y + 2, p.z) = special.ter;
        } else if (is_road(p.x - 1, p.y + 1, p.z)) { // Road to west
            ter(p.x, p.y + 1, p.z) = special.ter;
        } else { // Road to north, or no roads
            ter(p.x + 1, p.y, p.z) = special.ter;
        }
    }

 if (special.flags & mfb(OMS_FLAG_BLOB)) {
  for (int x = -2; x <= 2; x++) {
   for (int y = -2; y <= 2; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    omspec_place place;
    tripoint np(p.x + x, p.y + y, p.z);
    if (one_in(1 + abs(x) + abs(y)) && (place.*special.able)(this, special.flags, np))
     ter(p.x + x, p.y + y, p.z) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_BIG)) {
  for (int x = -3; x <= 3; x++) {
   for (int y = -3; y <= 3; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    omspec_place place;
    tripoint np(p.x + x, p.y + y, p.z);
    if ((place.*special.able)(this, special.flags, np))
     ter(p.x + x, p.y + y, p.z) = special.ter;
     ter(p.x + x, p.y + y, p.z) = special.ter;
   }
  }
 }

    if (special.flags & mfb(OMS_FLAG_3X3_FIXED)) {
        // road comes out of "_2" variant, rotations:
        //  |
        // 321   963   789   147
        // 654   852-  456  -258
        // 987   741   123   369
        //              |
        // reference point is top-left
        int dir = 0;
        if (is_road(p.x + 1, p.y - 1, p.z)) { // Road to north
            dir = 0;
        } else if (is_road(p.x + 3, p.y + 1, p.z)) { // Road to east
            dir = 1;
        } else if (is_road(p.x + 1, p.y + 3, p.z)) { // Road to south
            dir = 2;
        } else if (is_road(p.x - 1, p.y + 1, p.z)) { // Road to west
            dir = 3;
        } else {
            dir = rng(0, 3); // Random direction;
        }
//fixme

        // usually will be called with the _2 entrance variant,
        // for example "school_2"
        size_t suffix_pos = std::string(special.ter).rfind("_2", std::string::npos, 2);
        std::string ter_base = std::string(special.ter).substr(0, suffix_pos);
        const char* suffix[] = {
            "_1", "_2", "_3", "_4", "_5", "_6","_7", "_8", "_9"
        };
        if (dir == 0) {
            for (int i = 0, y = p.y; y <= p.y + 2; y++) {
                for (int x = p.x + 2; x >= p.x; x--, i++) {
                    ter(x, y, p.z) = ter_base + suffix[i];
                }
            }
            if (ter_base == "school") { // wat. fixme.
                make_hiway(p.x, p.y - 1, p.x + 1, p.y - 1, p.z, "road");
            }
        } else if (dir == 1) {
            for (int i = 0, x = p.x + 2; x >= p.x; x--) {
                for (int y = p.y + 2; y >= p.y; y--, i++) {
                    ter(x, y, p.z) = ter_base + suffix[i];
                }
            }
            if (ter_base == "school") {
                make_hiway(p.x + 3, p.y, p.x + 3, p.y + 1, p.z, "road");
            }
        } else if (dir == 2) {
            for (int i = 0, y = p.y + 2; y >= p.y; y--) {
                for (int x = p.x; x <= p.x + 2; x++, i++) {
                    ter(x, y, p.z) = ter_base + suffix[i];
                }
            }
            if (ter_base == "school") {
                make_hiway(p.x + 2, p.y + 3, p.x + 1, p.y + 3, p.z, "road");
            }
        } else if (dir == 3) {
            for (int i = 0, x = p.x; x <= p.x + 2; x++) {
                for (int y = p.y; y <= p.y + 2; y++, i++) {
                    ter(x, y, p.z) = ter_base + suffix[i];
                }
            }
            if (ter_base == "school") {
                make_hiway(p.x - 1, p.y + 2, p.x - 1, p.y + 1, p.z, "road");
            }
        }

        if (special.flags & mfb(OMS_FLAG_ROAD)) {
            if (dir == 0) {
                make_hiway(p.x + 1, p.y - 1, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 1) {
                make_hiway(p.x + 3, p.y + 1, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 2) {
                make_hiway(p.x + 1, p.y + 3, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 3) {
                make_hiway(p.x - 1, p.y + 1, cities[city].x,
                           cities[city].y, p.z, "road");
            }
        }
    }

    // Buildings should be designed with the entrance at the southwest corner
    // and open to the street on the south.

    if (special.flags & mfb(OMS_FLAG_2X2_SECOND)) {
        size_t e_pos = std::string(special.ter).find("_entrance",0,9);
        std::string ter_base;
        if (e_pos != std::string::npos) {
            // strip "_entrance" to get the base oter_id
            ter_base = std::string(special.ter).substr(0, e_pos);
        } else {
            ter_base = std::string(special.ter);
        }
        for (int x = p.x; x < p.x + 2; x++) {
            for (int y = p.y; y < p.y + 2; y++) {
                ter(x, y, p.z) = ter_base;
            }
        }

        int dir = 0;
        if (is_road(p.x + 1, p.y - 1, p.z)) { // Road to north
            dir = 0;
        } else if (is_road(p.x + 2, p.y + 1, p.z)) { // Road to east
            dir = 1;
        } else if (is_road(p.x, p.y + 2, p.z)) { // Road to south
            dir = 2;
        } else if (is_road(p.x - 1, p.y, p.z)) { // Road to west
            dir = 3;
        } else {
            dir = rng(0, 3); // Random direction;
        }

        if (dir == 0) {
            ter(p.x + 1, p.y, p.z) = special.ter;
        } else if (dir == 1) {
            ter(p.x + 1, p.y + 1, p.z) = special.ter;
        } else if (dir == 2) {
            ter(p.x, p.y + 1, p.z) = special.ter;
        } else if (dir == 3) {
            ter(p.x, p.y, p.z) = special.ter;
        }

        if (special.flags & mfb(OMS_FLAG_ROAD)) {
            if (dir == 0) {
                make_hiway(p.x + 1, p.y - 1, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 1) {
                make_hiway(p.x + 2, p.y + 1, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 2) {
                make_hiway(p.x, p.y + 2, cities[city].x,
                           cities[city].y, p.z, "road");
            } else if (dir == 3) {
                make_hiway(p.x - 1, p.y, cities[city].x,
                           cities[city].y, p.z, "road");
            }
        }
    }

 if (special.flags & mfb(OMS_FLAG_PARKING_LOT)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  if (special.flags & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND))) {
   ter(p.x + 1, p.y - 1, p.z) = "s_lot";
   make_hiway(p.x + 1, p.y - 1, cities[closest].x, cities[closest].y, p.z, "road");
  } else {
   ter(p.x, p.y - 1, p.z) = "s_lot";
   make_hiway(p.x, p.y - 1, cities[closest].x, cities[closest].y, p.z, "road");
  }
 }

 if (special.flags & mfb(OMS_FLAG_DIRT_LOT)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p.x, p.y, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  if (special.flags & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND))) {
   ter(p.x + 1, p.y - 1, p.z) = "dirtlot";
   make_hiway(p.x + 1, p.y - 1, cities[closest].x, cities[closest].y, p.z, "road");
  } else {
   ter(p.x, p.y - 1, p.z) = "dirtlot";
   make_hiway(p.x, p.y - 1, cities[closest].x, cities[closest].y, p.z, "road");
  }
 }

// Finally, place monsters if applicable
 if (special.monsters != "GROUP_NULL") {
  if (special.monster_pop_min == 0 || special.monster_pop_max == 0 ||
      special.monster_rad_min == 0 || special.monster_rad_max == 0   ) {
   debugmsg("Overmap special %s has bad spawn: pop(%d, %d) rad(%d, %d)",
            otermap[special.ter].name.c_str(), special.monster_pop_min,
            special.monster_pop_max, special.monster_rad_min,
            special.monster_rad_max);
   return;
  }

  int population = rng(special.monster_pop_min, special.monster_pop_max);
  int radius     = rng(special.monster_rad_min, special.monster_rad_max);
  zg.push_back(
     mongroup(special.monsters, p.x * 2, p.y * 2, p.z, radius, population));
 }
}

void overmap::place_mongroups()
{
 if (!ACTIVE_WORLD_OPTIONS["STATIC_SPAWN"]) {
  // Cities are full of zombies
  for (unsigned int i = 0; i < cities.size(); i++) {
   if (!one_in(16) || cities[i].s > 5)
    zg.push_back (mongroup("GROUP_ZOMBIE", (cities[i].x * 2), (cities[i].y * 2), 0,
                           int(cities[i].s * 2.5), cities[i].s * 80));
  }
 }

 if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
  // Figure out where swamps are, and place swamp monsters
  for (int x = 3; x < OMAPX - 3; x += 7) {
   for (int y = 3; y < OMAPY - 3; y += 7) {
    int swamp_count = 0;
    for (int sx = x - 3; sx <= x + 3; sx++) {
     for (int sy = y - 3; sy <= y + 3; sy++) {
      if (ter(sx, sy, 0) == "forest_water")
       swamp_count += 2;
     }
    }
    if (swamp_count >= 25)
     zg.push_back(mongroup("GROUP_SWAMP", x * 2, y * 2, 0, 3,
                           rng(swamp_count * 8, swamp_count * 25)));
   }
  }
 }

  if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
  // Figure out where rivers are, and place swamp monsters
  for (int x = 3; x < OMAPX - 3; x += 7) {
   for (int y = 3; y < OMAPY - 3; y += 7) {
    int river_count = 0;
    for (int sx = x - 3; sx <= x + 3; sx++) {
     for (int sy = y - 3; sy <= y + 3; sy++) {
      if (is_river(ter(sx, sy, 0)))
       river_count++;
     }
    }
    if (river_count >= 25)
     zg.push_back(mongroup("GROUP_RIVER", x * 2, y * 2, 0, 3,
                           rng(river_count * 8, river_count * 25)));
   }
  }
 }

 if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
  // Place the "put me anywhere" groups
  int numgroups = rng(0, 3);
  for (int i = 0; i < numgroups; i++) {
   zg.push_back(
    mongroup("GROUP_WORM", rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1), 0,
             rng(20, 40), rng(30, 50)));
  }
 }

 // Forest groups cover the entire map
 zg.push_back( mongroup("GROUP_FOREST", OMAPX / 2, OMAPY / 2, 0,
                        OMAPY, rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back( mongroup("GROUP_FOREST", OMAPX / 2, (OMAPY * 3) / 2, 0,
                        OMAPY, rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back( mongroup("GROUP_FOREST", (OMAPX * 3) / 2, OMAPY / 2, 0,
                        OMAPX, rng(2000, 12000)));
 zg.back().diffuse = true;
 zg.push_back( mongroup("GROUP_FOREST", (OMAPX * 3) / 2, (OMAPY * 3) / 2, 0,
                        OMAPX, rng(2000, 12000)));
 zg.back().diffuse = true;
}

void overmap::place_radios()
{
 char message[200];
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j, 0) == "radio_tower") {
       int choice = rng(0, 2);
       switch(choice)
       {
       case 0:
           snprintf( message, sizeof(message), _("This is emergency broadcast station %d%d.\
  Please proceed quickly and calmly to your designated evacuation point."), i, j);
           radios.push_back(radio_tower(i*2, j*2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
           break;
       case 1:
           radios.push_back(radio_tower(i*2, j*2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH),
               _("Head West.  All survivors, head West.  Help is waiting.")));
           break;
       case 2:
           radios.push_back(radio_tower(i*2, j*2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), "", WEATHER_RADIO));
           break;
       }
   } else if (ter(i, j, 0) == "lmoe") {
    snprintf( message, sizeof(message), _("This is automated emergency shelter beacon %d%d.\
  Supplies, amenities and shelter are stocked."), i, j);
    radios.push_back(radio_tower(i*2, j*2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH) / 2, message));
   } else if (ter(i, j, 0) == "fema_entrance") {
    snprintf( message, sizeof(message), _("This is FEMA camp %d%d.\
  Supplies are limited, please bring supplemental food, water, and bedding.\
  This is FEMA camp %d%d.  A designated long-term emergency shelter."), i, j, i, j);
    radios.push_back(radio_tower(i*2, j*2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
   }
  }
 }
}


void overmap::open()
{
 std::string const plrfilename = player_filename(loc.x, loc.y);
 std::string const terfilename = terrain_filename(loc.x, loc.y);
  std::ifstream fin;
// Set position IDs
 fin.open(terfilename.c_str());
 if (fin.is_open()) {
   unserialize(fin, plrfilename, terfilename);
   fin.close();
 } else { // No map exists!  Prepare neighbors, and generate one.
  std::vector<overmap*> pointers;
// Fetch north and south
  for (int i = -1; i <= 1; i+=2) {
   std::string const tmpfilename = terrain_filename(loc.x, loc.y + i);
   fin.open(tmpfilename.c_str());
   if (fin.is_open()) {
    fin.close();
    pointers.push_back(new overmap(loc.x, loc.y + i));
   } else
    pointers.push_back(NULL);
  }
// Fetch east and west
  for (int i = -1; i <= 1; i+=2) {
   std::string const tmpfilename = terrain_filename(loc.x + i, loc.y);
   fin.open(tmpfilename.c_str());
   if (fin.is_open()) {
    fin.close();
    pointers.push_back(new overmap(loc.x + i, loc.y));
   } else
    pointers.push_back(NULL);
  }
// pointers looks like (north, south, west, east)
  generate(pointers[0], pointers[3], pointers[1], pointers[2]);
  for (int i = 0; i < 4; i++)
   delete pointers[i];
  save();
 }
}

std::string overmap::terrain_filename(int const x, int const y) const
{
 std::stringstream filename;

 filename << world_generator->active_world->world_path << "/";

 if (!prefix.empty()) {
  filename << prefix << ".";
 }

 filename << "o." << x << "." << y;

 return filename.str();
}

std::string overmap::player_filename(int const x, int const y) const
{
 std::stringstream filename;

 filename << world_generator->active_world->world_path <<"/" << base64_encode(name) << ".seen." << x << "." << y;

 return filename.str();
}

// Overmap special placement functions // fixme oter_t: std::set<std::string> spec_flags;

bool omspec_place::water(overmap *om, unsigned long f, tripoint p)
{
 int size = 1;
 if (f & (mfb(OMS_FLAG_2X2) | mfb(OMS_FLAG_2X2_SECOND)))
     size = 2;
 else if (f & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND)))
     size = 3;

    for (int x = p.x; x < p.x + size; x++) {
        for (int y = p.y; y < p.y + size; y++) {
            oter_id oter = om->ter(x, y, p.z);
            if (!is_ot_type("river", oter)) {
                return false;
            }
        }
    }
    return true;
}

bool omspec_place::land(overmap *om, unsigned long f, tripoint p)
{
 int size = 1;
 if (f & (mfb(OMS_FLAG_2X2) | mfb(OMS_FLAG_2X2_SECOND)))
     size = 2;
 else if (f & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND)))
     size = 3;

    for (int x = p.x; x < p.x + size; x++) {
        for (int y = p.y; y < p.y + size; y++) {
            oter_id oter = om->ter(x, y, p.z);
            if (is_ot_type("river", oter)) {
                return false;
            }
        }
    }
    return true;
}

bool omspec_place::forest(overmap *om, unsigned long f, tripoint p)
{
 int size = 1;
 if (f & (mfb(OMS_FLAG_2X2) | mfb(OMS_FLAG_2X2_SECOND)))
     size = 2;
 else if (f & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND)))
     size = 3;

    for (int x = p.x; x < p.x + size; x++) {
        for (int y = p.y; y < p.y + size; y++) {
            oter_id oter = om->ter(x, y, p.z);
            if (!is_ot_type("forest", oter)) {
                return false;
            }
        }
    }
    return true;
}

bool omspec_place::wilderness(overmap *om, unsigned long f, tripoint p)
{
 int size = 1;
 if (f & (mfb(OMS_FLAG_2X2) | mfb(OMS_FLAG_2X2_SECOND)))
     size = 2;
 else if (f & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND)))
     size = 3;

    for (int x = p.x; x < p.x + size; x++) {
        for (int y = p.y; y < p.y + size; y++) {
            oter_id oter = om->ter(x, y, p.z);
            if (!is_ot_type("forest", oter) && !is_ot_type("field", oter)) { // fixme
                return false;
            }
        }
    }
    return true;
}

bool omspec_place::by_highway(overmap *om, unsigned long f, tripoint p)
{
 int size = 1;
 if (f & (mfb(OMS_FLAG_2X2) | mfb(OMS_FLAG_2X2_SECOND)))
     size = 2;
 else if (f & (mfb(OMS_FLAG_3X3) | mfb(OMS_FLAG_3X3_FIXED) | mfb(OMS_FLAG_3X3_SECOND)))
     size = 3;

    for (int x = p.x; x < p.x + size; x++) {
        for (int y = p.y; y < p.y + size; y++) {
            oter_id oter = om->ter(x, y, p.z);
            if (!is_ot_type("forest", oter) && !is_ot_type("field", oter)) {
                return false;
            }
        }
    }

 if (size == 3 &&
     !om->is_road_or_highway(p.x + 1, p.y - 1, p.z) &&
     !om->is_road_or_highway(p.x + 3, p.y + 1, p.z) &&
     !om->is_road_or_highway(p.x + 1, p.y + 3, p.z) &&
     !om->is_road_or_highway(p.x - 1, p.y + 1, p.z))
  return false;
 else if (size == 2 &&
          !om->is_road_or_highway(p.x + 1, p.y - 1, p.z) &&
          !om->is_road_or_highway(p.x + 2, p.y + 1, p.z) &&
          !om->is_road_or_highway(p.x, p.y + 2, p.z) &&
          !om->is_road_or_highway(p.x - 1, p.y, p.z))
  return false;
 else if (size == 1 &&
          !om->is_road_or_highway(p.x, p.y - 1, p.z) &&
          !om->is_road_or_highway(p.x, p.y + 1, p.z) &&
          !om->is_road_or_highway(p.x - 1, p.y, p.z) &&
          !om->is_road_or_highway(p.x + 1, p.y, p.z))
  return false;
 return true;
}














#include "omdata.h"

////////////////
oter_iid ot_null,
     ot_crater,
     ot_field,
     ot_forest,
     ot_forest_thick,
     ot_forest_water,
     ot_river_center;


oter_iid oterfind(const std::string id) {
    if( otermap.find(id) == otermap.end() ) {
         debugmsg("Can't find %s",id.c_str());
         return 0;
    }
    return otermap[id].loadid;
};

void set_oter_ids() { // fixme constify
    ot_null = oterfind("");
// NOT required.
    ot_crater = oterfind("crater");
    ot_field = oterfind("field");
    ot_forest = oterfind("forest");
    ot_forest_thick = oterfind("forest_thick");
    ot_forest_water = oterfind("forest_water");
    ot_river_center = oterfind("river_center");
};


//////////////////////////
//// sneaky

   // ter(...) = 0;
   const int& oter_id::operator=(const int& i) {
      _val = i;
      return _val;
   }
   // ter(...) = "rock"
   oter_id::operator std::string() const {
      if ( _val < 0 || _val > oterlist.size() ) {
          debugmsg("oterlist[%d] > %d",_val,oterlist.size()); // remove me after testing (?)
          return 0;
      }
      return std::string(oterlist[_val].id);
   }

   // int index = ter(...);
   oter_id::operator int() const {
      return _val;
   }

   // ter(...) != "foobar"
   bool oter_id::operator!=(const char * v) const {
      return oterlist[_val].id.compare(v) != 0;
/*    hellaciously slow string allocation frenzy -v
      std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
      return ( it == otermap.end() || it->second.loadid != _val);
*/
   }

   // ter(...) == "foobar"
   bool oter_id::operator==(const char * v) const {
      return oterlist[_val].id.compare(v) == 0;
   }
   bool oter_id::operator<=(const char * v) const {
      std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
      return ( it == otermap.end() || it->second.loadid <= _val);
   }
   bool oter_id::operator>=(const char * v) const {
      std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
      return ( it != otermap.end() && it->second.loadid >= _val);
   }

   // o_id1 != o_id2
   bool oter_id::operator!=(const oter_id & v) const {
       return ( _val != v._val );
   }
   bool oter_id::operator==(const oter_id & v) const {
       return ( _val == v._val );
   }

   // oter_t( ter(...) ).name // WARNING
   oter_id::operator oter_t() const {
       return oterlist[_val];
   }

const oter_t & oter_id::t() const {
       return oterlist[_val];
   }
   // ter(...).size()
   int oter_id::size() const {
       return oterlist[_val].id.size();
   }

   // ter(...).find("foo");
   int oter_id::find(const std::string &v, const int start, const int end) const {
       (void)start; (void)end; // TODO?
       return oterlist[_val].id.find(v);//, start, end);
   }
   // ter(...).compare(0, 3, "foo");
   int oter_id::compare(size_t pos, size_t len, const char* s, size_t n) const {
       if ( n != 0 ) {
           return oterlist[_val].id.compare(pos, len, s, n);
       } else {
           return oterlist[_val].id.compare(pos, len, s);
       }
   }

   // std::string("river_ne");  oter_id van_location(down_by);
   oter_id::oter_id(const std::string& v) {
      std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
      if ( it == otermap.end() ) {
          debugmsg("not found: %s",v.c_str());
      } else {
          _val = it->second.loadid;
      }
   }

   // oter_id b("house_north");
   oter_id::oter_id(const char * v) {
      std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
      if ( it == otermap.end() ) {
         debugmsg("not found: %s",v);
      } else {
         _val = it->second.loadid;
      }
   }

  // wprint("%s",ter(...).c_str() );
  const char * oter_id::c_str() const {
      return std::string(oterlist[_val].id).c_str();
  }


void groundcover_extra::setup() { // fixme return bool for failure
    default_ter = terfind( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for ( std::map<std::string, double>::const_iterator it = percent_str.begin(); it != percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) continue;
        if ( termap.find( it->first ) != termap.end() ) {
            tf_id.ter = termap[ it->first ].loadid;
        } else if ( furnmap.find( it->first ) != furnmap.end() ) { 
            tf_id.furn = furnmap[ it->first ].loadid;
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture",it->first.c_str() );
            continue;
        }
        wtotal += (int)(it->second * 10000.0);
        weightlist[ wtotal ] = tf_id;
    }

    for ( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin(); it != boosted_percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) continue;
        if ( termap.find( it->first ) != termap.end() ) {
            tf_id.ter = termap[ it->first ].loadid;
        } else if ( furnmap.find( it->first ) != furnmap.end() ) { 
            tf_id.furn = furnmap[ it->first ].loadid;
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture",it->first.c_str() );
            continue;
        }
        btotal += (int)(it->second * 10000.0);
        boosted_weightlist[ btotal ] = tf_id;
    }

    if ( wtotal > 1000000 ) {
        debugmsg("plant coverage total exceeds 100%%");
    }
    if ( btotal > 1000000 ) {
        debugmsg("boosted plant coverage total exceeds 100%%");
    }

    tf_id.furn = f_null;
    tf_id.ter = default_ter;
    weightlist[ 1000000 ] = tf_id;
    boosted_weightlist[ 1000000 ] = tf_id;

    percent_str.clear();
    boosted_percent_str.clear();
}

ter_furn_id groundcover_extra::pick( bool boosted ) const {
    if ( boosted ) {
        return boosted_weightlist.lower_bound( rng( 0, 1000000 ) )->second;
    }
    return weightlist.lower_bound( rng( 0, 1000000 ) )->second;
}

void regional_settings::setup() {
    if ( default_groundcover_str != NULL ) {
        default_groundcover.primary = terfind(default_groundcover_str->primary_str);
        default_groundcover.secondary = terfind(default_groundcover_str->secondary_str);
        field_coverage.setup();
        city_spec.shops.setup();
        city_spec.parks.setup();
        default_groundcover_str = NULL;
        optionsdata.add_value("DEFAULT_REGION", id );
    }
}
