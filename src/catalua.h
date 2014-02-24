#ifndef _CATALUA_H_
#define _CATALUA_H_

#include "monster.h"
#include "mapgen_functions.h"
#ifdef LUA

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

extern lua_State *lua_state;

/** If this returns 0, no lua function was defined to override behavior.
 *  If this returns 1, lua behavior was called and regular behavior should be omitted.
 */
int lua_monster_move(monster* m);

/**
 * Call the given string as lua code, used for interactive debugging.
 */
int call_lua(std::string tocall);
int lua_mapgen(map * m, std::string terrain_type, mapgendata md, int t, float d, const std::string & scr);

/**
 * Execute a lua file.
 */
void lua_dofile(lua_State *L, const char* path);

/**
 * Load the main file of a lua mod.
 * 
 * @param base_path The base path of the mod.
 * @param main_file_name The file name of the lua file, usually "main.lua"
 */
void lua_loadmod(lua_State *L, std::string base_path, std::string main_file_name);

#endif

#endif
