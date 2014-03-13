#include "catalua.h"

#include <sys/stat.h>

#include "game.h"
#include "item_factory.h"
#include "item.h"
#include "pldata.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "map.h"

#ifdef LUA
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

lua_State *lua_state;

// Keep track of the current mod from which we are executing, so that
// we know where to load files from.
std::string lua_file_path = "";

// Helper functions for making working with the lua API more straightforward.
// --------------------------------------------------------------------------

// Stores item at the given stack position into the registry.
int luah_store_in_registry(lua_State* L, int stackpos) {
    lua_pushvalue(L, stackpos);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Removes item from registry and pushes on the top of stack.
void luah_remove_from_registry(lua_State* L, int item_index) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, item_index);
    luaL_unref(L, LUA_REGISTRYINDEX, item_index);
}

// Sets the metatable for the element on top of the stack.
void luah_setmetatable(lua_State* L, const char* metatable_name) {
    // Push the metatable on top of the stack.
    lua_getglobal(L, metatable_name);

    // The element we want to set the metatable for is now below the top.
    lua_setmetatable(L, -2);
}

void luah_setglobal(lua_State*L, const char* name, int index) {
    lua_pushvalue(L, index);
    lua_setglobal(L, name);
}

void update_globals(lua_State *L) {
    // Make sure the player reference is up to date.
    {
        player** player_userdata = (player**) lua_newuserdata(L, sizeof(player*));
        *player_userdata = &g->u;

        // Set the metatable for the player.
        luah_setmetatable(L, "player_metatable");

        luah_setglobal(L, "player", -1);
    }

    // Make sure the map reference is up to date.
    {
        map** map_userdata = (map**) lua_newuserdata(L, sizeof(map*));
        *map_userdata = &g->m;

        // Set the metatable for the player.
        luah_setmetatable(L, "map_metatable");

        luah_setglobal(L, "map", -1);
    }
}

// iuse abstraction to make iuse's both in lua and C++ possible
// ------------------------------------------------------------
void Item_factory::register_iuse_lua(const char* name, int lua_function) {
    iuse_function_list[name] = use_function(lua_function);
}

// Call the given string directly, used in the lua debug command.
int call_lua(std::string tocall) {
    lua_State* L = lua_state;

    update_globals(L);
    int err = luaL_dostring(L, tocall.c_str());
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error in lua command: %s", error);
    }
    return err;
}

//
int lua_mapgen(map * m, std::string terrain_type, mapgendata, int t, float, const std::string & scr) {
    lua_State* L = lua_state;
    {
        map** map_userdata = (map**) lua_newuserdata(L, sizeof(map*));
        *map_userdata = m;
        luah_setmetatable(L, "map_metatable");
        luah_setglobal(L, "map", -1);
    }

    int err = luaL_loadstring(L, scr.c_str() );
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error loading lua mapgen: %s", error);
        return err;
    }
//    int function_index = luaL_ref(L, LUA_REGISTRYINDEX); // todo; make use of this
//    lua_rawgeti(L, LUA_REGISTRYINDEX, function_index);

    lua_pushstring(L, terrain_type.c_str());
    lua_setglobal(L, "tertype");
    lua_pushinteger(L, t);
    lua_setglobal(L, "turn");

    err=lua_pcall(L, 0 , LUA_MULTRET, 0);
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error running lua mapgen: %s", error);
    }

//    luah_remove_from_registry(L, function_index); // todo: make use of this

    return err;
}

// Lua monster movement override
int lua_monster_move(monster* m) {
    lua_State *L = lua_state;

    update_globals(L);

    lua_getglobal(L, "monster_move");
    lua_getfield(L, -1, m->type->name.c_str());

    // OK our function should now be at the top.
    if(lua_isnil(L, -1)) {
        lua_settop(L, 0);
        return 0;
    }

    // Push the monster on top of the stack.
    monster** monster_userdata = (monster**) lua_newuserdata(L, sizeof(monster*));
    *monster_userdata = m;

    // Set the metatable for the monster.
    luah_setmetatable(L, "monster_metatable");

    // Call the function
    int err = lua_pcall(lua_state, 1, 0, 0);
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error in lua monster move function: %s", error);
    }
    lua_settop(L, 0);

    return 1;
}

