#pragma once
#ifndef CATALUA_H
#define CATALUA_H

#include "int_id.h"

#include <string>
#include <sstream>

class map;
class monster;
struct mapgendata;
struct oter_t;

using oter_id = int_id<oter_t>;

extern std::stringstream lua_output_stream;
extern std::stringstream lua_error_stream;

/** If this returns 0, no lua function was defined to override behavior.
 *  If this returns 1, lua behavior was called and regular behavior should be omitted.
 */
int lua_monster_move( monster *m );

/**
 * Call the given string as lua code, used for interactive debugging.
 */
int call_lua( std::string tocall );
int lua_mapgen( map *m, const oter_id &terrain_type, const mapgendata &md, int t, float d,
                const std::string &scr );

/**
 * Execute a callback that can be overriden by all mods.
 */
void lua_callback( const char *callback_name );

/**
 * Load the main file of a lua mod.
 *
 * @param base_path The base path of the mod.
 * @param main_file_name The file name of the lua file, usually "main.lua"
 */
void lua_loadmod( std::string base_path, std::string main_file_name );

#endif
