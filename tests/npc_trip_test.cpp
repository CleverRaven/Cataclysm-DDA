#include <functional>
#include <optional>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_turn_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const faction_id faction_your_followers( "your_followers" );

// follower with every need settled, so only following drives it
static npc &spawn_settled_follower( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    return guy;
}

TEST_CASE( "npc_turn_helper_drives_a_follower_like_the_game", "[npc]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    avatar &u = get_avatar();
    u.setpos( here, tripoint_bub_ms( 65, 50, 0 ) );
    npc &guy = spawn_settled_follower( tripoint_bub_ms( 50, 50, 0 ) );
    REQUIRE( rl_dist( guy.pos_abs(), u.pos_abs() ) > guy.follow_distance() );

    const auto close_enough = [&u]( const npc & n ) {
        return rl_dist( n.pos_abs(), u.pos_abs() ) <= n.follow_distance();
    };
    run_npc_turns( guy, 30, close_enough );

    CHECK( close_enough( guy ) );
}
