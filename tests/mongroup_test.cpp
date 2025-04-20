#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "enums.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

static const mongroup_id GROUP_PETS( "GROUP_PETS" );
static const mongroup_id GROUP_PET_DOGS( "GROUP_PET_DOGS" );

static const mtype_id mon_null( "mon_null" );
static const mtype_id mon_test_CBM( "mon_test_CBM" );
static const mtype_id mon_test_bovine( "mon_test_bovine" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );
static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_speed_desc_base( "mon_test_speed_desc_base" );
static const mtype_id mon_test_speed_desc_base_immobile( "mon_test_speed_desc_base_immobile" );
static const mtype_id mon_test_zombie_cop( "mon_test_zombie_cop" );

static const int max_iters = 10000;

static void spawn_x_monsters( int x, const mongroup_id &grp, const std::vector<mtype_id> &yesspawn,
                              const std::vector<mtype_id> &nospawn )
{
    std::set<mtype_id> rand_gets;
    std::set<mtype_id> rand_results;
    calendar::turn = time_point( 1 );
    for( int i = 0; i < x; i++ ) {
        mtype_id tmp_get = MonsterGroupManager::GetRandomMonsterFromGroup( grp );
        if( !tmp_get.is_null() ) {
            rand_gets.emplace( tmp_get );
        }

        mtype_id tmp_res = MonsterGroupManager::GetResultFromGroup( grp ).front().id;
        if( !tmp_res.is_null() ) {
            rand_results.emplace( tmp_res );
        }
    }
    if( nospawn.empty() ) {
        CHECK( !rand_gets.empty() );
        CHECK( !rand_results.empty() );
    }
    if( yesspawn.empty() ) {
        CHECK( rand_gets.empty() );
        CHECK( rand_results.empty() );
    }
    for( const mtype_id &m : nospawn ) {
        CHECK( rand_gets.find( m ) == rand_gets.end() );
        CHECK( rand_results.find( m ) == rand_results.end() );
    }
    for( const mtype_id &m : yesspawn ) {
        CHECK( rand_gets.find( m ) != rand_gets.end() );
        CHECK( rand_results.find( m ) != rand_results.end() );
    }
}

TEST_CASE( "Event-based_monster_spawns_do_not_spawn_outside_event", "[monster][mongroup]" )
{
    override_option ev_spawn_opt( "EVENT_SPAWNS", "monsters" );
    REQUIRE( get_option<std::string>( "EVENT_SPAWNS" ) == "monsters" );

    GIVEN( "a monster group with entries from different events" ) {
        mongroup_id test_group( "test_event_mongroup" );
        REQUIRE( test_group->monsters.size() == 6 );

        WHEN( "current holiday == none" ) {
            holiday cur_event = get_holiday_from_time( 1614574800L, true );
            REQUIRE( cur_event == holiday::none );
            THEN( "only spawn non-event monsters" ) {
                std::vector<mtype_id> mons = MonsterGroupManager::GetMonstersFromGroup( test_group, true );
                CHECK( mons.size() == 1 );
                CHECK( mons[0] == mon_test_speed_desc_base_immobile );

                const std::vector<mtype_id> ylist = { mon_test_speed_desc_base_immobile };
                const std::vector<mtype_id> nlist = { mon_test_speed_desc_base, mon_test_shearable, mon_test_non_shearable, mon_test_bovine, mon_test_CBM };
                spawn_x_monsters( max_iters, test_group, ylist, nlist );
            }
        }

        WHEN( "current holiday == christmas" ) {
            holiday cur_event = get_holiday_from_time( 1640408400L, true );
            REQUIRE( cur_event == holiday::christmas );
            THEN( "only spawn christmas and non-event monsters" ) {
                std::vector<mtype_id> mons = MonsterGroupManager::GetMonstersFromGroup( test_group, true );
                CHECK( mons.size() == 4 );

                const std::vector<mtype_id> ylist = { mon_test_speed_desc_base_immobile, mon_test_non_shearable, mon_test_shearable, mon_test_bovine };
                const std::vector<mtype_id> nlist = { mon_test_speed_desc_base, mon_test_CBM };
                spawn_x_monsters( max_iters, test_group, ylist, nlist );
            }
        }

        WHEN( "current holiday == halloween" ) {
            holiday cur_event = get_holiday_from_time( 1635652800L, true );
            REQUIRE( cur_event == holiday::halloween );
            THEN( "only spawn halloween and non-event monsters" ) {
                std::vector<mtype_id> mons = MonsterGroupManager::GetMonstersFromGroup( test_group, true );
                CHECK( mons.size() == 3 );

                const std::vector<mtype_id> ylist = { mon_test_speed_desc_base_immobile, mon_test_CBM, mon_test_speed_desc_base };
                const std::vector<mtype_id> nlist = { mon_test_shearable, mon_test_non_shearable, mon_test_bovine };
                spawn_x_monsters( max_iters, test_group, ylist, nlist );
            }
        }
    }
}

