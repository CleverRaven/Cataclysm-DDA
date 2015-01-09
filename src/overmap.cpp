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
#include "uistate.h"
#define dbg(x) DebugLog((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

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

// Here are the global controls for map-extra spawning.
// The %%% line is chance that a given map square will have an extra
// (higher = less likely) and the individual numbers are the
// relative frequencies of each (higher = more likely).
// Adding or deleting map_extras will affect the amount
// of others, so be careful.
map_extras no_extras(0);
    // Formatting deviates from standard to make the headers read reliably
    // Careful with astyle here, please?
map_extras road_extras(
// %%% HEL MIL SCI BLK DRG SUP PRT MIN CRT FUM 1WY ART
    75, 40, 25, 40, 100, 30, 10, 5, 80, 10,  8,  2,  3);
map_extras field_extras(
// %%% HEL MIL SCI BLK DRG SUP PRT MIN CRT FUM 1WY ART
    90, 40, 8, 20,  0, 20, 10,  3, 50,  10,  8,  1,  3);
map_extras subway_extras(
// %%% HEL MIL SCI BLK DRG SUP PRT MIN CRT FUM 1WY ART
    75,  0,  5, 12,  0,  0,  0,  7,  0,  0, 20,  1,  3);
map_extras build_extras(
// %%% HEL MIL SCI BLK DRG SUP PRT MIN CRT FUM 1WY ART
    90,  0,  5, 12,  0, 0,  0,  5,  5, 60,  8,  1,  3);

std::unordered_map<std::string, oter_t> otermap;
std::vector<oter_t> oterlist;

std::unordered_map<std::string, oter_t> obasetermap;
//const regional_settings default_region_settings;
std::unordered_map<std::string, regional_settings> region_settings_map;

std::vector<overmap_special> overmap_specials;

void load_overmap_specials(JsonObject &jo)
{
    overmap_special spec;

    spec.id = jo.get_string("id");
    JsonArray om_array = jo.get_array("overmaps");
    while(om_array.has_more()) {
        JsonObject om = om_array.next_object();
        overmap_special_terrain terrain;
        JsonArray point = om.get_array("point");
        terrain.p = tripoint(point.get_int(0), point.get_int(1), point.get_int(2));
        terrain.terrain = om.get_string("overmap");
        terrain.connect = om.get_string("connect", "");
        JsonArray flagarray = om.get_array("flags");
        while(flagarray.has_more()) {
            terrain.flags.insert(flagarray.next_string());
        }
        spec.terrains.push_back(terrain);
    }
    JsonArray location_array = jo.get_array("locations");
    while(location_array.has_more()) {
        spec.locations.push_back(location_array.next_string());
    }
    JsonArray city_size_array = jo.get_array("city_sizes");
    if(city_size_array.has_more()) {
        spec.min_city_size = city_size_array.get_int(0);
        spec.max_city_size = city_size_array.get_int(1);
    }

    JsonArray occurrences_array = jo.get_array("occurrences");
    if(occurrences_array.has_more()) {
        spec.min_occurrences = occurrences_array.get_int(0);
        spec.max_occurrences = occurrences_array.get_int(1);
    }

    JsonArray city_distance_array = jo.get_array("city_distance");
    if(city_distance_array.has_more()) {
        spec.min_city_distance = city_distance_array.get_int(0);
        spec.max_city_distance = city_distance_array.get_int(1);
    }

    spec.rotatable = jo.get_bool("rotate", false);
    spec.unique = jo.get_bool("unique", false);
    spec.required = jo.get_bool("required", false);

    if(jo.has_object("spawns")) {
        JsonObject spawns = jo.get_object("spawns");
        spec.spawns.group = spawns.get_string("group");
        spec.spawns.min_population = spawns.get_array("population").get_int(0);
        spec.spawns.max_population = spawns.get_array("population").get_int(1);
        spec.spawns.min_radius = spawns.get_array("radius").get_int(0);
        spec.spawns.max_radius = spawns.get_array("radius").get_int(1);
    }

    JsonArray flag_array = jo.get_array("flags");
    while(flag_array.has_more()) {
        spec.flags.insert(flag_array.next_string());
    }

    overmap_specials.push_back(spec);
}

