#include <string>
#include <vector>

#include "cata_catch.h"
#include "cata_utility.h"
#include "item.h"
#include "mongroup.h"
#include "options.h"
#include "options_helpers.h"

static const mongroup_id GROUP_PETS( "GROUP_PETS" );
static const mongroup_id GROUP_PET_DOGS( "GROUP_PET_DOGS" );

static const mtype_id mon_null( "mon_null" );
static const mtype_id mon_test_CBM( "mon_test_CBM" );
static const mtype_id mon_test_bovine( "mon_test_bovine" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );
static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_speed_desc_base( "mon_test_speed_desc_base" );
static const mtype_id mon_test_speed_desc_base_immobile( "mon_test_speed_desc_base_immobile" );

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

        mtype_id tmp_res = MonsterGroupManager::GetResultFromGroup( grp ).front().name;
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

TEST_CASE( "Event-based monster spawns do not spawn outside event", "[monster][mongroup]" )
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

TEST_CASE( "Event-based monsters from an event-only mongroup", "[monster][mongroup]" )
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

TEST_CASE( "Using mon_null as mongroup default monster", "[mongroup]" )
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

TEST_CASE( "Nested monster groups spawn chance", "[mongroup]" )
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
        auto iter = results.find( res.name );
        CAPTURE( res.name.c_str() );
        REQUIRE( iter != results.end() );
        if( iter != results.end() ) {
            layers[std::get<0>( iter->second )].second++;
            std::get<2>( results[res.name] )++;
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
               Approx( static_cast<float>( std::get<2>( res.second ) ) / iters ).epsilon( 0.5 ) );
    }
}

TEST_CASE( "Nested monster group pack size", "[mongroup]" )
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
