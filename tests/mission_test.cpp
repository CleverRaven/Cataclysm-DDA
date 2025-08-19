#include <functional>
#include <memory>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "coordinates.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "mission.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const itype_id itype_test_rock( "test_rock" );

static const mission_type_id mission_TEST_MISSION_GOAL_CONDITION1( "TEST_MISSION_GOAL_CONDITION1" );
static const mission_type_id mission_TEST_MISSION_GOAL_CONDITION2( "TEST_MISSION_GOAL_CONDITION2" );

static const morale_type morale_feeling_good( "morale_feeling_good" );

static const npc_template_id npc_template_test_talker( "test_talker" );

TEST_CASE( "mission_goal_condition_test", "[mission]" )
{
    avatar &u = get_avatar();
    clear_character( u, true );
    clear_npcs();
    u.reset_all_missions();
    mission::clear_all();

    GIVEN( "no_npc" ) {
        WHEN( "mission_origin_start" ) {
            mission *m = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION1, character_id() );
            if( m->get_assigned_player_id() == u.getID() ) {
                // Due to some bizarre optimization, test binaries compiled with
                // RELEASE=1 and LTO=1 result in u.getID() == character_id( -1 ).
                // So set the mission's initial player_id to something else.
                m->set_assigned_player_id( character_id( -2 ) );
            }
            m->assign( u );
            WHEN( "condition_not_met" ) {
                REQUIRE( !u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_not_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                }
            }
            WHEN( "condition_met" ) {
                item rock( itype_test_rock );
                u.wield( rock );
                REQUIRE( u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == false );
                    CHECK( u.get_completed_missions().front()->mission_id() == mission_TEST_MISSION_GOAL_CONDITION1 );
                    CHECK( u.has_morale( morale_feeling_good ) == 10 );
                }
            }
        }
        WHEN( "mission_no_origin" ) {
            mission *m = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION2, character_id() );
            if( m->get_assigned_player_id() == u.getID() ) {
                m->set_assigned_player_id( character_id( -2 ) );
            }
            m->assign( u );
            WHEN( "condition_not_met" ) {
                REQUIRE( !u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_not_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                }
            }
            WHEN( "condition_met" ) {
                item rock( itype_test_rock );
                u.wield( rock );
                REQUIRE( u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_not_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                }
            }
        }
    }

    GIVEN( "with_npc" ) {
        const character_id guy_id = get_map().place_npc( point_bub_ms( 25, 25 ), npc_template_test_talker );
        g->load_npcs();
        WHEN( "mission_origin_start" ) {
            mission *m = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION1, guy_id );
            if( m->get_assigned_player_id() == u.getID() ) {
                m->set_assigned_player_id( character_id( -2 ) );
            }
            m->assign( u );
            WHEN( "condition_not_met" ) {
                REQUIRE( !u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_not_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( !u.get_active_mission()->is_complete( guy_id ) );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                }
            }
            WHEN( "condition_met" ) {
                item rock( itype_test_rock );
                u.wield( rock );
                REQUIRE( u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_active_mission()->is_complete( guy_id ) );
                    u.get_active_mission()->wrap_up();
                    CHECK( u.get_completed_missions().empty() == false );
                    CHECK( u.get_completed_missions().front()->mission_id() == mission_TEST_MISSION_GOAL_CONDITION1 );
                    CHECK( u.has_morale( morale_feeling_good ) == 10 );
                }
            }
        }
        WHEN( "mission_no_origin" ) {
            mission *m = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION2, guy_id );
            if( m->get_assigned_player_id() == u.getID() ) {
                m->set_assigned_player_id( character_id( -2 ) );
            }
            m->assign( u );
            WHEN( "condition_not_met" ) {
                REQUIRE( !u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_not_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( !u.get_active_mission()->is_complete( guy_id ) );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                }
            }
            WHEN( "condition_met" ) {
                item rock( itype_test_rock );
                u.wield( rock );
                REQUIRE( u.has_item_with( []( const item & it ) {
                    return it.typeId() == itype_test_rock;
                } ) );
                THEN( "mission_complete" ) {
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_completed_missions().empty() == true );
                    u.get_active_mission()->process();
                    CHECK( u.get_completed_missions().empty() == true );
                    CHECK( u.has_morale( morale_feeling_good ) == 0 );
                    CHECK( u.get_active_mission()->is_complete( guy_id ) );
                    u.get_active_mission()->wrap_up();
                    CHECK( u.get_completed_missions().empty() == false );
                    CHECK( u.get_completed_missions().front()->mission_id() == mission_TEST_MISSION_GOAL_CONDITION2 );
                    CHECK( u.has_morale( morale_feeling_good ) == 10 );
                }
            }
        }
    }
}
