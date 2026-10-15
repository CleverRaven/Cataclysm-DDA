#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character_id.h"
#include "coordinates.h"
#include "dialogue_chatbin.h"
#include "game.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "map_helpers.h"
#include "memory_fast.h"
#include "mission.h"
#include "npc.h"
#include "overmapbuffer.h"
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
    u.reset_all_missions();
    mission::clear_all();
    clear_npcs();

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

mission *assign_test_find_item_mission( avatar &dude )
{
    mission *m = mission::reserve_new( mission_TEST_MISSION_FIND_ITEM, character_id() );
    if( m->get_assigned_player_id() == dude.getID() ) {
        m->set_assigned_player_id( character_id( -2 ) );
    }
    m->assign( dude );
    return m;
}

} // namespace

TEST_CASE( "automatic_find_item_mission_batch_processing", "[mission]" )
{
    avatar &dude = get_avatar();
    map &here = get_map();
    clear_character( dude, true );
    dude.reset_all_missions();
    mission::clear_all();
    clear_map();

    SECTION( "missing item does not complete the mission" ) {
        const mission *m = assign_test_find_item_mission( dude );
        mission::process_all();
        CHECK( m->in_progress() );
    }

    SECTION( "item in character inventory completes the mission" ) {
        const mission *m = assign_test_find_item_mission( dude );
        item rock( itype_test_rock );
        dude.wield( rock );
        mission::process_all();
        CHECK_FALSE( m->in_progress() );
    }

    SECTION( "nearby item completes the mission" ) {
        const mission *m = assign_test_find_item_mission( dude );
        here.add_item( dude.pos_bub() + tripoint_rel_ms::east, item( itype_test_rock ) );
        mission::process_all();
        CHECK_FALSE( m->in_progress() );
    }

    SECTION( "one item cannot complete two missions" ) {
        const mission *first = assign_test_find_item_mission( dude );
        const mission *second = assign_test_find_item_mission( dude );
        item rock( itype_test_rock );
        dude.wield( rock );
        mission::process_all();
        CHECK( first->in_progress() != second->in_progress() );
    }
}

TEST_CASE( "automatic_find_item_mission_batch_performance", "[.][performance][mission]" )
{
    avatar &dude = get_avatar();
    clear_character( dude, true );
    dude.reset_all_missions();
    mission::clear_all();
    clear_map();

    for( int i = 0; i < 100; ++i ) {
        assign_test_find_item_mission( dude );
    }

    BENCHMARK( "missing find-item mission" ) {
        mission::process_all();
    };

    SUCCEED();
}

TEST_CASE( "clearing_missions_leaves_no_npc_holding_one", "[mission]" )
{
    clear_avatar();
    // an overmap-owned NPC far outside the reality bubble, never loaded
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    guy->randomize();
    guy->spawn_at_omt( get_avatar().pos_abs_omt() + tripoint( 30, 0, 0 ) );
    overmap_buffer.insert_npc( guy );
    const character_id guy_id = guy->getID();
    on_out_of_scope forget_guy( [guy_id]() {
        overmap_buffer.remove_npc( guy_id );
    } );
    REQUIRE( overmap_buffer.find_npc( guy_id ) == guy );
    bool loaded = false;
    for( const npc &active : g->all_npcs() ) {
        loaded |= active.getID() == guy_id;
    }
    REQUIRE_FALSE( loaded );

    mission *offered = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION1, guy_id );
    mission *taken = mission::reserve_new( mission_TEST_MISSION_GOAL_CONDITION2, guy_id );
    guy->add_new_mission( offered );
    guy->chatbin.missions_assigned.push_back( taken );
    guy->chatbin.mission_selected = taken;
    REQUIRE( guy->chatbin.missions.size() == 1 );

    SECTION( "every mission cleared, then the NPC saved" ) {
        mission::clear_all();
        CHECK( guy->chatbin.missions.empty() );
        CHECK( guy->chatbin.missions_assigned.empty() );
        CHECK( guy->chatbin.mission_selected == nullptr );
        std::ostringstream saved;
        JsonOut jsout( saved );
        guy->serialize( jsout );
        const std::string json = saved.str();
        CAPTURE( json );
        CHECK( json.find( "\"missions\":[]" ) != std::string::npos );
        CHECK( json.find( "\"missions_assigned\":[]" ) != std::string::npos );
        CHECK( json.find( "\"mission_selected\"" ) == std::string::npos );
    }
}
