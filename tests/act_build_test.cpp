#include <algorithm>
#include <vector>

#include "activity_handlers.h"
#include "avatar_action.h"
#include "cata_catch.h"
#include "activity_type.h"
#include "clzones.h"
#include "construction.h"
#include "game.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "pathfinding.h"
#include "player_helpers.h"

static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const faction_id faction_free_merchants( "free_merchants" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );

namespace
{
void run_activities( Character &u, int max_moves )
{
    u.assign_activity( ACT_MULTIPLE_CONSTRUCTION );
    int turns = 0;
    while( ( !u.activity.is_null() || u.is_auto_moving() ) && turns < max_moves ) {
        u.set_moves( u.get_speed() );
        if( u.is_auto_moving() ) {
            u.setpos( get_map().getlocal( *u.destination_point ) );
            get_map().build_map_cache( u.pos().z );
            u.start_destination_activity();
        }
        u.activity.do_turn( u );
        // npc plz do your thing
        if( u.is_npc() && u.activity.is_null() && !u.is_auto_moving() && !u.backlog.empty() &&
            u.backlog.back().id() == ACT_MULTIPLE_CONSTRUCTION ) {
            activity_handlers::resume_for_multi_activities( u );
        }
        turns++;
    }
}

void give_skills( Character &u, construction const &build )
{
    for( auto const *cons : constructions_by_group( build.group ) ) {
        for( auto const &skill : cons->required_skills ) {
            u.set_skill_level( skill.first,
                               std::max( static_cast<int>( static_cast<int>( u.get_skill_level( skill.first ) ) ),
                                         skill.second ) );
        }
    }
    REQUIRE( u.meets_skill_requirements( build ) );
}

construction get_construction( std::string const &name )
{
    std::vector<construction> const &cnstr = get_constructions();
    auto const build = std::find_if( cnstr.begin(), cnstr.end(), [&name]( const construction & it ) {
        return it.str_id == construction_str_id( name );
    } );
    return *build;
}

construction setup_testcase( Character &u, std::string const &constr,
                             tripoint_bub_ms const &build_loc, tripoint_bub_ms const &loot_loc )
{
    construction build = get_construction( constr );

    zone_manager &zmgr = zone_manager::get_manager();
    shared_ptr_fast<blueprint_options> options =
        make_shared_fast<blueprint_options>( build.pre_terrain, build.group, build.id );

    map &here = get_map();
    tripoint_abs_ms const loot_abs = here.getglobal( loot_loc );
    tripoint_abs_ms const build_abs = here.getglobal( build_loc );
    faction_id const fac = u.get_faction()->id;

    zmgr.add( constr + " loot zone", zone_type_LOOT_UNSORTED, fac, false, true, loot_abs.raw(),
              loot_abs.raw() );

    zmgr.add( constr + " construction zone", zone_type_CONSTRUCTION_BLUEPRINT, fac, false,
              true, build_abs.raw(), build_abs.raw(), options );

    for( auto const *cons : constructions_by_group( build.group ) ) {
        for( auto const &comp : cons->requirements->get_components() ) {
            for( int i = 0; i < comp.front().count; i++ ) {
                here.add_item_or_charges( loot_loc, item( comp.front().type, calendar::turn, 1 ),
                                          false );
            }
        }
    }

    give_skills( u, build );

    return build;
}

void run_test_case( Character &u )
{
    calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
    clear_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    clear_avatar();
    map &here = get_map();
    g->reset_light_level();

    u.wear_item( item( "test_backpack" ), false, false );
    u.wear_item( item( "wearable_atomic_light" ), false, true );
    u.i_add( item( "test_multitool" ) );
    u.i_add( item( "hammer" ) );
    u.i_add( item( "bow_saw" ) );
    u.i_add( item( "e_tool" ) );

    SECTION( "1-step construction activity with pre_terrain" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_door( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_door", tri_door, tripoint_bub_ms() );
        REQUIRE( u.sees( tri_door ) );
        here.ter_set( tri_door, ter_id( build.pre_terrain ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with pre_terrain and starting far away" ) {
        u.setpos( { ACTIVITY_SEARCH_DISTANCE - 1, 0, 0} );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_door", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( build.pre_terrain ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with pre_flags" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( "test_t_window_no_curtains" ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with prereq with only pre_special" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_gravel( tripoint_south );
        construction const pre_build =
            setup_testcase( u, "test_constr_railroad_rubble", tri_gravel, tripoint_bub_ms() );
        zone_manager::get_manager().clear();
        construction const build =
            setup_testcase( u, "test_constr_remove_gravel", tri_gravel, tripoint_bub_ms() );
        // first check that we don't get stuck in a loop
        here.ter_set( tri_gravel, t_dirt );
        run_activities( u, 1 );
        REQUIRE( here.partial_con_at( tri_gravel ) == nullptr );

        here.ter_set( tri_gravel, t_railroad_rubble );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_gravel ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity - alternative build from same group" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, t_window_empty );
        REQUIRE( build.pre_terrain != "t_window_empty" );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == t_window_boarded_noglass );
    }

    SECTION( "1-step construction activity with existing partial" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        partial_con pc;
        pc.id = build.id;
        here.partial_con_set( tri_window, pc );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with alternative partial" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        partial_con pc;
        pc.id = get_construction( "test_constr_window_boarded_noglass_empty" ).id;
        here.partial_con_set( tri_window, pc );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == t_window_boarded_noglass );
    }

    SECTION( "1-step construction activity with mismatched partial" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        ter_id const ter_pre = here.ter( tri_window );
        partial_con pc;
        pc.id = get_construction( "test_constr_door" ).id;
        here.partial_con_set( tri_window, pc );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_pre );
    }

    SECTION( "visible but unreachable construction" ) {
        u.setpos( tripoint_zero );
        u.path_settings->bash_strength = 0;
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_window = { 0, 5, 0 };
        for( tripoint_bub_ms const &it : here.points_in_radius( tri_window, 1 ) ) {
            here.ter_set( it, t_metal_grate_window );
        }
        construction const build =
            setup_testcase( u, "test_constr_door", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( build.pre_terrain ) );
        REQUIRE( u.sees( tri_window ) );
        REQUIRE( route_adjacent( u, tri_window ).empty() );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.pre_terrain ) );
    }

