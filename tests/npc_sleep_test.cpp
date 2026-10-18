#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "basecamp.h"
#include "behavior.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "character_id.h"
#include "character_oracle.h"
#include "coordinates.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"
#include "lightmap.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "npc_sleep.h"
#include "npc_turn_helpers.h"
#include "npctalk.h"
#include "options_helpers.h"
#include "overmapbuffer.h"
#include "overmap_ui.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "sounds.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const efftype_id effect_learning_serum_main( "learning_serum_main" );
static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_slept_through_alarm( "slept_through_alarm" );

static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_makeshift_bed( "f_makeshift_bed" );
static const furn_str_id furn_f_sofa( "f_sofa" );

static const json_character_flag json_flag_CANNOT_SLEEP( "CANNOT_SLEEP" );

static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_walk( "walk" );

static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_wall( "t_wall" );

static const trait_id trait_HEAVYSLEEPER( "HEAVYSLEEPER" );
static const trait_id trait_HEAVYSLEEPER2( "HEAVYSLEEPER2" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static const weather_type_id weather_clear( "clear" );

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

// non-ally far from the player, needs settled
static npc &stranger( const tripoint_bub_ms &pos )
{
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( get_map(), pos );
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    // otherwise a stranger who wants to talk walks up to the player once awake
    guy.set_attitude( NPCATT_NULL );
    // stays put: an idle stranger otherwise travels off and posts itself as a guard
    guy.set_mission( NPC_MISSION_SHELTER );
    guy.goal = guy.pos_abs_omt();
    REQUIRE_FALSE( guy.is_player_ally() );
    return guy;
}

static bool asleep( const npc &n )
{
    return n.has_effect( effect_sleep );
}

static bool settled( const npc &n )
{
    return n.in_sleep_state();
}

static void walled_in( const tripoint_bub_ms &p )
{
    for( const tripoint_bub_ms &q : get_map().points_in_radius( p, 1 ) ) {
        if( q != p ) {
            get_map().ter_set( q, ter_t_wall );
        }
    }
}

// on a bed at 600 with no stim: sure to fall asleep on the first try
static void make_asleep_on_bed( npc &guy )
{
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    guy.execute_need_goal( "go_to_sleep" );
    REQUIRE( asleep( guy ) );
}

// an engine revving next to it: loud enough to wake any light sleeper; needs clear weather
static void wake_by_noise( npc &guy )
{
    REQUIRE( ( guy.has_effect( effect_sleep ) || guy.has_effect( effect_lying_down ) ) );
    REQUIRE_FALSE( guy.has_trait( trait_HEAVYSLEEPER ) );
    REQUIRE_FALSE( guy.has_trait( trait_HEAVYSLEEPER2 ) );
    REQUIRE( guy.hearing_ability() == Approx( 1.0f ) );
    sounds::reset_sounds();
    sounds::sound( guy.pos_bub() + tripoint::east, 60, sounds::sound_t::movement, "engine" );
    sounds::process_sound_markers( &guy );
    // wake_up() only marks the sleep effect for removal
    guy.process_turn();
    REQUIRE_FALSE( asleep( guy ) );
    REQUIRE( guy.get_sleep_session().phase == npc_sleep_phase::woken );
}

// save then load into a new object, like a game load: nothing unsaved survives
static npc_sleep_session round_trip( npc &guy )
{
    std::ostringstream os;
    JsonOut jsout( os );
    guy.serialize( jsout );
    npc fresh;
    fresh.deserialize( json_loader::from_string( os.str() ).get_object() );
    guy = std::move( fresh );
    guy.on_load( &get_map() );
    return guy.get_sleep_session();
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

TEST_CASE( "npc_settling_ignores_player_sleep_throttle", "[npc][needs][sleep]" )
{
    sleep_setup();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );
    // a roll now starts the player's 30-minute throttle
    guy.can_sleep();
    REQUIRE_FALSE( guy.can_sleep() );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    CHECK( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::progressed );
    CHECK( asleep( guy ) );
    run_npc_turns( guy, to_turns<int>( 30_minutes ) );
    CHECK( guy.get_sleepiness() < 600 );
}

TEST_CASE( "npc_settling_rolls_until_asleep_or_gives_the_spot_up", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    const tripoint_bub_ms bed = guy.pos_bub();
    here.furn_set( bed, furn_f_bed );
    guy.set_sleepiness( 300 );
    guy.set_stim( 100 );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
    REQUIRE( guy.has_effect( effect_lying_down ) );
    REQUIRE_FALSE( asleep( guy ) );
    CHECK( guy.get_current_status() == "Trying to sleep" );
    CHECK( guy.get_sleep_session().phase == npc_sleep_phase::settling );

    SECTION( "stimulant wears off: falls asleep within minutes" ) {
        guy.set_stim( 0 );
        run_npc_turns( guy, to_turns<int>( 4_minutes ), asleep );
        CHECK( asleep( guy ) );
        CHECK( guy.get_current_status() == "Sleeping" );
        CHECK_FALSE( guy.get_sleep_plan().active() );
    }
    SECTION( "still wired after half an hour: it gives the bed up for another" ) {
        const tripoint_bub_ms other = bed + tripoint( 3, 0, 0 );
        here.furn_set( other, furn_f_bed );
        run_npc_turns( guy, to_turns<int>( 31_minutes ), []( const npc & n ) {
            return !n.has_effect( effect_lying_down );
        } );
        REQUIRE_FALSE( asleep( guy ) );
        CHECK( guy.failed_targets_for( npc::need_goal_id::go_to_sleep ).count( here.get_abs( bed ) ) );
        CHECK( guy.get_sleep_session().phase == npc_sleep_phase::none );
        run_npc_turns( guy, 10, settled );
        CHECK( guy.pos_bub() == other );
    }
    SECTION( "still wired, nowhere else to go: it stalls, then tries the same bed again" ) {
        walled_in( bed );
        here.build_map_cache( 0 );
        const time_point first_try = guy.get_sleep_session().settle_started;
        // giving the bed up and the search that finds nothing else happen in the same turn:
        // the effect ends during process_turn(), the executor runs right after
        run_npc_turns( guy, to_turns<int>( 31_minutes ), []( const npc & n ) {
            return !n.has_effect( effect_lying_down );
        } );
        REQUIRE_FALSE( asleep( guy ) );
        REQUIRE( guy.sleep_stalled() );
        CHECK_FALSE( guy.rest_cooldown_active() );
        CHECK( guy.failed_targets_for( npc::need_goal_id::go_to_sleep ).empty() );
        run_npc_turns( guy, 5, settled );
        CHECK( settled( guy ) );
        CHECK( guy.pos_bub() == bed );
        CHECK( guy.get_sleep_session().settle_started > first_try );
    }
}

TEST_CASE( "failed_bed_does_not_hide_a_worse_bed", "[npc][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    get_avatar().add_effect( effect_sleep, 2_hours );
    npc &guy = tired_follower( leader_pos + tripoint::east, 300 );
    const tripoint_bub_ms good = guy.pos_bub() + tripoint( 2, 0, 0 );
    const tripoint_bub_ms worse = guy.pos_bub() + tripoint( 0, 2, 0 );
    here.furn_set( good, furn_f_bed );
    here.furn_set( worse, furn_f_makeshift_bed );
    here.build_map_cache( 0 );
    guy.failed_targets_for( npc::need_goal_id::go_to_sleep ).insert( here.get_abs( good ) );

    bool lists_worse = false;
    for( const npc::need_candidate &c : guy.find_sleep_candidates() ) {
        CHECK( c.target != here.get_abs( good ) );
        lists_worse = lists_worse || c.target == here.get_abs( worse );
    }
    CHECK( lists_worse );
    CHECK( settles_within( guy, 20 ) );
    CHECK( guy.pos_bub() == worse );
    get_avatar().remove_effect( effect_sleep );
}

TEST_CASE( "shaded_bed_beats_sunlit_bed", "[npc][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    guy.set_sleepiness( 300 );
    const tripoint_bub_ms sunny = guy.pos_bub() + tripoint( 2, 0, 0 );
    const tripoint_bub_ms shaded = guy.pos_bub() + tripoint( -3, 0, 0 );
    // a roofed room around the shaded bed, open toward the NPC
    for( const tripoint_bub_ms &q : here.points_in_radius( shaded, 1 ) ) {
        here.ter_set( q, ter_t_wall );
        here.ter_set( q + tripoint::above, ter_t_flat_roof );
    }
    here.ter_set( shaded, ter_t_floor );
    here.ter_set( shaded + tripoint::east, ter_t_floor );
    here.furn_set( sunny, furn_f_bed );
    here.furn_set( shaded, furn_f_bed );
    here.build_map_cache( 1 );
    here.build_map_cache( 0 );
    REQUIRE( here.ambient_light_at( sunny ) > LIGHT_AMBIENT_LIT );
    REQUIRE( here.ambient_light_at( shaded ) < here.ambient_light_at( sunny ) );
    const int sunny_comfort = guy.evaluate_sleep_spot( sunny );
    REQUIRE( guy.evaluate_sleep_spot( shaded ) == sunny_comfort );

    const std::vector<npc::need_candidate> candidates = guy.find_sleep_candidates();
    REQUIRE_FALSE( candidates.empty() );
    CHECK( candidates.front().target == here.get_abs( shaded ) );
}

TEST_CASE( "sleep_session_survives_save_and_load", "[npc][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );

    SECTION( "asleep" ) {
        make_asleep_on_bed( guy );
        const tripoint_bub_ms bed = guy.pos_bub();
        const npc_sleep_session loaded = round_trip( guy );
        CHECK( asleep( guy ) );
        CHECK( loaded.phase == npc_sleep_phase::asleep );
        // no commitment after a load: the woken session brings sleep back by itself
        wake_by_noise( guy );
        run_npc_turns( guy, 3, asleep );
        CHECK( asleep( guy ) );
        CHECK( guy.pos_bub() == bed );
        CHECK( guy.get_sleep_session().wakes == 1 );
    }
    SECTION( "stay-awake grace" ) {
        make_asleep_on_bed( guy );
        REQUIRE( guy.wake_by_player() );
        const npc_sleep_session before = guy.get_sleep_session();
        const npc_sleep_session loaded = round_trip( guy );
        CHECK( loaded.stay_awake_until == before.stay_awake_until );
        CHECK( loaded.stay_awake_floor == before.stay_awake_floor );
        CHECK( guy.sleep_on_hold() );
        CHECK( never_settles( guy, 20 ) );
    }
    SECTION( "settling" ) {
        get_map().furn_set( guy.pos_bub(), furn_f_bed );
        guy.set_sleepiness( 600 );
        guy.set_stim( 100 );
        guy.set_committed_goal( "go_to_sleep" );
        guy.set_moves( 100 );
        REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
        const npc_sleep_session before = guy.get_sleep_session();
        const npc_sleep_session loaded = round_trip( guy );
        CHECK( loaded.phase == npc_sleep_phase::settling );
        CHECK( loaded.settle_started == before.settle_started );
        CHECK( loaded.next_attempt == before.next_attempt );
        guy.set_stim( 0 );
        run_npc_turns( guy, to_turns<int>( 4_minutes ), asleep );
        CHECK( asleep( guy ) );
    }
}

TEST_CASE( "settling_rolls_only_on_its_cadence", "[npc][needs][sleep]" )
{
    sleep_setup();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 100 );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
    // every roll from here succeeds: on a bed at 600 with no stim
    guy.set_stim( 0 );
    const time_point next = guy.get_sleep_session().next_attempt;
    const auto awake_until_next = [&guy, next]() {
        while( calendar::turn < next ) {
            run_npc_turn( guy );
            if( asleep( guy ) ) {
                return false;
            }
        }
        return true;
    };

    SECTION( "in play" ) {
        CHECK( awake_until_next() );
        run_npc_turn( guy );
        CHECK( asleep( guy ) );
    }
    SECTION( "loaded mid-settle" ) {
        run_npc_turns( guy, 30 );
        REQUIRE_FALSE( asleep( guy ) );
        round_trip( guy );
        CHECK( awake_until_next() );
        run_npc_turn( guy );
        CHECK( asleep( guy ) );
    }
}

