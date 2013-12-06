#ifndef _MAPGEN_H_
#define _MAPGEN_H_
#include <map>
#include <string>
#include "mapgenformat.h"
#include "mapgen_functions.h"

//////////////////////////////////////////////////////////////////////////
///// function pointer class; provides absract referencing of
///// map generator functions written in multiple ways for per-terrain
///// random selection pool
enum mapgen_function_type {
    MAPGENFUNC_ERROR,
    MAPGENFUNC_C,
    MAPGENFUNC_LUA,
    MAPGENFUNC_JSON,
};

class mapgen_function {
    public:
    mapgen_function_type ftype;
    int weight;
    virtual void dummy_() = 0;
    virtual mapgen_function_type function_type() { return ftype;/*MAPGENFUNC_ERROR;*/ };
};


/////////////////////////////////////////////////////////////////////////////////
///// builtin mapgen
class mapgen_function_builtin : public virtual mapgen_function {
    public:
    building_gen_pointer fptr;
    mapgen_function_builtin(building_gen_pointer ptr, int w = 1000) : fptr(ptr) {
        ftype = MAPGENFUNC_C;
        weight = w;
    };
    mapgen_function_builtin(std::string sptr, int w = 1000);
    virtual void dummy_() {}
};

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen (and friends)
/*
 * Actually a pair of shorts that can rng, for numbers that will never exceed 32768
 */
struct jmapgen_int {
  short val;
  short valmax;
  jmapgen_int(int v) : val(v), valmax(v) {}
  jmapgen_int(int v, int v2) : val(v), valmax(v2) {}
  jmapgen_int( point p ) : val(p.x), valmax(p.y) {}
  
  int get() const {
      return ( val == valmax ? val : rng(val, valmax) );
  }
};

enum jmapgen_setmap_op {
    JMAPGEN_SETMAP_OPTYPE_POINT = 0,
    JMAPGEN_SETMAP_TER,
    JMAPGEN_SETMAP_FURN,
    JMAPGEN_SETMAP_TRAP,
    JMAPGEN_SETMAP_RADIATION,
    JMAPGEN_SETMAP_OPTYPE_LINE = 100,
    JMAPGEN_SETMAP_LINE_TER,
    JMAPGEN_SETMAP_LINE_FURN,
    JMAPGEN_SETMAP_LINE_TRAP,
    JMAPGEN_SETMAP_LINE_RADIATION,
    JMAPGEN_SETMAP_OPTYPE_SQUARE = 200,
    JMAPGEN_SETMAP_SQUARE_TER,
    JMAPGEN_SETMAP_SQUARE_FURN,
    JMAPGEN_SETMAP_SQUARE_TRAP,
    JMAPGEN_SETMAP_SQUARE_RADIATION
};



struct jmapgen_setmap {
    jmapgen_int x;
    jmapgen_int y;
    jmapgen_int x2;
    jmapgen_int y2;
    jmapgen_setmap_op op;
    jmapgen_int val;
    int chance;
    jmapgen_int repeat;

    jmapgen_setmap(
       jmapgen_int ix, jmapgen_int iy, jmapgen_int ix2, jmapgen_int iy2,
       jmapgen_setmap_op iop, jmapgen_int ival, 
       int ione_in = 1, jmapgen_int irepeat = jmapgen_int(1,1)
    ) :
       x(ix), y(iy), x2(ix2), y2(iy2), op(iop), val(ival), chance(ione_in), repeat(irepeat)
    {}
    bool apply( map * m );
};

/* todo: place_gas_pump, place_toile, add_spawn */

enum jmapgen_place_group_op {
    JMAPGEN_PLACEGROUP_ITEM,
    JMAPGEN_PLACEGROUP_MONSTER
};

struct jmapgen_place_group {
    jmapgen_int x;
    jmapgen_int y;
    std::string gid;
    jmapgen_place_group_op op;
    int chance;
    float density;
    jmapgen_int repeat;    
    jmapgen_place_group(jmapgen_int ix, jmapgen_int iy, std::string igid, jmapgen_place_group_op iop, int ichance,
        float idensity = -1.0f, jmapgen_int irepeat = jmapgen_int(1,1)
      ) : x(ix), y(iy), gid(igid), op(iop), chance(ichance), density(idensity), repeat(irepeat) { }
    void apply( map * m, float mdensity, int t );
};

