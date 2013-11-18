#ifndef _CATALUA_H_
#define _CATALUA_H_

#include "monster.h"

#ifdef LUA

/** If this returns 0, no lua function was defined to override behavior.
 *  If this returns 1, lua behavior was called and regular behavior should be omitted.
 */
int lua_monster_move(monster* m);

/**
 * Call the given string as lua code, used for interactive debugging.
 */
int call_lua(std::string tocall);

#endif

#endif