TEST_CASE( "loaded_settler_rechecks_obligations_on_its_first_turn",
           "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    get_avatar().add_effect( effect_sleep, 2_hours );
    on_out_of_scope wake_leader( [] {
        get_avatar().remove_effect( effect_sleep );
    } );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 300 );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_stim( 100 );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
    round_trip( guy );
    REQUIRE( guy.has_effect( effect_lying_down ) );
    REQUIRE( guy.get_committed_goal().empty() );

    SECTION( "nothing changed: it keeps settling" ) {
        run_npc_turn( guy );
        CHECK( guy.has_effect( effect_lying_down ) );
    }
    SECTION( "leader wakes and walks off" ) {
        get_avatar().remove_effect( effect_sleep );
        get_avatar().setpos( get_map(), leader_pos + tripoint( -10, 0, 0 ) );
        run_npc_turn( guy );
        CHECK_FALSE( guy.in_sleep_state() );
    }
    SECTION( "an order to move" ) {
        guy.goto_to_this_pos = get_map().get_abs( leader_pos + tripoint( 0, 3, 0 ) );
        run_npc_turn( guy );
        CHECK_FALSE( guy.in_sleep_state() );
    }
}

TEST_CASE( "settling_patience_grows_with_time_lying_down", "[npc][sleep]" )
{
    npc_sleep_session ses;
    ses.begin_settling();
    CHECK( ses.patience() == 0 );
    calendar::turn += 7_minutes;
    CHECK( ses.patience() == 2 );
    calendar::turn += 2_hours;
    CHECK( ses.patience() == 10 );
    ses.begin_settling();
    CHECK( ses.patience() == 0 );
}