struct jmapgen_spawn_item {
    jmapgen_int x;
    jmapgen_int y;
    std::string itype;
    jmapgen_int amount;
    int chance;
    jmapgen_int repeat;
    jmapgen_spawn_item( const jmapgen_int ix, jmapgen_int iy, std::string iitype, jmapgen_int iamount, int ichance = 1,
        jmapgen_int irepeat = jmapgen_int(1,1) ) : 
      x(ix), y(iy), itype(iitype), amount(iamount), chance(ichance), repeat(irepeat) {}
    void apply( map * m );

};

class mapgen_function_json : public virtual mapgen_function {
    public:
    virtual void dummy_() {}
    bool check_inbounds( jmapgen_int & var );
    void setup_place_group(JsonArray &parray );
    void setup_setmap(JsonArray &parray);
    bool setup();
    void apply(map * m,oter_id id,mapgendata md ,int t,float d);
    mapgen_function_json(std::string s, int w = 1000) {
        ftype = MAPGENFUNC_JSON;
        weight = w;
        jdata = s;
        mapgensize = 24;
        fill_ter = -1;
        is_ready = false;
        do_format = false;
    }

    std::string jdata;
    int mapgensize;
    int fill_ter;
    terfurn_tile * format;
    std::vector<jmapgen_setmap> setmap_points;
    std::vector<jmapgen_spawn_item> spawnitems;
    std::vector<jmapgen_place_group> place_groups;
    std::string luascript;

    bool do_format;
    bool is_ready;
};

/////////////////////////////////////////////////////////////////////////////////
///// lua mapgen
class mapgen_function_lua : public virtual mapgen_function {
    public:
    virtual void dummy_() {}
    const std::string scr;
    mapgen_function_lua(std::string s, int w = 1000) : scr(s) {
        ftype = MAPGENFUNC_LUA;
        weight = w;
        // scr = s; // todo; if ( luaL_loadstring(L, scr.c_str() ) ) { error }
    }
};
/////////////////////////////////////////////////////////
///// global per-terrain mapgen function lists
/*
 * Load mapgen function of any type from a jsonobject
 */
mapgen_function * load_mapgen_function(JsonObject &jio, const std::string id_base, int default_idx);
/*
 * Load the above directly from a file via init, as opposed to riders attached to overmap_terrain. Added check
 * for oter_mapgen / oter_mapgen_weights key, multiple possible ( ie, [ "house", "house_base" ] )
 */
void load_mapgen ( JsonObject &jio );
/*
 * stores function ref and/or required data
 */
extern std::map<std::string, std::vector<mapgen_function*> > oter_mapgen;
/*
 * random selector list for the nested vector above, as per indivdual mapgen_function_::weight value
 */
extern std::map<std::string, std::map<int, int> > oter_mapgen_weights;
/*
 * Sets the above after init, and initializes mapgen_function_json instances as well
 */
void calculate_mapgen_weights();

/// move to building_generation
enum room_type {
    room_null,
    room_closet,
    room_lobby,
    room_chemistry,
    room_teleport,
    room_goo,
    room_cloning,
    room_vivisect,
    room_bionics,
    room_dorm,
    room_living,
    room_bathroom,
    room_kitchen,
    room_bedroom,
    room_backyard,
    room_study,
    room_mine_shaft,
    room_mine_office,
    room_mine_storage,
    room_mine_fuel,
    room_mine_housing,
    room_bunker_bots,
    room_bunker_launcher,
    room_bunker_rifles,
    room_bunker_grenades,
    room_bunker_armor,
    room_mansion_courtyard,
    room_mansion_entry,
    room_mansion_bedroom,
    room_mansion_library,
    room_mansion_kitchen,
    room_mansion_dining,
    room_mansion_game,
    room_mansion_pool,
    room_mansion_study,
    room_mansion_bathroom,
    room_mansion_gallery,
    room_split
};

void house_room(map *m, room_type type, int x1, int y1, int x2, int y2);
// helpful functions
bool connects_to(oter_id there, int dir);
void mapgen_rotate( map * m, oter_id terrain_type, bool north_is_down = false );
// wrappers for map:: functions
void line(map *m, const ter_id type, int x1, int y1, int x2, int y2);
void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void fill_background(map *m, ter_id type);
void fill_background(map *m, ter_id (*f)());
void square(map *m, ter_id type, int x1, int y1, int x2, int y2);
void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2);
void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void rough_circle(map *m, ter_id type, int x, int y, int rad);
void add_corpse(game *g, map *m, int x, int y);

#endif