void clear_overmap_specials()
{
    overmap_specials.clear();
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
oter_id shop(int dir, oter_weight_list &weightlist )
  // todo: rename to something better than 'shop', make it an oter_weight_list method?
{
    if ( dir > 3 ) {
        debugmsg("Bad rotation of weightlist pick: %d.", dir);
        return "";
    }

    dir = dir % 4;
    if (dir < 0) {
        dir += 4;
    }
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

map_extras &get_extras(const std::string &name)
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
bool isroad(std::string bstr)
{
    if (bstr == "road" || bstr == "bridge" ||
        bstr == "subway" || bstr == "sewer" ||
        bstr == "sewage_treatment_hub" ||
        bstr == "sewage_treatment_under" ||
        bstr == "rift" || bstr == "hellmouth") {
        return true;
    }
    return false;
}

void load_oter(oter_t &oter)
{
    oter.loadid = oterlist.size();
    otermap[oter.id] = oter;
    oterlist.push_back(oter);
}

void reset_overmap_terrain()
{
    otermap.clear();
    oterlist.clear();
}

/*
 * load mapgen functions from an overmap_terrain json entry
 * suffix is for roads/subways/etc which have "_straight", "_curved", "_tee", "_four_way" function mappings
 */
void load_overmap_terrain_mapgens(JsonObject &jo, const std::string id_base,
                                  const std::string suffix = "")
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
        int c = 0;
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
        for( auto &sym : syms ) {
            sym = ja.next_int();
        }
        oter.sym = syms[0];
    } else if (oter.rotates) {
        oter.sym = jo.get_int("sym");
        for( auto &sym : syms ) {
            sym = oter.sym;
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

    if( jo.has_object( "spawns" ) ) {
        JsonObject spawns = jo.get_object( "spawns" );
        oter.static_spawns.group = spawns.get_string( "group" );
        oter.static_spawns.min_population = spawns.get_array( "population" ).get_int( 0 );
        oter.static_spawns.max_population = spawns.get_array( "population" ).get_int( 1 );
        oter.static_spawns.chance = spawns.get_int( "chance" );
    }

    oter.is_road = isroad(id_base);
    oter.is_river = (id_base.compare(0, 5, "river", 5) == 0 ||
                     id_base.compare(0, 6, "bridge", 6) == 0);

    oter.id_mapgen = id_base; // What, another identifier? Whyyy...
    if ( ! oter.line_drawing ) { // ...oh
        load_overmap_terrain_mapgens(jo, id_base);
    }

    if (oter.line_drawing) {
        // add variants for line drawing
        for( int i = start_iid; i < start_iid + 12; i++ ) {
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

        for( int i = start_iid; i < start_iid + 5; i++ ) {
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
void finalize_overmap_terrain( )
{
    unsigned c = 0;
    for( std::vector<oter_t>::const_iterator it = oterlist.begin(); it != oterlist.end(); ++it ) {
        if ( (*it).loadid == (*it).loadid_base ) {
            if ( (*it).loadid != c ) { // might as well sanity check while we're here da? da.
                debugmsg("ERROR: oterlist[%d]: mismatch with loadid (%d). (id = %s, id_base = %s)",
                         c, (*it).loadid, (*it).id.c_str(), (*it).id_base.c_str()
                        );
                // aaaaaaaand continue to inevitable crash
            }
            obasetermap.insert( std::pair<std::string, oter_t>( (*it).id_base, oterlist[c] ) );;
        }
        c++;
    }
    // here's another sanity check, yay.
    if ( region_settings_map.find("default") == region_settings_map.end() ) {
        debugmsg("ERROR: can't find default overmap settings (region_map_settings 'default'),"
                 " cataclysm pending. And not the fun kind.");
    }

    for( auto &elem : region_settings_map ) {
        elem.second.setup();
    }
};



void load_region_settings( JsonObject &jo )
{
    regional_settings new_region;
    if ( ! jo.read("id", new_region.id) ) {
        jo.throw_error("No 'id' field.");
    }
    bool strict = ( new_region.id == "default" );
    if ( ! jo.read("default_oter", new_region.default_oter) && strict ) {
        jo.throw_error("default_oter required for default ( though it should probably remain 'field' )");
    }
    if ( jo.has_object("default_groundcover") ) {
        JsonObject jio = jo.get_object("default_groundcover");
        new_region.default_groundcover_str = new sid_or_sid("t_grass", 4, "t_dirt");
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
        if ( strict ) {
            jo.throw_error("\"field_coverage\": { ... } required for default");
        }
    } else {
        JsonObject pjo = jo.get_object("field_coverage");
        double tmpval = 0.0f;
        if ( ! pjo.read("percent_coverage", tmpval) ) {
            pjo.throw_error("field_coverage: percent_coverage required");
        }
        new_region.field_coverage.mpercent_coverage = (int)(tmpval * 10000.0);
        if ( ! pjo.read("default_ter", new_region.field_coverage.default_ter_str) ) {
            pjo.throw_error("field_coverage: default_ter required");
        }
        tmpval = 0.0f;
        if ( pjo.has_object("other") ) {
            JsonObject opjo = pjo.get_object("other");
            std::set<std::string> keys = opjo.get_member_names();
            for( const auto &key : keys ) {
                tmpval = 0.0f;
                if( key != "//" ) {
                    if( opjo.read( key, tmpval ) ) {
                        new_region.field_coverage.percent_str[key] = tmpval;
                    }
                }
            }
        }
        if ( pjo.read("boost_chance", tmpval) && tmpval != 0.0f ) {
            new_region.field_coverage.boost_chance = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_percent_coverage", tmpval) ) {
                pjo.throw_error("boost_chance > 0 requires boosted_percent_coverage");
            }
            new_region.field_coverage.boosted_mpercent_coverage = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_other_percent", tmpval) ) {
                pjo.throw_error("boost_chance > 0 requires boosted_other_percent");
            }
            new_region.field_coverage.boosted_other_mpercent = (int)(tmpval * 10000.0);
            if ( pjo.has_object("boosted_other") ) {
                JsonObject opjo = pjo.get_object("boosted_other");
                std::set<std::string> keys = opjo.get_member_names();
                for( const auto &key : keys ) {
                    tmpval = 0.0f;
                    if( key != "//" ) {
                        if( opjo.read( key, tmpval ) ) {
                            new_region.field_coverage.boosted_percent_str[key] = tmpval;
                        }
                    }
                }
            } else {
                pjo.throw_error("boost_chance > 0 requires boosted_other { ... }");
            }
        }
    }

    if ( ! jo.has_object("city") ) {
        if ( strict ) {
            jo.throw_error("\"city\": { ... } required for default");
        }
    } else {
        JsonObject cjo = jo.get_object("city");
        if ( ! cjo.read("shop_radius", new_region.city_spec.shop_radius) && strict ) {
            jo.throw_error("city: shop_radius required for default");
        }
        if ( ! cjo.read("park_radius", new_region.city_spec.park_radius) && strict ) {
            jo.throw_error("city: park_radius required for default");
        }
        if ( ! cjo.has_object("shops") && strict ) {
            if ( strict ) {
                jo.throw_error("city: \"shops\": { ... } required for default");
            }
        } else {
            JsonObject wjo = cjo.get_object("shops");
            std::set<std::string> keys = wjo.get_member_names();
            for( const auto &key : keys ) {
                if( key != "//" ) {
                    if( wjo.has_int( key ) ) {
                        new_region.city_spec.shops.add_item( key, wjo.get_int( key ) );
                    }
                }
            }
        }
        if ( ! cjo.has_object("parks") && strict ) {
            if ( strict ) {
                jo.throw_error("city: \"parks\": { ... } required for default");
            }
        } else {
            JsonObject wjo = cjo.get_object("parks");
            std::set<std::string> keys = wjo.get_member_names();
            for( const auto &key : keys ) {
                if( key != "//" ) {
                    if( wjo.has_int( key ) ) {
                        new_region.city_spec.parks.add_item( key, wjo.get_int( key ) );
                    }
                }
            }
        }
    }
    region_settings_map[new_region.id] = new_region;
};

void reset_region_settings()
{
    region_settings_map.clear();
}


// *** BEGIN overmap FUNCTIONS ***

overmap::overmap(int const x, int const y)
    : loc(x, y)
    , nullret("")
    , nullbool(false)
    , nullstr("")
{
    // STUB: need region map:
    // settings = regionmap->calculate_settings( loc );
    const std::string rsettings_id = ACTIVE_WORLD_OPTIONS["DEFAULT_REGION"].getValue();
    std::unordered_map<std::string, regional_settings>::const_iterator rsit =
        region_settings_map.find( rsettings_id );

    if ( rsit == region_settings_map.end() ) {
        debugmsg("overmap(%d,%d): can't find region '%s'", x, y, rsettings_id.c_str() ); // gonna die now =[
    }
    settings = rsit->second;

    init_layers();
    open();
}

overmap::~overmap()
{
}

void overmap::init_layers()
{
    for(int z = 0; z < OVERMAP_LAYERS; ++z) {
        oter_id default_type = (z < OVERMAP_DEPTH) ? "rock" : (z == OVERMAP_DEPTH) ? settings.default_oter :
                               "open_air";
        for(int i = 0; i < OMAPX; ++i) {
            for(int j = 0; j < OMAPY; ++j) {
                layer[z].terrain[i][j] = default_type;
                layer[z].visible[i][j] = false;
                layer[z].explored[i][j] = false;
            }
        }
    }
}

oter_id &overmap::ter(const int x, const int y, const int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        nullret = "";
        return nullret;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

const oter_id overmap::get_ter(const int x, const int y, const int z) const
{

    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return nullret;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

bool &overmap::seen(int x, int y, int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].visible[x][y];
}

bool &overmap::explored(int x, int y, int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

// this uses om_pos (overmap tiles, aka levxy / 2)
bool overmap::is_safe(int x, int y, int z)
{
    // use the monsters_at function of the overmapbuffer, which requires *absolute*
    // coordinates. is_safe should be moved to the overmapbuffer, too.
    std::vector<mongroup *> mons = overmap_buffer.monsters_at( x + loc.x * OMAPX, y + loc.y * OMAPY, z );
    if (mons.empty()) {
        return true;
    }

    for (auto &n : mons) {
        if (!n->is_safe()) {
            return false;
        }
    }
    return true;
}

bool overmap::is_explored(int const x, int const y, int const z) const
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY || z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return false;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

bool overmap::has_note(int const x, int const y, int const z) const
{
    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return false;
    }

    for (auto &i : layer[z + OVERMAP_DEPTH].notes) {
        if (i.x == x && i.y == y) {
            return true;
        }
    }
    return false;
}

std::string const &overmap::note(int const x, int const y, int const z) const
{
    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return nullstr;
    }

    for (auto &i : layer[z + OVERMAP_DEPTH].notes) {
        if (i.x == x && i.y == y) {
            return i.text;
        }
    }

    return nullstr;
}

void overmap::add_note(int const x, int const y, int const z, std::string const &message)
{
    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        debugmsg("Attempting to add not to overmap for blank layer %d", z);
        return;
    }

    for (size_t i = 0; i < layer[z + OVERMAP_DEPTH].notes.size(); i++) {
        if (layer[z + OVERMAP_DEPTH].notes[i].x == x && layer[z + OVERMAP_DEPTH].notes[i].y == y) {
            if (message.empty()) {
                layer[z + OVERMAP_DEPTH].notes.erase(layer[z + OVERMAP_DEPTH].notes.begin() + i);
            } else {
                layer[z + OVERMAP_DEPTH].notes[i].text = message;
            }
            return;
        }
    }
    if (message.length() > 0) {
        layer[z + OVERMAP_DEPTH].notes.push_back(om_note(x, y, layer[z + OVERMAP_DEPTH].notes.size(),
                message));
    }
}

extern bool lcmatch(const std::string& text, const std::string& pattern);
std::vector<point> overmap::find_notes(int const z, std::string const &text)
{
    std::vector<point> note_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( auto note : this_layer.notes ) {
        if( lcmatch( note.text, text ) ) {
            note_locations.push_back( point( get_left_border() + note.x, get_top_border() + note.y ) );
        }
    }
    return note_locations;
}

point overmap::display_notes(int z)
{
    const overmapbuffer::t_notes_vector notes = overmap_buffer.get_all_notes(z);
    WINDOW *w_notes = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                              (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                              (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    draw_border(w_notes);

    const std::string title = _("Notes:");
    const std::string back_msg = _("< Prev notes");
    const std::string forward_msg = _("Next notes >");

    // Number of items to show at one time, 2 rows for border, 2 for title & bottom text
    const unsigned maxitems = FULL_SCREEN_HEIGHT - 4;
    int ch = '.';
    unsigned start = 0;
    const int back_len = utf8_width(back_msg.c_str());
    bool redraw = true;
    point result(-1, -1);

    mvwprintz(w_notes, 1, 1, c_ltgray, title.c_str());
    do {
        if (redraw) {
            for (int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++) {
                for (int j = 1; j < FULL_SCREEN_WIDTH - 1; j++) {
                    mvwputch(w_notes, i, j, c_black, ' ');
                }
            }
            for (unsigned i = 0; i < maxitems; i++) {
                const unsigned cur_it = start + i;
                if (cur_it >= notes.size()) {
                    continue;
                }
                // Print letter ('a' <=> cur_it == start)
                mvwputch (w_notes, i + 2, 1, c_white, 'a' + i);
                mvwprintz(w_notes, i + 2, 3, c_ltgray, "- %s", notes[cur_it].second.c_str());
            }
            if (start >= maxitems) {
                mvwprintw(w_notes, maxitems + 2, 1, back_msg.c_str());
            }
            if (start + maxitems < notes.size()) {
                mvwprintw(w_notes, maxitems + 2, 2 + back_len, forward_msg.c_str());
            }
            mvwprintz(w_notes, 1, 40, c_white, _("Press letter to center on note"));
            mvwprintz(w_notes, FULL_SCREEN_HEIGHT - 2, 40, c_white, _("Spacebar - Return to map  "));
            wrefresh(w_notes);
            redraw = false;
        }
        ch = getch();
        if (ch == '<' && start >= maxitems) {
            start -= maxitems;
            redraw = true;
        } else if (ch == '>' && start + maxitems < notes.size()) {
            start += maxitems;
            redraw = true;
        } else if(ch >= 'a' && ch <= 'z') {
            const unsigned chosen = ch - 'a' + start;
            if (chosen < notes.size()) {
                result = notes[chosen].first;
                break; // -> return result
            }
        }
    } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    delwin(w_notes);
    return result;
}

void overmap::generate(const overmap *north, const overmap *east,
                       const overmap *south, const overmap *west)
{
    dbg(D_INFO) << "overmap::generate start...";
    std::vector<city> road_points; // cities and roads_out together
    std::vector<point> river_start;// West/North endpoints of rivers
    std::vector<point> river_end; // East/South endpoints of rivers

    // Determine points where rivers & roads should connect w/ adjacent maps
    const oter_id river_center("river_center"); // optimized comparison.

    if (north != NULL) {
        for (int i = 2; i < OMAPX - 2; i++) {
            if (is_river(north->get_ter(i, OMAPY - 1, 0))) {
                ter(i, 0, 0) = river_center;
            }
            if (is_river(north->get_ter(i, OMAPY - 1, 0)) &&
                is_river(north->get_ter(i - 1, OMAPY - 1, 0)) &&
                is_river(north->get_ter(i + 1, OMAPY - 1, 0))) {
                if (river_start.empty() ||
                    river_start[river_start.size() - 1].x < i - 6) {
                    river_start.push_back(point(i, 0));
                }
            }
        }
        for (auto &i : north->roads_out) {
            if (i.y == OMAPY - 1) {
                roads_out.push_back(city(i.x, 0, 0));
            }
        }
    }
    size_t rivers_from_north = river_start.size();
    if (west != NULL) {
        for (int i = 2; i < OMAPY - 2; i++) {
            if (is_river(west->get_ter(OMAPX - 1, i, 0))) {
                ter(0, i, 0) = river_center;
            }
            if (is_river(west->get_ter(OMAPX - 1, i, 0)) &&
                is_river(west->get_ter(OMAPX - 1, i - 1, 0)) &&
                is_river(west->get_ter(OMAPX - 1, i + 1, 0))) {
                if (river_start.size() == rivers_from_north ||
                    river_start[river_start.size() - 1].y < i - 6) {
                    river_start.push_back(point(0, i));
                }
            }
        }
        for (auto &i : west->roads_out) {
            if (i.x == OMAPX - 1) {
                roads_out.push_back(city(0, i.y, 0));
            }
        }
    }
    if (south != NULL) {
        for (int i = 2; i < OMAPX - 2; i++) {
            if (is_river(south->get_ter(i, 0, 0))) {
                ter(i, OMAPY - 1, 0) = river_center;
            }
            if (is_river(south->get_ter(i,     0, 0)) &&
                is_river(south->get_ter(i - 1, 0, 0)) &&
                is_river(south->get_ter(i + 1, 0, 0))) {
                if (river_end.empty() ||
                    river_end[river_end.size() - 1].x < i - 6) {
                    river_end.push_back(point(i, OMAPY - 1));
                }
            }
            if (south->get_ter(i, 0, 0) == "road_nesw") {
                roads_out.push_back(city(i, OMAPY - 1, 0));
            }
        }
        for (auto &i : south->roads_out) {
            if (i.y == 0) {
                roads_out.push_back(city(i.x, OMAPY - 1, 0));
            }
        }
    }
    size_t rivers_to_south = river_end.size();
    if (east != NULL) {
        for (int i = 2; i < OMAPY - 2; i++) {
            if (is_river(east->get_ter(0, i, 0))) {
                ter(OMAPX - 1, i, 0) = river_center;
            }
            if (is_river(east->get_ter(0, i, 0)) &&
                is_river(east->get_ter(0, i - 1, 0)) &&
                is_river(east->get_ter(0, i + 1, 0))) {
                if (river_end.size() == rivers_to_south ||
                    river_end[river_end.size() - 1].y < i - 6) {
                    river_end.push_back(point(OMAPX - 1, i));
                }
            }
            if (east->get_ter(0, i, 0) == "road_nesw") {
                roads_out.push_back(city(OMAPX - 1, i, 0));
            }
        }
        for (auto &i : east->roads_out) {
            if (i.x == 0) {
                roads_out.push_back(city(OMAPX - 1, i.y, 0));
            }
        }
    }

    // Even up the start and end points of rivers. (difference of 1 is acceptable)
    // Also ensure there's at least one of each.
    std::vector<point> new_rivers;
    if (north == NULL || west == NULL) {
        while (river_start.empty() || river_start.size() + 1 < river_end.size()) {
            new_rivers.clear();
            if (north == NULL) {
                new_rivers.push_back( point(rng(10, OMAPX - 11), 0) );
            }
            if (west == NULL) {
                new_rivers.push_back( point(0, rng(10, OMAPY - 11)) );
            }
            river_start.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
        }
    }
    if (south == NULL || east == NULL) {
        while (river_end.empty() || river_end.size() + 1 < river_start.size()) {
            new_rivers.clear();
            if (south == NULL) {
                new_rivers.push_back( point(rng(10, OMAPX - 11), OMAPY - 1) );
            }
            if (east == NULL) {
                new_rivers.push_back( point(OMAPX - 1, rng(10, OMAPY - 11)) );
            }
            river_end.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
        }
    }

    // Now actually place those rivers.
    if (river_start.size() > river_end.size() && !river_end.empty()) {
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
    } else if (river_end.size() > river_start.size() && !river_start.empty()) {
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
    } else if (!river_end.empty()) {
        if (river_start.size() != river_end.size())
            river_start.push_back( point(rng(OMAPX * .25, OMAPX * .75),
                                         rng(OMAPY * .25, OMAPY * .75)));
        for (size_t i = 0; i < river_start.size(); i++) {
            place_river(river_start[i], river_end[i]);
        }
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
            do {
                tmp = rng(10, OMAPX - 11);
            } while (is_river(ter(tmp, 0, 0)) || is_river(ter(tmp - 1, 0, 0)) ||
                     is_river(ter(tmp + 1, 0, 0)) );
            viable_roads.push_back(city(tmp, 0, 0));
        }
        if (east == NULL) {
            do {
                tmp = rng(10, OMAPY - 11);
            } while (is_river(ter(OMAPX - 1, tmp, 0)) || is_river(ter(OMAPX - 1, tmp - 1, 0)) ||
                     is_river(ter(OMAPX - 1, tmp + 1, 0)));
            viable_roads.push_back(city(OMAPX - 1, tmp, 0));
        }
        if (south == NULL) {
            do {
                tmp = rng(10, OMAPX - 11);
            } while (is_river(ter(tmp, OMAPY - 1, 0)) || is_river(ter(tmp - 1, OMAPY - 1, 0)) ||
                     is_river(ter(tmp + 1, OMAPY - 1, 0)));
            viable_roads.push_back(city(tmp, OMAPY - 1, 0));
        }
        if (west == NULL) {
            do {
                tmp = rng(10, OMAPY - 11);
            } while (is_river(ter(0, tmp, 0)) || is_river(ter(0, tmp - 1, 0)) ||
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
    for (auto &i : roads_out) {
        road_points.push_back(i);
    }
    for (auto &i : cities) {
        road_points.push_back(i);
    }
    // And finally connect them via "highways"
    place_hiways(road_points, 0, "road");
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
            for( auto &elem : skip_above ) {
                if( oter_above == elem ) {
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
            } else if (oter_above == "road_nesw_manhole") {
                ter(i, j, z) = "sewer_nesw";
                sewer_points.push_back(city(i, j, 0));
            } else if (oter_above == "sewage_treatment") {
                sewer_points.push_back(city(i, j, 0));
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
                add_mon_group(mongroup("GROUP_ANT", i * 2, j * 2, z, size * 1.5, rng(6000, 8000)));
            } else if (oter_above == "slimepit_down") {
                int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
                goo_points.push_back(city(i, j, size));
            } else if (oter_above == "forest_water") {
                ter(i, j, z) = "cavern";
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
                add_mon_group(mongroup("GROUP_SPIRAL", i * 2, j * 2, z, 2, 200));
            } else if (oter_above == "silo") {
                if (rng(2, 7) < abs(z) || rng(2, 7) < abs(z)) {
                    ter(i, j, z) = "silo_finale";
                } else {
                    ter(i, j, z) = "silo";
                    requires_sub = true;
                }
            }
        }
    }

    for (auto &i : goo_points) {
        requires_sub |= build_slimepit(i.x, i.y, z, i.s);
    }
    place_hiways(sewer_points, z, "sewer");
    polish(z, "sewer");
    place_hiways(subway_points, z, "subway");
    for (auto &i : subway_points) {
        ter(i.x, i.y, z) = "subway_station";
    }
    for (auto &i : lab_points) {
        bool lab = build_lab(i.x, i.y, z, i.s);
        requires_sub |= lab;
        if (!lab && ter(i.x, i.y, z) == "lab_core") {
            ter(i.x, i.y, z) = "lab";
        }
    }
    for (auto &i : ice_lab_points) {
        bool ice_lab = build_ice_lab(i.x, i.y, z, i.s);
        requires_sub |= ice_lab;
        if (!ice_lab && ter(i.x, i.y, z) == "ice_lab_core") {
            ter(i.x, i.y, z) = "ice_lab";
        }
    }
    for (auto &i : ant_points) {
        build_anthill(i.x, i.y, z, i.s);
    }
    polish(z, "subway");
    polish(z, "ants");

    for (auto &i : cities) {
        if (one_in(3)) {
            add_mon_group(mongroup("GROUP_CHUD", i.x * 2, i.y * 2, z, i.s, i.s * 20));
        }
        if (!one_in(8)) {
            add_mon_group(mongroup("GROUP_SEWER", i.x * 2, i.y * 2, z, i.s * 3.5, i.s * 70));
        }
    }

    place_rifts(z);
    for (auto &i : mine_points) {
        build_mine(i.x, i.y, z, i.s);
    }

    for (auto &i : shaft_points) {
        ter(i.x, i.y, z) = "mine_shaft";
        requires_sub = true;
    }
    return requires_sub;
}

std::vector<point> overmap::find_terrain(const std::string &term, int zlevel)
{
    std::vector<point> found;
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            if (seen(x, y, zlevel) &&
                lcmatch( otermap[ter(x, y, zlevel)].name, term ) ) {
                found.push_back( point( get_left_border() + x, get_top_border() + y) );
            }
        }
    }
    return found;
}

int overmap::closest_city(point p)
{
    int distance = 999;
    size_t ret = -1;
    for (size_t i = 0; i < cities.size(); i++) {
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
    if (city_id < 0 || size_t(city_id) >= cities.size()) {
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
    for (auto &i : cities) {
        int dist = rl_dist(p.x, p.y, i.x, i.y);
        dist -= i.s;
        if (dist < distance) {
            distance = dist;
        }
    }
    return distance;
}

void overmap::draw(WINDOW *w, WINDOW *wbar, const tripoint &center,
                   const tripoint &orig, bool blink, bool show_explored,
                   input_context *inp_ctxt,
                   bool debug_monstergroups,
                   const int iZoneIndex)
{
    const int z = center.z;
    const int cursx = center.x;
    const int cursy = center.y;
    const int om_map_width = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;


    // Target of current mission
    point target;
    bool has_target = false;
    if (g->u.active_mission >= 0 && size_t(g->u.active_mission) < g->u.active_missions.size()) {
        target = g->find_mission(g->u.active_missions[g->u.active_mission])->target;
        has_target = target != overmap::invalid_point;
    }
    // seen status & terrain of center position
    bool csee = false;
    oter_id ccur_ter = "";
    // used inside the loop
    oter_id cur_ter = ot_null;
    nc_color ter_color;
    long ter_sym;
    // sight_points is hoisted for speed reasons.
    int sight_points = g->u.overmap_sight_range(g->light_level());

    std::string sZoneName = "";
    tripoint tripointZone = tripoint(-1, -1, -1);

    if (iZoneIndex != -1) {
        sZoneName = g->u.Zones.vZones[iZoneIndex].getName();
        point pOMZone = overmapbuffer::ms_to_omt_copy(g->u.Zones.vZones[iZoneIndex].getCenterPoint());
        tripointZone = tripoint(pOMZone.x, pOMZone.y);
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = NULL;
    std::vector<mongroup *> mgroups;
    if(debug_monstergroups) {
        mgroups = overmap_buffer.monsters_at( center.x, center.y, center.z );
        for( const auto &mgp : mgroups ) {
            mgroup = mgp;
            if( mgp->horde ) {
                break;
            }
        }
    }

    for (int i = 0; i < om_map_width; i++) {
        for (int j = 0; j < om_map_height; j++) {
            const int omx = cursx + i - (om_map_width / 2);
            const int omy = cursy + j - (om_map_height / 2);
            const bool see = overmap_buffer.seen(omx, omy, z);
            bool los = false;
            if (see) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter(omx, omy, z);

                // Check if location is within player line-of-sight
                if (g->u.overmap_los(omx, omy, sight_points)) {
                    los = true;
                }
            }
            //Check if there is an npc.
            const bool npc_here = overmap_buffer.has_npc(omx, omy, z);
            // Check for hordes within player line-of-sight
            bool horde_here = false;
            if (los) {
                std::vector<mongroup *> hordes = overmap_buffer.monsters_at(omx, omy, z);
                for (const auto &ih : hordes) {
                    if (ih->horde) {
                        horde_here = true;
                    }
                }
            }
            // and a vehicle
            const bool veh_here = overmap_buffer.has_vehicle(omx, omy, z);
            if (blink && omx == orig.x && omy == orig.y && z == orig.z) {
                // Display player pos, should always be visible
                ter_color = g->u.symbol_color();
                ter_sym = '@';
            } else if (blink && has_target && omx == target.x && omy == target.y && z == 0) {
                // TODO: mission targets currently have no z-component, are assumed to be on z=0
                // Mission target, display always, player should know where it is anyway.
                ter_color = c_red;
                ter_sym = '*';
            } else if (blink && overmap_buffer.has_note(omx, omy, z)) {
                // Display notes in all situations, even when not seen
                ter_color = c_yellow;
                const std::string &note_text = overmap_buffer.note(omx, omy, z);
                if (note_text.length() >= 2 && note_text[1] == ';') {
                    if (note_text[0] == 'r') {
                        ter_color = c_ltred;
                    }
                    if (note_text[0] == 'R') {
                        ter_color = c_red;
                    }
                    if (note_text[0] == 'g') {
                        ter_color = c_ltgreen;
                    }
                    if (note_text[0] == 'G') {
                        ter_color = c_green;
                    }
                    if (note_text[0] == 'b') {
                        ter_color = c_ltblue;
                    }
                    if (note_text[0] == 'B') {
                        ter_color = c_blue;
                    }
                    if (note_text[0] == 'W') {
                        ter_color = c_white;
                    }
                    if (note_text[0] == 'C') {
                        ter_color = c_cyan;
                    }
                    if (note_text[0] == 'P') {
                        ter_color = c_pink;
                    }
                } else if (note_text.length() >= 4 && note_text[3] == ';') {
                    if (note_text[2] == 'r') {
                        ter_color = c_ltred;
                    }
                    if (note_text[2] == 'R') {
                        ter_color = c_red;
                    }
                    if (note_text[2] == 'g') {
                        ter_color = c_ltgreen;
                    }
                    if (note_text[2] == 'G') {
                        ter_color = c_green;
                    }
                    if (note_text[2] == 'b') {
                        ter_color = c_ltblue;
                    }
                    if (note_text[2] == 'B') {
                        ter_color = c_blue;
                    }
                    if (note_text[2] == 'W') {
                        ter_color = c_white;
                    }
                    if (note_text[2] == 'C') {
                        ter_color = c_cyan;
                    }
                    if (note_text[2] == 'P') {
                        ter_color = c_pink;
                    }
                } else {
                    ter_color = c_yellow;
                }

                if (note_text.length() >= 2 && note_text[1] == ':') {
                    ter_sym = note_text[0];
                } else if (note_text.length() >= 4 && note_text[3] == ':') {
                    ter_sym = note_text[2];
                } else {
                    ter_sym = 'N';
                }
            } else if (!see) {
                // All cases above ignore the seen-status,
                ter_color = c_dkgray;
                ter_sym = '#';
                // All cases below assume that see is true.
            } else if (blink && npc_here) {
                // Display NPCs only when player can see the location
                ter_color = c_pink;
                ter_sym = '@';
            } else if (blink && horde_here) {
                // Display Hordes only when within player line-of-sight
                ter_color = c_green;
                ter_sym = 'Z';
            } else if (blink && veh_here) {
                // Display Vehicles only when player can see the location
                ter_color = c_cyan;
                ter_sym = 'c';
            } else {
                if (sZoneName != "" && tripointZone.x == omx && tripointZone.y == omy) {
                    ter_color = c_yellow;
                    ter_sym = 'Z';
                } else {
                    // Nothing special, but is visible to the player.
                    if (otermap.find(cur_ter) == otermap.end()) {
                        debugmsg("Bad ter %s (%d, %d)", cur_ter.c_str(), omx, omy);
                        ter_color = c_red;
                        ter_sym = '?';
                    } else {
                        // Map tile marked as explored
                        if (show_explored && overmap_buffer.is_explored(omx, omy, z)) {
                            ter_color = c_dkgray;
                        } else {
                            ter_color = otermap[cur_ter].color;
                        }
                        ter_sym = otermap[cur_ter].sym;
                    }
                }
            }

            // Are we debugging monster groups?
            if(blink && debug_monstergroups) {
                // Check if this tile is the target of the currently selected group
                if(mgroup && mgroup->tx / 2 == omx && mgroup->ty / 2 == omy) {
                    ter_color = c_red;
                    ter_sym = 'x';
                } else {
                    const auto &groups = overmap_buffer.monsters_at( omx, omy, center.z );
                    for( auto &mgp : groups ) {
                        if( mgp->type == "GROUP_FOREST" ) {
                            // Don't flood the map with forest creatures.
                            continue;
                        }
                        if( mgp->horde ) {
                            // Hordes show as +
                            ter_sym = '+';
                            break;
                        } else {
                            // Regular groups show as -
                            ter_sym = '-';
                        }
                    }
                    // Set the color only if we encountered an eligible group.
                    if( ter_sym == '+' || ter_sym == '-' ) {
                        if( los ) {
                            ter_color = c_ltblue;
                        } else {
                            ter_color = c_blue;
                        }
                    }
                }
            }

            if (omx == cursx && omy == cursy) {
                csee = see;
                ccur_ter = cur_ter;
                mvwputch_hi(w, j, i, ter_color, ter_sym);
            } else {
                mvwputch(w, j, i, ter_color, ter_sym);
            }
        }
    }
    if (has_target && blink &&
        (target.x < cursx - om_map_height / 2 ||
         target.x > cursx + om_map_height / 2  ||
         target.y < cursy - om_map_width / 2 ||
         target.y > cursy + om_map_width / 2)) {
        // TODO: mission targets currently have no z-component, are assumed to be on z=0
        switch (direction_from(cursx, cursy, target.x, target.y)) {
        case NORTH:
            mvwputch(w, 0, om_map_width / 2, c_red, '^');
            break;
        case NORTHEAST:
            mvwputch(w, 0, om_map_width - 1, c_red, LINE_OOXX);
            break;
        case EAST:
            mvwputch(w, om_map_height / 2, om_map_width - 1, c_red, '>');
            break;
        case SOUTHEAST:
            mvwputch(w, om_map_height, om_map_width - 1, c_red, LINE_XOOX);
            break;
        case SOUTH:
            mvwputch(w, om_map_height, om_map_height / 2, c_red, 'v');
            break;
        case SOUTHWEST:
            mvwputch(w, om_map_height, 0, c_red, LINE_XXOO);
            break;
        case WEST:
            mvwputch(w, om_map_height / 2,  0, c_red, '<');
            break;
        case NORTHWEST:
            mvwputch(w,  0,  0, c_red, LINE_OXXO);
            break;
        default:
            break; //Do nothing
        }
    }

    std::string note_text = overmap_buffer.note(cursx, cursy, z);
    if ((note_text.length() >= 4 && note_text[3] == ':') || (note_text.length() >= 4 &&
            note_text[3] == ';')) {
        note_text.erase(0, 4);
    } else if ((note_text.length() >= 2 && note_text[1] == ':') || (note_text.length() >= 2 &&
               note_text[1] == ';') ) {
        note_text.erase(0, 2);
    }
    std::vector<std::string> corner_text;
    if (!note_text.empty()) {
        corner_text.push_back( note_text );
    }
    const auto npcs = overmap_buffer.get_npcs_near_omt(cursx, cursy, z, 0);
    for( const auto &npc : npcs ) {
        if( npc->marked_for_death ) {
            continue;
        }
        corner_text.push_back( npc->name );
    }
    const auto vehicles = overmap_buffer.get_vehicle(cursx, cursy, z);
    for( const auto &v : vehicles ) {
        corner_text.push_back( v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for( const auto &line : corner_text ) {
            maxlen = std::max( maxlen, utf8_width( line.c_str() ) );
        }
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            // clear line, print line, print vertical line at the right side.
            mvwprintz( w, i, 0, c_yellow, std::string(maxlen, ' ').c_str() );
            mvwprintz( w, i, 0, c_yellow, "%s", corner_text[i].c_str() );
            mvwputch( w, i, maxlen, c_white, LINE_XOXO );
        }
        for (int i = 0; i <= maxlen; i++) {
            mvwputch( w, corner_text.size(), i, c_white, LINE_OXOX );
        }
        mvwputch( w, corner_text.size(), maxlen, c_white, LINE_XOOX );
    }

    if (sZoneName != "" && tripointZone.x == cursx && tripointZone.y == cursy) {
        std::string sTemp = _("Zone:");
        sTemp += " " + sZoneName;

        const int length = utf8_width(sTemp.c_str());
        for (int i = 0; i <= length; i++) {
            mvwputch(w, om_map_height-2, i, c_white, LINE_OXOX);
        }

        mvwprintz(w, om_map_height-1, 0, c_yellow, "%s", sTemp.c_str());
        mvwputch(w, om_map_height-2, length, c_white, LINE_OOXX);
        mvwputch(w, om_map_height-1, length, c_white, LINE_XOXO);
    }

    // Draw the vertical line
    for (int j = 0; j < TERMY; j++) {
        mvwputch(wbar, j, 0, c_white, LINE_XOXO);
    }

    // Clear the legend
    for (int i = 1; i < 55; i++) {
        for (int j = 0; j < TERMY; j++) {
            mvwputch(wbar, j, i, c_black, ' ');
        }
    }

    // Draw text describing the overmap tile at the cursor position.
    if (csee) {
        if(!mgroups.empty()) {
            int line_number = 1;
            for( const auto &mgroup : mgroups ) {
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "  Species: %s", mgroup->type.c_str());
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "# monsters: %d", mgroup->population);
                if( !mgroup->horde ) {
                    continue;
                }
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "  Interest: %d", mgroup->interest);
                mvwprintz(wbar, line_number, 3,
                          c_blue, "  Target: %d, %d", mgroup->tx, mgroup->ty);
                mvwprintz(wbar, line_number++, 3,
                          c_red, "x");
            }
        } else {
            mvwputch(wbar, 1, 1, otermap[ccur_ter].color, otermap[ccur_ter].sym);
            std::vector<std::string> name = foldstring(otermap[ccur_ter].name, 25);
            for (size_t i = 0; i < name.size(); i++) {
                mvwprintz(wbar, i + 1, 3, otermap[ccur_ter].color, "%s", name[i].c_str());
            }
        }
    } else {
        mvwprintz(wbar, 1, 1, c_dkgray, _("# Unexplored"));
    }

    if (has_target) {
        // TODO: mission targets currently have no z-component, are assumed to be on z=0
        int distance = rl_dist(orig.x, orig.y, target.x, target.y);
        mvwprintz(wbar, 3, 1, c_white, _("Distance to target: %d"), distance);
    }
    mvwprintz(wbar, 14, 1, c_magenta, _("Use movement keys to pan."));
    if (inp_ctxt != NULL) {
        mvwprintz(wbar, 15, 1, c_magenta, (inp_ctxt->get_desc("CENTER") +
                  _(" - Center map on character")).c_str());
        mvwprintz(wbar, 16, 1, c_magenta, (inp_ctxt->get_desc("SEARCH") +
                  _(" - Search")).c_str());
        mvwprintz(wbar, 17, 1, c_magenta, (inp_ctxt->get_desc("CREATE_NOTE") +
                  _(" - Add/Edit a note")).c_str());
        mvwprintz(wbar, 18, 1, c_magenta, (inp_ctxt->get_desc("DELETE_NOTE") +
                  _(" - Delete a note")).c_str());
        mvwprintz(wbar, 19, 1, c_magenta, (inp_ctxt->get_desc("LIST_NOTES") +
                  _(" - List notes")).c_str());
        mvwprintz(wbar, 20, 1, c_magenta, (inp_ctxt->get_desc("TOGGLE_BLINKING") +
                  _(" - Toggle Blinking")).c_str());
        mvwprintz(wbar, 21, 1, c_magenta, (inp_ctxt->get_desc("TOGGLE_OVERLAYS") +
                  _(" - Toggle Overlays")).c_str());
        mvwprintz(wbar, 22, 1, c_magenta, (inp_ctxt->get_desc("TOGGLE_EXPLORED") +
                  _(" - Toggle Explored")).c_str());
        mvwprintz(wbar, 23, 1, c_magenta, (inp_ctxt->get_desc("HELP_KEYBINDINGS") +
                  _(" - Change keys")).c_str());
        fold_and_print(wbar, 24, 1, 27, c_magenta, ("m, " + inp_ctxt->get_desc("QUIT") +
                       _(" - Return to game")).c_str());
    }
    point omt(cursx, cursy);
    const point om = overmapbuffer::omt_to_om_remain(omt);
    mvwprintz(wbar, getmaxy(wbar) - 1, 1, c_red,
              _("LEVEL %i, %d'%d, %d'%d"), z, om.x, omt.x, om.y, omt.y);
    // Done with all drawing!
    wrefresh(w);
    wrefresh(wbar);
}

tripoint overmap::draw_overmap()
{
    return draw_overmap(g->om_global_location());
}

tripoint overmap::draw_overmap(int z)
{
    tripoint loc = g->om_global_location();
    loc.z = z;
    return draw_overmap(loc);
}

//Start drawing the overmap on the screen using the (m)ap command.
tripoint overmap::draw_overmap(const tripoint &orig, bool debug_mongroup, const tripoint &select, const int iZoneIndex)
{
    delwin(g->w_omlegend);
    g->w_omlegend = newwin(TERMY, 28, 0, TERMX - 28);
    delwin(g->w_overmap);
    g->w_overmap = newwin(OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, 0, 0);

    // Draw black padding space to avoid gap between map and legend
    delwin(g->w_blackspace);
    g->w_blackspace = newwin(TERMY, TERMX - 28, 0, 0);
    mvwputch(g->w_blackspace, 0, 0, c_black, ' ');
    wrefresh(g->w_blackspace);

    tripoint ret = invalid_tripoint;
    tripoint curs(orig);

    if (select.x != -1 && select.y != -1 && select.z != -1) {
        curs = tripoint(select);
    }

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
    ictxt.register_action("TOGGLE_BLINKING");
    ictxt.register_action("TOGGLE_OVERLAYS");
    ictxt.register_action("TOGGLE_EXPLORED");
    ictxt.register_action("QUIT");
    std::string action;
    do {
        timeout( BLINK_SPEED );
        bool show_explored = uistate.overmap_blinking || uistate.overmap_show_overlays;
        draw(g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays, show_explored, &ictxt, debug_mongroup, iZoneIndex);
        action = ictxt.handle_input();
        timeout(-1);

        int dirx, diry;
        if (ictxt.get_direction(dirx, diry, action)) {
            curs.x += dirx;
            curs.y += diry;
        } else if (action == "CENTER") {
            curs = orig;
        } else if (action == "LEVEL_DOWN" && curs.z > -OVERMAP_DEPTH) {
            curs.z -= 1;
        } else if (action == "LEVEL_UP" && curs.z < OVERMAP_HEIGHT) {
            curs.z += 1;
        } else if (action == "CONFIRM") {
            ret = tripoint(curs.x, curs.y, curs.z);
        } else if (action == "QUIT") {
            ret = invalid_tripoint;
        } else if (action == "CREATE_NOTE") {
            const std::string old_note = overmap_buffer.note(curs);
            const std::string new_note = string_input_popup(_("Note (X:TEXT for custom symbol, G; for color):"),
                                         45, old_note); // 45 char max
            if(old_note != new_note) {
                overmap_buffer.add_note(curs, new_note);
            }
        } else if(action == "DELETE_NOTE") {
            if (overmap_buffer.has_note(curs) &&
                query_yn(_("Really delete note?"))) {
                overmap_buffer.delete_note(curs);
            }
        } else if (action == "LIST_NOTES") {
            const point p = display_notes(curs.z);
            if (p.x != -1 && p.y != -1) {
                curs.x = p.x;
                curs.y = p.y;
            }
        } else if (action == "TOGGLE_BLINKING") {
            uistate.overmap_blinking = !uistate.overmap_blinking;
        } else if (action == "TOGGLE_OVERLAYS") {
            uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
        } else if (action == "TOGGLE_EXPLORED") {
            overmap_buffer.toggle_explored(curs.x, curs.y, curs.z);
        } else if (action == "SEARCH") {
            std::string term = string_input_popup(_("Search term:"));
            if(term.empty()) {
                continue;
            }
            std::transform( term.begin(), term.end(), term.begin(), tolower );

            // This is on purpose only the current overmap, otherwise
            // it would contain way to many entries
            overmap &om = overmap_buffer.get_om_global(point(curs.x, curs.y));
            std::vector<point> locations = om.find_notes(curs.z, term);
            std::vector<point> terlist = om.find_terrain(term, curs.z);
            locations.insert( locations.end(), terlist.begin(), terlist.end() );
            if( locations.empty() ) {
                continue;
            }
            int i = 0;
            //Navigate through results
            tripoint tmp = curs;
            WINDOW *w_search = newwin(13, 27, 3, TERMX - 27);
            input_context ctxt("OVERMAP_SEARCH");
            ctxt.register_action("NEXT_TAB", _("Next target"));
            ctxt.register_action("PREV_TAB", _("Previous target"));
            ctxt.register_action("QUIT");
            ctxt.register_action("CONFIRM");
            ctxt.register_action("HELP_KEYBINDINGS");
            ctxt.register_action("ANY_INPUT");
            do {
                tmp.x = locations[i].x;
                tmp.y = locations[i].y;
                draw(g->w_overmap, g->w_omlegend, tmp, orig, uistate.overmap_show_overlays, show_explored, NULL);
                //Draw search box
                draw_border(w_search);
                mvwprintz(w_search, 1, 1, c_red, _("Find place:"));
                mvwprintz(w_search, 2, 1, c_ltblue, "                         ");
                mvwprintz(w_search, 2, 1, c_ltblue, "%s", term.c_str());
                mvwprintz(w_search, 4, 1, c_white, _("'<' '>' Cycle targets."));
                mvwprintz(w_search, 10, 1, c_white, _("Enter/Spacebar to select."));
                mvwprintz(w_search, 11, 1, c_white, _("q or ESC to return."));
                wrefresh(w_search);
                timeout( BLINK_SPEED );
                action = ctxt.handle_input();
                timeout(-1);
                if (uistate.overmap_blinking) {
                    uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                }
                if (action == "NEXT_TAB") {
                    i = (i + 1) % locations.size();
                } else if (action == "PREV_TAB") {
                    i = (i + locations.size() - 1) % locations.size();
                } else if (action == "CONFIRM") {
                    curs = tmp;
                }
            } while(action != "CONFIRM" && action != "QUIT");
            delwin(w_search);
            action = "";
        } else if (action == "TIMEOUT") {
            if (uistate.overmap_blinking) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        } else if (action == "ANY_INPUT") {
            if (uistate.overmap_blinking) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
            input_event e = ictxt.get_raw_input();
            if(e.type == CATA_INPUT_KEYBOARD && e.get_first_input() == 'm') {
                action = "QUIT";
            }
        }
    } while (action != "QUIT" && action != "CONFIRM");
    werase(g->w_overmap);
    werase(g->w_omlegend);
    erase();
    g->refresh_all();
    return ret;
}

tripoint overmap::find_random_omt( const std::string &omt_base_type ) const
{
    std::vector<tripoint> valid;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                if( get_ter( i, j, k ).t().id_base == omt_base_type ) {
                    valid.push_back( tripoint( i, j, k ) );
                }
            }
        }
    }
    if( valid.empty() ) {
        return invalid_tripoint;
    }
    return valid[rng( 0, valid.size() - 1 )];
}

