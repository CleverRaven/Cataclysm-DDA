#pragma once
#ifndef CATA_TESTS_NPC_TURN_HELPERS_H
#define CATA_TESTS_NPC_TURN_HELPERS_H

#include <functional>

class npc;

// process_npc_turn(), then advance calendar::turn; fails the test if the NPC spun out
void run_npc_turn( npc &guy );

// stops early once done( guy ); returns turns run
int run_npc_turns( npc &guy, int max_turns, const std::function<bool( const npc & )> &done = {} );

#endif // CATA_TESTS_NPC_TURN_HELPERS_H
