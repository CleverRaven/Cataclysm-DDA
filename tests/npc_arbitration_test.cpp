#include <functional>
#include <initializer_list>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "npc_trip.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const efftype_id effect_sleep( "sleep" );

static const faction_id faction_your_followers( "your_followers" );

static const field_type_str_id field_fd_fire( "fd_fire" );

static const npc_class_id NC_ROBOFAC_INTERCOM_DAY( "NC_ROBOFAC_INTERCOM_DAY" );

static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const tripoint_bub_ms leader_pos( 50, 50, 0 );

static void place_leader()
{
    clear_map_without_vision();
    clear_avatar();
    get_avatar().setpos( get_map(), leader_pos );
}

static npc &spawn_follower( const tripoint_bub_ms &pos )
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
    REQUIRE( guy.is_player_ally() );
    REQUIRE( guy.is_walking_with() );
    return guy;
}

static npc_option_cost trip_to( const tripoint_bub_ms &p, const bool responsive = true )
{
    return npc_option_cost::trip( get_map().get_abs( p ), true, responsive, true );
}

static npc_option_cost in_place( const npc &guy )
{
    return npc_option_cost::in_place( guy.pos_abs() );
}

static void check_verdict( const npc_permission_verdict &v, const npc_permission p,
                           const npc_obligation by )
{
    CAPTURE( permission_name( v.permission ), obligation_name( v.by ) );
    CHECK( v.permission == p );
    CHECK( v.by == by );
}

