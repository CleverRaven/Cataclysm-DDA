#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <utility>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "options.h"
#include "player.h"
#include "test_statistics.h"
#include "game_constants.h"
#include "item.h"
#include "line.h"
#include "point.h"

using move_statistics = statistics<int>;

static int moves_to_destination( const std::string &monster_type,
                                 const tripoint &start, const tripoint &end )
{
    clear_creatures();
    REQUIRE( g->num_creatures() == 1 ); // the player
    monster &test_monster = spawn_test_monster( monster_type, start );
    // Get it riled up and give it a goal.
    test_monster.anger = 100;
    test_monster.set_dest( end );
    test_monster.set_moves( 0 );
    const int monster_speed = test_monster.get_speed();
    int moves_spent = 0;
    for( int turn = 0; turn < 1000; ++turn ) {
        test_monster.mod_moves( monster_speed );
        while( test_monster.moves >= 0 ) {
            test_monster.anger = 100;
            const int moves_before = test_monster.moves;
            test_monster.move();
            moves_spent += moves_before - test_monster.moves;
            if( test_monster.pos() == test_monster.move_target() ) {
                g->remove_zombie( test_monster );
                return moves_spent;
            }
        }
    }
    g->remove_zombie( test_monster );
    // Return an unreasonably high number.
    return 100000;
}

struct track {
    char participant;
    int moves;
    int distance;
    tripoint location;
};

static std::ostream &operator<<( std::ostream &os, track const &value )
{
    os << value.participant <<
       " l:" << value.location <<
       " d:" << value.distance <<
       " m:" << value.moves;
    return os;
}

static std::ostream &operator<<( std::ostream &os, const std::vector<track> &vec )
{
    for( auto &track_instance : vec ) {
        os << track_instance << " ";
    }
    return os;
}

/**
 * Simulate a player running from the monster, checking if it can catch up.
 **/
static int can_catch_player( const std::string &monster_type, const tripoint &direction_of_flight )
{
    clear_map();
    REQUIRE( g->num_creatures() == 1 ); // the player
    player &test_player = g->u;
    // Strip off any potentially encumbering clothing.
    std::list<item> temp;
    while( test_player.takeoff( test_player.i_at( -2 ), &temp ) );

    const tripoint center{ 65, 65, 0 };
    test_player.setpos( center );
    test_player.set_moves( 0 );
    // Give the player a head start.
    const tripoint monster_start = { -10 * direction_of_flight + test_player.pos()
                                   };
    monster &test_monster = spawn_test_monster( monster_type, monster_start );
    // Get it riled up and give it a goal.
    test_monster.anger = 100;
    test_monster.set_dest( test_player.pos() );
    test_monster.set_moves( 0 );
    const int monster_speed = test_monster.get_speed();
    const int target_speed = 100;

    int moves_spent = 0;
    std::vector<track> tracker;
    for( int turn = 0; turn < 1000; ++turn ) {
        test_player.mod_moves( target_speed );
        while( test_player.moves >= 0 ) {
            test_player.setpos( test_player.pos() + direction_of_flight );
            if( test_player.pos().x < SEEX * int( MAPSIZE / 2 ) ||
                test_player.pos().y < SEEY * int( MAPSIZE / 2 ) ||
                test_player.pos().x >= SEEX * ( 1 + int( MAPSIZE / 2 ) ) ||
                test_player.pos().y >= SEEY * ( 1 + int( MAPSIZE / 2 ) ) ) {
                tripoint offset = center - test_player.pos();
                test_player.setpos( center );
                test_monster.setpos( test_monster.pos() + offset );
                // Verify that only the player and one monster are present.
                REQUIRE( g->num_creatures() == 2 );
            }
            const int move_cost = g->m.combined_movecost(
                                      test_player.pos(), test_player.pos() + direction_of_flight, nullptr, 0 );
            tracker.push_back( {'p', move_cost, rl_dist( test_monster.pos(), test_player.pos() ),
                                test_player.pos()
                               } );
            test_player.mod_moves( -move_cost );
        }
        g->m.clear_traps();
        test_monster.set_dest( test_player.pos() );
        test_monster.mod_moves( monster_speed );
        while( test_monster.moves >= 0 ) {
            const int moves_before = test_monster.moves;
            test_monster.move();
            tracker.push_back( {'m', moves_before - test_monster.moves,
                                rl_dist( test_monster.pos(), test_player.pos() ),
                                test_monster.pos()
                               } );
            moves_spent += moves_before - test_monster.moves;
            if( rl_dist( test_monster.pos(), test_player.pos() ) == 1 ) {
                INFO( tracker );
                clear_map();
                return turn;
            } else if( rl_dist( test_monster.pos(), test_player.pos() ) > 20 ) {
                INFO( tracker );
                clear_map();
                return -turn;
            }
        }
    }
    WARN( tracker );
    return -1000;
}

