#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "gates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "mapgen_helpers.h"
#include "memory_fast.h"
#include "npc.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

static const furn_str_id furn_f_chair( "f_chair" );

static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_o( "t_door_o" );

static const update_mapgen_id
update_mapgen_debug_npc_rules_test_avoid_doors( "debug_npc_rules_test_avoid_doors" );
static const update_mapgen_id
update_mapgen_debug_npc_rules_test_close_doors( "debug_npc_rules_test_close_doors" );

static const vproto_id vehicle_prototype_locked_as_hell_car( "locked_as_hell_car" );

static shared_ptr_fast<npc> setup_generic_rules_test( ally_rule rule_to_test,
        update_mapgen_id update_mapgen_id_to_apply )
{
    map &here = get_map();
    clear_map();
    clear_vehicles();
    clear_avatar();
    Character &player = get_player_character();
    tripoint_bub_ms next_to = player.pos_bub() + point::north;
    REQUIRE( next_to != player.pos_bub() );
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    overmap_buffer.insert_npc( guy );
    g->load_npcs();
    clear_character( *guy );
    guy->setpos( here, next_to );
    talk_function::follow( *guy );
    // rules don't work unless they're an ally.
    REQUIRE( guy->is_player_ally() );
    npc_follower_rules &tester_rules = guy->rules;
    tester_rules = npc_follower_rules(); // just to be sure
    tester_rules.clear_overrides(); // just to be sure
    tester_rules.set_flag( rule_to_test );
    REQUIRE( tester_rules.has_flag( rule_to_test ) );
    const tripoint_abs_omt test_omt_pos = guy->pos_abs_omt() + point::north;
    manual_mapgen( test_omt_pos, manual_update_mapgen, update_mapgen_id_to_apply );
    return guy;
}

TEST_CASE( "NPC-rules-avoid-doors", "[npc_rules]" )
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

TEST_CASE( "NPC-rules-close-doors", "[npc_rules]" )
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

    // we must force them to actually walk the path for this test
    test_subject->goto_to_this_pos = here.get_abs( test_subject->path.back() );

    // copy our path before we lose it
    std::vector<tripoint_bub_ms> path_taken = test_subject->path;
    int turns_taken = 0;
    while( turns_taken++ < 100 && test_subject->pos_bub() != chair_target ) {
        test_subject->set_moves( 100 );
        test_subject->move();
    }

    // if one of these fails we didn't make it to the chair, somehow!
    CHECK( turns_taken < 100 );
    CHECK( test_subject->pos_bub() == chair_target );

    for( tripoint_bub_ms &loc : path_taken ) {
        // any other terrain on the way is valid, as long as it's not an open door.
        // (since test area is spawned *nearby* they might walk over some random dirt, grass, w/e on the way to the test area)
        CHECK( here.ter( loc ).id() != ter_t_door_o );
    }

}

TEST_CASE( "NPC-rules-avoid-locks", "[npc_rules]" )
{
    /* Avoid locked doors rule
    * Target is a the north side of a locked door (otherwise inaccessible room)
    * We can open the door, but our rules forbid it
    * Test succeeds if NPC fails to path to other side of door
    *
    * For the second part, we repeat the test with a vehicle locked door.
    * The NPC is placed inside the vehicle for this test.
    * In this case, our target is the north side of a locked door (otherwise inaccessible vehicle)
    * We can unlock and open the door, but our rules forbid it.
    * Test succeeds if NPC fails to path to outside the vehicle.
    */
    const ally_rule rule_to_test = ally_rule::avoid_locks;
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test,
            update_mapgen_debug_npc_rules_test_close_doors );
    // Some sanity checking to make sure we can even do this test
    REQUIRE( !test_subject->rules.has_flag( ally_rule::avoid_doors ) );
    map &here = get_map();


    tripoint_bub_ms door_position;
    for( const tripoint_bub_ms &ter_loc : here.points_in_radius( test_subject->pos_bub(), 60 ) ) {
        if( here.ter( ter_loc ) == ter_t_door_locked ) {
            door_position = ter_loc;
            break;
        }
    }
    tripoint_bub_ms door_unlock_position = door_position + point::south;
    tripoint_bub_ms past_the_door = door_position + point::north;
    here.set_outside_cache_dirty( door_position.z() );
    here.build_outside_cache( door_position.z() );
    REQUIRE( !here.is_outside( door_unlock_position ) );

    test_subject->update_path( door_unlock_position, true, true );
    REQUIRE( !test_subject->path.empty() ); // we can reach the unlock position
    test_subject->path.clear();
    test_subject->update_path( past_the_door, true, true );
    // FIXME: NPC rules do not consider locked terrain doors, only vehicles
    // CHECK( test_subject->path.empty() );
    test_subject->path.clear();

    const tripoint_bub_ms car_center_pos = test_subject->pos_bub() + tripoint_rel_ms{0, 10, 0};
    const tripoint_bub_ms car_door_pos = car_center_pos + point_rel_ms{0, -2};
    const tripoint_bub_ms car_door_unlock_pos = car_door_pos + point::south;
    const tripoint_bub_ms outside_car_door_pos = car_door_pos + point::north;


    vehicle *test_vehicle = here.add_vehicle( vehicle_prototype_locked_as_hell_car,
                            car_center_pos, 0_degrees, 0, 0 );

    // vehicle is a 5x5 grid, car_door_pos is the only door/exit
    std::vector<vehicle_part *> door_parts_at_target = test_vehicle->get_parts_at(
                &here, car_door_pos, "LOCKABLE_DOOR", part_status_flag::available );
    REQUIRE( !door_parts_at_target.empty() );
    vehicle_part *door = door_parts_at_target.front();
    // The door must be closed for the lock to be effective.
    door->open = false;
    // For some reason, both the door and the door lock must be set to locked.
    door->locked = true;

    // NOTE: The door lock is a separate part. We must ensure both the door exists and the door lock exists for this test.
    std::vector<vehicle_part *> door_lock_parts_at_target = test_vehicle->get_parts_at(
                &here, car_door_pos, "DOOR_LOCKING", part_status_flag::available );
    REQUIRE( !door_lock_parts_at_target.empty() );
    vehicle_part *door_lock = door_lock_parts_at_target.front();
    door_lock->locked = true;
    door_lock->open = false;
    REQUIRE( ( door_lock->is_available() && door_lock->locked ) );


    test_subject->setpos( here, car_door_unlock_pos );
    here.board_vehicle( car_door_unlock_pos, &*test_subject );

    CHECK( doors::can_unlock_door( here, *test_subject, car_door_pos ) );

    test_subject->update_path( outside_car_door_pos, true, true );
    // if this check fails with a path size of 2, we pathed straight through the door.
    // if size is > 2, we somehow pathed out of the vehicle without going through the door
    CAPTURE( test_subject->path.size() );
    CHECK( test_subject->path.empty() );

    // and now we check that the opposite is true, that they are allowed to exit the vehicle when the flag is not set
    test_subject->rules.clear_flag( rule_to_test );
    test_subject->update_path( outside_car_door_pos, true, true );
    CHECK( !test_subject->path.empty() );

}