TEST_CASE( "npc_obligations_judge_options", "[npc][needs][obligation]" )
{
    place_leader();
    on_out_of_scope wake_leader( [] {
        get_avatar().remove_effect( effect_sleep );
    } );

    GIVEN( "a follower two tiles from the leader" ) {
        npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
        THEN( "using things in place is always fine" ) {
            check_verdict( guy.judge_option( in_place( guy ) ), npc_permission::allowed,
                           npc_obligation::none );
        }
        THEN( "a trip inside the follow spacing is allowed" ) {
            check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 3, 0, 0 ) ) ),
                           npc_permission::allowed, npc_obligation::none );
        }
        THEN( "a trip past the spacing is discouraged" ) {
            check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 8, 0, 0 ) ) ),
                           npc_permission::discouraged, npc_obligation::accompany );
        }
        THEN( "a route that leaves the spacing counts as beyond" ) {
            npc_option_cost cost = trip_to( leader_pos + tripoint( 3, 0, 0 ) );
            cost.route_in_reach = false;
            check_verdict( guy.judge_option( cost ), npc_permission::discouraged,
                           npc_obligation::accompany );
        }
        THEN( "resting near an active leader is discouraged" ) {
            check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 2, 0, 0 ), false ) ),
                           npc_permission::discouraged, npc_obligation::accompany );
        }
        THEN( "resting far from the leader is for emergencies" ) {
            check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 8, 0, 0 ), false ) ),
                           npc_permission::emergency_only, npc_obligation::accompany );
        }
        WHEN( "the leader sleeps" ) {
            get_avatar().add_effect( effect_sleep, 8_hours );
            THEN( "resting nearby is allowed" ) {
                check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 2, 0, 0 ), false ) ),
                               npc_permission::allowed, npc_obligation::none );
            }
        }
        WHEN( "follower told to stay awake" ) {
            guy.rules.clear_flag( ally_rule::allow_sleep );
            THEN( "resting waits for exhaustion while the leader is up" ) {
                check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 2, 0, 0 ), false ) ),
                               npc_permission::emergency_only, npc_obligation::stay_awake_rule );
            }
            THEN( "resting waits for dead tired while the leader sleeps" ) {
                get_avatar().add_effect( effect_sleep, 8_hours );
                check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 2, 0, 0 ), false ) ),
                               npc_permission::discouraged, npc_obligation::stay_awake_rule );
            }
        }
        WHEN( "follower told to move somewhere" ) {
            guy.goto_to_this_pos = get_map().get_abs( leader_pos + tripoint( -3, 0, 0 ) );
            THEN( "any trip waits for an emergency" ) {
                check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 3, 0, 0 ) ) ),
                               npc_permission::emergency_only, npc_obligation::goto_order );
            }
            THEN( "using something where it stands is still fine" ) {
                check_verdict( guy.judge_option( in_place( guy ) ), npc_permission::allowed,
                               npc_obligation::none );
            }
            THEN( "an activity that holds it in place counts like a trip" ) {
                npc_option_cost cost = in_place( guy );
                cost.occupies = true;
                check_verdict( guy.judge_option( cost ), npc_permission::emergency_only,
                               npc_obligation::goto_order );
            }
        }
        WHEN( "follower has an old guard post" ) {
            guy.guard_pos = guy.pos_abs();
            THEN( "no post obligation while it follows" ) {
                CHECK_FALSE( guy.is_on_shift() );
                check_verdict( guy.judge_option( trip_to( leader_pos + tripoint( 3, 0, 0 ) ) ),
                               npc_permission::allowed, npc_obligation::none );
            }
        }
    }
    GIVEN( "a round-the-clock guard at its post" ) {
        const tripoint_bub_ms post( 30, 30, 0 );
        npc &guy = spawn_npc( post.xy(), "test_talker" );
        clear_character( guy, true );
        guy.setpos( get_map(), post );
        guy.set_mission( NPC_MISSION_GUARD );
        guy.set_guard_pos( guy.pos_abs() );
        REQUIRE( guy.is_on_shift() );
        THEN( "a trip near the post is allowed" ) {
            check_verdict( guy.judge_option( trip_to( post + tripoint( 3, 0, 0 ) ) ),
                           npc_permission::allowed, npc_obligation::none );
        }
        THEN( "a trip past the post's reach is for emergencies" ) {
            check_verdict( guy.judge_option( trip_to( post + tripoint( 6, 0, 0 ) ) ),
                           npc_permission::emergency_only, npc_obligation::post );
        }
        THEN( "sleeping at post is for emergencies" ) {
            check_verdict( guy.judge_option( trip_to( post, false ) ),
                           npc_permission::emergency_only, npc_obligation::post );
        }
    }
    GIVEN( "a shift worker with a post" ) {
        const tripoint_bub_ms post( 30, 30, 0 );
        npc &guy = spawn_npc( post.xy(), "test_talker" );
        clear_character( guy, true );
        guy.setpos( get_map(), post );
        guy.myclass = NC_ROBOFAC_INTERCOM_DAY;
        guy.guard_pos = guy.pos_abs();
        WHEN( "on shift" ) {
            set_time( calendar::turn_zero + 12_hours );
            THEN( "sleeping is blocked" ) {
                check_verdict( guy.judge_option( trip_to( post, false ) ), npc_permission::blocked,
                               npc_obligation::post );
            }
        }
        WHEN( "off shift" ) {
            set_time( calendar::turn_zero + 20_hours );
            THEN( "nothing holds it" ) {
                check_verdict( guy.judge_option( trip_to( post, false ) ), npc_permission::allowed,
                               npc_obligation::none );
            }
        }
    }
    GIVEN( "an NPC with no obligation" ) {
        npc &guy = spawn_npc( point_bub_ms( 30, 30 ), "test_talker" );
        clear_character( guy, true );
        REQUIRE_FALSE( guy.is_player_ally() );
        THEN( "all options are allowed" ) {
            check_verdict( guy.judge_option( trip_to( tripoint_bub_ms( 40, 30, 0 ), false ) ),
                           npc_permission::allowed, npc_obligation::none );
            CHECK_FALSE( guy.obligation_reach() );
        }
    }
}

