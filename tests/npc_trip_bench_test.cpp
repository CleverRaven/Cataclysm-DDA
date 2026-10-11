#include <algorithm>
#include <chrono>
#include <map>
#include <ratio>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "do_turn.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static const itype_id itype_orange( "orange" );

static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

// run by hand with "[npc_trip_bench]"; prints one line per case and phase:
// BENCH,case,phase,npcs,npc_turns,route_calls,ms_total,ms_per_npc_turn,steps,drinks,spins

namespace
{

struct bench_npc {
    npc *guy;
    tripoint_bub_ms start;
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

// fixed workload: thirst back every round, start position every reset_every rounds
void restore( bench_npc &b, const int round, const int reset_every )
{
    b.guy->stomach.empty();
    b.guy->set_thirst( 600 );
    if( reset_every > 0 && round % reset_every == reset_every - 1 ) {
        b.guy->setpos( get_map(), b.start );
        b.guy->path.clear();
    }
}

void run_case( const std::string &name, std::vector<bench_npc> &npcs, const int rounds,
               const int reset_every )
{
    map &here = get_map();
    std::map<std::string, phase_total> phases;
    for( int round = 0; round < rounds; ++round ) {
        for( bench_npc &b : npcs ) {
            const std::string phase = b.guy->get_water_plan().active() ? "pursue" : "acquire";
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
}
