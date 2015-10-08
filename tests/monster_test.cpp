#include "catch/catch.hpp"

#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::ostream& operator << ( std::ostream& os, tripoint const& value ) {
    os << "(" << value.x << "," << value.y << "," << value.z << ")";
    return os;
}

void wipe_map_terrain()
{
    // Remove all the obstacles.
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            g->m.set(x, y, t_grass, f_null);
        }
    }
}

monster &spawn_test_monster( const std::string &monster_type, const tripoint &start )
{
    monster temp_monster( mtype_id(monster_type), start);
    // Bypassing game::add_zombie() since it sometimes upgrades the monster instantly.
    g->critter_tracker->add( temp_monster );
    return g->critter_tracker->find( 0 );
}

int moves_to_destination( const std::string &monster_type,
                          const tripoint &start, const tripoint &end )
{
    REQUIRE( g->num_zombies() == 0 );
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
            int moves_before = test_monster.moves;
            test_monster.move();
            moves_spent += moves_before - test_monster.moves;
            if( test_monster.pos() == test_monster.move_target() ) {
                g->remove_zombie( 0 );
                return moves_spent;
            }
        }
    }
    g->remove_zombie( 0 );
    // Return an unreasonably high number.
    return 100000;
}

/**
 * Simulate a player running from the monster, checking if it can catch up.
 **/
int can_catch_player( const std::string &monster_type, const tripoint &direction_of_flight )
{
    REQUIRE( g->num_zombies() == 0 );
    player &test_player = g->u;
    // Strip off any potentially encumbering clothing.
    std::vector<item> taken_off_items;
    while( test_player.takeoff( -2, true, &taken_off_items) );

    test_player.setpos( { 65, 65, 0 } );
    test_player.set_moves( 0 );
    // Give the player a head start.
    const tripoint monster_start = { test_player.pos().x - (10 * direction_of_flight.x),
                                     test_player.pos().y - (10 * direction_of_flight.y),
                                     test_player.pos().z - (10 * direction_of_flight.z) };
    monster &test_monster = spawn_test_monster( monster_type, monster_start );
    // Get it riled up and give it a goal.
    test_monster.anger = 100;
    test_monster.set_dest( test_player.pos() );
    test_monster.set_moves( 0 );
    const int monster_speed = test_monster.get_speed();
    const int target_speed = 100;

    int moves_spent = 0;
    struct track {
        char participant;
        int distance;
        int moves;
        tripoint location;
    };
    std::vector<track> tracker;
    for( int turn = 0; turn < 1000; ++turn ) {
        test_player.mod_moves( target_speed );
        while( test_player.moves >= 0 ) {
            test_player.setpos( test_player.pos() + direction_of_flight );
            if( test_player.pos().x < SEEX * int(MAPSIZE / 2) ||
                test_player.pos().y < SEEY * int(MAPSIZE / 2) ||
                test_player.pos().x >= SEEX * (1 + int(MAPSIZE / 2)) ||
                test_player.pos().y >= SEEY * (1 + int(MAPSIZE / 2)) ) {
                g->update_map( &test_player );
                wipe_map_terrain();
                for( unsigned int i = 0; i < g->num_zombies(); ) {
                    if( &g->zombie( i ) == &test_monster ) {
                        i++;
                    } else {
                        g->remove_zombie( i );
                    }
                }
            }
            const int move_cost = g->m.combined_movecost(
                test_player.pos(), test_player.pos() + direction_of_flight, nullptr, 0 );
            tracker.push_back({'p', move_cost, rl_dist(test_monster.pos(), test_player.pos()),
                        test_player.pos()} );
            test_player.mod_moves( -move_cost );
        }
        test_monster.set_dest( test_player.pos() );
        test_monster.mod_moves( monster_speed );
        while( test_monster.moves >= 0 ) {
            int moves_before = test_monster.moves;
            test_monster.move();
            tracker.push_back({'m', moves_before - test_monster.moves,
                        rl_dist(test_monster.pos(), test_player.pos()),
                        test_monster.pos()} );
            moves_spent += moves_before - test_monster.moves;
            if( rl_dist( test_monster.pos(), test_player.pos() ) == 1 ) {
                g->remove_zombie( 0 );
                std::stringstream path_string;
                for( const auto track_inst : tracker ) {
                    path_string << track_inst.participant << "," << track_inst.moves << "," <<
                        track_inst.distance << "," << track_inst.location << " ";
                }
                WARN(path_string.str());
                return turn;
            } else if( rl_dist( test_monster.pos(), test_player.pos() ) > 20 ) {
                g->remove_zombie( 0 );
                std::stringstream path_string;
                for( const auto track_inst : tracker ) {
                    path_string << track_inst.participant << "," << track_inst.moves << "," <<
                        track_inst.distance << "," << track_inst.location << " ";
                }
                WARN(path_string.str());
                return -turn;
            }
        }
    }
    g->remove_zombie( 0 );
    return -1000;
}

class statistics {
private:
    int _types;
    int _n;
    int _sum;
    int _max;
    int _min;

public:
    statistics() : _types(0), _n(0), _sum(0), _max(INT_MIN), _min(INT_MAX) {}

    void new_type() {
        _types++;
    }
    void add( int new_val ) {
        _n++;
        _sum += new_val;
        _max = std::max(_max, new_val);
        _min = std::min(_min, new_val);
    }
    int types() const { return _types; }
    int sum() const { return _sum; }
    int max() const { return _max; }
    int min() const { return _min; }
    float avg() const { return (float)_sum / (float)_n; }
};