void overmap::process_mongroups()
{
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( mg.dying ) {
            mg.population *= .8;
            mg.radius *= .9;
        }
        if( mg.population <= 0 ) {
            zg.erase( it++ );
        } else {
            ++it;
        }
    }
}

void mongroup::wander()
{
    // TODO: More interesting stuff possible, like looking for nearby shelter.
    // What a monster thinks of as shelter is another matter...
    tx += rng( -10, 10 );
    ty += rng( -10, 10 );
    interest = 30;
}

void overmap::move_hordes()
{
    // Prevent hordes to be moved twice by putting them in here after moving.
    decltype(zg) tmpzg;
    //MOVE ZOMBIE GROUPS
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( !mg.horde ) {
            ++it;
            continue;
        }
        if( rng(0, 100) < mg.interest ) {
            // TODO: Adjust for monster speed.
            // TODO: Handle moving to adjacent overmaps.
            if( mg.posx > mg.tx) {
                mg.posx--;
            }
            if( mg.posx < mg.tx) {
                mg.posx++;
            }
            if( mg.posy > mg.ty) {
                mg.posy--;
            }
            if( mg.posy < mg.ty) {
                mg.posy++;
            }

            if( mg.posx == mg.tx && mg.posy == mg.ty ) {
                mg.wander();
            } else {
                mg.dec_interest( 1 );
            }
            // Erase the group at it's old location, add the group with the new location
            tmpzg.insert( std::pair<tripoint, mongroup>( tripoint(mg.posx, mg.posy, mg.posz ), mg ) );
            zg.erase( it++ );
        }
    }
    // and now back into the monster group map.
    zg.insert( tmpzg.begin(), tmpzg.end() );
}

