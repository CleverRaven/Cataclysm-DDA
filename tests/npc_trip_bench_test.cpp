#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ratio>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "clzones.h"
#include "coordinates.h"
#include "do_turn.h"
#include "enums.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "npc.h"
#include "options_helpers.h"
#include "overmap_ui.h"
#include "player_helpers.h"
#include "point.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const efftype_id effect_sleep( "sleep" );

static const faction_id faction_your_followers( "your_followers" );

static const furn_str_id furn_f_bed( "f_bed" );

static const itype_id itype_orange( "orange" );

static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static const weather_type_id weather_clear( "clear" );

static const zone_type_id zone_type_NPC_NO_GO( "NPC_NO_GO" );

// run by hand with "[npc_trip_bench]"; prints one line per case and phase:
// BENCH,case,phase,npcs,npc_turns,route_calls,ms_total,ms_per_npc_turn,steps,drinks,spins

namespace
{

struct bench_npc {
    npc *guy;
    tripoint_bub_ms start;
    // sleep cases hold tiredness instead of thirst
    bool tired = false;
};

struct phase_total {
    int turns = 0;
    double ms = 0.0;
    // tiles walked: tells a slower phase from one that walks more
    int steps = 0;
    // turns that ended with water drunk
    int drinks = 0;
    int route_calls = 0;
    // spun-out turns; their time isn't real work
    int spins = 0;
};

npc &spawn_thirsty( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    guy.set_thirst( 600 );
    return guy;
}

npc &make_follower( npc &guy )
{
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_mission( NPC_MISSION_NULL );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.rules.set_flag( ally_rule::allow_sleep );
    return guy;
}

// fixed workload: need back every round, start position every reset_every rounds
void restore( bench_npc &b, const int round, const int reset_every )
{
    if( b.tired ) {
        b.guy->set_sleepiness( 300 );
    } else {
        b.guy->stomach.empty();
        b.guy->set_thirst( 600 );
    }
    if( reset_every > 0 && round % reset_every == reset_every - 1 ) {
        b.guy->setpos( get_map(), b.start );
        b.guy->path.clear();
    }
}

void run_case( const std::string &name, std::vector<bench_npc> &npcs, const int rounds,
               const int reset_every, const std::function<void( int )> &between_rounds = {} )
{
    map &here = get_map();
    std::map<std::string, phase_total> phases;
    for( int round = 0; round < rounds; ++round ) {
        for( bench_npc &b : npcs ) {
            const bool pursuing = b.tired ? b.guy->get_sleep_plan().active() :
                                  b.guy->get_water_plan().active();
            const std::string phase = pursuing ? "pursue" : "acquire";
            const tripoint_bub_ms from = b.guy->pos_bub();
            const int routes_before = b.guy->trip_route_calls();
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            const npc_turn_result result = process_npc_turn( here, *b.guy );
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
            phase_total &p = phases[phase];
            p.turns++;
            p.route_calls += b.guy->trip_route_calls() - routes_before;
            if( result.spun_out ) {
                p.spins++;
            }
            p.ms += std::chrono::duration<double, std::milli>( t1 - t0 ).count();
            p.steps += rl_dist( from, b.guy->pos_bub() );
            if( b.guy->stomach.get_water() > 0_ml ) {
                p.drinks++;
            }
        }
        calendar::turn += 1_turns;
        for( bench_npc &b : npcs ) {
            restore( b, round, reset_every );
        }
        if( between_rounds ) {
            between_rounds( round );
        }
    }
    for( const auto &[phase, total] : phases ) {
        WARN( string_format( "BENCH,%s,%s,%zu,%d,%d,%.3f,%.4f,%d,%d,%d", name, phase, npcs.size(),
                             total.turns, total.route_calls, total.ms,
                             total.turns > 0 ? total.ms / total.turns : 0.0, total.steps, total.drinks,
                             total.spins ) );
    }
}

void bench_setup()
{
    // candidate scans need sight: vision on, daylight
    clear_map_with_vision();
    set_time_to_day();
    clear_avatar();
    get_avatar().setpos( get_map(), tripoint_bub_ms( 20, 20, 0 ) );
    get_avatar().camps.clear();
}

void lay_pond( const tripoint_bub_ms &center, const int radius )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( center, radius ) ) {
        here.ter_set( p, ter_t_water_sh );
    }
}

} // namespace