TEST_CASE( "old_save_without_session_still_settles", "[npc][sleep]" )
{
    sleep_setup();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );
    // lying down as a save from before sleep sessions leaves it
    guy.add_effect( effect_lying_down, 30_minutes, false, 1, false, true );
    std::ostringstream os;
    JsonOut jsout( os );
    guy.serialize( jsout );
    std::string saved = os.str();
    const size_t at = saved.find( "\"sleep_session\":" );
    REQUIRE( at != std::string::npos );
    const size_t end = saved.find( '}', at );
    saved.erase( at, end - at + 2 );
    guy.setID( character_id(), true );
    guy.deserialize( json_loader::from_string( saved ).get_object() );
    REQUIRE( guy.has_effect( effect_lying_down ) );
    REQUIRE( guy.get_sleep_session().phase == npc_sleep_phase::none );

    run_npc_turn( guy );
    CHECK( guy.get_sleep_session().phase == npc_sleep_phase::settling );
    run_npc_turns( guy, to_turns<int>( 4_minutes ), asleep );
    CHECK( asleep( guy ) );
}

TEST_CASE( "sleep_removed_by_script_is_reconciled_before_the_next_decision", "[npc][sleep]" )
{
    sleep_setup();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_asleep_on_bed( guy );
    guy.remove_effect( effect_sleep );
    run_npc_turn( guy );
    CHECK( guy.get_sleep_session().wakes == 1 );
    CHECK( settled( guy ) );
    if( guy.get_sleep_plan().active() ) {
        CHECK( guy.get_sleep_plan().target == guy.pos_abs() );
    }
}

