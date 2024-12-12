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

static const furn_str_id furn_f_chair( "f_chair" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_o( "t_door_o" );

static shared_ptr_fast<npc> setup_generic_rules_test( ally_rule rule_to_test )
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
    const shared_ptr_fast<npc> &test_subject = setup_generic_rules_test( rule_to_test );
    const tripoint_abs_omt test_omt_pos = test_subject->global_omt_location() + point::north;
    manual_mapgen( test_omt_pos, manual_update_mapgen, update_mapgen_debug_npc_rules_test_avoid_doors );
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