TEST_CASE( "npc_need_view_keeps_raw_need", "[npc][needs][obligation]" )
{
    place_leader();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    const npc_option_cost far_trip = trip_to( leader_pos + tripoint( 8, 0, 0 ) );

    SECTION( "thirst below 800 skips a discouraged trip" ) {
        guy.set_thirst( 600 );
        const npc_need_view v = guy.need_view( npc::need_goal_id::drink_water, far_trip );
        CHECK( v.raw == Approx( 0.5f ) );
        CHECK( v.floor == Approx( 800.0f / 1200.0f ) );
        CHECK( v.selection == 0.0f );
        CHECK_FALSE( v.eligible() );
    }
    SECTION( "thirst 800+ takes a discouraged trip at its raw score" ) {
        guy.set_thirst( 900 );
        const npc_need_view v = guy.need_view( npc::need_goal_id::drink_water, far_trip );
        CHECK( v.eligible() );
        CHECK( v.selection == Approx( 0.75f ) );
    }
    SECTION( "each threshold is inclusive" ) {
        const auto eligible_at = [&guy]( const npc::need_goal_id id, const npc_option_cost & cost,
        const int level ) {
            if( id == npc::need_goal_id::drink_water ) {
                guy.set_thirst( level );
            } else {
                guy.set_sleepiness( level );
            }
            CAPTURE( level );
            return guy.need_view( id, cost ).eligible();
        };
        // discouraged: a follower's trip past the spacing, its rest while the leader is active
        CHECK( eligible_at( npc::need_goal_id::drink_water, far_trip, 800 ) );
        CHECK_FALSE( eligible_at( npc::need_goal_id::drink_water, far_trip, 799 ) );
        const npc_option_cost near_rest = trip_to( leader_pos + tripoint( 2, 0, 0 ), false );
        const int dead_tired = static_cast<int>( sleepiness_levels::DEAD_TIRED );
        CHECK( eligible_at( npc::need_goal_id::go_to_sleep, near_rest, dead_tired ) );
        CHECK_FALSE( eligible_at( npc::need_goal_id::go_to_sleep, near_rest, dead_tired - 1 ) );
        // emergency only: rest past the spacing, any trip while an order stands
        const npc_option_cost far_rest = trip_to( leader_pos + tripoint( 8, 0, 0 ), false );
        const int exhausted = static_cast<int>( sleepiness_levels::EXHAUSTED );
        CHECK( eligible_at( npc::need_goal_id::go_to_sleep, far_rest, exhausted ) );
        CHECK_FALSE( eligible_at( npc::need_goal_id::go_to_sleep, far_rest, exhausted - 1 ) );
        guy.goto_to_this_pos = get_map().get_abs( leader_pos + tripoint( -3, 0, 0 ) );
        const npc_option_cost near_trip = trip_to( leader_pos + tripoint( 3, 0, 0 ) );
        CHECK( eligible_at( npc::need_goal_id::drink_water, near_trip, 900 ) );
        CHECK_FALSE( eligible_at( npc::need_goal_id::drink_water, near_trip, 899 ) );
    }
    SECTION( "an unreachable option is no option" ) {
        guy.set_thirst( 900 );
        const npc_need_view v = guy.need_view( npc::need_goal_id::drink_water,
                                               std::optional<npc_option_cost>() );
        CHECK( v.kind == npc_option_kind::none );
        CHECK_FALSE( v.may_consider() );
        CHECK( v.raw == Approx( 0.75f ) );
    }
    SECTION( "best case: may be considered, not a concrete option" ) {
        guy.set_sleepiness( 400 );
        const npc_option_cost best = npc_option_cost::best_case( guy.pos_abs(), false );
        const npc_need_view v = guy.need_view( npc::need_goal_id::go_to_sleep, best );
        CHECK( v.kind == npc_option_kind::best_case );
        CHECK( v.may_consider() );
        CHECK_FALSE( v.eligible() );
    }
}

TEST_CASE( "should_interrupt_rules", "[npc][needs]" )
{
    const auto pursuit = []( const npc_interrupt_cost cost, const float selection,
    const bool permitted = true ) {
        return npc_pursuit{ "go_to_sleep", cost, selection, permitted };
    };

    SECTION( "sleep is never interrupted by another goal" ) {
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::wake_only, 0.3f ),
                                       npc_challenger{ "fight", 1.0f } ) );
    }
    SECTION( "a pursuit the obligations no longer allow ends" ) {
        CHECK( should_interrupt( pursuit( npc_interrupt_cost::costly, 0.3f, false ),
                                 npc_challenger{ "idle", 0.0f } ) );
    }
    SECTION( "combat > needs" ) {
        CHECK( should_interrupt( pursuit( npc_interrupt_cost::costly, 0.3f ),
                                 npc_challenger{ "fight", 1.0f } ) );
    }
    SECTION( "following and orders rank below needs" ) {
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::cheap, 0.3f ),
                                       npc_challenger{ "follow_player", 0.6f } ) );
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::cheap, 0.3f ),
                                       npc_challenger{ "goto_ordered_position", 0.65f } ) );
    }
    SECTION( "margins grow with what the pursuit has invested" ) {
        CHECK( should_interrupt( pursuit( npc_interrupt_cost::cheap, 0.3f ),
                                 npc_challenger{ "drink_water", 0.46f } ) );
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::cheap, 0.3f ),
                                       npc_challenger{ "drink_water", 0.44f } ) );
        CHECK( should_interrupt( pursuit( npc_interrupt_cost::stalled, 0.3f ),
                                 npc_challenger{ "drink_water", 0.61f } ) );
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::stalled, 0.3f ),
                                       npc_challenger{ "drink_water", 0.59f } ) );
        // challengers below critical level, so only the margin decides
        CHECK( should_interrupt( pursuit( npc_interrupt_cost::costly, 0.2f ),
                                 npc_challenger{ "drink_water", 0.66f } ) );
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::costly, 0.2f ),
                                       npc_challenger{ "drink_water", 0.64f } ) );
    }
    SECTION( "the same goal never interrupts itself" ) {
        CHECK_FALSE( should_interrupt( pursuit( npc_interrupt_cost::cheap, 0.3f ),
                                       npc_challenger{ "go_to_sleep", 0.9f } ) );
    }
    SECTION( "a critical need interrupts whatever the pursuit cost" ) {
        for( const npc_interrupt_cost cost : {
                 npc_interrupt_cost::cheap, npc_interrupt_cost::stalled, npc_interrupt_cost::costly
             } ) {
            CAPTURE( static_cast<int>( cost ) );
            CHECK( should_interrupt( pursuit( cost, 0.7f ), npc_challenger{ "drink_water", 0.8f } ) );
        }
    }
}