// Verify that the named monster has the expected effective speed, not reduced
// due to wasted motion from shambling.
void check_shamble_speed( const std::string monster_type, const tripoint &destination )
{
    // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
    const float slope = get_normalized_angle( {0, 0}, {destination.x, destination.y} );
    const float diagonal_multiplier = 1.0 + (OPTIONS["CIRCLEDIST"] ? (slope * 0.41) : 0.0);
    INFO( monster_type << " " << destination );
    // Wandering makes things nondeterministic, so look at the distribution rather than a target number.
    statistics move_stats;
    for( int i = 0; i < 10; ++i ) {
        move_stats.add( moves_to_destination( monster_type, {0, 0, 0}, destination ) );
        if( (move_stats.avg() / (10000.0 * diagonal_multiplier)) ==
            Approx(1.0).epsilon(0.02) ) {
            break;
        }
    }
    CAPTURE(slope);
    CAPTURE(move_stats.avg());
    INFO(diagonal_multiplier);
    CHECK( (move_stats.avg() / (10000.0 * diagonal_multiplier)) ==
           Approx(1.0).epsilon(0.02) );
}

void monster_check() {
    // Make sure the player doesn't block the path of the monster being tested.
    g->u.setpos( { 0, 0, -2 } );
    const float diagonal_multiplier = (OPTIONS["CIRCLEDIST"] ? 1.41 : 1.0);
    // Have a monster walk some distance in a direction and measure how long it takes.
    float vert_move = moves_to_destination( "mon_pig", {0,0,0}, {100,0,0} );
    CHECK( (vert_move / 10000.0) == Approx(1.0) );
    int horiz_move = moves_to_destination( "mon_pig", {0,0,0}, {0,100,0} );
    CHECK( (horiz_move / 10000.0) == Approx(1.0) );
    int diag_move = moves_to_destination( "mon_pig", {0,0,0}, {100,100,0} );
    CHECK( (diag_move / (10000.0 * diagonal_multiplier)) == Approx(1.0).epsilon(0.01) );

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

    // Check some angles between 0deg and 45deg
    for( int x = 0; x <= 100; x += 2 ) {
        check_shamble_speed( "mon_zombie", {x, 100, 0} );
    }

    std::map<int, statistics> turns_at_distance;
    std::map<float, statistics> turns_at_angle;
    for( int x = 0; x <= 100; x += 2 ) {
        for( int y = 0; y <= 100; y += 2 ) {
            const int distance = rl_dist({50,50,0}, {x,y,0});
                // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
            const float norm_slope = get_normalized_angle( {50, 50}, {x, y} );
            const float diagonal_multiplier = 1.0 + (OPTIONS["CIRCLEDIST"] ? (norm_slope * 0.41) : 0.0);
            if( distance < 5 ) {
                // Very short ranged tests are squirrely.
                continue;
            }
            const int rise = 50 - y;
            const int run = 50 - x;
            const float slope = atan2( run, rise );
            turns_at_angle[slope].new_type();
            for( int i = 0; i < 500; ++i ) {
                int moves = moves_to_destination( "mon_zombie_dog", {50, 50, 0}, {x, y, 0} );
                turns_at_distance[distance].add( (moves * diagonal_multiplier) / distance );
                turns_at_angle[slope].add( (moves * diagonal_multiplier) / distance);
                if( turns_at_angle[slope].avg() == Approx(100.0).epsilon(0.04) ) {
                    break;
                }
            }
        }
    }
    std::stringstream slope_map;
    for( const auto &stat_pair : turns_at_distance ) {
        INFO( "Dist: " << stat_pair.first << " moves: " << stat_pair.second.avg() );
        CHECK( stat_pair.second.avg() == Approx(100.0).epsilon(0.02) );
    }
    for( const auto &stat_pair : turns_at_angle ) {
        WARN( "Slope: " << stat_pair.first << " moves: " << stat_pair.second.avg() <<
              " types: " << stat_pair.second.types() );
        slope_map << stat_pair.first << " " << stat_pair.second.avg() << "\n" ;
        CHECK( stat_pair.second.avg() == Approx(100.0).epsilon(0.04) );
    }
    std::ofstream data;
    data.open("slope_test_data");
    data << slope_map.str();
    data.close();
    
    INFO( "Trigdist is " << ( OPTIONS["CIRCLEDIST"] ? "on" : "off" ) );
    int zombie_horizontal_capture_speed = can_catch_player( "mon_zombie", {1,0,0} );
    WARN( zombie_horizontal_capture_speed );
    CHECK( zombie_horizontal_capture_speed < 0 );
    int zombie_vertical_capture_speed = can_catch_player( "mon_zombie", {1,1,0} );
    WARN( zombie_vertical_capture_speed );
    CHECK( zombie_vertical_capture_speed < 0 );
    int zombie_dog_horizontal_capture_speed = can_catch_player( "mon_zombie_dog", {1,0,0} );
    WARN( zombie_dog_horizontal_capture_speed );
    CHECK( zombie_dog_horizontal_capture_speed > 0 );
    int zombie_dog_vertical_capture_speed = can_catch_player( "mon_zombie_dog", {1,1,0} );
    WARN( zombie_dog_vertical_capture_speed );
    CHECK( zombie_dog_vertical_capture_speed > 0 );
}

// Characterization test for monster movement speed.
// It's not necessarally the one true speed for monsters, we just want notice if it changes.
TEST_CASE("monster_speed_square") {
    wipe_map_terrain();
    // Remove any interfering monsters.
    while( g->num_zombies() ) {
        g->remove_zombie( 0 );
    }
    OPTIONS["CIRCLEDIST"].setValue("false");
    trigdist = false;
    monster_check();
}

TEST_CASE("monster_speed_trig") {
    wipe_map_terrain();
    // Remove any interfering monsters.
    while( g->num_zombies() ) {
        g->remove_zombie( 0 );
    }
    OPTIONS["CIRCLEDIST"].setValue("true");
    trigdist = true;
    monster_check();
}
