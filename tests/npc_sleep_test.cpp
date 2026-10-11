#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "npc_turn_helpers.h"
#include "options_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const efftype_id effect_sleep( "sleep" );

static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_bed( "f_bed" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static const tripoint_bub_ms leader_pos( 50, 50, 0 );

static void sleep_setup()
{
    // time first: placing the leader starts its time_since_relocated clock
    clear_map_without_vision();
    clear_avatar();
    set_time_to_day();
    get_avatar().setpos( get_map(), leader_pos );
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
}

// leader walks off and back, then stays in one place for `still`
static void leader_arrived( const time_duration &still )
{
    get_avatar().setpos( get_map(), leader_pos + tripoint( 30, 0, 0 ) );
    get_avatar().setpos( get_map(), leader_pos );
    calendar::turn += still;
}

static npc &tired_follower( const tripoint_bub_ms &pos, const int sleepiness )
{
    map &here = get_map();
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.rules.set_flag( ally_rule::follow_close );
    guy.rules.set_flag( ally_rule::allow_sleep );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    guy.set_sleepiness( sleepiness );
    REQUIRE( guy.is_walking_with() );
    REQUIRE( guy.needs_food() );
    return guy;
}

static int dist_to_leader( const npc &guy )
{
    return rl_dist( guy.pos_bub(), leader_pos );
}

static bool settles_within( npc &guy, const int max_turns )
{
    run_npc_turns( guy, max_turns, []( const npc & n ) {
        return n.in_sleep_state();
    } );
    return guy.in_sleep_state();
}

static bool never_settles( npc &guy, const int turns )
{
    for( int turn = 0; turn < turns; ++turn ) {
        run_npc_turn( guy );
        if( guy.in_sleep_state() ) {
            return false;
        }
    }
    return true;
}

static tripoint_bub_ms mount_tile( const vehicle &veh, const point_rel_ms &mount )
{
    for( const vpart_reference &vp : veh.get_all_parts() ) {
        if( vp.mount_pos() == mount ) {
            return veh.bub_part_pos( get_map(), vp.part() );
        }
    }
    FAIL( "no part at that mount" );
    return tripoint_bub_ms::zero;
}

static vehicle &parked_rv( const tripoint_bub_ms &where )
{
    vehicle *veh = get_map().add_vehicle( vehicle_prototype_test_rv, where, 0_degrees, 100,
                                          veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    return *veh;
}

static void seat( Character &who, const tripoint_bub_ms &p )
{
    map &here = get_map();
    who.setpos( here, p );
    if( !who.in_vehicle ) {
        here.board_vehicle( p, &who );
    }
    REQUIRE( who.in_vehicle );
    if( who.is_avatar() ) {
        // boarding recenters the map and can load submaps with monsters
        clear_creatures();
    }
}

static bool on_belted_seat( const npc &guy, const vehicle &veh )
{
    const optional_vpart_position vp = get_map().veh_at( guy.pos_bub() );
    return vp && &vp->vehicle() == &veh && vp->part_with_feature( "SEATBELT", true );
}

static bool on_bed( const npc &guy, const vehicle &veh )
{
    const optional_vpart_position vp = get_map().veh_at( guy.pos_bub() );
    return vp && &vp->vehicle() == &veh &&
           ( vp->part_with_feature( "SEATBELT", true ) || vp->part_with_feature( "BED", true ) );
}

TEST_CASE( "no_npc_food_sleepiness_stops_short_of_dead_tired", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    override_option no_food( "NO_NPC_FOOD", "true" );
    npc &guy = spawn_npc( point_bub_ms( 50, 50 ), "test_talker" );
    clear_character( guy, true );
    REQUIRE_FALSE( guy.needs_food() );

    SECTION( "tiredness still rises past tired" ) {
        const int start = static_cast<int>( sleepiness_levels::TIRED ) + 10;
        guy.set_sleepiness( start );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() > start );
    }
    SECTION( "it stops just short of dead tired, with no deprivation" ) {
        guy.set_sleepiness( static_cast<int>( sleepiness_levels::DEAD_TIRED ) + 50 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() == static_cast<int>( sleepiness_levels::DEAD_TIRED ) - 1 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
    SECTION( "already past where tiredness stops rising, it comes down too" ) {
        guy.set_sleepiness( 1100 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() == static_cast<int>( sleepiness_levels::DEAD_TIRED ) - 1 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
    SECTION( "asleep, lack of sleep it already had goes away" ) {
        guy.add_effect( effect_sleep, 1_hours );
        guy.set_sleepiness( 300 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
}

TEST_CASE( "sleep_readiness_reflects_bed_and_stimulants", "[npc][sleep]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( point_bub_ms( 50, 50 ), "test_talker" );
    clear_character( guy, true );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );
    // well past the rng( -8, 8 ) of a sleep roll either way
    CHECK( guy.sleep_readiness() > 8 );
    guy.set_stim( 50 );
    CHECK( guy.sleep_readiness() < -8 );
}

TEST_CASE( "leader_relocation_tracks_travel_not_shuffling", "[npc][sleep]" )
{
    sleep_setup();
    avatar &you = get_avatar();
    map &here = get_map();
    CHECK( you.time_since_relocated() == 0_turns );
    calendar::turn += 10_minutes;
    CHECK( you.time_since_relocated() >= 10_minutes );

    SECTION( "a step or two keeps it" ) {
        you.setpos( here, leader_pos + tripoint( 2, 0, 0 ) );
        CHECK( you.time_since_relocated() >= 10_minutes );
    }
    SECTION( "a walk resets it" ) {
        you.setpos( here, leader_pos + tripoint( 10, 0, 0 ) );
        CHECK( you.time_since_relocated() == 0_turns );
    }
    SECTION( "another level resets it" ) {
        you.setpos( here, leader_pos + tripoint::above );
        CHECK( you.time_since_relocated() == 0_turns );
    }
    SECTION( "staying in one place after a load counts from the load" ) {
        std::ostringstream os;
        JsonOut jsout( os );
        you.serialize( jsout );
        calendar::turn += 1_hours;
        // a fresh avatar, as a game load makes one: no anchor until deserialize sets it
        avatar loaded;
        loaded.deserialize( json_loader::from_string( os.str() ).get_object() );
        calendar::turn += 15_minutes;
        CHECK( loaded.time_since_relocated() >= 15_minutes );
        CHECK( loaded.time_since_relocated() < 16_minutes );
    }
}

TEST_CASE( "need_floor_order_holds_for_every_need", "[npc][needs][obligation]" )
{
    for( const npc::need_goal_id id : {
             npc::need_goal_id::drink_water, npc::need_goal_id::go_to_sleep,
             npc::need_goal_id::eat_food, npc::need_goal_id::seek_warmth
         } ) {
        const float strict = npc_need_floor( id, { npc_permission::discouraged, npc_obligation::none, 0.0f } );
        const float emergency = npc_need_floor( id, { npc_permission::emergency_only, npc_obligation::none, 0.0f } );
        CAPTURE( static_cast<int>( id ) );
        CHECK( npc_need_floor( id, { npc_permission::allowed, npc_obligation::none, 0.0f } ) == 0.0f );
        CHECK( strict <= emergency );
        CHECK( emergency < npc_need_floor( id, { npc_permission::blocked, npc_obligation::none, 0.0f } ) );
        float looser = strict;
        for( const float ease : {
                 0.5f, 0.999f
             } ) {
            const float relaxed = npc_need_floor( id, { npc_permission::discouraged, npc_obligation::none, ease } );
            CAPTURE( ease );
            CHECK( 0.0f < relaxed );
            CHECK( relaxed <= looser );
            looser = relaxed;
        }
    }
}

TEST_CASE( "accompany_floor_slides_while_the_leader_stays_in_one_place",
           "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 0 );
    const npc_option_cost here_rest = npc_option_cost::trip( here.get_abs( leader_pos + tripoint( 2, 0,
                                      0 ) ), false, false, true );
    const auto rest_view = [&guy]( const npc_option_cost & c ) {
        return guy.need_view( npc::need_goal_id::go_to_sleep, c );
    };

    SECTION( "just arrived: floor at dead tired" ) {
        leader_arrived( 0_minutes );
        const npc_need_view v = rest_view( here_rest );
        CHECK( v.verdict.permission == npc_permission::discouraged );
        CHECK( v.verdict.ease == 0.0f );
        CHECK( v.floor == Approx( 0.383f ) );
    }
    SECTION( "8.5 minutes in place: floor half way to tired" ) {
        leader_arrived( 8_minutes + 30_seconds );
        const npc_need_view v = rest_view( here_rest );
        CHECK( v.verdict.permission == npc_permission::discouraged );
        CHECK( v.floor == Approx( 0.287f ).margin( 0.01f ) );
        CHECK( v.strict_floor == Approx( 0.383f ) );

        WHEN( "also told to stay awake" ) {
            guy.rules.clear_flag( ally_rule::allow_sleep );
            const npc_need_view w = rest_view( here_rest );
            CHECK( w.verdict.by == npc_obligation::stay_awake_rule );
            CHECK( w.floor == Approx( 0.575f ) );
        }
        WHEN( "it has an order to move" ) {
            guy.goto_to_this_pos = here.get_abs( leader_pos + tripoint( -3, 0, 0 ) );
            const npc_need_view w = rest_view( here_rest );
            CHECK( w.verdict.by == npc_obligation::goto_order );
            CHECK( w.floor == Approx( 0.575f ) );
        }
        WHEN( "the spot is past the spacing" ) {
            const npc_need_view w = rest_view( npc_option_cost::trip( here.get_abs( leader_pos + tripoint( 8, 0,
                                               0 ) ), true, false, true ) );
            CHECK( w.verdict.by == npc_obligation::accompany );
            CHECK( w.floor == Approx( 0.575f ) );
        }
    }
    SECTION( "settled for fifteen minutes: allowed" ) {
        leader_arrived( 15_minutes );
        const npc_need_view v = rest_view( here_rest );
        CHECK( v.verdict.permission == npc_permission::allowed );
        CHECK( v.floor == 0.0f );
    }
    SECTION( "leader asleep: allowed at once" ) {
        leader_arrived( 0_minutes );
        get_avatar().add_effect( effect_sleep, 2_hours );
        CHECK( rest_view( here_rest ).verdict.permission == npc_permission::allowed );
        get_avatar().remove_effect( effect_sleep );
    }
}

TEST_CASE( "follower_naps_once_the_leader_has_settled", "[npc][behavior][sleep]" )
{
    sleep_setup();
    get_map().furn_set( leader_pos + tripoint( 3, 0, 0 ), furn_f_bed );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 300 );

    SECTION( "four minutes: still too soon" ) {
        leader_arrived( 4_minutes );
        CHECK( never_settles( guy, 20 ) );
    }
    SECTION( "nine minutes: it lies down nearby, and doesn't explain itself" ) {
        leader_arrived( 9_minutes );
        CHECK( settles_within( guy, 20 ) );
        CHECK( dist_to_leader( guy ) <= 4 );
        CHECK_FALSE( guy.last_complaint( "need_override_sleep" ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
    }
}

TEST_CASE( "vehicle_bed_relaxes_accompaniment_only_on_a_belted_seat_or_bed",
           "[npc][needs][sleep][vehicle][obligation]" )
{
    sleep_setup();
    clear_vehicles();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 0 );
    const auto rest_here = [&guy]() {
        return guy.judge_option( npc_option_cost::trip( guy.pos_abs(), false, false, true ) );
    };

    SECTION( "a belted seat" ) {
        seat( guy, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
        CHECK( guy.in_vehicle_bed() );
        CHECK( rest_here().permission == npc_permission::allowed );
    }
    SECTION( "the bed" ) {
        seat( guy, mount_tile( veh, point_rel_ms( -5, 2 ) ) );
        CHECK( guy.in_vehicle_bed() );
        CHECK( rest_here().permission == npc_permission::allowed );
    }
    SECTION( "a seat without a belt" ) {
        seat( guy, mount_tile( veh, point_rel_ms( -2, 0 ) ) );
        CHECK_FALSE( guy.in_vehicle_bed() );
        CHECK( rest_here().permission == npc_permission::discouraged );
    }
    SECTION( "the aisle" ) {
        seat( guy, mount_tile( veh, point_rel_ms( -2, 1 ) ) );
        CHECK_FALSE( guy.in_vehicle_bed() );
        CHECK( rest_here().permission == npc_permission::discouraged );
    }
    SECTION( "a belted seat in another vehicle" ) {
        vehicle &other = parked_rv( leader_pos + tripoint( 0, 12, 0 ) );
        seat( guy, mount_tile( other, point_rel_ms( 0, 2 ) ) );
        CHECK_FALSE( guy.in_vehicle_bed() );
        CHECK( rest_here().permission == npc_permission::emergency_only );
    }
}

TEST_CASE( "embark_moves_an_unassigned_passenger_to_a_belted_seat", "[npc][behavior][vehicle]" )
{
    sleep_setup();
    clear_vehicles();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 0 );
    const tripoint_bub_ms unbelted = mount_tile( veh, point_rel_ms( -2, 0 ) );
    seat( guy, unbelted );

    SECTION( "no assigned seat: it moves to the belted one" ) {
        run_npc_turns( guy, 10, [&veh]( const npc & n ) {
            return on_belted_seat( n, veh );
        } );
        CHECK( on_belted_seat( guy, veh ) );
        CHECK( guy.in_vehicle );
    }
    SECTION( "its assigned seat has no belt: it stays" ) {
        const optional_vpart_position vp = get_map().veh_at( unbelted );
        REQUIRE( vp );
        const std::optional<vpart_reference> seat_part = vp->part_with_feature( "SEAT", true );
        REQUIRE( seat_part );
        REQUIRE( seat_part->part().set_crew( guy ) );
        run_npc_turns( guy, 10 );
        CHECK( guy.pos_bub() == unbelted );
    }
}

TEST_CASE( "belted_passenger_naps_while_the_leader_is_awake_aboard",
           "[npc][behavior][vehicle][sleep]" )
{
    sleep_setup();
    clear_vehicles();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 250 );
    seat( guy, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
    REQUIRE( guy.in_vehicle_bed() );
    REQUIRE_FALSE( get_avatar().can_use_floor_warmth() );
    on_out_of_scope stop_crafting( [] {
        get_avatar().activity = player_activity();
    } );

    SECTION( "leader just sat down" ) {
        CHECK( settles_within( guy, 20 ) );
        CHECK( on_bed( guy, veh ) );
    }
    SECTION( "leader crafting" ) {
        get_avatar().activity = player_activity( ACT_CRAFT );
        CHECK( settles_within( guy, 20 ) );
        CHECK( on_bed( guy, veh ) );
    }
}

TEST_CASE( "seated_follower_sleeps_when_leader_sleeps_aboard", "[npc][behavior][vehicle][sleep]" )
{
    sleep_setup();
    clear_vehicles();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    get_avatar().add_effect( effect_sleep, 2_hours );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 250 );
    seat( guy, mount_tile( veh, point_rel_ms( -2, 0 ) ) );

    run_npc_turns( guy, 20, []( const npc & n ) {
        return n.in_sleep_state();
    } );
    CAPTURE( guy.pos_bub(), guy.get_committed_goal(), guy.in_vehicle, guy.in_vehicle_bed() );
    CHECK( guy.in_sleep_state() );
    CHECK( on_bed( guy, veh ) );
    get_avatar().remove_effect( effect_sleep );
}

TEST_CASE( "passenger_naps_while_riding_along", "[npc][needs][sleep][vehicle]" )
{
    sleep_setup();
    clear_vehicles();
    map &here = get_map();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 250 );
    guy.rules.set_flag( ally_rule::follow_distance_2 );
    // spawn_npc can shift the map: look tiles up only after every spawn

    SECTION( "belted in, it lies down in its seat and rides along" ) {
        seat( guy, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
        veh.velocity = 1000;
        CHECK( settles_within( guy, 20 ) );
        for( int round = 0; round < 5; ++round ) {
            CAPTURE( round );
            veh.precalc_mounts( 1, veh.face.dir(), veh.pivot_point( here ) );
            REQUIRE( here.displace_vehicle( veh, tripoint_rel_ms::east ) );
            run_npc_turn( guy );
            CHECK( on_bed( guy, veh ) );
            CHECK( guy.in_sleep_state() );
        }
    }
    SECTION( "no bed free, it stays awake" ) {
        npc &other = tired_follower( leader_pos + tripoint( 2, 10, 0 ), 0 );
        npc &third = tired_follower( leader_pos + tripoint( 4, 10, 0 ), 0 );
        seat( other, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
        seat( third, mount_tile( veh, point_rel_ms( -5, 2 ) ) );
        seat( guy, mount_tile( veh, point_rel_ms( -2, 0 ) ) );
        veh.velocity = 1000;
        CHECK( never_settles( guy, 20 ) );
    }
    SECTION( "a bed free, it takes it" ) {
        seat( guy, mount_tile( veh, point_rel_ms( -2, 0 ) ) );
        veh.velocity = 1000;
        CHECK( settles_within( guy, 20 ) );
        CHECK( on_bed( guy, veh ) );
    }
}