    SECTION( "multiple-step construction activity with fetch required" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_door( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_door_peep", tri_door, { 0, PICKUP_RANGE * 2 + 1, 0 } );
        run_activities( u, build.time * 100 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "multiple-step construction activity with prereq from a different group" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_door( tripoint_south );
        construction const build =
            setup_testcase( u, "test_constr_palisade_gate", tri_door, tripoint_bub_ms( tripoint_south_east ) );
        run_activities( u, build.time * 200 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "multiple-step construction activity with partial of a recursive prerequisite" ) {
        u.setpos( tripoint_zero );
        here.build_map_cache( u.pos().z );
        tripoint_bub_ms const tri_door( tripoint_south );
        partial_con pc;
        pc.id = get_construction( "test_constr_pit_shallow" ).id;
        here.partial_con_set( tri_door, pc );
        construction const build =
            setup_testcase( u, "test_constr_palisade_gate", tri_door, tripoint_bub_ms( tripoint_south_east ) );
        run_activities( u, build.time * 200 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }
}

} // namespace

TEST_CASE( "act_multiple_construction", "[zones][activities][construction]" )
{
    run_test_case( get_avatar() );
}

TEST_CASE( "npc_act_multiple_construction", "[npc][zones][activities][construction]" )
{
    standard_npc u( "Mr. Builderman" );
    u.set_body();
    u.set_fac( faction_free_merchants );
    run_test_case( u );
}
