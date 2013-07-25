#include "game.h"
#include "item_factory.h"
#include "item.h"
#include "pldata.h"

extern "C" {
#include "lua5.1/lua.h"
#include "lua5.1/lualib.h"
#include "lua5.1/lauxlib.h"
}

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

// Wrapper functions accessible from lua
// -------------------------------------

void Item_factory::register_iuse_lua(const char* name, int lua_function) {
    iuse_function_list[name] = Use_function(lua_function);
}

void Use_function::call(game* game, player* player_instance, item* item_instance, bool active) {
    // We'll be using g->lua_state a lot!
    lua_State* L = g->lua_state;
    
    if(function_type == USE_FUNCTION_CPP) {
        // If it's a C++ function, simply call it with the given arguments.
        iuse tmp;
        (tmp.*cpp_function)(game, player_instance, item_instance, active);
    } else {
        // If it's a lua function, the arguments have to be wrapped in
        // lua userdata's and passed on the lua stack.
        // We will now call the function f(item, active)
        
        // Push the lua function on top of the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, lua_function_index);

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
        lua_pushboolean(g->lua_state, active);

        // Make sure the player global is up-to-date.
        {
            player** player_userdata = (player**) lua_newuserdata(g->lua_state, sizeof(player*));
            *player_userdata = &(g->u);

            // Set the metatable for the player.
            luah_setmetatable(g->lua_state, "player_metatable");
            
            // Store the player as global and back them up in the registry.
            lua_setglobal(g->lua_state, "player");
        }

        // Call the iuse function
        int err = lua_pcall(g->lua_state, 2, 0, 0);
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
    }
}

// Popup a text input to enter arbitrary scripts and execute it.
void lua_command() {
    std::string command = string_input_popup("Lua:", 70, "");

    // Make sure the player global is up-to-date.
    {
        player** player_userdata = (player**) lua_newuserdata(g->lua_state, sizeof(player*));
        *player_userdata = &(g->u);

        // Set the metatable for the player.
        luah_setmetatable(g->lua_state, "player_metatable");
        
        // Store the player as global and back them up in the registry.
        lua_setglobal(g->lua_state, "player");
    }
    
    int err = luaL_dostring(g->lua_state, command.c_str());
    if(err) {
        // Error handling.
        const char* error = lua_tostring(g->lua_state, -1);
        debugmsg("Error in lua command: %s", error);
    }
}

// Call a "lua tick" function that will be called once per game-tick.
void lua_tick() {
    lua_State *L = g->lua_state;
    lua_getglobal(L, "game");
    if(!lua_istable(L, -1)) {
        debugmsg("LUA ERROR: Lua global 'game' is not a table. Please do not modify the game global.");
        return;
    }

    // We want to call game.tick()
    lua_pushstring(L, "tick");
    lua_rawget(L, -2);

    // Now our function is at the top of the stack.
    if(lua_isnil(L, -1)) {
        // Tick function may be undefined.
        return;
    }
    if(!lua_isfunction(L, -1)) {
        debugmsg("LUA ERROR: Lua global 'game.tick' is not a function. Please make sure to define it as function.");
        return;
    }

    // Make sure the player global is up-to-date.
    {
        player** player_userdata = (player**) lua_newuserdata(g->lua_state, sizeof(player*));
        *player_userdata = &(g->u);

        // Set the metatable for the player.
        luah_setmetatable(g->lua_state, "player_metatable");
        
        // Store the player as global and back them up in the registry.
        lua_setglobal(g->lua_state, "player");
    }
    
    int err = lua_pcall(L, 0, 0, 0);
    if(err) {
        // Error handling.
        const char* error = lua_tostring(L, -1);
        debugmsg("Error in lua game.tick function: %s", error);
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


#include "catalua/catabindings.cpp"

// Registry containing all the game functions exported to lua.
// -----------------------------------------------------------
static const struct luaL_Reg global_funcs [] = {
    {"register_iuse", game_register_iuse},
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
    luaL_dofile(lua_state,"catalua.lua");

    // Load main lua mod
    int err = luaL_dofile(lua_state,"data/luamods/core/main.lua");
    if(err) {
        // Error handling.
        const char* error = lua_tostring(lua_state, -1);
        debugmsg("Error in lua main module: %s", error);
    }
}