/**
* @param sig_power - power of signal or max distantion for reaction of zombies
*/
void overmap::signal_hordes( const int x, const int y, const int sig_power)
{
    // TODO: Signal adjacent overmaps too. (the 3 nearest ones)
    for( auto &elem : zg ) {
        mongroup &mg = elem.second;
        if( !mg.horde ) {
            continue;
        }
            const int dist = rl_dist( x, y, mg.posx, mg.posy );
            if( sig_power <= dist ) {
                continue;
            }
            // TODO: base this in monster attributes, foremost GOODHEARING.
            const int d_inter = (sig_power - dist) * 5;
            const int roll = rng( 0, mg.interest );
            if( roll < d_inter ) {
                const int targ_dist = rl_dist( x, y, mg.tx, mg.ty );
                // TODO: Base this on targ_dist:dist ratio.
                if (targ_dist < 5) {
                    mg.set_target( (mg.tx + x) / 2, (mg.ty + y) / 2 );
                    mg.inc_interest( d_inter );
                } else {
                    mg.set_target( x, y );
                    mg.set_interest( d_inter );
                }
            }
    }
}

void grow_forest_oter_id(oter_id &oid, bool swampy)
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
        for (auto j = cities.begin(); j != cities.end(); j++) {
            inner_tries = 1000;
            while (trig_dist(forx, fory, j->x, j->y) - fors / 2 < j->s ) {
                // Set forx and fory far enough from cities
                forx = rng(0, OMAPX - 1);
                fory = rng(0, OMAPY - 1);
                // Set fors to determine the size of the forest; usually won't overlap w/ cities
                fors = rng(settings.forest_size_min, settings.forest_size_max);
                j = cities.begin();
                if( 0 == --inner_tries ) {
                    break;
                }
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
                        check_ot_type("river", x + k, y + l, 0)) {
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
                    grow_forest_oter_id( ter(x + mx, y + my, 0),
                                         ( mx == 0 && my == 0 ? false : swampy ) );
                }
            }
            // Random walk our forest
            x += rng(-2, 2);
            if (x < 0    ) {
                x = 0;
            }
            if (x > OMAPX) {
                x = OMAPX;
            }
            y += rng(-2, 2);
            if (y < 0    ) {
                y = 0;
            }
            if (y > OMAPY) {
                y = OMAPY;
            }
        }
    }
}

