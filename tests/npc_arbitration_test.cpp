#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "behavior.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "character_oracle.h"
#include "clzones.h"
#include "coordinates.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "monster.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "npc_trip.h"
#include "npc_turn_helpers.h"
#include "options_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

static const activity_id ACT_FORAGE( "ACT_FORAGE" );
static const activity_id ACT_HARVEST( "ACT_HARVEST" );

static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_sleep( "sleep" );

static const faction_id faction_your_followers( "your_followers" );

static const field_type_str_id field_fd_fire( "fd_fire" );

static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_makeshift_bed( "f_makeshift_bed" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_orange( "orange" );

static const mtype_id mon_zombie( "mon_zombie" );

static const npc_class_id NC_ROBOFAC_INTERCOM_DAY( "NC_ROBOFAC_INTERCOM_DAY" );

static const string_id<behavior::node_t> behavior_node_t_npc_decision( "npc_decision" );

static const ter_str_id ter_t_tree_apple( "t_tree_apple" );
static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const trait_id trait_RETURN_TO_START_POS( "RETURN_TO_START_POS" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static const zone_type_id zone_type_NPC_NO_GO( "NPC_NO_GO" );

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
    SECTION( "a yielding pursuit steps aside for work, not for idleness" ) {
        const npc_pursuit stalled{ "go_to_sleep", npc_interrupt_cost::costly, 0.6f, true, true };
        CHECK( should_interrupt( stalled, npc_challenger{ "return_to_guard_pos", 0.5f } ) );
        CHECK_FALSE( should_interrupt( stalled, npc_challenger{ "idle", 0.0f, true } ) );
        CHECK_FALSE( should_interrupt( stalled, npc_challenger{ "go_to_sleep", 0.6f } ) );
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

// candidate scans need sight and daylight; 20 C and no camp keep warmth and camp water out
static void drink_setup()
{
    clear_map_with_vision();
    clear_avatar();
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    get_avatar().setpos( get_map(), leader_pos );
}

static void settle_body( npc &guy, const int thirst )
{
    guy.set_hunger( 0 );
    guy.set_thirst( thirst );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    REQUIRE( guy.needs_food() );
}

static void put_water( const tripoint_bub_ms &p )
{
    get_map().ter_set( p, ter_t_water_sh );
}

static bool drank( const npc &guy )
{
    return guy.stomach.get_water() > 0_ml;
}

static int dist_to_leader( const npc &guy )
{
    return rl_dist( guy.pos_abs(), get_avatar().pos_abs() );
}

TEST_CASE( "committed_need_yields_to_goto_order", "[npc][needs][water][obligation]" )
{
    drink_setup();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    settle_body( guy, 600 );
    put_water( leader_pos + tripoint( 2, 3, 0 ) );
    get_map().build_map_cache( 0 );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "drink_water" );
    REQUIRE( guy.get_water_plan().active() );

    const tripoint_bub_ms order = leader_pos + tripoint( -3, 0, 0 );
    guy.goto_to_this_pos = get_map().get_abs( order );
    run_npc_turns( guy, 10, [&order]( const npc & n ) {
        return n.pos_bub() == order;
    } );
    CHECK( guy.pos_bub() == order );
    CHECK( guy.get_committed_goal() != "drink_water" );
}

TEST_CASE( "next_goal_after_completion_is_adopted", "[npc][needs][water][obligation]" )
{
    drink_setup();
    npc &guy = spawn_follower( leader_pos + tripoint( 7, 0, 0 ) );
    settle_body( guy, 0 );
    put_water( leader_pos + tripoint( 3, 2, 0 ) );
    get_map().build_map_cache( 0 );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "follow_player" );
    guy.set_thirst( 600 );

    bool adopted = false;
    for( int turn = 0; turn < 10 && !adopted; ++turn ) {
        const time_point before = calendar::turn;
        run_npc_turn( guy );
        if( guy.get_committed_goal() != "follow_player" ) {
            // follow_player completes and the next goal is adopted in the same turn
            CHECK( guy.get_committed_goal() == "drink_water" );
            CHECK( guy.get_intention().since == before );
            adopted = true;
        }
    }
    CHECK( adopted );
}

TEST_CASE( "bt_score_matches_current_need_urgency", "[npc][needs]" )
{
    drink_setup();
    npc &guy = spawn_npc( point_bub_ms( 30, 30 ), "test_talker" );
    clear_character( guy, true );
    settle_body( guy, 700 );
    guy.i_add( item( itype_orange ) );
    behavior::character_oracle_t oracle( &guy );
    behavior::tree tree;
    tree.add( &behavior_node_t_npc_decision.obj() );
    const std::pair<std::string, float> picked = tree.tick_full( &oracle );
    REQUIRE( picked.first == "drink_water" );
    CHECK( picked.second == Approx( guy.current_need_urgency( npc::need_goal_id::drink_water ) ) );
}

TEST_CASE( "committed_drink_yields_when_leader_departs", "[npc][needs][water][obligation]" )
{
    drink_setup();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    settle_body( guy, 600 );
    put_water( leader_pos + tripoint( 2, 3, 0 ) );
    get_map().build_map_cache( 0 );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "drink_water" );

    get_avatar().setpos( get_map(), leader_pos + tripoint( -8, 0, 0 ) );
    run_npc_turn( guy );
    CHECK( guy.get_committed_goal() == "follow_player" );
    CHECK_FALSE( guy.get_water_plan().active() );
    for( int turn = 0; turn < 5; ++turn ) {
        run_npc_turn( guy );
        CHECK( guy.get_committed_goal() != "drink_water" );
    }
}

