#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "options.h"
#include "options_helpers.h"
#include "overmapbuffer.h"
#include "point.h"
#include "sounds.h"
#include "test_statistics.h"
#include "type_id.h"

class item;

using move_statistics = statistics<int>;

static const mtype_id mon_dog_zombie_brute( "mon_dog_zombie_brute" );
static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_fence( "t_fence" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_water_dp( "t_water_dp" );


static int moves_to_destination( const std::string &monster_type,
                                 const tripoint_bub_ms &start, const tripoint_bub_ms &end )
{
    clear_creatures();
    REQUIRE( g->num_creatures() == 1 ); // the player
    monster &test_monster = spawn_test_monster( monster_type, start );
    // Get it riled up and give it a goal.
    test_monster.anger = 100;
    test_monster.set_dest( get_map().get_abs( end ) );
    test_monster.set_moves( 0 );
    const int monster_speed = test_monster.get_speed();
    int moves_spent = 0;
    for( int turn = 0; turn < 1000; ++turn ) {
        test_monster.mod_moves( monster_speed );

        while( test_monster.get_moves() >= 0 ) {
            test_monster.anger = 100;
            const int moves_before = test_monster.get_moves();
            test_monster.move();
            moves_spent += moves_before - test_monster.get_moves();
            if( test_monster.pos_bub() == end ) {
                g->remove_zombie( test_monster );
                return moves_spent;

                // return early if it doesn't move at all
            } else if( test_monster.pos_bub() == start ) {
                return 100000;
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
    for( const track &track_instance : vec ) {
        os << track_instance << " ";
    }
    return os;
}

/**
 * Simulate a player running from the monster, checking if it can catch up.
 **/
static int can_catch_player( const std::string &monster_type, const tripoint &direction_of_flight )
{
    map &here = get_map();
    clear_map();
    REQUIRE( g->num_creatures() == 1 ); // the player
    Character &test_player = get_player_character();
    // Strip off any potentially encumbering clothing.
    test_player.remove_worn_items_with( []( item & ) {
        return true;
    } );

    const tripoint_bub_ms center{ 65, 65, 0 };
    test_player.setpos( here, center );
    test_player.set_moves( 0 );
    // Give the player a head start.
    const tripoint_bub_ms monster_start = { -10 * direction_of_flight + test_player.pos_bub()
                                          };
    monster &test_monster = spawn_test_monster( monster_type, monster_start );
    // Get it riled up and give it a goal.
    test_monster.anger = 100;
    test_monster.set_dest( test_player.pos_abs() );
    test_monster.set_moves( 0 );
    const int monster_speed = test_monster.get_speed();
    const int target_speed = 100;

    std::vector<track> tracker;
    for( int turn = 0; turn < 1000; ++turn ) {
        test_player.mod_moves( target_speed );
        while( test_player.get_moves() >= 0 ) {
            test_player.setpos( test_player.pos_abs() + direction_of_flight );
            const tripoint_bub_ms pos = test_player.pos_bub( here );

            if( pos.x() < SEEX * static_cast<int>( MAPSIZE / 2 ) ||
                pos.y() < SEEY * static_cast<int>( MAPSIZE / 2 ) ||
                pos.x() >= SEEX * ( 1 + static_cast<int>( MAPSIZE / 2 ) ) ||
                pos.y() >= SEEY * ( 1 + static_cast<int>( MAPSIZE / 2 ) ) ) {
                tripoint_rel_ms offset = center - test_player.pos_bub();
                test_player.setpos( here, center );
                test_monster.setpos( test_monster.pos_abs() + offset );
                // Verify that only the player and one monster are present.
                REQUIRE( g->num_creatures() == 2 );
            }
            const int move_cost = here.combined_movecost(
                                      test_player.pos_bub(), test_player.pos_bub() + direction_of_flight, nullptr, 0 );
            tracker.push_back( {'p', move_cost, rl_dist( test_monster.pos_bub(), test_player.pos_bub() ),
                                test_player.pos_bub().raw()
                               } );
            test_player.mod_moves( -move_cost );
        }
        here.clear_traps();
        test_monster.set_dest( test_player.pos_abs() );
        test_monster.mod_moves( monster_speed );
        while( test_monster.get_moves() >= 0 ) {
            const int moves_before = test_monster.get_moves();
            test_monster.move();
            tracker.push_back( {'m', moves_before - test_monster.get_moves(),
                                rl_dist( test_monster.pos_bub(), test_player.pos_bub() ),
                                test_monster.pos_bub().raw()
                               } );
            if( rl_dist( test_monster.pos_bub(), test_player.pos_bub() ) == 1 ) {
                INFO( tracker );
                clear_map();
                return turn;
            } else if( rl_dist( test_monster.pos_bub(), test_player.pos_bub() ) > 20 ) {
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
static void check_shamble_speed( const std::string &monster_type,
                                 const tripoint_bub_ms &destination )
{
    // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
    const float slope = get_normalized_angle( point::zero, destination.xy().raw() );
    const float diagonal_multiplier = 1.0 + ( get_option<bool>( "CIRCLEDIST" ) ?
                                      ( slope * 0.41 ) : 0.0 );
    INFO( monster_type << " " << destination );
    // Wandering makes things nondeterministic, so look at the distribution rather than a target number.
    move_statistics move_stats;
    for( int i = 0; i < 10; ++i ) {
        move_stats.add( moves_to_destination( monster_type, tripoint_bub_ms::zero, destination ) );
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
        for( int sample : stat_pair.second.get_samples() ) {
            sample_string << sample << ", ";
        }
        INFO( "Monster:" << monster_type << " Angle: " << stat_pair.first <<
              " moves: " << stat_pair.second.avg() << " types: " << stat_pair.second.types() <<
              " samples: " << sample_string.str() );
        CHECK( stat_pair.second.avg() == Approx( 100.0 ).epsilon( 0.1 ) );
    }

    if( write_data ) {
        std::ofstream data;
        data.open( std::filesystem::u8path( "slope_test_data_" + std::string( (
                                                trigdist ? "trig_" : "square_" ) ) +
                                            monster_type ) );
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
    float vert_move = moves_to_destination( "mon_pig", tripoint_bub_ms::zero, {100, 0, 0} );
    CHECK( ( vert_move / 10000.0 ) == Approx( 1.0 ) );
    int horiz_move = moves_to_destination( "mon_pig", tripoint_bub_ms::zero, {0, 100, 0} );
    CHECK( ( horiz_move / 10000.0 ) == Approx( 1.0 ) );
    int diag_move = moves_to_destination( "mon_pig", tripoint_bub_ms::zero, {100, 100, 0} );
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
    CHECK( can_catch_player( "mon_zombie", tripoint::east ) < 0 );
    CHECK( can_catch_player( "mon_zombie", tripoint::south_east ) < 0 );
    CHECK( can_catch_player( "mon_zombie_dog", tripoint::east ) > 0 );
    CHECK( can_catch_player( "mon_zombie_dog", tripoint::south_east ) > 0 );
}

TEST_CASE( "check_mon_id" )
{
    for( const mtype &mon : MonsterGenerator::generator().get_all_mtypes() ) {
        if( !mon.src.empty() && mon.src.back().second.str() != "dda" ) {
            continue;
        }
        std::string mon_id = mon.id.str();
        std::string suffix_id = mon_id.substr( 0, mon_id.find( '_' ) );
        INFO( "Now checking the id of " << mon.id.str() );
        CHECK( ( suffix_id == "mon"  || suffix_id == "pseudo" ) );
    }
}

// Write out a map of slope at which monster is moving to time required to reach their destination.
TEST_CASE( "write_slope_to_speed_map_trig", "[.]" )
{
    clear_map_and_put_player_underground();
    restore_on_out_of_scope restore_trigdist( trigdist );
    override_option opt( "CIRCLEDIST", "true" );
    trigdist = true;
    test_moves_to_squares( "mon_zombie_dog", true );
    test_moves_to_squares( "mon_pig", true );
}

TEST_CASE( "write_slope_to_speed_map_square", "[.]" )
{
    clear_map_and_put_player_underground();
    restore_on_out_of_scope restore_trigdist( trigdist );
    override_option opt( "CIRCLEDIST", "false" );
    trigdist = false;
    test_moves_to_squares( "mon_zombie_dog", true );
    test_moves_to_squares( "mon_pig", true );
}

// Characterization test for monster movement speed.
// It's not necessarally the one true speed for monsters, we just want notice if it changes.
TEST_CASE( "monster_speed_square", "[speed]" )
{
    clear_map_and_put_player_underground();
    restore_on_out_of_scope restore_trigdist( trigdist );
    override_option opt( "CIRCLEDIST", "false" );
    trigdist = false;
    monster_check();
}

TEST_CASE( "monster_speed_trig", "[speed]" )
{
    clear_map_and_put_player_underground();
    restore_on_out_of_scope restore_trigdist( trigdist );
    override_option opt( "CIRCLEDIST", "true" );
    trigdist = true;
    monster_check();
}

TEST_CASE( "monster_special_move", "[speed]" )
{
    clear_map_and_put_player_underground();

    map &here = get_map();
    const tripoint_bub_ms from = tripoint_bub_ms::zero;
    const tripoint_bub_ms to = {1, 0, 0};


    GIVEN( "CLIMBABLE terrain" ) {
        const int ter_mod = 3;
        REQUIRE( here.ter_set( to, ter_t_fence ) );
        ter_id ter = here.ter( to );
        ter_id ter_from = here.ter( from );
        REQUIRE( ter->id == ter_t_fence );

        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_CLIMBABLE, to ) );

        ;

        AND_GIVEN( "Non-climbing monster" ) {
            const std::string &non_climber = "mon_pig";
            const mtype_id nclimber_id( non_climber );
            REQUIRE( !nclimber_id->move_skills.climb.has_value() );
            REQUIRE( !nclimber_id->has_flag( mon_flag_CLIMBS ) );

            WHEN( "Moving to climbable tile" ) {
                const int moves = moves_to_destination( non_climber, from, to );

                THEN( "The monster cannot go there" ) {
                    CHECK( moves == 100000 );
                }
            }
        }
        ;
        AND_GIVEN( "Climbing monster" ) {
            const std::string &climber = "mon_test_climb_nobash";
            const mtype_id climber_id( climber );
            const int mon_skill = 8;
            REQUIRE( climber_id->move_skills.climb.has_value() );

            AND_GIVEN( "the monster has the known climbskill" ) {
                REQUIRE( climber_id->move_skills.climb.value() == mon_skill );
            }
            AND_GIVEN( "the terrain has a known movemod" ) {
                REQUIRE( ter->movecost == ter_mod );
            }

            AND_WHEN( "Moving to climbable tile" ) {
                const int moves = moves_to_destination( climber, from, to );

                THEN( "It took the correct amount of moves" ) {
                    const int expected_mod = ter_mod * ( 10 - mon_skill );
                    INFO( "if the expected formula changes, change here too expected_mod = ter_mod * ( 10 - mon_skill )" );
                    INFO( "expected_movecost = ( ( expected_mod * 50 ) + 100 ) / 2" );
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    const int expected_movecost = ( expected_mod + 2 ) * 25;
                    INFO( "expected: " << expected_movecost ) ;
                    INFO( "actual: " << moves );
                    CHECK( moves == expected_movecost );
                }
            }
        }
    }

    GIVEN( "SWIMMABLE terrain" ) {
        const int ter_mod = 8;

        REQUIRE( here.ter_set( to, ter_t_water_dp ) );
        ter_id ter = here.ter( to );
        ter_id ter_from = here.ter( from );
        REQUIRE( ter->id == ter_t_water_dp );

        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, to ) );

        ;

        AND_GIVEN( "Non-swimming breathing monster" ) {
            const std::string &non_swimmer = "mon_pig";
            const mtype_id non_swimmer_id( non_swimmer );
            REQUIRE( !non_swimmer_id->move_skills.swim.has_value() );
            REQUIRE( !non_swimmer_id->has_flag( mon_flag_SWIMS ) );
            REQUIRE( !non_swimmer_id->has_flag( mon_flag_AQUATIC ) );

            WHEN( "Moving to swimable tile" ) {
                const int moves = moves_to_destination( non_swimmer, from, to );

                THEN( "The monster cannot go there" ) {
                    CHECK( moves == 100000 );
                }
            }
        }
        AND_GIVEN( "NOBREATH monster" ) {
            const std::string &zed = "mon_zombie";
            const mtype_id zed_id( zed );
            REQUIRE_FALSE( zed_id->move_skills.swim.has_value() );
            REQUIRE_FALSE( zed_id->has_flag( mon_flag_SWIMS ) );
            REQUIRE( zed_id->has_flag( mon_flag_NO_BREATHE ) );

            WHEN( "Moving to swimable tile" ) {
                const int moves = moves_to_destination( zed, from, to );

                THEN( "The monster is impacted by terrain-cost" ) {
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    const int expected_movecost = ( ter->movecost + ter_from->movecost ) * 25 ;
                    INFO( "expected: " << expected_movecost ) ;
                    INFO( "actual: " << moves );
                    CHECK( moves == expected_movecost );
                }
            }
        }

        AND_GIVEN( "AQUATIC monster and from ter is swimmable" ) {
            ;
            REQUIRE( here.ter_set( from, ter_t_water_dp ) );
            const std::string &swimmer = "mon_fish_brook_trout";
            const mtype_id swimmer_id( swimmer );
            const int mon_skill = 10;
            REQUIRE( swimmer_id->move_skills.swim.has_value() );
            REQUIRE( swimmer_id->has_flag( mon_flag_AQUATIC ) );
            AND_GIVEN( "the monster has the known swimskill" ) {
                REQUIRE( swimmer_id->move_skills.swim.value() == mon_skill );
            }
            AND_GIVEN( "the terrain has a known movemod" ) {
                REQUIRE( ter->movecost == ter_mod );
            }

            AND_WHEN( "Moving to swimable tile" ) {
                const int moves = moves_to_destination( swimmer, from, to );

                THEN( "It took the correct amount of moves" ) {
                    INFO( "SWIMMERS ignore terraincost, resulting in the formular std::max( ( 10 - mon_skill ) * 50, 25 )" );
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    const int expected_movecost =  std::max( ( 10 - mon_skill ) * 50, 25 ) ;
                    INFO( "expected: " << expected_movecost ) ;
                    INFO( "actual: " << moves );
                    CHECK( moves == expected_movecost );
                }
            }

            AND_WHEN( "trying to move onto land" ) {
                ;
                REQUIRE( here.ter_set( to,  ter_t_grass ) );
                ter = here.ter( to );
                ter_from = here.ter( from );
                REQUIRE_FALSE( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, to ) );
                const int moves = moves_to_destination( swimmer, from, to );

                THEN( "It cannot move" ) {
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    CHECK( moves == 100000 );
                }
            }
        }
    }

    GIVEN( "DIGGABLE terrain" ) {
        ter_id ter = here.ter( to );
        ter_id ter_from = here.ter( from );

        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, to ) );

        ;
        AND_GIVEN( "DIGGING monster and from ter is digmable" ) {
            const std::string &digger = "mon_yugg";
            const mtype_id digger_id( digger );
            const int mon_skill = 9;
            REQUIRE( digger_id->move_skills.dig.has_value() );
            AND_GIVEN( "the monster has the known digskill" ) {
                REQUIRE( digger_id->move_skills.dig.value() == mon_skill );
            }
            AND_WHEN( "Moving to digable tile" ) {
                const int moves = moves_to_destination( digger, from, to );

                THEN( "It took the correct amount of moves" ) {
                    INFO( "Diggers, just like swimmers ignore terraincost.  movecost = std::max( ( 10 - mon_skill ) * 50, 25 )" );
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    const int expected_movecost =  std::max( ( 10 - mon_skill ) * 50, 25 ) ;
                    INFO( "expected: " << expected_movecost ) ;
                    INFO( "actual: " << moves );
                    CHECK( moves == expected_movecost );
                }
            }
            AND_WHEN( "trying to move into non-diggable terrain" ) {
                ;
                REQUIRE( here.ter_set( to,  ter_t_water_dp ) );
                ter = here.ter( to );
                ter_from = here.ter( from );
                REQUIRE_FALSE( here.has_flag( ter_furn_flag::TFLAG_DIGGABLE, to ) );
                const int moves = moves_to_destination( digger, from, to );

                THEN( "It cannot move" ) {
                    INFO( "from: " << ter_from->name() << " to " << ter->name() );
                    CHECK( moves == 100000 );
                }
            }
        }
    }
}

TEST_CASE( "monster_extend_flags", "[monster]" )
{
    // mon_dog_zombie_brute is copy-from mon_dog_zombie_rot
    // mon_dog_zombie_rot contains
    // "flags": [ "SEES", "HEARS", "SMELLS", "STUMBLES", "WARM", "BASHES", "POISON", "NO_BREATHE", "REVIVES", "PUSH_MON", "FILTHY" ]
    // mon_dog_zombie_brute contains
    // "extend": { "flags": [ "GROUP_BASH", "PUSH_VEH" ] }

    // This test verifies that "extend" works on monster flags by checking both
    // those take effect
    const mtype &m = *mon_dog_zombie_brute;
    CHECK( m.has_flag( mon_flag_SEES ) );
    CHECK( m.has_flag( mon_flag_PUSH_VEH ) );
}

TEST_CASE( "monster_broken_verify", "[monster]" )
{
    // verify monsters with death_function = BROKEN
    // actually have appropriate broken_name items
    const MonsterGenerator &generator = MonsterGenerator::generator();
    for( const mtype &montype : generator.get_all_mtypes() ) {
        if( montype.mdeath_effect.corpse_type != mdeath_type::BROKEN ) {
            continue;
        }

        // this contraption should match mdeath::broken in mondeath.cpp
        std::string broken_id_str = montype.id.str();
        if( broken_id_str.compare( 0, 4, "mon_" ) == 0 ) {
            broken_id_str.erase( 0, 4 );
        }
        broken_id_str.insert( 0, "broken_" ); // "broken_manhack", or "broken_eyebot", ...
        const itype_id targetitemid( broken_id_str );

        CAPTURE( montype.id.c_str() );
        CHECK( targetitemid.is_valid() );
    }
}

TEST_CASE( "limit_mod_size_bonus", "[monster]" )
{
    clear_creatures();
    const std::string monster_type = "mon_zombie";
    monster &test_monster = spawn_test_monster( monster_type, tripoint_bub_ms::zero );

    REQUIRE( test_monster.get_size() == creature_size::medium );

    test_monster.mod_size_bonus( -3 );
    CHECK( test_monster.get_size() == creature_size::tiny );

    clear_creatures();

    const std::string monster_type2 = "mon_feral_human_pipe";
    monster &test_monster2 = spawn_test_monster( monster_type2, tripoint_bub_ms::zero );

    REQUIRE( test_monster2.get_size() == creature_size::medium );

    test_monster2.mod_size_bonus( 3 );
    CHECK( test_monster2.get_size() == creature_size::huge );
}

TEST_CASE( "monsters_spawn_eggs", "[monster][reproduction]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms loc = get_avatar().pos_bub() + tripoint::east;
    monster &test_monster = spawn_test_monster( "mon_dummy_reproducer_eggs", loc );
    bool test_monster_spawns_eggs = false;
    int amount_of_iteration = 0;
    while( amount_of_iteration < 100 ) {
        test_monster.set_baby_timer( calendar::turn - 2_days );
        test_monster.try_reproduce();
        if( here.has_items( loc ) ) {
            test_monster_spawns_eggs = true;
            break;
        } else {
            amount_of_iteration++;
        }
    }
    CAPTURE( amount_of_iteration );
    CHECK( test_monster_spawns_eggs );
}

TEST_CASE( "monsters_spawn_egg_itemgroups", "[monster][reproduction]" )
{
    clear_map();
    map &here = get_map();
    tripoint_bub_ms loc = get_avatar().pos_bub() + tripoint::east;
    monster &test_monster = spawn_test_monster( "mon_dummy_reproducer_egg_group", loc );
    bool test_monster_spawns_egg_group = false;
    int amount_of_iteration = 0;
    while( amount_of_iteration < 100 ) {
        test_monster.set_baby_timer( calendar::turn - 2_days );
        test_monster.try_reproduce();
        if( here.has_items( loc ) ) {
            test_monster_spawns_egg_group = true;
            break;
        } else {
            amount_of_iteration++;
        }
    }
    CAPTURE( amount_of_iteration );
    CHECK( test_monster_spawns_egg_group );
}

TEST_CASE( "monsters_spawn_babies", "[monster][reproduction]" )
{
    clear_map();
    creature_tracker &creatures = get_creature_tracker();
    tripoint_bub_ms loc = get_avatar().pos_bub() + tripoint::east;
    monster &test_monster = spawn_test_monster( "mon_dummy_reproducer_mon", loc );
    bool test_monster_spawns_babies = false;
    int amount_of_iteration = 0;
    while( amount_of_iteration < 100 ) {
        test_monster.set_baby_timer( calendar::turn - 2_days );
        test_monster.try_reproduce();
        get_map().spawn_monsters( true );
        if( creatures.get_monsters_list().size() > 1 ) {
            test_monster_spawns_babies = true;
            break;
        } else {
            amount_of_iteration++;
        }
    }
    CAPTURE( amount_of_iteration );
    CHECK( test_monster_spawns_babies );
}

TEST_CASE( "monsters_spawn_baby_groups", "[monster][reproduction]" )
{
    clear_map();
    creature_tracker &creatures = get_creature_tracker();
    tripoint_bub_ms loc = get_avatar().pos_bub() + tripoint::east;
    monster &test_monster = spawn_test_monster( "mon_dummy_reproducer_mon_group", loc );
    bool test_monster_spawns_baby_mongroup = false;
    int amount_of_iteration = 0;
    while( amount_of_iteration < 100 ) {
        test_monster.set_baby_timer( calendar::turn - 2_days );
        test_monster.try_reproduce();
        get_map().spawn_monsters( true );
        if( creatures.get_monsters_list().size() > 1 ) {
            test_monster_spawns_baby_mongroup = true;
            break;
        } else {
            amount_of_iteration++;
        }
    }
    CAPTURE( amount_of_iteration );
    CHECK( test_monster_spawns_baby_mongroup );
}

static void test_move_to_location( monster &test_monster, const tripoint_bub_ms &destination )
{
    tripoint_bub_ms old_location = test_monster.pos_bub();
    int steps = 0;
    while( test_monster.pos_bub() != destination ) {
        test_monster.set_moves( 100 );
        test_monster.anger = 100;
        test_monster.wandf = 100;
        // Monsters can do silly things like trigger a no-op special attack instead of moving.
        // So keep trying until the monster does something meaningful.
        while( test_monster.get_moves() >= 0 && test_monster.pos_bub() == old_location && steps < 1000 ) {
            test_monster.move();
            steps++;
        }
        tripoint_bub_ms new_location = test_monster.pos_bub();
        if( new_location == destination ) {
            SUCCEED();
            return;
        }
        if( new_location == old_location || steps > 1000 ) {
            CAPTURE( steps );
            CAPTURE( destination );
            CAPTURE( old_location );
            CAPTURE( new_location );
            CAPTURE( get_player_character().pos_bub() );
            FAIL();
        }
        old_location = new_location;
    }
}


static void monster_can_move_to_map_center( const tripoint_bub_ms &origin )
{
    // Head for map center?
    const tripoint_bub_ms destination{ 11 * 6, 11 * 6, 0 };
    clear_creatures();
    REQUIRE( g->num_creatures() == 1 ); // the player
    monster &test_monster = spawn_test_monster( "mon_zombie", origin );
    map &m = get_map();
    test_monster.anger = 100;
    // TODO: check wander too
    test_monster.set_dest( m.get_abs( destination ) );
    test_move_to_location( test_monster, destination );
}

// This is a pathological test for an optimization added to mattack::parrot_at_danger because the monster
// does a flood-fill every time it tries to move instead of reusing an existing flood fill.
// Possibly this is because there's context that we normally set up first that this test is missing.
TEST_CASE( "monster_can_navigate_from_anywhere_in_reality_bubble", "[monster]" )
{
    // Remove interacting with the player as a complication.
    clear_map_and_put_player_underground();
    map &m = get_map();
    for( tripoint_bub_ms start_loc : m.points_on_zlevel( 0 ) ) {
        monster_can_move_to_map_center( start_loc );
    }
}

TEST_CASE( "monster_can_navigate_from_overmap_to_reality_bubble", "[monster][hordes]" )
{
    // Remove interacting with the player as a complication.
    clear_map_and_put_player_underground();
    const tripoint_bub_ms destination{ 11 * 6, 11 * 6, 0 };
    // Place monster on the local overmap.monster_map just outside the reality bubble.
    map &m = get_map();
    monster &test_mon = overmap_buffer.spawn_monster( m.get_abs( { -12, 66, 0 } ), mon_zombie );
    // Give the monster a goal location inside the bubble.
    test_mon.set_dest( m.get_abs( destination ) );
    // This reference will be invalidated once the monster spawns in the reality bubble,
    // don't access it again after calling move_hordes().
    // Process hordes and verify the monster appears on the reality bubble.
    do {
        overmap_buffer.move_hordes();
    } while( g->num_creatures() == 1 );
    monster &local_test_monster = *g->all_monsters().items.front().lock();
    test_move_to_location( local_test_monster, destination );
}

TEST_CASE( "monster_can_navigate_from_overmap_to_reality_bubble_following_sound",
           "[monster][hordes][sound]" )
{
    // Remove interacting with the player as a complication.
    clear_map_and_put_player_underground();
    const tripoint_bub_ms destination{ 11 * 6, 11 * 6, 0 };
    // Place monster on the local overmap.monster_map just outside the reality bubble.
    map &m = get_map();
    monster &test_mon = overmap_buffer.spawn_monster( m.get_abs( { -12, 66, 0 } ), mon_zombie );
    // Assert monster is not wandering
    REQUIRE( test_mon.wandf == 0 );
    // Give the monster a goal location inside the bubble by making a loud noise.
    std::string test_sound( "test sound" );
    sound( destination, 200, sounds::sound_t::combat, test_sound );
    sounds::process_sounds();
    // Assert monster is wandering
    REQUIRE( test_mon.wandf != 0 );
    // This reference will be invalidated once the monster spawns in the reality bubble,
    // don't access it again after calling move_hordes().
    // Process hordes and verify the monster appears on the reality bubble.
    do {
        overmap_buffer.move_hordes();
    } while( g->num_creatures() == 1 );
    monster &local_test_monster = *g->all_monsters().items.front().lock();
    test_move_to_location( local_test_monster, destination );
}

TEST_CASE( "monster_moved_to_overmap_after_map_shift", "[monster][hordes]" )
{
    clear_map();
    map &here = get_map();
    // Place character in the central submap of map.
    tripoint_bub_ms player_start_pos{ 11 * 6, 11 * 6, 0 };
    get_player_character().setpos( here, player_start_pos );

    const tripoint_bub_ms destination{ 11 * 6, 11 * 6, 0 };
    const tripoint_abs_ms abs_destination = here.get_abs( destination );
    // Place monster on the left edge of the reality bubble.
    monster &test_monster = spawn_test_monster( "mon_zombie", { 0, 11 * 6, 0 } );
    // Give the monster a goal location targting the player.
    test_monster.set_dest( abs_destination );
    // Move player to right, shifting the map so the monster is no longer present.
    int num_steps = 0;
    while( g->num_creatures() != 1 && num_steps < 36 ) {
        ++num_steps;
        g->walk_move( get_player_character().pos_bub() + point::east );
        for( monster &critter : g->all_monsters() ) {
            // TODO better check here, stash a label in the spawned monster and check for that?
            if( &critter != &test_monster ) {
                g->remove_zombie( critter );
            }
        }
    }
    // In case something goes wrong, just how far did we move?
    CAPTURE( num_steps );
    // Verify that we shifted the monster off the map.
    REQUIRE( g->num_creatures() == 1 );

    do {
        // TODO failsafe so this breaks out and fails instead of infinite loop.
        overmap_buffer.move_hordes();
    } while( g->num_creatures() == 1 );
    monster &local_test_monster = *g->all_monsters().items.front().lock();
    test_move_to_location( local_test_monster, here.get_bub( abs_destination ) );
}


TEST_CASE( "monster_cant_enter_reality_bubble_because_wall", "[monster][hordes]" )
{
    // Remove interacting with the player as a complication.
    clear_map_and_put_player_underground();
    const tripoint_bub_ms destination{ 11 * 6, 11 * 6, 0 };
    // Place monster on the local overmap.monster_map just outside the reality bubble.
    map &m = get_map();
    monster &test_mon = overmap_buffer.spawn_monster( m.get_abs( { -24, 66, 0 } ), mon_zombie );
    // Give the monster a goal location inside the bubble.
    test_mon.set_dest( m.get_abs( destination ) );
    // Put a wall between the monster and the overmap so they can't enter.
    for( int i = -12; i < 12; ++i ) {
        overmap_buffer.set_passable( m.get_abs( { -12, 66 + i, 0 } ), false );
    }
    // This reference will be invalidated once the monster spawns in the reality bubble,
    // don't access it again after calling move_hordes().
    // Process hordes and verify the monster doesn't make it onto the bubble.
    int step_attempts = 0;
    do {
        step_attempts++;
        overmap_buffer.move_hordes();
    } while( g->num_creatures() == 1 && step_attempts < 100 );
    CAPTURE( step_attempts );
    REQUIRE( g->num_creatures() == 1 );
}