void overmap::place_river(point pa, point pb)
{
    int x = pa.x, y = pa.y;
    do {
        x += rng(-1, 1);
        y += rng(-1, 1);
        if (x < 0) {
            x = 0;
        }
        if (x > OMAPX - 1) {
            x = OMAPX - 1;
        }
        if (y < 0) {
            y = 0;
        }
        if (y > OMAPY - 1) {
            y = OMAPY - 1;
        }
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (y + i >= 0 && y + i < OMAPY && x + j >= 0 && x + j < OMAPX) {
                    ter(x + j, y + i, 0) = "river_center";
                }
            }
        }
        if (pb.x > x && (rng(0, int(OMAPX * 1.2) - 1) < pb.x - x ||
                         (rng(0, int(OMAPX * .2) - 1) > pb.x - x &&
                          rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y)))) {
            x++;
        }
        if (pb.x < x && (rng(0, int(OMAPX * 1.2) - 1) < x - pb.x ||
                         (rng(0, int(OMAPX * .2) - 1) > x - pb.x &&
                          rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y)))) {
            x--;
        }
        if (pb.y > y && (rng(0, int(OMAPY * 1.2) - 1) < pb.y - y ||
                         (rng(0, int(OMAPY * .2) - 1) > pb.y - y &&
                          rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x)))) {
            y++;
        }
        if (pb.y < y && (rng(0, int(OMAPY * 1.2) - 1) < y - pb.y ||
                         (rng(0, int(OMAPY * .2) - 1) > y - pb.y &&
                          rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x)))) {
            y--;
        }
        x += rng(-1, 1);
        y += rng(-1, 1);
        if (x < 0) {
            x = 0;
        }
        if (x > OMAPX - 1) {
            x = OMAPX - 2;
        }
        if (y < 0) {
            y = 0;
        }
        if (y > OMAPY - 1) {
            y = OMAPY - 1;
        }
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                // We don't want our riverbanks touching the edge of the map for many reasons
                if ((y + i >= 1 && y + i < OMAPY - 1 && x + j >= 1 && x + j < OMAPX - 1) ||
                    // UNLESS, of course, that's where the river is headed!
                    (abs(pb.y - (y + i)) < 4 && abs(pb.x - (x + j)) < 4)) {
                    ter(x + j, y + i, 0) = "river_center";
                }
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

    while (cities.size() < size_t(NUM_CITIES)) {
        int cx = rng(12, OMAPX - 12);
        int cy = rng(12, OMAPY - 12);
        int size = dice(town_min, town_max);
        if (one_in(6)) {
            size = city_size;
        } else if (one_in(3)) {
            size = village_size;
        }
        if (ter(cx, cy, 0) == settings.default_oter ) {
            ter(cx, cy, 0) = "road_nesw";
            city tmp;
            tmp.x = cx;
            tmp.y = cy;
            tmp.s = size;
            cities.push_back(tmp);
            start_dir = rng(0, 3);
            for (int j = 0; j < 4; j++) {
                make_road(cx, cy, size, (start_dir + j) % 4, tmp);
            }
        }
    }
}

void overmap::put_buildings(int x, int y, int dir, city town)
{
    int ychange = dir % 2, xchange = (dir + 1) % 2;
    for (int i = -1; i <= 1; i += 2) {
        if ((ter(x + i * xchange, y + i * ychange, 0) == settings.default_oter ) &&
            !one_in(STREETCHANCE)) {
            if (rng(0, 99) > 80 * trig_dist(x, y, town.x, town.y) / town.s) {
                ter(x + i * xchange, y + i * ychange, 0) =
                    shop( ((dir % 2) - i) % 4, settings.city_spec.shops );
            } else {
                if (rng(0, 99) > 130 * trig_dist(x, y, town.x, town.y) / town.s) {
                    ter(x + i * xchange, y + i * ychange, 0) =
                        shop( ((dir % 2) - i) % 4, settings.city_spec.parks );
                } else {
                    ter(x + i * xchange, y + i * ychange, 0) =
                        house( ((dir % 2) - i) % 4, settings.house_basement_chance );
                }
            }
        }
    }
}

