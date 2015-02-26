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

#if LUA_VERSION_NUM < 502
#define LUA_OK 0
#endif

lua_State *lua_state;

// Keep track of the current mod from which we are executing, so that
// we know where to load files from.
std::string lua_file_path = "";

// Helper functions for making working with the lua API more straightforward.
// --------------------------------------------------------------------------

// Stores item at the given stack position into the registry.
int luah_store_in_registry(lua_State *L, int stackpos)
{
    lua_pushvalue(L, stackpos);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Removes item from registry and pushes on the top of stack.
void luah_remove_from_registry(lua_State *L, int item_index)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, item_index);
    luaL_unref(L, LUA_REGISTRYINDEX, item_index);
}

// Sets the metatable for the element on top of the stack.
void luah_setmetatable(lua_State *L, const char *metatable_name)
{
    // Push the metatable on top of the stack.
    lua_getglobal(L, metatable_name);

    // The element we want to set the metatable for is now below the top.
    lua_setmetatable(L, -2);
}

void luah_setglobal(lua_State *L, const char *name, int index)
{
    lua_pushvalue(L, index);
    lua_setglobal(L, name);
}

// Given a Lua return code and a file that it happened in, print a debugmsg with the error and path.
// Returns true if there was an error, false if there was no error at all.
bool lua_report_error(lua_State *L, int err, const char *path) {
    if( err == LUA_OK || err == LUA_ERRRUN ) {
        // No error or error message already shown via traceback function.
        return err != LUA_OK;
    }
    const char *error = lua_tostring(L, -1);
    switch(err) {
        case LUA_ERRSYNTAX:
            debugmsg( "Lua returned syntax error for %s\n%s", path, error );
            break;
        case LUA_ERRMEM:
            debugmsg( "Lua is out of memory" );
            break;
        case LUA_ERRFILE:
            debugmsg( "Lua returned file io error for %s\n%s", path, error );
            break;
        default:
            debugmsg( "Lua returned unknown error %d for %s\n%s", err, path, error );
            break;
    }
    return true;
}

void update_globals(lua_State *L)
{
    // Make sure the player reference is up to date.
    {
        player **player_userdata = (player **) lua_newuserdata(L, sizeof(player *));
        *player_userdata = &g->u;

        // Set the metatable for the player.
        luah_setmetatable(L, "player_metatable");

        luah_setglobal(L, "player", -1);
    }

    // Make sure the map reference is up to date.
    {
        map **map_userdata = (map **) lua_newuserdata(L, sizeof(map *));
        *map_userdata = &g->m;

        // Set the metatable for the player.
        luah_setmetatable(L, "map_metatable");

        luah_setglobal(L, "map", -1);
    }
}

// iuse abstraction to make iuse's both in lua and C++ possible
// ------------------------------------------------------------
void Item_factory::register_iuse_lua(const std::string &name, int lua_function)
{
    if( iuse_function_list.count( name ) > 0 ) {
        DebugLog(D_INFO, D_MAIN) << "lua iuse function " << name << " overrides existing iuse function";
    }
    iuse_function_list[name] = use_function(lua_function);
}

// Call the given string directly, used in the lua debug command.
int call_lua(std::string tocall)
{
    lua_State *L = lua_state;

    update_globals(L);
    int err = luaL_dostring(L, tocall.c_str());
    lua_report_error(L, err, tocall.c_str());
    return err;
}


void lua_callback(lua_State *, const char *callback_name)
{
    call_lua(std::string("mod_callback(\"") + std::string(callback_name) + "\")");
}