TEST_CASE( "follower_drinks_within_reach_and_keeps_following", "[npc][needs][water][behavior]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    settle_body( guy, 600 );
    const tripoint_bub_ms water = leader_pos + tripoint( 2, 3, 0 );
    put_water( water );

    SECTION( "water inside the spacing" ) {
        here.build_map_cache( 0 );
        int farthest = 0;
        run_npc_turns( guy, 30, [&farthest]( const npc & n ) {
            farthest = std::max( farthest, dist_to_leader( n ) );
            return drank( n );
        } );
        CHECK( drank( guy ) );
        CHECK( farthest <= 4 );
        WHEN( "the leader walks on" ) {
            guy.stomach.empty();
            guy.set_thirst( 0 );
            get_avatar().setpos( here, leader_pos + tripoint( 12, 0, 0 ) );
            run_npc_turns( guy, 30, []( const npc & n ) {
                return dist_to_leader( n ) <= 4;
            } );
            THEN( "it follows" ) {
                CHECK( dist_to_leader( guy ) <= 4 );
            }
        }
    }
    SECTION( "the only way to the water leaves the spacing" ) {
        // glass keeps the water in sight; the gap is 8 from the leader
        wall_row( leader_pos.y() + 2, leader_pos.x() - 14, leader_pos.x() + 14, { leader_pos.x() + 8 } );
        here.build_map_cache( 0 );
        REQUIRE( guy.sees( here, water ) );
        const std::optional<npc_trip> trip =
            guy.resolve_trip( npc_trip_spec::within_range( water, 1, true ) );
        REQUIRE( trip );
        REQUIRE_FALSE( trip->route_in_reach );
        int farthest = 0;
        for( int turn = 0; turn < 20; ++turn ) {
            run_npc_turn( guy );
            farthest = std::max( farthest, dist_to_leader( guy ) );
        }
        CHECK( farthest <= 4 );
        CHECK_FALSE( drank( guy ) );
        CHECK_FALSE( guy.get_water_plan().active() );
        CHECK( guy.last_complaint( "water_out_of_reach" ) );
    }
}