static void wall_row( const int y, const int x_from, const int x_to, const std::vector<int> &gaps )
{
    map &here = get_map();
    for( int x = x_from; x <= x_to; ++x ) {
        if( std::find( gaps.begin(), gaps.end(), x ) == gaps.end() ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_wall_glass );
        }
    }
}

static void wall_column( const int x, const int y_from, const int y_to,
                         const std::vector<int> &gaps )
{
    map &here = get_map();
    for( int y = y_from; y <= y_to; ++y ) {
        if( std::find( gaps.begin(), gaps.end(), y ) == gaps.end() ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_wall_glass );
        }
    }
}

TEST_CASE( "trip_cost_route_policy", "[npc][movement][obligation]" )
{
    place_leader();
    map &here = get_map();

    SECTION( "starting inside, a route that stays inside is in reach" ) {
        npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
        const tripoint_bub_ms water = leader_pos + tripoint( 0, 3, 0 );
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost =
            guy.trip_cost( npc_trip_spec::within_range( water, 1, true ), true );
        REQUIRE( cost );
        CHECK( cost->route_in_reach );
    }
    SECTION( "starting inside, a detour outside leaves the reach" ) {
        npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
        const tripoint_bub_ms water = leader_pos + tripoint( 2, 4, 0 );
        here.ter_set( water, ter_t_water_sh );
        // only way through is 6 tiles from the leader, outside the spacing of 4
        wall_row( leader_pos.y() + 3, leader_pos.x() - 12, leader_pos.x() + 12, { leader_pos.x() + 6 } );
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost =
            guy.trip_cost( npc_trip_spec::within_range( water, 1, true ), true );
        REQUIRE( cost );
        CHECK_FALSE( cost->route_in_reach );
    }
    SECTION( "starting outside, walking in is in reach" ) {
        npc &guy = spawn_follower( leader_pos + tripoint( 7, 0, 0 ) );
        const tripoint_bub_ms water = leader_pos + tripoint( 3, 0, 0 );
        here.ter_set( water, ter_t_water_sh );
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost =
            guy.trip_cost( npc_trip_spec::within_range( water, 1, true ), true );
        REQUIRE( cost );
        CHECK( cost->route_in_reach );
    }
    SECTION( "starting outside, a route that first heads further away leaves the reach" ) {
        npc &guy = spawn_follower( leader_pos + tripoint( 7, 0, 0 ) );
        const tripoint_bub_ms water = leader_pos + tripoint( 3, 0, 0 );
        here.ter_set( water, ter_t_water_sh );
        // only way in is 11 from the leader, further than the NPC's 7
        wall_column( leader_pos.x() + 6, leader_pos.y() - 12, leader_pos.y() + 12,
        { leader_pos.y() + 10 } );
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost =
            guy.trip_cost( npc_trip_spec::within_range( water, 1, true ), true );
        REQUIRE( cost );
        CHECK_FALSE( cost->route_in_reach );
    }
}