TEST_CASE( "Event-based_monsters_from_an_event-only_mongroup", "[monster][mongroup]" )
{
    override_option ev_spawn_opt( "EVENT_SPAWNS", "monsters" );
    REQUIRE( get_option<std::string>( "EVENT_SPAWNS" ) == "monsters" );

    GIVEN( "a monster group with entries from a single event" ) {
        mongroup_id test_group( "test_event_only" );
        REQUIRE( test_group->monsters.size() == 3 );

        WHEN( "current holiday == none" ) {
            holiday cur_event = get_holiday_from_time( 1614574800L, true );
            REQUIRE( cur_event == holiday::none );
            THEN( "don't spawn any monsters" ) {
                std::vector<mtype_id> mons = MonsterGroupManager::GetMonstersFromGroup( test_group, true );
                CHECK( mons.empty() );

                const std::vector<mtype_id> ylist = {};
                const std::vector<mtype_id> nlist = { mon_test_non_shearable, mon_test_shearable, mon_test_bovine };
                spawn_x_monsters( max_iters, test_group, ylist, nlist );
            }
        }

        WHEN( "current holiday == christmas" ) {
            holiday cur_event = get_holiday_from_time( 1640408400L, true );
            REQUIRE( cur_event == holiday::christmas );
            THEN( "spawn all monsters" ) {
                std::vector<mtype_id> mons = MonsterGroupManager::GetMonstersFromGroup( test_group, true );
                CHECK( mons.size() == 3 );

                const std::vector<mtype_id> ylist = { mon_test_non_shearable, mon_test_shearable, mon_test_bovine };
                const std::vector<mtype_id> nlist = {};
                spawn_x_monsters( max_iters, test_group, ylist, nlist );
            }
        }
    }
}

TEST_CASE( "Using_mon_null_as_mongroup_default_monster", "[mongroup]" )
{
    mongroup_id test_group1( "test_default_monster" );
    mongroup_id test_group2( "test_group_default_monster" );
    mongroup_id test_group3( "test_event_only" );
    mongroup_id test_group4( "test_event_mongroup" );
    CHECK( test_group1->defaultMonster == mon_null );
    CHECK( test_group2->defaultMonster == mon_null );
    CHECK( test_group3->defaultMonster == mon_null );
    CHECK( test_group4->defaultMonster != mon_null );
}

