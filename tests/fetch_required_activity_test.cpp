#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "activity_actor.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "clone_ptr.h"
#include "clzones.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "mtype.h"
#include "npc.h"
#include "npc_turn_helpers.h"
#include "npctalk.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );

static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_butcher_rack( "f_butcher_rack" );

static const itype_id itype_fanny( "fanny" );
static const itype_id itype_knife_hunting( "knife_hunting" );
static const itype_id itype_mc_nodachi( "mc_nodachi" );
static const itype_id itype_saw_bronze( "saw_bronze" );
static const itype_id itype_test_hacksaw( "test_hacksaw" );

static const mtype_id mon_chicken( "mon_chicken" );
static const mtype_id mon_cow( "mon_cow" );

static const quality_id qual_BUTCHER( "BUTCHER" );

static const ter_str_id ter_t_wall( "t_wall" );

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

npc &butcher_follower( const tripoint_bub_ms &pos )
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
    npc &guy = butcher_follower( corpse_pos + tripoint( -6, 0, 0 ) );
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
    npc &guy = butcher_follower( corpse_pos + tripoint::west );
    guy.worn.wear_item( guy, item( itype_fanny ), false, false );
    const activity_reason_info first =
        multi_butchery_activity_actor().multi_activity_can_do( guy, corpse_pos );
    REQUIRE( first.reason == do_activity_reason::NEEDS_BIG_BUTCHERING );

    const item &too_big = here.add_item( loot_near, item( itype_mc_nodachi ) );
    const item &fits = here.add_item( loot_mid, item( itype_knife_hunting ) );
    here.add_item( loot_far, item( itype_saw_bronze ) );
    here.add_item( loot_far, item( itype_test_hacksaw ) );
    REQUIRE( too_big.volume() > guy.free_space() );
    REQUIRE( fits.volume() <= guy.free_space() );

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
}
