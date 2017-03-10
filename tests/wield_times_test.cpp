#include "catch/catch.hpp"

#include "player.h"
#include "game.h"
#include "map.h"

#include <string>

// As macro, so that we can generate the test cases for easy copypasting
#define wield_check(dummy, the_item, expected_cost) wield_check_internal(dummy, the_item, #the_item, just_generating ? -1 : expected_cost)

void wield_check_internal( player &dummy, item &the_item, const std::string &var_name, int expected_cost )
{
    dummy.weapon = dummy.ret_null;
    dummy.set_moves( 1000 );
    int old_moves = dummy.moves;
    dummy.wield( the_item );
    int move_cost = old_moves - dummy.moves;
    if( expected_cost < 0 ) {
        printf( "        wield_time( dummy, %s, %d );\n", var_name.c_str(), move_cost );
    } else {
        int max_cost = move_cost * 1.1f;
        int min_cost = move_cost * 0.9f;
        CHECK( expected_cost <= max_cost );
        CHECK( expected_cost >= min_cost );
    }
}

void prepare_test()
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    // Prevent spilling, but don't cause encumbrance
    dummy.set_mutation( "DEBUG_STORAGE" );

    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
    g->m.ter_set( spot, ter_id( "t_dirt" ) );
    g->m.furn_set( spot, furn_id( "f_null" ) );
    g->m.i_clear( spot );
}

void do_test( bool just_generating )
{
    player &dummy = g->u;
    const tripoint spot = dummy.pos();

    SECTION("Wielding halberd from inventory while unencumbered.") {
        wield_check( dummy, dummy.i_add( item( "halberd" ) ), 0 );
    }
    SECTION("Wielding 1 aspirin from inventory while unencumbered.") {
        wield_check( dummy, dummy.i_add( item( "aspirin" ).split( 1 ) ), 0 );
    }
    SECTION("Wielding combat knife from inventory while unencumbered.") {
        wield_check( dummy, dummy.i_add( item( "knife_combat" ) ), 0 );
    }
    SECTION("Wielding metal tank from outside inventory while unencumbered.") {
        wield_check( dummy, g->m.add_item( spot, item( "metal_tank" ) ), 0 );
    }
}

TEST_CASE("Wield time test", "[wield]") {
    prepare_test();
    do_test( false );
}

TEST_CASE("Wield time make cases", "[.]") {
    prepare_test();
    do_test( true );
}