TEST_CASE( "Nested_monster_groups_spawn_chance", "[mongroup]" )
{
    mongroup_id mg( "test_top_level_mongroup" );

    const int iters = 10000;

    // < result, < layer, expected prob, count > >
    std::map<mtype_id, std::tuple<int, float, int>> results {
        // top layer - 33.3%
        { mon_test_CBM,                      { 0, 1.f / 3.f, 0 } },
        { mon_test_bovine,                   { 0, 1.f / 3.f, 0 } },
        // nested layer 1 - 15.9% (47.6%)
        { mon_test_shearable,                { 1, ( 1.f / 3.f ) *( 50.f / 105.f ), 0 } },
        { mon_test_non_shearable,            { 1, ( 1.f / 3.f ) *( 50.f / 105.f ), 0 } },
        // nested layer 2 - 0.8% (2.4% (50%))
        { mon_test_speed_desc_base,          { 2, ( ( 1.f / 3.f ) * ( 5.f / 105.f ) ) * 0.5f, 0 } },
        { mon_test_speed_desc_base_immobile, { 2, ( ( 1.f / 3.f ) * ( 5.f / 105.f ) ) * 0.5f, 0 } }
    };

    // < layer, < expected prob, count > >
    std::map<int, std::pair<float, int>> layers {
        // top layer - 66.7%
        { 0, { 2.f / 3.f, 0 } },
        // nested layer 1 - 31.7% (95.2%)
        { 1, { ( 1.f / 3.f ) *( 100.f / 105.f ), 0 } },
        // nested layer 2 - 1.6% (4.8% (100%))
        { 2, { ( 1.f / 3.f ) *( 5.f / 105.f ), 0 } }
    };

    calendar::turn += 1_turns;

    for( int i = 0; i < iters; i++ ) {
        MonsterGroupResult res = MonsterGroupManager::GetResultFromGroup( mg ).front();
        auto iter = results.find( res.id );
        CAPTURE( res.id.c_str() );
        REQUIRE( iter != results.end() );
        if( iter != results.end() ) {
            layers[std::get<0>( iter->second )].second++;
            std::get<2>( results[res.id] )++;
        }
    }

    for( const auto &lyr : layers ) {
        INFO( string_format( "layer %d - expected vs. actual", lyr.first ) );
        CHECK( lyr.second.first ==
               Approx( static_cast<float>( lyr.second.second ) / iters ).epsilon( 0.5 ) );
    }

    for( const auto &res : results ) {
        INFO( string_format( "monster %s - expected vs. actual", res.first.c_str() ) );
        CHECK( std::get<1>( res.second ) ==
               Approx( static_cast<float>( std::get<2>( res.second ) ) / iters ).epsilon( 0.6 ) );
    }
}

TEST_CASE( "Nested_monster_group_pack_size", "[mongroup]" )
{
    const int iters = 100;
    calendar::turn += 1_turns;

    SECTION( "Nested group pack size used as-is" ) {
        mongroup_id mg( "test_top_level_no_packsize" );
        for( int i = 0; i < iters; i++ ) {
            bool found = false;
            std::vector<MonsterGroupResult> res =
                MonsterGroupManager::GetResultFromGroup( mg, nullptr, &found );
            REQUIRE( found );

            // pack_size == [2, 4] * 1
            CHECK( res.size() == 1 );
            int total = 0;
            for( const MonsterGroupResult &mgr : res ) {
                total += mgr.pack_size;
            }
            CHECK( total == Approx( 3 ).margin( 1 ) );
        }
    }

    SECTION( "Nested group pack size multiplied by top level pack size" ) {
        mongroup_id mg( "test_top_level_packsize" );
        for( int i = 0; i < iters; i++ ) {
            bool found = false;
            std::vector<MonsterGroupResult> res =
                MonsterGroupManager::GetResultFromGroup( mg, nullptr, &found );
            REQUIRE( found );

            // pack_size == [2, 4] * [4, 6]
            CHECK( res.size() == Approx( 5 ).margin( 1 ) );
            int total = 0;
            for( const MonsterGroupResult &mgr : res ) {
                total += mgr.pack_size;
            }
            CHECK( total == Approx( 16 ).margin( 8 ) );
        }
    }
}

TEST_CASE( "mongroup_sets_quantity_correctly", "[mongroup]" )
{
    mongroup_id mg = GENERATE( GROUP_PET_DOGS, GROUP_PETS );
    CAPTURE( mg );

    int quantity = 10;
    bool found = false;
    std::vector<MonsterGroupResult> res =
        MonsterGroupManager::GetResultFromGroup( mg, &quantity, &found );
    CHECK( found );
    CHECK( 10 - quantity == static_cast<int>( res.size() ) );
}

