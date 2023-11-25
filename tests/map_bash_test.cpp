#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "test_data.h"

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
            here.ter_set( test_pt, t_floor );
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
                // TODO: make bashing deterministic, this is just a "can bash" test right now
                // CHECK( tries >= it->second.first );
                // CHECK( tries <= it->second.second );
                CHECK( tries != max_tries );
            }
        }
        for( const ter_id &ter : set.tested_ter ) {
            INFO( string_format( "%s bashing %s", test.id, ter.id().str() ) );
            here.ter_set( test_pt, ter );
            here.furn_set( test_pt, f_null );
            int tries = 0;
            while( here.ter( test_pt ) == ter && tries < max_tries ) {
                ++tries;
                here.bash( test_pt.raw(), guy.smash_ability() );
            }
            auto it = test.ter_tries.find( ter );
            if( it == test.ter_tries.end() ) {
                CHECK( tries == max_tries );
            } else {
                // TODO: make bashing deterministic, this is just a "can bash" test right now
                // CHECK( tries >= it->second.first );
                // CHECK( tries <= it->second.second );
                CHECK( tries != max_tries );
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