TEST_CASE( "drink_verdict_matches_selected_source", "[npc][needs][water][obligation]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    settle_body( guy, 1000 );
    const tripoint_bub_ms near_water = leader_pos + tripoint( 2, 2, 0 );
    const tripoint_bub_ms far_water = leader_pos + tripoint( 8, 0, 0 );
    put_water( near_water );
    put_water( far_water );
    here.build_map_cache( 0 );

    SECTION( "near source failed before: the far one is an override" ) {
        guy.failed_targets_for( npc::need_goal_id::drink_water ).insert( here.get_abs( near_water ) );
        run_npc_turn( guy );
        REQUIRE( guy.get_water_plan().active() );
        CHECK( guy.get_water_plan().target == here.get_abs( far_water ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
        CHECK( guy.get_intention().verdict.by == npc_obligation::accompany );
        CHECK( guy.last_complaint( "need_override_water" ) );
    }
    SECTION( "near source is fine: no override" ) {
        run_npc_turn( guy );
        REQUIRE( guy.get_water_plan().active() );
        CHECK( guy.get_water_plan().target == here.get_abs( near_water ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::allowed );
        CHECK_FALSE( guy.last_complaint( "need_override_water" ) );
    }
}

TEST_CASE( "retained_trip_records_its_override", "[npc][needs][water][obligation]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, -1, 0 ) );
    settle_body( guy, 1000 );
    const tripoint_bub_ms water = leader_pos + tripoint( 2, 3, 0 );
    put_water( water );
    here.build_map_cache( 0 );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "drink_water" );
    REQUIRE( guy.get_intention().verdict.permission == npc_permission::allowed );
    REQUIRE_FALSE( guy.last_complaint( "need_override_water" ) );

    // leader walks off: the trip's now beyond the spacing, but thirst is past insist
    get_avatar().setpos( here, leader_pos + tripoint( -8, 0, 0 ) );
    run_npc_turn( guy );
    REQUIRE( guy.get_water_plan().active() );
    CHECK( guy.get_water_plan().target == here.get_abs( water ) );
    CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
    const std::optional<time_point> said = guy.last_complaint( "need_override_water" );
    REQUIRE( said );
    if( !drank( guy ) ) {
        run_npc_turn( guy );
        CHECK( guy.last_complaint( "need_override_water" ) == said );
    }
}

TEST_CASE( "follower_asks_before_leaving_for_far_water", "[npc][needs][water][behavior]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 3, 0, 0 ) );
    put_water( leader_pos + tripoint( 8, 0, 0 ) );
    here.build_map_cache( 0 );

    WHEN( "thirsty" ) {
        settle_body( guy, 600 );
        int farthest = 0;
        for( int turn = 0; turn < 20; ++turn ) {
            run_npc_turn( guy );
            farthest = std::max( farthest, dist_to_leader( guy ) );
        }
        THEN( "it stays and complains of thirst" ) {
            CHECK( farthest <= 4 );
            CHECK_FALSE( drank( guy ) );
            CHECK( guy.last_complaint( "water_out_of_reach" ) );
            CHECK( guy.last_complaint( "thirst" ) == guy.last_complaint( "water_out_of_reach" ) );
            const npc_need_view water = guy.best_need_view( npc::need_goal_id::drink_water );
            CHECK( water.verdict.permission == npc_permission::discouraged );
        }
    }
    WHEN( "badly dehydrated" ) {
        settle_body( guy, 1000 );
        run_npc_turn( guy );
        THEN( "it says so and goes" ) {
            CHECK( guy.get_committed_goal() == "drink_water" );
            CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
            CHECK( guy.last_complaint( "need_override_water" ) );
            run_npc_turns( guy, 20, drank );
            CHECK( drank( guy ) );
        }
    }
}

TEST_CASE( "guard_drinks_without_leaving_post", "[npc][needs][water][guard]" )
{
    drink_setup();
    map &here = get_map();
    const tripoint_bub_ms post( 30, 30, 0 );
    npc &guy = spawn_npc( post.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, post );
    guy.set_mission( NPC_MISSION_GUARD );
    guy.set_guard_pos( guy.pos_abs() );
    REQUIRE( guy.is_on_shift() );
    const auto from_post = [&]( const npc & n ) {
        return rl_dist( n.pos_abs(), here.get_abs( post ) );
    };

    SECTION( "water near the post" ) {
        settle_body( guy, 600 );
        put_water( post + tripoint( 3, 0, 0 ) );
        here.build_map_cache( 0 );
        int farthest = 0;
        run_npc_turns( guy, 15, [&]( const npc & n ) {
            farthest = std::max( farthest, from_post( n ) );
            return drank( n );
        } );
        CHECK( drank( guy ) );
        CHECK( farthest <= 4 );
        run_npc_turns( guy, 15, [&]( const npc & n ) {
            return n.pos_bub() == post;
        } );
        CHECK( guy.pos_bub() == post );
    }
    SECTION( "water too far for a quick trip" ) {
        settle_body( guy, 600 );
        put_water( post + tripoint( 6, 0, 0 ) );
        here.build_map_cache( 0 );
        for( int turn = 0; turn < 10; ++turn ) {
            run_npc_turn( guy );
            CHECK( guy.pos_bub() == post );
        }
        CHECK_FALSE( drank( guy ) );
    }
    SECTION( "too far, but badly dehydrated" ) {
        settle_body( guy, 1000 );
        put_water( post + tripoint( 6, 0, 0 ) );
        here.build_map_cache( 0 );
        run_npc_turns( guy, 15, drank );
        CHECK( drank( guy ) );
        CHECK( guy.last_complaint( "need_override_water" ) );
    }
}

TEST_CASE( "npc_water_out_of_reach_is_not_planned", "[npc][needs][water]" )
{
    drink_setup();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    settle_body( guy, 600 );
    put_water( leader_pos + tripoint( 8, 0, 0 ) );
    get_map().build_map_cache( 0 );
    guy.set_moves( 100 );
    CHECK( guy.execute_need_goal( "drink_water" ) == npc::need_result::impossible );
    CHECK_FALSE( guy.get_water_plan().active() );
}

TEST_CASE( "water_just_outside_reach_is_used_from_inside", "[npc][needs][water][obligation]" )
{
    drink_setup();
    npc &guy = spawn_follower( leader_pos + tripoint( 3, 0, 0 ) );
    settle_body( guy, 600 );
    // water's 5 from the leader; the tile next to it is 4
    put_water( leader_pos + tripoint( 5, 0, 0 ) );
    get_map().build_map_cache( 0 );
    int farthest = 0;
    run_npc_turns( guy, 10, [&farthest]( const npc & n ) {
        farthest = std::max( farthest, dist_to_leader( n ) );
        return drank( n );
    } );
    CHECK( drank( guy ) );
    CHECK( farthest <= 4 );
}

TEST_CASE( "follower_finishes_in_reach_drink_when_leader_out_of_sight",
           "[npc][needs][water][behavior]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 3, 0, 0 ) );
    settle_body( guy, 600 );
    // opaque wall: out of sight, follow_distance() drops to 2 but the spacing stays 4
    for( int y = -1; y <= 1; ++y ) {
        here.ter_set( leader_pos + tripoint( 1, y, 0 ), ter_t_wall );
    }
    put_water( leader_pos + tripoint( 3, 2, 0 ) );
    here.build_map_cache( 0 );
    REQUIRE_FALSE( guy.sees( here, get_avatar() ) );
    REQUIRE( guy.follow_distance() == 2 );
    run_npc_turns( guy, 10, drank );
    CHECK( drank( guy ) );
    run_npc_turns( guy, 20, []( const npc & n ) {
        return dist_to_leader( n ) <= 2;
    } );
    CHECK( dist_to_leader( guy ) <= 2 );
}

TEST_CASE( "changed_route_is_judged_again_before_moving", "[npc][needs][water][obligation]" )
{
    drink_setup();
    map &here = get_map();
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    const tripoint_bub_ms water = leader_pos + tripoint( 0, 3, 0 );
    put_water( water );
    // glass ring around the water with a gap near the leader and one past the reach
    const tripoint_bub_ms near_gap = water + tripoint( 0, -2, 0 );
    const tripoint_bub_ms far_gap = water + tripoint( 0, 2, 0 );
    for( const tripoint_bub_ms &p : here.points_in_radius( water, 2 ) ) {
        if( square_dist( p, water ) == 2 && p != near_gap && p != far_gap ) {
            here.ter_set( p, ter_t_wall_glass );
        }
    }
    const int thirst = GENERATE( 600, 1000 );
    CAPTURE( thirst );
    settle_body( guy, thirst );
    here.build_map_cache( 0 );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "drink_water" );
    REQUIRE( guy.get_intention().verdict.permission == npc_permission::allowed );
    REQUIRE( std::find( guy.path.begin(), guy.path.end(), near_gap ) != guy.path.end() );

    here.ter_set( near_gap, ter_t_wall_glass );
    here.build_map_cache( 0 );
    const tripoint_bub_ms before = guy.pos_bub();
    run_npc_turn( guy );
    if( thirst == 600 ) {
        CHECK( guy.pos_bub() == before );
        CHECK( guy.get_committed_goal() != "drink_water" );
        CHECK_FALSE( guy.get_water_plan().active() );
        CHECK( guy.last_complaint( "water_out_of_reach" ) );
    } else {
        CHECK( guy.get_committed_goal() == "drink_water" );
        CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
        CHECK( guy.last_complaint( "need_override_water" ) );
        run_npc_turns( guy, 30, drank );
        CHECK( drank( guy ) );
    }
}

TEST_CASE( "follower_outside_moving_vehicle_holds_back", "[npc][needs][water][vehicle]" )
{
    drink_setup();
    clear_vehicles();
    map &here = get_map();
    Character &pc = get_player_character();
    pc.setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, pc.pos_bub(), 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    here.board_vehicle( pc.pos_bub(), &pc );
    REQUIRE( pc.in_vehicle );
    clear_creatures();
    veh->velocity = 1000;
    npc &guy = spawn_follower( tripoint_bub_ms( 58, 50, 0 ) );
    put_water( guy.pos_bub() + tripoint( 0, 3, 0 ) );
    here.build_map_cache( 0 );
    const std::optional<npc_reach> reach = guy.obligation_reach();
    REQUIRE( reach );
    REQUIRE( reach->aboard == nullptr );

    WHEN( "thirsty" ) {
        settle_body( guy, 600 );
        run_npc_turns( guy, 10 );
        THEN( "it holds back and complains of thirst" ) {
            CHECK_FALSE( drank( guy ) );
            CHECK_FALSE( guy.get_water_plan().active() );
            CHECK( guy.last_complaint( "water_out_of_reach" ) );
        }
    }
    WHEN( "badly dehydrated" ) {
        settle_body( guy, 1000 );
        run_npc_turns( guy, 20, drank );
        THEN( "it drinks and says so" ) {
            CHECK( drank( guy ) );
            CHECK( guy.last_complaint( "need_override_water" ) );
        }
    }
}

TEST_CASE( "harvesting_npc_answers_order_and_leader_departure", "[npc][needs][water][obligation]" )
{
    drink_setup();
    map &here = get_map();
    // apple trees bear fruit in autumn
    set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );
    npc &guy = spawn_follower( leader_pos + tripoint( 2, 0, 0 ) );
    guy.worn.wear_item( guy, item( itype_backpack ), false, false );
    settle_body( guy, 600 );
    const tripoint_bub_ms tree = leader_pos + tripoint( 3, 0, 0 );
    here.ter_set( tree, ter_t_tree_apple );
    here.build_map_cache( 0 );
    REQUIRE( here.is_harvestable( tree ) );

    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "drink_water" );
    const npc::need_plan &plan = guy.get_water_plan();
    REQUIRE( plan.active() );
    REQUIRE( plan.source_kind == npc_short_term_cache::need_source::harvestable );
    REQUIRE( plan.target == here.get_abs( tree ) );
    const std::optional<npc_option_cost> cost =
        guy.water_option_cost( { plan.source_kind, plan.target, 0.0f } );
    REQUIRE( cost );
    REQUIRE( cost->occupies );
    REQUIRE( ( guy.activity.id() == ACT_HARVEST || guy.activity.id() == ACT_FORAGE ) );

    SECTION( "an order to move" ) {
        const tripoint_bub_ms order = leader_pos + tripoint( -3, 0, 0 );
        guy.goto_to_this_pos = here.get_abs( order );
        run_npc_turn( guy );
        CHECK_FALSE( guy.activity.id() == ACT_HARVEST );
        CHECK_FALSE( guy.activity.id() == ACT_FORAGE );
        run_npc_turns( guy, 10, [&order]( const npc & n ) {
            return n.pos_bub() == order;
        } );
        CHECK( guy.pos_bub() == order );
    }
    SECTION( "leader walks off" ) {
        get_avatar().setpos( here, leader_pos + tripoint( -10, 0, 0 ) );
        run_npc_turn( guy );
        CHECK_FALSE( guy.activity.id() == ACT_HARVEST );
        CHECK_FALSE( guy.activity.id() == ACT_FORAGE );
        CHECK( guy.get_committed_goal() != "drink_water" );
    }
    SECTION( "leader walks off while it is badly dehydrated" ) {
        guy.set_thirst( 1000 );
        get_avatar().setpos( here, leader_pos + tripoint( -10, 0, 0 ) );
        run_npc_turn( guy );
        CHECK( ( guy.activity.id() == ACT_HARVEST || guy.activity.id() == ACT_FORAGE ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
        const std::optional<time_point> said = guy.last_complaint( "need_override_water" );
        REQUIRE( said );
        run_npc_turn( guy );
        CHECK( guy.last_complaint( "need_override_water" ) == said );
    }
    SECTION( "no pursuit recorded" ) {
        // as after an address_needs() harvest or a load
        const auto forget_pursuit = [&guy]() {
            guy.set_committed_goal( "" );
            guy.plan_for( npc::need_goal_id::drink_water ).clear();
        };
        WHEN( "the leader stays" ) {
            forget_pursuit();
            run_npc_turn( guy );
            THEN( "it keeps harvesting" ) {
                CHECK( ( guy.activity.id() == ACT_HARVEST || guy.activity.id() == ACT_FORAGE ) );
            }
        }
        WHEN( "the leader walks off" ) {
            forget_pursuit();
            get_avatar().setpos( here, leader_pos + tripoint( -10, 0, 0 ) );
            run_npc_turn( guy );
            THEN( "it stops" ) {
                CHECK_FALSE( guy.activity.id() == ACT_HARVEST );
                CHECK_FALSE( guy.activity.id() == ACT_FORAGE );
            }
        }
        WHEN( "the game is saved and loaded, then the leader walks off" ) {
            std::ostringstream os;
            JsonOut jsout( os );
            guy.serialize( jsout );
            forget_pursuit();
            guy.cancel_activity();
            // loaded NPC starts without an id; the save brings it back
            guy.setID( character_id(), true );
            guy.deserialize( json_loader::from_string( os.str() ).get_object() );
            REQUIRE( ( guy.activity.id() == ACT_HARVEST || guy.activity.id() == ACT_FORAGE ) );
            get_avatar().setpos( here, leader_pos + tripoint( -10, 0, 0 ) );
            run_npc_turn( guy );
            THEN( "it stops" ) {
                CHECK_FALSE( guy.activity.id() == ACT_HARVEST );
                CHECK_FALSE( guy.activity.id() == ACT_FORAGE );
            }
        }
    }
}

TEST_CASE( "idle_harvest_keeps_to_obligations", "[npc][needs][water][obligation]" )
{
    drink_setup();
    map &here = get_map();
    set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );
    const auto harvesting = []( const npc & n ) {
        return n.activity.id() == ACT_HARVEST || n.activity.id() == ACT_FORAGE;
    };
    const auto follower_by_a_tree = [&here]( const tripoint_bub_ms & pos ) -> npc & {
        npc &guy = spawn_follower( pos );
        guy.worn.wear_item( guy, item( itype_backpack ), false, false );
        settle_body( guy, 600 );
        here.ter_set( pos + tripoint::east, ter_t_tree_apple );
        here.build_map_cache( 0 );
        REQUIRE( here.is_harvestable( pos + tripoint::east ) );
        return guy;
    };

    SECTION( "inside the reach: it harvests" ) {
        npc &guy = follower_by_a_tree( leader_pos + tripoint( 2, 0, 0 ) );
        guy.address_needs( 0 );
        CHECK( harvesting( guy ) );
    }
    SECTION( "outside the reach: it doesn't start" ) {
        npc &guy = follower_by_a_tree( leader_pos + tripoint( 10, 0, 0 ) );
        REQUIRE_FALSE( guy.obligation_reach()->contains( guy.pos_abs() ) );
        guy.address_needs( 0 );
        CHECK_FALSE( harvesting( guy ) );
    }
}

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

