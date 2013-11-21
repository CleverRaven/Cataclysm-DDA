#ifndef _MAPGEN_H_
#define _MAPGEN_H_
#include <map>
#include <string>
#include "mapgen_functions.h"

enum mapgen_function_type {
    MAPGENFUNC_ERROR,
    MAPGENFUNC_C,
    MAPGENFUNC_LUA,
    MAPGENFUNC_JSON,
};


class mapgen_function {
    public:
    mapgen_function_type ftype;
    //virtual building_gen_pointer getfunction() { return NULL; }
    virtual void dummy_() = 0;
    virtual mapgen_function_type function_type() { return ftype;/*MAPGENFUNC_ERROR;*/ };
    
};


class mapgen_function_builtin : public virtual mapgen_function {
    public:

    building_gen_pointer fptr;
    mapgen_function_builtin(building_gen_pointer ptr) : fptr(ptr) {
        ftype = MAPGENFUNC_C;
    };
    mapgen_function_builtin(std::string sptr);
    virtual void dummy_() {}
};

class mapgen_function_json : public virtual mapgen_function {
    public:
    virtual void dummy_() {}
    std::vector <std::string> data;
    mapgen_function_json(std::vector<std::string> s) {
        ftype = MAPGENFUNC_JSON;
        data = s; // dummy test
    }
};

class mapgen_function_lua : public virtual mapgen_function {
    public:
    virtual void dummy_() {}
    const std::string scr;
    mapgen_function_lua(std::string s) : scr(s) {
        ftype = MAPGENFUNC_LUA;
//        scr = s;
    }
};

extern std::map<std::string, std::vector<mapgen_function*> > oter_mapgen;

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

#endif