void overmap::make_road(int x, int y, int cs, int dir, city town)
{
    int c = cs;
    int croad = cs;
    int dirx = 0;
    int diry = 0;
    std::string road;
    std::string crossroad;
    switch( dir ) {
    case 0:
        dirx = 0;
        diry = -1;
        road = "road_ns";
        crossroad = "road_ew";
        break;
    case 1:
        dirx = 1;
        diry = 0;
        road = "road_ew";
        crossroad = "road_ns";
        break;
    case 2:
        dirx = 0;
        diry = 1;
        road = "road_ns";
        crossroad = "road_ew";
        break;
    case 3:
        dirx = -1;
        diry = 0;
        road = "road_ew";
        crossroad = "road_ns";
        break;
    default:
        // Out-of-range dir value, bail out.
        return;
    }

    // Grow in the stated direction, sprouting off sub-roads and placing buildings as we go.
    while( c > 0 && y > 0 && x > 0 && y < OMAPY - 1 && x < OMAPX - 1 &&
           (ter(x + dirx, y + diry, 0) == settings.default_oter || c == cs) ) {
        x += dirx;
        y += diry;
        c--;
        ter( x, y, 0 ) = road.c_str();
        // Look for a crossroad or a road ahead, if we find one,
        // set current tile to be road_null and c to -1 to prevent further branching.
        if( ter( x + dirx, y + diry, 0 ) == road.c_str() ||
            ter( x + dirx, y + diry, 0 ) == crossroad.c_str() ||
            // This looks left and right of the current motion of travel.
            ter( x + diry, y + dirx, 0 ) == road.c_str() ||
            ter( x + diry, y + dirx, 0 ) == crossroad.c_str() ||
            ter( x - diry, y - dirx, 0 ) == road.c_str() ||
            ter( x - diry, y - dirx, 0 ) == crossroad.c_str()) {
            ter(x, y, 0) = "road_null";
            c = -1;

        }
        put_buildings(x, y, dir, town);
        // Look to each side, and branch if the way is clear.
        if (c < croad - 1 && c >= 2 && ( ter(x + diry, y + dirx, 0) == settings.default_oter &&
                                         ter(x - diry, y - dirx, 0) == settings.default_oter ) ) {
            croad = c;
            make_road(x, y, cs - rng(1, 3), (dir + 1) % 4, town);
            make_road(x, y, cs - rng(1, 3), (dir + 3) % 4, town);
        }
    }
    // Now we're done growing, if there's a road ahead, add one more road segment to meet it.
    if (is_road(x + (2 * dirx) , y + (2 * diry), 0)) {
        ter(x + dirx, y + diry, 0) = "road_ns";
    }

    // If we're big, make a right turn at the edge of town.
    // Seems to make little neighborhoods.
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
                        generated_lab.push_back(point(lx, ly));
                    }
                }
            }
        }
    }

    bool generate_stairs = true;
    for( auto &elem : generated_lab ) {
        if( ter( elem.x, elem.y, z + 1 ) == "lab_stairs" ) {
            generate_stairs = false;
        }
    }
    if (generate_stairs && !generated_lab.empty()) {
        int v = rng(0, generated_lab.size() - 1);
        point p = generated_lab[v];
        ter(p.x, p.y, z + 1) = "lab_stairs";
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
                        generated_ice_lab.push_back(point(lx, ly));
                    }
                }
            }
        }
    }

    bool generate_stairs = true;
    for( auto &elem : generated_ice_lab ) {
        if( ter( elem.x, elem.y, z + 1 ) == "ice_lab_stairs" ) {
            generate_stairs = false;
        }
    }
    if (generate_stairs && !generated_ice_lab.empty()) {
        int v = rng(0, generated_ice_lab.size() - 1);
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
    case 0:
        next = point(x    , y - 1);
    case 1:
        next = point(x + 1, y    );
    case 2:
        next = point(x    , y + 1);
    case 3:
        next = point(x - 1, y    );
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
    for (auto &i : valid) {
        if (i.x != next.x || i.y != next.y) {
            if (one_in(s * 2)) {
                if (one_in(2)) {
                    ter(i.x, i.y, z) = "ants_food";
                } else {
                    ter(i.x, i.y, z) = "ants_larvae";
                }
            } else if (one_in(5)) {
                int dir2 = 0;
                if (i.y == y - 1) {
                    dir2 = 0;
                }
                if (i.x == x + 1) {
                    dir2 = 1;
                }
                if (i.y == y + 1) {
                    dir2 = 2;
                }
                if (i.x == x - 1) {
                    dir2 = 3;
                }
                build_tunnel(i.x, i.y, z, s - rng(0, 3), dir2);
            }
        }
    }
    build_tunnel(next.x, next.y, z, s - 1, dir);
}