static npc &tired_follower( const tripoint_bub_ms &pos, const int sleepiness )
{
    npc &guy = spawn_follower( pos );
    settle_body( guy, 0 );
    guy.set_sleepiness( sleepiness );
    return guy;
}

static int dist_to( const npc &guy, const tripoint_bub_ms &p )
{
    return rl_dist( guy.pos_bub(), p );
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

static void put_leader_to_sleep()
{
    get_avatar().add_effect( effect_sleep, 2_hours );
}

static void wake_leader()
{
    get_avatar().remove_effect( effect_sleep );
}

static void forbid_area( const npc &guy, const tripoint_abs_ms &center, const int radius )
{
    zone_manager &mgr = zone_manager::get_manager();
    mgr.add( "no_rest", zone_type_NPC_NO_GO, guy.get_fac_id(), false, true,
             center + tripoint( -radius, -radius, 0 ), center + tripoint( radius, radius, 0 ) );
    mgr.cache_data();
}

TEST_CASE( "tired_follower_stays_with_walking_leader", "[npc][needs][sleep][behavior]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = tired_follower( leader_pos + tripoint::east, 250 );
    here.furn_set( leader_pos + tripoint( 15, 0, 0 ), furn_f_bed );

    SECTION( "leader awake and walking" ) {
        for( int turn = 0; turn < 40; ++turn ) {
            if( turn % 2 == 1 ) {
                get_avatar().setpos( here, get_avatar().pos_bub() + tripoint::east );
            }
            run_npc_turn( guy );
            CAPTURE( turn );
            CHECK_FALSE( guy.in_sleep_state() );
            CHECK( guy.get_committed_goal() != "go_to_sleep" );
            CHECK_FALSE( guy.get_sleep_plan().active() );
        }
        CHECK( dist_to_leader( guy ) <= 6 );
    }
    SECTION( "leader asleep" ) {
        put_leader_to_sleep();
        on_out_of_scope wake( wake_leader );
        int farthest = 0;
        run_npc_turns( guy, 20, [&farthest]( const npc & n ) {
            farthest = std::max( farthest, dist_to_leader( n ) );
            return n.in_sleep_state();
        } );
        CHECK( guy.in_sleep_state() );
        CHECK( farthest <= 4 );
    }
}

TEST_CASE( "stay_awake_rule_thresholds_follow_leader_state", "[npc][needs][sleep][behavior]" )
{
    sleep_setup();
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 0 );
    guy.rules.clear_flag( ally_rule::allow_sleep );

    SECTION( "leader asleep" ) {
        put_leader_to_sleep();
        on_out_of_scope wake( wake_leader );
        WHEN( "tired but not dead tired" ) {
            guy.set_sleepiness( 300 );
            THEN( "it keeps watch" ) {
                CHECK( never_settles( guy, 20 ) );
            }
        }
        WHEN( "dead tired" ) {
            guy.set_sleepiness( 400 );
            THEN( "it lies down to sleep" ) {
                CHECK( settles_within( guy, 20 ) );
            }
        }
    }
    SECTION( "leader awake" ) {
        WHEN( "dead tired but not exhausted" ) {
            guy.set_sleepiness( 500 );
            THEN( "it stays awake" ) {
                CHECK( never_settles( guy, 20 ) );
            }
        }
        WHEN( "exhausted" ) {
            guy.set_sleepiness( 600 );
            THEN( "it lies down to sleep" ) {
                CHECK( settles_within( guy, 20 ) );
            }
        }
    }
}

