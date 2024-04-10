#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "test_data.h"

static const furn_str_id furn_test_f_bash_persist( "test_f_bash_persist" );
static const furn_str_id furn_test_f_eoc( "test_f_eoc" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_test_t_bash_persist( "test_t_bash_persist" );
static const ter_str_id ter_test_t_pit_shallow( "test_t_pit_shallow" );

void bash_test_loadout::apply( Character &guy ) const
{
    clear_character( guy );

    guy.str_max = strength;
    guy.reset_stats();
    REQUIRE( guy.str_cur == strength );

    for( const itype_id &it : worn ) {
        REQUIRE( guy.wear_item( item( it ), false, false ).has_value() );
    }
    guy.calc_encumbrance();

    if( wielded.has_value() ) {
        item to_wield( wielded.value() );
        REQUIRE( guy.wield( to_wield ) );
    }

    REQUIRE( guy.smash_ability() == expected_smash_ability );
}

static void test_bash_set( const bash_test_set &set )
{
    map &here = get_map();
    Character &guy = get_player_character();
    // Arbitrary point on the map
    const tripoint_bub_ms test_pt( 40, 40, 0 );

    for( const single_bash_test &test : set.tests ) {
        test.loadout.apply( guy );

        constexpr int max_tries = 999;
        for( const furn_id &furn : set.tested_furn ) {
            INFO( string_format( "%s bashing %s", test.id, furn.id().str() ) );
            here.ter_set( test_pt, ter_t_floor );
            here.furn_set( test_pt, furn );
            int tries = 0;
            while( here.furn( test_pt ) == furn && tries < max_tries ) {
                ++tries;
                here.bash( test_pt.raw(), guy.smash_ability() );
            }
            auto it = test.furn_tries.find( furn );
            if( it == test.furn_tries.end() ) {
                CHECK( tries == max_tries );
            } else {
                CHECK( tries >= it->second.first );
                CHECK( tries <= it->second.second );
            }
        }
        for( const ter_id &ter : set.tested_ter ) {
            INFO( string_format( "%s bashing %s", test.id, ter.id().str() ) );
            here.ter_set( test_pt, ter );
            here.furn_set( test_pt, furn_str_id::NULL_ID() );
            int tries = 0;
            while( here.ter( test_pt ) == ter && tries < max_tries ) {
                ++tries;
                here.bash( test_pt.raw(), guy.smash_ability() );
            }
            auto it = test.ter_tries.find( ter );
            if( it == test.ter_tries.end() ) {
                CHECK( tries == max_tries );
            } else {
                CHECK( tries >= it->second.first );
                CHECK( tries <= it->second.second );
            }
        }
    }
}

TEST_CASE( "map_bash_chances", "[map][bash]" )
{
    clear_map();

    for( const bash_test_set &set : test_data::bash_tests ) {
        test_bash_set( set );
    }
}

TEST_CASE( "map_bash_ephemeral_persistence", "[map][bash]" )
{
    clear_map();
    map &here = get_map();

    const tripoint_bub_ms test_pt( 40, 40, 0 );

    // Assumptions
    REQUIRE( furn_test_f_bash_persist->bash.str_min == 4 );
    REQUIRE( furn_test_f_bash_persist->bash.str_max == 100 );
    REQUIRE( ter_test_t_bash_persist->bash.str_min == 4 );
    REQUIRE( ter_test_t_bash_persist->bash.str_max == 100 );

    SECTION( "bashing a furniture to completion leaves behind no map bash info" ) {
        here.furn_set( test_pt, furn_test_f_bash_persist );

        REQUIRE( here.furn( test_pt ) == furn_test_f_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt.raw(), 5 );
        // Does not destroy it
        CHECK( here.furn( test_pt ) == furn_test_f_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Bash it again to destroy it
        here.bash( test_pt.raw(), 999 );
        CHECK( here.furn( test_pt ) != furn_test_f_bash_persist );
        // Then, it is gone and there is no map damage
        CHECK( here.furn( test_pt ) != furn_test_f_bash_persist );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "bashing a terrain to completion leaves behind no map bash info" ) {
        here.ter_set( test_pt, ter_test_t_bash_persist );

        REQUIRE( here.ter( test_pt ) == ter_test_t_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt.raw(), 5 );
        // Does not destroy the terrain
        CHECK( here.ter( test_pt ) == ter_test_t_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Bash it again to destroy it
        here.bash( test_pt.raw(), 999 );
        CHECK( here.ter( test_pt ) != ter_test_t_bash_persist );
        // Then, it is gone and there is no map damage
        CHECK( here.ter( test_pt ) != ter_test_t_bash_persist );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "when a terrain changes, map damage is lost" ) {
        here.ter_set( test_pt, ter_test_t_bash_persist );

        REQUIRE( here.ter( test_pt ) == ter_test_t_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt.raw(), 5 );
        // Does not destroy the terrain
        CHECK( here.ter( test_pt ) == ter_test_t_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Change the terrain
        here.ter_set( test_pt, ter_test_t_pit_shallow );
        CHECK( here.ter( test_pt ) == ter_test_t_pit_shallow );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "when a furniture changes, map damage is lost" ) {
        here.furn_set( test_pt, furn_test_f_bash_persist );

        REQUIRE( here.furn( test_pt ) == furn_test_f_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt.raw(), 5 );
        // Does not destroy the terrain
        CHECK( here.furn( test_pt ) == furn_test_f_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Change the terrain
        here.furn_set( test_pt, furn_test_f_eoc );
        CHECK( here.furn( test_pt ) == furn_test_f_eoc );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
}