TEST_CASE( "npc_woken_early_lies_back_down_in_place", "[npc][needs][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_asleep_on_bed( guy );
    const tripoint_bub_ms bed = guy.pos_bub();

    wake_by_noise( guy );
    run_npc_turns( guy, 3, asleep );
    CHECK( asleep( guy ) );
    CHECK( guy.pos_bub() == bed );
}

TEST_CASE( "npc_displaced_while_asleep_does_not_walk_back", "[npc][needs][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    map &here = get_map();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_asleep_on_bed( guy );
    const tripoint_bub_ms first = guy.pos_bub();
    const tripoint_abs_ms first_abs = guy.pos_abs();
    const tripoint_bub_ms second = first + tripoint( 4, 0, 0 );
    here.furn_set( first, furn_str_id::NULL_ID() );
    here.furn_set( second, furn_f_bed );
    guy.setpos( here, second );

    wake_by_noise( guy );
    for( int turn = 0; turn < 5; ++turn ) {
        run_npc_turn( guy );
        CAPTURE( turn, guy.pos_bub() );
        if( guy.get_sleep_plan().active() ) {
            CHECK( guy.get_sleep_plan().target != first_abs );
        }
    }
    CHECK( guy.pos_bub() == second );
    CHECK( settled( guy ) );
    CHECK( guy.get_sleep_session().wakes == 1 );
}