TEST_CASE( "emergency_sleep_overrides_with_explanation", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();

    SECTION( "follower told to stay awake, leader awake" ) {
        npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 600 );
        guy.rules.clear_flag( ally_rule::allow_sleep );
        CHECK( settles_within( guy, 20 ) );
        CHECK( dist_to_leader( guy ) <= 4 );
        CHECK( guy.last_complaint( "need_override_sleep" ) );
    }
    SECTION( "24/7 guard" ) {
        const tripoint_bub_ms post( 30, 30, 0 );
        npc &guy = spawn_npc( post.xy(), "test_talker" );
        clear_character( guy, true );
        guy.setpos( here, post );
        guy.set_mission( NPC_MISSION_GUARD );
        guy.set_guard_pos( guy.pos_abs() );
        settle_body( guy, 0 );
        guy.set_sleepiness( 600 );
        REQUIRE( guy.is_on_shift() );
        here.furn_set( post + tripoint( 8, 0, 0 ), furn_f_bed );
        CHECK( settles_within( guy, 20 ) );
        CHECK( dist_to( guy, post ) <= 4 );
        CHECK( guy.last_complaint( "need_override_sleep" ) );
    }
}

TEST_CASE( "adopted_sleep_records_its_override", "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = tired_follower( leader_pos + tripoint( 6, 0, 0 ), 0 );
    guy.rules.clear_flag( ally_rule::allow_sleep );
    here.furn_set( leader_pos + tripoint::east, furn_f_bed );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "follow_player" );

    guy.set_sleepiness( 600 );
    run_npc_turn( guy );
    CHECK( guy.get_committed_goal() == "go_to_sleep" );
    CHECK( guy.get_intention().verdict.permission == npc_permission::emergency_only );
    CHECK( guy.get_intention().verdict.by == npc_obligation::stay_awake_rule );
    CHECK( guy.last_complaint( "need_override_sleep" ) );
}