//
int lua_mapgen(map *m, std::string terrain_type, mapgendata, int t, float, const std::string &scr)
{
    if( lua_state == nullptr ) {
        return 0;
    }
    lua_State *L = lua_state;
    {
        map **map_userdata = (map **) lua_newuserdata(L, sizeof(map *));
        *map_userdata = m;
        luah_setmetatable(L, "map_metatable");
        luah_setglobal(L, "map", -1);
    }

    int err = luaL_loadstring(L, scr.c_str() );
    if( lua_report_error( L, err, scr.c_str() ) ) {
        return err;
    }
    //    int function_index = luaL_ref(L, LUA_REGISTRYINDEX); // todo; make use of this
    //    lua_rawgeti(L, LUA_REGISTRYINDEX, function_index);

    lua_pushstring(L, terrain_type.c_str());
    lua_setglobal(L, "tertype");
    lua_pushinteger(L, t);
    lua_setglobal(L, "turn");

    err = lua_pcall(L, 0 , LUA_MULTRET, 0);
    lua_report_error( L, err, scr.c_str() );

    //    luah_remove_from_registry(L, function_index); // todo: make use of this

    return err;
}

// Custom functions that are to be wrapped from lua.
// -------------------------------------------------
static uimenu uimenu_instance;
uimenu *create_uimenu()
{
    uimenu_instance = uimenu();
    return &uimenu_instance;
}

ter_t *get_terrain_type(int id)
{
    return (ter_t *) &terlist[id];
}

overmap *get_current_overmap()
{
    return g->cur_om;
}

/** Create a new monster of the given type. */
monster *create_monster(std::string mon_type, int x, int y)
{
    monster new_monster(GetMType(mon_type), x, y);
    if(!g->add_zombie(new_monster)) {
        return NULL;
    } else {
        return &(g->zombie(g->mon_at(x, y)));
    }
}

