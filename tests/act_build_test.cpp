#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "build_reqs.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "clzones.h"
#include "construction.h"
#include "coordinates.h"
#include "enums.h"
#include "faction.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "memory_fast.h"
#include "npc.h"
#include "options_helpers.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "requirements.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather_type.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );

static const construction_str_id construction_constr_floor( "constr_floor" );
static const construction_str_id
construction_constr_ov_smreb_cage_thconc_floor( "constr_ov_smreb_cage_thconc_floor" );
static const construction_str_id construction_constr_pit_shallow( "constr_pit_shallow" );
static const construction_str_id construction_constr_thconc_floor( "constr_thconc_floor" );
static const construction_str_id construction_constr_wall_half( "constr_wall_half" );
static const construction_str_id construction_constr_wall_wood( "constr_wall_wood" );
static const construction_str_id construction_constr_window_empty( "constr_window_empty" );
static const construction_str_id
construction_constr_window_no_curtains( "constr_window_no_curtains" );

static const faction_id faction_free_merchants( "free_merchants" );

static const itype_id itype_bow_saw( "bow_saw" );
static const itype_id itype_e_tool( "e_tool" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_multitool( "test_multitool" );
static const itype_id itype_wearable_test_lamp( "wearable_test_lamp" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_metal_grate_window( "t_metal_grate_window" );
static const ter_str_id ter_t_railroad_rubble( "t_railroad_rubble" );
static const ter_str_id ter_t_thconc_floor( "t_thconc_floor" );
static const ter_str_id ter_t_wall_wood( "t_wall_wood" );
static const ter_str_id ter_t_window_boarded_noglass( "t_window_boarded_noglass" );
static const ter_str_id ter_t_window_empty( "t_window_empty" );
static const ter_str_id ter_t_window_no_curtains( "t_window_no_curtains" );

static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );

namespace
{
void run_activities( Character &u, int max_moves )
{
    u.assign_activity( multi_build_construction_activity_actor() );
    if( !process_activity_with_travel( u, max_moves ) ) {
        FAIL( "turn count exceeded, infinite loop possible" );
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
        return it.id == construction_str_id( name );
    } );
    return *build;
}

construction setup_testcase( Character &u, std::string const &constr,
                             tripoint_bub_ms const &build_loc, tripoint_bub_ms const &loot_loc )
{
    u.backlog.clear();
    u.cancel_activity();
    construction build = get_construction( constr );

    zone_manager &zmgr = zone_manager::get_manager();
    shared_ptr_fast<blueprint_options> options =
        make_shared_fast<blueprint_options>( build.pre_terrain.empty() ? "" : *
                build.pre_terrain.begin(), build.group, build.id );

    map &here = get_map();
    tripoint_abs_ms const loot_abs = here.get_abs( loot_loc );
    tripoint_abs_ms const build_abs = here.get_abs( build_loc );
    faction_id const fac = u.get_faction()->id;

    zmgr.add( constr + " loot zone", zone_type_LOOT_UNSORTED, fac, false, true, loot_abs,
              loot_abs );

    zmgr.add( constr + " construction zone", zone_type_CONSTRUCTION_BLUEPRINT, fac, false,
              true, build_abs, build_abs, options );

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
    clear_map_without_vision();
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    clear_avatar();
    map &here = get_map();
    g->reset_light_level();

    u.wear_item( item( itype_test_backpack ), false, false );
    u.wear_item( item( itype_wearable_test_lamp ), false, true );
    //TODO: this test assumes that tools are on-person, but it should also test without tools on-person
    u.i_add( item( itype_test_multitool ) );
    u.i_add( item( itype_hammer ) );
    u.i_add( item( itype_bow_saw ) );
    u.i_add( item( itype_e_tool ) );

    SECTION( "1-step construction activity with pre_terrain" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_door", tri_door, tripoint_bub_ms() );
        REQUIRE( u.sees( here,  tri_door ) );
        here.ter_set( tri_door, ter_id( build.pre_terrain.empty() ? "" : *
                                        build.pre_terrain.begin() ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity keeps one task to come back to" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_door", tri_door, tripoint_bub_ms() );
        here.ter_set( tri_door, ter_id( build.pre_terrain.empty() ? "" : *
                                        build.pre_terrain.begin() ) );
        u.assign_activity( multi_build_construction_activity_actor() );
        for( int turn = 0; turn < 100 && u.activity.id() != ACT_BUILD; ++turn ) {
            u.set_moves( u.get_speed() );
            u.activity.do_turn( u );
        }
        REQUIRE( u.activity.id() == ACT_BUILD );
        REQUIRE( u.backlog.size() == 1 );
        CHECK( u.backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION );
        CHECK( u.backlog.front().actor );
    }

    SECTION( "1-step construction activity with pre_terrain and starting far away" ) {
        u.setpos( here, tripoint_bub_ms{ MAX_VIEW_DISTANCE - 1, 0, 0} );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_door", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( build.pre_terrain.empty() ? "" : *
                                          build.pre_terrain.begin() ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with pre_flags" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( "test_t_window_no_curtains" ) );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with prereq with only pre_special" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_gravel( tripoint::south );
        construction const pre_build =
            setup_testcase( u, "test_constr_railroad_rubble", tri_gravel, tripoint_bub_ms() );
        zone_manager::get_manager().clear();
        construction const build =
            setup_testcase( u, "test_constr_remove_gravel", tri_gravel, tripoint_bub_ms() );
        // first check that we don't get stuck in a loop
        here.ter_set( tri_gravel, ter_t_dirt );
        run_activities( u, 1 );
        REQUIRE( here.partial_con_at( tri_gravel ) == nullptr );

        here.ter_set( tri_gravel, ter_t_railroad_rubble );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_gravel ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity - alternative build from same group" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_t_window_empty );
        REQUIRE( ( build.pre_terrain.empty() ? "" : * ( build.pre_terrain.begin() ) ) != "t_window_empty" );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_t_window_boarded_noglass );
    }

    SECTION( "1-step construction activity with existing partial" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        partial_con pc;
        pc.id = build.id;
        here.partial_con_set( tri_window, pc );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.post_terrain ) );
    }

    SECTION( "1-step construction activity with alternative partial" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_window_boarded", tri_window, tripoint_bub_ms() );
        partial_con pc;
        pc.id = get_construction( "test_constr_window_boarded_noglass_empty" ).id;
        here.partial_con_set( tri_window, pc );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_t_window_boarded_noglass );
    }

    SECTION( "1-step construction activity with mismatched partial" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window( tripoint::south );
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
        u.setpos( here, tripoint_bub_ms::zero );
        u.path_settings->bash_strength = {};
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_window = { 0, 5, 0 };
        for( tripoint_bub_ms const &it : here.points_in_radius( tri_window, 1 ) ) {
            here.ter_set( it, ter_t_metal_grate_window );
        }
        construction const build =
            setup_testcase( u, "test_constr_door", tri_window, tripoint_bub_ms() );
        here.ter_set( tri_window, ter_id( build.pre_terrain.empty() ? "" : *
                                          build.pre_terrain.begin() ) );
        REQUIRE( u.sees( here, tri_window ) );
        REQUIRE( route_adjacent( u, tri_window ).empty() );
        run_activities( u, build.time * 10 );
        REQUIRE( here.ter( tri_window ) == ter_id( build.pre_terrain.empty() ? "" : *
                 ( build.pre_terrain.begin() ) ) );
    }

    SECTION( "1-step construction activity fetching its tools" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        u.remove_items_with( []( const item & it ) {
            return it.typeId() == itype_test_multitool || it.typeId() == itype_hammer ||
                   it.typeId() == itype_bow_saw || it.typeId() == itype_e_tool;
        } );
        tripoint_bub_ms const tri_door( tripoint::south );
        tripoint_bub_ms const loot( 0, PICKUP_RANGE * 2 + 1, 0 );
        construction const build = setup_testcase( u, "test_constr_door", tri_door, loot );
        here.add_item_or_charges( loot, item( itype_hammer ) );
        here.add_item_or_charges( loot, item( itype_bow_saw ) );
        here.ter_set( tri_door, ter_id( build.pre_terrain.empty() ? "" : *
                                        build.pre_terrain.begin() ) );
        run_activities( u, build.time * 100 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
        int tools_moved = 0;
        for( const tripoint_bub_ms &p : here.points_in_radius( tri_door, PICKUP_RANGE ) ) {
            for( const item &it : here.i_at( p ) ) {
                if( it.typeId() == itype_hammer || it.typeId() == itype_bow_saw ) {
                    ++tools_moved;
                    CAPTURE( it.typeId().str(), p );
                    CHECK_FALSE( it.has_var( "activity_var" ) );
                }
            }
        }
        // both tools must be near door, or check above proves nothing
        CHECK( tools_moved == 2 );
    }

    SECTION( "1-step construction activity with a component partly carried" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        tripoint_bub_ms const loot( 0, PICKUP_RANGE * 2 + 1, 0 );
        construction const build = setup_testcase( u, "test_constr_door", tri_door, loot );
        // door needs 12 nails: 4 carried, 8 in loot zone
        map_stack loot_items = here.i_at( loot );
        for( auto it = loot_items.begin(); it != loot_items.end(); ) {
            it = it->typeId() == itype_nail ? loot_items.erase( it ) : std::next( it );
        }
        here.add_item_or_charges( loot, item( itype_nail, calendar::turn, 8 ) );
        REQUIRE( u.i_add( item( itype_nail, calendar::turn, 4 ) ) );
        here.ter_set( tri_door, ter_id( build.pre_terrain.empty() ? "" : *
                                        build.pre_terrain.begin() ) );
        run_activities( u, build.time * 100 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "multiple-step construction activity with fetch required" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_door_peep", tri_door, { 0, PICKUP_RANGE * 2 + 1, 0 } );
        run_activities( u, build.time * 100 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "multiple-step construction activity with prereq from a different group" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        construction const build =
            setup_testcase( u, "test_constr_palisade_gate", tri_door, tripoint_bub_ms( tripoint::south_east ) );
        run_activities( u, build.time * 200 );
        REQUIRE( here.ter( tri_door ) == ter_id( build.post_terrain ) );
    }

    SECTION( "multiple-step construction activity with partial of a recursive prerequisite" ) {
        u.setpos( here, tripoint_bub_ms::zero );
        here.build_map_cache( u.posz() );
        tripoint_bub_ms const tri_door( tripoint::south );
        partial_con pc;
        pc.id = get_construction( "test_constr_pit_shallow" ).id;
        here.partial_con_set( tri_door, pc );
        construction const build =
            setup_testcase( u, "test_constr_palisade_gate", tri_door, tripoint_bub_ms( tripoint::south_east ) );
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

TEST_CASE( "fetched_charges_split_into_cargo_all_lose_their_mark", "[activities][zones][fetch]" )
{
    clear_map_without_vision();
    clear_avatar();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms cart_pos = u.pos_bub() + tripoint::east;
    REQUIRE( here.add_vehicle( vehicle_prototype_test_shopping_cart, cart_pos, 0_degrees, 0,
                               veh_spawn_status::UNDAMAGED ) );
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo );
    while( cargo->vehicle().add_item( here, cargo->part(), item( itype_rock ) ) ) {}
    for( int i = 0; i < 2; ++i ) {
        cargo->vehicle().remove_item( cargo->part(), &*cargo->items().begin() );
    }
    const item nails( itype_nail, calendar::turn, 1000000 );
    const int fit = cargo->items().amount_can_fit( nails );
    REQUIRE( fit > 0 );
    REQUIRE( fit < nails.charges );
    u.assign_activity( multi_build_construction_activity_actor() );
    activity_handlers::drop_fetched_items( u, { nails }, &here, cart_pos );
    int in_cargo = 0;
    for( const item &it : cargo->items() ) {
        in_cargo += it.typeId() == itype_nail ? it.charges : 0;
    }
    REQUIRE( in_cargo == fit );
    const auto cargo_marked = [&cargo]() {
        bool marked = false;
        for( const item &it : cargo->items() ) {
            marked = marked || it.has_var( "activity_var" );
        }
        return marked;
    };
    const auto ground_marked = [&here, &u]() {
        bool marked = false;
        for( const tripoint_bub_ms &p : here.points_in_radius( u.pos_bub(), 2 ) ) {
            for( const item &it : here.i_at( p ) ) {
                marked = marked || it.has_var( "activity_var" );
            }
        }
        return marked;
    };
    // overflows into character's hands
    const auto held_marked = [&u]() {
        return character_has_item_with_var_val( u, "activity_var", u.name );
    };
    REQUIRE( cargo_marked() );
    REQUIRE( held_marked() );

    u.cancel_activity();
    CHECK_FALSE( cargo_marked() );
    CHECK_FALSE( held_marked() );
    CHECK_FALSE( ground_marked() );
}

TEST_CASE( "camp_blueprint_autocalc", "[camp][construction]" )
{


    build_reqs total_reqs;
    const auto add_build = [&total_reqs]( const construction_str_id & con_id ) {
        const construction &build = con_id.obj();
        total_reqs.time += build.time;
        total_reqs.raw_reqs[build.requirements] += 1;
    };

    WHEN( "ter_t_thconc_floor" ) {
        std::pair<std::map<ter_id, int>, std::map<furn_id, int>> changed_ids = { {{ter_t_thconc_floor.id(), 1}}, {} };
        build_reqs auto_build_reqs = get_build_reqs_for_furn_ter_ids( changed_ids );
        total_reqs.time = 0;
        total_reqs.raw_reqs.clear();

        add_build( construction_constr_thconc_floor );
        add_build( construction_constr_ov_smreb_cage_thconc_floor );
        add_build( construction_constr_pit_shallow );

        CHECK( total_reqs.time == auto_build_reqs.time );
        for( const auto &req : total_reqs.raw_reqs ) {
            CHECK( req.second == auto_build_reqs.raw_reqs[req.first] );
        }
    }
    WHEN( "ter_t_floor" ) {
        std::pair<std::map<ter_id, int>, std::map<furn_id, int>> changed_ids = { { {ter_t_floor.id(), 1}}, {} };
        build_reqs auto_build_reqs = get_build_reqs_for_furn_ter_ids( changed_ids );
        total_reqs.time = 0;
        total_reqs.raw_reqs.clear();

        add_build( construction_constr_floor );

        CHECK( total_reqs.time == auto_build_reqs.time );
        for( const auto &req : total_reqs.raw_reqs ) {
            CHECK( req.second == auto_build_reqs.raw_reqs[req.first] );
        }
    }
    WHEN( "ter_t_window_no_curtains" ) {
        std::pair<std::map<ter_id, int>, std::map<furn_id, int>> changed_ids = { {{ter_t_window_no_curtains.id(), 1}}, {} };
        build_reqs auto_build_reqs = get_build_reqs_for_furn_ter_ids( changed_ids );
        total_reqs.time = 0;
        total_reqs.raw_reqs.clear();

        add_build( construction_constr_window_no_curtains );
        add_build( construction_constr_window_empty );

        CHECK( total_reqs.time == auto_build_reqs.time );
        for( const auto &req : total_reqs.raw_reqs ) {
            CHECK( req.second == auto_build_reqs.raw_reqs[req.first] );
        }
    }
    WHEN( "ter_t_wall_wood" ) {
        std::pair<std::map<ter_id, int>, std::map<furn_id, int>> changed_ids = { { {ter_t_wall_wood.id(), 1}}, {} };
        build_reqs auto_build_reqs = get_build_reqs_for_furn_ter_ids( changed_ids );
        total_reqs.time = 0;
        total_reqs.raw_reqs.clear();

        add_build( construction_constr_wall_wood );
        add_build( construction_constr_wall_half );
        CHECK( total_reqs.time == auto_build_reqs.time );
        for( const auto &req : total_reqs.raw_reqs ) {
            CHECK( req.second == auto_build_reqs.raw_reqs[req.first] );
        }
    }

}