// Verify that the named monster has the expected effective speed, not reduced
// due to wasted motion from shambling.
static void check_shamble_speed( const std::string &monster_type, const tripoint &destination )
{
    // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
    const float slope = get_normalized_angle( point_zero, destination.xy() );
    const float diagonal_multiplier = 1.0 + ( get_option<bool>( "CIRCLEDIST" ) ?
                                      ( slope * 0.41 ) : 0.0 );
    INFO( monster_type << " " << destination );
    // Wandering makes things nondeterministic, so look at the distribution rather than a target number.
    move_statistics move_stats;
    for( int i = 0; i < 10; ++i ) {
        move_stats.add( moves_to_destination( monster_type, tripoint_zero, destination ) );
        if( ( move_stats.avg() / ( 10000.0 * diagonal_multiplier ) ) ==
            Approx( 1.0 ).epsilon( 0.02 ) ) {
            break;
        }
    }
    CAPTURE( slope );
    CAPTURE( move_stats.avg() );
    INFO( diagonal_multiplier );
    CHECK( ( move_stats.avg() / ( 10000.0 * diagonal_multiplier ) ) ==
           Approx( 1.0 ).epsilon( 0.02 ) );
}

static void test_moves_to_squares( const std::string &monster_type, const bool write_data = false )
{
    std::map<int, move_statistics> turns_at_distance;
    std::map<int, move_statistics> turns_at_slope;
    std::map<int, move_statistics> turns_at_angle;
    // For the regression test we want just enough samples, for data we want a lot more.
    const int required_samples = write_data ? 100 : 20;
    const int sampling_resolution = write_data ? 1 : 20;
    bool not_enough_samples = true;
    while( not_enough_samples ) {
        not_enough_samples = false;
        for( int x = 0; x <= 100; x += sampling_resolution ) {
            for( int y = 0; y <= 100; y += sampling_resolution ) {
                const int distance = square_dist( {50, 50, 0}, {x, y, 0} );
                if( distance <= 5 ) {
                    // Very short ranged tests are squirrely.
                    continue;
                }
                const int rise = 50 - y;
                const int run = 50 - x;
                const float angle = atan2( run, rise );
                // Bail out if we already have enough samples for this angle.
                if( turns_at_angle[angle * 100].n() >= required_samples ) {
                    continue;
                }
                // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
                const float slope = get_normalized_angle( {50, 50}, {x, y} );
                const float diagonal_multiplier = 1.0 + ( get_option<bool>( "CIRCLEDIST" ) ?
                                                  ( slope * 0.41 ) : 0.0 );
                turns_at_angle[angle * 100].new_type();
                turns_at_slope[slope].new_type();
                for( int i = 0; i < 5; ++i ) {
                    const int moves = moves_to_destination( monster_type, {50, 50, 0}, {x, y, 0} );
                    const int adjusted_moves = moves / ( diagonal_multiplier * distance );
                    turns_at_distance[distance].add( adjusted_moves );
                    turns_at_angle[angle * 100].add( adjusted_moves );
                    turns_at_slope[slope].add( adjusted_moves );
                }
                if( turns_at_angle[angle * 100].n() < required_samples ) {
                    not_enough_samples = true;
                }
            }
        }
    }
    for( const auto &stat_pair : turns_at_distance ) {
        INFO( "Monster:" << monster_type << " Dist: " << stat_pair.first << " moves: " <<
              stat_pair.second.avg() );
        CHECK( stat_pair.second.avg() == Approx( 100.0 ).epsilon( 0.1 ) );
    }
    for( const auto &stat_pair : turns_at_slope ) {
        INFO( "Monster:" << monster_type << " Slope: " << stat_pair.first <<
              " moves: " << stat_pair.second.avg() << " types: " << stat_pair.second.types() );
        CHECK( stat_pair.second.avg() == Approx( 100.0 ).epsilon( 0.1 ) );
    }
    for( auto &stat_pair : turns_at_angle ) {
        std::stringstream sample_string;
        for( auto sample : stat_pair.second.get_samples() ) {
            sample_string << sample << ", ";
        }
        INFO( "Monster:" << monster_type << " Angle: " << stat_pair.first <<
              " moves: " << stat_pair.second.avg() << " types: " << stat_pair.second.types() <<
              " samples: " << sample_string.str() );
        CHECK( stat_pair.second.avg() == Approx( 100.0 ).epsilon( 0.1 ) );
    }

    if( write_data ) {
        std::ofstream data;
        data.open( "slope_test_data_" + std::string( ( trigdist ? "trig_" : "square_" ) ) + monster_type );
        for( const auto &stat_pair : turns_at_angle ) {
            data << stat_pair.first << " " << stat_pair.second.avg() << "\n";
        }
        data.close();
    }
}