TEST_CASE( "npc_trip_bench", "[.][npc_trip_bench]" )
{
    constexpr int rounds = 100;
    constexpr int walk_cycle = 10;

    SECTION( "pond_many_sources" ) {
        bench_setup();
        lay_pond( tripoint_bub_ms( 58, 50, 0 ), 3 );
        get_map().build_map_cache( 0 );
        std::vector<bench_npc> npcs{ { &spawn_thirsty( tripoint_bub_ms( 50, 50, 0 ) ), tripoint_bub_ms( 50, 50, 0 ) } };
        run_case( "pond_many_sources", npcs, rounds, walk_cycle );
    }
    SECTION( "drinks_behind_obstacles" ) {
        bench_setup();
        map &here = get_map();
        const tripoint_bub_ms start( 50, 50, 0 );
        // each orange sealed in glass, so every candidate is routed and none reached.
        // rings apart: an orange under another's ring wall is reachable from outside
        std::vector<tripoint_bub_ms> oranges;
        for( const tripoint_bub_ms &p : here.points_in_radius( start, 9 ) ) {
            const bool apart = std::none_of( oranges.begin(), oranges.end(),
            [&p]( const tripoint_bub_ms & o ) {
                return square_dist( o, p ) < 3;
            } );
            if( oranges.size() >= 12 || rl_dist( p, start ) < 5 || rl_dist( p, start ) > 6 || !apart ) {
                continue;
            }
            here.add_item_or_charges( p, item( itype_orange ) );
            for( const tripoint_bub_ms &w : here.points_in_radius( p, 1 ) ) {
                if( w != p ) {
                    here.ter_set( w, ter_t_wall_glass );
                }
            }
            oranges.push_back( p );
        }
        REQUIRE( oranges.size() == 12 );
        here.build_map_cache( 0 );
        std::vector<bench_npc> npcs{ { &spawn_thirsty( start ), start } };
        run_case( "drinks_behind_obstacles", npcs, rounds, 0 );
        // the workload held: nothing was reached
        for( const tripoint_bub_ms &p : oranges ) {
            CAPTURE( p );
            CHECK( here.has_items( p ) );
        }
    }
    SECTION( "many_thirsty" ) {
        bench_setup();
        lay_pond( tripoint_bub_ms( 58, 50, 0 ), 3 );
        get_map().build_map_cache( 0 );
        std::vector<bench_npc> npcs;
        for( int i = 0; i < 8; ++i ) {
            const tripoint_bub_ms start( 49 + i % 2, 46 + i, 0 );
            npcs.push_back( { &spawn_thirsty( start ), start } );
        }
        run_case( "many_thirsty", npcs, rounds, walk_cycle );
    }
    SECTION( "follower_retained_plan_many_sources" ) {
        bench_setup();
        const tripoint_bub_ms leader( 50, 50, 0 );
        get_avatar().setpos( get_map(), leader );
        lay_pond( tripoint_bub_ms( 54, 50, 0 ), 3 );
        get_map().build_map_cache( 0 );
        const tripoint_bub_ms start( 47, 50, 0 );
        std::vector<bench_npc> npcs{ { &make_follower( spawn_thirsty( start ) ), start } };
        run_case( "follower_retained_plan_many_sources", npcs, rounds, walk_cycle );
    }
    SECTION( "follower_no_acceptable_route" ) {
        bench_setup();
        map &here = get_map();
        const tripoint_bub_ms leader( 50, 50, 0 );
        get_avatar().setpos( here, leader );
        const tripoint_bub_ms start = leader + tripoint::east;
        // 12 oranges in sight and reachable, all past the follower's spacing
        int placed = 0;
        for( const tripoint_bub_ms &p : here.points_in_radius( start, 6 ) ) {
            if( placed >= 12 || rl_dist( p, start ) != 6 || rl_dist( p, leader ) <= 5 ||
                ( p.x() + p.y() ) % 2 != 0 ) {
                continue;
            }
            here.add_item_or_charges( p, item( itype_orange ) );
            placed++;
        }
        REQUIRE( placed == 12 );
        here.build_map_cache( 0 );
        std::vector<bench_npc> npcs{ { &make_follower( spawn_thirsty( start ) ), start } };
        run_case( "follower_no_acceptable_route", npcs, rounds, 0 );
    }
    SECTION( "many_thirsty_followers" ) {
        bench_setup();
        const tripoint_bub_ms leader( 50, 50, 0 );
        get_avatar().setpos( get_map(), leader );
        lay_pond( tripoint_bub_ms( 54, 50, 0 ), 3 );
        get_map().build_map_cache( 0 );
        std::vector<bench_npc> npcs;
        for( int i = 0; i < 6; ++i ) {
            const tripoint_bub_ms start( 47 + i % 2, 47 + i, 0 );
            npcs.push_back( { &make_follower( spawn_thirsty( start ) ), start } );
        }
        run_case( "many_thirsty_followers", npcs, rounds, walk_cycle );
    }
    SECTION( "rest_cooldown_walking_leader" ) {
        bench_setup();
        map &here = get_map();
        const tripoint_bub_ms leader( 40, 50, 0 );
        get_avatar().setpos( here, leader );
        get_avatar().add_effect( effect_sleep, 24_hours );
        std::vector<bench_npc> npcs;
        for( int i = 0; i < 3; ++i ) {
            const tripoint_bub_ms start = leader + tripoint( -1, i - 1, 0 );
            npc &guy = make_follower( spawn_thirsty( start ) );
            guy.set_thirst( 0 );
            guy.set_sleepiness( 300 );
            npcs.push_back( { &guy, start, true } );
        }
        // no spot along the leader's walk
        zone_manager &mgr = zone_manager::get_manager();
        mgr.add( "no_rest", zone_type_NPC_NO_GO, npcs.front().guy->get_fac_id(), false, true,
                 here.get_abs( leader + tripoint( -10, -10, 0 ) ),
                 here.get_abs( leader + tripoint( 70, 10, 0 ) ) );
        mgr.cache_data();
        here.build_map_cache( 0 );
        run_case( "rest_cooldown_walking_leader", npcs, rounds, 0, [&here]( const int round ) {
            if( round % 2 == 1 ) {
                get_avatar().setpos( here, get_avatar().pos_bub() + tripoint::east );
            }
        } );
        get_avatar().remove_effect( effect_sleep );
    }
    SECTION( "settling_npcs" ) {
        bench_setup();
        map &here = get_map();
        std::vector<bench_npc> npcs;
        for( int i = 0; i < 4; ++i ) {
            const tripoint_bub_ms start( 48 + 2 * i, 50, 0 );
            here.furn_set( start, furn_f_bed );
            npc &guy = spawn_thirsty( start );
            guy.set_thirst( 0 );
            guy.set_sleepiness( 300 );
            guy.set_stim( 100 );
            npcs.push_back( { &guy, start, true } );
        }
        // hostile behind a wall: in range, out of view
        for( int x = 44; x <= 60; ++x ) {
            here.ter_set( tripoint_bub_ms( x, 53, 0 ), ter_t_wall );
        }
        REQUIRE( g->place_critter_at( mon_zombie, tripoint_bub_ms( 52, 55, 0 ) ) != nullptr );
        here.build_map_cache( 0 );
        const auto lie_down = [&npcs]( int ) {
            for( bench_npc &b : npcs ) {
                if( !b.guy->in_sleep_state() ) {
                    // lie down the way the game does: the roll fails on stim, so it settles
                    b.guy->set_committed_goal( "go_to_sleep" );
                    b.guy->set_moves( 100 );
                    b.guy->execute_need_goal( "go_to_sleep" );
                }
            }
        };
        lie_down( 0 );
        run_case( "settling_npcs", npcs, rounds, 0, lie_down );
    }
    SECTION( "woken_passengers" ) {
        bench_setup();
        scoped_weather_override clear_weather( weather_clear );
        map &here = get_map();
        const tripoint_abs_sm origin = here.get_abs_sub();
        const tripoint_bub_ms leader( 50, 50, 0 );
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_rv, leader, 0_degrees, 100,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        avatar &you = get_avatar();
        you.setpos( here, leader );
        here.board_vehicle( leader, &you );
        REQUIRE( you.in_vehicle );
        // boarding recenters the map and can load submaps with monsters
        clear_creatures();
        // spawn_npc can shift the map: look bed tiles up only after every spawn
        const auto find_beds = [&]() {
            std::vector<tripoint_bub_ms> beds;
            for( const vpart_reference &vp : veh->get_all_parts() ) {
                const tripoint_bub_ms p = veh->bub_part_pos( here, vp.part() );
                const optional_vpart_position at = here.veh_at( p );
                const bool bed = at && ( at->part_with_feature( "SEATBELT", true ) ||
                                         at->part_with_feature( "BED", true ) );
                if( bed && p != you.pos_bub() &&
                    std::find( beds.begin(), beds.end(), p ) == beds.end() ) {
                    beds.push_back( p );
                }
            }
            beds.resize( std::min<size_t>( beds.size(), 4 ) );
            return beds;
        };
        const size_t count = find_beds().size();
        REQUIRE( count >= 2 );
        std::vector<npc *> passengers;
        for( size_t i = 0; i < count; ++i ) {
            // spawned off the vehicle, then seated: spawning onto a part boards on its own
            npc &guy = make_follower( spawn_thirsty( leader + tripoint( 2 * static_cast<int>( i ), 10, 0 ) ) );
            guy.set_thirst( 0 );
            passengers.push_back( &guy );
        }
        const std::vector<tripoint_bub_ms> beds = find_beds();
        REQUIRE( beds.size() == count );
        std::vector<bench_npc> npcs;
        for( size_t i = 0; i < count; ++i ) {
            npc &guy = *passengers[i];
            const tripoint_bub_ms &p = beds[i];
            guy.setpos( here, p );
            here.board_vehicle( p, &guy );
            REQUIRE( guy.in_vehicle );
            // sure to drop off at the first roll on a bed; the bench then holds it at 300
            guy.set_sleepiness( 600 );
            guy.set_stim( 0 );
        }
        // each falls asleep where it was seated before the timed rounds
        for( int turn = 0; turn < 30; ++turn ) {
            for( npc *guy : passengers ) {
                if( !guy->has_effect( effect_sleep ) ) {
                    process_npc_turn( here, *guy );
                }
            }
            calendar::turn += 1_turns;
        }
        for( npc *guy : passengers ) {
            REQUIRE( guy->has_effect( effect_sleep ) );
            npcs.push_back( { guy, guy->pos_bub(), true } );
        }
        veh->velocity = 1000;
        run_case( "woken_passengers", npcs, rounds, 0, [&]( const int round ) {
            veh->precalc_mounts( 1, veh->face.dir(), veh->pivot_point( here ) );
            here.displace_vehicle( *veh, tripoint_rel_ms::east );
            if( round % 5 == 4 ) {
                // an engine revving next to each of them
                sounds::reset_sounds();
                for( bench_npc &b : npcs ) {
                    sounds::sound( b.guy->pos_bub() + tripoint::east, 60, sounds::sound_t::movement, "engine" );
                    sounds::process_sound_markers( b.guy );
                }
            }
        } );
        here.unboard_vehicle( you.pos_bub() );
        clear_vehicles();
        here.load( origin, false, false );
    }
}