TEST_CASE( "approach_bound_holds_for_the_walked_trip", "[npc][movement][obligation]" )
{
    place_leader();
    map &here = get_map();
    // trigdist truncates: the gap and the start are both 8 from the leader
    npc &guy = spawn_follower( leader_pos + tripoint( 8, 0, 0 ) );
    // only way in is a gap south: the walk closes in, bends outward to it, then enters
    wall_column( leader_pos.x() + 4, leader_pos.y() - 10, leader_pos.y() + 10,
    { leader_pos.y() + 7 } );
    const tripoint_bub_ms water = leader_pos + tripoint( 1, 3, 0 );
    here.ter_set( water, ter_t_water_sh );
    here.build_map_cache( 0 );
    const tripoint_abs_ms leader_abs = here.get_abs( leader_pos );
    REQUIRE( rl_dist( guy.pos_abs(), leader_abs ) == 8 );
    REQUIRE( rl_dist( here.get_abs( leader_pos + tripoint( 4, 7, 0 ) ), leader_abs ) <= 8 );
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );

    int closest = rl_dist( guy.pos_abs(), leader_abs );
    bool bent_outward = false;
    for( int turn = 0; turn < 30; ++turn ) {
        const std::optional<npc_option_cost> cost = guy.trip_cost( spec, true );
        REQUIRE( cost );
        CAPTURE( turn, guy.pos_bub() );
        CHECK( cost->route_in_reach );
        if( !cost->needs_walk ) {
            break;
        }
        guy.set_moves( 100 );
        REQUIRE( guy.move_to_and_verify( spec ) );
        const int d = rl_dist( guy.pos_abs(), leader_abs );
        bent_outward = bent_outward || d > closest;
        closest = std::min( closest, d );
        calendar::turn += 1_turns;
    }
    CHECK( bent_outward );
    CHECK( square_dist( guy.pos_bub(), water ) <= 1 );
}

TEST_CASE( "self_care_step_stays_in_reach_around_a_fire", "[npc][movement][obligation]" )
{
    place_leader();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 3, -2, 0 ) );
    const tripoint_bub_ms water = leader_pos + tripoint( 3, 2, 0 );
    here.ter_set( water, ter_t_water_sh );
    here.build_map_cache( 0 );
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );
    guy.set_moves( 100 );
    REQUIRE( guy.move_to_and_verify( spec ) );
    REQUIRE_FALSE( guy.path.empty() );
    const tripoint_bub_ms next = guy.path.front();
    here.add_field( next, field_fd_fire, 1, 10_minutes );
    for( int turn = 0; turn < 12 && square_dist( guy.pos_bub(), water ) > 1; ++turn ) {
        calendar::turn += 1_turns;
        guy.set_moves( 100 );
        guy.move_to_and_verify( spec );
        CAPTURE( turn, guy.pos_bub() );
        CHECK( guy.pos_bub() != next );
        CHECK( guy.obligation_reach()->contains( guy.pos_abs() ) );
    }
    CHECK( square_dist( guy.pos_bub(), water ) <= 1 );
}

TEST_CASE( "same_turn_replacement_hands_back", "[npc][movement][obligation]" )
{
    place_leader();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    // two gaps south: one below the NPC inside the spacing, one 7 from the leader outside
    wall_row( leader_pos.y() + 1, leader_pos.x() - 12, leader_pos.x() + 12,
    { leader_pos.x() + 2, leader_pos.x() + 7 } );
    const tripoint_bub_ms water = leader_pos + tripoint( 2, 3, 0 );
    here.ter_set( water, ter_t_water_sh );
    here.build_map_cache( 0 );
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );
    const tripoint_bub_ms inner_gap = leader_pos + tripoint( 2, 1, 0 );

    const std::optional<npc_option_cost> cost = guy.trip_cost( spec, true );
    REQUIRE( cost );
    REQUIRE( cost->route_in_reach );
    const std::optional<npc_trip> judged = guy.resolve_trip( spec );
    REQUIRE( judged );
    REQUIRE( judged->path.front() == inner_gap );

    // fire in the inner gap the same turn: only the outer way is left
    here.add_field( inner_gap, field_fd_fire, 1, 10_minutes );
    const tripoint_bub_ms before = guy.pos_bub();
    guy.set_moves( 100 );
    CHECK_FALSE( guy.move_to_and_verify( spec ) );
    CHECK( guy.pos_bub() == before );
    CHECK( guy.path.empty() );
}