TEST_CASE( "boarding_interrupts_bed_seeking", "[npc][behavior][vehicle][sleep]" )
{
    sleep_setup();
    clear_vehicles();
    map &here = get_map();
    const tripoint_bub_ms veh_pos = leader_pos + tripoint( 0, -6, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, veh_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    veh->velocity = 0;
    const tripoint_bub_ms bed = leader_pos + tripoint( 3, 0, 0 );
    here.furn_set( bed, furn_f_bed );
    npc &guy = tired_follower( leader_pos + tripoint( -3, 0, 0 ), 400 );
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    run_npc_turn( guy );
    REQUIRE( guy.get_committed_goal() == "go_to_sleep" );
    REQUIRE( guy.get_sleep_plan().target == here.get_abs( bed ) );
    REQUIRE_FALSE( guy.in_sleep_state() );

    WHEN( "the leader only wakes" ) {
        wake_leader();
        run_npc_turn( guy );
        THEN( "the bed still clears the floor at this tiredness" ) {
            CHECK( guy.get_committed_goal() == "go_to_sleep" );
            CHECK( guy.get_sleep_plan().target == here.get_abs( bed ) );
        }
    }
    WHEN( "leader wakes and boards" ) {
        wake_leader();
        Character &pc = get_player_character();
        pc.setpos( here, veh_pos );
        here.board_vehicle( veh_pos, &pc );
        REQUIRE( pc.in_vehicle );
        // boarding recenters the map; new submaps can bring monsters it'd flee
        clear_creatures();
        run_npc_turns( guy, 15, []( const npc & n ) {
            return n.in_vehicle;
        } );
        THEN( "it gives up the bed and gets in" ) {
            CHECK( guy.get_committed_goal() != "go_to_sleep" );
            CHECK_FALSE( guy.get_sleep_plan().active() );
            CHECK( guy.in_vehicle );
        }
    }
}

TEST_CASE( "shift_worker_does_not_seek_bed_on_shift", "[npc][schedule][sleep]" )
{
    sleep_setup();
    calendar::turn = calendar::turn_zero + 12_hours;
    map &here = get_map();
    const tripoint_bub_ms post( 30, 30, 0 );
    npc &guy = spawn_npc( post.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, post );
    guy.myclass = NC_ROBOFAC_INTERCOM_DAY;
    guy.set_mutation( trait_RETURN_TO_START_POS );
    guy.set_mission( NPC_MISSION_SHOPKEEP );
    guy.set_guard_pos( guy.pos_abs() );
    settle_body( guy, 0 );
    guy.set_sleepiness( 600 );
    REQUIRE( guy.is_on_shift() );
    here.furn_set( post + tripoint( 3, 0, 0 ), furn_f_bed );
    for( int turn = 0; turn < 10; ++turn ) {
        run_npc_turn( guy );
        CAPTURE( turn );
        CHECK( guy.pos_bub() == post );
        CHECK_FALSE( guy.in_sleep_state() );
    }
}

TEST_CASE( "sleep_on_own_bed_with_unreachable_anchor", "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    map &here = get_map();
    // no route reaches the leader
    for( const tripoint_bub_ms &p : here.points_in_radius( leader_pos, 1 ) ) {
        if( p != leader_pos ) {
            here.ter_set( p, ter_t_wall_glass );
        }
    }
    const tripoint_bub_ms bed = leader_pos + tripoint( 3, 0, 0 );
    here.furn_set( bed, furn_f_bed );
    npc &guy = tired_follower( bed, 600 );
    guy.set_stim( 0 );
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    REQUIRE( calendar::turn > calendar::turn_zero + 1_hours );
    run_npc_turns( guy, 20, []( const npc & n ) {
        return n.has_effect( effect_sleep );
    } );
    CHECK( guy.has_effect( effect_sleep ) );
    CHECK( guy.pos_bub() == bed );
}

TEST_CASE( "permitted_worse_bed_beats_impermissible_better_bed", "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    map &here = get_map();
    // bed A walled in, only way in is from outside the spacing
    const tripoint_bub_ms bed_a = leader_pos + tripoint( 3, 0, 0 );
    const tripoint_bub_ms gap = bed_a + tripoint::east;
    for( const tripoint_bub_ms &p : here.points_in_radius( bed_a, 1 ) ) {
        if( p != bed_a && p != gap ) {
            here.ter_set( p, ter_t_wall_glass );
        }
    }
    here.furn_set( bed_a, furn_f_bed );
    const tripoint_bub_ms bed_b = leader_pos + tripoint( 1, 2, 0 );
    here.furn_set( bed_b, furn_f_makeshift_bed );
    npc &guy = tired_follower( leader_pos + tripoint::south_east, 300 );
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    const int comfort_a = guy.evaluate_sleep_spot( bed_a );
    const int comfort_b = guy.evaluate_sleep_spot( bed_b );
    REQUIRE( comfort_a > comfort_b );

    const std::vector<npc::need_candidate> candidates = guy.find_sleep_candidates();
    const auto listed = [&]( const tripoint_bub_ms & p ) {
        return std::any_of( candidates.begin(), candidates.end(),
        [&]( const npc::need_candidate & c ) {
            return c.target == here.get_abs( p );
        } );
    };
    CHECK( listed( bed_b ) );
    CHECK_FALSE( listed( bed_a ) );
    CHECK( settles_within( guy, 20 ) );
    CHECK( guy.pos_bub() == bed_b );
}

TEST_CASE( "no_needs_follower_naps_only_when_leader_rests", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ),
                               static_cast<int>( sleepiness_levels::TIRED ) );
    override_option no_needs( "NO_NPC_FOOD", "true" );
    REQUIRE_FALSE( guy.needs_food() );

    SECTION( "leader walking" ) {
        for( int turn = 0; turn < 40; ++turn ) {
            if( turn % 2 == 1 ) {
                get_avatar().setpos( here, get_avatar().pos_bub() + tripoint::east );
            }
            run_npc_turn( guy );
            CAPTURE( turn );
            CHECK_FALSE( guy.in_sleep_state() );
        }
    }
    SECTION( "leader waits" ) {
        get_avatar().assign_activity( wait_activity_actor( 1_hours ) );
        REQUIRE( get_avatar().can_use_floor_warmth() );
        CHECK( settles_within( guy, 20 ) );
        CHECK( dist_to_leader( guy ) <= 4 );
    }
    SECTION( "told to stay awake, leader asleep" ) {
        guy.rules.clear_flag( ally_rule::allow_sleep );
        put_leader_to_sleep();
        on_out_of_scope wake( wake_leader );
        CHECK( never_settles( guy, 40 ) );
    }
}

TEST_CASE( "passenger_rests_in_far_seat_while_moving", "[npc][needs][sleep][vehicle]" )
{
    sleep_setup();
    clear_vehicles();
    map &here = get_map();
    Character &pc = get_player_character();
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, leader_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    here.board_vehicle( leader_pos, &pc );
    REQUIRE( pc.in_vehicle );
    // the boardable part farthest from the driver
    tripoint_bub_ms seat = leader_pos;
    for( const vpart_reference &part : veh->get_avail_parts( VPFLAG_BOARDABLE ) ) {
        const tripoint_bub_ms p = veh->bub_part_pos( here, part.part() );
        if( rl_dist( p, leader_pos ) > rl_dist( seat, leader_pos ) ) {
            seat = p;
        }
    }
    REQUIRE( rl_dist( seat, leader_pos ) >= 3 );
    // spawned off the vehicle, then seated
    npc &guy = tired_follower( leader_pos + tripoint( 0, 10, 0 ), 400 );
    guy.rules.set_flag( ally_rule::follow_distance_2 );
    guy.setpos( here, seat );
    if( !guy.in_vehicle ) {
        here.board_vehicle( seat, &guy );
    }
    REQUIRE( guy.in_vehicle );
    veh->velocity = 1000;
    const std::optional<npc_reach> reach = guy.obligation_reach();
    REQUIRE( reach );
    REQUIRE( reach->aboard == veh );

    CHECK( settles_within( guy, 20 ) );
    CHECK( reach->contains( guy.pos_abs() ) );
}