TEST_CASE( "woken_below_tired_resumes_only_where_rest_is_allowed", "[npc][needs][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );

    SECTION( "a stranger with no commitment lies back down" ) {
        npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 150 );
        guy.set_committed_goal( "" );
        wake_by_noise( guy );
        run_npc_turns( guy, 5, settled );
        CHECK( settled( guy ) );
    }
    SECTION( "a committed stranger keeps its pursuit and lies back down" ) {
        npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 150 );
        REQUIRE( guy.get_committed_goal() == "go_to_sleep" );
        wake_by_noise( guy );
        run_npc_turns( guy, 5, settled );
        CHECK( settled( guy ) );
        CHECK( guy.get_sleep_session().wakes == 1 );
    }
    SECTION( "same after a load" ) {
        npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 150 );
        round_trip( guy );
        wake_by_noise( guy );
        run_npc_turns( guy, 5, settled );
        CHECK( settled( guy ) );
    }
    SECTION( "a follower whose leader is on the move stays up" ) {
        npc &guy = tired_follower( leader_pos + tripoint::east, 150 );
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 150 );
        leader_arrived( 0_turns );
        wake_by_noise( guy );
        for( int turn = 0; turn < 5; ++turn ) {
            get_avatar().setpos( get_map(), leader_pos + ( turn % 2 == 0 ? tripoint::west :
                                 tripoint::zero ) );
            run_npc_turn( guy );
            CAPTURE( turn, guy.get_committed_goal() );
            CHECK_FALSE( settled( guy ) );
        }
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
    }
    SECTION( "a rested NPC is done sleeping" ) {
        npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 50 );
        wake_by_noise( guy );
        run_npc_turns( guy, 5 );
        CHECK_FALSE( settled( guy ) );
        CHECK( guy.get_sleep_session().phase == npc_sleep_phase::none );
    }
}

// driving shifts the map: a driving test puts the leader back on foot and the map
// back for the next test
static std::function<void()> leader_off_the_road()
{
    const tripoint_abs_sm origin = get_map().get_abs_sub();
    return [origin]() {
        map &here = get_map();
        avatar &you = get_avatar();
        if( you.in_vehicle ) {
            here.unboard_vehicle( you.pos_bub() );
        }
        clear_vehicles();
        here.load( origin, false, false );
        you.setpos( here, leader_pos );
    };
}

