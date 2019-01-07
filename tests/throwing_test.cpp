#include <ostream>
#include <vector>

#include "catch/catch.hpp"
#include "dispersion.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "player.h"
#include "projectile.h"
#include "test_statistics.h"

TEST_CASE( "throwing distance test", "[throwing], [balance]" )
{
    const standard_npc thrower( "Thrower", {}, 4, 10, 10, 10, 10 );
    item grenade( "grenade" );
    CHECK( thrower.throw_range( grenade ) >= 30 );
    CHECK( thrower.throw_range( grenade ) <= 35 );
}

struct throw_test_data {
    statistics<bool> hits;
    statistics<double> dmg;

    throw_test_data() : dmg( Z95 ) {}
};

struct throw_test_pstats {
    int skill_lvl;
    int str;
    int dex;
    int per;
};

std::ostream &operator<<( std::ostream &stream, const throw_test_pstats &pstats )
{
    return( stream << "STR: " << pstats.str << " DEX: " << pstats.dex <<
            " PER: " << pstats.per << " SKL: " << pstats.skill_lvl );
}

static const skill_id skill_throw = skill_id( "throw" );

static void reset_player( player &p, const throw_test_pstats &pstats, const tripoint &pos )
{
    p.reset();
    p.stamina = p.get_stamina_max();
    p.setpos( pos );
    p.str_max = pstats.str;
    p.dex_max = pstats.dex;
    p.per_max = pstats.per;
    p.set_str_bonus( 0 );
    p.set_per_bonus( 0 );
    p.set_dex_bonus( 0 );
    p.worn.clear();
    p.inv.clear();
    p.remove_weapon();
    p.set_skill_level( skill_throw, pstats.skill_lvl );
}

// If tests are routinely failing you should:
//  1. Make sure some change hasn't caused some regression
//  2. Make sure test is accurate by testing with a large minimum iterations (min > 5000)
//  3. Increase bounds on thresholds
//  4. Increase max iterations which will make the CI smaller and more likely to
//     fit inside the threshold but also increase the average test length
// In that order.
constexpr int min_throw_test_iterations = 100;
constexpr int max_throw_test_iterations = 10000;

// tighter thresholds here will increase accuracy but also increase average test
// time since more samples are required to get a more accurate test
void test_throwing_player_versus( player &p, const std::string &mon_id, const std::string &throw_id,
                                  const int range, const throw_test_pstats &pstats,
                                  const epsilon_threshold &hit_thresh, const epsilon_threshold &dmg_thresh,
                                  const int min_throws = min_throw_test_iterations,
                                  int max_throws = max_throw_test_iterations )
{
    const tripoint monster_start = { 30 + range, 30, 0 };
    const tripoint player_start = { 30, 30, 0 };
    bool hit_thresh_met = false;
    bool dmg_thresh_met = false;
    throw_test_data data;
    item it( throw_id );

    max_throws = std::max( min_throws, max_throws );
    do {
        reset_player( p, pstats, player_start );
        p.set_moves( 1000 );
        p.stamina = p.get_stamina_max();

        p.wield( it );
        monster &mon = spawn_test_monster( mon_id, monster_start );
        mon.set_moves( 0 );

        auto atk = p.throw_item( mon.pos(), it );
        data.hits.add( atk.hit_critter != nullptr );
        data.dmg.add( atk.dealt_dam.total_damage() );

        if( data.hits.n() >= min_throws ) {
            // ideally we should actually still checking the threshold after we
            // meet it but we're busy people and don't have time for that
            if( !hit_thresh_met ) {
                hit_thresh_met = data.hits.test_threshold( hit_thresh );
            }
            // don't do an else here because it's possible we just made
            // hit_thresh_met true
            if( hit_thresh_met ) {
                // commenting this out is a super easy way to force all the
                // test to fail if you want to reset the baseline after
                // making balance changes or if many of the tests are failing
                dmg_thresh_met = data.dmg.test_threshold( dmg_thresh );
            }
        }
        g->remove_zombie( mon );
        p.i_rem( -1 );
        // only need to check dmg_thresh_met because it can only be true if
        // hit_thresh_met first
    } while( !dmg_thresh_met && data.hits.n() < max_throws );

    INFO( "Monster: '" << mon_id << "' Item: '" << throw_id );
    INFO( "Range: " << range << " Pstats: " << pstats );
    INFO( "Total throws: " << data.hits.n() );
    INFO( "Ratio: " << data.hits.avg() * 100 << "%" );
    INFO( "Hit Lower: " << data.hits.lower() * 100 << "% Hit Upper: " << data.hits.upper() * 100 <<
          "%" );
    INFO( "Hit Thresh: " << ( hit_thresh.midpoint - hit_thresh.epsilon ) * 100 << "% - " <<
          ( hit_thresh.midpoint + hit_thresh.epsilon ) * 100 << "%" );
    INFO( "Adj Wald error: " << data.hits.margin_of_error() );
    INFO( "Avg total damage: " << data.dmg.avg() );
    INFO( "Dmg Lower: " << data.dmg.lower() << " Dmg Upper: " << data.dmg.upper() );
    INFO( "Dmg Thresh: " << dmg_thresh.midpoint - dmg_thresh.epsilon << " - " <<
          dmg_thresh.midpoint + dmg_thresh.epsilon );
    INFO( "Margin of error: " << data.hits.margin_of_error() );
    CHECK( dmg_thresh_met );
}