static void monster_check()
{
    const float diagonal_multiplier = ( get_option<bool>( "CIRCLEDIST" ) ? 1.41 : 1.0 );
    // Have a monster walk some distance in a direction and measure how long it takes.
    float vert_move = moves_to_destination( "mon_pig", tripoint_zero, {100, 0, 0} );
    CHECK( ( vert_move / 10000.0 ) == Approx( 1.0 ) );
    int horiz_move = moves_to_destination( "mon_pig", tripoint_zero, {0, 100, 0} );
    CHECK( ( horiz_move / 10000.0 ) == Approx( 1.0 ) );
    int diag_move = moves_to_destination( "mon_pig", tripoint_zero, {100, 100, 0} );
    CHECK( ( diag_move / ( 10000.0 * diagonal_multiplier ) ) == Approx( 1.0 ).epsilon( 0.05 ) );

    check_shamble_speed( "mon_pig", {100, 0, 0} );
    check_shamble_speed( "mon_pig", {0, 100, 0} );
    check_shamble_speed( "mon_pig", {100, 100, 0} );
    check_shamble_speed( "mon_zombie", {100, 0, 0} );
    check_shamble_speed( "mon_zombie", {0, 100, 0} );
    check_shamble_speed( "mon_zombie", {100, 100, 0} );
    check_shamble_speed( "mon_zombie_dog", {100, 0, 0} );
    check_shamble_speed( "mon_zombie_dog", {0, 100, 0} );
    check_shamble_speed( "mon_zombie_dog", {100, 100, 0} );
    check_shamble_speed( "mon_zombear", {100, 0, 0} );
    check_shamble_speed( "mon_zombear", {0, 100, 0} );
    check_shamble_speed( "mon_zombear", {100, 100, 0} );
    check_shamble_speed( "mon_jabberwock", {100, 0, 0} );
    check_shamble_speed( "mon_jabberwock", {0, 100, 0} );
    check_shamble_speed( "mon_jabberwock", {100, 100, 0} );

    // Check moves to all squares relative to monster start within 50 squares.
    test_moves_to_squares( "mon_zombie_dog" );
    test_moves_to_squares( "mon_pig" );

    // Verify that a walking player can escape from a zombie, but is caught by a zombie dog.
    INFO( "Trigdist is " << ( get_option<bool>( "CIRCLEDIST" ) ? "on" : "off" ) );
    CHECK( can_catch_player( "mon_zombie", tripoint_east ) < 0 );
    CHECK( can_catch_player( "mon_zombie", tripoint_south_east ) < 0 );
    CHECK( can_catch_player( "mon_zombie_dog", tripoint_east ) > 0 );
    CHECK( can_catch_player( "mon_zombie_dog", tripoint_south_east ) > 0 );
}

// Write out a map of slope at which monster is moving to time required to reach their destination.
TEST_CASE( "write_slope_to_speed_map_trig", "[.]" )
{
    clear_map_and_put_player_underground();
    get_options().get_option( "CIRCLEDIST" ).setValue( "true" );
    trigdist = true;
    test_moves_to_squares( "mon_zombie_dog", true );
    test_moves_to_squares( "mon_pig", true );
}

TEST_CASE( "write_slope_to_speed_map_square", "[.]" )
{
    clear_map_and_put_player_underground();
    get_options().get_option( "CIRCLEDIST" ).setValue( "false" );
    trigdist = false;
    test_moves_to_squares( "mon_zombie_dog", true );
    test_moves_to_squares( "mon_pig", true );
}

// Characterization test for monster movement speed.
// It's not necessarally the one true speed for monsters, we just want notice if it changes.
TEST_CASE( "monster_speed_square", "[speed]" )
{
    clear_map_and_put_player_underground();
    get_options().get_option( "CIRCLEDIST" ).setValue( "false" );
    trigdist = false;
    monster_check();
}

TEST_CASE( "monster_speed_trig", "[speed]" )
{
    clear_map_and_put_player_underground();
    get_options().get_option( "CIRCLEDIST" ).setValue( "true" );
    trigdist = true;
    monster_check();
}