TEST_CASE( "passenger_woken_by_engine_noise_sleeps_on_in_its_seat",
           "[npc][needs][sleep][vehicle]" )
{
    sleep_setup();
    clear_vehicles();
    on_out_of_scope cleanup( leader_off_the_road() );
    scoped_weather_override clear_weather( weather_clear );
    map &here = get_map();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 450 );
    seat( guy, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
    veh.velocity = 1000;
    run_npc_turns( guy, 20, asleep );
    REQUIRE( asleep( guy ) );
    REQUIRE( on_bed( guy, veh ) );
    const point_rel_ms bed = here.veh_at( guy.pos_bub() )->mount_pos();

    for( int round = 0; round < 10; ++round ) {
        CAPTURE( round );
        veh.precalc_mounts( 1, veh.face.dir(), veh.pivot_point( here ) );
        REQUIRE( here.displace_vehicle( veh, tripoint_rel_ms::east ) );
        if( round == 1 || round == 6 ) {
            REQUIRE( asleep( guy ) );
            wake_by_noise( guy );
        }
        run_npc_turn( guy );
        CHECK( guy.in_vehicle );
        CHECK( on_bed( guy, veh ) );
        const optional_vpart_position vp = here.veh_at( guy.pos_bub() );
        CHECK( ( vp && vp->mount_pos() == bed ) );
        if( guy.get_sleep_plan().active() ) {
            const optional_vpart_position target = here.veh_at( here.get_bub( guy.get_sleep_plan().target ) );
            CHECK( ( target && &target->vehicle() == &veh ) );
        }
    }
    CHECK( guy.get_sleep_session().wakes == 2 );
    run_npc_turns( guy, 5, asleep );
    CHECK( asleep( guy ) );
}

// three wakes in half an hour stall the sleep
static void wake_three_times( npc &guy )
{
    for( int wake = 0; wake < 3; ++wake ) {
        CAPTURE( wake );
        run_npc_turns( guy, 3, asleep );
        REQUIRE( asleep( guy ) );
        wake_by_noise( guy );
    }
    REQUIRE( guy.get_sleep_session().wakes == 3 );
    REQUIRE( guy.sleep_stalled() );
}

TEST_CASE( "restless_sleeper_keeps_trying_when_nothing_else_is_on_offer", "[npc][needs][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_asleep_on_bed( guy );
    wake_three_times( guy );

    behavior::character_oracle_t oracle( &guy );
    CHECK( oracle.can_sleep( "" ) == behavior::status_t::failure );
    CHECK_FALSE( guy.sleep_on_hold() );
    CHECK( settles_within( guy, 3 ) );
    // same attempt keeps going, so the fourth wake stalls it again
    CHECK( guy.get_sleep_session().wakes == 3 );
}

TEST_CASE( "restless_guard_returns_to_its_post_and_keeps_guarding", "[npc][needs][sleep][guard]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    map &here = get_map();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    const tripoint_abs_ms post = guy.pos_abs();
    guy.set_mission( NPC_MISSION_GUARD );
    guy.set_guard_pos( post );
    REQUIRE( guy.is_on_shift() );
    guy.setpos( here, guy.pos_bub() + tripoint( 6, 0, 0 ) );
    make_asleep_on_bed( guy );
    wake_three_times( guy );

    run_npc_turns( guy, 15, [&post]( const npc & n ) {
        return n.pos_abs() == post;
    } );
    REQUIRE( guy.pos_abs() == post );
    for( int turn = 0; turn < 20; ++turn ) {
        run_npc_turn( guy );
        CAPTURE( turn, guy.get_committed_goal() );
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
        CHECK_FALSE( settled( guy ) );
        CHECK( guy.sleep_stalled() );
    }
    calendar::turn += 31_minutes;
    CHECK( settles_within( guy, 20 ) );
    CHECK( rl_dist( guy.pos_abs(), post ) <= 4 );
}