// Debugging function to force a large sample count.  This will fail but will
// dump an extremely accurate population sample that can be used to tune a
// broken test.
// WARNING: these will take a long time likely
void test_throwing_player_versus( player &p, const std::string &mon_id, const std::string &throw_id,
                                  const int range, const throw_test_pstats &pstats )
{
    test_throwing_player_versus( p, mon_id, throw_id, range, pstats, { 0, 0 }, { 0, 0 }, 5000, 5000 );
}

constexpr throw_test_pstats lo_skill_base_stats = { 0, 8, 8, 8 };
constexpr throw_test_pstats mid_skill_base_stats = { MAX_SKILL / 2, 8, 8, 8 };
constexpr throw_test_pstats hi_skill_base_stats = { MAX_SKILL, 8, 8, 8 };
constexpr throw_test_pstats hi_skill_athlete_stats = { MAX_SKILL, 12, 12, 12 };

TEST_CASE( "basic_throwing_sanity_tests", "[throwing],[balance]" )
{
    player &p = g->u;
    clear_map();

    SECTION( "test_player_vs_zombie_rock_basestats" ) {
        test_throwing_player_versus( p, "mon_zombie", "rock", 1, lo_skill_base_stats, { 0.78, 0.10 }, { 5, 3 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 5, lo_skill_base_stats, { 0.07, 0.10 }, { 0.7, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 10, lo_skill_base_stats, { 0.04, 0.10 }, { 0.5, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 15, lo_skill_base_stats, { 0.03, 0.10 }, { 0.5, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 20, lo_skill_base_stats, { 0.03, 0.10 }, { 0.5, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 25, lo_skill_base_stats, { 0.03, 0.10 }, { 0.5, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 30, lo_skill_base_stats, { 0.03, 0.10 }, { 0.5, 2 } );
    }

    SECTION( "test_player_vs_zombie_javelin_iron_basestats" ) {
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 1, lo_skill_base_stats, { 0.64, 0.10 }, { 11, 5 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 5, lo_skill_base_stats, { 0.05, 0.10 }, { 1.5, 3 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 10, lo_skill_base_stats, { 0.04, 0.10 }, { 1.50, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 15, lo_skill_base_stats, { 0.03, 0.10 }, { 1.29, 3 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 20, lo_skill_base_stats, { 0.03, 0.10 }, { 1.66, 2 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 25, lo_skill_base_stats, { 0.03, 0.10 }, { 1.0, 2 } );
    }

    SECTION( "test_player_vs_zombie_rock_athlete" ) {
        test_throwing_player_versus( p, "mon_zombie", "rock", 1, hi_skill_athlete_stats, { 1.00, 0.10 }, { 25.96, 8 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 5, hi_skill_athlete_stats, { 1.00, 0.10 }, { 21.59, 6 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 10, hi_skill_athlete_stats, { 1.00, 0.10 }, { 16.27, 6 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 15, hi_skill_athlete_stats, { 0.97, 0.10 }, { 12.83, 4 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 20, hi_skill_athlete_stats, { 0.82, 0.10 }, { 9.10, 4 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 25, hi_skill_athlete_stats, { 0.64, 0.10 }, { 6.54, 4 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 30, hi_skill_athlete_stats, { 0.47, 0.10 }, { 4.90, 3 } );
    }

    SECTION( "test_player_vs_zombie_javelin_iron_athlete" ) {
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 1, hi_skill_athlete_stats, { 1.00, 0.10 }, { 55.48, 8 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 5, hi_skill_athlete_stats, { 1.00, 0.10 }, { 44.81, 8 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 10, hi_skill_athlete_stats, { 1.00, 0.10 }, { 35.16, 8 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 15, hi_skill_athlete_stats, { 0.97, 0.10 }, { 25.21, 6 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 20, hi_skill_athlete_stats, { 0.82, 0.10 }, { 18.90, 5 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 25, hi_skill_athlete_stats, { 0.63, 0.10 }, { 13.59, 5 } );
        test_throwing_player_versus( p, "mon_zombie", "javelin_iron", 30, hi_skill_athlete_stats, { 0.48, 0.10 }, { 10.00, 4 } );
    }
}

TEST_CASE( "throwing_skill_impact_test", "[throwing],[balance]" )
{
    player &p = g->u;
    clear_map();

    // we already cover low stats in the sanity tests and we only cover a few
    // ranges here because what we're really trying to capture is the effect
    // the throwing skill has while the sanity tests are more explicit.
    SECTION( "mid_skill_basestats_rock" ) {
        test_throwing_player_versus( p, "mon_zombie", "rock", 5, mid_skill_base_stats, { 1.00, 0.10 }, { 12, 6 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 10, mid_skill_base_stats, { 0.86, 0.10 }, { 7.0, 4 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 15, mid_skill_base_stats, { 0.52, 0.10 }, { 3, 2 } );
    }

    SECTION( "hi_skill_basestats_rock" ) {
        test_throwing_player_versus( p, "mon_zombie", "rock", 5, hi_skill_base_stats, { 1.00, 0.10 }, { 18, 5 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 10, hi_skill_base_stats, { 1.00, 0.10 }, { 14.7, 5 } );
        test_throwing_player_versus( p, "mon_zombie", "rock", 15, hi_skill_base_stats, { 0.97, 0.10 }, { 10.5, 4 } );
    }
}

void test_player_kills_monster( player &p, const std::string &mon_id, const std::string &item_id,
                                const int range, const int dist_thresh, const throw_test_pstats &pstats,
                                const int iterations )
{
    const tripoint monster_start = { 30 + range, 30, 0 };
    const tripoint player_start = { 30, 30, 0 };
    int failure_turns = -1;
    int failure_num_items = -1;
    int failure_last_range = -1;
    item it( item_id );
    int num_failures = 0;

    // We want to be real sure it isn't possible so do it a bunch of times
    // until we manage to make it happen or if we hit iterations then we're
    // good.
    for( int i = 0; i < iterations; i++ ) {
        bool mon_is_dead = false;
        int turns = 0;
        int num_items = 0;
        int last_range = -1;

        reset_player( p, pstats, player_start );

        monster &mon = spawn_test_monster( mon_id, monster_start );
        mon.set_moves( 0 );

        while( !mon_is_dead ) {

            ++turns;
            mon.process_turn();
            mon.set_dest( p.pos() );
            while( mon.moves > 0 ) {
                mon.move();
            }

            // zombie made it to player, we're done with this iteration
            if( ( last_range = rl_dist( p.pos(), mon.pos() ) ) <= dist_thresh ) {
                break;
            }

            p.mod_moves( p.get_speed() );
            while( p.get_moves() > 0 ) {
                p.wield( it );
                p.throw_item( mon.pos(), it );
                p.i_rem( -1 );
                ++num_items;
            }
            mon_is_dead = mon.is_dead();
        }

        if( !mon_is_dead ) {
            g->remove_zombie( mon );
        } else {
            ++num_failures;
            failure_turns = turns;
            failure_num_items = num_items;
            failure_last_range = last_range;
        }
    }

    INFO( "You killed him :( He had kids you know." );
    INFO( "Distance - Start: " << range << " End: " << failure_last_range );
    INFO( "Turns: " << failure_turns );
    INFO( "# Items thrown: " << failure_num_items );
    CHECK( num_failures <= 1 );
}

TEST_CASE( "player_kills_zombie_before_reach", "[throwing],[balance][scenario]" )
{
    player &p = g->u;
    clear_map();

    SECTION( "test_player_kills_zombie_with_rock_basestats" ) {
        test_player_kills_monster( p, "mon_zombie", "rock", 15, 1, lo_skill_base_stats, 500 );
    }
}
