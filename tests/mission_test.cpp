#include <chrono>
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

static const mission_type_id mission_TEST_MISSION_FIND_ITEM( "TEST_MISSION_FIND_ITEM" );
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

namespace
{

mission *assign_test_find_item_mission( avatar &u )
{
    mission *m = mission::reserve_new( mission_TEST_MISSION_FIND_ITEM, character_id() );
    if( m->get_assigned_player_id() == u.getID() ) {
        m->set_assigned_player_id( character_id( -2 ) );
    }
    m->assign( u );
    return m;
}

void clear_test_missions( avatar &u )
{
    u.reset_all_missions();
    mission::clear_all();
}

} // namespace

TEST_CASE( "automatic_find_item_mission_batch_processing", "[mission]" )
{
    avatar &u = get_avatar();
    map &here = get_map();
    clear_character( u, true );
    clear_map();
    clear_test_missions( u );

    SECTION( "missing item does not complete the mission" ) {
        mission *m = assign_test_find_item_mission( u );
        mission::process_all();
        CHECK( m->in_progress() );
    }

    SECTION( "item in character inventory completes the mission" ) {
        mission *m = assign_test_find_item_mission( u );
        item rock( itype_test_rock );
        u.wield( rock );
        mission::process_all();
        CHECK_FALSE( m->in_progress() );
    }

    SECTION( "nearby item completes the mission" ) {
        mission *m = assign_test_find_item_mission( u );
        here.add_item( u.pos_bub() + tripoint_rel_ms::east, item( itype_test_rock ) );
        mission::process_all();
        CHECK_FALSE( m->in_progress() );
    }

    SECTION( "one item cannot complete two missions" ) {
        mission *first = assign_test_find_item_mission( u );
        mission *second = assign_test_find_item_mission( u );
        item rock( itype_test_rock );
        u.wield( rock );
        mission::process_all();
        CHECK( first->in_progress() != second->in_progress() );
    }
}

TEST_CASE( "automatic_find_item_mission_batch_performance", "[.][performance][mission]" )
{
    avatar &u = get_avatar();
    clear_character( u, true );
    clear_map();
    clear_test_missions( u );

    constexpr int mission_count = 100;
    for( int i = 0; i < mission_count; ++i ) {
        assign_test_find_item_mission( u );
    }

    const auto start = std::chrono::steady_clock::now();
    mission::process_all();
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>( end - start );
    INFO( "100 missing find-item missions: " << elapsed.count() << " us" );
}