TEST_CASE( "stalled_passenger_sleeps_once_the_car_stops", "[npc][needs][sleep][vehicle]" )
{
    sleep_setup();
    clear_vehicles();
    on_out_of_scope cleanup( leader_off_the_road() );
    scoped_weather_override clear_weather( weather_clear );
    map &here = get_map();
    vehicle &veh = parked_rv( leader_pos );
    seat( get_avatar(), leader_pos );
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 450 );
    seat( guy, mount_tile( veh, point_rel_ms( 0, 2 ) ) );
    veh.velocity = 1000;
    for( int wake = 0; wake < 3; ++wake ) {
        CAPTURE( wake );
        run_npc_turns( guy, 20, asleep );
        REQUIRE( asleep( guy ) );
        veh.precalc_mounts( 1, veh.face.dir(), veh.pivot_point( here ) );
        REQUIRE( here.displace_vehicle( veh, tripoint_rel_ms::east ) );
        wake_by_noise( guy );
    }
    REQUIRE( guy.sleep_stalled() );
    const tripoint_bub_ms seat_tile = guy.pos_bub();

    veh.velocity = 0;
    get_avatar().assign_activity( wait_activity_actor( 1_hours ) );
    run_npc_turns( guy, 20, asleep );
    CHECK( asleep( guy ) );
    CHECK( guy.pos_bub() == seat_tile );
    get_avatar().cancel_activity();
}

TEST_CASE( "wake_up_dialogue_keeps_rule_and_grants_grace", "[npc][sleep]" )
{
    sleep_setup();
    get_avatar().add_effect( effect_sleep, 2_hours );
    npc &guy = tired_follower( leader_pos + tripoint::east, 600 );

    SECTION( "asleep: the rule stays, it stays up" ) {
        make_asleep_on_bed( guy );
        guy.rules.clear_flag( ally_rule::allow_sleep );
        talk_function::wake_up( guy );
        CHECK_FALSE( guy.rules.has_flag( ally_rule::allow_sleep ) );
        CHECK_FALSE( guy.rules.has_override_enable( ally_rule::allow_sleep ) );
        CHECK_FALSE( settled( guy ) );
        CHECK( guy.sleep_on_hold() );
        CHECK( guy.get_committed_goal().empty() );
        CHECK( never_settles( guy, 20 ) );
    }
    SECTION( "much more tired, it lies down anyway" ) {
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 300 );
        talk_function::wake_up( guy );
        CHECK( never_settles( guy, 20 ) );
        guy.set_sleepiness( 420 );
        CHECK( settles_within( guy, 20 ) );
    }
    SECTION( "after an hour, it lies down again" ) {
        make_asleep_on_bed( guy );
        guy.set_sleepiness( 300 );
        talk_function::wake_up( guy );
        calendar::turn += 61_minutes;
        CHECK( settles_within( guy, 20 ) );
    }
    SECTION( "lying down, not asleep yet" ) {
        get_map().furn_set( guy.pos_bub(), furn_f_bed );
        guy.set_sleepiness( 300 );
        guy.set_stim( 100 );
        guy.set_committed_goal( "go_to_sleep" );
        guy.set_moves( 100 );
        REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
        talk_function::wake_up( guy );
        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
        CHECK( guy.get_sleep_session().phase == npc_sleep_phase::none );
        CHECK( guy.sleep_on_hold() );
    }
    get_avatar().remove_effect( effect_sleep );
}

TEST_CASE( "wake_by_player_goes_through_the_normal_wake", "[npc][sleep]" )
{
    sleep_setup();
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_asleep_on_bed( guy );

    SECTION( "the alarm and prone posture go with the sleep" ) {
        guy.add_effect( effect_slept_through_alarm, 1_hours );
        guy.set_movement_mode( move_mode_prone );
        CHECK( guy.wake_by_player() );
        CHECK_FALSE( guy.has_effect( effect_slept_through_alarm ) );
        CHECK( guy.movement_mode_is( move_mode_walk ) );
        CHECK_FALSE( settled( guy ) );
    }
    SECTION( "sedated, can't be woken" ) {
        guy.add_effect( effect_narcosis, 1_hours );
        CHECK_FALSE( guy.wake_by_player() );
        CHECK( asleep( guy ) );
        CHECK_FALSE( guy.sleep_on_hold() );
    }
}