bool overmap::build_slimepit(int x, int y, int z, int s)
{
    bool requires_sub = false;
    for (int n = 1; n <= s; n++) {
        for (int i = x - n; i <= x + n; i++) {
            for (int j = y - n; j <= y + n; j++) {
                if (rng(1, s * 2) >= n) {
                    if (one_in(8) && z > -OVERMAP_DEPTH) {
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
    if (!one_in(4)) {
        num_rifts++;
    }
    for (int n = 0; n < num_rifts; n++) {
        int x = rng(MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE);
        int y = rng(MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE);
        int xdist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE),
            ydist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE);
        // We use rng(0, 10) as the t-value for this Bresenham Line, because by
        // repeating this twice, we can get a thick line, and a more interesting rift.
        for (int o = 0; o < 3; o++) {
            if (xdist > ydist) {
                riftline = line_to(x - xdist, y - ydist + o, x + xdist, y + ydist, rng(0, 10));
            } else {
                riftline = line_to(x - xdist + o, y - ydist, x + xdist, y + ydist, rng(0, 10));
            }
            for (size_t i = 0; i < riftline.size(); i++) {
                if (i == riftline.size() / 2 && !one_in(3)) {
                    ter(riftline[i].x, riftline[i].y, z) = "hellmouth";
                } else {
                    ter(riftline[i].x, riftline[i].y, z) = "rift";
                }
            }
        }
    }
}

void overmap::make_hiway(int x1, int y1, int x2, int y2, int z, const std::string &base)
{
    if (x1 == x2 && y1 == y2) {
        return;
    }

    std::priority_queue<node, std::deque<node> > nodes[2];
    bool closed[OMAPX][OMAPY] = {{false}};
    int open[OMAPX][OMAPY] = {{0}};
    int dirs[OMAPX][OMAPY] = {{0}};
    int dx[4] = {1, 0, -1, 0};
    int dy[4] = {0, 1, 0, -1};
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

                if (nodes[i].size() > nodes[1 - i].size()) {
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
    int xdif = dir * (1 - 2 * rng(0, 1));
    int ydif = (1 - dir) * (1 - 2 * rng(0, 1));
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
    for (size_t i = 0; i < cities.size(); i++) {
        int closest = -1;
        for (size_t j = i + 1; j < cities.size(); j++) {
            int distance = trig_dist(cities[i].x, cities[i].y, cities[j].x, cities[j].y);
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
                if (check_ot_type("bridge", x, y, z) &&
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
                    // also taking other road pieces that may be next
                    // to it into account. A bit of a kludge but it works.
                } else if (ter(x, y, z) == "bridge_ns" &&
                           (!is_river(ter(x - 1, y, z)) ||
                            !is_river(ter(x + 1, y, z)))) {
                    good_road("road", x, y, z);
                } else if (ter(x, y, z) == "bridge_ew" &&
                           (!is_river(ter(x, y - 1, z)) ||
                            !is_river(ter(x, y + 1, z)))) {
                    good_road("road", x, y, z);
                } else if (check_ot_type("road", x, y, z)) {
                    good_road("road", x, y, z);
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
                    && ter(x + 1, y, z) == "road_nsw"
                    && ter(x, y + 1, z) == "road_nes"
                    && ter(x + 1, y + 1, z) == "road_nsw") {
                    ter(x, y, z) = "hiway_ns";
                    ter(x + 1, y, z) = "hiway_ns";
                    ter(x, y + 1, z) = "hiway_ns";
                    ter(x + 1, y + 1, z) = "hiway_ns";
                } else if (ter(x, y, z) == "road_esw"
                           && ter(x + 1, y, z) == "road_esw"
                           && ter(x, y + 1, z) == "road_new"
                           && ter(x + 1, y + 1, z) == "road_new") {
                    ter(x, y, z) = "hiway_ew";
                    ter(x + 1, y, z) = "hiway_ew";
                    ter(x, y + 1, z) = "hiway_ew";
                    ter(x + 1, y + 1, z) = "hiway_ew";
                }
            }
        }
    }
}

bool overmap::check_ot_type(const std::string &otype, int x, int y, int z) const
{
    const oter_id oter = get_ter(x, y, z);
    return is_ot_type(otype, oter);
}

bool overmap::check_ot_type_road(const std::string &otype, int x, int y, int z)
{
    const oter_id oter = ter(x, y, z);
    if(otype == "road" || otype == "bridge" || otype == "hiway") {
        if(is_ot_type("road", oter) || is_ot_type ("bridge", oter) || is_ot_type("hiway", oter)) {
            return true;
        } else {
            return false;
        }
    }
    return is_ot_type(otype, oter);
}



bool overmap::is_road(int x, int y, int z)
{
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
        for (auto &it : roads_out) {
            if (abs(it.x - x) + abs(it.y - y) <= 1) {
                return true;
            }
        }
    }
    return ter(x, y, z).t().is_road;
    //oter_t(ter(x, y, z)).is_road;
}

bool overmap::is_road_or_highway(int x, int y, int z)
{
    if (is_road(x, y, z) || check_ot_type("hiway", x, y, z)) {
        return true;
    }
    return false;
}

void overmap::good_road(const std::string &base, int x, int y, int z)
{
    if (check_ot_type_road(base, x, y - 1, z)) {
        if (check_ot_type_road(base, x + 1, y, z)) {
            if (check_ot_type_road(base, x, y + 1, z)) {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_nesw";
                } else {
                    ter(x, y, z) = base + "_nes";
                }
            } else {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_new";
                } else {
                    ter(x, y, z) = base + "_ne";
                }
            }
        } else {
            if (check_ot_type_road(base, x, y + 1, z)) {
                if (check_ot_type(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_nsw";
                } else {
                    ter(x, y, z) = base + "_ns";
                }
            } else {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_wn";
                } else {
                    if(base == "road" && (y != OMAPY - 1)) {
                        ter(x, y, z) = base + "_end_south";
                    } else {
                        ter(x, y, z) = base + "_ns";
                    }
                }
            }
        }
    } else {
        if (check_ot_type_road(base, x + 1, y, z)) {
            if (check_ot_type_road(base, x, y + 1, z)) {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_esw";
                } else {
                    ter(x, y, z) = base + "_es";
                }
            } else {
                if( check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_ew";
                } else {
                    if(base == "road" && (x != 0)) {
                        ter(x, y, z) = base + "_end_west";
                    } else {
                        ter(x, y, z) = base + "_ew";
                    }
                }
            }
        } else {
            if (check_ot_type_road(base, x, y + 1, z)) {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    ter(x, y, z) = base + "_sw";
                } else {
                    if(base == "road" && (y != 0)) {
                        ter(x, y, z) = base + "_end_north";
                    } else {
                        ter(x, y, z) = base + "_ns";
                    }
                }
            } else {
                if (check_ot_type_road(base, x - 1, y, z)) {
                    if(base == "road" && (x != OMAPX-1)) {
                        ter(x, y, z) = base + "_end_east";
                    } else {
                        ter(x, y, z) = base + "_ew";
                    }
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
    if((x == 0) || (x == OMAPX-1)) {
        if(!is_river(ter(x, y - 1, z))) {
            ter(x, y, z) = "river_north";
        } else if(!is_river(ter(x, y + 1, z))) {
            ter(x, y, z) = "river_south";
        } else {
            ter(x, y, z) = "river_center";
        }
        return;
    }
    if((y == 0) || (y == OMAPY-1)) {
        if(!is_river(ter(x - 1, y, z))) {
            ter(x, y, z) = "river_west";
        } else if(!is_river(ter(x + 1, y, z))) {
            ter(x, y, z) = "river_east";
        } else {
            ter(x, y, z) = "river_center";
        }
        return;
    }
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

bool overmap::allowed_terrain(tripoint p, int width, int height, std::list<std::string> allowed)
{
    for(int h = 0; h < height; ++h) {
        for(int w = 0; w < width; ++w) {
            for( auto &elem : allowed ) {
                oter_id oter = this->ter(p.x + w, p.y + h, p.z);
                if( !is_ot_type( elem, oter ) ) {
                    return false;
                }
            }
        }
    }
    return true;
}

// checks the area around the selected point to ensure terrain is valid for special
bool overmap::allowed_terrain(tripoint p, std::list<tripoint> tocheck,
                              std::list<std::string> allowed, std::list<std::string> disallowed)
{
    for( auto t : tocheck ) {

        bool passed = false;
        for( auto &elem : allowed ) {
            oter_id oter = this->ter(p.x + t.x, p.y + t.y, p.z);
            if( is_ot_type( elem, oter ) ) {
                passed = true;
            }
        }
        // if we are only checking against disallowed types, we don't want this to fail us
        if(!passed && allowed.size() > 0) {
            return false;
        }

        for( auto &elem : disallowed ) {
            oter_id oter = this->ter(p.x + t.x, p.y + t.y, p.z);
            if( is_ot_type( elem, oter ) ) {
                return false;
            }
        }
    }

    return true;
}

// new x = (x-c.x)*cos() - (y-c.y)*sin() + c.x
// new y = (x-c.x)*sin() + (y-c.y)*cos() + c.y
// r1x = 0*x - 1*y = -1*y, r1y = 1*x + y*0 = x
// r2x = -1*x - 0*y = -1*x , r2y = x*0 + y*-1 = -1*y
// r3x = x*0 - (-1*y) = y, r3y = x*-1 + y*0 = -1*x
// c=0,0, rot90 = (-y, x); rot180 = (-x, y); rot270 = (y, -x)
/*
    (0,0)(1,0)(2,0) 90 (0,0)(0,1)(0,2)       (-2,0)(-1,0)(0,0)
    (0,1)(1,1)(2,1) -> (-1,0)(-1,1)(-1,2) -> (-2,1)(-1,1)(0,1)
    (0,2)(1,2)(2,2)    (-2,0)(-2,1)(-2,2)    (-2,2)(-1,2)(0,2)
*/

inline tripoint rotate_tripoint(tripoint p, int rotations)
{
    if(rotations == 1) {
        return tripoint(-1 * p.y, p.x, p.z);
    } else if(rotations == 2) {
        return tripoint(-1 * p.x, -1 * p.y, p.z);
    } else if(rotations == 3) {
        return tripoint(p.y, -1 * p.x, p.z);
    }
    return p;
}

// checks around the selected point to see if the special can be placed there
bool overmap::allow_special(tripoint p, overmap_special special, int &rotate)
{
    // check if rotation is allowed, and if necessary
    rotate = 0;
    // check to see if road is nearby, if so, rotate to face road
    // if no road && special requires road, return false
    // if no road && special does not require it, pick a random rotation
    if(special.rotatable) {
        // if necessary:
        if(check_ot_type("road", p.x + 1, p.y, p.z)) {
            // road to right
            rotate = 1;
        } else if(check_ot_type("road", p.x - 1, p.y, p.z)) {
            // road to left
            rotate = 3;
        } else if(check_ot_type("road", p.x, p.y + 1, p.z)) {
            // road to south
            rotate = 2;
        } else if(check_ot_type("road", p.x, p.y - 1, p.z)) {
            // road to north
        } else {
            if(std::find(special.locations.begin(), special.locations.end(),
                 "by_hiway") != special.locations.end()) {
                return false;
            } else {
                rotate = rng(0, 3);
            }
        }
    }

    // do bounds & connection checking
    std::list<tripoint> rotated_points;
    for( auto t : special.terrains ) {

        tripoint rotated_point = rotate_tripoint(t.p, rotate);
        rotated_points.push_back(rotated_point);

        tripoint testpoint = tripoint(rotated_point.x + p.x, rotated_point.y + p.y, p.z);
        if((testpoint.x >= OMAPX - 1) ||
           (testpoint.x < 0) || (testpoint.y < 0) ||
           (testpoint.y >= OMAPY - 1)) {
            return false;
        }
        if(t.connect == "road")
        {
            switch(rotate){
            case 0:
                testpoint = tripoint(testpoint.x, testpoint.y - 1, testpoint.z);
                break;
            case 1:
                testpoint = tripoint(testpoint.x + 1, testpoint.y, testpoint.z);
                break;
            case 2:
                testpoint = tripoint(testpoint.x, testpoint.y + 1, testpoint.z);
                break;
            case 3:
                testpoint = tripoint(testpoint.x - 1, testpoint.y, testpoint.z);
                break;
            default:
                break;
            }
            if(!road_allowed(get_ter(testpoint.x, testpoint.y, testpoint.z)))
            {
                return false;
            }
        }
    }

    // then do city range checking
    point citypt = point(p.x, p.y);
    if(!(special.min_city_distance == -1 || dist_from_city(citypt) >= special.min_city_distance) ||
       !(special.max_city_distance == -1 || dist_from_city(citypt) <= special.max_city_distance)) {
        return false;
    }
    // then check location flags
    bool passed = false;
    for( auto location : special.locations ) {
        // check each location, if one returns true, then return true, else return false
        // never, always, water, land, forest, wilderness, by_hiway
        // false, true,   river, !river, forest, forest/field, special
        std::list<std::string> allowed_terrains;
        std::list<std::string> disallowed_terrains;

        if(location == "never") {
            return false;
        } else if(location == "always") {
            return true;
        } else if(location == "water") {
            allowed_terrains.push_back("river");
        } else if(location == "land") {
            disallowed_terrains.push_back("river");
            disallowed_terrains.push_back("road");
        } else if(location == "forest") {
            allowed_terrains.push_back("forest");
        } else if(location == "wilderness") {
            allowed_terrains.push_back("forest");
            allowed_terrains.push_back("field");
        } else if(location == "by_hiway") {
            disallowed_terrains.push_back("road");
        }

        passed = allowed_terrain(p, rotated_points, allowed_terrains, disallowed_terrains);
        if(passed) {
            return true;
        }
    }
    return false;
}

// should work essentially the same as previously
// split map into sections, iterate through sections
// iterate through specials, check if special is valid
// pick & place special

void overmap::place_specials()
{
    std::map<overmap_special, int> num_placed;

    for( auto &overmap_specials_it : overmap_specials ) {
        overmap_special special = overmap_specials_it;
            if (special.max_occurrences != 100){
                num_placed.insert( std::pair<overmap_special, int>( overmap_specials_it,
                                                                    0 ) ); // normal circumstances
            }
            else{
                if (rand() % 100 <= special.min_occurrences){ //occurance is actually a % chance, so less than 1
                    num_placed.insert( std::pair<overmap_special, int>(
                        overmap_specials_it, -1 ) ); // Priority add one in this map
                }
                else
                    num_placed.insert( std::pair<overmap_special, int>(
                        overmap_specials_it, 999 ) ); // Don't add one in this map
            }
    }

    std::vector<point> sectors;
    for (int x = 0; x < OMAPX; x += OMSPEC_FREQ) {
        for (int y = 0; y < OMAPY; y += OMSPEC_FREQ) {
            sectors.push_back(point(x, y));
        }
    }

    while(!sectors.empty()) {
        int pick = rng(0, sectors.size() - 1);
        int x = sectors.at(pick).x;
        int y = sectors.at(pick).y;

        sectors.erase(sectors.begin() + pick);

        //std::vector<overmap_special> valid_specials;
        // second parameter is rotation
        std::map<overmap_special, int> valid_specials;
        int tries = 0;
        tripoint p;
        int rotation = 0;

        do {
            p = tripoint(rng(x, x + OMSPEC_FREQ - 1), rng(y, y + OMSPEC_FREQ - 1), 0);
            // dont need to check for edges yet
            for( auto special : overmap_specials ) {
                std::list<std::string> allowed_terrains;
                allowed_terrains.push_back("forest");

                if (ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] && (special.flags.count("CLASSIC") < 1)) {
                    continue;
                }
                if ((num_placed[special] < special.max_occurrences || special.max_occurrences <= 0) &&
                    allow_special(p, special, rotation)) {
                    valid_specials[special] = rotation;
                }
            }
            ++tries;
        } while(valid_specials.empty() && tries < 20);

        // selection & placement happens here
        std::pair<overmap_special, int> place;
        if(!valid_specials.empty()) {
            // Place the MUST HAVE ones first, to try and guarantee that they appear
            //std::vector<overmap_special> must_place;
            std::map<overmap_special, int> must_place;
            for( auto &valid_special : valid_specials ) {
                place = valid_special;
                if(num_placed[place.first] < place.first.min_occurrences) {
                    must_place.insert(place);
                }
            }
            if (must_place.empty()) {
                int selection = rng(0, valid_specials.size() - 1);
                //overmap_special special = valid_specials.at(valid_specials.begin() + selection).first;
                std::map<overmap_special, int>::iterator it = valid_specials.begin();
                std::advance(it, selection);
                place = *it;
                overmap_special special = place.first;
                if (num_placed[special] == -1)
                    num_placed[special] = 999;//if you build one, never build another.  For [x:100] spawn % chance
                num_placed[special]++;
                place_special(special, p, place.second);
            } else {
                int selection = rng(0, must_place.size() - 1);
                //overmap_special special = must_place.at(must_place.begin() + selection).first;
                std::map<overmap_special, int>::iterator it = must_place.begin();
                std::advance(it, selection);
                place = *it;
                overmap_special special = place.first;
                if (num_placed[special] == -1)
                    num_placed[special] = 999;//if you build one, never build another.  For [x:100] spawn % chance
                num_placed[special]++;
                place_special(special, p, place.second);
            }
        }
    }
}

// does the actual placement.  should do rotation, but rotation validity should be checked before
// c = center point about rotation
// new x = (x-c.x)*cos() - (y-c.y)*sin() + c.x
// new y = (x-c.x)*sin() + (y-c.y)*cos() + c.y
// c=0,0, rot90 = (-y, x); rot180 = (-x, y); rot270 = (y, -x)
/*
    (0,0)(1,0)(2,0) 90 (0,0)(0,1)(0,2)       (-2,0)(-1,0)(0,0)
    (0,1)(1,1)(2,1) -> (-1,0)(-1,1)(-1,2) -> (-2,1)(-1,1)(0,1)
    (0,2)(1,2)(2,2)    (-2,0)(-2,1)(-2,2)    (-2,2)(-1,2)(0,2)
*/
void overmap::place_special(overmap_special special, tripoint p, int rotation)
{
    //std::map<std::string, tripoint> connections;
    std::vector<std::pair<std::string, tripoint> > connections;

    for(std::list<overmap_special_terrain>::iterator it = special.terrains.begin();
        it != special.terrains.end(); ++it) {
        overmap_special_terrain terrain = *it;

        oter_id id = (oter_id) terrain.terrain;
        oter_t t = (oter_t) id;

        tripoint rp = rotate_tripoint(terrain.p, rotation);
        tripoint location = tripoint(p.x + rp.x, p.y + rp.y, p.z + rp.z);

        if(!t.rotates) {
            this->ter(location.x, location.y, location.z) = terrain.terrain;
        } else {
            this->ter(location.x, location.y, location.z) = rotate(terrain.terrain, rotation);
        }

        if(terrain.connect.size() > 0) {
            //connections[terrain.connect] = location;
            std::pair<std::string, tripoint> connection;
            connection.first = terrain.connect;
            connection.second = location;
            connections.push_back(connection);
        }

        if(special.flags.count("BLOB") > 0) {
            for (int x = -2; x <= 2; x++) {
                for (int y = -2; y <= 2; y++) {
                    if (one_in(1 + abs(x) + abs(y))) {
                        ter(location.x + x, location.y + y, location.z) = terrain.terrain;
                    }
                }
            }
        }
    }

    for( auto connection : connections ) {

        if(connection.first == "road") {
            city closest;
            int distance = 999;
            for( auto c : cities ) {

                int dist = rl_dist(connection.second.x, connection.second.y, c.x, c.y);
                if (dist < distance) {
                    closest = c;
                    distance = dist;
                }
            }
            // generally entrances come out of the top,
            // so we want to rotate the road connection with the point.
            tripoint conn = connection.second;

            switch(rotation)
            {
            case 0:
                conn.y = conn.y - 1;
                break;
            case 1:
                conn.x = conn.x + 1;
                break;
            case 2:
                conn.y = conn.y + 1;
                break;
            case 3:
                conn.x = conn.x - 1;
                break;
            default:
                break;
            }
            if(ter(conn.x, conn.y, p.z).t().allow_road) {
                make_hiway(conn.x, conn.y, closest.x, closest.y, p.z, "road");
            } else { // in case the entrance does not come out the top, try wherever possible...
                conn = connection.second;
                make_hiway(conn.x, conn.y, closest.x, closest.y, p.z, "road");
            }
        }
    }

    // place spawns
    if(special.spawns.group != "GROUP_NULL") {
        overmap_special_spawns spawns = special.spawns;
        int pop = rng(spawns.min_population, spawns.max_population);
        int rad = rng(spawns.min_radius, spawns.max_radius);
        add_mon_group(mongroup(spawns.group, p.x * 2, p.y * 2, p.z, rad, pop));
    }
}

oter_id overmap::rotate(const oter_id &oter, int dir)
{
    const oter_t &otert = oter;
    if (! otert.rotates  && dir != 0) {
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

void overmap::place_mongroups()
{
    // Cities are full of zombies
    for( auto &elem : cities ) {
        if( ACTIVE_WORLD_OPTIONS["WANDER_SPAWNS"] ) {
            if( !one_in( 16 ) || elem.s > 5 ) {
                mongroup m( "GROUP_ZOMBIE", ( elem.x * 2 ), ( elem.y * 2 ), 0, int( elem.s * 2.5 ),
                            elem.s * 80 );
//                m.set_target( zg.back().posx, zg.back().posy );
                m.horde = true;
                m.wander();
                add_mon_group( m );
            }
        }
        if( !ACTIVE_WORLD_OPTIONS["STATIC_SPAWN"] ) {
            add_mon_group( mongroup( "GROUP_ZOMBIE", ( elem.x * 2 ), ( elem.y * 2 ), 0,
                                     int( elem.s * 2.5 ), elem.s * 80 ) );
        }
    }

    if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
        // Figure out where swamps are, and place swamp monsters
        for (int x = 3; x < OMAPX - 3; x += 7) {
            for (int y = 3; y < OMAPY - 3; y += 7) {
                int swamp_count = 0;
                for (int sx = x - 3; sx <= x + 3; sx++) {
                    for (int sy = y - 3; sy <= y + 3; sy++) {
                        if (ter(sx, sy, 0) == "forest_water") {
                            swamp_count += 2;
                        }
                    }
                }
                if (swamp_count >= 25)
                    add_mon_group(mongroup("GROUP_SWAMP", x * 2, y * 2, 0, 3,
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
                        if (is_river(ter(sx, sy, 0))) {
                            river_count++;
                        }
                    }
                }
                if (river_count >= 25)
                    add_mon_group(mongroup("GROUP_RIVER", x * 2, y * 2, 0, 3,
                                          rng(river_count * 8, river_count * 25)));
            }
        }
    }

    if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
        // Place the "put me anywhere" groups
        int numgroups = rng(0, 3);
        for (int i = 0; i < numgroups; i++) {
            add_mon_group(mongroup("GROUP_WORM", rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1), 0,
                         rng(20, 40), rng(30, 50)));
        }
    }
}

int overmap::get_top_border()
{
    return loc.y * OMAPY;
}

int overmap::get_left_border()
{
    return loc.x * OMAPX;
}

int overmap::get_bottom_border()
{
    return get_top_border() + OMAPY;
}

int overmap::get_right_border()
{
    return get_left_border() + OMAPX;
}

void overmap::place_radios()
{
    std::string message;
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            if (ter(i, j, 0) == "radio_tower") {
                int choice = rng(0, 2);
                switch(choice) {
                case 0:
                    message = string_format(_("This is emergency broadcast station %d%d.\
  Please proceed quickly and calmly to your designated evacuation point."), i, j);
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
                    break;
                case 1:
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH),
                                                 _("Head West.  All survivors, head West.  Help is waiting.")));
                    break;
                case 2:
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), "",
                                                 WEATHER_RADIO));
                    break;
                }
            } else if (ter(i, j, 0) == "lmoe") {
                message = string_format(_("This is automated emergency shelter beacon %d%d.\
  Supplies, amenities and shelter are stocked."), i, j);
                radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH) / 2,
                                             message));
            } else if (ter(i, j, 0) == "fema_entrance") {
                message = string_format(_("This is FEMA camp %d%d.\
  Supplies are limited, please bring supplemental food, water, and bedding.\
  This is FEMA camp %d%d.  A designated long-term emergency shelter."), i, j, i, j);
                radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
            }
        }
    }
}


