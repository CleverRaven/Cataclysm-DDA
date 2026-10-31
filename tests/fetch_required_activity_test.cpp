#include <cstdint>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "activity_actor.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_item_handling.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "clone_ptr.h"
#include "clzones.h"
#include "construction.h"
#include "craft_reservation.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "faction.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_uid.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "mtype.h"
#include "npc.h"
#include "npc_turn_helpers.h"
#include "npctalk.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "requirements.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "visitable.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );

static const construction_str_id construction_test_constr_door( "test_constr_door" );

static const faction_id faction_your_followers( "your_followers" );

static const flag_id json_flag_FILTHY( "FILTHY" );

static const furn_str_id furn_f_butcher_rack( "f_butcher_rack" );

static const item_group_id Item_spawn_data_pelmeni_can_medium_2( "pelmeni_can_medium_2" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_backpack_small( "backpack_small" );
static const itype_id itype_bow_saw( "bow_saw" );
static const itype_id itype_box_small( "box_small" );
static const itype_id itype_briefcase( "briefcase" );
static const itype_id itype_e_tool( "e_tool" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_hoe( "hoe" );
static const itype_id itype_knife_hunting( "knife_hunting" );
static const itype_id itype_mc_nodachi( "mc_nodachi" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_pelmeni( "pelmeni" );
static const itype_id itype_pockknife( "pockknife" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_saw_bronze( "saw_bronze" );
static const itype_id itype_test_hacksaw( "test_hacksaw" );
static const itype_id
itype_test_watertight_open_sealed_container_1L( "test_watertight_open_sealed_container_1L" );
static const itype_id itype_wearable_test_lamp( "wearable_test_lamp" );

static const mtype_id mon_chicken( "mon_chicken" );
static const mtype_id mon_cow( "mon_cow" );

static const quality_id qual_BUTCHER( "BUTCHER" );

static const skill_id skill_fabrication( "fabrication" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_frame( "t_door_frame" );
static const ter_str_id ter_t_wall( "t_wall" );

static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );

namespace
{
const tripoint_bub_ms corpse_pos( 60, 60, 0 );
// loot tiles beyond PICKUP_RANGE of the corpse
const tripoint_bub_ms loot_near( 60, 68, 0 );
const tripoint_bub_ms loot_mid( 60, 72, 0 );
const tripoint_bub_ms loot_far( 60, 76, 0 );

void butchery_setup( const mtype_id &kind )
{
    clear_map_without_vision();
    clear_avatar();
    zone_manager::get_manager().clear();
    set_time_to_day();
    map &here = get_map();
    get_avatar().setpos( here, corpse_pos + tripoint( -3, 0, 0 ) );
    here.furn_set( corpse_pos + tripoint::east, furn_f_butcher_rack );
    here.add_item( corpse_pos, item::make_corpse( kind ) );
    const faction_id fac = faction_your_followers;
    zone_manager &zmgr = zone_manager::get_manager();
    zmgr.add( "corpses", zone_type_LOOT_CORPSE, fac, false, true, here.get_abs( corpse_pos ),
              here.get_abs( corpse_pos ) );
    zmgr.add( "loot", zone_type_LOOT_UNSORTED, fac, false, true, here.get_abs( loot_near ),
              here.get_abs( loot_far ) );
    here.build_map_cache( 0 );
}

npc &spawn_follower( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_mission( NPC_MISSION_NULL );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    return guy;
}

int corpses_at( const tripoint_bub_ms &p )
{
    int n = 0;
    for( const item &it : get_map().i_at( p ) ) {
        n += it.is_corpse() ? 1 : 0;
    }
    return n;
}

const tripoint_bub_ms plot_pos( 60, 60, 0 );
// 3x3 loot zone with character next to its north edge, so dropped tool lands in zone
const tripoint_bub_ms loot_centre( 60, 64, 0 );

int carried( const Character &u, const itype_id &type )
{
    int n = 0;
    u.visit_items( [&n, &type]( const item_location & loc ) {
        n += loc->typeId() == type ? 1 : 0;
        return VisitResponse::NEXT;
    } );
    return n;
}

int carried_dig_tools( const Character &u )
{
    return carried( u, itype_hoe ) + carried( u, itype_e_tool );
}

int count_on_map( const tripoint_bub_ms &around, const itype_id &type )
{
    map &here = get_map();
    int n = 0;
    for( const tripoint_bub_ms &p : here.points_in_radius( around, 20 ) ) {
        for( const item &it : here.i_at( p ) ) {
            n += it.typeId() == type ? 1 : 0;
        }
    }
    return n;
}

bool any_item_marked( const Character &u )
{
    map &here = get_map();
    bool found = false;
    for( const tripoint_bub_ms &p : here.points_in_radius( u.pos_bub(), 20 ) ) {
        for( const item &it : here.i_at( p ) ) {
            it.visit_items( [&found]( const item * e, const item * ) {
                found = found || e->has_var( "activity_var" );
                return VisitResponse::NEXT;
            } );
        }
    }
    u.visit_items( [&found]( const item_location & loc ) {
        found = found || loc->has_var( "activity_var" );
        return VisitResponse::NEXT;
    } );
    return found;
}

void farm_setup( avatar &u )
{
    clear_map_without_vision();
    clear_avatar();
    zone_manager::get_manager().clear();
    set_time_to_day();
    map &here = get_map();
    u.setpos( here, plot_pos + tripoint( 0, 2, 0 ) );
    // no pocket long enough for hoe
    u.wear_item( item( itype_backpack_small ), false, false );
    u.wear_item( item( itype_wearable_test_lamp ), false, true );
    here.ter_set( plot_pos, ter_t_dirt );
    REQUIRE( here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, plot_pos ) );
    const faction_id fac = u.get_faction()->id;
    zone_manager &zmgr = zone_manager::get_manager();
    zmgr.add( "plot", zone_type_FARM_PLOT, fac, false, true, here.get_abs( plot_pos ),
              here.get_abs( plot_pos ) );
    zmgr.add( "loot", zone_type_LOOT_UNSORTED, fac, false, true,
              here.get_abs( loot_centre + tripoint::north_west ),
              here.get_abs( loot_centre + tripoint::south_east ) );
    here.build_map_cache( 0 );
}

const tripoint_bub_ms work_pos( 60, 60, 0 );
const tripoint_bub_ms far_loot( 60, 70, 0 );

// far_loot is beyond PICKUP_RANGE of work spot, so nothing there is already at hand
avatar &construction_fetch_setup()
{
    clear_map_without_vision();
    clear_avatar();
    zone_manager::get_manager().clear();
    set_time_to_day();
    map &here = get_map();
    avatar &u = get_avatar();
    u.setpos( here, work_pos + tripoint::west );
    u.wear_item( item( itype_backpack ), false, false );
    zone_manager::get_manager().add( "loot", zone_type_LOOT_UNSORTED, u.get_faction()->id, false,
                                     true, here.get_abs( far_loot ), here.get_abs( far_loot ) );
    u.backlog.emplace_front( multi_build_construction_activity_actor() );
    here.build_map_cache( 0 );
    return u;
}

std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> construction_fetch_plan( Character &u,
        const requirement_id &needed )
{
    map &here = get_map();
    fetch_required_activity_actor fetch( needed, do_activity_reason::NO_COMPONENTS,
                                         here.get_abs( work_pos + tripoint::north ), here.get_abs( work_pos ) );
    return fetch.requirements_map( u );
}

player_activity current_fetch( const npc &guy )
{
    return guy.activity.id() == ACT_FETCH_REQUIRED ? guy.activity : guy.get_destination_activity();
}
} // namespace

TEST_CASE( "zone_task_skips_a_work_spot_it_cannot_reach", "[activities][zones]" )
{
    butchery_setup( mon_chicken );
    map &here = get_map();
    // closer than the open corpse, so the scan meets it first
    const tripoint_bub_ms walled = corpse_pos + tripoint( -6, -3, 0 );
    for( const tripoint_bub_ms &p : here.points_in_radius( walled, 1 ) ) {
        if( p != walled ) {
            here.ter_set( p, ter_t_wall );
        }
    }
    here.add_item( walled, item::make_corpse( mon_chicken ) );
    zone_manager::get_manager().add( "walled corpse", zone_type_LOOT_CORPSE,
                                     faction_your_followers, false, true, here.get_abs( walled ),
                                     here.get_abs( walled ) );
    here.build_map_cache( 0 );
    npc &guy = spawn_follower( corpse_pos + tripoint( -6, 0, 0 ) );
    REQUIRE( rl_dist( guy.pos_bub(), walled ) < rl_dist( guy.pos_bub(), corpse_pos ) );
    REQUIRE( route_adjacent( guy, walled ).empty() );
    REQUIRE( guy.wield( *guy.i_add( item( itype_knife_hunting ) ) ) );

    talk_function::do_butcher( guy );
    const std::string debugmsgs = capture_debugmsg_during( [&guy]() {
        run_npc_turns( guy, 20000, []( const npc & ) {
            return corpses_at( corpse_pos ) == 0;
        } );
    } );
    CHECK( debugmsgs.empty() );
    CHECK( corpses_at( corpse_pos ) == 0 );
    CHECK( corpses_at( walled ) == 1 );
}

TEST_CASE( "fetch_that_cannot_take_its_item_ends_the_task", "[activities][zones][fetch]" )
{
    butchery_setup( mon_cow );
    map &here = get_map();
    REQUIRE( mon_cow->size > creature_size::medium );
    npc &guy = spawn_follower( corpse_pos + tripoint::west );
    REQUIRE( guy.wear_item( item( itype_briefcase ), false, false ) );
    const activity_reason_info first =
        multi_butchery_activity_actor().multi_activity_can_do( guy, corpse_pos );
    REQUIRE( first.reason == do_activity_reason::NEEDS_BIG_BUTCHERING );

    const item &bulky = here.add_item( loot_near, item( itype_mc_nodachi ) );
    here.add_item( loot_far, item( itype_saw_bronze ) );
    here.add_item( loot_far, item( itype_test_hacksaw ) );
    REQUIRE( bulky.volume() <= guy.free_space() );
    REQUIRE( bulky.weight() <= guy.weight_capacity() - guy.weight_carried() );
    REQUIRE_FALSE( guy.can_stash( bulky ) );
    REQUIRE_FALSE( guy.can_wield( bulky ).success() );

    talk_function::do_butcher( guy );
    run_npc_turns( guy, 100, []( const npc & n ) {
        return current_fetch( n ).id() == ACT_FETCH_REQUIRED;
    } );
    player_activity fetch = current_fetch( guy );
    REQUIRE( fetch.id() == ACT_FETCH_REQUIRED );
    fetch_required_activity_actor *actor =
        dynamic_cast<fetch_required_activity_actor *>( fetch.actor.get() );
    REQUIRE( actor != nullptr );
    const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> plan =
                actor->requirements_map( guy );
    REQUIRE( plan.size() == 3 );
    bool bulky_planned = false;
    for( const std::tuple<tripoint_bub_ms, itype_id, int> &step : plan ) {
        if( item( std::get<1>( step ) ).has_quality( qual_BUTCHER ) ) {
            bulky_planned = std::get<0>( step ) == loot_near;
        }
    }
    REQUIRE( bulky_planned );

    const std::string debugmsgs = capture_debugmsg_during( [&guy]() {
        run_npc_turns( guy, 300 );
    } );
    CHECK( debugmsgs.empty() );
    CHECK_FALSE( guy.activity );
    CHECK_FALSE( guy.has_destination() );
    int saws_left = 0;
    for( const item &it : here.i_at( loot_far ) ) {
        saws_left += it.typeId() == itype_saw_bronze || it.typeId() == itype_test_hacksaw ? 1 : 0;
    }
    CHECK( saws_left == 2 );
    CHECK( count_on_map( corpse_pos, itype_mc_nodachi ) == 1 );
    CHECK_FALSE( any_item_marked( guy ) );
}

TEST_CASE( "farm_fetch_takes_one_digging_tool", "[activities][zones][fetch]" )
{
    avatar &u = get_avatar();
    map &here = get_map();
    farm_setup( u );
    REQUIRE_FALSE( u.can_stash( item( itype_hoe ) ) );

    GIVEN( "entrenching tool in loot zone, small enough to stow" ) {
        here.add_item_or_charges( loot_centre, item( itype_e_tool ) );
        WHEN( "fetched tool wielded, then task cancelled" ) {
            u.assign_activity( multi_farm_activity_actor() );
            for( int turn = 0; turn < 2000 && carried( u, itype_e_tool ) == 0; ++turn ) {
                u.set_moves( u.get_speed() );
                if( u.is_auto_moving() ) {
                    u.setpos( here, here.get_bub( *u.destination_point ) );
                    here.build_map_cache( u.posz() );
                    u.start_destination_activity();
                }
                u.activity.do_turn( u );
            }
            item_location tool;
            u.visit_items( [&tool]( const item_location & loc ) {
                if( loc->typeId() == itype_e_tool ) {
                    tool = loc;
                    return VisitResponse::ABORT;
                }
                return VisitResponse::NEXT;
            } );
            REQUIRE( tool );
            REQUIRE( tool->has_var( "activity_var" ) );
            REQUIRE_FALSE( u.is_wielding( *tool ) );
            REQUIRE( u.wield( tool ) );
            u.cancel_activity();
            THEN( "no item stays marked" ) {
                CHECK_FALSE( any_item_marked( u ) );
            }
        }
    }
    GIVEN( "hoe and entrenching tool in the loot zone" ) {
        here.add_item_or_charges( loot_centre, item( itype_hoe ) );
        here.add_item_or_charges( loot_centre, item( itype_e_tool ) );
        WHEN( "farm plot worked" ) {
            u.assign_activity( multi_farm_activity_actor() );
            REQUIRE( process_activity_with_travel( u, 20000 ) );
            THEN( "one tool taken, plot tilled, nothing stays in use" ) {
                CHECK( carried_dig_tools( u ) == 1 );
                CHECK( here.ter( plot_pos ) == ter_t_dirtmound );
                CHECK_FALSE( any_item_marked( u ) );
            }
        }
    }
    GIVEN( "two hoes, too long for any pocket, in loot zone" ) {
        here.add_item_or_charges( loot_centre, item( itype_hoe ) );
        here.add_item_or_charges( loot_centre, item( itype_hoe ) );
        WHEN( "farm plot worked" ) {
            u.assign_activity( multi_farm_activity_actor() );
            THEN( "fetch ends with one hoe in hand and plot tilled" ) {
                REQUIRE( process_activity_with_travel( u, 20000 ) );
                REQUIRE( u.get_wielded_item() );
                CHECK( u.get_wielded_item()->typeId() == itype_hoe );
                CHECK( carried_dig_tools( u ) == 1 );
                CHECK( count_on_map( plot_pos, itype_hoe ) == 1 );
                CHECK( here.ter( plot_pos ) == ter_t_dirtmound );
                CHECK_FALSE( any_item_marked( u ) );
            }
        }
    }
    GIVEN( "hoe in loot zone, rock in character's hand" ) {
        here.add_item_or_charges( loot_centre, item( itype_hoe ) );
        REQUIRE( u.wield( *u.i_add( item( itype_rock ) ) ) );
        WHEN( "farm plot worked" ) {
            // unwielding asks first; under test the answer is no, as when a player presses Escape
            u.assign_activity( multi_farm_activity_actor() );
            THEN( "task ends, rock in hand, hoe on ground and free" ) {
                REQUIRE( process_activity_with_travel( u, 20000 ) );
                CHECK_FALSE( u.activity );
                CHECK( u.backlog.empty() );
                REQUIRE( u.get_wielded_item() );
                CHECK( u.get_wielded_item()->typeId() == itype_rock );
                CHECK( carried_dig_tools( u ) == 0 );
                CHECK( count_on_map( plot_pos, itype_hoe ) == 1 );
                CHECK( here.ter( plot_pos ) == ter_t_dirt );
                CHECK_FALSE( any_item_marked( u ) );
            }
        }
    }
}

TEST_CASE( "big_butchery_fetch_takes_one_of_each_tool", "[activities][zones][fetch]" )
{
    butchery_setup( mon_cow );
    map &here = get_map();
    avatar &u = get_avatar();
    u.wear_item( item( itype_backpack ), false, false );
    REQUIRE( multi_butchery_activity_actor().multi_activity_can_do( u, corpse_pos ).reason ==
             do_activity_reason::NEEDS_BIG_BUTCHERING );
    for( int i = 0; i < 3; ++i ) {
        here.add_item( loot_near, item( itype_knife_hunting ) );
    }
    here.add_item( loot_mid, item( itype_test_hacksaw ) );
    here.add_item( loot_mid, item( itype_saw_bronze ) );

    u.assign_activity( multi_butchery_activity_actor() );
    REQUIRE( process_activity_with_travel( u, 100000 ) );
    CHECK( corpses_at( corpse_pos ) == 0 );
    CHECK( carried( u, itype_knife_hunting ) == 1 );
    // one saw per saw quality line
    CHECK( carried( u, itype_test_hacksaw ) == 1 );
    CHECK( carried( u, itype_saw_bronze ) == 1 );
    int knives_left = 0;
    for( const item &it : here.i_at( loot_near ) ) {
        knives_left += it.typeId() == itype_knife_hunting ? 1 : 0;
    }
    CHECK( knives_left == 2 );
    CHECK_FALSE( any_item_marked( u ) );
}

TEST_CASE( "butchery_fetch_unseals_container_it_takes_a_knife_from",
           "[activities][zones][fetch]" )
{
    butchery_setup( mon_chicken );
    map &here = get_map();
    avatar &u = get_avatar();
    u.wear_item( item( itype_backpack ), false, false );
    item jug( itype_test_watertight_open_sealed_container_1L );
    REQUIRE( jug.put_in( item( itype_pockknife ), pocket_type::CONTAINER ).success() );
    // an emptied pocket unseals itself, so something stays behind
    REQUIRE( jug.put_in( item( itype_nail, calendar::turn, 10 ), pocket_type::CONTAINER ).success() );
    REQUIRE( jug.seal() );
    const item &stored = here.add_item( loot_near, jug );
    REQUIRE( stored.all_pockets_sealed() );

    u.assign_activity( multi_butchery_activity_actor() );
    REQUIRE( process_activity_with_travel( u, 100000 ) );
    CHECK( corpses_at( corpse_pos ) == 0 );
    CHECK( carried( u, itype_pockknife ) == 1 );
    REQUIRE( here.i_at( loot_near ).size() == 1 );
    const item &left = here.i_at( loot_near ).only_item();
    CHECK( left.typeId() == itype_test_watertight_open_sealed_container_1L );
    CHECK( left.only_item().typeId() == itype_nail );
    CHECK_FALSE( left.any_pockets_sealed() );
    CHECK_FALSE( any_item_marked( u ) );
}

TEST_CASE( "fetch_brings_a_second_item_for_a_constructed_tool_and_component_pair",
           "[activities][zones][fetch]" )
{
    avatar &u = construction_fetch_setup();
    map &here = get_map();
    REQUIRE( u.i_add( item( itype_hammer ) ) );
    here.add_item_or_charges( far_loot, item( itype_hammer ) );
    // no game requirement asks for one item as both tool and component; requirements_map
    // handles any requirement alike
    const requirement_id hammer_twice = multi_activity_actor::synthesize_requirements(
    { { item_comp( itype_hammer, 1 ) } }, {}, { { tool_comp( itype_hammer, -1 ) } } );

    const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> plan =
                construction_fetch_plan( u, hammer_twice );
    // carried hammer is the tool, component hammer is from loot tile
    REQUIRE( plan.size() == 1 );
    CHECK( std::get<0>( plan.front() ) == far_loot );
    CHECK( std::get<1>( plan.front() ) == itype_hammer );
}

TEST_CASE( "fetch_counts_filthy_carried_items_for_qualities_not_components",
           "[activities][zones][fetch]" )
{
    avatar &u = construction_fetch_setup();
    map &here = get_map();
    here.add_item_or_charges( far_loot, item( itype_hammer ) );
    here.add_item_or_charges( far_loot, item( itype_bow_saw ) );
    const requirement_data &door = construction_test_constr_door->requirements.obj();
    for( const std::vector<item_comp> &line : door.get_components() ) {
        const item one( line.front().type );
        for( int i = 0; i < ( one.count_by_charges() ? 1 : line.front().count ); ++i ) {
            here.add_item_or_charges( far_loot, one.count_by_charges() ?
                                      item( one.typeId(), calendar::turn, line.front().count ) : one );
        }
    }
    const auto carry_filthy = [&u]( const itype_id & type ) {
        item filthy( type );
        filthy.set_flag( json_flag_FILTHY );
        REQUIRE( u.i_add( filthy ) );
    };
    const auto planned = [&u]( const itype_id & type ) {
        const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> plan =
                    construction_fetch_plan( u, construction_test_constr_door->requirements );
        REQUIRE_FALSE( plan.empty() );
        int n = 0;
        for( const std::tuple<tripoint_bub_ms, itype_id, int> &step : plan ) {
            n += std::get<1>( step ) == type ? std::get<2>( step ) : 0;
        }
        return n;
    };

    GIVEN( "filthy hammer carried" ) {
        carry_filthy( itype_hammer );
        THEN( "it serves HAMMER quality, so no hammer is fetched" ) {
            CHECK( planned( itype_hammer ) == 0 );
        }
    }
    GIVEN( "two filthy 2x4s carried" ) {
        carry_filthy( itype_2x4 );
        carry_filthy( itype_2x4 );
        THEN( "they don't count as components, so door's two 2x4s are fetched" ) {
            CHECK( planned( itype_2x4 ) == 2 );
        }
    }
}

TEST_CASE( "construction_fetch_takes_components_out_of_containers",
           "[activities][zones][fetch][construction]" )
{
    get_craft_reservations().clear();
    avatar &u = construction_fetch_setup();
    map &here = get_map();
    const auto box_nails = [&here]( int nails ) -> item & {
        item box( itype_box_small );
        REQUIRE( box.put_in( item( itype_nail, calendar::turn, nails ),
                             pocket_type::CONTAINER ).success() );
        return here.add_item( far_loot, box );
    };
    const auto nails_with = [&here]( const std::function<bool( const item & )> &pick ) {
        int n = 0;
        for( const item &top : here.i_at( far_loot ) ) {
            top.visit_items( [&n, &pick]( const item * e, const item * ) {
                n += e->typeId() == itype_nail && pick( *e ) ? e->charges : 0;
                return VisitResponse::NEXT;
            } );
        }
        return n;
    };
    REQUIRE( here.i_at( far_loot ).empty() );

    GIVEN( "only nail source is box of 30 nails on a loot tile" ) {
        box_nails( 30 );
        WHEN( "12 nails planned for fetch" ) {
            const requirement_id twelve_nails = multi_activity_actor::synthesize_requirements(
            { { item_comp( itype_nail, 12 ) } }, {}, {} );
            fetch_required_activity_actor fetch( twelve_nails, do_activity_reason::NO_COMPONENTS,
                                                 here.get_abs( work_pos + tripoint::north ), here.get_abs( work_pos ) );
            const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> plan =
                        fetch.requirements_map( u );
            THEN( "12 nails planned from loot tile" ) {
                CHECK( fetch.last_plan == fetch_required_activity_actor::fetch_plan::fetching );
                REQUIRE( plan.size() == 1 );
                CHECK( std::get<0>( plan.front() ) == far_loot );
                CHECK( std::get<1>( plan.front() ) == itype_nail );
                CHECK( std::get<2>( plan.front() ) == 12 );
            }
        }
    }

    GIVEN( "sealed can of two pelmeni from a loot spawn, on a loot tile" ) {
        const item_group::ItemList spawned =
            item_group::items_from( Item_spawn_data_pelmeni_can_medium_2 );
        REQUIRE( spawned.size() == 1 );
        const item &can = here.add_item( far_loot, spawned.front() );
        REQUIRE( can.all_pockets_sealed() );
        REQUIRE( can.all_items_top( pocket_type::CONTAINER ).size() == 2 );
        WHEN( "one pelmeni fetched" ) {
            const requirement_id one_pelmeni = multi_activity_actor::synthesize_requirements(
            { { item_comp( itype_pelmeni, 1 ) } }, {}, {} );
            fetch_required_activity_actor fetch( one_pelmeni, do_activity_reason::NO_COMPONENTS,
                                                 here.get_abs( work_pos + tripoint::north ), here.get_abs( work_pos ) );
            REQUIRE( fetch.fetch_activity( u, far_loot, ACT_FETCH_REQUIRED, MAX_VIEW_DISTANCE ) );
            THEN( "one pelmeni at work spot, other stays in can, can unsealed" ) {
                CHECK( count_on_map( work_pos, itype_pelmeni ) == 1 );
                CHECK( can.all_items_top( pocket_type::CONTAINER ).size() == 1 );
                CHECK_FALSE( can.any_pockets_sealed() );
            }
        }
    }

    GIVEN( "door blueprint on a frame, door's nails only in boxes, one box reserved" ) {
        u.backlog.clear();
        here.ter_set( work_pos, ter_t_door_frame );
        const construction &door = construction_test_constr_door.obj();
        zone_manager::get_manager().add( "door", zone_type_CONSTRUCTION_BLUEPRINT,
                                         u.get_faction()->id, false, true, here.get_abs( work_pos ), here.get_abs( work_pos ),
                                         make_shared_fast<blueprint_options>( ter_t_door_frame.str(), door.group, door.id ) );
        u.set_skill_level( skill_fabrication, 2 );
        REQUIRE( u.meets_skill_requirements( door ) );
        REQUIRE( u.i_add( item( itype_hammer ) ) );
        REQUIRE( u.i_add( item( itype_bow_saw ) ) );
        int nails_needed = 0;
        for( const std::vector<item_comp> &line : door.requirements->get_components() ) {
            const item one( line.front().type );
            if( one.typeId() == itype_nail ) {
                nails_needed = line.front().count;
                continue;
            }
            for( int i = 0; i < ( one.count_by_charges() ? 1 : line.front().count ); ++i ) {
                here.add_item_or_charges( far_loot, one.count_by_charges() ?
                                          item( one.typeId(), calendar::turn, line.front().count ) : one );
            }
        }
        REQUIRE( nails_needed > 0 );
        const item &reserved_nails = box_nails( 30 ).only_item();
        const int64_t reserved_uid = reserved_nails.uid().get_value();
        craft_reservation_index::record claim;
        claim.craft_uid = 4242;
        claim.provider_item_uids.push_back( reserved_uid );
        claim.expires_at = calendar::turn + 1_days;
        get_craft_reservations().set( claim );
        REQUIRE( craft_reservation::contains_reserved( reserved_nails ) );
        const item &free_box = box_nails( nails_needed + 8 );
        here.build_map_cache( 0 );

        WHEN( "character works construction zone" ) {
            u.assign_activity( multi_build_construction_activity_actor() );
            const std::string debugmsgs = capture_debugmsg_during( [&u]() {
                REQUIRE( process_activity_with_travel( u, 20000 ) );
            } );
            THEN( "free box's nails taken out, door built, reserved box untouched" ) {
                CHECK( debugmsgs.empty() );
                CHECK( here.ter( work_pos ) == ter_t_door_c );
                CHECK( nails_with( [reserved_uid]( const item & e ) {
                    return e.uid().get_value() == reserved_uid;
                } ) == 30 );
                CHECK( nails_with( [reserved_uid]( const item & e ) {
                    return e.uid().get_value() != reserved_uid;
                } ) == 8 );
                CHECK( free_box.only_item().charges == 8 );
                for( const item &top : here.i_at( far_loot ) ) {
                    CHECK( top.typeId() != itype_nail );
                }
                CHECK_FALSE( any_item_marked( u ) );
            }
        }
    }
    get_craft_reservations().clear();
}

TEST_CASE( "followers_build_separate_construction_sites", "[activities][zones][construction]" )
{
    clear_map_without_vision();
    clear_avatar();
    zone_manager::get_manager().clear();
    set_time_to_day();
    Messages::clear_messages();
    map &here = get_map();
    get_avatar().setpos( here, work_pos + tripoint( -15, 0, 0 ) );
    const tripoint_bub_ms near_site = work_pos;
    const tripoint_bub_ms far_site = work_pos + tripoint( 4, 0, 0 );
    // within both sites' PICKUP_RANGE, so nothing is fetched
    const tripoint_bub_ms stock = work_pos + tripoint( 2, 2, 0 );
    const construction &door = construction_test_constr_door.obj();
    const auto add_door_site = [&here, &stock, &door]( const tripoint_bub_ms & site ) {
        here.ter_set( site, ter_t_door_frame );
        zone_manager::get_manager().add( "door", zone_type_CONSTRUCTION_BLUEPRINT,
                                         faction_your_followers, false, true, here.get_abs( site ), here.get_abs( site ),
                                         make_shared_fast<blueprint_options>( ter_t_door_frame.str(), door.group, door.id ) );
        for( const std::vector<item_comp> &line : door.requirements->get_components() ) {
            const item one( line.front().type );
            for( int i = 0; i < ( one.count_by_charges() ? 1 : line.front().count ); ++i ) {
                here.add_item_or_charges( stock, one.count_by_charges() ?
                                          item( one.typeId(), calendar::turn, line.front().count ) : one );
            }
        }
    };
    const auto builder = [&door]( const tripoint_bub_ms & pos ) -> npc & {
        npc &guy = spawn_follower( pos );
        guy.set_skill_level( skill_fabrication, 2 );
        REQUIRE( guy.meets_skill_requirements( door ) );
        REQUIRE( guy.i_add( item( itype_hammer ) ) );
        REQUIRE( guy.i_add( item( itype_bow_saw ) ) );
        return guy;
    };
    const auto start_building = [&here]( npc & guy ) {
        talk_function::do_construction( guy );
        run_npc_turns( guy, 500, []( const npc & n ) {
            return n.activity.id() == ACT_BUILD;
        } );
        REQUIRE( guy.activity.id() == ACT_BUILD );
        here.build_map_cache( 0 );
    };
    const auto avatar_turn = [&here]( avatar & u ) {
        u.set_moves( u.get_speed() );
        if( u.is_auto_moving() ) {
            u.setpos( here, here.get_bub( *u.destination_point ) );
            here.build_map_cache( u.posz() );
            u.start_destination_activity();
        }
        u.activity.do_turn( u );
    };
    const auto avatar_starts_building = [&here, &door, &avatar_turn]( const tripoint_bub_ms & pos ) {
        avatar &u = get_avatar();
        u.setpos( here, pos );
        u.set_skill_level( skill_fabrication, 2 );
        REQUIRE( u.meets_skill_requirements( door ) );
        REQUIRE( u.i_add( item( itype_hammer ) ) );
        REQUIRE( u.i_add( item( itype_bow_saw ) ) );
        u.assign_activity( multi_build_construction_activity_actor() );
        for( int i = 0; i < 500 && u.activity.id() != ACT_BUILD; ++i ) {
            avatar_turn( u );
        }
        REQUIRE( u.activity.id() == ACT_BUILD );
        here.build_map_cache( 0 );
    };
    // section start clears message log, so count before entering THEN
    const auto reports_of = []( const Character & guy, const std::string & what ) {
        int n = 0;
        for( const std::pair<std::string, std::string> &msg : Messages::recent_messages( 1000 ) ) {
            if( msg.second.find( guy.disp_name() ) != std::string::npos &&
                msg.second.find( what ) != std::string::npos ) {
                ++n;
            }
        }
        return n;
    };
    const auto failures_of = [&reports_of]( const Character & guy ) {
        return reports_of( guy, "failed to perform" ) + reports_of( guy, "did not find" );
    };
    const auto ended_cleanly = []( const npc & guy ) {
        if( guy.activity ) {
            return false;
        }
        for( const player_activity &act : guy.backlog ) {
            if( act.id() == ACT_MULTIPLE_CONSTRUCTION ) {
                return false;
            }
        }
        return true;
    };
    // both nearer near_site
    const tripoint_bub_ms first_pos = work_pos + tripoint( -1, 2, 0 );
    const tripoint_bub_ms second_pos = work_pos + tripoint( -1, 3, 0 );

    GIVEN( "two door sites, first follower builds one" ) {
        add_door_site( near_site );
        add_door_site( far_site );
        here.build_map_cache( 0 );
        npc &first = builder( first_pos );
        npc &second = builder( second_pos );
        REQUIRE( second.is_player_ally() );
        start_building( first );
        const bool near_taken = here.partial_con_at( near_site ) != nullptr;
        const tripoint_bub_ms taken = near_taken ? near_site : far_site;
        const tripoint_bub_ms free_site = near_taken ? far_site : near_site;
        REQUIRE( here.partial_con_at( taken ) != nullptr );
        REQUIRE( here.partial_con_at( free_site ) == nullptr );

        WHEN( "second follower told to construct too" ) {
            talk_function::do_construction( second );
            run_npc_turns( second, 500, []( const npc & n ) {
                return n.activity.id() == ACT_BUILD;
            } );
            THEN( "second builds free site while first still builds taken one" ) {
                CHECK( first.activity.id() == ACT_BUILD );
                CHECK( first.activity.placement == here.get_abs( taken ) );
                CHECK( second.activity.id() == ACT_BUILD );
                CHECK( second.activity.placement == here.get_abs( free_site ) );
                CHECK( here.partial_con_at( taken ) != nullptr );
                CHECK( here.partial_con_at( free_site ) != nullptr );
                AND_WHEN( "both work on" ) {
                    const auto built = [&here, &near_site, &far_site]() {
                        return here.ter( near_site ) == ter_t_door_c && here.ter( far_site ) == ter_t_door_c;
                    };
                    for( int i = 0; i < 20000 && !built(); ++i ) {
                        run_npc_turn( first );
                        run_npc_turn( second );
                    }
                    const int first_failures = failures_of( first );
                    const int second_failures = failures_of( second );
                    THEN( "both doors built, neither follower fails" ) {
                        CHECK( built() );
                        CHECK( first_failures == 0 );
                        CHECK( second_failures == 0 );
                    }
                }
            }
        }
    }

    GIVEN( "one door site, first follower builds it" ) {
        add_door_site( near_site );
        here.build_map_cache( 0 );
        npc &first = builder( first_pos );
        npc &second = builder( second_pos );
        REQUIRE( second.is_player_ally() );
        start_building( first );
        REQUIRE( here.partial_con_at( near_site ) != nullptr );

        WHEN( "second follower told to construct too" ) {
            talk_function::do_construction( second );
            run_npc_turns( second, 500, []( const npc & n ) {
                return n.activity.id() == ACT_BUILD;
            } );
            THEN( "second helps first on the one site" ) {
                CHECK( first.activity.placement == here.get_abs( near_site ) );
                CHECK( second.activity.id() == ACT_BUILD );
                CHECK( second.activity.placement == here.get_abs( near_site ) );
                AND_WHEN( "both work on" ) {
                    for( int i = 0; i < 20000 && here.ter( near_site ) != ter_t_door_c; ++i ) {
                        run_npc_turn( first );
                        run_npc_turn( second );
                    }
                    run_npc_turns( first, 10 );
                    run_npc_turns( second, 10 );
                    const int first_failures = failures_of( first );
                    const int second_failures = failures_of( second );
                    THEN( "door built, both followers end without failing" ) {
                        CHECK( here.ter( near_site ) == ter_t_door_c );
                        CHECK( ended_cleanly( first ) );
                        CHECK( ended_cleanly( second ) );
                        CHECK( first_failures == 0 );
                        CHECK( second_failures == 0 );
                    }
                }
            }
        }

        WHEN( "site cancelled before door stands" ) {
            here.partial_con_remove( near_site );
            run_npc_turn( first );
            const int site_gone_reports = reports_of( first, "did not find" );
            THEN( "first follower stops building and says site is gone" ) {
                CHECK( here.ter( near_site ) == ter_t_door_frame );
                CHECK( first.activity.id() != ACT_BUILD );
                CHECK( site_gone_reports == 1 );
            }
        }
    }

    GIVEN( "two door sites, avatar builds the near one" ) {
        add_door_site( near_site );
        add_door_site( far_site );
        here.build_map_cache( 0 );
        avatar_starts_building( near_site + tripoint::west );
        REQUIRE( get_avatar().activity.placement == here.get_abs( near_site ) );
        npc &guy = builder( first_pos );
        WHEN( "follower told to construct" ) {
            start_building( guy );
            THEN( "follower builds the far site" ) {
                CHECK( guy.activity.placement == here.get_abs( far_site ) );
                CHECK( here.partial_con_at( far_site ) != nullptr );
            }
        }
    }

    GIVEN( "one door site, avatar builds it" ) {
        add_door_site( near_site );
        here.build_map_cache( 0 );
        avatar_starts_building( near_site + tripoint::west );
        npc &guy = builder( first_pos );
        WHEN( "follower told to construct" ) {
            start_building( guy );
            THEN( "follower helps" ) {
                CHECK( guy.activity.placement == here.get_abs( near_site ) );
                AND_WHEN( "both work on" ) {
                    avatar &u = get_avatar();
                    for( int i = 0; i < 20000 && here.ter( near_site ) != ter_t_door_c; ++i ) {
                        avatar_turn( u );
                        run_npc_turn( guy );
                    }
                    run_npc_turns( guy, 10 );
                    const int failures = failures_of( guy );
                    THEN( "door built, follower ends without failing" ) {
                        CHECK( here.ter( near_site ) == ter_t_door_c );
                        CHECK( ended_cleanly( guy ) );
                        CHECK( failures == 0 );
                    }
                }
            }
        }
    }
}
