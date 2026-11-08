#include "npc_turn_helpers.h"

#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "do_turn.h"
#include "map.h"
#include "npc.h"
#include "player_activity.h"

void run_npc_turn( npc &guy )
{
    const npc_turn_result result = process_npc_turn( get_map(), guy );
    CAPTURE( guy.get_name(), guy.get_committed_goal(), guy.activity.id().str() );
    CHECK_FALSE( result.spun_out );
    calendar::turn += 1_turns;
}

int run_npc_turns( npc &guy, const int max_turns, const std::function<bool( const npc & )> &done )
{
    int turns = 0;
    while( turns < max_turns && !( done && done( guy ) ) ) {
        run_npc_turn( guy );
        ++turns;
    }
    return turns;
}