void overmap::open()
{
    std::string const plrfilename = overmapbuffer::player_filename(loc.x, loc.y);
    std::string const terfilename = overmapbuffer::terrain_filename(loc.x, loc.y);
    std::ifstream fin;

    fin.open(terfilename.c_str());
    if (fin.is_open()) {
        unserialize(fin, plrfilename, terfilename);
        fin.close();
    } else { // No map exists!  Prepare neighbors, and generate one.
        std::vector<const overmap*> pointers;
        // Fetch south and north
        for (int i = -1; i <= 1; i += 2) {
            pointers.push_back(overmap_buffer.get_existing(loc.x, loc.y+i));
        }
        // Fetch east and west
        for (int i = -1; i <= 1; i += 2) {
            pointers.push_back(overmap_buffer.get_existing(loc.x+i, loc.y));
        }
        // pointers looks like (north, south, west, east)
        generate(pointers[0], pointers[3], pointers[1], pointers[2]);
    }
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


oter_iid oterfind(const std::string id)
{
    if( otermap.find(id) == otermap.end() ) {
        debugmsg("Can't find %s", id.c_str());
        return 0;
    }
    return otermap[id].loadid;
};

void set_oter_ids()   // fixme constify
{
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
const unsigned &oter_id::operator=(const int &i)
{
    _val = i;
    return _val;
}
// ter(...) = "rock"
oter_id::operator std::string() const
{
    if ( _val > oterlist.size() ) {
        debugmsg("oterlist[%d] > %d", _val, oterlist.size()); // remove me after testing (?)
        return 0;
    }
    return std::string(oterlist[_val].id);
}

// int index = ter(...);
oter_id::operator int() const
{
    return _val;
}

// ter(...) != "foobar"
bool oter_id::operator!=(const char *v) const
{
    return oterlist[_val].id.compare(v) != 0;
    /*    hellaciously slow string allocation frenzy -v
          std::map<std::string, oter_t>::const_iterator it=otermap.find(v);
          return ( it == otermap.end() || it->second.loadid != _val);
    */
}

// ter(...) == "foobar"
bool oter_id::operator==(const char *v) const
{
    return oterlist[_val].id.compare(v) == 0;
}
bool oter_id::operator<=(const char *v) const
{
    std::unordered_map<std::string, oter_t>::const_iterator it = otermap.find(v);
    return ( it == otermap.end() || it->second.loadid <= _val);
}
bool oter_id::operator>=(const char *v) const
{
    std::unordered_map<std::string, oter_t>::const_iterator it = otermap.find(v);
    return ( it != otermap.end() && it->second.loadid >= _val);
}

// o_id1 != o_id2
bool oter_id::operator!=(const oter_id &v) const
{
    return ( _val != v._val );
}
bool oter_id::operator==(const oter_id &v) const
{
    return ( _val == v._val );
}

// oter_t( ter(...) ).name // WARNING
oter_id::operator oter_t() const
{
    return oterlist[_val];
}

const oter_t &oter_id::t() const
{
    return oterlist[_val];
}
// ter(...).size()
size_t oter_id::size() const
{
    return oterlist[_val].id.size();
}

// ter(...).find("foo");
int oter_id::find(const std::string &v, const int start, const int end) const
{
    (void)start;
    (void)end; // TODO?
    return oterlist[_val].id.find(v);//, start, end);
}
// ter(...).compare(0, 3, "foo");
int oter_id::compare(size_t pos, size_t len, const char *s, size_t n) const
{
    if ( n != 0 ) {
        return oterlist[_val].id.compare(pos, len, s, n);
    } else {
        return oterlist[_val].id.compare(pos, len, s);
    }
}

// std::string("river_ne");  oter_id van_location(down_by);
oter_id::oter_id(const std::string &v)
{
    std::unordered_map<std::string, oter_t>::const_iterator it = otermap.find(v);
    if ( it == otermap.end() ) {
        debugmsg("not found: %s", v.c_str());
    } else {
        _val = it->second.loadid;
    }
}

// oter_id b("house_north");
oter_id::oter_id(const char *v)
{
    std::unordered_map<std::string, oter_t>::const_iterator it = otermap.find(v);
    if ( it == otermap.end() ) {
        debugmsg("not found: %s", v);
    } else {
        _val = it->second.loadid;
    }
}

// wprint("%s",ter(...).c_str() );
const char *oter_id::c_str() const
{
    return std::string(oterlist[_val].id).c_str();
}


void groundcover_extra::setup()   // fixme return bool for failure
{
    default_ter = terfind( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for ( std::map<std::string, double>::const_iterator it = percent_str.begin();
          it != percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) {
            continue;
        }
        if ( termap.find( it->first ) != termap.end() ) {
            tf_id.ter = termap[ it->first ].loadid;
        } else if ( furnmap.find( it->first ) != furnmap.end() ) {
            tf_id.furn = furnmap[ it->first ].loadid;
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
            continue;
        }
        wtotal += (int)(it->second * 10000.0);
        weightlist[ wtotal ] = tf_id;
    }

    for ( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin();
          it != boosted_percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) {
            continue;
        }
        if ( termap.find( it->first ) != termap.end() ) {
            tf_id.ter = termap[ it->first ].loadid;
        } else if ( furnmap.find( it->first ) != furnmap.end() ) {
            tf_id.furn = furnmap[ it->first ].loadid;
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
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

ter_furn_id groundcover_extra::pick( bool boosted ) const
{
    if ( boosted ) {
        return boosted_weightlist.lower_bound( rng( 0, 1000000 ) )->second;
    }
    return weightlist.lower_bound( rng( 0, 1000000 ) )->second;
}

void regional_settings::setup()
{
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

void overmap::add_mon_group(const mongroup &group)
{
    // Monster groups: the old system had large groups (radius > 1),
    // the new system transforms them into groups of radius 1, this also
    // makes the diffuse setting obsolete (as it only controls how the radius
    // is interpreted) - it's only used when adding monster groups with function.
    if( group.radius == 1 ) {
        zg.insert(std::pair<tripoint, mongroup>( tripoint( group.posx, group.posy, group.posz ), group ) );
        return;
    }
    // diffuse groups use a circular area, non-diffuse groups use a rectangular area
    const int rad = std::max<int>( 0, group.radius );
    const double total_area = group.diffuse ? std::pow( rad + 1, 2 ) : ( rad * rad * M_PI + 1 );
    const double pop = std::max<int>( 1, group.population );
    int xpop = 0;
    for( int x = -rad; x <= rad; x++ ) {
        for( int y = -rad; y <= rad; y++ ) {
            const int dist = group.diffuse ? square_dist( x, y, 0, 0 ) : trig_dist( x, y, 0, 0 );
            if( dist > rad ) {
                continue;
            }
            // Population on a single submap, *not* a integer
            double pop_here;
            if( rad == 0 ) {
                pop_here = pop;
            } else if( group.diffuse ) {
                pop_here = pop / total_area;
            } else {
                // non-diffuse groups are more dense towards the center.
                pop_here = ( 1.0 - static_cast<double>( dist ) / rad ) * pop / total_area;
            }
            if( pop_here > pop || pop_here < 0 ) {
                DebugLog( D_ERROR, D_GAME ) << group.type << ": invalid population here: " << pop_here;
            }
            int p = std::max<int>( 0, std::floor( pop_here ) );
            if( pop_here - p != 0 ) {
                // in case the population is something like 0.2, randomly add a
                // single population unit, this *should* on average give the correct
                // total population.
                const int mod = 10000 * ( pop_here - p );
                if( x_in_y( mod, 10000 ) ) {
                    p++;
                }
            }
            if( p == 0 ) {
                continue;
            }
            // Exact copy to keep all important values, only change what's needed
            // for a single-submap group.
            mongroup tmp( group );
            tmp.radius = 1;
            tmp.posx += x;
            tmp.posy += y;
            tmp.population = p;
            // This *can* create groups outside of the area of this overmap.
            // As this function is called during generating the overmap, the
            // neighboring overmaps might not have been generated and one can't access
            // them through the overmapbuffer as this would trigger generating them.
            // This would in turn to lead to a call to this function again.
            // To avoid this, the overmapbufer checks the monster groups when loading
            // an overmap and moves groups with out-of-bounds position to another overmap.
            add_mon_group( tmp );
            xpop += tmp.population;
        }
    }
    DebugLog( D_ERROR, D_GAME ) << group.type << ": " << group.population << " => " << xpop;
}

const point overmap::invalid_point = point(INT_MIN, INT_MIN);
const tripoint overmap::invalid_tripoint = tripoint(INT_MIN, INT_MIN, INT_MIN);
//oter_id overmap::nulloter = "";