it_comest *get_comestible_type(std::string name)
{
    return dynamic_cast<it_comest *>(item::find_type(name));
}
it_tool *get_tool_type(std::string name)
{
    return dynamic_cast<it_tool *>(item::find_type(name));
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
static int game_monster_type(lua_State *L)
{
    const char *parameter1 = (const char *) lua_tostring(L, 1);

    mtype **monster_type = (mtype **) lua_newuserdata(L, sizeof(mtype *));
    *monster_type = GetMType(parameter1);
    luah_setmetatable(L, "mtype_metatable");

    return 1; // 1 return values

}

static void popup_wrapper(const std::string &text) {
    popup( "%s", text.c_str() );
}

static void add_msg_wrapper(const std::string &text) {
    add_msg( "%s", text.c_str() );
}

// items = game.items_at(x, y)
static int game_items_at(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    auto items = g->m.i_at(x, y);
    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    int i = 0;
    for( auto &an_item : items ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i++ + 1);
        item **item_userdata = (item **) lua_newuserdata(L, sizeof(item *));
        *item_userdata = &an_item;
        luah_setmetatable(L, "item_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// item_groups = game.get_item_groups()
static int game_get_item_groups(lua_State *L)
{
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

// monster_types = game.get_monster_types()
static int game_get_monster_types(lua_State *L)
{
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
static int game_monster_at(lua_State *L)
{
    int parameter1 = (int) lua_tonumber(L, 1);
    int parameter2 = (int) lua_tonumber(L, 2);
    int monster_idx = g->mon_at(parameter1, parameter2);

    monster &mon_ref = g->zombie(monster_idx);
    monster **monster_userdata = (monster **) lua_newuserdata(L, sizeof(monster *));
    *monster_userdata = &mon_ref;
    luah_setmetatable(L, "monster_metatable");

    return 1; // 1 return values
}

// type = game.item_type(item)
static int game_item_type(lua_State *L)
{
    // Create a table of the form
    // t["id"] = item.type.id
    // t["name"] = item.type.name
    // then return t

    item **item_instance = (item **) lua_touserdata(L, 1);

    lua_createtable(L, 0, 2); // Preallocate enough space for all type properties.

    lua_pushstring(L, "name");
    lua_pushstring(L, (*item_instance)->type_name( 1 ).c_str());
    lua_rawset(L, -3);

    lua_pushstring(L, "id");
    lua_pushstring(L, (*item_instance)->type->id.c_str());
    lua_rawset(L, -3);

    return 1; // 1 return values
}

// game.remove_item(x, y, item)
void game_remove_item(int x, int y, item *it)
{
    g->m.i_rem( x, y, it );
}

// x, y = choose_adjacent(query_string, x, y)
static int game_choose_adjacent(lua_State *L)
{
    const char *parameter1 = (const char *) lua_tostring(L, 1);
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
static int game_register_iuse(lua_State *L)
{
    // Make sure the first argument is a string.
    const char *name = luaL_checkstring(L, 1);
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
void lua_loadmod(lua_State *L, std::string base_path, std::string main_file_name)
{
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
static int traceback(lua_State *L)
{
    // Get the error message
    const char *error = lua_tostring(L, -1);

    // Get the lua stack trace
#if LUA_VERSION_NUM < 502
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    lua_getfield(L, -1, "traceback");
#else
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
#endif
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    const char *stacktrace = lua_tostring(L, -1);

    // Print a debug message.
    debugmsg("Error in lua module: %s", error);

    // Print the stack trace to our debug log.
    DebugLog( D_ERROR, DC_ALL ) << stacktrace;
    return 1;
}

// Load an arbitrary lua file
void lua_dofile(lua_State *L, const char *path)
{
    lua_pushcfunction(L, &traceback);
    int err = luaL_loadfile(L, path);
    if( lua_report_error( L, err, path ) ) {
        return;
    }
    err = lua_pcall(L, 0, LUA_MULTRET, -2);
    lua_report_error( L, err, path );
}

// game.dofile(file)
//
// Method to load files from lua, later should be made "safe" by
// ensuring it's being loaded from a valid path etc.
static int game_dofile(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

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
    {"get_item_groups", game_get_item_groups},
    {NULL, NULL}
};

// Lua initialization.
void game::init_lua()
{
    lua_state = luaL_newstate();
    if( lua_state == nullptr ) {
        debugmsg( "Failed to start Lua. Lua scripting won't be available." );
        return;
    }

    luaL_openlibs(lua_state); // Load standard lua libs

    // Load our custom "game" module
#if LUA_VERSION_NUM < 502
    luaL_register(lua_state, "game", gamelib);
    luaL_register(lua_state, "game", global_funcs);
#else
    std::vector<luaL_Reg> lib_funcs;
    for( auto x = gamelib; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    for( auto x = global_funcs; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    lib_funcs.push_back( luaL_Reg { NULL, NULL } );
    luaL_newmetatable(lua_state, "game");
    lua_pushvalue(lua_state, -1);
    luaL_setfuncs(lua_state, &lib_funcs.front(), 0);
    lua_setglobal(lua_state, "game");
#endif

    // Load lua-side metatables etc.
    lua_dofile(lua_state, FILENAMES["class_defslua"].c_str());
    lua_dofile(lua_state, FILENAMES["autoexeclua"].c_str());
}

#endif // #ifdef LUA

use_function::~use_function()
{
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
long use_function::call(player *player_instance, item *item_instance, bool active, point pos) const
{
    if (function_type == USE_FUNCTION_NONE) {
        if (player_instance != NULL && player_instance->is_player()) {
            add_msg(_("You can't do anything interesting with your %s."), item_instance->tname().c_str());
        }
    } else if (function_type == USE_FUNCTION_CPP) {
        // If it's a C++ function, simply call it with the given arguments.
        iuse tmp;
        return (tmp.*cpp_function)(player_instance, item_instance, active, pos);
    } else if (function_type == USE_FUNCTION_ACTOR_PTR) {
        return actor_ptr->use(player_instance, item_instance, active, pos);
    } else {
#ifdef LUA

        // We'll be using lua_state a lot!
        lua_State *L = lua_state;

        // If it's a lua function, the arguments have to be wrapped in
        // lua userdata's and passed on the lua stack.
        // We will now call the function f(player, item, active)

        update_globals(L);

        // Push the lua function on top of the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, lua_function);

        // TODO: also pass the player object, because of NPCs and all
        //       I guess

        // Push the item on top of the stack.
        int item_in_registry;
        {
            item **item_userdata = (item **) lua_newuserdata(L, sizeof(item *));
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
        lua_report_error( L, err, "iuse function" );

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