TEST_CASE( "vehicle_reach_forbids_ground_shortcut", "[npc][movement][obligation][vehicle]" )
{
    place_leader();
    clear_vehicles();
    map &here = get_map();
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, leader_pos, 0_degrees, 100,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    veh->velocity = 0;
    // front row of the cab: seat, aisle, seat, with an outside door at each end
    const auto at_mount = [&]( const point_rel_ms & mount ) {
        return veh->mount_to_tripoint( &here, mount );
    };
    Character &pc = get_player_character();
    pc.setpos( here, at_mount( point_rel_ms( -2, 0 ) ) );
    here.board_vehicle( pc.pos_bub(), &pc );
    REQUIRE( pc.in_vehicle );
    npc &guy = spawn_follower( leader_pos + tripoint( 0, 10, 0 ) );
    guy.setpos( here, at_mount( point_rel_ms( 0, 2 ) ) );
    if( !guy.in_vehicle ) {
        here.board_vehicle( guy.pos_bub(), &guy );
    }
    REQUIRE( guy.in_vehicle );
    REQUIRE( guy.obligation_reach()->aboard == veh );
    const npc_trip_spec to_seat = npc_trip_spec::stand_on( at_mount( point_rel_ms( 0, 0 ) ), true );

    SECTION( "aisle is free: walk stays aboard" ) {
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost = guy.trip_cost( to_seat, false );
        REQUIRE( cost );
        CHECK( cost->route_in_reach );
    }
    SECTION( "aisle blocked: going round outside leaves the reach" ) {
        // internal door behind the aisle is the other way through; open it so it can
        // be blocked too
        const std::optional<vpart_reference> inner_door =
            here.veh_at( at_mount( point_rel_ms( -1, 1 ) ) ).part_with_feature( VPFLAG_OPENABLE, true );
        REQUIRE( inner_door );
        veh->open( here, inner_door->part_index() );
        int spare = 12;
        for( const tripoint_bub_ms &p : {
                 at_mount( point_rel_ms( 0, 1 ) ), at_mount( point_rel_ms( -1, 1 ) )
             } ) {
            // spawned off the vehicle, then moved in
            npc &blocker = spawn_npc( ( leader_pos + tripoint( 0, spare++, 0 ) ).xy(), "test_talker" );
            blocker.setpos( here, p );
        }
        here.build_map_cache( 0 );
        const std::optional<npc_option_cost> cost = guy.trip_cost( to_seat, false );
        REQUIRE( cost );
        CHECK_FALSE( cost->route_in_reach );
    }
}

TEST_CASE( "rest_cooldown_arms_when_no_spot_is_eligible", "[npc][needs][sleep][obligation]" )
{
    sleep_setup();
    map &here = get_map();
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 300 );
    // the whole reach, own tile included, is NPC_NO_GO
    forbid_area( guy, here.get_abs( leader_pos ), 4 );
    behavior::character_oracle_t oracle( &guy );
    REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
    REQUIRE( guy.find_sleep_candidates().empty() );

    guy.set_moves( 100 );
    CHECK( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::impossible );
    CHECK_FALSE( guy.get_sleep_plan().active() );
    CHECK( guy.rest_cooldown_active() );
    CHECK( oracle.can_sleep( "" ) == behavior::status_t::failure );
}

TEST_CASE( "rest_cooldown_reached_through_normal_dispatch", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 300 );
    forbid_area( guy, here.get_abs( leader_pos ), 4 );
    run_npc_turn( guy );
    CHECK( guy.rest_cooldown_active() );
    CHECK( guy.get_committed_goal().empty() );
    for( int turn = 0; turn < 5; ++turn ) {
        run_npc_turn( guy );
        CHECK_FALSE( guy.get_sleep_plan().active() );
        CHECK( guy.get_committed_goal() != "go_to_sleep" );
    }
}

