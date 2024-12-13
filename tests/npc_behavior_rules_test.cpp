#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "common_types.h"
#include "creature_tracker.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "memory_fast.h"
#include "npc.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "test_data.h"
#include "text_snippets.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

class Creature;

static const update_mapgen_id
update_mapgen_debug_npc_rules_test_avoid_doors( "debug_npc_rules_test_avoid_doors" );

static const update_mapgen_id
update_mapgen_debug_npc_rules_test_close_doors( "debug_npc_rules_test_close_doors" );

static const furn_str_id furn_f_chair( "f_chair" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_o( "t_door_o" );

static shared_ptr_fast<npc> setup_generic_rules_test( ally_rule rule_to_test,
        update_mapgen_id update_mapgen_id_to_apply )
{
    clear_map();
    clear_avatar();
    Character &player = get_player_character();
    tripoint_bub_ms next_to = player.pos_bub() + point::north;
    REQUIRE( next_to != player.pos_bub() );
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    clear_character( *guy );
    guy->setpos( next_to );
    npc_follower_rules &tester_rules = guy->rules;
    tester_rules = npc_follower_rules(); // just to be sure
    tester_rules.clear_overrides(); // just to be sure
    tester_rules.set_flag( rule_to_test );
    REQUIRE( tester_rules.has_flag( rule_to_test ) );
    const tripoint_abs_omt test_omt_pos = guy->global_omt_location() + point::north;
    manual_mapgen( test_omt_pos, manual_update_mapgen, update_mapgen_id_to_apply );
    return guy;
}

TEST_CASE( "NPC rules (avoid doors)", "[npc_rules]" )
{
    /* Avoid doors rule
    * Allows: Door frame, Open doors(??? pre-existing behavior)
    * DOES NOT ALLOW: closed door (unlocked)
    * Target is a chair in a room fully enclosed by concrete walls
    * The straight-line path would take them through a door frame, closed door, and open door. This would fail.
    * The legal path winds back and forth through the corridors (snake pattern) but only crosses a door frame,
    * already opened doors, and concrete floors.
    */
    const ally_rule rule_to_test = ally_rule::avoid_doors;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_avoid_doors );
    map &here = get_map();
    tripoint_bub_ms chair_target = test_subject->pos_bub();
    for( const tripoint_bub_ms &furn_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.furn( furn_loc ) == furn_f_chair ) {
            chair_target = furn_loc;
            break;
        }
    }
    // if this fails, we somehow didn't find the destination chair (???)
    REQUIRE( test_subject->pos_bub() != chair_target );
    test_subject->update_path( chair_target, true, true );
    // if this fails, we somehow didn't find a path
    REQUIRE( !test_subject->path.empty() );
    int path_position = 0;
    for( tripoint_bub_ms &loc : test_subject->path ) {
        std::string debug_log_msg = string_format( "Terrain at path position %d was %s",
                                    path_position, here.ter( loc ).id().c_str() );
        path_position++;
        CAPTURE( debug_log_msg );
        CHECK( here.ter( loc ).id() != ter_t_door_c );
    }
}

TEST_CASE( "NPC rules (close doors)", "[npc_rules]" )
{
    /* Close doors rule
    * Target is a chair in a room fully enclosed by concrete walls
    * We have a straight line path to the target, but in the way are several vexing trials!
    * An open door, a closed door, and a locked door (unlockable from adjacent INDOORS tile).
    * We must open them all, *and* close them behind us!
    */
    const ally_rule rule_to_test = ally_rule::close_doors;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_close_doors );
    // Some sanity checking to make sure we can even do this test
    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_doors ) );
    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_locks ) );
    map &here = get_map();


    tripoint_bub_ms door_unlock_position;
    for( const tripoint_bub_ms &ter_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.ter( ter_loc ) == ter_t_door_locked ) {
            door_unlock_position = ter_loc + point::south;
            break;
        }
    }
    here.set_outside_cache_dirty( door_unlock_position.z() );
    here.build_outside_cache( door_unlock_position.z() );
    REQUIRE( !here.is_outside( door_unlock_position ) );

    tripoint_bub_ms chair_target = test_subject->pos_bub();
    for( const tripoint_bub_ms &furn_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.furn( furn_loc ) == furn_f_chair ) {
            chair_target = furn_loc;
            break;
        }
    }
    // if this fails, we somehow didn't find the destination chair (???)
    REQUIRE( test_subject->pos_bub() != chair_target );
    test_subject->update_path( chair_target, true, true );
    // if this fails, we somehow didn't find a path
    REQUIRE( !test_subject->path.empty() );

    // copy our path before we lose it
    std::vector<tripoint_bub_ms> path_taken = test_subject->path;
    int turns_taken = 0;
    while( turns_taken++ < 100 && test_subject->pos_bub() != chair_target ) {
        test_subject->set_moves( 100 );
        test_subject->move();
    }

    // if this fails we didn't make it to the chair, somehow!
    CHECK( test_subject->pos_bub() == chair_target );

    for( tripoint_bub_ms &loc : path_taken ) {
        // any other terrain on the way is valid, as long as it's not an open door.
        CHECK( here.ter( loc ).id() != ter_t_door_o );
    }

}