TEST_CASE( "sleeping_follower_stays_asleep_when_left_behind", "[npc][behavior][sleep]" )
{
    sleep_setup();
    npc &guy = tired_follower( leader_pos + tripoint::east, 400 );
    make_asleep_on_bed( guy );
    guy.set_sleepiness( 400 );
    const tripoint_bub_ms bed = guy.pos_bub();
    get_avatar().setpos( get_map(), leader_pos + tripoint( 20, 0, 0 ) );
    for( int turn = 0; turn < 15; ++turn ) {
        run_npc_turn( guy );
        CAPTURE( turn );
        CHECK( asleep( guy ) );
        CHECK( guy.pos_bub() == bed );
    }
}

TEST_CASE( "follower_with_assigned_camp_does_not_wander_to_camp_seats", "[npc][camp][behavior]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = tired_follower( leader_pos + tripoint::east, 0 );
    const tripoint_abs_omt omt = project_to<coords::omt>( here.get_abs( leader_pos ) );
    guy.assigned_camp = omt;
    REQUIRE( guy.mission == NPC_MISSION_NULL );

    SECTION( "a camp around the leader, with a sofa in it" ) {
        here.add_camp( omt, "faction_camp" );
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( omt.xy() );
        REQUIRE( bcp );
        basecamp *camp = *bcp;
        camp->define_camp( omt, "faction_base_bare_bones_NPC_camp_0", false );
        camp->set_owner( faction_your_followers );
        camp->set_bb_pos( here.get_abs( leader_pos ) );
        const tripoint_bub_ms sofa = leader_pos + tripoint( 8, 0, 0 );
        here.furn_set( sofa, furn_f_sofa );
        REQUIRE( camp->point_within_camp( project_to<coords::omt>( here.get_abs( sofa ) ) ) );

        for( int turn = 0; turn < 20; ++turn ) {
            run_npc_turn( guy );
            CAPTURE( turn, guy.pos_bub() );
            CHECK( rl_dist( guy.pos_bub(), sofa ) > 2 );
        }
        CHECK( guy.assigned_camp == omt );
        CHECK( dist_to_leader( guy ) <= 4 );
    }
    SECTION( "no camp object" ) {
        run_npc_turns( guy, 5 );
        CHECK( guy.assigned_camp.has_value() );
    }
}

TEST_CASE( "npc_that_cannot_sleep_does_not_try", "[npc][needs][sleep]" )
{
    sleep_setup();
    scoped_weather_override clear_weather( weather_clear );
    npc &guy = stranger( leader_pos + tripoint( 0, -20, 0 ) );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );

    SECTION( "a serum that keeps it awake: sleep is never picked" ) {
        guy.add_effect( effect_learning_serum_main, 2_hours );
        REQUIRE( guy.has_flag( json_flag_CANNOT_SLEEP ) );
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.can_sleep( "" ) == behavior::status_t::failure );
        CHECK( never_settles( guy, 20 ) );
    }
    SECTION( "the serum takes hold while it lies down: it gets up" ) {
        guy.set_stim( 100 );
        guy.set_committed_goal( "go_to_sleep" );
        guy.set_moves( 100 );
        REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
        guy.set_stim( 0 );
        guy.add_effect( effect_learning_serum_main, 2_hours );
        run_npc_turns( guy, 3, []( const npc & n ) {
            return !n.has_effect( effect_lying_down );
        } );
        CHECK_FALSE( guy.has_effect( effect_lying_down ) );
        CHECK( never_settles( guy, 10 ) );
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
    }
    SECTION( "woken, doesn't lie back down" ) {
        make_asleep_on_bed( guy );
        wake_by_noise( guy );
        guy.add_effect( effect_learning_serum_main, 2_hours );
        CHECK( never_settles( guy, 10 ) );
    }
}