TEST_CASE( "rest_cooldown_clears_when_its_inputs_change", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 300 );
    forbid_area( guy, here.get_abs( leader_pos ), 4 );
    guy.set_moves( 100 );
    REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::impossible );
    REQUIRE( guy.rest_cooldown_active() );

    SECTION( "spot becomes usable: waits out the minute" ) {
        zone_manager::get_manager().clear();
        zone_manager::get_manager().cache_data();
        CHECK( guy.rest_cooldown_active() );
        calendar::turn += 1_minutes;
        CHECK_FALSE( guy.rest_cooldown_active() );
        CHECK( settles_within( guy, 5 ) );
    }
    SECTION( "leader steps" ) {
        get_avatar().setpos( here, leader_pos + tripoint::south );
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
    SECTION( "leader wakes" ) {
        wake_leader();
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
    SECTION( "an order" ) {
        guy.goto_to_this_pos = here.get_abs( leader_pos + tripoint( -3, 0, 0 ) );
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
    SECTION( "NPC drifts one tile" ) {
        guy.setpos( here, guy.pos_bub() + tripoint::south );
        CHECK( guy.rest_cooldown_active() );
    }
    SECTION( "NPC moves three tiles" ) {
        guy.setpos( here, guy.pos_bub() + tripoint( 0, 3, 0 ) );
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
    SECTION( "tiredness crosses the next level" ) {
        guy.set_sleepiness( 400 );
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
    SECTION( "a parked vehicle appears under the leader" ) {
        // leader stays put: only the reach turns into the vehicle
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, leader_pos, 0_degrees, 100,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        veh->velocity = 0;
        here.board_vehicle( leader_pos, &get_player_character() );
        REQUIRE( get_player_character().in_vehicle );
        REQUIRE( guy.obligation_reach()->aboard == veh );
        CHECK_FALSE( guy.rest_cooldown_active() );
    }
}

TEST_CASE( "sleep_target_becomes_impassable_is_dropped", "[npc][needs][sleep]" )
{
    sleep_setup();
    map &here = get_map();
    const tripoint_bub_ms bed = leader_pos + tripoint( 3, 0, 0 );
    here.furn_set( bed, furn_f_bed );
    npc &guy = tired_follower( leader_pos + tripoint::west, 400 );
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    run_npc_turn( guy );
    REQUIRE( guy.get_sleep_plan().target == here.get_abs( bed ) );
    REQUIRE( guy.pos_bub() != bed );

    here.ter_set( bed, ter_t_wall_glass );
    here.build_map_cache( 0 );
    for( int turn = 0; turn < 5; ++turn ) {
        run_npc_turn( guy );
        CAPTURE( turn );
        CHECK( guy.get_sleep_plan().target != here.get_abs( bed ) );
        CHECK( guy.pos_bub() != bed );
    }
}

// awake on a bed: stim makes the executor's roll fail, so it lies down and settles
static void make_settling( npc &guy )
{
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 300 );
    guy.set_stim( 100 );
    guy.set_committed_goal( "go_to_sleep" );
    guy.set_moves( 100 );
    REQUIRE( guy.execute_need_goal( "go_to_sleep" ) == npc::need_result::holding );
    REQUIRE( guy.has_effect( effect_lying_down ) );
    REQUIRE_FALSE( guy.has_effect( effect_sleep ) );
}

static npc &spawn_stranger( const tripoint_bub_ms &pos )
{
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( get_map(), pos );
    settle_body( guy, 0 );
    REQUIRE_FALSE( guy.is_player_ally() );
    return guy;
}

static const monster &zombie_in_view( const npc &guy )
{
    map &here = get_map();
    const monster *zed = g->place_critter_at( mon_zombie, guy.pos_bub() + tripoint( 3, 0, 0 ) );
    REQUIRE( zed != nullptr );
    here.build_map_cache( 0 );
    REQUIRE( guy.sees( here, *zed ) );
    return *zed;
}

static void check_got_up( const npc &guy )
{
    CHECK_FALSE( guy.has_effect( effect_lying_down ) );
    CHECK_FALSE( guy.has_effect( effect_sleep ) );
    CHECK_FALSE( guy.get_sleep_plan().active() );
}

TEST_CASE( "settling_npc_gets_up_for_new_interruptions", "[npc][needs][sleep]" )
{
    clear_map_with_vision();
    clear_avatar();
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    map &here = get_map();
    get_avatar().setpos( here, leader_pos );

    SECTION( "critical thirst with water at hand" ) {
        npc &guy = spawn_stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_settling( guy );
        guy.set_thirst( 1000 );
        guy.i_add( item( itype_orange ) );
        run_npc_turn( guy );
        check_got_up( guy );
    }
    SECTION( "hostile in view" ) {
        npc &guy = spawn_stranger( leader_pos + tripoint( 0, -20, 0 ) );
        make_settling( guy );
        zombie_in_view( guy );
        run_npc_turn( guy );
        check_got_up( guy );
    }
    SECTION( "an order to move" ) {
        put_leader_to_sleep();
        on_out_of_scope wake( wake_leader );
        npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 0 );
        make_settling( guy );
        guy.goto_to_this_pos = here.get_abs( leader_pos + tripoint( -3, 0, 0 ) );
        run_npc_turn( guy );
        check_got_up( guy );
    }
    SECTION( "leader wakes and walks off" ) {
        put_leader_to_sleep();
        npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 0 );
        make_settling( guy );
        wake_leader();
        get_avatar().setpos( here, leader_pos + tripoint( -10, 0, 0 ) );
        run_npc_turn( guy );
        check_got_up( guy );
    }
}

TEST_CASE( "settling_npc_records_a_new_override", "[npc][needs][sleep][obligation]" )
{
    clear_map_with_vision();
    clear_avatar();
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    map &here = get_map();
    get_avatar().setpos( here, leader_pos );
    put_leader_to_sleep();
    on_out_of_scope wake( wake_leader );
    npc &guy = tired_follower( leader_pos + tripoint( 2, 0, 0 ), 0 );
    make_settling( guy );
    REQUIRE( guy.get_intention().verdict.permission == npc_permission::allowed );

    SECTION( "leader wakes nearby while it's dead tired" ) {
        guy.set_sleepiness( 450 );
        wake_leader();
        run_npc_turn( guy );
        CHECK( guy.has_effect( effect_lying_down ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::discouraged );
        CHECK( guy.get_intention().verdict.by == npc_obligation::accompany );
        const std::optional<time_point> said = guy.last_complaint( "need_override_sleep" );
        CHECK( said );
        run_npc_turn( guy );
        CHECK( guy.last_complaint( "need_override_sleep" ) == said );
    }
    SECTION( "an order to move while it is exhausted" ) {
        guy.set_sleepiness( 600 );
        guy.goto_to_this_pos = here.get_abs( leader_pos + tripoint( -3, 0, 0 ) );
        run_npc_turn( guy );
        CHECK( guy.has_effect( effect_lying_down ) );
        CHECK( guy.get_intention().verdict.permission == npc_permission::emergency_only );
        CHECK( guy.get_intention().verdict.by == npc_obligation::goto_order );
    }
}

TEST_CASE( "interrupted_settler_does_not_fall_asleep", "[npc][needs][sleep]" )
{
    clear_map_with_vision();
    clear_avatar();
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    map &here = get_map();
    get_avatar().setpos( here, leader_pos );
    npc &guy = spawn_stranger( leader_pos + tripoint( 0, -20, 0 ) );
    make_settling( guy );
    // next roll is certain: f_bed, sleepiness 600, no stim, past the settle cadence
    guy.set_stim( 0 );
    guy.set_sleepiness( 600 );
    calendar::turn += 31_minutes;
    here.build_map_cache( 0 );

    GIVEN( "nothing to get up for" ) {
        run_npc_turn( guy );
        THEN( "it falls asleep" ) {
            CHECK( guy.has_effect( effect_sleep ) );
        }
    }
    GIVEN( "zombie in view on the turn the roll would succeed" ) {
        zombie_in_view( guy );
        run_npc_turn( guy );
        THEN( "it doesn't fall asleep and gets up in that turn" ) {
            CHECK_FALSE( guy.has_effect( effect_sleep ) );
            CHECK_FALSE( guy.has_effect( effect_lying_down ) );
        }
    }
}

TEST_CASE( "rest_review_gets_a_settler_up_and_leaves_sleepers_alone", "[npc][needs][sleep]" )
{
    clear_map_with_vision();
    clear_avatar();
    get_avatar().camps.clear();
    get_weather().forced_temperature = 20_C;
    set_time_to_day();
    map &here = get_map();
    get_avatar().setpos( here, leader_pos );
    npc &guy = spawn_stranger( leader_pos + tripoint( 0, -20, 0 ) );

    SECTION( "settling, interrupted" ) {
        make_settling( guy );
        zombie_in_view( guy );
        guy.review_rest();
        check_got_up( guy );
        CHECK( guy.failed_targets_for( npc::need_goal_id::go_to_sleep ).empty() );
        CHECK( guy.get_committed_goal().empty() );
    }
    SECTION( "asleep" ) {
        guy.add_effect( effect_sleep, 1_hours );
        guy.set_committed_goal( "go_to_sleep" );
        zombie_in_view( guy );
        CHECK_FALSE( guy.rest_interruption() );
        guy.review_rest();
        CHECK( guy.has_effect( effect_sleep ) );
        CHECK( guy.get_committed_goal() == "go_to_sleep" );
    }
}
