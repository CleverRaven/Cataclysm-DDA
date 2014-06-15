#include "catalua.h"

#include <sys/stat.h>

#include "game.h"
#include "item_factory.h"
#include "item.h"
#include "pldata.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "map.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "messages.h"
#include "debug.h"

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


void lua_callback(lua_State *, const char* callback_name) {
    call_lua(std::string("mod_callback(\"")+std::string(callback_name)+"\")");
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

overmap *get_current_overmap() {
    return g->cur_om;
}

mongroup *create_monster_group(overmap *map, std::string type, int overmap_x, int overmap_y, int z, int radius, int population) {
    map->zg.push_back(mongroup(type, overmap_x * 2, overmap_y * 2, z, radius, population));
    return &(map->zg.back());
}

it_comest *get_comestible_type(std::string name) {
    return dynamic_cast<it_comest*>(item_controller->find_template(name));
}
it_tool *get_tool_type(std::string name) {
    return dynamic_cast<it_tool*>(item_controller->find_template(name));
}
it_gun *get_gun_type(std::string name) {
    return dynamic_cast<it_gun*>(item_controller->find_template(name));
}
it_gunmod *get_gunmod_type(std::string name) {
    return dynamic_cast<it_gunmod*>(item_controller->find_template(name));
}
it_armor *get_armor_type(std::string name) {
    return dynamic_cast<it_armor*>(item_controller->find_template(name));
}


// Manually implemented lua functions
//
// Most lua functions are generated by src/lua/generate_bindings.lua,
// these generated functions can be found in src/lua/catabindings.cpp

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

// mtype = game.monster_type(name)
static int game_monster_type(lua_State *L) {
    const char* parameter1 = (const char*) lua_tostring(L, 1);

    mtype** monster_type = (mtype**) lua_newuserdata(L, sizeof(mtype*));
    *monster_type = GetMType(parameter1);
    luah_setmetatable(L, "mtype_metatable");

    return 1; // 1 return values

}

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

// item_groups = game.get_item_groups()
static int game_get_item_groups(lua_State *L) {
    std::vector<std::string> items = item_controller->get_all_group_names();

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
        lua_pushstring(L, items[i].c_str());
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// monstergroups = game.monstergroups(overmap)
static int game_monstergroups(lua_State *L) {
    overmap **userdata = (overmap**) lua_touserdata(L, 1);

    std::vector<mongroup>& mongroups = (*userdata)->zg;

    lua_createtable(L, mongroups.size(), 0); // Preallocate enough space for all our monster groups.

    // Iterate over the monster group list and insert each monster group into our returned table.
    for( size_t i = 0; i < mongroups.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing group
        // 2 - k, index at which the next group will be inserted
        // 3 - v, next group to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        mongroup** mongroup_userdata = (mongroup**) lua_newuserdata(L, sizeof(mongroup*));
        *mongroup_userdata = &(mongroups[i]);
        luah_setmetatable(L, "mongroup_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// monster_types = game.get_monster_types()
static int game_get_monster_types(lua_State *L) {
    std::vector<std::string> mtypes = MonsterGenerator::generator().get_all_mtype_ids();

    lua_createtable(L, mtypes.size(), 0); // Preallocate enough space for all our monster types.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < mtypes.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing id
        // 2 - k, index at which the next id will be inserted
        // 3 - v, next id to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        lua_pushstring(L, mtypes[i].c_str());
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
    bool success = (bool) choose_adjacent(parameter1, parameter2, parameter3);
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

#include "lua/catabindings.cpp"

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

// Custom error handler
static int traceback(lua_State *L) {
    // Get the error message
    const char* error = lua_tostring(L, -1);

    // Get the lua stack trace
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    const char* stacktrace = lua_tostring(L, -1);

    // Print a debug message.
    debugmsg("Error in lua module: %s", error);

    // Print the stack trace to our debug log.
    std::ofstream debug_out;
    debug_out.open(FILENAMES["debug"].c_str(), std::ios_base::app | std::ios_base::out);
    debug_out << stacktrace << "\n";
    debug_out.close();
    return 1;
}

// Load an arbitrary lua file
void lua_dofile(lua_State *L, const char* path) {
    lua_pushcfunction(L, &traceback);
    luaL_loadfile(L, path) || lua_pcall(L, 0, LUA_MULTRET, -2);
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
    {"monster_type", game_monster_type},
    {"dofile", game_dofile},
    {"get_monster_types", game_get_monster_types},
    {"monstergroups", game_monstergroups},
    {"get_item_groups", game_get_item_groups},
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
    luaL_dofile(lua_state, FILENAMES["class_defslua"].c_str());
    luaL_dofile(lua_state, FILENAMES["autoexeclua"].c_str());
}
#endif // #ifdef LUA

use_function::~use_function() {
    if (function_type == USE_FUNCTION_ACTOR_PTR) {
        delete actor_ptr;
    }
}

use_function::use_function(const use_function &other)
: function_type(other.function_type)
{
    if (function_type == USE_FUNCTION_CPP) {
        cpp_function = other.cpp_function;
    } else if (function_type == USE_FUNCTION_ACTOR_PTR) {
        actor_ptr = other.actor_ptr->clone();
    } else {
        lua_function = other.lua_function;
    }
}

void use_function::operator=(use_function_pointer f)
{
    this->~use_function();
    new (this) use_function(f);
}

void use_function::operator=(iuse_actor *f)
{
    this->~use_function();
    new (this) use_function(f);
}

void use_function::operator=(const use_function &other)
{
    this->~use_function();
    new (this) use_function(other);
}

// If we're not using lua, need to define Use_function in a way to always call the C++ function
int use_function::call(player* player_instance, item* item_instance, bool active) const {
    if (function_type == USE_FUNCTION_NONE) {
        if (player_instance != NULL && player_instance->is_player()) {
            add_msg(_("You can't do anything interesting with your %s."), item_instance->tname().c_str());
        }
    } else if (function_type == USE_FUNCTION_CPP) {
        // If it's a C++ function, simply call it with the given arguments.
        iuse tmp;
        return (tmp.*cpp_function)(player_instance, item_instance, active);
    } else if (function_type == USE_FUNCTION_ACTOR_PTR) {
        return actor_ptr->use(player_instance, item_instance, active);
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
    return 0;
}