static void test_multi_spawn( const mtype_id &old_mon, int range, int min, int max,
                              const time_point &start, const std::set<mtype_id> &new_mons,
                              std::map<mtype_id, int> &new_count )
{
    const int upgrade_attempts = 100;
    clear_avatar();
    // make sure tested scenarios haven't messed with our start time
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::start_of_game = calendar::turn_zero;

    for( int i = 0; i < upgrade_attempts; i++ ) {
        clear_map();
        map &m = get_map();
        calendar::turn = start;
        const tripoint_bub_ms ground_zero = get_player_character().pos_bub() - tripoint( 5, 5, 0 );

        monster *orig = g->place_critter_at( old_mon, ground_zero );
        REQUIRE( orig );
        REQUIRE( orig->type->id == old_mon );
        REQUIRE( orig->pos_bub() == ground_zero );
        REQUIRE( orig->can_upgrade() );

        // monster::next_upgrade_time has a ~3% chance to outright fail
        // so keep trying until we succeed
        orig->try_upgrade( false );
        if( orig->type->id == old_mon ) {
            continue;
        }

        REQUIRE( new_mons.count( orig->type->id ) > 0 );
        int total_spawns = 0;
        for( const Creature *c : m.get_creatures_in_radius( ground_zero, 10 ) ) {
            if( !c->is_monster() || new_mons.count( c->as_monster()->type->id ) == 0 ) {
                continue;
            }
            total_spawns++;
            CHECK( std::abs( c->pos_bub().x() - ground_zero.x() ) <= range );
            CHECK( std::abs( c->pos_bub().y() - ground_zero.y() ) <= range );
            if( new_count.count( c->as_monster()->type->id ) == 0 ) {
                new_count[c->as_monster()->type->id] = 0;
            }
            new_count[c->as_monster()->type->id]++;
        }
        CHECK( total_spawns == Approx( ( max + min ) / 2 ).margin( ( max - min ) / 2 ) );
    }
}

TEST_CASE( "mongroup_multi_spawn_upgrades", "[mongroup]" )
{
    std::map<mtype_id, int> counts;

    test_multi_spawn( mon_test_non_shearable, 5, 4, 6,
                      calendar::turn_zero + 1_days, { mon_test_shearable }, counts );

    CHECK( counts.size() == 1 );
    CHECK( counts.count( mon_test_shearable ) > 0 );
}

TEST_CASE( "mongroup_multi_spawn_restrictions", "[mongroup]" )
{
    const std::set<mtype_id> all_types { mon_test_shearable, mon_test_CBM, mon_test_zombie_cop };

    SECTION( "no valid spawns" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 4 * 3_days,
                          all_types, counts );

        CHECK( counts.empty() );
    }

    SECTION( "1 valid spawn, with DAY" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 4 * 3_days + 12_hours,
                          all_types, counts );

        CHECK( counts.size() == 1 );
        CHECK( counts.count( mon_test_zombie_cop ) > 0 );
    }

    SECTION( "1 valid spawn, with ends" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 1_days,
                          all_types, counts );

        CHECK( counts.size() == 1 );
        CHECK( counts.count( mon_test_CBM ) > 0 );
    }

    SECTION( "2 valid spawn, with ends and DAY" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 1_days + 12_hours,
                          all_types, counts );

        CHECK( counts.size() == 2 );
        CHECK( counts.count( mon_test_CBM ) > 0 );
        CHECK( counts.count( mon_test_zombie_cop ) > 0 );
    }

    SECTION( "1 valid spawn, with starts" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 4 * 8_days,
                          all_types, counts );

        CHECK( counts.size() == 1 );
        CHECK( counts.count( mon_test_shearable ) > 0 );
    }

    SECTION( "2 valid spawn, with starts and DAY" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_bovine, 5, 4, 6, calendar::turn_zero + 4 * 8_days + 12_hours,
                          all_types, counts );

        CHECK( counts.size() == 2 );
        CHECK( counts.count( mon_test_shearable ) > 0 );
        CHECK( counts.count( mon_test_zombie_cop ) > 0 );
    }

    SECTION( "no valid spawns, with mon_null" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_CBM, 5, 4, 6, calendar::turn_zero + 1_days,
        { mon_null, mon_test_CBM, mon_test_zombie_cop }, counts );

        CHECK( counts.empty() );
    }

    SECTION( "1 valid spawn, with mon_null" ) {
        std::map<mtype_id, int> counts;

        test_multi_spawn( mon_test_CBM, 5, 4, 6, calendar::turn_zero + 4 * 3_days + 12_hours,
        { mon_null, mon_test_CBM, mon_test_zombie_cop }, counts );

        CHECK( counts.size() == 1 );
        CHECK( counts.count( mon_test_zombie_cop ) > 0 );
    }
}
