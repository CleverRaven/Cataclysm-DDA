#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "itype.h"
#include "npc.h"
#include "type_id.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "activity_scheduling_helper.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );

static const efftype_id effect_bandaged( "bandaged" );

static const itype_id itype_bandages( "bandages" );

static const skill_id skill_firstaid( "firstaid" );

static void process_activity_interrupt( Character &guy, const int interrupt_time )
{
    do {
        guy.moves += guy.get_speed();
        while( guy.moves > 0 && guy.has_activity( ACT_FIRSTAID ) ) {
            guy.activity.do_turn( guy );
            if( guy.activity.moves_total - guy.activity.moves_left >= interrupt_time ) {
                // Assume the player confirms the option to cancel the activity when getting interrupted,
                // as in Character::react_to_felt_pain()
                guy.cancel_activity();
            }
        }
    } while( guy.has_activity( ACT_FIRSTAID ) );
}

TEST_CASE( "avatar does healing", "[activity][firstaid][avatar]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map();
    const bodypart_id right_arm( "arm_r" );
    npc &dunsel = spawn_npc( point_east, "test_talker" );
    set_time( calendar::turn_zero + 12_hours );
    dunsel.pos() = dummy.pos() + point_east;
    dummy.set_skill_level( skill_firstaid, 10 );
    int moves = 500;
    item_location bandages = dummy.i_add( item( itype_bandages ) );
    int start_bandage_count = bandages->count();
    REQUIRE( dummy.has_item( *bandages ) );
    GIVEN( "avatar has a damaged right arm" ) {
        dummy.apply_damage( nullptr, right_arm, 20 );
        WHEN( "avatar bandages self" ) {
            dummy.assign_activity( player_activity( firstaid_activity_actor( moves, bandages->tname(),
                                                    dummy.getID() ) ) );
            dummy.activity.targets.emplace_back( bandages );
            dummy.activity.str_values.emplace_back( right_arm.id().c_str() );
            process_activity( dummy );
            THEN( "Check that bandage was consumed and arm is bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 1 );
                CHECK( dummy.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
    GIVEN( "avatar has a damaged right arm" ) {
        dummy.apply_damage( nullptr, right_arm, 20 );
        WHEN( "avatar bandages self and is interrupted before finishing" ) {
            dummy.assign_activity( player_activity( firstaid_activity_actor( moves, bandages->tname(),
                                                    dummy.getID() ) ) );
            dummy.activity.targets.emplace_back( bandages );
            dummy.activity.str_values.emplace_back( right_arm.id().c_str() );
            process_activity_interrupt( dummy, moves / 2 );
            THEN( "Check that bandage was not consumed and arm is not bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 0 );
                CHECK( !dummy.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
    GIVEN( "npc has a damaged right arm" ) {
        dunsel.apply_damage( nullptr, right_arm, 20 );
        WHEN( "avatar bandages npc" ) {
            dummy.assign_activity( player_activity( firstaid_activity_actor( moves, bandages->tname(),
                                                    dunsel.getID() ) ) );
            dummy.activity.targets.emplace_back( bandages );
            dummy.activity.str_values.emplace_back( right_arm.id().c_str() );
            process_activity( dummy );
            THEN( "Check that bandage was consumed and npc's arm is bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 1 );
                CHECK( dunsel.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
}

TEST_CASE( "npc does healing", "[activity][firstaid][npc]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_map();
    const bodypart_id right_arm( "arm_r" );
    npc &dunsel = spawn_npc( point_east, "test_talker" );
    set_time( calendar::turn_zero + 12_hours );
    dunsel.pos() = dummy.pos() + point_east;
    dunsel.set_skill_level( skill_firstaid, 10 );
    int moves = 500;
    item_location bandages = dunsel.i_add( item( itype_bandages ) );
    int start_bandage_count = bandages->count();
    REQUIRE( dunsel.has_item( *bandages ) );
    GIVEN( "npc has a damaged right arm" ) {
        dunsel.apply_damage( nullptr, right_arm, 20 );
        WHEN( "npc bandages self" ) {
            // See npc::heal_self()
            bandages->type->invoke( dunsel, *bandages, dunsel.pos(), "heal" ).value_or( 0 );
            process_activity( dunsel );
            THEN( "Check that bandage was consumed and arm is bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 1 );
                CHECK( dunsel.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
    GIVEN( "npc has a damaged right arm" ) {
        dunsel.apply_damage( nullptr, right_arm, 20 );
        WHEN( "npc bandages self and is interrupted before finishing" ) {
            // See npc::heal_self()
            bandages->type->invoke( dunsel, *bandages, dunsel.pos(), "heal" ).value_or( 0 );
            process_activity_interrupt( dunsel, moves / 2 );
            THEN( "Check that bandage was not consumed and arm is not bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 0 );
                CHECK( !dunsel.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
    GIVEN( "avatar has a damaged right arm" ) {
        dummy.apply_damage( nullptr, right_arm, 20 );
        WHEN( "npc bandages avatar" ) {
            // See npc::heal_player
            bandages->type->invoke( dunsel, *bandages, dummy.pos(), "heal" ).value_or( 0 );
            process_activity( dunsel );
            THEN( "Check that bandage was consumed and avatar's arm is bandaged" ) {
                CHECK( start_bandage_count - bandages->count() == 1 );
                CHECK( dummy.has_effect( effect_bandaged, right_arm ) );
            }
        }
    }
}