// Custom functions that are to be wrapped from lua.
// -------------------------------------------------
static uimenu uimenu_instance;
uimenu* create_uimenu() {
    uimenu_instance = uimenu();
    return &uimenu_instance;
}

ter_t* get_terrain_type(int id) {
    return (ter_t*) &terlist[id];
}

// Manually implemented lua functions
//
// Most lua functions are generated by lua/generate_bindings.lua,
// these generated functions can be found in lua/catabindings.cpp

/*
 This function is commented out until I find a way to get a list of all
 currently loaded monsters >_>

// monster_list = game.get_monsters()
static int game_get_monsters(lua_State *L) {
    lua_createtable(L, g->_z.size(), 0); // Preallocate enough space for all our monsters.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < g->_z.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing monsters
        // 2 - k, index at which the next monster will be inserted
        // 3 - v, next monster to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        monster** monster_userdata = (monster**) lua_newuserdata(L, sizeof(monster*));
        *monster_userdata = &(g->_z[i]);
        luah_setmetatable(L, "monster_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}
*/

// items = game.items_at(x, y)
static int game_items_at(lua_State *L) {
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    std::vector<item>& items = g->m.i_at(x, y);

    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < items.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        item** item_userdata = (item**) lua_newuserdata(L, sizeof(item*));
        *item_userdata = &(items[i]);
        luah_setmetatable(L, "item_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// monster = game.monster_at(x, y)
static int game_monster_at(lua_State *L) {
    int parameter1 = (int) lua_tonumber(L, 1);
    int parameter2 = (int) lua_tonumber(L, 2);
    int monster_idx = g->mon_at(parameter1, parameter2);

    monster& mon_ref = g->zombie(monster_idx);
    monster** monster_userdata = (monster**) lua_newuserdata(L, sizeof(monster*));
    *monster_userdata = &mon_ref;
    luah_setmetatable(L, "monster_metatable");

    return 1; // 1 return values
}

// type = game.item_type(item)
static int game_item_type(lua_State *L) {
    // Create a table of the form
    // t["id"] = item.type.id
    // t["name"] = item.type.name
    // then return t

    item** item_instance = (item**) lua_touserdata(L, 1);

    lua_createtable(L, 0, 2); // Preallocate enough space for all type properties.

    lua_pushstring(L, "name");
    lua_pushstring(L, (*item_instance)->type->name.c_str());
    lua_rawset(L, -3);

    lua_pushstring(L, "id");
    lua_pushstring(L, (*item_instance)->type->id.c_str());
    lua_rawset(L, -3);

    return 1; // 1 return values
}

// game.remove_item(x, y, item)
void game_remove_item(int x, int y, item *it) {
    std::vector<item>& items = g->m.i_at(x, y);

    for( size_t i = 0; i < items.size(); ++i ) {
        if(&(items[i]) == it) {
            items.erase(items.begin() + i);
        }
    }
}

// x, y = choose_adjacent(query_string, x, y)
static int game_choose_adjacent(lua_State *L) {
    const char* parameter1 = (const char*) lua_tostring(L, 1);
    int parameter2 = (int) lua_tonumber(L, 2);
    int parameter3 = (int) lua_tonumber(L, 3);
    bool success = (bool) g->choose_adjacent(parameter1, parameter2, parameter3);
    if(success) {
        // parameter2 and parameter3 were updated by the call
        lua_pushnumber(L, parameter2);
        lua_pushnumber(L, parameter3);
        return 2; // 2 return values
    } else {
        return 0; // 0 return values
    }
}

// game.register_iuse(string, function_object)
static int game_register_iuse(lua_State *L) {
    // Make sure the first argument is a string.
    const char* name = luaL_checkstring(L, 1);
    if(!name) {
        return luaL_error(L, "First argument to game.register_iuse is not a string.");
    }

    // Make sure the second argument is a function
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // function_object is at the top of the stack, so we can just pop
    // it with luaL_ref
    int function_index = luaL_ref(L, LUA_REGISTRYINDEX);

    // Now register function_object with our iuse's
    item_controller->register_iuse_lua(name, function_index);

    return 0; // 0 return values
}

#include "../lua/catabindings.cpp"

// Load the main file of a mod
void lua_loadmod(lua_State *L, std::string base_path, std::string main_file_name) {
    std::string full_path = base_path + "/" + main_file_name;
    
    // Check if file exists first
    struct stat buffer;   
    int file_exists = stat(full_path.c_str(), &buffer) == 0;
    if(file_exists) {
        lua_file_path = base_path;
        lua_dofile(L, full_path.c_str());
        lua_file_path = "";
    }
    // debugmsg("Loading from %s", full_path.c_str());
}

// Load an arbitrary lua file
void lua_dofile(lua_State *L, const char* path) {
    int err = luaL_dofile(lua_state, path);
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error in lua module: %s", error);
    }
}

// game.dofile(file)
//
// Method to load files from lua, later should be made "safe" by
// ensuring it's being loaded from a valid path etc.
static int game_dofile(lua_State *L) {
    const char* path = luaL_checkstring(L, 1);
    
    std::string full_path = lua_file_path + "/" + path;
    lua_dofile(L, full_path.c_str());
    return 0;
}

// Registry containing all the game functions exported to lua.
// -----------------------------------------------------------
static const struct luaL_Reg global_funcs [] = {
    {"register_iuse", game_register_iuse},
    //{"get_monsters", game_get_monsters},
    {"items_at", game_items_at},
    {"item_type", game_item_type},
    {"monster_at", game_monster_at},
    {"choose_adjacent", game_choose_adjacent},
    {"dofile", game_dofile},
    {NULL, NULL}
};

// Lua initialization.
void game::init_lua() {
    lua_state = luaL_newstate();

    luaL_openlibs(lua_state); // Load standard lua libs

    // Load our custom "game" module
    luaL_register(lua_state, "game", gamelib);
    luaL_register(lua_state, "game", global_funcs);

    // Load lua-side metatables etc.
    luaL_dofile(lua_state, "lua/autoexec.lua");
}
#endif // #ifdef LUA

// If we're not using lua, need to define Use_function in a way to always call the C++ function
int use_function::call(player* player_instance, item* item_instance, bool active) {
    if(function_type == USE_FUNCTION_CPP) {
        // If it's a C++ function, simply call it with the given arguments.
        iuse tmp;
        return (tmp.*cpp_function)(player_instance, item_instance, active);
    } else {
        #ifdef LUA

        // We'll be using lua_state a lot!
        lua_State* L = lua_state;

        // If it's a lua function, the arguments have to be wrapped in
        // lua userdata's and passed on the lua stack.
        // We will now call the function f(player, item, active)

        update_globals(L);

        // Push the lua function on top of the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, lua_function);

        // Push the item on top of the stack.
        int item_in_registry;
        {
            item** item_userdata = (item**) lua_newuserdata(L, sizeof(item*));
            *item_userdata = item_instance;

            // Save a reference to the item in the registry so that we can deallocate it
            // when we're done.
            item_in_registry = luah_store_in_registry(L, -1);

            // Set the metatable for the item.
            luah_setmetatable(L, "item_metatable");
        }

        // Push the "active" parameter on top of the stack.
        lua_pushboolean(L, active);

        // Call the iuse function
        int err = lua_pcall(L, 2, 1, 0);
        if(err) {
            // Error handling.
            const char* error = lua_tostring(L, -1);
            debugmsg("Error in lua iuse function: %s", error);
        }

        // Make sure the now outdated parameters we passed to lua aren't
        // being used anymore by setting a metatable that will error on
        // access.
        luah_remove_from_registry(L, item_in_registry);
        luah_setmetatable(L, "outdated_metatable");

        return lua_tointeger(L, -1);

        #else

        // If LUA isn't defined and for some reason we registered a lua function,
        // simply do nothing.
        return 0;

        #endif

    }
}
